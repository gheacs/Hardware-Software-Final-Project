#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>

MPU6050 mpu(0x69); // Initialize MPU6050 with I2C address 0x69

#define SERVICE_UUID        "b26fd5cf-62f8-466b-9442-cae332f210ba"
#define CHARACTERISTIC_UUID "d4fe36bd-59ea-4edf-a5b4-b4485f3bcbf8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

float prevMagnitude = 0.0;
const float LOW_THRESHOLD = 100;    // Adjust thresholds as needed
const float MEDIUM_THRESHOLD = 1000; // These are just example values
const float HIGH_THRESHOLD = 5000.0;

unsigned long startTime = 0;
int movementCount = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting MPU6050 Server...");

    Wire.begin();
    mpu.initialize();

    BLEDevice::init("MPU6050_Server");
    pServer = BLEDevice::createServer();
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Minimum connection interval
    pAdvertising->setMinPreferred(0x12); // Maximum connection interval
    BLEDevice::startAdvertising();

    startTime = millis(); // Initialize start time
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // Read MPU6050 data
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    // Calculate magnitude of acceleration
    float magnitude = sqrt((float)ax * ax + (float)ay * ay + (float)az * az);

    // Determine the level of change
    String changeLevel;
    if (abs(magnitude - prevMagnitude) < LOW_THRESHOLD) {
        changeLevel = "Low";
    } else if (abs(magnitude - prevMagnitude) < MEDIUM_THRESHOLD) {
        changeLevel = "Medium";
    } else {
        changeLevel = "High";
        // Increment movement count if magnitude change is high
        movementCount++;
    }

    // Check if 1 minute has elapsed
    unsigned long currentTime = millis();
    if (currentTime - startTime >=  60 * 1000) {
        // Print movement count and reset count
        Serial.print("Movement count in last 1 minute: ");
        Serial.println(movementCount);

        // Categorize the movement count
        if (movementCount > 3) {
            Serial.println("Many movements detected in last 1 minute!");
            changeLevel = "Many Movements";
        }
        else {
            Serial.println("Not many movement detected in last 1 minute!");
            changeLevel = "Few Movements";
        }

        // Reset count
        movementCount = 0;
        startTime = currentTime; // Reset start time
    }

    // Compose the data string
    char data[50];
    snprintf(data, sizeof(data), "Change Level: %s", changeLevel.c_str());

    // Update characteristic value and notify client
    pCharacteristic->setValue(data);
    pCharacteristic->notify(); // Send notification

    Serial.print("Notified value: ");
    Serial.println(data);

    // Debug print to confirm notification status
    Serial.println("Notification sent successfully.");


    
    // Update previous magnitude
    prevMagnitude = magnitude;

    delay(1000); // Adjust delay as needed for stability
}
