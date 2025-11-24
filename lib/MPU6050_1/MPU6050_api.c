#include "MPU6050_api.h"

int          step_cntr   = 0;
WristState_t wrist_state = {0.0f, false};

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

void print_acc(const Three_Axis_final_t* ax) {

    if(ax == NULL) {
        printf("ACCELERATION NULL\n");
        return;
    }

    printf("ACCEL --- X: %f  Y: %f  Z: %f\n", ax->a_x, ax->a_y, ax->a_z);
}

void print_gyro(const Gyro_Axis_final_t* gyro) {

    if(gyro == NULL) {
        printf("GYROSCOPE NULL\n");
        return;
    }

    printf("GYRO --- X: %f  Y: %f  Z: %f\n", gyro->g_x, gyro->g_y, gyro->g_z);
}

void read_sample_ACC(Three_Axis_t* ax, Three_Axis_final_t* f_ax, const uint8_t* r_buff) {

    if(ax == NULL || r_buff == NULL) {
        return;
    }

    ax->a_x = (r_buff[0] << 8) | r_buff[1];
    ax->a_y = (r_buff[2] << 8) | r_buff[3];
    ax->a_z = (r_buff[4] << 8) | r_buff[5];

    f_ax->a_x = ax->a_x / 16384.0;
    f_ax->a_y = ax->a_y / 16384.0;
    f_ax->a_z = ax->a_z / 16384.0;
    
    // printf("ACC --- X: %f, Y: %f, Z: %f\n", f_ax->a_x, f_ax->a_y, f_ax->a_z);
}

void read_sample_GYRO(Gyro_Axis_t* gyro, Gyro_Axis_final_t* f_gyro, const uint8_t* r_buff) {

    if(gyro == NULL || r_buff == NULL) {
        return;
    }

    gyro->g_x = (r_buff[0] << 8) | r_buff[1];
    gyro->g_y = (r_buff[2] << 8) | r_buff[3];
    gyro->g_z = (r_buff[4] << 8) | r_buff[5];

    //"normalization"
    f_gyro->g_x = gyro->g_x / SENS_GYRO_RANGE;
    f_gyro->g_y = gyro->g_y / SENS_GYRO_RANGE;
    f_gyro->g_z = gyro->g_z / SENS_GYRO_RANGE;

   // printf("GYRO --- X: %f, Y: %f, Z: %f\n", f_gyro->g_x, f_gyro->g_y, f_gyro->g_z);
}

