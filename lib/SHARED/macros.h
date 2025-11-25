#ifndef __MACROS_H__
#define __MACROS_H__

<<<<<<< HEAD
#ifdef DEBUG
    #define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define DBG_PRINTF(...)
#endif


//MAX30102
#define I2C_PORT_0            I2C_NUM_0   // I2C port number
#define MAX30102_I2C_SDA_PIN  GPIO_NUM_21 // I2C SDA pin
#define MAX30102_I2C_SCL_PIN  GPIO_NUM_22 // I2C SCL pin
#define I2C_MAX30102_ADDR     0x57        // MAX30102 I2C address
=======
// MAX30102/SH1106
#define I2C_PORT_0            I2C_NUM_0   // I2C port number
#define MAX30102_I2C_SDA_PIN GPIO_NUM_21  // I2C SDA pin SH1106/MAX30102
#define MAX30102_I2C_SCL_PIN  GPIO_NUM_22 // I2C SCL pin
#define I2C_MAX30102_ADDR 0x57            // MAX30102 I2C address
#define I2C_SH1106_ADDR 0x3C              // SH1106 I2C address
>>>>>>> 96dc577a5cd4103de5a0ea23f1154b02468df0bf

//MPU6050
#define I2C_PORT_1            I2C_NUM_1   // I2C port number
#define MPU6050_I2C_SDA_PIN   GPIO_NUM_19 // I2C SDA pin
#define MPU6050_I2C_SCL_PIN   GPIO_NUM_23 // I2C SCL pin
#define I2C_MPU6050_ADDR      0x68        //I2C address of MPU6050

#define I2C_GLITCH_IGNORE_CNT 7           // Glitch filter
#define I2C_FREQ_HZ           100000      // I2C master frequency

#endif