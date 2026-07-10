#ifndef SWITCH_INPUT_H
#define SWITCH_INPUT_H

#include <stdbool.h>
#include <stdint.h>

/* KEYBOARD */
#define	Key1		0x00			//None
#define	Key2		0x01			//None
#define	Key3		0x02			//None
#define	Key4		0x03			//None
#define	Key5		0x04			//None
#define	Key6		0x05			//None
#define	Key7		0x06			//None
#define	Key8		0x07			//None
#define	Key9		0x08			//None
#define	Key10		0x09			//None
#define	Key11		0x0a			//None
#define	Key12		0x0b			//None
#define	Key13		0x0c			//None
#define	Key14		0x0d			//None
#define	Key15		0x0e			//None
#define	Key16		0x0f			//None
#define	Key17		0x10			//None
#define	Key18		0x11			//None
#define	Key19		0x12			//None
#define	Key20		0x13			//None
#define	Key21		0x14			//None
#define	Key22		0x15			//None
#define	Key23		0x16			//None
#define	Key24		0x17			//None
#define	Key25		0x18			//None
#define	Key26		0x19			//None
#define	Key27		0x20			//None
#define	Key28		0x21			//None
#define	Key29		0x22			//None
#define	Key30		0x23			//None
#define	Key31		0x24			//None
#define	Key32		0x25			//None
#define	Keyxx		0x26			//None

/* Switch input model
 * - Incoming frames are matched by panel address + panel key number.
 * - The map can translate one panel key into a different LED key number.
 * - Higher-level logic uses internal keys (Key1..Key42 style IDs) only. */
void SwitchInput_Init(void);
void SwitchInput_ReceiveByte(uint8_t data);

/* Keybox/keycard panel event from AHF-V4 address 0x01.
 * cardType matches the legacy main.c CARD_TYPE numeric values:
 * 0 = NONE, 1 = GUEST. */
bool SwitchInput_PollKeybox(uint8_t *cardType);

/* Mapping helpers for multi-panel LED sync in main.c. */
bool SwitchInput_GetPanelMappingDetails(uint8_t key, uint8_t occurrence, uint8_t *panelAddress, uint8_t *panelKeyNumber);

/* Poll one decoded key event from the switch queue.
 * Returns true only when the frame matches an entry in SwitchKeyMap[]. */
bool SwitchInput_Poll(uint8_t *key);

#endif /* SWITCH_INPUT_H */
