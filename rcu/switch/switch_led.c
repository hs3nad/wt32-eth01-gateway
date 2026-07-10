#include <stdbool.h>
#include <stdint.h>

#include "../mcc_generated_files/mcc.h"
#include "switch_led.h"

#define SWITCH_LED_DEFAULT_INTERVAL_TICKS  2u
#define SWITCH_LED_FRAME_LENGTH            8u
#define SWITCH_LED_TX_WAIT_LIMIT           60000u
#define SWITCH_LED_ALL_KEYS_MASK           0x0007u
#define SWITCH_AHF_TABLE_COUNT             33u
#define SWITCH_BACKLIGHT_OFF_FRAME_COUNT   6u

typedef struct {
    uint8_t panelAddress;
    uint8_t currentState;
    uint8_t lastSentState;
    bool pending;
} SWITCH_LED_PANEL_STATE;

typedef enum {
    SWITCH_LED_TX_KIND_NONE = 0u,
    SWITCH_LED_TX_KIND_PANEL_UPDATE,
    SWITCH_LED_TX_KIND_BACKLIGHT_OFF
} SWITCH_LED_TX_KIND;

typedef struct {
    uint8_t frame[SWITCH_LED_FRAME_LENGTH];
    uint8_t length;
    uint8_t index;
    uint8_t panelIndex;
    uint8_t panelValue;
    SWITCH_LED_TX_KIND kind;
    bool active;
} SWITCH_LED_TX_STATE;

typedef struct {
    uint8_t sendIntervalTicks;
    uint8_t cooldownTicks;
    uint8_t nextPanelIndex;
    bool initialized;
    bool backlightOffPending;
    uint8_t backlightOffIndex;
    SWITCH_LED_PANEL_STATE panels[SWITCH_LED_MAX_PANELS];
    SWITCH_LED_TX_STATE tx;
} SWITCH_LED_STATE;

typedef struct {
    uint8_t panelAddress;
    uint8_t state;
    uint8_t frame[SWITCH_LED_FRAME_LENGTH];
} SWITCH_LED_AHF_FRAME;

static SWITCH_LED_STATE SwitchLedState;

