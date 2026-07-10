#ifndef THERMOSTAT_PANEL_H
#define THERMOSTAT_PANEL_H

#include <stdbool.h>
#include <stdint.h>

#define THERMOSTAT_PANEL_DEFAULT_ADDRESS 0x3Cu

#define THERMOSTAT_PANEL_MODE_COOL       0u
#define THERMOSTAT_PANEL_MODE_HEAT       1u
#define THERMOSTAT_PANEL_MODE_FAN        2u
#define THERMOSTAT_PANEL_MODE_DRY        3u

typedef struct {
    bool power;
    uint8_t mode;
    uint8_t fanSpeed;
    uint8_t tempSetC;
    uint8_t roomTempByte;
} THERMOSTAT_PANEL_STATE;

typedef enum {
    THERMOSTAT_PANEL_EVENT_NONE = 0u,
    THERMOSTAT_PANEL_EVENT_POWER,
    THERMOSTAT_PANEL_EVENT_MODE,
    THERMOSTAT_PANEL_EVENT_FAN_SPEED,
    THERMOSTAT_PANEL_EVENT_ROOM_TEMP
} THERMOSTAT_PANEL_EVENT_TYPE;

typedef struct {
    uint8_t address;
    uint8_t function;
    uint16_t registerAddress;
    uint16_t value;
    THERMOSTAT_PANEL_EVENT_TYPE type;
    THERMOSTAT_PANEL_STATE state;
} THERMOSTAT_PANEL_EVENT;

void ThermostatPanel_Init(void);

void ThermostatPanel_ReceiveByte(uint8_t data);
bool ThermostatPanel_Poll(THERMOSTAT_PANEL_EVENT *event);

void ThermostatPanel_RequestPower(bool enabled);
void ThermostatPanel_RequestPowerTemp(bool enabled, uint8_t tempSetC);
void ThermostatPanel_RequestPowerFanSpeedTemp(bool enabled, uint8_t fanSpeed, uint8_t tempSetC);
void ThermostatPanel_RequestMode(uint8_t mode);
void ThermostatPanel_RequestFanSpeedTemp(uint8_t fanSpeed, uint8_t tempSetC);
void ThermostatPanel_TimerTick(void);
void ThermostatPanel_Process(void);
void ThermostatPanel_MarkBusActivity(void);
bool ThermostatPanel_IsBusReserved(void);
bool ThermostatPanel_IsTxLocked(void);

#endif /* THERMOSTAT_PANEL_H */
