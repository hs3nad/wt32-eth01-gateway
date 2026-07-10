#pragma once

#include "esp_err.h"

void otaUpdateStart(void);
void otaUpdateNotifyNetworkReady(void);
esp_err_t otaUpdateCheckNow(void);
