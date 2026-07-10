#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../mcc_generated_files/mcc.h"
#include "switch_input.h"

#define SWITCH_FRAME_LENGTH 8u             /* AHF-V4 fixed frame length */
#define SWITCH_KEY_QUEUE_LENGTH 4u         /* Buffered key events waiting for main loop */
#define SWITCH_AHF_SLEEP_HEADER       0x7Fu
#define SWITCH_AHF_KEYCARD_TYPE       0x01u
#define SWITCH_AHF_KEYCARD_REMOVED    0x02u
#define SWITCH_AHF_KEYCARD_PRESENT    0x03u
#define SWITCH_AHF_SERVICE_SW1_EVENT  0x03u
#define SWITCH_AHF_SERVICE_SW2_EVENT  0x05u
#define SWITCH_AHF_SERVICE_SW3_EVENT  0x07u
#define SWITCH_PANEL_KEYCARD_ADDRESS  0x01u
#define SWITCH_PANEL_ETH_ADDRESS      0x31u
#define SWITCH_PANEL_07_ADDRESS       0x07u
#define SWITCH_PANEL_SERVICE_ADDRESS  0x0Du
#define SWITCH_PANEL_3GANG_0E_ADDRESS 0x0Eu
#define SWITCH_PANEL_3GANG_0F_ADDRESS 0x0Fu
#define SWITCH_PANEL_MASTER_11_ADDRESS 0x11u
#define SWITCH_PANEL_MASTER_12_ADDRESS 0x12u

#define SWITCH_PANEL_KEY_1            1u
#define SWITCH_PANEL_KEY_2            2u
#define SWITCH_PANEL_KEY_3            3u
#define LED_PANEL_KEY_1               1u
#define LED_PANEL_KEY_2               2u
#define LED_PANEL_KEY_3               3u
#define SWITCH_KEYBOX_CARD_NONE       0u
#define SWITCH_KEYBOX_CARD_GUEST      1u

#define SWITCH_KEY_IN1              Key1
#define SWITCH_KEY_IN2              Key2
#define SWITCH_KEY_IN3              Key3
#define SWITCH_KEY_IN4              Key4
#define SWITCH_KEY_IN5              Key5
#define SWITCH_KEY_IN6              Key6
#define SWITCH_KEY_IN7              Key7
#define SWITCH_KEY_IN8              Key8
#define SWITCH_KEY_IN9              Key9
#define SWITCH_KEY_IN10             Key10
#define SWITCH_KEY_IN11             Key11
#define SWITCH_KEY_MASTER1          Key29
#define SWITCH_KEY_DND              Key30
#define SWITCH_KEY_MUR              Key31
typedef struct {
    uint8_t header;
    uint8_t type;
    uint8_t source;
    uint8_t command;
} SWITCH_FRAME;

typedef struct {
    uint8_t panel_address;
    uint8_t panel_key_number;
    uint8_t led_key_number;
    uint8_t internal_key;
} SWITCH_KEY_MAP;

typedef struct {
    uint8_t raw[SWITCH_FRAME_LENGTH];
    uint8_t count;
    uint8_t frameAvailable;
    uint8_t keyQueue[SWITCH_KEY_QUEUE_LENGTH];
    uint8_t keyHead;
    uint8_t keyTail;
    uint8_t keyCount;
    uint8_t keyboxCardType;
    uint8_t keyboxAvailable;
    SWITCH_FRAME frame;
} SWITCH_INPUT_STATE;

/* Map format: {panel_address, panel_key_number, led_key_number, internal_key}
 * panel_key_number is the key value reported by the panel when pressed.
 * led_key_number is the LED number to command back to the same panel.
 * Thai note: บางแผงเลข key ที่ส่งเข้ามา กับเลข LED ที่ต้องสั่งกลับ ไม่ตรงกัน
 * จึงต้องแยก panel_key_number และ led_key_number ออกจากกัน
 *
 * New workflow:
 * 1. Poll frame from UART.
 * 2. Match panel_address + panel_key_number in this table.
 * 3. Return internal_key to main.c.
 * 4. Let main.c decide output state.
 * 5. Map the same internal_key back to panel LED state when syncing LEDs. */