static const SWITCH_LED_AHF_FRAME SwitchLedAhfFrames[SWITCH_AHF_TABLE_COUNT] = {
    {0x01u, 0x01u, {0x7Eu, 0x02u, 0x01u, 0xC1u, 0x01u, 0x00u, 0x00u, 0x44u}},
    {0x07u, 0x00u, {0x7Eu, 0x02u, 0x07u, 0xC1u, 0x00u, 0x00u, 0x00u, 0x46u}},
    {0x07u, 0x01u, {0x7Eu, 0x02u, 0x07u, 0xC1u, 0x01u, 0x00u, 0x00u, 0x4Eu}},
    {0x07u, 0x02u, {0x7Eu, 0x02u, 0x07u, 0xC1u, 0x02u, 0x00u, 0x00u, 0xE0u}},
    {0x07u, 0x03u, {0x7Eu, 0x02u, 0x07u, 0xC1u, 0x03u, 0x00u, 0x00u, 0x46u}},
    {0x0Du, 0x00u, {0x7Eu, 0x02u, 0x0Du, 0xC1u, 0x00u, 0x00u, 0x00u, 0x60u}},
    {0x0Du, 0x01u, {0x7Eu, 0x02u, 0x0Du, 0xC1u, 0x01u, 0x00u, 0x00u, 0x08u}},
    {0x0Du, 0x02u, {0x7Eu, 0x02u, 0x0Du, 0xC1u, 0x02u, 0x00u, 0x00u, 0x6Au}},
    {0x0Du, 0x03u, {0x7Eu, 0x02u, 0x0Du, 0xC1u, 0x03u, 0x00u, 0x00u, 0x80u}},
    {0x0Du, 0x04u, {0x7Eu, 0x02u, 0x0Du, 0xC1u, 0x04u, 0x00u, 0x00u, 0x66u}},
    {0x0Du, 0x05u, {0x7Eu, 0x02u, 0x0Du, 0xC1u, 0x05u, 0x00u, 0x00u, 0x06u}},
    {0x0Eu, 0x00u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x00u, 0x00u, 0x00u, 0x68u}},
    {0x0Eu, 0x01u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x01u, 0x00u, 0x00u, 0x6Au}},
    {0x0Eu, 0x02u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x02u, 0x00u, 0x00u, 0x60u}},
    {0x0Eu, 0x03u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x03u, 0x00u, 0x00u, 0xE6u}},
    {0x0Eu, 0x04u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x04u, 0x00u, 0x00u, 0xE6u}},
    {0x0Eu, 0x05u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x05u, 0x00u, 0x00u, 0xEEu}},
    {0x0Eu, 0x06u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x06u, 0x00u, 0x00u, 0xE4u}},
    {0x0Eu, 0x07u, {0x7Eu, 0x02u, 0x0Eu, 0xC1u, 0x07u, 0x00u, 0x00u, 0x64u}},
    {0x0Fu, 0x00u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x00u, 0x00u, 0x00u, 0x6Au}},
    {0x0Fu, 0x01u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x01u, 0x00u, 0x00u, 0xE0u}},
    {0x0Fu, 0x02u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x02u, 0x00u, 0x00u, 0xE6u}},
    {0x0Fu, 0x03u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x03u, 0x00u, 0x00u, 0xE6u}},
    {0x0Fu, 0x04u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x04u, 0x00u, 0x00u, 0xEEu}},
    {0x0Fu, 0x05u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x05u, 0x00u, 0x00u, 0xE4u}},
    {0x0Fu, 0x06u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x06u, 0x00u, 0x00u, 0xE4u}},
    {0x0Fu, 0x07u, {0x7Eu, 0x02u, 0x0Fu, 0xC1u, 0x07u, 0x00u, 0x00u, 0xEAu}},
    {0x11u, 0x01u, {0x7Eu, 0x02u, 0x11u, 0xC1u, 0x01u, 0x00u, 0x00u, 0x86u}},
    {0x11u, 0x00u, {0x7Eu, 0x02u, 0x11u, 0xC1u, 0x00u, 0x00u, 0x00u, 0x06u}},
    {0x12u, 0x02u, {0x7Eu, 0x02u, 0x12u, 0xC1u, 0x02u, 0x00u, 0x00u, 0xE4u}},
    {0x12u, 0x03u, {0x7Eu, 0x02u, 0x12u, 0xC1u, 0x03u, 0x00u, 0x00u, 0xA4u}},
    {0x12u, 0x00u, {0x7Eu, 0x02u, 0x12u, 0xC1u, 0x00u, 0x00u, 0x00u, 0x46u}},
    {0x12u, 0x01u, {0x7Eu, 0x02u, 0x12u, 0xC1u, 0x01u, 0x00u, 0x00u, 0x6Eu}}
};

static const uint8_t SwitchBacklightOffFrames[SWITCH_BACKLIGHT_OFF_FRAME_COUNT][SWITCH_LED_FRAME_LENGTH] = {
    {0x7Eu, 0x02u, 0x07u, 0x41u, 0x01u, 0x00u, 0x00u, 0x4Eu},
    {0x7Eu, 0x02u, 0x0Du, 0x41u, 0x00u, 0x00u, 0x00u, 0xEAu},
    {0x7Eu, 0x02u, 0x0Eu, 0x41u, 0x00u, 0x00u, 0x00u, 0xEAu},
    {0x7Eu, 0x02u, 0x0Fu, 0x41u, 0x00u, 0x00u, 0x00u, 0xAEu},
    {0x7Eu, 0x02u, 0x11u, 0x41u, 0x01u, 0x00u, 0x00u, 0xAEu},
    {0x7Eu, 0x02u, 0x12u, 0x41u, 0x02u, 0x00u, 0x00u, 0xE4u}
};

