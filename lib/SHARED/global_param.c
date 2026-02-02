#include "global_param.h"

struct global_param global_parameters = {
    .BPM = 0,
    .AVG_BPM = 0,
    .show_heart = true,
    .step_cntr = 9,
    .battery_state = BATTERY_EMPTY,
    .weather = 0,
    .date = {},
    .time_str = {},
    .CO2 = 0,
    .temperature = 20.0,
};
