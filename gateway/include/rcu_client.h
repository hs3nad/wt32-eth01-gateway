#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "register_map.h"

struct RcuBusEvent {
  uint8_t frame[8];
  uint8_t inpbuf[RCU_INPBUF_COUNT];
  uint8_t outbuf[RCU_OUTBUF_COUNT];
  bool stateChanged;
  const char* reason;
};

enum RcuActionResult {
  RCU_ACTION_SENT,
  RCU_ACTION_ALREADY_APPLIED,
  RCU_ACTION_CACHE_NOT_READY,
  RCU_ACTION_UNSUPPORTED,
  RCU_ACTION_TX_FAILED,
};

void rcuBegin(void);
bool rcuPing(uint8_t aa, uint8_t floor);
bool rcuSnapshot(uint8_t aa, uint8_t floor, uint8_t inpbuf[RCU_INPBUF_COUNT], uint8_t outbuf[RCU_OUTBUF_COUNT]);
bool rcuReadBusEvent(RcuBusEvent* event);
bool rcuSetCachedOutputByte(uint8_t index, uint8_t value);
uint8_t rcuGetNodeId(void);
bool rcuSetNodeId(uint8_t nodeId);
bool rcuWriteOutputByte(uint8_t index, uint8_t value);
bool rcuWriteOutputBit(uint8_t index, uint8_t mask, bool enabled);
RcuActionResult rcuApplyAction(const char* action);