static bool SwitchLed_CopyFrameFromTable(const SWITCH_LED_AHF_FRAME *table, uint8_t tableCount, uint8_t *frame, uint8_t panelAddress, uint8_t state) {
    uint8_t i;

    for (i = 0u; i < tableCount; i++) {
        if ((table[i].panelAddress == panelAddress)
         && (table[i].state == state)) {
            uint8_t j;
            for (j = 0u; j < SWITCH_LED_FRAME_LENGTH; j++) {
                frame[j] = table[i].frame[j];
            }
            return true;
        }
    }

    return false;
}

static bool SwitchLed_CopyAhfFrame(uint8_t *frame, uint8_t panelAddress, uint8_t state) {
    return SwitchLed_CopyFrameFromTable(SwitchLedAhfFrames,
                                        SWITCH_AHF_TABLE_COUNT,
                                        frame,
                                        panelAddress,
                                        state);
}

static bool SwitchLed_StartRawFrame(const uint8_t *frame, SWITCH_LED_TX_KIND kind, uint8_t panelIndex, uint8_t panelValue) {
    uint8_t i;

    if (SwitchLedState.tx.active == true) {
        return false;
    }

    for (i = 0u; i < SWITCH_LED_FRAME_LENGTH; i++) {
        SwitchLedState.tx.frame[i] = frame[i];
    }

    SwitchLedState.tx.length = SWITCH_LED_FRAME_LENGTH;
    SwitchLedState.tx.index = 0u;
    SwitchLedState.tx.panelIndex = panelIndex;
    SwitchLedState.tx.panelValue = panelValue;
    SwitchLedState.tx.kind = kind;
    SwitchLedState.tx.active = true;

    DIR_SetHigh();
    return true;
}

static bool SwitchLed_StartFrame(uint8_t panelAddress, uint8_t value, SWITCH_LED_TX_KIND kind, uint8_t panelIndex) {
    uint8_t frame[SWITCH_LED_FRAME_LENGTH];

    if (SwitchLedState.tx.active == true) {
        return false;
    }

    if (!SwitchLed_CopyAhfFrame(frame,
                                panelAddress,
                                (uint8_t)(value & SWITCH_LED_ALL_KEYS_MASK))) {
        if (panelIndex < SWITCH_LED_MAX_PANELS) {
            SwitchLedState.panels[panelIndex].lastSentState = value;
            SwitchLedState.panels[panelIndex].pending = false;
        }
        return false;
    }

    return SwitchLed_StartRawFrame(frame, kind, panelIndex, (uint8_t)(value & SWITCH_LED_ALL_KEYS_MASK));
}

static void SwitchLed_FinishTransmission(void) {
    SWITCH_LED_PANEL_STATE *panelState;

    DIR_SetLow();

    switch (SwitchLedState.tx.kind) {
        case SWITCH_LED_TX_KIND_PANEL_UPDATE:
            if (SwitchLedState.tx.panelIndex < SWITCH_LED_MAX_PANELS) {
                panelState = &SwitchLedState.panels[SwitchLedState.tx.panelIndex];
                panelState->lastSentState = SwitchLedState.tx.panelValue;
                panelState->pending = (panelState->currentState != SwitchLedState.tx.panelValue);
                SwitchLedState.nextPanelIndex = (uint8_t)((SwitchLedState.tx.panelIndex + 1u) % SWITCH_LED_MAX_PANELS);
            }
            break;

        case SWITCH_LED_TX_KIND_BACKLIGHT_OFF:
            SwitchLedState.backlightOffIndex++;
            if (SwitchLedState.backlightOffIndex >= SWITCH_BACKLIGHT_OFF_FRAME_COUNT) {
                SwitchLedState.backlightOffPending = false;
                SwitchLedState.backlightOffIndex = 0u;
            }
            break;

        default:
            break;
    }

    SwitchLedState.tx.active = false;
    SwitchLedState.tx.kind = SWITCH_LED_TX_KIND_NONE;
    SwitchLedState.cooldownTicks = SwitchLedState.sendIntervalTicks;
}

static void SwitchLed_AbortTransmission(void) {
    DIR_SetLow();
    SwitchLedState.tx.active = false;
    SwitchLedState.tx.kind = SWITCH_LED_TX_KIND_NONE;
    SwitchLedState.cooldownTicks = SwitchLedState.sendIntervalTicks;
}

