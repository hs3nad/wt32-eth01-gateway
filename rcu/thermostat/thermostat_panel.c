#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../mcc_generated_files/mcc.h"
#include "thermostat_panel.h"

#define THERMOSTAT_PANEL_FRAME_LENGTH        8u
#define THERMOSTAT_PANEL_TIMER_TICK_MS       10u
#define THERMOSTAT_PANEL_COMMAND_INTERVAL_MS 20u
#define THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS \
    (THERMOSTAT_PANEL_COMMAND_INTERVAL_MS / THERMOSTAT_PANEL_TIMER_TICK_MS)
#define THERMOSTAT_PANEL_RESPONSE_DELAY_MS   600u
#define THERMOSTAT_PANEL_RESPONSE_DELAY_TICKS \
    (THERMOSTAT_PANEL_RESPONSE_DELAY_MS / THERMOSTAT_PANEL_TIMER_TICK_MS)
#define THERMOSTAT_PANEL_FAN_TEMP_RESPONSE_DELAY_MS 320u
#define THERMOSTAT_PANEL_FAN_TEMP_RESPONSE_DELAY_TICKS \
    (THERMOSTAT_PANEL_FAN_TEMP_RESPONSE_DELAY_MS / THERMOSTAT_PANEL_TIMER_TICK_MS)
#define THERMOSTAT_PANEL_TX_WAIT_LIMIT       60000u

#define THERMOSTAT_PANEL_HEADER              0x7Eu
#define THERMOSTAT_PANEL_FRAME_TYPE          0x04u
#define THERMOSTAT_PANEL_FUNCTION_WRITE      0x08u

#define THERMOSTAT_PANEL_FAN_LOW             0x00u
#define THERMOSTAT_PANEL_FAN_MEDIUM          0x10u
#define THERMOSTAT_PANEL_FAN_HIGH            0x20u
#define THERMOSTAT_PANEL_FAN_AUTO            0x30u

typedef enum {
    THERMOSTAT_PANEL_POWER_FRAME_DEFAULT = 0u,
    THERMOSTAT_PANEL_POWER_FRAME_WORKING,
    THERMOSTAT_PANEL_POWER_FRAME_1C,
    THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH,
    THERMOSTAT_PANEL_POWER_FRAME_COOL_MEDIUM,
    THERMOSTAT_PANEL_POWER_FRAME_COOL_LOW,
    THERMOSTAT_PANEL_POWER_FRAME_COOL_AUTO
} THERMOSTAT_PANEL_POWER_FRAME_KIND;

typedef struct {
    uint8_t raw[THERMOSTAT_PANEL_FRAME_LENGTH];
    uint8_t count;
    THERMOSTAT_PANEL_STATE state;
    THERMOSTAT_PANEL_EVENT event;
    bool eventAvailable;
    uint8_t txFrame[THERMOSTAT_PANEL_FRAME_LENGTH];
    uint8_t txIndex;
    uint8_t cooldownTicks;
    uint8_t responseDelayTicks;
    uint8_t ambientTempByte;
    bool lastTempStepDown;
    bool fanSpeedChangeOnly;
    bool txActive;
    bool commandPending;
    bool pendingPower;
    THERMOSTAT_PANEL_POWER_FRAME_KIND powerFrameKind;
} THERMOSTAT_PANEL_RUNTIME;

static THERMOSTAT_PANEL_RUNTIME ThermostatPanelState;

static void ThermostatPanel_QueueFanTempCommand(uint8_t fanSpeed, uint8_t tempSetC);

static void ThermostatPanel_CopyFrame(uint8_t *dest, const uint8_t *src) {
    uint8_t i;

    for (i = 0u; i < THERMOSTAT_PANEL_FRAME_LENGTH; i++) {
        dest[i] = src[i];
    }
}

static void ThermostatPanel_ShiftLeft(void) {
    uint8_t i;

    for (i = 1u; i < THERMOSTAT_PANEL_FRAME_LENGTH; i++) {
        ThermostatPanelState.raw[i - 1u] = ThermostatPanelState.raw[i];
    }

    ThermostatPanelState.count = (uint8_t)(THERMOSTAT_PANEL_FRAME_LENGTH - 1u);
}

static bool ThermostatPanel_IsRxHeader(void) {
    return ((ThermostatPanelState.raw[0] == 0x7Fu)
         && (ThermostatPanelState.raw[1] == 0x05u)
         && (ThermostatPanelState.raw[2] == THERMOSTAT_PANEL_DEFAULT_ADDRESS));
}

static bool ThermostatPanel_MatchesPowerKey(uint8_t data4,
                                            uint8_t data5,
                                            uint8_t data6,
                                            uint8_t data7) {
    return ((ThermostatPanelState.raw[4] == data4)
         && (ThermostatPanelState.raw[5] == data5)
         && (ThermostatPanelState.raw[6] == data6)
         && (ThermostatPanelState.raw[7] == data7));
}

static void ThermostatPanel_PublishPowerEventWithMode(bool power, uint8_t mode) {
    (void)mode;

    ThermostatPanelState.state.power = power;
    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;

    ThermostatPanelState.event.address = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    ThermostatPanelState.event.function = 0x01u;
    ThermostatPanelState.event.registerAddress = 0u;
    ThermostatPanelState.event.value = power ? 1u : 0u;
    ThermostatPanelState.event.type = THERMOSTAT_PANEL_EVENT_POWER;
    ThermostatPanelState.event.state = ThermostatPanelState.state;
    ThermostatPanelState.eventAvailable = true;
    ThermostatPanelState.responseDelayTicks = THERMOSTAT_PANEL_RESPONSE_DELAY_TICKS;
}

static void ThermostatPanel_PublishPowerEvent(bool power) {
    ThermostatPanel_PublishPowerEventWithMode(power, THERMOSTAT_PANEL_MODE_COOL);
}

static void ThermostatPanel_SelectCoolPowerFrameKind(uint8_t fanSpeed) {
    fanSpeed = (uint8_t)(fanSpeed & THERMOSTAT_PANEL_FAN_AUTO);

    if (fanSpeed == THERMOSTAT_PANEL_FAN_AUTO) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_AUTO;
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_LOW) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_LOW;
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_MEDIUM) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_MEDIUM;
    } else {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH;
    }
}

static void ThermostatPanel_UpdateAmbientTempByte(void) {
    if ((ThermostatPanelState.count >= THERMOSTAT_PANEL_FRAME_LENGTH)
     && ThermostatPanel_IsRxHeader()
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x20u)) {
        ThermostatPanelState.ambientTempByte = ThermostatPanelState.raw[6];
        ThermostatPanelState.state.roomTempByte = ThermostatPanelState.raw[6];
    }
}

static void ThermostatPanel_PublishRoomTempEvent(void) {
    ThermostatPanelState.event.address = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    ThermostatPanelState.event.function = 0x01u;
    ThermostatPanelState.event.registerAddress = 0u;
    ThermostatPanelState.event.value = ThermostatPanelState.state.roomTempByte;
    ThermostatPanelState.event.type = THERMOSTAT_PANEL_EVENT_ROOM_TEMP;
    ThermostatPanelState.event.state = ThermostatPanelState.state;
    ThermostatPanelState.eventAvailable = true;
    ThermostatPanelState.responseDelayTicks = 0u;
}

static void ThermostatPanel_PublishFanTempEvent(uint8_t fanSpeed, uint8_t tempSetC) {
    ThermostatPanelState.state.power = true;
    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
    ThermostatPanelState.state.fanSpeed = fanSpeed;
    ThermostatPanelState.state.tempSetC = tempSetC;
    ThermostatPanelState.state.roomTempByte = ThermostatPanelState.ambientTempByte;

    ThermostatPanelState.event.address = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    ThermostatPanelState.event.function = 0x01u;
    ThermostatPanelState.event.registerAddress = 0u;
    ThermostatPanelState.event.value = (uint16_t)(fanSpeed | ((uint16_t)tempSetC << 8));
    ThermostatPanelState.event.type = THERMOSTAT_PANEL_EVENT_FAN_SPEED;
    ThermostatPanelState.event.state = ThermostatPanelState.state;
    ThermostatPanelState.eventAvailable = true;
    ThermostatPanelState.responseDelayTicks = THERMOSTAT_PANEL_FAN_TEMP_RESPONSE_DELAY_TICKS;
}

static bool ThermostatPanel_TryParseTempStepFrame(void) {
    uint8_t fanSpeed;
    uint8_t tempSetC;

    if ((ThermostatPanelState.count < THERMOSTAT_PANEL_FRAME_LENGTH)
     || !ThermostatPanel_IsRxHeader()
     || (ThermostatPanelState.raw[5] < 0x10u)
     || (ThermostatPanelState.raw[5] > 0x1Fu)) {
        return false;
    }

    if ((ThermostatPanelState.raw[3] == 0x02u)
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x20u)) {
        if (ThermostatPanelState.raw[4] == 0xF3u) {
            fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
        } else if (ThermostatPanelState.raw[4] == 0xD3u) {
            fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
        } else if (ThermostatPanelState.raw[4] == 0xE3u) {
            fanSpeed = THERMOSTAT_PANEL_FAN_MEDIUM;
        } else if ((ThermostatPanelState.raw[4] == 0xCBu)
                || (ThermostatPanelState.raw[4] == 0xDBu)
                || (ThermostatPanelState.raw[4] == 0xEBu)
                || (ThermostatPanelState.raw[4] == 0xFBu)) {
            fanSpeed = THERMOSTAT_PANEL_FAN_AUTO;
        } else {
            return false;
        }
        tempSetC = (uint8_t)(ThermostatPanelState.raw[5] + 1u);
        ThermostatPanelState.lastTempStepDown = false;
    } else if (ThermostatPanelState.raw[3] == 0x03u) {
        if (ThermostatPanelState.raw[4] == 0xF3u) {
            fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
        } else if (ThermostatPanelState.raw[4] == 0xD3u) {
            fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
        } else if (ThermostatPanelState.raw[4] == 0xE3u) {
            fanSpeed = THERMOSTAT_PANEL_FAN_MEDIUM;
        } else if ((ThermostatPanelState.raw[4] == 0xCBu)
                || (ThermostatPanelState.raw[4] == 0xDBu)
                || (ThermostatPanelState.raw[4] == 0xEBu)
                || (ThermostatPanelState.raw[4] == 0xFBu)) {
            fanSpeed = THERMOSTAT_PANEL_FAN_AUTO;
        } else {
            return false;
        }
        tempSetC = (uint8_t)(ThermostatPanelState.raw[5] - 1u);
        ThermostatPanelState.lastTempStepDown = true;
    } else {
        return false;
    }

    if (tempSetC > 31u) {
        tempSetC = 31u;
    } else if (tempSetC < 16u) {
        tempSetC = 16u;
    }

    ThermostatPanel_PublishFanTempEvent(fanSpeed, tempSetC);
    ThermostatPanel_QueueFanTempCommand(fanSpeed, tempSetC);
    ThermostatPanelState.count = 0u;
    return true;
}

