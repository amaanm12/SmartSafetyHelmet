#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

struct IMUData{
    float accX;    // m/s^2
    float accY;    // m/s^2
    float accZ;    // m/s^2
    float gyroX;   // rad/s
    float gyroY;   // rad/s
    float gyroZ;   // rad/s
    float temp;    // Celsius
};

class IMUSensor{
public: 
    IMUSensor();

    // initilze the I2C pins on the esp32 
    bool begin(int sdaPin = 21, int sclPin =22) ; 
    // Refreshes data from registers into the currentData struct
    void update();
    
    // Clean getter method to read values
    IMUData getData() const { 
        return currentData; 
    }

private:
    Adafruit_MPU6050 mpu;
    IMUData currentData;
};
