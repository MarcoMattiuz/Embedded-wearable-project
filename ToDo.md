[1.0] Collegamenti hardware
    VCC → 3.3V, GND → GND
    SDA → GPIO (es. 21), SCL → GPIO (eg. 22)
    INT → opzionale

[2.0] init I2C
    config master SDA/SCL a 400kHz

[3.0] turn off sleep mode
    wr in reg 0x6B 0x00

[4.0] read data from acc
    read 6byte starting from 0x3B
    combine data in int16 for X, Y, Z

[5.0] read data drom gir
    read 6byte starting from 0x43
    combine data in int16 for X, Y, Z

[6.0] convert data in real data
    acc value/16384.0 
    gir value/131.0 

[7.0] Moving average o low-pass filter

[8.0] Step counter

[9.0] reading loop