#include "MPU6050_api.h"


int step_cntr = 0;

esp_err_t mpu6050_write_reg(struct i2c_device *device, uint8_t reg, uint8_t data) { 

    uint8_t txbuf[2];
    txbuf[0] = reg;
    txbuf[1] = data;
    return i2c_master_transmit(device->i2c_dev_handle, txbuf, sizeof(txbuf), 1000);
}

esp_err_t mpu6050_read_reg(struct i2c_device *device, uint8_t reg, uint8_t *data, size_t len) {
    //read data (size: len) from reg
    /*
        starting in
        MPU6050_ACCEL_XOUT_H 0x3B
        and reading 6 bytes it reaches
        MPU6050_ACCEL_ZOUT_L 0x50
        going through 
        MPU6050_ACCEL_XOUT_L
        MPU6050_ACCEL_YOUT_H
        MPU6050_ACCEL_YOUT_L
        MPU6050_ACCEL_ZOUT_H
        ending up obtain all data for axis: X, Y, Z

        write_read idicates where start the reading
        sending 1 byte (the reg to start the reading) 
        to MPU6050's internal ptr reg
    */                                                       

    // 1) Write register
    esp_err_t err = i2c_master_transmit(device->i2c_dev_handle, &reg, 1, 1000);
    if (err != ESP_OK) {
        return err;
    }

    // 2) Read data
    return i2c_master_receive(device->i2c_dev_handle, data, len, 1000);
    
}

void read_sample(Three_Axis_t* ax, const Reading_Buffer_t r_buff) {

    ax->a_x = (r_buff[0] << 8) | r_buff[1];
    ax->a_y = (r_buff[2] << 8) | r_buff[3];
    ax->a_z = (r_buff[4] << 8) | r_buff[5];
}

void print_acc(const Three_Axis_t* ax) {

    //"normalization"
    float ax_g = ax->a_x / 16384.0;
    float ay_g = ax->a_y / 16384.0;
    float az_g = ax->a_z / 16384.0;

    printf("ACCEL -> X: %f  Y: %f  Z: %f\n", ax_g, ay_g, az_g);
}
bool written = true;
void acc_config(struct i2c_device *device) {

    //sensor wake up
    if(mpu6050_write_reg(device,PWR_MGMT_1, 0x00)!=ESP_OK){
        written=false;
    }
    mpu6050_write_reg(device,SMPLRT_DIV, 0x07);
    mpu6050_write_reg(device,CONFIG, 0x06);
    
    /*
        config g_range 8 for:
          - wrist rotation
          - step_counter
    */
   uint8_t a_config = MPU6050_ACC_G_RANGE; //AFS_SELF = 2 dec
   mpu6050_write_reg(device, ACC_CONFIG, a_config);
}

int low_pass_filter(const int M) {

    static float y = M_REST;  

    y += (M - y) / SMOOTHING_FACTOR;  //previous low pass filter value which needs for the next

    return (int)y;
}

bool verify_step(const Three_Axis_t* ax) {

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

// bool wrist_detection(const Three_Axis_t* ax) {

//     /*
//         Detect wrist rotation send to X an interrupt which turns on/off (sleep mode) the display
//     */
    
// }

void task_acc(void* pvParameters) {
    struct i2c_device *device = (struct i2c_device *) pvParameters;
    acc_config(device);

    Three_Axis_t axis;

    for(;;) {
      
        Reading_Buffer_t reading_buffer; 
        /*
            6 Byte
            0 | 1 : X
            2 | 3 : Y
            4 | 5 : Z
        */
        if(mpu6050_read_reg(device,MPU6050_ACCEL_XOUT_H, reading_buffer, 6) != ESP_OK) {
            printf("Error reading!\n");
        } else {

            read_sample(&axis, reading_buffer);

            step_counter(&axis);

            // //TODO: wrist detection
            // if(wrist_detection(&axis)) {
            //     xTaskNotify(
            //         task_turn_on_display,
            //         1,
            //         eSetBits
            //     );
            // }

            //print_acc(&axis);

            vTaskDelay(pdMS_TO_TICKS(100));
        }   
        //TODO: sleep_mode 
    }
}

void task_gyro() {

}
 