#include "MPU6050_api.h"

int step_cntr = 0;

esp_err_t mpu6050_write_reg(struct i2c_device* device, uint8_t reg_to_write, uint8_t val_to_write) { 

    uint8_t buf[2] = {reg_to_write, val_to_write};

    return i2c_master_transmit(device->i2c_dev_handle, 
                               buf, 
                               sizeof(buf), 
                               1000);
}

esp_err_t mpu6050_read_reg(struct i2c_device* device, uint8_t reg_to_read, uint8_t* val_to_read, size_t val_size) {

    uint8_t reg = reg_to_read;
    return i2c_master_transmit_receive(device->i2c_dev_handle, 
                                       &reg,
                                       1,
                                       val_to_read, 
                                       val_size, 
                                       1000);
}

void print_acc(const Three_Axis_t* ax) {

    if(ax == NULL) {
        printf("ACCELERATION NULL\n");
        return;
    }

    //"normalization"
    float ax_g = ax->a_x / 16384.0;
    float ay_g = ax->a_y / 16384.0;
    float az_g = ax->a_z / 16384.0;

    printf("ACCEL --- X: %f  Y: %f  Z: %f\n", ax_g, ay_g, az_g);
}

void read_sample(Three_Axis_t* ax, const uint8_t* r_buff) {

    if(ax == NULL || r_buff == NULL) {
        return;
    }

    ax->a_x = (r_buff[0] << 8) | r_buff[1];
    ax->a_y = (r_buff[2] << 8) | r_buff[3];
    ax->a_z = (r_buff[4] << 8) | r_buff[5];
}

// esp_err_t mpu6050_read_ACC(struct i2c_device* device, Three_Axis_t* axis) {

//     if(device == NULL || axis == NULL) {
//         return ESP_ERR_INVALID_ARG;
//     }
   
//     //read MPU6050_FIFO_COUNT_H and MPU6050_FIFO_COUNT_L for FIFO dim
//     uint8_t fifo_h;
//     uint8_t fifo_l;
    
//     if(mpu6050_read_reg(device, MPU6050_FIFO_COUNT_H, &fifo_h, sizeof(fifo_h)) != ESP_OK) {
//         return ESP_ERR_INVALID_ARG;
//     }
//     if(mpu6050_read_reg(device, MPU6050_FIFO_COUNT_L, &fifo_l, sizeof(fifo_l)) != ESP_OK) {
//         return ESP_ERR_INVALID_ARG;
//     }

//     uint16_t fifo_size = (fifo_h << 8) | fifo_l;

//     if(fifo_size < 6) {
//         return ESP_ERR_INVALID_ARG;
//     }

//     if (fifo_size > 1024) {
//         fifo_size = 1024;
//     }
    
//     //read FIFO
//     // uint8_t reading_buffer[6]; // DIO BOIAAAAAAAAAAAAAAAAAAAAAAA
//     uint8_t reading_buffer[fifo_size];
//     /*
//         6 Byte
//         0 | 1 : X
//         2 | 3 : Y
//         4 | 5 : Z
//     */
//     for(int i = 0; i < fifo_size; i++) {
//         /* 
//             I have to read 1byte * 6 due to the FIFO_DATA_R_W
//         */

//         uint8_t buf;
//         if(mpu6050_read_reg(device, MPU6050_FIFO_DATA_R_W, &buf, 1) != ESP_OK) {
//             return ESP_ERR_INVALID_ARG;
//         }
//         reading_buffer[i] = buf;
        
//         read_sample(axis, reading_buffer);

//     }

//     //reset FIFO
//     mpu6050_write_reg(device, 
//                       MPU6050_USER_CTRL, 
//                       USER_CTRL_BIT_FIFO_RST | USER_CTRL_BIT_FIFO_EN);
    
//     return ESP_OK;
// }