static bool ThermostatPanel_TryParseFanSpeedFrame(void) {
    uint8_t fanSpeed;

    if ((ThermostatPanelState.count < THERMOSTAT_PANEL_FRAME_LENGTH)
     || !ThermostatPanel_IsRxHeader()
     || (ThermostatPanelState.raw[3] != 0x04u)
     || (ThermostatPanelState.raw[5] < 0x10u)
     || (ThermostatPanelState.raw[5] > 0x1Fu)
     || (ThermostatPanelState.raw[6] < 0x1Bu)
     || (ThermostatPanelState.raw[6] > 0x20u)) {
        return false;
    }

    if ((ThermostatPanelState.raw[4] == 0xDBu)
     || (ThermostatPanelState.raw[4] == 0xEBu)
     || (ThermostatPanelState.raw[4] == 0xFBu)) {
        fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
    } else if (ThermostatPanelState.raw[4] == 0xD3u) {
        fanSpeed = THERMOSTAT_PANEL_FAN_MEDIUM;
    } else if (ThermostatPanelState.raw[4] == 0xE3u) {
        fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
    } else if (ThermostatPanelState.raw[4] == 0xF3u) {
        fanSpeed = THERMOSTAT_PANEL_FAN_AUTO;
    } else {
        ThermostatPanelState.count = 0u;
        return false;
    }

    ThermostatPanelState.lastTempStepDown = false;
    ThermostatPanelState.fanSpeedChangeOnly = true;
    ThermostatPanel_PublishFanTempEvent(fanSpeed, ThermostatPanelState.raw[5]);
    ThermostatPanel_QueueFanTempCommand(fanSpeed, ThermostatPanelState.raw[5]);
    ThermostatPanelState.count = 0u;
    return true;
}

static bool ThermostatPanel_TryParsePowerFrame(void) {
    if ((ThermostatPanelState.count < THERMOSTAT_PANEL_FRAME_LENGTH)
     || !ThermostatPanel_IsRxHeader()) {
        return false;
    }

    if (ThermostatPanelState.raw[3] == 0x00u) {
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_MatchesPowerKey(0xF3u, 0x19u, 0x1Cu, 0x5Du)
     && (ThermostatPanelState.raw[3] == 0x06u)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanelState.raw[3] != 0x01u) {
        ThermostatPanelState.count = 0u;
        return false;
    }

    if (ThermostatPanel_MatchesPowerKey(0x56u, 0x18u, 0x1Fu, 0x51u)
     || ThermostatPanel_MatchesPowerKey(0xD2u, 0x18u, 0x1Au, 0x55u)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_WORKING;
        ThermostatPanel_PublishPowerEvent(true);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_MatchesPowerKey(0xD2u, 0x18u, 0x1Bu, 0x56u)
     || ThermostatPanel_MatchesPowerKey(0xD2u, 0x18u, 0x1Cu, 0x58u)) {
        ThermostatPanel_SelectCoolPowerFrameKind(ThermostatPanelState.state.fanSpeed);
        ThermostatPanel_PublishPowerEvent(true);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0xD2u)
     && (ThermostatPanelState.raw[5] == 0x18u)
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x20u)) {
        ThermostatPanel_SelectCoolPowerFrameKind(ThermostatPanelState.state.fanSpeed);
        ThermostatPanel_PublishPowerEvent(true);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_MatchesPowerKey(0xF7u, 0x19u, 0x1Fu, 0x5Eu)
     || ThermostatPanel_MatchesPowerKey(0x77u, 0x19u, 0x1Fu, 0x55u)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_WORKING;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_MatchesPowerKey(0x73u, 0x19u, 0x1Eu, 0x52u)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_DEFAULT;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0xE3u)
     && (ThermostatPanelState.raw[5] >= 0x10u)
     && (ThermostatPanelState.raw[5] <= 0x1Fu)
     && ((ThermostatPanelState.raw[6] == 0x1Du)
      || (ThermostatPanelState.raw[6] == 0x1Eu))) {
        ThermostatPanelState.state.fanSpeed = THERMOSTAT_PANEL_FAN_MEDIUM;
        ThermostatPanelState.state.tempSetC = ThermostatPanelState.raw[5];
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_MEDIUM;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0xD3u)
     && (ThermostatPanelState.raw[5] >= 0x10u)
     && (ThermostatPanelState.raw[5] <= 0x1Fu)
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x20u)) {
        ThermostatPanelState.state.fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
        ThermostatPanelState.state.tempSetC = ThermostatPanelState.raw[5];
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_LOW;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if ((ThermostatPanelState.raw[3] == 0x01u)
     && ((ThermostatPanelState.raw[4] == 0xCBu)
      || (ThermostatPanelState.raw[4] == 0xDBu)
      || (ThermostatPanelState.raw[4] == 0xEBu)
      || (ThermostatPanelState.raw[4] == 0xFBu))
     && (ThermostatPanelState.raw[5] >= 0x10u)
     && (ThermostatPanelState.raw[5] <= 0x1Fu)
     && (ThermostatPanelState.raw[6] == 0x1Eu)) {
        ThermostatPanelState.state.fanSpeed = THERMOSTAT_PANEL_FAN_AUTO;
        ThermostatPanelState.state.tempSetC = ThermostatPanelState.raw[5];
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_AUTO;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if ((ThermostatPanelState.raw[3] == 0x01u)
     && (ThermostatPanelState.raw[4] == 0xF3u)
     && (ThermostatPanelState.raw[5] >= 0x10u)
     && (ThermostatPanelState.raw[5] <= 0x1Fu)
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x20u)) {
        ThermostatPanelState.state.fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
        ThermostatPanelState.state.tempSetC = ThermostatPanelState.raw[5];
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_MatchesPowerKey(0xF3u, 0x19u, 0x1Bu, 0x5Au)
     || ThermostatPanel_MatchesPowerKey(0xF3u, 0x19u, 0x1Cu, 0x5Au)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0xF3u)
     && (ThermostatPanelState.raw[5] == 0x19u)
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x1Du)
     && (ThermostatPanelState.raw[7] == 0x5Au)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_MatchesPowerKey(0x73u, 0x19u, 0x1Cu, 0x51u)
     || ThermostatPanel_MatchesPowerKey(0xF7u, 0x19u, 0x1Au, 0x5Cu)
     || ThermostatPanel_MatchesPowerKey(0xF7u, 0x19u, 0x1Bu, 0x5Bu)
     || ThermostatPanel_MatchesPowerKey(0x73u, 0x19u, 0x1Bu, 0x50u)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_1C;
        ThermostatPanel_PublishPowerEvent(false);
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (((ThermostatPanelState.raw[4] == 0xD2u)
      || (ThermostatPanelState.raw[4] == 0xD3u))
     && ((ThermostatPanelState.raw[5] == 0x18u)
      || (ThermostatPanelState.raw[5] == 0x19u))
     && (ThermostatPanelState.raw[6] >= 0x1Bu)
     && (ThermostatPanelState.raw[6] <= 0x20u)) {
        ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_DEFAULT;
        ThermostatPanel_PublishPowerEvent(!ThermostatPanelState.state.power);
        ThermostatPanelState.count = 0u;
        return true;
    }

    ThermostatPanelState.count = 0u;
    return false;
}

static bool ThermostatPanel_CopyFanTempFrame(uint8_t *frame, uint8_t fanSpeed, uint8_t tempSetC);

static void ThermostatPanel_CopyPowerOffCoolHighFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x2Au, 0x2Cu, 0x2Cu, 0x2Cu,
        0x2Du, 0x2Eu, 0x2Du, 0x2Fu,
        0x30u, 0x30u, 0x30u, 0x32u,
        0x31u, 0x32u, 0x33u, 0x33u
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[4] = 0x32u;
    frame[5] = tempSetC;
    frame[6] = 0x1Eu;
    frame[7] = data7[index];
}

static void ThermostatPanel_CopyPowerOnCoolHighFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x74u, 0x75u, 0x76u, 0x75u,
        0x77u, 0x78u, 0x78u, 0x78u,
        0x7Au, 0x79u, 0x7Au, 0x7Bu,
        0x7Bu, 0x7Cu, 0x33u, 0x34u
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[4] = (tempSetC >= 30u) ? 0x33u : 0xB3u;
    frame[5] = tempSetC;
    frame[6] = 0x1Eu;
    frame[7] = data7[index];
}

static void ThermostatPanel_CopyPowerOffCoolMediumFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x21u, 0x22u, 0x22u, 0x23u,
        0x24u, 0x23u, 0x25u, 0x25u,
        0x25u, 0x26u, 0x28u, 0x28u,
        0x29u, 0x29u, 0x29u, 0x2Bu
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[4] = 0x22u;
    frame[5] = tempSetC;
    frame[6] = (tempSetC >= 27u) ? 0x1Eu : 0x1Du;
    frame[7] = data7[index];
}

static void ThermostatPanel_CopyPowerOnCoolMediumFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x6Bu, 0x6Cu, 0x6Bu, 0x6Du,
        0x6Du, 0x6Du, 0x6Eu, 0x70u,
        0x6Fu, 0x70u, 0x71u, 0x71u,
        0x73u, 0x72u, 0x2Bu, 0x2Au
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[4] = (tempSetC >= 30u) ? 0x23u : 0xA3u;
    frame[5] = tempSetC;
    frame[6] = (tempSetC >= 27u) ? 0x1Eu : 0x1Du;
    frame[7] = data7[index];
}

static void ThermostatPanel_CopyPowerOffCoolLowFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x18u, 0x18u, 0x1Au, 0x19u,
        0x1Au, 0x1Bu, 0x1Bu, 0x1Cu,
        0x1Du, 0x1Cu, 0x1Eu, 0x1Eu,
        0x1Fu, 0x1Fu, 0x21u, 0x20u
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[4] = 0x12u;
    frame[5] = tempSetC;
    frame[6] = 0x1Du;
    frame[7] = data7[index];
}

static void ThermostatPanel_CopyPowerOnCoolLowFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x61u, 0x62u, 0x63u, 0x63u,
        0x64u, 0x65u, 0x64u, 0x66u,
        0x66u, 0x67u, 0x67u, 0x69u,
        0x68u, 0x21u, 0x20u, 0x21u
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[4] = (tempSetC >= 29u) ? 0x13u : 0x93u;
    frame[5] = tempSetC;
    frame[6] = 0x1Du;
    frame[7] = data7[index];
}

static void ThermostatPanel_CopyPowerOffCoolAutoFrame(uint8_t *frame, uint8_t tempSetC) {
    static const uint8_t data7[] = {
        0x30u, 0x30u, 0x30u, 0x32u,
        0x31u, 0x32u, 0x33u, 0x33u,
        0x34u, 0x35u, 0x2Cu, 0x2Cu,
        0x23u, 0x25u, 0x1Cu, 0x1Du
    };
    uint8_t index;

    if (frame == NULL) {
        return;
    }

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[5] = tempSetC;
    frame[6] = 0x1Eu;
    frame[7] = data7[index];

    if (tempSetC >= 30u) {
        frame[4] = 0x0Au;
    } else if (tempSetC >= 28u) {
        frame[4] = 0x1Au;
    } else if (tempSetC >= 26u) {
        frame[4] = 0x2Au;
    } else {
        frame[4] = 0x3Au;
    }
}

