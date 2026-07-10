# Gateway Auto OTA

The gateway supports automatic HTTPS OTA updates after Ethernet receives an IP
address. The device downloads a small manifest from GitHub, compares the
manifest version with `GATEWAY_FW_VERSION`, and downloads/flashes the firmware
binary only when the manifest version is newer.

## Current Status

OTA has been tested on the real WT32-ETH01 gateway.

Confirmed path:

```text
gateway 0.4.1
  -> HTTPS GET GitHub raw manifest
  -> HTTPS download gateway-0.4.2.bin
  -> write ota_1 at 0x400000
  -> restart
  -> boot 0.4.2 from ota_1
```

Confirmed serial log:

```text
gateway-ota: Starting OTA update 0.4.1 -> 0.4.2
esp_https_ota: Writing to <ota_1> partition at offset 0x400000
gateway-ota: OTA update complete; restarting
boot: Loaded app from partition at offset 0x400000
gateway-ota: Firmware is current: local=0.4.2 available=0.4.2
```

The GitHub repository must be public, or the manifest and firmware binary must
be hosted somewhere public. Do not embed a GitHub token in the firmware.

## Configuration

Edit `gateway/include/config.h`:

```c
#define GATEWAY_FW_VERSION "0.4.2"
#define GATEWAY_OTA_ENABLED 1
#define GATEWAY_OTA_MANIFEST_URL "https://raw.githubusercontent.com/hs3nad/wt32-eth01-gateway/main/docs/gateway_ota.json"
#define GATEWAY_OTA_CHECK_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#define GATEWAY_OTA_FIRST_CHECK_DELAY_MS (30UL * 1000UL)
```

Set `GATEWAY_OTA_MANIFEST_URL` to a raw GitHub URL. When the URL is empty, the
OTA task starts but skips update checks.

Example:

```c
#define GATEWAY_OTA_MANIFEST_URL \
  "https://raw.githubusercontent.com/OWNER/REPO/main/docs/gateway_ota.json"
```

## Manifest

The manifest is intentionally small:

```json
{
  "version": "0.4.2",
  "firmware_url": "https://raw.githubusercontent.com/hs3nad/wt32-eth01-gateway/main/releases/gateway-0.4.2.bin"
}
```

`version` uses dotted numeric comparison. For example, `0.4.1` is newer than
`0.4.0`, and `0.5.0` is newer than `0.4.9`.

The tested firmware URL uses `raw.githubusercontent.com`. A GitHub Release asset
URL can redirect through larger response headers, so older firmware with the
default 512-byte ESP-IDF HTTP client buffer can fail with:

```text
HTTP_CLIENT: Out of buffer
```

The gateway now sets the ESP-IDF HTTP client receive buffer to 8192 bytes and
transmit buffer to 1024 bytes for both manifest and firmware downloads.

## Release Flow

1. Update `GATEWAY_FW_VERSION` in `gateway/include/config.h`.
2. Build the firmware.
3. Copy `gateway/build-wt32/gateway.bin` to `releases/gateway-X.Y.Z.bin`.
4. Update the manifest `version` and `firmware_url`.
5. Commit and push the firmware binary and manifest.
6. Wait for GitHub raw cache to update. The raw URL can cache for about 5
   minutes.
7. Devices check after boot and then every `GATEWAY_OTA_CHECK_INTERVAL_MS`.

Example:

```bash
cd gateway
./build.sh
cd ..
cp gateway/build-wt32/gateway.bin releases/gateway-0.4.3.bin
```

Then update `docs/gateway_ota.json`:

```json
{
  "version": "0.4.3",
  "firmware_url": "https://raw.githubusercontent.com/hs3nad/wt32-eth01-gateway/main/releases/gateway-0.4.3.bin"
}
```

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
- DNS/TLS/GitHub manifest access has been tested from the actual gateway.
- The gateway only updates when the manifest version is strictly newer than the
  local `GATEWAY_FW_VERSION`.
- If the OTA download or image validation fails, the device keeps running the
  current firmware.
- Room/node ID stored in NVS is not erased by OTA.