static uint16_t SwitchLed_KeyMaskFromNumber(uint8_t keyNumber) {
    if ((keyNumber == 0u) || (keyNumber > 6u)) {
        return 0u;
    }

    return (uint16_t)(1u << (keyNumber - 1u));
}

static SWITCH_LED_PANEL_STATE *SwitchLed_FindPanelState(uint8_t panelAddress, bool createIfMissing) {
    uint8_t index;
    SWITCH_LED_PANEL_STATE *freeSlot = (SWITCH_LED_PANEL_STATE *)0;

    if (panelAddress == 0u) {
        return (SWITCH_LED_PANEL_STATE *)0;
    }

    for (index = 0u; index < SWITCH_LED_MAX_PANELS; index++) {
        if (SwitchLedState.panels[index].panelAddress == panelAddress) {
            return &SwitchLedState.panels[index];
        }
        if ((createIfMissing == true)
         && (freeSlot == (SWITCH_LED_PANEL_STATE *)0)
         && (SwitchLedState.panels[index].panelAddress == 0u)) {
            freeSlot = &SwitchLedState.panels[index];
        }
    }

    if ((createIfMissing == true) && (freeSlot != (SWITCH_LED_PANEL_STATE *)0)) {
        freeSlot->panelAddress = panelAddress;
        freeSlot->currentState = 0u;
        freeSlot->lastSentState = 0u;
        freeSlot->pending = false;
    }

    return freeSlot;
}

static void SwitchLed_ClearAllPanelStates(void) {
    uint8_t index;

    for (index = 0u; index < SWITCH_LED_MAX_PANELS; index++) {
        SwitchLedState.panels[index].panelAddress = 0u;
        SwitchLedState.panels[index].currentState = 0u;
        SwitchLedState.panels[index].lastSentState = 0u;
        SwitchLedState.panels[index].pending = false;
    }
}

/* Initialize the runtime-only LED controller.
 * No panel configuration is written to EEPROM here. */
void SwitchLed_Init(const SWITCH_LED_CONFIG *config) {
    SwitchLedState.sendIntervalTicks = SWITCH_LED_DEFAULT_INTERVAL_TICKS;
    SwitchLedState.cooldownTicks = 0u;
    SwitchLedState.nextPanelIndex = 0u;
    SwitchLedState.initialized = true;
    SwitchLedState.backlightOffPending = false;
    SwitchLedState.backlightOffIndex = 0u;
    SwitchLedState.tx.length = 0u;
    SwitchLedState.tx.index = 0u;
    SwitchLedState.tx.panelIndex = 0u;
    SwitchLedState.tx.panelValue = 0u;
    SwitchLedState.tx.kind = SWITCH_LED_TX_KIND_NONE;
    SwitchLedState.tx.active = false;
    PIE1bits.TXIE = 0;
    SwitchLed_ClearAllPanelStates();

    if (config != (const SWITCH_LED_CONFIG *)0) {
        if (config->sendIntervalTicks != 0u) {
            SwitchLedState.sendIntervalTicks = config->sendIntervalTicks;
        }
    }
}

static void SwitchLed_SetPanelState(uint8_t panelAddress, uint8_t ledState) {
    SWITCH_LED_PANEL_STATE *panelState = SwitchLed_FindPanelState(panelAddress, true);

    if (panelState == (SWITCH_LED_PANEL_STATE *)0) {
        return;
    }

    panelState->currentState = (uint8_t)(ledState & SWITCH_LED_ALL_KEYS_MASK);
    /* Mark only changed panels so Process() can skip the rest. */
    if (panelState->lastSentState != panelState->currentState) {
        panelState->pending = true;
    }
}

static uint8_t SwitchLed_GetPanelState(uint8_t panelAddress) {
    SWITCH_LED_PANEL_STATE *panelState = SwitchLed_FindPanelState(panelAddress, false);

    if (panelState == (SWITCH_LED_PANEL_STATE *)0) {
        return 0u;
    }

    return panelState->currentState;
}

