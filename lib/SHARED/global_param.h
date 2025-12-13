#ifndef __GLOBAL_PARAM_H__
#define __GLOBAL_PARAM_H__

struct global_param
{
    float BPM;
    float AVG_BPM;
    bool show_heart;
    int step_cntr;
};

extern struct global_param global_parameters;

#endif