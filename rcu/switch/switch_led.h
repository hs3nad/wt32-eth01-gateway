#ifndef SWITCH_LED_H
#define SWITCH_LED_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t sendIntervalTicks;
} SWITCH_LED_CONFIG;

#define SWITCH_LED_MAX_PANELS 8u

/* Switch LED model
 * - Each panel keeps its own cached LED state.
 * - Only panels whose state changed are transmitted.
 * - One call to SwitchLed_Process() sends at most one full packet for one panel. */
void SwitchLed_Init(const SWITCH_LED_CONFIG *config);

void SwitchLed_SetPanelKey(uint8_t panelAddress, uint8_t keyNumber, bool enabled);
void SwitchLed_RequestRefresh(void);
void SwitchLed_RequestBacklightOff(void);

void SwitchLed_TimerTick(void);
void SwitchLed_Process(void);
bool SwitchLed_IsBusy(void);
bool SwitchLed_HasPendingWork(void);

#endif /* SWITCH_LED_H */
