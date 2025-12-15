#ifndef __GLOBAL_PARAM_H__
#define __GLOBAL_PARAM_H__

struct global_param
{
    int16_t BPM;
    int16_t AVG_BPM;
    bool show_heart;
    int step_cntr;
};

extern struct global_param global_parameters;

#endif