static const SWITCH_KEY_MAP SwitchKeyMap[] = {
    {0x07u, SWITCH_PANEL_KEY_1, LED_PANEL_KEY_1, SWITCH_KEY_MASTER1},  /* Panel 07 SW1 Master */
    {0x07u, SWITCH_PANEL_KEY_2, LED_PANEL_KEY_2, SWITCH_KEY_IN5},      /* Panel 07 SW2 Bathroom */
    {0x0Du, SWITCH_PANEL_KEY_1, LED_PANEL_KEY_1, SWITCH_KEY_IN8},      /* Panel 0D SW1 Entrance */
    {0x0Du, SWITCH_PANEL_KEY_2, LED_PANEL_KEY_2, SWITCH_KEY_DND},      /* Panel 0D SW2 DND */
    {0x0Du, SWITCH_PANEL_KEY_3, LED_PANEL_KEY_3, SWITCH_KEY_MUR},      /* Panel 0D SW3 MUR */
    {SWITCH_PANEL_ETH_ADDRESS, SWITCH_PANEL_KEY_2, LED_PANEL_KEY_2, SWITCH_KEY_DND}, /* ETH SW2 DND */
    {SWITCH_PANEL_ETH_ADDRESS, SWITCH_PANEL_KEY_3, LED_PANEL_KEY_3, SWITCH_KEY_MUR}, /* ETH SW3 MUR */
    {0x0Eu, SWITCH_PANEL_KEY_1, LED_PANEL_KEY_1, SWITCH_KEY_IN7},      /* Panel 0E SW1 Ceiling-2 */
    {0x0Eu, SWITCH_PANEL_KEY_2, LED_PANEL_KEY_2, SWITCH_KEY_IN6},      /* Panel 0E SW2 Ceiling-1 */
    {0x0Eu, SWITCH_PANEL_KEY_3, LED_PANEL_KEY_3, SWITCH_KEY_IN9},      /* Panel 0E SW3 Reading-L */
    {0x0Fu, SWITCH_PANEL_KEY_1, LED_PANEL_KEY_1, SWITCH_KEY_IN10},     /* Panel 0F SW1 Reading-R */
    {0x0Fu, SWITCH_PANEL_KEY_2, LED_PANEL_KEY_2, SWITCH_KEY_IN6},      /* Panel 0F SW2 Ceiling-1 */
    {0x0Fu, SWITCH_PANEL_KEY_3, LED_PANEL_KEY_3, SWITCH_KEY_IN7},      /* Panel 0F SW3 Ceiling-2 */
    {0x11u, SWITCH_PANEL_KEY_1, LED_PANEL_KEY_1, SWITCH_KEY_MASTER1},  /* Panel 11 SW1 Master */
    {0x12u, SWITCH_PANEL_KEY_1, LED_PANEL_KEY_1, SWITCH_KEY_IN11},     /* Panel 12 SW1 Bedside */
    {0x12u, SWITCH_PANEL_KEY_2, LED_PANEL_KEY_2, SWITCH_KEY_MASTER1},  /* Panel 12 SW2 Master */
};

static SWITCH_INPUT_STATE SwitchInputState;

static bool SwitchInput_PopFrame(SWITCH_FRAME *frame);
static bool SwitchInput_PushKey(uint8_t key);
static bool SwitchInput_TryResolveKeyboxFrame(const SWITCH_FRAME *frame, uint8_t *cardType);
static bool SwitchInput_TryMapPanelKeyNumberToKey(uint8_t source, uint8_t panelKeyNumber, uint8_t *key);
static bool SwitchInput_TryResolveFrameKey(const SWITCH_FRAME *frame, uint8_t *key);

static void SwitchInput_ShiftLeft(void) {
    uint8_t i;

    for (i = 1u; i < SWITCH_FRAME_LENGTH; i++) {
        SwitchInputState.raw[i - 1u] = SwitchInputState.raw[i];
    }

    SwitchInputState.count = (uint8_t)(SWITCH_FRAME_LENGTH - 1u);
}

