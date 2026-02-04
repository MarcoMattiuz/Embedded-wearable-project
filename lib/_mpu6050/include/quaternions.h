/********************************************************************************************
 * Description:
 * This header file defines the quaternion structure and functions for quaternion-based 
 * angle estimation. It provides an interface for initializing, updating, and retrieving 
 * quaternion-based roll, pitch, and yaw.
 * 
 * Key Features:
 * - Quaternion structure and math functions.
 * - Interfaces for roll, pitch, and yaw computation.
 ********************************************************************************************/

#ifndef __QUATERNIONS_H__
#define __QUATERNIONS_H__

#include <math.h>

// Quaternion structure
typedef struct {
    float w;
    float x;
    float y;
    float z;
} Quaternion;

// Function prototypes
void quaternion_init       (Quaternion* q);
void quaternion_update     (Quaternion* q, float gx, float gy, float gz, float ax, float ay, float az, float dt);
float quaternion_get_roll  (const Quaternion* q);
float quaternion_get_pitch (const Quaternion* q);
float quaternion_get_yaw   (const Quaternion* q);

#endif

