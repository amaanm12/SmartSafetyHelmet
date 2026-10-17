#include "sensor.h"

IMUSensor::IMUSensor() {
    // Zero-initialize data structure properties
    currentData = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

bool IMUSensor::begin(int sdaPin, int sclPin) {
    // Explicitly configure the default ESP32 hardware I2C wire bus pins
    Wire.begin(sdaPin, sclPin);
    
    if (!mpu.begin()) {
        Serial.println("[IMU] Error: MPU6050 chip not detected over I2C.");
        return false;
    }
    
    // Set standard operating scale thresholds
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);  // Options: 2G, 4G, 8G, 16G
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);       // Options: 250, 500, 1000, 2000 deg/s
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);    // Dynamic digital low-pass filtering
    
    Serial.println("[IMU] Adafruit MPU6050 driver initialized successfully.");
    return true;
}

void IMUSensor::update() {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);
    
    // Store physics-calculated properties directly into our data layout
    currentData.accX  = accel.acceleration.x;
    currentData.accY  = accel.acceleration.y;
    currentData.accZ  = accel.acceleration.z;
    
    currentData.gyroX = gyro.gyro.x;
    currentData.gyroY = gyro.gyro.y;
    currentData.gyroZ = gyro.gyro.z;
    
    currentData.temp  = temp.temperature;
}
