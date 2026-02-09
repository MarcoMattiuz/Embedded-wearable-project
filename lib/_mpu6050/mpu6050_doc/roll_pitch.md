```c
./lib/_mpu6050/include/roll_pitch.h 
./lib/_mpu6050/src/roll_pitch.c
```
# Types
```c
typedef struct {
    float    integrated_angle;
    uint32_t last_trigger;
} Rotation_t;

typedef struct {
    float roll;
    float pitch;  
    float yaw;
} Orientation_t;

typedef struct {
    float m[3][3];
} RotationMatrix_t;
```
# Code Explanation

This document explains the purpose and internal logic of the main functions implemented in the **motion analysis and orientation module** based on accelerometer and gyroscope data.

---

# Step Detection 
```c
bool verify_step(ACC_Three_Axis_t *ax)
```

This function implements a step detection algorithm using accelerometer data.

### Core idea
Acceleration is a 3D vector `(ax, ay, az)` and we use the **magnitude** of the vector, computed as:

```
M = sqrt(x² + y² + z²)
```
The magnitude oscillates around `GRAVITY` when the device is still, but when a step is taken, `M` will show a peak that can be detected by setting two thresholds.

### Filtering
- **Low-pass filter (exponential moving average)** to estimate the gravity component:

```
M_lp = 0.95 * M_lp + 0.05 * M
```

- **High-pass component** to isolate dynamic motion:

```
M_hp = M - M_lp
```

### Detection logic
- Uses two thresholds
- Detection is based on a **rising edge**
- A **500 ms refractory window** avoids double triggers (500ms is an average step time for a normal walking pace 120 steps/min)

Where:
```c
//init
static float M_lp = GRAVITY;
static bool up = false;
static uint32_t last_step = 0;

#define THRESHOLD_H 4.0f     
#define THRESHOLD_L 0.5f 
```
### Return value
- `true` → step detected
- `false` → no step detected

---

# Wrist Rotation Detection
```c
bool verify_wrist_rotation(GYRO_Three_Axis_t *g)
```

Detects a **wrist rotation gesture** using gyroscope data.

### How it works
- Uses the angular velocity of a selected axis (here `g_x`)
- Angular velocity is integrated over time:

```
θ = ∫ ω dt ≈ ω * DT
```

### Protections
- **Refractory window** (`REFRACT_MS`) prevents multiple triggers

### Trigger condition
- The integrated angle exceeds `WRIST_ROT_THRESHOLD`

Where:
```c
//init
static Rotation_t rotation = {0.0f, 0};

#define REFRACT_MS          500    // ro avoid double triggers
#define WRIST_ROT_THRESHOLD 60.0f  // min wrist rotation
#define DT                  0.008f // time between samples relate to 125Hz 
```

### Return value
- `true` → wrist rotation detected
- `false` → no significant rotation

---

# Verify Movement

```c
void verify_motion(ACC_Three_Axis_t *acc_data, GYRO_Three_Axis_t *gyro_data)
```

This function calls:
  - `verify_step()`
  - `verify_wrist_rotation()`

Updates the global step counter if verify_step() is `TRUE`.

---

# Update Orientation

```c
void update_orientation(const GYRO_Three_Axis_t *gyro, const ACC_Three_Axis_t  *acc)
```

Computes the **absolute orientation** of the device: roll, pitch, and yaw.

### Accelerometer contribution
Used to estimate **roll** and **pitch** with respect to gravity:

- Roll - lateral inclination angle:
```
atan2(ay, az)
```

- Pitch - longitudinal inclination angle:
```
atan2(-ax, sqrt(ay² + az²))
```

### Gyroscope contribution
- Angular velocity integration:

```c
angle += ω * DT

orient.roll  += gyro->g_x * DT;
orient.pitch += gyro->g_y * DT;
orient.yaw   += gyro->g_z * DT;
```

- `yaw` drifts over time due to the absence of a magnetometer

### Complementary filter
Combines gyroscope responsiveness with accelerometer stability (weighted average):

```c
angle = ALPHA * gyro + (1 - ALPHA) * acc

#define ALPHA 0.9f
```

This significantly reduces gyroscope drift while preserving dynamic response.

---

# Orientation Matrix

```c
void get_orientation_matrix (RotationMatrix_t *R, GYRO_Three_Axis_t *tmp)
```

This function builds up the **3D rotation matrix (ZYX convention)** from roll, pitch, and yaw.

### Steps
1. Convert angles from degrees to radians
2. Construct the rotation matrix, computing ([Basic-3D-Rotation](https://en.wikipedia.org/wiki/Rotation_matrix)):
    ```
    R = Rz(yaw) · Ry(pitch) · Rx(roll)
    ```
    ![alt text](image.png)
    ```c
    float cr = cosf(roll);
    float sr = sinf(roll);
    float cp = cosf(pitch);
    float sp = sinf(pitch);
    float cy = cosf(yaw);
    float sy = sinf(yaw);

    // ZYX rotation matrix
    R->m[0][0] = cy * cp;
    R->m[0][1] = cy * sp * sr - sy * cr;
    R->m[0][2] = cy * sp * cr + sy * sr;

    R->m[1][0] = sy * cp;
    R->m[1][1] = sy * sp * sr + cy * cr;
    R->m[1][2] = sy * sp * cr - cy * sr;

    R->m[2][0] = -sp;
    R->m[2][1] = cp * sr;
    R->m[2][2] = cp * cr;

    ```
3. Taking the 3th column of the matrix which rappresents the vertical inclination vector (Z axis, it doesn't show forward direction)
    ```c
    tmp->g_x = R->m[0][2];
    tmp->g_y = R->m[1][2];
    tmp->g_z = R->m[2][2];
    ``` 
4. Alternately we can take the first column, it shows the forward direction vector (X axis,  it doesn't show the roll: rotation around itself)
    ```c
    tmp->g_x = R->m[0][0];
    tmp->g_y = R->m[1][0];
    tmp->g_z = R->m[2][0];
    ```

Where:
```c
// init 
RotationMatrix_t R = {
    .m = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    }
};
static Orientation_t orient = {0, 0, 0};

#define DEG_TO_RAD 0.01745329252f // π/180
```
---