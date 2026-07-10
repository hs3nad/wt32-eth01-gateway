#include "ota_update.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "config.h"

static const char* TAG = "gateway-ota";
static constexpr EventBits_t OTA_NETWORK_READY_BIT = BIT0;
static constexpr size_t OTA_MANIFEST_MAX_BYTES = 2048;
static constexpr int OTA_HTTP_TIMEOUT_MS = 10000;
static constexpr int OTA_HTTP_RX_BUFFER = 8192;
static constexpr int OTA_HTTP_TX_BUFFER = 1024;
static constexpr int OTA_TASK_STACK = 8192;

struct OtaManifest {
  char version[32];
  char firmwareUrl[256];
};

static EventGroupHandle_t otaEvents = nullptr;
static bool otaTaskStarted = false;
static bool otaInProgress = false;

// Compare dotted numeric versions like 0.4.0 and 0.4.1. Missing fields are
// treated as zero, so 0.4 and 0.4.0 compare equal.
static int compareVersions(const char* available, const char* current) {
  const char* a = available ? available : "";
  const char* b = current ? current : "";

  while (*a != '\0' || *b != '\0') {
    char* aEnd = nullptr;
    char* bEnd = nullptr;
    long aPart = strtol(a, &aEnd, 10);
    long bPart = strtol(b, &bEnd, 10);

    if (aEnd == a) aPart = 0;
    if (bEnd == b) bPart = 0;
    if (aPart > bPart) return 1;
    if (aPart < bPart) return -1;

    a = aEnd && *aEnd == '.' ? aEnd + 1 : (aEnd ? aEnd : a);
    b = bEnd && *bEnd == '.' ? bEnd + 1 : (bEnd ? bEnd : b);
    if ((aEnd == a || *a == '\0') && (bEnd == b || *b == '\0')) break;
  }

  return 0;
}

// Append downloaded bytes to the manifest buffer while keeping room for a final
// null terminator used by cJSON_Parse().
static bool appendManifestBytes(char* buffer, size_t* used, const char* data, int len) {
  if (len <= 0) return true;
  if (*used + (size_t)len >= OTA_MANIFEST_MAX_BYTES) return false;
  memcpy(buffer + *used, data, len);
  *used += len;
  buffer[*used] = '\0';
  return true;
}

// Download the small GitHub manifest into memory. The manifest must stay tiny;
// firmware itself is streamed later by esp_https_ota().
static esp_err_t fetchManifest(char* buffer, size_t bufferLen) {
  if (!buffer || bufferLen == 0) return ESP_ERR_INVALID_ARG;
  if (GATEWAY_OTA_MANIFEST_URL[0] == '\0') return ESP_ERR_INVALID_STATE;

  esp_http_client_config_t config = {};
  config.url = GATEWAY_OTA_MANIFEST_URL;
  config.timeout_ms = OTA_HTTP_TIMEOUT_MS;
  config.buffer_size = OTA_HTTP_RX_BUFFER;
  config.buffer_size_tx = OTA_HTTP_TX_BUFFER;
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.user_agent = "wt32-eth01-gateway/" GATEWAY_FW_VERSION;
  config.keep_alive_enable = true;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) return ESP_ERR_NO_MEM;

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    return err;
  }

  int contentLength = esp_http_client_fetch_headers(client);
  if (contentLength >= (int)bufferLen) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return ESP_ERR_INVALID_SIZE;
  }

  size_t used = 0;
  char chunk[256];
  while (true) {
    int read = esp_http_client_read(client, chunk, sizeof(chunk));
    if (read < 0) {
      err = ESP_FAIL;
      break;
    }
    if (read == 0) break;
    if (!appendManifestBytes(buffer, &used, chunk, read)) {
      err = ESP_ERR_INVALID_SIZE;
      break;
    }
  }

  int status = esp_http_client_get_status_code(client);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  if (err != ESP_OK) return err;
  return status == 200 ? ESP_OK : ESP_FAIL;
}

// Extract one simple JSON string field without bringing in an extra JSON
// component. The OTA manifest is intentionally tiny and controlled by us.
static bool extractJsonString(const char* json, const char* key, char* out, size_t outLen) {
  if (!json || !key || !out || outLen == 0) return false;
  char pattern[40];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);

  const char* pos = strstr(json, pattern);
  if (!pos) return false;
  pos = strchr(pos + strlen(pattern), ':');
  if (!pos) return false;
  pos++;
  while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;
  if (*pos != '"') return false;
  pos++;

  size_t used = 0;
  while (*pos != '\0' && *pos != '"') {
    if (*pos == '\\') return false;
    if (used + 1 >= outLen) return false;
    out[used++] = *pos++;
  }
  if (*pos != '"') return false;
  out[used] = '\0';
  return used > 0;
}