static bool SwitchInput_TryAssembleFrame(void) {
    if (SwitchInputState.count < SWITCH_FRAME_LENGTH) {
        return false;
    }

    if (SwitchInputState.raw[0] != SWITCH_AHF_SLEEP_HEADER) {
        return false;
    }

    SwitchInputState.frame.header = SwitchInputState.raw[0];
    SwitchInputState.frame.type = SwitchInputState.raw[1];
    SwitchInputState.frame.source = SwitchInputState.raw[2];
    SwitchInputState.frame.command = SwitchInputState.raw[3];
    SwitchInputState.frameAvailable = 1u;
    SwitchInputState.count = 0u;
    return true;
}

void SwitchInput_ReceiveByte(uint8_t data) {
    SWITCH_FRAME frame;
    uint8_t mappedKey;
    uint8_t cardType;

    if (SwitchInputState.count >= SWITCH_FRAME_LENGTH) {
        SwitchInput_ShiftLeft();
    }

    SwitchInputState.raw[SwitchInputState.count++] = data;

    if (SwitchInput_TryAssembleFrame()) {
        if (SwitchInput_PopFrame(&frame)) {
            if (SwitchInput_TryResolveKeyboxFrame(&frame, &cardType)) {
                SwitchInputState.keyboxCardType = cardType;
                SwitchInputState.keyboxAvailable = 1u;
            } else if (SwitchInput_TryResolveFrameKey(&frame, &mappedKey)) {
                (void)SwitchInput_PushKey(mappedKey);
            }
        }
        return;
    }

    if (SwitchInputState.count >= SWITCH_FRAME_LENGTH) {
        SwitchInput_ShiftLeft();
    }
}

static bool SwitchInput_TryResolveKeyboxFrame(const SWITCH_FRAME *frame, uint8_t *cardType) {
    if ((frame == NULL) || (cardType == NULL)) {
        return false;
    }

    if ((frame->source != SWITCH_PANEL_KEYCARD_ADDRESS)
     && (frame->source != SWITCH_PANEL_ETH_ADDRESS)) {
        return false;
    }

    if ((frame->header == SWITCH_AHF_SLEEP_HEADER)
     && (frame->type == SWITCH_AHF_KEYCARD_TYPE)
     && (frame->command == SWITCH_AHF_KEYCARD_PRESENT)) {
        *cardType = SWITCH_KEYBOX_CARD_GUEST;
        return true;
    }

    if ((frame->header == SWITCH_AHF_SLEEP_HEADER)
     && (frame->type == SWITCH_AHF_KEYCARD_TYPE)
     && (frame->command == SWITCH_AHF_KEYCARD_REMOVED)) {
        *cardType = SWITCH_KEYBOX_CARD_NONE;
        return true;
    }

    return false;
}

static bool SwitchInput_TryResolveFrameKey(const SWITCH_FRAME *frame, uint8_t *key) {
    uint8_t panelKeyNumber = 0u;

    if ((frame == NULL) || (key == NULL)) {
        return false;
    }

    if ((frame->header == SWITCH_AHF_SLEEP_HEADER)
     && (frame->type == SWITCH_AHF_KEYCARD_TYPE)
     && ((frame->source == SWITCH_PANEL_07_ADDRESS)
      || (frame->source == SWITCH_PANEL_SERVICE_ADDRESS)
      || (frame->source == SWITCH_PANEL_3GANG_0E_ADDRESS)
      || (frame->source == SWITCH_PANEL_3GANG_0F_ADDRESS)
      || (frame->source == SWITCH_PANEL_MASTER_11_ADDRESS)
      || (frame->source == SWITCH_PANEL_MASTER_12_ADDRESS)
      || (frame->source == SWITCH_PANEL_ETH_ADDRESS))) {
        if (frame->command == SWITCH_AHF_SERVICE_SW1_EVENT) {
            panelKeyNumber = SWITCH_PANEL_KEY_1;
        } else if (frame->command == SWITCH_AHF_SERVICE_SW2_EVENT) {
            panelKeyNumber = SWITCH_PANEL_KEY_2;
        } else if (frame->command == SWITCH_AHF_SERVICE_SW3_EVENT) {
            panelKeyNumber = SWITCH_PANEL_KEY_3;
        }

        if (panelKeyNumber != 0u) {
            return SwitchInput_TryMapPanelKeyNumberToKey(frame->source, panelKeyNumber, key);
        }
    }

    return false;
}

