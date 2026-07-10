#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "state_cache.h"

void modbusTcpSetBoardInfo(uint8_t boardId, uint8_t dipRaw, bool valid);
void modbusTcpSetNetworkOnline(bool online);
void modbusTcpSetRoomState(const RoomCache& room);
esp_err_t modbusTcpStart(void);