esp_err_t mpu6050_read_ACC(struct i2c_device* device, Three_Axis_t* axis) {

    if(device == NULL || axis == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
   
    uint8_t fifo_h, fifo_l;
    if(mpu6050_read_reg(device, MPU6050_FIFO_COUNT_H, &fifo_h, sizeof(fifo_h)) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    if(mpu6050_read_reg(device, MPU6050_FIFO_COUNT_L, &fifo_l, sizeof(fifo_l)) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t fifo_size = (fifo_h << 8) | fifo_l;

    if (fifo_size < 6) {
        return ESP_ERR_INVALID_ARG;
    }

    if (fifo_size > 1024) {
        fifo_size = 1024;
    }

    uint8_t reading_buffer[fifo_size];

    uint8_t reg = MPU6050_FIFO_DATA_R_W;
    // 2) Burst read da FIFO_DATA_R_W
    if (i2c_master_transmit_receive(device->i2c_dev_handle,
                                    &reg, 
                                    1,
                                    reading_buffer, 
                                    fifo_size,
                                    1000) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    //reading burst
    for (int i = 0; i + 5 < fifo_size; i += 6) {

        Three_Axis_t local_ax;
        local_ax.a_x = (reading_buffer[i + 0] << 8) | reading_buffer[i + 1];
        local_ax.a_y = (reading_buffer[i + 2] << 8) | reading_buffer[i + 3];
        local_ax.a_z = (reading_buffer[i + 4] << 8) | reading_buffer[i + 5];

        step_counter(&local_ax);

        *axis = local_ax;
    }

    mpu6050_write_reg(device, 
                      MPU6050_USER_CTRL,
                      USER_CTRL_BIT_FIFO_RST | USER_CTRL_BIT_FIFO_EN);

    return ESP_OK;
}

esp_err_t set_USR_CTRL(struct i2c_device* device) {

    /*
        set USR_CTRL register to:

        7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
        -----------------------------
        0 | 1 | 0 | 0 | 0 | 0 | 0 | 0
            ^
            | FIFO_EN 
    */
    if(mpu6050_write_reg(device, MPU6050_USER_CTRL, 0x00 | USER_CTRL_BIT_FIFO_RST | USER_CTRL_BIT_FIFO_EN) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t set_FIFO_EN(struct i2c_device* device) {

    /*
        set FIFO_EN register to:

        7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
        -----------------------------
        0 | 0 | 0 | 0 | 1 | 0 | 0 | 0
                        ^
                        | ACCEL_FIFO_EN 
    */
    return mpu6050_write_reg(device, MPU6050_FIFO_EN, FIFO_EN_BIT_ACCEL);
}

esp_err_t acc_config(struct i2c_device* device) {

    if(device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //sensor wake up
    if(mpu6050_write_reg(device, PWR_MGMT_1, 0x00) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    // Sample rate = 1kHz / (1 + 7) = 125 Hz
    if(mpu6050_write_reg(device, SMPLRT_DIV, 0x07) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    // Low-pass filter 5Hz (0x06 è ok)
    if(mpu6050_write_reg(device, CONFIG, 0x06) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
        config g_range 8 for:
            - wrist rotation
            - step_counter
            
        AFS_SELF = 2 dec
    */
    if(mpu6050_write_reg(device, MPU6050_ACCEL_CONFIG, MPU6050_ACC_G_RANGE) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    if(set_USR_CTRL(device) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    if(set_FIFO_EN(device) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

int low_pass_filter(const int M) {

    static float y = M_REST;  

    y += (M - y) / SMOOTHING_FACTOR;  //previous low pass filter value which needs for the next

    return (int)y;
}

bool verify_step(const Three_Axis_t* ax) {

    if(ax == NULL) {
        return false;
    }

    int M = sqrt((ax->a_x * ax->a_x) + 
                 (ax->a_y * ax->a_y) + 
                 (ax->a_z * ax->a_z));

    int filtered_M = low_pass_filter(M);
        
    static bool up = false;

    if(!up && filtered_M > (M_REST + THRESHOLD_HIGH)) { //rising edge
        /*
            this means one step is detected when M raises above TH_H and up is false (down) 
        */
        up = true;
        return true;
    } else if(up && filtered_M < (M_REST + THRESHOLD_LOW)) { //falling edge
        up = false;
    }
    return false;
}

void step_counter(const Three_Axis_t* ax) {

    if(verify_step(ax)) {
        STEP_COUNTER_INC(step_cntr);
        printf("STEPS: %d\n", step_cntr);
    }
}

bool wrist_detection(const Three_Axis_t* ax) {

     /*
         Detect wrist rotation send to X an interrupt which turns on/off (sleep mode) the display
     */
    return true;
}

void task_acc(void* pvParameters) {

    vTaskDelay(pdMS_TO_TICKS(50));

    struct i2c_device* device = (struct i2c_device *) pvParameters;

    if(device == NULL) {
        printf("task_acc: invalid device\n");
        vTaskDelete(NULL);
        return;
    }

    if(acc_config(device) != ESP_OK) {
        printf("Configuration error!\n");
        abort();
    }

    // printf("Task ACCELEROMETER is RUNNING!\n");

    Three_Axis_t axis;

    for(;;) {
        if(mpu6050_read_ACC(device, &axis) != ESP_OK) {
            printf("Error reading!\n");
        } else {
            step_counter(&axis);

            vTaskDelay(pdMS_TO_TICKS(20));
        }   
    }
}
        /*
            //TODO: wrist detection
            if(wrist_detection(&axis)) {
                xTaskNotify(
                    task_turn_on_display,
                    1,
                    eSetBits
                );
            }
            */
void task_gyro(void* pvParameters) {

    for(;;) {

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
 