void SwitchLed_SetPanelKey(uint8_t panelAddress, uint8_t keyNumber, bool enabled) {
    uint8_t keyMask = (uint8_t)SwitchLed_KeyMaskFromNumber(keyNumber);
    uint8_t nextState;

    if (keyMask == 0u) {
        return;
    }

    nextState = SwitchLed_GetPanelState(panelAddress);

    if (enabled) {
        nextState |= keyMask;
    } else {
        nextState &= (uint8_t)~keyMask;
    }

    SwitchLed_SetPanelState(panelAddress, nextState);
}

void SwitchLed_RequestRefresh(void) {
    uint8_t index;

    for (index = 0u; index < SWITCH_LED_MAX_PANELS; index++) {
        if (SwitchLedState.panels[index].panelAddress != 0u) {
            SwitchLedState.panels[index].pending = true;
        }
    }
}

void SwitchLed_RequestBacklightOff(void) {
    SwitchLedState.backlightOffPending = true;
    SwitchLedState.backlightOffIndex = 0u;
}

void SwitchLed_TimerTick(void) {
    if (SwitchLedState.cooldownTicks > 0u) {
        SwitchLedState.cooldownTicks--;
    }
}

static void SwitchLed_ProcessTx(void) {
    uint16_t waitCount;

    if (SwitchLedState.tx.active == false) {
        return;
    }

    while (SwitchLedState.tx.index < SwitchLedState.tx.length) {
        waitCount = 0u;
        while (EUSART_is_tx_ready() == false) {
            waitCount++;
            if (waitCount >= SWITCH_LED_TX_WAIT_LIMIT) {
                SwitchLed_AbortTransmission();
                return;
            }
        }
        TX1REG = SwitchLedState.tx.frame[SwitchLedState.tx.index++];
    }

    waitCount = 0u;
    while (EUSART_is_tx_done() == false) {
        waitCount++;
        if (waitCount >= SWITCH_LED_TX_WAIT_LIMIT) {
            SwitchLed_AbortTransmission();
            return;
        }
    }

    SwitchLed_FinishTransmission();
}

bool SwitchLed_IsBusy(void) {
    return (SwitchLedState.tx.active == true);
}

bool SwitchLed_HasPendingWork(void) {
    uint8_t index;

    if (SwitchLedState.tx.active == true) {
        return true;
    }

    if (SwitchLedState.backlightOffPending == true) {
        return true;
    }

    for (index = 0u; index < SWITCH_LED_MAX_PANELS; index++) {
        if ((SwitchLedState.panels[index].panelAddress != 0u)
         && (SwitchLedState.panels[index].pending == true)) {
            return true;
        }
    }

    return false;
}

void SwitchLed_Process(void) {
    uint8_t offset;
    uint8_t panelIndex;
    SWITCH_LED_PANEL_STATE *panelState;

    if (SwitchLedState.initialized == false) {
        return;
    }

    SwitchLed_ProcessTx();
    if (SwitchLedState.tx.active == true) {
        return;
    }

    if ((SwitchLedState.cooldownTicks != 0u)
     || EUSART_is_rx_ready()) {
        return;
    }

    /* Normal path: send one changed panel per call, then return control to main.c. */
    for (offset = 0u; offset < SWITCH_LED_MAX_PANELS; offset++) {
        panelIndex = (uint8_t)((SwitchLedState.nextPanelIndex + offset) % SWITCH_LED_MAX_PANELS);
        panelState = &SwitchLedState.panels[panelIndex];

        if ((panelState->panelAddress != 0u) && (panelState->pending == true)) {
            (void)SwitchLed_StartFrame(
                panelState->panelAddress,
                panelState->currentState,
                SWITCH_LED_TX_KIND_PANEL_UPDATE,
                panelIndex
            );
            return;
        }
    }

    if (SwitchLedState.backlightOffPending == true) {
        (void)SwitchLed_StartRawFrame(
            SwitchBacklightOffFrames[SwitchLedState.backlightOffIndex],
            SWITCH_LED_TX_KIND_BACKLIGHT_OFF,
            SWITCH_LED_MAX_PANELS,
            0u
        );
        return;
    }
}