static bool SwitchInput_PopFrame(SWITCH_FRAME *frame) {
    if ((frame == NULL) || (SwitchInputState.frameAvailable == 0u)) {
        return false;
    }

    *frame = SwitchInputState.frame;
    SwitchInputState.frameAvailable = 0u;
    return true;
}

static bool SwitchInput_PushKey(uint8_t key) {
    SwitchInputState.keyQueue[0] = key;
    SwitchInputState.keyHead = 0u;
    SwitchInputState.keyTail = 0u;
    SwitchInputState.keyCount = 1u;
    return true;
}

static bool SwitchInput_PopKey(uint8_t *key) {
    if ((key == NULL) || (SwitchInputState.keyCount == 0u)) {
        return false;
    }

    *key = SwitchInputState.keyQueue[0];
    SwitchInputState.keyHead = 0u;
    SwitchInputState.keyTail = 0u;
    SwitchInputState.keyCount = 0u;
    return true;
}

bool SwitchInput_PollKeybox(uint8_t *cardType) {
    if ((cardType == NULL) || (SwitchInputState.keyboxAvailable == 0u)) {
        return false;
    }

    *cardType = SwitchInputState.keyboxCardType;
    SwitchInputState.keyboxAvailable = 0u;
    return true;
}

/* Look up the internal key by panel address and the panel-reported key number.
 * Thai note: ถ้า address และลำดับปุ่มตรงกับในตาราง จึงยอมรับว่าเป็น key นี้ */
static bool SwitchInput_TryMapPanelKeyNumberToKey(uint8_t source, uint8_t panelKeyNumber, uint8_t *key) {
    uint8_t index;

    if (key == NULL) {
        return false;
    }

    for (index = 0u; index < (uint8_t)(sizeof(SwitchKeyMap) / sizeof(SwitchKeyMap[0])); index++) {
        if ((SwitchKeyMap[index].panel_address == source)
         && (SwitchKeyMap[index].panel_key_number == panelKeyNumber)) {
            *key = SwitchKeyMap[index].internal_key;
            return true;
        }
    }

    return false;
}

void SwitchInput_Init(void) {
    uint8_t i;

    for (i = 0u; i < SWITCH_FRAME_LENGTH; i++) {
        SwitchInputState.raw[i] = 0u;
    }

    SwitchInputState.count = 0u;
    SwitchInputState.frameAvailable = 0u;
    SwitchInputState.frame.header = 0u;
    SwitchInputState.frame.type = 0u;
    SwitchInputState.frame.source = 0u;
    SwitchInputState.frame.command = 0u;
    SwitchInputState.keyHead = 0u;
    SwitchInputState.keyTail = 0u;
    SwitchInputState.keyCount = 0u;
    SwitchInputState.keyboxCardType = SWITCH_KEYBOX_CARD_NONE;
    SwitchInputState.keyboxAvailable = 0u;
}

bool SwitchInput_GetPanelMappingDetails(uint8_t key, uint8_t occurrence, uint8_t *panelAddress, uint8_t *panelKeyNumber) {
    uint8_t index;
    uint8_t matchIndex = 0u;

    if ((panelAddress == NULL) || (panelKeyNumber == NULL)) {
        return false;
    }

    for (index = 0u; index < (uint8_t)(sizeof(SwitchKeyMap) / sizeof(SwitchKeyMap[0])); index++) {
        if (SwitchKeyMap[index].internal_key == key) {
            if (matchIndex == occurrence) {
                *panelAddress = SwitchKeyMap[index].panel_address;
                *panelKeyNumber = SwitchKeyMap[index].led_key_number;
                return true;
            }
            matchIndex++;
        }
    }

    return false;
}

bool SwitchInput_Poll(uint8_t *key) {
    if (key == NULL) {
        return false;
    }

    return SwitchInput_PopKey(key);
}
