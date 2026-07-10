#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "register_map.h"

struct RoomCache {
  const char* room;
  uint8_t aa;
  bool rcuOnline;
  uint8_t inpbuf[RCU_INPBUF_COUNT];
  uint8_t outbuf[RCU_OUTBUF_COUNT];
  uint8_t lastFrame[8];
  uint8_t lastEventId;
  uint32_t lastSeenMs;
};
