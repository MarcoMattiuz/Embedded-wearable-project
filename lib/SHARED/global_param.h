#ifndef __GLOBAL_PARAM_H__
#define __GLOBAL_PARAM_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum BATTERY_STATE_ENUM
{
    BATTERY_EMPTY = 0,
    BATTERY_LOW = 25,
    BATTERY_MEDIUM = 50,
    BATTERY_HIGH = 75,
    BATTERY_FULL = 100
};

typedef enum
{
    STATE_BPM,
    STATE_CLOCK,
    STATE_WEATHER,
    STATE_STEPS,
    STATE_BATTERY,
    STATE_CO2,
    STATE_COUNT,

} State_t;

typedef enum
{
    SUNNY,
    CLOUDY,
    RAINY,
    FOGGY,
    SNOWY,
    THUNDERSTORM,

} WeatherType;

typedef enum
{
    EVT_BUTTON_EDGE,
    EVT_LONG_PRESS,
    EVT_REFRESH,
    EVT_TURN_ON_DISPLAY
} EventType;

struct global_param
{
    int16_t BPM;
    int16_t AVG_BPM;
    bool show_heart;
    int step_cntr;
    enum BATTERY_STATE_ENUM battery_state;
    WeatherType weather;
    float temperature;
    char date[9];
    char time_str[6];
    int CO2;
};

extern struct global_param global_parameters;

#endif