esp_err_t mpu6050_read_FIFO(struct i2c_device* device, Three_Axis_t* axis, Gyro_Axis_t* gyro, Three_Axis_final_t* f_ax, Gyro_Axis_final_t* f_gyro) {

    if(device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
   
    //read MPU6050_FIFO_COUNT_H and MPU6050_FIFO_COUNT_L for FIFO dim
    uint8_t fifo_h;
    uint8_t fifo_l;
    
    if(mpu6050_read_reg(device, MPU6050_FIFO_COUNT_H, &fifo_h, sizeof(fifo_h)) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    if(mpu6050_read_reg(device, MPU6050_FIFO_COUNT_L, &fifo_l, sizeof(fifo_l)) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t fifo_size = (fifo_h << 8) | fifo_l;
    //printf("FIFO size: %d\n", fifo_size);

    if(fifo_size < 12) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t frames = fifo_size / 12;
    uint16_t offset = (frames - 1) * 12;

    // get rid of old data
    for(int i = 0; i < offset; i++) {
        uint8_t dump;
        mpu6050_read_reg(device, MPU6050_FIFO_DATA_R_W, &dump, 1);
    }
    
    //read FIFO
    uint8_t reading_buffer[12];
    /*
        6 Byte * 2
        0 | 1 : X
        2 | 3 : Y
        4 | 5 : Z
    */
    for(int i = 0; i < 12; ++i) {
        /*
            The FIFO will contain ACC data e GYRO data
            first bytes for ACC
            second bytes for GYRO
        */
        uint8_t buf;
        if(mpu6050_read_reg(device, MPU6050_FIFO_DATA_R_W, &buf, 1) != ESP_OK) {
            return ESP_ERR_INVALID_ARG;
        }
        reading_buffer[i] = buf;
    }

    //ACC
    read_sample_ACC(axis, f_ax, reading_buffer);
    // GYRO
    read_sample_GYRO(gyro, f_gyro, reading_buffer + 6);
    
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
    return mpu6050_write_reg(device,
                             MPU6050_USER_CTRL, 
                             0x00 | USER_CTRL_BIT_FIFO_RST | USER_CTRL_BIT_FIFO_EN);
}

esp_err_t set_FIFO_EN(struct i2c_device* device) {

    /*
        set FIFO_EN register to:

        7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
        -----------------------------
        0 | 1 | 1 | 1 | 1 | 0 | 0 | 0
            ^    ^   ^   ^
            |    |   |   | ACCEL_FIFO_EN 
            |    |   | ZG_FIFO_EN
            |    | YG_FIFO_EN
            | XG_FIFO_EN

    */
    return mpu6050_write_reg(device, 
                             MPU6050_FIFO_EN, 
                             FIFO_EN_BIT_ACCEL | FIFO_EN_BIT_XG | FIFO_EN_BIT_YG | FIFO_EN_BIT_ZG);
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
        config g_range 8
        AFS_SELF = 2 dec
    */
    if(mpu6050_write_reg(device, MPU6050_ACCEL_CONFIG, MPU6050_ACC_G_RANGE) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
        config full_range 250  
        FS_SELF = 0 dec
    */
    if(mpu6050_write_reg(device, MPU6050_GYRO_CONFIG, MPU6050_GYRO_RANGE) != ESP_OK) {
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

int low_pass_filter_M(const int M) {

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

    int filtered_M = low_pass_filter_M(M);
        
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

bool verify_wrist_rotation(const Gyro_Axis_final_t* gyro) {

    if (gyro == NULL) {
        return false;
    }

    // smooth factor
    static float wx_f = 0.0f;
    wx_f =  gyro->g_x * 0.15f;

    // angle integration
    wrist_state.angle += wx_f * DT;
    
    // is rotation active?
    if (fabs(wx_f) > WRIST_ROT_THRESHOLD) {
        wrist_state.rotating = true;
    } else {
        wrist_state.rotating = false;
    }

    if (wrist_state.rotating && fabs(wrist_state.angle) >= MIN_ROT_ANGLE) {
        wrist_state.angle = 0;   // reset
        return true;             // rotation detected
    }

    return false;
}

void motion_analysis(const Three_Axis_t* ax, const Gyro_Axis_final_t* gyro) {

    bool step  = verify_step(ax);
    bool wrist = verify_wrist_rotation(gyro);

    //printf("%d - %d\n", step, wrist);

    if(step && !wrist) {
        STEP_COUNTER_INC(step_cntr);
        printf("STEPS: %d\n", step_cntr);
    } else if(step && wrist) {
        printf("WRIST ROTATION DETECT\n");
        /*
        //Notify wrist rotation to display with 1 -> turn on display
        xTaskNotify(
            task_turn_on_display,
            1,
            eSetBits
        );
        */
    } else if(!step && wrist) {
        printf("WRIST ROTATION DETECT\n");
        /*
        //Notify wrist rotation to display with 1 -> turn on display
        xTaskNotify(
            task_turn_on_display,
            1,
            eSetBits
        );
        */
    } else {
        return;
    }
}

void task_acc(void* pvParameters) {

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

    printf("Task ACCELEROMETER is RUNNING!\n");

    Three_Axis_t       axis;
    Three_Axis_final_t f_axis;
    Gyro_Axis_t        gyro;
    Gyro_Axis_final_t  f_gyro;

    for(;;) {
        if(mpu6050_read_FIFO(device, &axis, &gyro, &f_axis, &f_gyro) != ESP_OK) {
            printf("Error reading!\n");
        } else {
            motion_analysis(&axis, &f_gyro);
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }   
    }
} 