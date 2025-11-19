#ifndef __MACROS_H__
#define __MACROS_H__

#define I2C_PORT_0 I2C_NUM_0 // I2C port number
#define I2C_SDA_PIN GPIO_NUM_21 // I2C SDA pin
#define I2C_SCL_PIN GPIO_NUM_22 // I2C SCL pin
#define I2C_GLITCH_IGNORE_CNT 7 // Glitch filter
#define I2C_FREQ_HZ 100000 // I2C master frequency
#define I2C_MAX30102_ADDR 0x57 // MAX30102 I2C address
#define I2C_MPU6050_ADDR 0x68 //I2C address of MPU6050

#endif