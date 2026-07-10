# Gateway Auto OTA

The gateway supports automatic HTTPS OTA updates after Ethernet receives an IP
address. The device downloads a small manifest from GitHub, compares the
manifest version with `GATEWAY_FW_VERSION`, and downloads/flashes the firmware
binary only when the manifest version is newer.

## Configuration

Edit `gateway/include/config.h`:

```c
#define GATEWAY_FW_VERSION "0.4.0"
#define GATEWAY_OTA_ENABLED 1
#define GATEWAY_OTA_MANIFEST_URL ""
#define GATEWAY_OTA_CHECK_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#define GATEWAY_OTA_FIRST_CHECK_DELAY_MS (30UL * 1000UL)
```

Set `GATEWAY_OTA_MANIFEST_URL` to a raw GitHub URL before production use. When
the URL is empty, the OTA task starts but skips update checks.

Example:

```c
#define GATEWAY_OTA_MANIFEST_URL \
  "https://raw.githubusercontent.com/OWNER/REPO/main/releases/gateway_ota.json"
```

## Manifest

The manifest is intentionally small:

```json
{
  "version": "0.4.1",
  "firmware_url": "https://github.com/OWNER/REPO/releases/download/v0.4.1/gateway.bin"
}
```

`version` uses dotted numeric comparison. For example, `0.4.1` is newer than
`0.4.0`, and `0.5.0` is newer than `0.4.9`.

## Release Flow

1. Update `GATEWAY_FW_VERSION` in `gateway/include/config.h`.
2. Build the firmware.
3. Upload `gateway/build-wt32/gateway.bin` to a GitHub release or another HTTPS
   URL trusted by the ESP-IDF certificate bundle.
4. Update the manifest `version` and `firmware_url`.
5. Devices check after boot and then every `GATEWAY_OTA_CHECK_INTERVAL_MS`.

## Partition Layout

OTA requires two app partitions, so the gateway uses:

```text
nvs      0x09000  0x06000
otadata  0x0F000  0x02000
phy_init 0x11000  0x01000
ota_0    0x20000  0x3E0000
ota_1    0x400000 0x3E0000
```

This is defined in `gateway/partitions_ota_8mb.csv` and matches the detected
8MB flash module. Because the partition table
changed from the earlier single-app layout, the first deployment of OTA firmware
must flash bootloader, partition table, OTA data, and app:

```bash
cd gateway
./build.sh
PORT=/dev/ttyUSB0 ./flash.sh
```

After that, later firmware updates can arrive through OTA.

## Safety Notes

- OTA uses HTTPS and the ESP-IDF certificate bundle.
- The gateway only updates when the manifest version is strictly newer than the
  local `GATEWAY_FW_VERSION`.
- If the OTA download or image validation fails, the device keeps running the
  current firmware.
- Room/node ID stored in NVS is not erased by OTA.