// Parse the update manifest. Expected JSON:
// {"version":"0.4.1","firmware_url":"https://.../gateway.bin"}
static bool parseManifest(const char* json, OtaManifest* manifest) {
  if (!json || !manifest) return false;
  return extractJsonString(json, "version", manifest->version, sizeof(manifest->version)) &&
         extractJsonString(json, "firmware_url", manifest->firmwareUrl, sizeof(manifest->firmwareUrl));
}

// Stream the new binary to the inactive OTA partition, set it as boot target,
// and restart. esp_https_ota validates the image header before committing.
static esp_err_t performOta(const OtaManifest& manifest) {
  ESP_LOGW(TAG, "Starting OTA update %s -> %s", GATEWAY_FW_VERSION, manifest.version);

  esp_http_client_config_t httpConfig = {};
  httpConfig.url = manifest.firmwareUrl;
  httpConfig.timeout_ms = OTA_HTTP_TIMEOUT_MS;
  httpConfig.buffer_size = OTA_HTTP_RX_BUFFER;
  httpConfig.buffer_size_tx = OTA_HTTP_TX_BUFFER;
  httpConfig.crt_bundle_attach = esp_crt_bundle_attach;
  httpConfig.user_agent = "wt32-eth01-gateway/" GATEWAY_FW_VERSION;
  httpConfig.keep_alive_enable = true;

  esp_https_ota_config_t otaConfig = {};
  otaConfig.http_config = &httpConfig;

  esp_err_t err = esp_https_ota(&otaConfig);
  if (err == ESP_OK) {
    ESP_LOGW(TAG, "OTA update complete; restarting");
    esp_restart();
  }

  ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(err));
  return err;
}

// One full update pass: download manifest, compare versions, then OTA only when
// the manifest advertises a strictly newer semantic version.
esp_err_t otaUpdateCheckNow(void) {
  if (!GATEWAY_OTA_ENABLED) return ESP_ERR_INVALID_STATE;
  if (GATEWAY_OTA_MANIFEST_URL[0] == '\0') {
    ESP_LOGI(TAG, "OTA manifest URL is empty; auto OTA disabled");
    return ESP_ERR_INVALID_STATE;
  }
  if (otaInProgress) return ESP_ERR_INVALID_STATE;

  otaInProgress = true;
  char* manifestJson = (char*)calloc(1, OTA_MANIFEST_MAX_BYTES);
  if (!manifestJson) {
    otaInProgress = false;
    return ESP_ERR_NO_MEM;
  }

  esp_err_t err = fetchManifest(manifestJson, OTA_MANIFEST_MAX_BYTES);
  if (err == ESP_OK) {
    OtaManifest manifest = {};
    if (!parseManifest(manifestJson, &manifest)) {
      err = ESP_ERR_INVALID_RESPONSE;
    } else if (compareVersions(manifest.version, GATEWAY_FW_VERSION) <= 0) {
      ESP_LOGI(TAG, "Firmware is current: local=%s available=%s", GATEWAY_FW_VERSION, manifest.version);
    } else {
      err = performOta(manifest);
    }
  }

  free(manifestJson);
  otaInProgress = false;
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGW(TAG, "OTA check failed: %s", esp_err_to_name(err));
  }
  return err;
}

// Background checker. It wakes after Ethernet receives an IP, waits a short
// grace period, checks once, then repeats at the configured interval.
static void otaUpdateTask(void* arg) {
  (void)arg;
  xEventGroupWaitBits(otaEvents, OTA_NETWORK_READY_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(GATEWAY_OTA_FIRST_CHECK_DELAY_MS));

  while (true) {
    otaUpdateCheckNow();
    vTaskDelay(pdMS_TO_TICKS(GATEWAY_OTA_CHECK_INTERVAL_MS));
  }
}

void otaUpdateNotifyNetworkReady(void) {
  if (otaEvents) xEventGroupSetBits(otaEvents, OTA_NETWORK_READY_BIT);
}

void otaUpdateStart(void) {
  if (!GATEWAY_OTA_ENABLED || otaTaskStarted) return;

  if (!otaEvents) otaEvents = xEventGroupCreate();
  if (!otaEvents) {
    ESP_LOGE(TAG, "Failed to create OTA event group");
    return;
  }

  BaseType_t created = xTaskCreate(otaUpdateTask, "ota_update", OTA_TASK_STACK, nullptr, 4, nullptr);
  if (created != pdPASS) {
    ESP_LOGE(TAG, "Failed to create OTA task");
    return;
  }
  otaTaskStarted = true;
}