static void ThermostatPanel_QueuePowerCommand(bool power) {
    static const uint8_t powerOnFrame[THERMOSTAT_PANEL_FRAME_LENGTH] =
        {0x7Eu, 0x04u, 0x3Cu, 0x08u, 0x37u, 0x19u, 0x1Fu, 0x33u};
    static const uint8_t powerOffDefaultFrame[THERMOSTAT_PANEL_FRAME_LENGTH] =
        {0x7Eu, 0x04u, 0x3Cu, 0x08u, 0x32u, 0x19u, 0x1Eu, 0x30u};
    static const uint8_t powerOffWorkingFrame[THERMOSTAT_PANEL_FRAME_LENGTH] =
        {0x7Eu, 0x04u, 0x3Cu, 0x08u, 0x36u, 0x19u, 0x1Fu, 0x33u};
    static const uint8_t powerOff1CFrame[THERMOSTAT_PANEL_FRAME_LENGTH] =
        {0x7Eu, 0x04u, 0x3Cu, 0x08u, 0x2Au, 0x19u, 0x1Cu, 0x2Bu};

    if (power && (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH)) {
        ThermostatPanel_CopyPowerOnCoolHighFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (power && (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_MEDIUM)) {
        ThermostatPanel_CopyPowerOnCoolMediumFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (power && (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_LOW)) {
        ThermostatPanel_CopyPowerOnCoolLowFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (power && (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_AUTO)) {
        if (!ThermostatPanel_CopyFanTempFrame(
            ThermostatPanelState.txFrame,
            THERMOSTAT_PANEL_FAN_AUTO,
            ThermostatPanelState.state.tempSetC
        )) {
            ThermostatPanel_CopyFrame(ThermostatPanelState.txFrame, powerOnFrame);
        }
    } else if (power) {
        ThermostatPanel_CopyFrame(ThermostatPanelState.txFrame, powerOnFrame);
    } else if (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_HIGH) {
        ThermostatPanel_CopyPowerOffCoolHighFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_MEDIUM) {
        ThermostatPanel_CopyPowerOffCoolMediumFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_LOW) {
        ThermostatPanel_CopyPowerOffCoolLowFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_COOL_AUTO) {
        ThermostatPanel_CopyPowerOffCoolAutoFrame(
            ThermostatPanelState.txFrame,
            ThermostatPanelState.state.tempSetC
        );
    } else if (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_WORKING) {
        ThermostatPanel_CopyFrame(ThermostatPanelState.txFrame, powerOffWorkingFrame);
    } else if (ThermostatPanelState.powerFrameKind == THERMOSTAT_PANEL_POWER_FRAME_1C) {
        ThermostatPanel_CopyFrame(ThermostatPanelState.txFrame, powerOff1CFrame);
    } else {
        ThermostatPanel_CopyFrame(ThermostatPanelState.txFrame, powerOffDefaultFrame);
    }

    ThermostatPanelState.pendingPower = power;
    ThermostatPanelState.commandPending = true;
}

static bool ThermostatPanel_CopyFanTempFrame(uint8_t *frame, uint8_t fanSpeed, uint8_t tempSetC) {
    static const uint8_t fanHighData7[] = {
        0x74u, 0x75u, 0x76u, 0x75u,
        0x77u, 0x78u, 0x78u, 0x78u,
        0x7Au, 0x79u, 0x7Au, 0x7Bu,
        0x7Bu, 0x7Cu, 0x7Du, 0x34u
    };
    static const uint8_t fanHighChangeData7[] = {
        0x74u, 0x75u, 0x75u, 0x77u,
        0x78u, 0x78u, 0x78u, 0x7Au,
        0x79u, 0x7Au, 0x7Bu, 0x7Bu,
        0x7Cu, 0x7Du, 0x7Cu, 0x7Eu
    };
    static const uint8_t fanMediumData7[] = {
        0x6Bu, 0x6Cu, 0x6Bu, 0x6Du,
        0x6Du, 0x6Du, 0x6Eu, 0x70u,
        0x6Fu, 0x70u, 0x71u, 0x71u,
        0x73u, 0x72u, 0x74u, 0x2Au
    };
    static const uint8_t fanMediumChangeData7[] = {
        0x6Cu, 0x6Bu, 0x6Du, 0x6Du,
        0x6Eu, 0x70u, 0x6Fu, 0x70u,
        0x71u, 0x71u, 0x71u, 0x73u,
        0x72u, 0x74u, 0x74u, 0x74u
    };
    static const uint8_t fanLowData7[] = {
        0x61u, 0x62u, 0x63u, 0x63u,
        0x64u, 0x65u, 0x64u, 0x66u,
        0x66u, 0x67u, 0x67u, 0x69u,
        0x68u, 0x69u, 0x20u, 0x21u
    };
    static const uint8_t fanLowChangeData7[] = {
        0x62u, 0x63u, 0x64u, 0x65u,
        0x64u, 0x66u, 0x66u, 0x67u,
        0x67u, 0x69u, 0x68u, 0x69u,
        0x6Au, 0x6Au, 0x6Bu, 0x6Cu
    };
    static const uint8_t fanAutoData7[] = {
        0x7Au, 0x79u, 0x7Au, 0x7Bu,
        0x7Bu, 0x7Cu, 0x7Du, 0x7Cu,
        0x7Eu, 0x7Eu, 0x76u, 0x75u,
        0x6Du, 0x6Eu, 0x1Du, 0x1Cu
    };
    static const uint8_t fanAutoChangeData7[] = {
        0x7Au, 0x79u, 0x7Bu, 0x7Bu,
        0x7Cu, 0x7Du, 0x7Cu, 0x7Eu,
        0x7Eu, 0x7Fu, 0x7Fu, 0x77u,
        0x78u, 0x70u, 0x6Fu, 0x70u
    };
    uint8_t index;

    if ((frame == NULL) || (tempSetC < 16u) || (tempSetC > 31u)) {
        return false;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    frame[5] = tempSetC;

    if (fanSpeed == THERMOSTAT_PANEL_FAN_HIGH) {
        if (tempSetC == 31u) {
            frame[4] = 0x33u;
        } else {
            frame[4] = 0xB3u;
        }
        frame[6] = 0x1Eu;
        frame[7] = fanHighData7[index];
        if (ThermostatPanelState.fanSpeedChangeOnly) {
            frame[4] = 0xB3u;
            frame[6] = ThermostatPanelState.ambientTempByte;
            frame[7] = fanHighChangeData7[index];
            if ((tempSetC == 24u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x7Au;
            }
            if ((tempSetC == 25u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x79u;
            }
            if ((tempSetC == 26u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x7Au;
            }
        }
        if ((tempSetC == 29u) && ThermostatPanelState.lastTempStepDown) {
            frame[4] = 0x33u;
            frame[6] = 0x1Du;
            frame[7] = 0x32u;
        }
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_MEDIUM) {
        frame[4] = (tempSetC == 31u) ? 0x23u : 0xA3u;
        frame[6] = (tempSetC >= 27u) ? 0x1Eu : 0x1Du;
        frame[7] = fanMediumData7[index];
        if (ThermostatPanelState.fanSpeedChangeOnly) {
            frame[4] = 0xA3u;
            frame[6] = ThermostatPanelState.ambientTempByte;
            frame[7] = fanMediumChangeData7[index];
        }
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_LOW) {
        frame[4] = (tempSetC >= 30u) ? 0x13u : 0x93u;
        frame[6] = 0x1Du;
        frame[7] = fanLowData7[index];
        if (ThermostatPanelState.fanSpeedChangeOnly) {
            frame[4] = 0x93u;
            frame[6] = ThermostatPanelState.ambientTempByte;
            frame[7] = fanLowChangeData7[index];
            if ((tempSetC == 25u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x67u;
            }
        }
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_AUTO) {
        if (tempSetC >= 30u) {
            frame[4] = 0x0Bu;
        } else if (tempSetC >= 28u) {
            frame[4] = 0x9Bu;
        } else if (tempSetC >= 26u) {
            frame[4] = 0xABu;
        } else {
            frame[4] = 0xBBu;
        }
        frame[6] = 0x1Eu;
        frame[7] = fanAutoData7[index];
        if (ThermostatPanelState.fanSpeedChangeOnly) {
            if (tempSetC >= 29u) {
                frame[4] = 0x9Bu;
            }
            frame[6] = ThermostatPanelState.ambientTempByte;
            frame[7] = fanAutoChangeData7[index];
            if ((tempSetC == 24u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x7Eu;
            }
            if ((tempSetC == 25u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x7Eu;
            }
            if ((tempSetC == 26u) && (ThermostatPanelState.ambientTempByte == 0x1Eu)) {
                frame[7] = 0x7Eu;
            }
        }
    } else {
        return false;
    }

    return true;
}

static void ThermostatPanel_QueueFanTempCommand(uint8_t fanSpeed, uint8_t tempSetC) {
    bool tempStepDown;

    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    fanSpeed = (uint8_t)(fanSpeed & THERMOSTAT_PANEL_FAN_AUTO);
    tempStepDown = (tempSetC < ThermostatPanelState.state.tempSetC);
    ThermostatPanelState.lastTempStepDown = tempStepDown;

    if (ThermostatPanel_CopyFanTempFrame(ThermostatPanelState.txFrame, fanSpeed, tempSetC)) {
        ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
        ThermostatPanelState.state.fanSpeed = fanSpeed;
        ThermostatPanelState.state.tempSetC = tempSetC;
        ThermostatPanel_SelectCoolPowerFrameKind(fanSpeed);
        ThermostatPanelState.pendingPower = true;
        ThermostatPanelState.commandPending = true;
    }
    ThermostatPanelState.fanSpeedChangeOnly = false;
}

static void ThermostatPanel_ProcessTx(void) {
    uint16_t waitCount;

    if (!ThermostatPanelState.txActive) {
        return;
    }

    while (ThermostatPanelState.txIndex < THERMOSTAT_PANEL_FRAME_LENGTH) {
        waitCount = 0u;
        while (!EUSART_is_tx_ready()) {
            waitCount++;
            if (waitCount >= THERMOSTAT_PANEL_TX_WAIT_LIMIT) {
                DIR_SetLow();
                ThermostatPanelState.txActive = false;
                ThermostatPanelState.cooldownTicks = THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS;
                return;
            }
        }
        TX1REG = ThermostatPanelState.txFrame[ThermostatPanelState.txIndex++];
    }

    waitCount = 0u;
    while (!EUSART_is_tx_done()) {
        waitCount++;
        if (waitCount >= THERMOSTAT_PANEL_TX_WAIT_LIMIT) {
            DIR_SetLow();
            ThermostatPanelState.txActive = false;
            ThermostatPanelState.cooldownTicks = THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS;
            return;
        }
    }

    DIR_SetLow();
    ThermostatPanelState.state.power = ThermostatPanelState.pendingPower;
    ThermostatPanelState.txActive = false;
    ThermostatPanelState.cooldownTicks = THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS;
}

void ThermostatPanel_Init(void) {
    uint8_t i;

    for (i = 0u; i < THERMOSTAT_PANEL_FRAME_LENGTH; i++) {
        ThermostatPanelState.raw[i] = 0u;
        ThermostatPanelState.txFrame[i] = 0u;
    }

    ThermostatPanelState.count = 0u;
    ThermostatPanelState.state.power = false;
    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
    ThermostatPanelState.state.fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
    ThermostatPanelState.state.tempSetC = 25u;
    ThermostatPanelState.state.roomTempByte = 0x1Eu;
    ThermostatPanelState.eventAvailable = false;
    ThermostatPanelState.txIndex = 0u;
    ThermostatPanelState.cooldownTicks = 0u;
    ThermostatPanelState.responseDelayTicks = 0u;
    ThermostatPanelState.ambientTempByte = 0x1Eu;
    ThermostatPanelState.lastTempStepDown = false;
    ThermostatPanelState.fanSpeedChangeOnly = false;
    ThermostatPanelState.txActive = false;
    ThermostatPanelState.commandPending = false;
    ThermostatPanelState.pendingPower = false;
    ThermostatPanelState.powerFrameKind = THERMOSTAT_PANEL_POWER_FRAME_DEFAULT;
}

void ThermostatPanel_ReceiveByte(uint8_t data) {
    if (ThermostatPanelState.count >= THERMOSTAT_PANEL_FRAME_LENGTH) {
        ThermostatPanel_ShiftLeft();
    }

    ThermostatPanelState.raw[ThermostatPanelState.count++] = data;
    ThermostatPanel_UpdateAmbientTempByte();
    if (ThermostatPanel_TryParseFanSpeedFrame()) {
        return;
    }
    if (ThermostatPanel_TryParseTempStepFrame()) {
        return;
    }
    if (ThermostatPanel_TryParsePowerFrame()) {
        return;
    }

    if (ThermostatPanelState.count >= THERMOSTAT_PANEL_FRAME_LENGTH) {
        if (ThermostatPanel_IsRxHeader()
         && (ThermostatPanelState.raw[6] >= 0x1Bu)
         && (ThermostatPanelState.raw[6] <= 0x20u)) {
            ThermostatPanel_PublishRoomTempEvent();
        }
        ThermostatPanel_ShiftLeft();
    }
}

bool ThermostatPanel_Poll(THERMOSTAT_PANEL_EVENT *event) {
    if ((event == NULL) || !ThermostatPanelState.eventAvailable) {
        return false;
    }

    *event = ThermostatPanelState.event;
    ThermostatPanelState.eventAvailable = false;
    return true;
}

void ThermostatPanel_RequestPower(bool enabled) {
    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
    ThermostatPanel_SelectCoolPowerFrameKind(ThermostatPanelState.state.fanSpeed);
    ThermostatPanel_QueuePowerCommand(enabled);
}

void ThermostatPanel_RequestPowerTemp(bool enabled, uint8_t tempSetC) {
    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
    ThermostatPanelState.state.tempSetC = tempSetC;
    ThermostatPanel_SelectCoolPowerFrameKind(ThermostatPanelState.state.fanSpeed);
    ThermostatPanel_QueuePowerCommand(enabled);
}

void ThermostatPanel_RequestPowerFanSpeedTemp(bool enabled, uint8_t fanSpeed, uint8_t tempSetC) {
    if (tempSetC < 16u) {
        tempSetC = 16u;
    } else if (tempSetC > 31u) {
        tempSetC = 31u;
    }

    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
    ThermostatPanelState.state.fanSpeed = (uint8_t)(fanSpeed & THERMOSTAT_PANEL_FAN_AUTO);
    ThermostatPanelState.state.tempSetC = tempSetC;
    fanSpeed = ThermostatPanelState.state.fanSpeed;
    ThermostatPanel_SelectCoolPowerFrameKind(fanSpeed);

    if (enabled && (fanSpeed == THERMOSTAT_PANEL_FAN_AUTO)) {
        ThermostatPanel_QueuePowerCommand(enabled);
        return;
    }
    if (enabled && (fanSpeed == THERMOSTAT_PANEL_FAN_LOW)) {
        ThermostatPanel_QueuePowerCommand(enabled);
        return;
    }
    if (enabled && (fanSpeed == THERMOSTAT_PANEL_FAN_MEDIUM)) {
        ThermostatPanel_QueuePowerCommand(enabled);
        return;
    }
    if (enabled && (fanSpeed == THERMOSTAT_PANEL_FAN_HIGH)) {
        ThermostatPanel_QueuePowerCommand(enabled);
        return;
    }
    if (enabled) {
        ThermostatPanel_QueueFanTempCommand(fanSpeed, tempSetC);
        return;
    }
    ThermostatPanel_QueuePowerCommand(enabled);
}

void ThermostatPanel_RequestMode(uint8_t mode) {
    (void)mode;
}

void ThermostatPanel_RequestFanSpeedTemp(uint8_t fanSpeed, uint8_t tempSetC) {
    ThermostatPanel_QueueFanTempCommand(fanSpeed, tempSetC);
}

void ThermostatPanel_TimerTick(void) {
    if (ThermostatPanelState.cooldownTicks > 0u) {
        ThermostatPanelState.cooldownTicks--;
    }
    if (ThermostatPanelState.responseDelayTicks > 0u) {
        ThermostatPanelState.responseDelayTicks--;
    }
}

void ThermostatPanel_MarkBusActivity(void) {
}

void ThermostatPanel_Process(void) {
    if (ThermostatPanelState.txActive) {
        ThermostatPanel_ProcessTx();
        return;
    }

    if (!ThermostatPanelState.commandPending
     || (ThermostatPanelState.cooldownTicks != 0u)
     || (ThermostatPanelState.responseDelayTicks != 0u)
     || EUSART_is_rx_ready()
     || !EUSART_is_tx_done()) {
        return;
    }

    ThermostatPanelState.commandPending = false;
    ThermostatPanelState.txIndex = 0u;
    ThermostatPanelState.txActive = true;
    DIR_SetHigh();
    ThermostatPanel_ProcessTx();
}

bool ThermostatPanel_IsBusReserved(void) {
    return (ThermostatPanelState.txActive
         || ThermostatPanelState.commandPending
         || (ThermostatPanelState.cooldownTicks != 0u));
}

bool ThermostatPanel_IsTxLocked(void) {
    return ThermostatPanelState.txActive;
}

#if 0

#define THERMOSTAT_PANEL_FRAME_LENGTH        8u
#define THERMOSTAT_PANEL_EVENT_QUEUE_LENGTH  8u
#define THERMOSTAT_PANEL_COMMAND_QUEUE_LENGTH 6u
#define THERMOSTAT_PANEL_TIMER_TICK_MS       10u
#define THERMOSTAT_PANEL_COMMAND_INTERVAL_MS 20u
#define THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS \
    (THERMOSTAT_PANEL_COMMAND_INTERVAL_MS / THERMOSTAT_PANEL_TIMER_TICK_MS)
#define THERMOSTAT_PANEL_BUS_QUIET_TICKS THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS
#define THERMOSTAT_PANEL_TX_WAIT_LIMIT     60000u

#define THERMOSTAT_PANEL_HEADER              0x7Eu
#define THERMOSTAT_PANEL_FRAME_TYPE          0x04u
#define THERMOSTAT_PANEL_FUNCTION_WRITE      0x08u

#define THERMOSTAT_PANEL_REG_MODE            0x0031u
#define THERMOSTAT_PANEL_REG_POWER           0x0039u
#define THERMOSTAT_PANEL_REG_FAN_SPEED       0x003Au

#define THERMOSTAT_PANEL_FAN_LOW             0x00u
#define THERMOSTAT_PANEL_FAN_MEDIUM          0x10u
#define THERMOSTAT_PANEL_FAN_HIGH            0x20u
#define THERMOSTAT_PANEL_FAN_AUTO            0x30u

typedef struct {
    uint8_t address;
    uint8_t function;
    uint16_t registerAddress;
    uint16_t value;
} THERMOSTAT_PANEL_FRAME;

typedef struct {
    uint16_t registerAddress;
    uint16_t value;
} THERMOSTAT_PANEL_COMMAND;

typedef struct {
    uint16_t registerAddress;
    uint16_t value;
    uint8_t data4;
    uint8_t data5;
    uint8_t data6;
    uint8_t data7;
} THERMOSTAT_PANEL_COMMAND_FRAME;

typedef struct {
    uint8_t frame[THERMOSTAT_PANEL_FRAME_LENGTH];
    uint8_t index;
    bool active;
} THERMOSTAT_PANEL_TX_STATE;

typedef struct {
    uint8_t raw[THERMOSTAT_PANEL_FRAME_LENGTH];
    uint8_t count;
    THERMOSTAT_PANEL_STATE state;
    THERMOSTAT_PANEL_EVENT eventQueue[THERMOSTAT_PANEL_EVENT_QUEUE_LENGTH];
    uint8_t eventHead;
    uint8_t eventTail;
    uint8_t eventCount;
    THERMOSTAT_PANEL_COMMAND commandQueue[THERMOSTAT_PANEL_COMMAND_QUEUE_LENGTH];
    uint8_t commandHead;
    uint8_t commandTail;
    uint8_t commandCount;
    uint8_t cooldownTicks;
    uint8_t busQuietTicks;
    bool commandSequenceActive;
    bool useHighPower25WorkingFrame;
    bool useHighPower25PowerOff1CFrame;
    THERMOSTAT_PANEL_TX_STATE tx;
} THERMOSTAT_PANEL_RUNTIME;

static THERMOSTAT_PANEL_RUNTIME ThermostatPanelState;

static bool ThermostatPanel_CopyFanTempFrame(uint8_t *frame, uint8_t fanSpeed, uint8_t tempSetC) {
    static const uint8_t fanHighData7[] = {
        0x74u, 0x75u, 0x76u, 0x75u,
        0x77u, 0x78u, 0x78u, 0x78u,
        0x7Au, 0x79u, 0x7Au, 0x7Bu,
        0x7Bu, 0x7Cu, 0x33u, 0x34u
    };
    static const uint8_t fanMediumData7[] = {
        0x6Bu, 0x6Du, 0x6Du, 0x6Du,
        0x6Eu, 0x70u, 0x6Fu, 0x70u,
        0x71u, 0x71u, 0x71u, 0x71u,
        0x73u, 0x72u, 0x2Bu, 0x2Au
    };
    static const uint8_t fanLowData7[] = {
        0x62u, 0x63u, 0x63u, 0x64u,
        0x65u, 0x64u, 0x66u, 0x66u,
        0x67u, 0x67u, 0x69u, 0x68u,
        0x69u, 0x6Au, 0x21u, 0x22u
    };
    static const uint8_t fanAutoData7[] = {
        0x79u, 0x7Au, 0x7Bu, 0x7Bu,
        0x7Cu, 0x7Du, 0x7Cu, 0x7Eu,
        0x7Eu, 0x7Fu, 0x7Fu, 0x77u,
        0x78u, 0x6Eu, 0x1Du, 0x1Cu
    };
    uint8_t index;

    if ((frame == NULL)
     || ((fanSpeed != THERMOSTAT_PANEL_FAN_HIGH)
      && (fanSpeed != THERMOSTAT_PANEL_FAN_MEDIUM)
      && (fanSpeed != THERMOSTAT_PANEL_FAN_LOW)
      && (fanSpeed != THERMOSTAT_PANEL_FAN_AUTO))
     || (tempSetC < 16u)
     || (tempSetC > 31u)) {
        return false;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    if (fanSpeed == THERMOSTAT_PANEL_FAN_HIGH) {
        frame[4] = (tempSetC >= 30u) ? 0x33u : 0xB3u;
        frame[6] = 0x1Eu;
        frame[7] = fanHighData7[index];
        if ((tempSetC == 25u) && ThermostatPanelState.useHighPower25WorkingFrame) {
            frame[4] = 0x37u;
            frame[6] = 0x1Fu;
            frame[7] = 0x33u;
        }
    } else {
        if (fanSpeed == THERMOSTAT_PANEL_FAN_MEDIUM) {
            frame[4] = (tempSetC >= 30u) ? 0x23u : 0xA3u;
            frame[6] = (tempSetC <= 24u) ? 0x1Fu : 0x1Eu;
            frame[7] = fanMediumData7[index];
        } else {
            if (fanSpeed == THERMOSTAT_PANEL_FAN_LOW) {
                frame[4] = (tempSetC >= 30u) ? 0x13u : 0x93u;
                frame[6] = 0x1Eu;
                frame[7] = fanLowData7[index];
            } else {
                if (tempSetC >= 30u) {
                    frame[4] = 0x0Bu;
                } else if (tempSetC == 29u) {
                    frame[4] = 0x9Bu;
                } else if (tempSetC >= 27u) {
                    frame[4] = 0xABu;
                } else {
                    frame[4] = 0xBBu;
                }
                frame[6] = (tempSetC >= 29u) ? 0x1Eu : 0x1Fu;
                frame[7] = fanAutoData7[index];
            }
        }
    }
    frame[5] = tempSetC;
    return true;
}

static bool ThermostatPanel_CopyPowerOffTempFrame(uint8_t *frame, uint8_t fanSpeed, uint8_t tempSetC) {
    static const uint8_t fanHighPowerOffData7[] = {
        0x2Au, 0x2Cu, 0x2Cu, 0x2Cu,
        0x2Du, 0x2Eu, 0x2Du, 0x2Fu,
        0x30u, 0x30u, 0x30u, 0x32u,
        0x31u, 0x32u, 0x33u, 0x33u
    };
    static const uint8_t fanMediumPowerOffData7[] = {
        0x22u, 0x23u, 0x24u, 0x23u,
        0x25u, 0x25u, 0x25u, 0x26u,
        0x26u, 0x28u, 0x27u, 0x28u,
        0x29u, 0x29u, 0x29u, 0x2Bu
    };
    static const uint8_t fanLowPowerOffData7[] = {
        0x18u, 0x1Au, 0x19u, 0x1Au,
        0x1Bu, 0x1Bu, 0x1Cu, 0x1Du,
        0x1Cu, 0x1Eu, 0x1Eu, 0x1Fu,
        0x1Fu, 0x21u, 0x20u, 0x21u
    };
    static const uint8_t fanAutoPowerOffData7[] = {
        0x30u, 0x30u, 0x32u, 0x31u,
        0x32u, 0x33u, 0x33u, 0x34u,
        0x35u, 0x34u, 0x36u, 0x2Du,
        0x2Eu, 0x25u, 0x1Cu, 0x1Du
    };
    uint8_t index;

    if ((frame == NULL)
     || ((fanSpeed != THERMOSTAT_PANEL_FAN_HIGH)
      && (fanSpeed != THERMOSTAT_PANEL_FAN_MEDIUM)
      && (fanSpeed != THERMOSTAT_PANEL_FAN_LOW)
      && (fanSpeed != THERMOSTAT_PANEL_FAN_AUTO))
     || (tempSetC < 16u)
     || (tempSetC > 31u)) {
        return false;
    }

    index = (uint8_t)(tempSetC - 16u);
    frame[0] = THERMOSTAT_PANEL_HEADER;
    frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
    frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
    frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
    if (fanSpeed == THERMOSTAT_PANEL_FAN_LOW) {
        frame[4] = 0x12u;
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_AUTO) {
        if (tempSetC >= 30u) {
            frame[4] = 0x0Au;
        } else if (tempSetC == 29u) {
            frame[4] = 0x1Au;
        } else if (tempSetC >= 27u) {
            frame[4] = 0x2Au;
        } else {
            frame[4] = 0x3Au;
        }
    } else {
        frame[4] = (fanSpeed == THERMOSTAT_PANEL_FAN_MEDIUM) ? 0x22u : 0x32u;
    }
    frame[5] = tempSetC;
    if (fanSpeed == THERMOSTAT_PANEL_FAN_MEDIUM) {
        frame[6] = (tempSetC <= 23u) ? 0x1Fu : 0x1Eu;
        frame[7] = fanMediumPowerOffData7[index];
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_LOW) {
        frame[6] = 0x1Eu;
        frame[7] = fanLowPowerOffData7[index];
    } else if (fanSpeed == THERMOSTAT_PANEL_FAN_AUTO) {
        frame[6] = (tempSetC >= 29u) ? 0x1Eu : 0x1Fu;
        frame[7] = fanAutoPowerOffData7[index];
    } else {
        frame[6] = 0x1Eu;
        frame[7] = fanHighPowerOffData7[index];
        if (tempSetC == 25u) {
            if (ThermostatPanelState.useHighPower25PowerOff1CFrame) {
                frame[4] = 0x2Au;
                frame[6] = 0x1Cu;
                frame[7] = 0x2Bu;
            } else if (ThermostatPanelState.useHighPower25WorkingFrame) {
                frame[4] = 0x36u;
                frame[6] = 0x1Fu;
                frame[7] = 0x33u;
            }
        }
    }
    return true;
}

static bool ThermostatPanel_IsPanelPowerOffFrame(uint8_t *fanSpeed, uint8_t *tempSetC) {
    static const uint8_t panelPowerOffData7[] = {
        0x56u, 0x57u, 0x57u, 0x59u,
        0x58u, 0x59u, 0x5Au, 0x5Au,
        0x5Bu, 0x5Cu, 0x5Bu, 0x5Du,
        0x5Du, 0x5Du, 0x5Eu, 0x50u
    };
    static const uint8_t fanMediumPanelPowerOffData7[] = {
        0x5Eu, 0x5Du, 0x5Fu, 0x50u,
        0x50u, 0x50u, 0x52u, 0x51u,
        0x51u, 0x52u, 0x53u, 0x53u,
        0x54u, 0x55u, 0x54u, 0x56u
    };
    static const uint8_t fanLowPanelPowerOffData7[] = {
        0x53u, 0x55u, 0x55u, 0x55u,
        0x56u, 0x58u, 0x57u, 0x58u,
        0x59u, 0x59u, 0x59u, 0x5Bu,
        0x5Au, 0x5Cu, 0x5Cu, 0x5Cu
    };
    static const uint8_t fanAutoPanelPowerOffData7[] = {
        0x5Cu, 0x5Bu, 0x5Du, 0x5Du,
        0x5Du, 0x5Eu, 0x50u, 0x5Fu,
        0x50u, 0x51u, 0x51u, 0x58u,
        0x59u, 0x50u, 0x57u, 0x58u
    };
    uint8_t index;
    uint8_t expectedData6;
    const uint8_t *data7Table;

    if ((ThermostatPanelState.raw[5] < 0x10u)
     || (ThermostatPanelState.raw[5] > 0x1Fu)
     || ((ThermostatPanelState.raw[4] != 0xF3u)
      && (ThermostatPanelState.raw[4] != 0xE3u)
      && (ThermostatPanelState.raw[4] != 0xD3u)
      && (ThermostatPanelState.raw[4] != 0xCBu)
      && (ThermostatPanelState.raw[4] != 0xDBu)
      && (ThermostatPanelState.raw[4] != 0xEBu)
      && (ThermostatPanelState.raw[4] != 0xFBu))) {
        return false;
    }

    index = (uint8_t)(ThermostatPanelState.raw[5] - 0x10u);
    if (ThermostatPanelState.raw[4] == 0xE3u) {
        if (fanSpeed != NULL) {
            *fanSpeed = THERMOSTAT_PANEL_FAN_MEDIUM;
        }
        expectedData6 = (ThermostatPanelState.raw[5] <= 0x17u) ? 0x1Fu : 0x1Eu;
        data7Table = fanMediumPanelPowerOffData7;
    } else if (ThermostatPanelState.raw[4] == 0xD3u) {
        if (fanSpeed != NULL) {
            *fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
        }
        expectedData6 = 0x1Eu;
        data7Table = fanLowPanelPowerOffData7;
    } else if ((ThermostatPanelState.raw[4] == 0xCBu)
            || (ThermostatPanelState.raw[4] == 0xDBu)
            || (ThermostatPanelState.raw[4] == 0xEBu)
            || (ThermostatPanelState.raw[4] == 0xFBu)) {
        if (fanSpeed != NULL) {
            *fanSpeed = THERMOSTAT_PANEL_FAN_AUTO;
        }
        if (((ThermostatPanelState.raw[4] == 0xCBu) && (ThermostatPanelState.raw[5] < 0x1Eu))
         || ((ThermostatPanelState.raw[4] == 0xDBu) && (ThermostatPanelState.raw[5] != 0x1Du))
         || ((ThermostatPanelState.raw[4] == 0xEBu) && ((ThermostatPanelState.raw[5] < 0x1Bu) || (ThermostatPanelState.raw[5] > 0x1Cu)))
         || ((ThermostatPanelState.raw[4] == 0xFBu) && (ThermostatPanelState.raw[5] > 0x1Au))) {
            return false;
        }
        expectedData6 = (ThermostatPanelState.raw[5] >= 0x1Du) ? 0x1Eu : 0x1Fu;
        data7Table = fanAutoPanelPowerOffData7;
    } else {
        if (fanSpeed != NULL) {
            *fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
        }
        data7Table = panelPowerOffData7;
        ThermostatPanelState.useHighPower25WorkingFrame = false;
        ThermostatPanelState.useHighPower25PowerOff1CFrame = false;
        expectedData6 = 0x1Eu;
    }

    if ((ThermostatPanelState.raw[6] != expectedData6)
     || (ThermostatPanelState.raw[7] != data7Table[index])) {
        return false;
    }

    if (tempSetC != NULL) {
        *tempSetC = ThermostatPanelState.raw[5];
    }

    return true;
}

static bool ThermostatPanel_TryParseWorkingPower25Frame(uint16_t *value) {
    if ((value == NULL)
     || (ThermostatPanelState.raw[0] != 0x7Fu)
     || (ThermostatPanelState.raw[1] != 0x05u)
     || (ThermostatPanelState.raw[2] != THERMOSTAT_PANEL_DEFAULT_ADDRESS)
     || (ThermostatPanelState.raw[3] != 0x01u)) {
        return false;
    }

    if ((((ThermostatPanelState.raw[4] == 0xF7u)
       && (ThermostatPanelState.raw[6] == 0x1Fu)
       && (ThermostatPanelState.raw[7] == 0x5Eu))
      || ((ThermostatPanelState.raw[4] == 0x77u)
       && (ThermostatPanelState.raw[6] == 0x1Fu)
       && (ThermostatPanelState.raw[7] == 0x55u)))
     && (ThermostatPanelState.raw[5] == 0x19u)) {
        ThermostatPanelState.useHighPower25WorkingFrame = true;
        ThermostatPanelState.useHighPower25PowerOff1CFrame = false;
        *value = (uint16_t)((25u << 8) | THERMOSTAT_PANEL_FAN_HIGH);
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0x73u)
     && (ThermostatPanelState.raw[5] == 0x19u)
     && (ThermostatPanelState.raw[6] == 0x1Eu)
     && (ThermostatPanelState.raw[7] == 0x52u)) {
        ThermostatPanelState.useHighPower25WorkingFrame = false;
        ThermostatPanelState.useHighPower25PowerOff1CFrame = false;
        *value = (uint16_t)((25u << 8) | THERMOSTAT_PANEL_FAN_HIGH);
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0x73u)
     && (ThermostatPanelState.raw[5] == 0x19u)
     && (((ThermostatPanelState.raw[6] == 0x1Cu)
       && (ThermostatPanelState.raw[7] == 0x51u))
      || ((ThermostatPanelState.raw[6] == 0x1Bu)
       && (ThermostatPanelState.raw[7] == 0x50u)))) {
        ThermostatPanelState.useHighPower25WorkingFrame = false;
        ThermostatPanelState.useHighPower25PowerOff1CFrame = true;
        *value = (uint16_t)((25u << 8) | THERMOSTAT_PANEL_FAN_HIGH);
        return true;
    }

    if ((ThermostatPanelState.raw[4] == 0x56u)
     && (ThermostatPanelState.raw[5] == 0x18u)
     && (ThermostatPanelState.raw[6] == 0x1Fu)
     && (ThermostatPanelState.raw[7] == 0x51u)) {
        ThermostatPanelState.useHighPower25WorkingFrame = true;
        ThermostatPanelState.useHighPower25PowerOff1CFrame = false;
        *value = (uint16_t)((25u << 8) | THERMOSTAT_PANEL_FAN_HIGH | 1u);
        return true;
    }

    return false;
}

static bool ThermostatPanel_TryParsePanelTempStep(uint16_t *value) {
    static const uint8_t fanHighTempUpData7[] = {
        0x57u, 0x57u, 0x59u, 0x58u,
        0x59u, 0x5Au, 0x5Au, 0x5Bu,
        0x5Cu, 0x5Bu, 0x5Du, 0x5Du,
        0x5Du, 0x5Eu, 0x50u, 0x5Fu
    };
    static const uint8_t fanLowTempUpData7[] = {
        0x55u, 0x55u, 0x55u, 0x56u,
        0x58u, 0x57u, 0x58u, 0x59u,
        0x59u, 0x59u, 0x5Bu, 0x5Au,
        0x5Cu, 0x5Cu, 0x5Cu, 0x5Cu
    };
    static const uint8_t fanMediumTempDownData7[] = {
        0x5Eu, 0x50u, 0x50u, 0x50u,
        0x52u, 0x51u, 0x52u, 0x53u,
        0x53u, 0x53u, 0x54u, 0x55u,
        0x54u, 0x56u, 0x56u, 0x57u
    };
    static const uint8_t fanAutoTempDownData7[] = {
        0x5Cu, 0x5Du, 0x5Du, 0x5Eu,
        0x50u, 0x5Fu, 0x50u, 0x51u,
        0x51u, 0x51u, 0x53u, 0x5Au,
        0x5Au, 0x51u, 0x59u, 0x59u
    };
    uint8_t index;
    uint8_t tempSetC;
    uint8_t fanSpeed;
    uint8_t expectedData6;
    const uint8_t *data7Table;

    if ((value == NULL)
     || (ThermostatPanelState.raw[0] != 0x7Fu)
     || (ThermostatPanelState.raw[1] != 0x05u)
     || (ThermostatPanelState.raw[2] != THERMOSTAT_PANEL_DEFAULT_ADDRESS)
     || (ThermostatPanelState.raw[5] < 0x10u)
     || (ThermostatPanelState.raw[5] > 0x1Fu)) {
        return false;
    }

    index = (uint8_t)(ThermostatPanelState.raw[5] - 0x10u);
    if ((ThermostatPanelState.raw[3] == 0x02u)
     && (ThermostatPanelState.raw[4] == 0xF3u)) {
        fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
        tempSetC = (uint8_t)(ThermostatPanelState.raw[5] + 1u);
        expectedData6 = 0x1Eu;
        data7Table = fanHighTempUpData7;
    } else if ((ThermostatPanelState.raw[3] == 0x02u)
            && (ThermostatPanelState.raw[4] == 0xD3u)) {
        fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
        tempSetC = (uint8_t)(ThermostatPanelState.raw[5] + 1u);
        expectedData6 = 0x1Eu;
        data7Table = fanLowTempUpData7;
    } else if ((ThermostatPanelState.raw[3] == 0x03u)
            && (ThermostatPanelState.raw[4] == 0xE3u)) {
        fanSpeed = THERMOSTAT_PANEL_FAN_MEDIUM;
        tempSetC = (uint8_t)(ThermostatPanelState.raw[5] - 1u);
        expectedData6 = (ThermostatPanelState.raw[5] <= 0x17u) ? 0x1Fu : 0x1Eu;
        data7Table = fanMediumTempDownData7;
    } else if ((ThermostatPanelState.raw[3] == 0x03u)
            && ((ThermostatPanelState.raw[4] == 0xCBu)
             || (ThermostatPanelState.raw[4] == 0xDBu)
             || (ThermostatPanelState.raw[4] == 0xEBu)
             || (ThermostatPanelState.raw[4] == 0xFBu))) {
        if (((ThermostatPanelState.raw[4] == 0xCBu) && (ThermostatPanelState.raw[5] < 0x1Eu))
         || ((ThermostatPanelState.raw[4] == 0xDBu) && (ThermostatPanelState.raw[5] != 0x1Du))
         || ((ThermostatPanelState.raw[4] == 0xEBu) && ((ThermostatPanelState.raw[5] < 0x1Bu) || (ThermostatPanelState.raw[5] > 0x1Cu)))
         || ((ThermostatPanelState.raw[4] == 0xFBu) && (ThermostatPanelState.raw[5] > 0x1Au))) {
            return false;
        }
        fanSpeed = THERMOSTAT_PANEL_FAN_AUTO;
        tempSetC = (uint8_t)(ThermostatPanelState.raw[5] - 1u);
        expectedData6 = (ThermostatPanelState.raw[5] >= 0x1Du) ? 0x1Eu : 0x1Fu;
        data7Table = fanAutoTempDownData7;
    } else {
        return false;
    }

    if (tempSetC > 31u) {
        tempSetC = 31u;
    } else if (tempSetC < 16u) {
        tempSetC = 16u;
    }

    if ((ThermostatPanelState.raw[6] != expectedData6)
     || (ThermostatPanelState.raw[7] != data7Table[index])) {
        return false;
    }

    *value = (uint16_t)(fanSpeed | ((uint16_t)tempSetC << 8));
    return true;
}

static bool ThermostatPanel_CopyExactCommandFrame(uint8_t *frame, uint16_t registerAddress, uint16_t value) {
    static const THERMOSTAT_PANEL_COMMAND_FRAME commandFrames[] = {
        {THERMOSTAT_PANEL_REG_POWER, 1u,                         0x13u, 0x19u, 0x19u, 0x1Bu},
        {THERMOSTAT_PANEL_REG_POWER, 0u,                         0x32u, 0x19u, 0x1Eu, 0x30u},
        {THERMOSTAT_PANEL_REG_MODE, THERMOSTAT_PANEL_MODE_COOL,  0xABu, 0x19u, 0x1Cu, 0x74u},
        {THERMOSTAT_PANEL_REG_MODE, THERMOSTAT_PANEL_MODE_HEAT,  0x0Du, 0x19u, 0x1Cu, 0x19u},
        {THERMOSTAT_PANEL_REG_MODE, THERMOSTAT_PANEL_MODE_FAN,   0x0Fu, 0x19u, 0x1Cu, 0x1Bu},
        {THERMOSTAT_PANEL_REG_FAN_SPEED, THERMOSTAT_PANEL_FAN_LOW,    0x93u, 0x19u, 0x1Du, 0x67u},
        {THERMOSTAT_PANEL_REG_FAN_SPEED, THERMOSTAT_PANEL_FAN_MEDIUM, 0xA3u, 0x19u, 0x1Du, 0x70u},
        {THERMOSTAT_PANEL_REG_FAN_SPEED, THERMOSTAT_PANEL_FAN_HIGH,   0xB3u, 0x11u, 0x20u, 0x75u},
        {THERMOSTAT_PANEL_REG_FAN_SPEED, THERMOSTAT_PANEL_FAN_AUTO,   0xABu, 0x19u, 0x1Cu, 0x74u},
    };
    uint8_t i;

    if (frame == NULL) {
        return false;
    }

    if ((registerAddress == THERMOSTAT_PANEL_REG_POWER)
     && ((value & 0xFF00u) != 0u)
     && ((value & 0x000Fu) == 0u)) {
        if (ThermostatPanel_CopyPowerOffTempFrame(frame, (uint8_t)(value & 0x00F0u), (uint8_t)(value >> 8))) {
            return true;
        }
        value &= 0x00FFu;
    }

    if ((registerAddress == THERMOSTAT_PANEL_REG_FAN_SPEED)
     && ((value & 0xFF00u) != 0u)) {
        if (ThermostatPanel_CopyFanTempFrame(frame, (uint8_t)(value & 0x00FFu), (uint8_t)(value >> 8))) {
            return true;
        }
        value &= 0x00FFu;
    }

    for (i = 0u; i < (uint8_t)(sizeof(commandFrames) / sizeof(commandFrames[0])); i++) {
        if ((commandFrames[i].registerAddress == registerAddress)
         && (commandFrames[i].value == value)) {
            frame[0] = THERMOSTAT_PANEL_HEADER;
            frame[1] = THERMOSTAT_PANEL_FRAME_TYPE;
            frame[2] = THERMOSTAT_PANEL_DEFAULT_ADDRESS;
            frame[3] = THERMOSTAT_PANEL_FUNCTION_WRITE;
            frame[4] = commandFrames[i].data4;
            frame[5] = commandFrames[i].data5;
            frame[6] = commandFrames[i].data6;
            frame[7] = commandFrames[i].data7;
            return true;
        }
    }

    return false;
}

static void ThermostatPanel_ShiftLeft(void) {
    uint8_t i;

    for (i = 1u; i < THERMOSTAT_PANEL_FRAME_LENGTH; i++) {
        ThermostatPanelState.raw[i - 1u] = ThermostatPanelState.raw[i];
    }

    ThermostatPanelState.count = (uint8_t)(THERMOSTAT_PANEL_FRAME_LENGTH - 1u);
}

static bool ThermostatPanel_TryParseFrame(THERMOSTAT_PANEL_FRAME *frame) {
    uint16_t registerAddress = THERMOSTAT_PANEL_REG_MODE;
    uint16_t value = THERMOSTAT_PANEL_MODE_COOL;
    uint8_t tempSetC = 0u;
    uint8_t fanSpeed = THERMOSTAT_PANEL_FAN_HIGH;
    uint16_t tempStepValue;
    uint16_t workingPowerValue;

    if ((frame == NULL) || (ThermostatPanelState.count < THERMOSTAT_PANEL_FRAME_LENGTH)) {
        return false;
    }

    if ((ThermostatPanelState.raw[0] == 0x7Fu)
     && (ThermostatPanelState.raw[1] == 0x05u)
     && (ThermostatPanelState.raw[2] == THERMOSTAT_PANEL_DEFAULT_ADDRESS)
     && (ThermostatPanelState.raw[3] == 0x00u)) {
        frame->address = ThermostatPanelState.raw[2];
        frame->function = ThermostatPanelState.raw[3];
        frame->registerAddress = 0u;
        frame->value = 0u;
        ThermostatPanelState.count = 0u;
        return true;
    }

    if (ThermostatPanel_TryParseWorkingPower25Frame(&workingPowerValue)) {
        registerAddress = THERMOSTAT_PANEL_REG_POWER;
        value = workingPowerValue;
    } else if ((ThermostatPanelState.raw[0] == 0x7Fu)
     && (ThermostatPanelState.raw[1] == 0x05u)
     && (ThermostatPanelState.raw[2] == THERMOSTAT_PANEL_DEFAULT_ADDRESS)
     && (ThermostatPanelState.raw[3] == 0x01u)) {
        if (((ThermostatPanelState.raw[4] == 0xD2u)
          || (ThermostatPanelState.raw[4] == 0xD3u))
         && ((ThermostatPanelState.raw[5] == 0x18u)
          || (ThermostatPanelState.raw[5] == 0x19u))
        && (ThermostatPanelState.raw[6] >= 0x1Bu)
         && (ThermostatPanelState.raw[6] <= 0x20u)) {
            registerAddress = THERMOSTAT_PANEL_REG_POWER;
            value = ThermostatPanelState.state.power ? 0u : 1u;
        } else if (((ThermostatPanelState.raw[4] == 0xEBu)
                 && (ThermostatPanelState.raw[5] == 0x19u)
                 && (ThermostatPanelState.raw[6] == 0x1Cu)
                 && (ThermostatPanelState.raw[7] == 0x56u))
                || ThermostatPanel_IsPanelPowerOffFrame(&fanSpeed, &tempSetC)) {
            registerAddress = THERMOSTAT_PANEL_REG_POWER;
            value = ThermostatPanelState.state.power ? 0u : 1u;
            if (tempSetC != 0u) {
                value |= ((uint16_t)tempSetC << 8);
                value |= (uint16_t)fanSpeed;
            }
        } else {
            ThermostatPanelState.count = 0u;
            return false;
        }
    } else if (ThermostatPanel_TryParsePanelTempStep(&tempStepValue)) {
        registerAddress = THERMOSTAT_PANEL_REG_FAN_SPEED;
        value = tempStepValue;
    } else if ((ThermostatPanelState.raw[0] == 0x7Fu)
     && (ThermostatPanelState.raw[1] == 0x05u)
     && (ThermostatPanelState.raw[2] == THERMOSTAT_PANEL_DEFAULT_ADDRESS)
     && (ThermostatPanelState.raw[5] == 0x19u)
     && (ThermostatPanelState.raw[6] == 0x1Cu)) {
        if (ThermostatPanelState.raw[3] == 0x04u) {
            registerAddress = THERMOSTAT_PANEL_REG_FAN_SPEED;
            if (ThermostatPanelState.raw[4] == 0xEBu) {
                value = THERMOSTAT_PANEL_FAN_LOW;
            } else if (ThermostatPanelState.raw[4] == 0xD3u) {
                value = THERMOSTAT_PANEL_FAN_MEDIUM;
            } else if (ThermostatPanelState.raw[4] == 0xE3u) {
                value = THERMOSTAT_PANEL_FAN_HIGH;
            } else if (ThermostatPanelState.raw[4] == 0xF3u) {
                value = THERMOSTAT_PANEL_FAN_AUTO;
            } else {
                ThermostatPanelState.count = 0u;
                return false;
            }
        } else if (ThermostatPanelState.raw[3] == 0x05u) {
            registerAddress = THERMOSTAT_PANEL_REG_MODE;
            if (ThermostatPanelState.raw[4] == 0xCFu) {
                value = THERMOSTAT_PANEL_MODE_COOL;
            } else if (ThermostatPanelState.raw[4] == 0xEBu) {
                value = THERMOSTAT_PANEL_MODE_HEAT;
            } else if (ThermostatPanelState.raw[4] == 0xCDu) {
                value = THERMOSTAT_PANEL_MODE_FAN;
            } else {
                ThermostatPanelState.count = 0u;
                return false;
            }
        } else if ((ThermostatPanelState.raw[3] == 0x06u)
                && (ThermostatPanelState.raw[4] == 0xEBu)
                && (ThermostatPanelState.raw[7] == 0x58u)) {
            registerAddress = THERMOSTAT_PANEL_REG_POWER;
            value = 0u;
        } else {
            ThermostatPanelState.count = 0u;
            return false;
        }
    } else if ((ThermostatPanelState.raw[0] != THERMOSTAT_PANEL_HEADER)
            || (ThermostatPanelState.raw[1] != THERMOSTAT_PANEL_FRAME_TYPE)
            || (ThermostatPanelState.raw[2] != THERMOSTAT_PANEL_DEFAULT_ADDRESS)
            || (ThermostatPanelState.raw[3] != THERMOSTAT_PANEL_FUNCTION_WRITE)) {
        return false;
    } else if ((ThermostatPanelState.raw[4] == 0x13u)
     && (ThermostatPanelState.raw[5] == 0x19u)
     && (ThermostatPanelState.raw[6] == 0x19u)
     && (ThermostatPanelState.raw[7] == 0x1Bu)) {
        registerAddress = THERMOSTAT_PANEL_REG_POWER;
        value = 1u;
    } else if ((ThermostatPanelState.raw[4] == 0x2Au)
            && (ThermostatPanelState.raw[5] == 0x19u)
            && (ThermostatPanelState.raw[6] == 0x1Cu)
            && (ThermostatPanelState.raw[7] == 0x2Bu)) {
        registerAddress = THERMOSTAT_PANEL_REG_POWER;
        value = 0u;
    } else if ((ThermostatPanelState.raw[4] == 0xABu)
            && (ThermostatPanelState.raw[5] == 0x19u)
            && (ThermostatPanelState.raw[6] == 0x1Cu)
            && (ThermostatPanelState.raw[7] == 0x74u)) {
        registerAddress = THERMOSTAT_PANEL_REG_MODE;
        value = THERMOSTAT_PANEL_MODE_COOL;
    } else if ((ThermostatPanelState.raw[4] == 0x0Du)
            && (ThermostatPanelState.raw[5] == 0x19u)
            && (ThermostatPanelState.raw[6] == 0x1Cu)
            && (ThermostatPanelState.raw[7] == 0x19u)) {
        registerAddress = THERMOSTAT_PANEL_REG_MODE;
        value = THERMOSTAT_PANEL_MODE_HEAT;
    } else if ((ThermostatPanelState.raw[4] == 0x0Fu)
            && (ThermostatPanelState.raw[5] == 0x19u)
            && (ThermostatPanelState.raw[6] == 0x1Cu)
            && (ThermostatPanelState.raw[7] == 0x1Bu)) {
        registerAddress = THERMOSTAT_PANEL_REG_MODE;
        value = THERMOSTAT_PANEL_MODE_FAN;
    } else {
        ThermostatPanelState.count = 0u;
        return false;
    }

    frame->address = ThermostatPanelState.raw[2];
    frame->function = ThermostatPanelState.raw[3];
    frame->registerAddress = registerAddress;
    frame->value = value;
    ThermostatPanelState.count = 0u;
    return true;
}

static bool ThermostatPanel_PushEvent(const THERMOSTAT_PANEL_EVENT *event) {
    if ((event == NULL) || (ThermostatPanelState.eventCount >= THERMOSTAT_PANEL_EVENT_QUEUE_LENGTH)) {
        return false;
    }

    ThermostatPanelState.eventQueue[ThermostatPanelState.eventTail] = *event;
    ThermostatPanelState.eventTail++;
    if (ThermostatPanelState.eventTail >= THERMOSTAT_PANEL_EVENT_QUEUE_LENGTH) {
        ThermostatPanelState.eventTail = 0u;
    }

    ThermostatPanelState.eventCount++;
    return true;
}

static bool ThermostatPanel_PushCommand(uint16_t registerAddress, uint16_t value) {
    if (ThermostatPanelState.commandCount >= THERMOSTAT_PANEL_COMMAND_QUEUE_LENGTH) {
        return false;
    }

    ThermostatPanelState.commandQueue[ThermostatPanelState.commandTail].registerAddress = registerAddress;
    ThermostatPanelState.commandQueue[ThermostatPanelState.commandTail].value = value;
    ThermostatPanelState.commandTail++;
    if (ThermostatPanelState.commandTail >= THERMOSTAT_PANEL_COMMAND_QUEUE_LENGTH) {
        ThermostatPanelState.commandTail = 0u;
    }

    ThermostatPanelState.commandCount++;
    return true;
}

static void ThermostatPanel_ClearCommands(void) {
    ThermostatPanelState.commandHead = 0u;
    ThermostatPanelState.commandTail = 0u;
    ThermostatPanelState.commandCount = 0u;
    ThermostatPanelState.busQuietTicks = 0u;
    ThermostatPanelState.commandSequenceActive = false;
}

static void ThermostatPanel_StartTx(const THERMOSTAT_PANEL_COMMAND *command) {
    if (command == NULL) {
        return;
    }

    if (!ThermostatPanel_CopyExactCommandFrame(ThermostatPanelState.tx.frame, command->registerAddress, command->value)) {
        return;
    }
    ThermostatPanelState.tx.index = 0u;
    ThermostatPanelState.tx.active = true;
    ThermostatPanelState.commandSequenceActive = true;
    DIR_SetHigh();
}

static void ThermostatPanel_AbortTx(void) {
    DIR_SetLow();
    ThermostatPanelState.tx.active = false;
    ThermostatPanel_ClearCommands();
    ThermostatPanelState.cooldownTicks = THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS;
}

static void ThermostatPanel_ProcessTx(void) {
    uint16_t waitCount;

    if (!ThermostatPanelState.tx.active) {
        return;
    }

    while (ThermostatPanelState.tx.index < THERMOSTAT_PANEL_FRAME_LENGTH) {
        waitCount = 0u;
        while (!EUSART_is_tx_ready()) {
            waitCount++;
            if (waitCount >= THERMOSTAT_PANEL_TX_WAIT_LIMIT) {
                ThermostatPanel_AbortTx();
                return;
            }
        }
        TX1REG = ThermostatPanelState.tx.frame[ThermostatPanelState.tx.index++];
    }

    waitCount = 0u;
    while (!EUSART_is_tx_done()) {
        waitCount++;
        if (waitCount >= THERMOSTAT_PANEL_TX_WAIT_LIMIT) {
            ThermostatPanel_AbortTx();
            return;
        }
    }

    DIR_SetLow();
    ThermostatPanelState.tx.active = false;
    ThermostatPanelState.commandSequenceActive = false;
    ThermostatPanelState.cooldownTicks = THERMOSTAT_PANEL_COMMAND_INTERVAL_TICKS;
}

static bool ThermostatPanel_PopCommand(THERMOSTAT_PANEL_COMMAND *command) {
    if ((command == NULL) || (ThermostatPanelState.commandCount == 0u)) {
        return false;
    }

    *command = ThermostatPanelState.commandQueue[ThermostatPanelState.commandHead];
    ThermostatPanelState.commandHead++;
    if (ThermostatPanelState.commandHead >= THERMOSTAT_PANEL_COMMAND_QUEUE_LENGTH) {
        ThermostatPanelState.commandHead = 0u;
    }

    ThermostatPanelState.commandCount--;
    return true;
}

static void ThermostatPanel_PublishEvent(const THERMOSTAT_PANEL_FRAME *frame, THERMOSTAT_PANEL_EVENT_TYPE type) {
    THERMOSTAT_PANEL_EVENT event;

    if ((frame == NULL) || (type == THERMOSTAT_PANEL_EVENT_NONE)) {
        return;
    }

    event.address = frame->address;
    event.function = frame->function;
    event.registerAddress = frame->registerAddress;
    event.value = frame->value;
    event.type = type;
    event.state = ThermostatPanelState.state;
    (void)ThermostatPanel_PushEvent(&event);
}

static THERMOSTAT_PANEL_EVENT_TYPE ThermostatPanel_ApplyFrame(const THERMOSTAT_PANEL_FRAME *frame) {
    uint8_t tempSetC;

    if (frame == NULL) {
        return THERMOSTAT_PANEL_EVENT_NONE;
    }

    if (frame->registerAddress == THERMOSTAT_PANEL_REG_POWER) {
        ThermostatPanelState.state.power = ((frame->value & 0x000Fu) != 0u);
        tempSetC = (uint8_t)(frame->value >> 8);
        if (tempSetC != 0u) {
            ThermostatPanelState.state.tempSetC = tempSetC;
            ThermostatPanelState.state.fanSpeed = (uint8_t)(frame->value & THERMOSTAT_PANEL_FAN_AUTO);
        }
        return THERMOSTAT_PANEL_EVENT_POWER;
    }

    if (frame->registerAddress == THERMOSTAT_PANEL_REG_MODE) {
        ThermostatPanelState.state.mode = (uint8_t)frame->value;
        ThermostatPanelState.state.power = true;
        return THERMOSTAT_PANEL_EVENT_MODE;
    }

    if (frame->registerAddress == THERMOSTAT_PANEL_REG_FAN_SPEED) {
        ThermostatPanelState.state.fanSpeed = (uint8_t)frame->value;
        tempSetC = (uint8_t)(frame->value >> 8);
        if (tempSetC != 0u) {
            ThermostatPanelState.state.tempSetC = tempSetC;
        }
        ThermostatPanelState.state.power = true;
        return THERMOSTAT_PANEL_EVENT_FAN_SPEED;
    }

    return THERMOSTAT_PANEL_EVENT_NONE;
}

void ThermostatPanel_Init(void) {
    uint8_t i;

    for (i = 0u; i < THERMOSTAT_PANEL_FRAME_LENGTH; i++) {
        ThermostatPanelState.raw[i] = 0u;
    }

    ThermostatPanelState.count = 0u;
    ThermostatPanelState.state.power = false;
    ThermostatPanelState.state.mode = THERMOSTAT_PANEL_MODE_COOL;
    ThermostatPanelState.state.fanSpeed = THERMOSTAT_PANEL_FAN_LOW;
    ThermostatPanelState.state.tempSetC = 25u;
    ThermostatPanelState.eventHead = 0u;
    ThermostatPanelState.eventTail = 0u;
    ThermostatPanelState.eventCount = 0u;
    ThermostatPanelState.commandHead = 0u;
    ThermostatPanelState.commandTail = 0u;
    ThermostatPanelState.commandCount = 0u;
    ThermostatPanelState.cooldownTicks = 0u;
    ThermostatPanelState.busQuietTicks = 0u;
    ThermostatPanelState.commandSequenceActive = false;
    ThermostatPanelState.useHighPower25WorkingFrame = false;
    ThermostatPanelState.useHighPower25PowerOff1CFrame = false;
    ThermostatPanelState.tx.index = 0u;
    ThermostatPanelState.tx.active = false;
}

void ThermostatPanel_ReceiveByte(uint8_t data) {
    THERMOSTAT_PANEL_FRAME frame;
    THERMOSTAT_PANEL_EVENT_TYPE eventType;

    if (ThermostatPanelState.count >= THERMOSTAT_PANEL_FRAME_LENGTH) {
        ThermostatPanel_ShiftLeft();
    }

    ThermostatPanelState.raw[ThermostatPanelState.count++] = data;

    if (ThermostatPanel_TryParseFrame(&frame)) {
        eventType = ThermostatPanel_ApplyFrame(&frame);
        ThermostatPanel_PublishEvent(&frame, eventType);
        return;
    }

    if (ThermostatPanelState.count >= THERMOSTAT_PANEL_FRAME_LENGTH) {
        ThermostatPanel_ShiftLeft();
    }
}

bool ThermostatPanel_Poll(THERMOSTAT_PANEL_EVENT *event) {
    if ((event == NULL) || (ThermostatPanelState.eventCount == 0u)) {
        return false;
    }

    *event = ThermostatPanelState.eventQueue[ThermostatPanelState.eventHead];
    ThermostatPanelState.eventHead++;
    if (ThermostatPanelState.eventHead >= THERMOSTAT_PANEL_EVENT_QUEUE_LENGTH) {
        ThermostatPanelState.eventHead = 0u;
    }

    ThermostatPanelState.eventCount--;
    return true;
}

void ThermostatPanel_RequestPower(bool enabled) {
    ThermostatPanel_RequestPowerTemp(enabled, 0u);
}

void ThermostatPanel_RequestPowerTemp(bool enabled, uint8_t tempSetC) {
    ThermostatPanel_RequestPowerFanSpeedTemp(enabled, THERMOSTAT_PANEL_FAN_HIGH, tempSetC);
}

void ThermostatPanel_RequestPowerFanSpeedTemp(bool enabled, uint8_t fanSpeed, uint8_t tempSetC) {
    uint16_t powerValue;

    powerValue = enabled ? 1u : 0u;
    if ((!enabled) && (tempSetC != 0u)) {
        powerValue |= ((uint16_t)tempSetC << 8);
        powerValue |= (uint16_t)(fanSpeed & THERMOSTAT_PANEL_FAN_AUTO);
    }

    ThermostatPanel_ClearCommands();
    (void)ThermostatPanel_PushCommand(
        THERMOSTAT_PANEL_REG_POWER,
        powerValue
    );
}

void ThermostatPanel_RequestMode(uint8_t mode) {
    uint16_t modeValue = mode;

    if ((mode != THERMOSTAT_PANEL_MODE_COOL)
     && (mode != THERMOSTAT_PANEL_MODE_HEAT)
     && (mode != THERMOSTAT_PANEL_MODE_FAN)) {
        modeValue = THERMOSTAT_PANEL_MODE_COOL;
    }

    (void)ThermostatPanel_PushCommand(THERMOSTAT_PANEL_REG_MODE, modeValue);
}

void ThermostatPanel_RequestFanSpeedTemp(uint8_t fanSpeed, uint8_t tempSetC) {
    uint16_t fanSpeedValue = (uint16_t)(fanSpeed & THERMOSTAT_PANEL_FAN_AUTO);

    if (tempSetC != 0u) {
        fanSpeedValue |= ((uint16_t)tempSetC << 8);
    }

    (void)ThermostatPanel_PushCommand(THERMOSTAT_PANEL_REG_FAN_SPEED, fanSpeedValue);
}

void ThermostatPanel_TimerTick(void) {
    if (ThermostatPanelState.cooldownTicks > 0u) {
        ThermostatPanelState.cooldownTicks--;
    }

    if (ThermostatPanelState.busQuietTicks > 0u) {
        ThermostatPanelState.busQuietTicks--;
    }

}

void ThermostatPanel_MarkBusActivity(void) {
    if (!ThermostatPanelState.commandSequenceActive
     && (ThermostatPanelState.commandCount != 0u)) {
        ThermostatPanelState.busQuietTicks = THERMOSTAT_PANEL_BUS_QUIET_TICKS;
    }
}

void ThermostatPanel_Process(void) {
    THERMOSTAT_PANEL_COMMAND command;

    ThermostatPanel_ProcessTx();
    if (ThermostatPanelState.tx.active) {
        return;
    }

    if (ThermostatPanelState.commandCount == 0u) {
        return;
    }

    if ((ThermostatPanelState.cooldownTicks != 0u)
     || (ThermostatPanelState.busQuietTicks != 0u)
     || EUSART_is_rx_ready()
     || !EUSART_is_tx_done()) {
        return;
    }

    if (!ThermostatPanel_PopCommand(&command)) {
        return;
    }

    ThermostatPanel_StartTx(&command);
    ThermostatPanel_ProcessTx();
}

bool ThermostatPanel_IsBusReserved(void) {
    if (ThermostatPanelState.tx.active
     || ThermostatPanelState.commandSequenceActive
     || (ThermostatPanelState.cooldownTicks != 0u)) {
        return true;
    }

    return false;
}

bool ThermostatPanel_IsTxLocked(void) {
    if (ThermostatPanelState.tx.active) {
        return true;
    }

    return false;
}
#endif
