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
struct global_param
{
    int16_t BPM;
    int16_t AVG_BPM;
    bool show_heart;
    int step_cntr;
    enum BATTERY_STATE_ENUM battery_state;
};

extern struct global_param global_parameters;

#endif