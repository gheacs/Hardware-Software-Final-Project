#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BME_SDA A4
#define BME_SCL A5
#define MIC_ANALOG_PIN A0
#define IN1 D7 // Motor driver input pins
#define IN2 D8
#define IN3 D9
#define IN4 D10

#define STEPS_PER_REVOLUTION 2048

Stepper myStepper(STEPS_PER_REVOLUTION, IN1, IN3, IN2, IN4);

Adafruit_BME280 bme;

#define SERVICE_UUID        "b26fd5cf-62f8-466b-9442-cae332f210ba"
#define CHARACTERISTIC_UUID "d4fe36bd-59ea-4edf-a5b4-b4485f3bcbf8"

static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(CHARACTERISTIC_UUID);

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

unsigned long previousMillis = 0; // For sleep quality calculation intervals
unsigned long previousMillisConnectionCheck = 0; // For connection check intervals
const long connectionCheckInterval = 5000;
const long dataTransmissionInterval = 1000;

String changeLevel = "";
float tempSum = 0;
int audioSum = 0;
int count = 0;

String sleepQuality = "";

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveServiceUUID() and advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
        }
    }
};

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    processData(pData, length);
}

void processData(uint8_t* pData, size_t length) {
    char buffer[length + 1];
    memset(buffer, 0, length + 1);
    strncpy(buffer, (char*)pData, length);
    changeLevel = buffer;

    Serial.print("Received Change Level from Server: ");
    Serial.println(changeLevel);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");
    Wire.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Infinite loop
    }
    display.display();
    delay(2000); // Show initial display

    display.clearDisplay();

    if (!bme.begin(0x76)) {
        Serial.println("Could not find BME280 sensor, check wiring!");
        while (1); // Infinite loop
    }
    Serial.println("BME280 sensor found!");

    myStepper.setSpeed(10); // Set the speed of the stepper motor

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    if (!pClient->connect(myDevice)) {
        Serial.println("Failed to connect to the server");
        return false;
    }
    Serial.println(" - Connected to server");

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find the service UUID");
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("Failed to find the characteristic UUID");
        pClient->disconnect();
        return false;
    }

    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    connected = true;
    return true;
}


void loop() {
    unsigned long currentMillis = millis();

    if (!connected && currentMillis - previousMillisConnectionCheck > connectionCheckInterval) {
        Serial.println("Checking connection...");
        if (doConnect) {
            if (connectToServer()) {
                Serial.println("Reconnected to the BLE Server.");
                connected = true;
            } else {
                Serial.println("Reconnection attempt failed.");
            }
            doConnect = false;
        } else {
            BLEDevice::getScan()->start(0); // 0 = continue forever
        }
        previousMillisConnectionCheck = currentMillis;
    }

    if (connected) {
        // Display and sensor logic remains unchanged
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Your Sleep Summary");

        float temp = bme.readTemperature();
        int audioValue = analogRead(MIC_ANALOG_PIN);

        display.setCursor(0, 10);
        display.print("Temp: ");
        display.print(temp);
        display.println(" C");

        display.setCursor(0, 20);
        display.print("Audio Value: ");
        display.println(audioValue);
                
        // Adjusting the stepper motor based on the audio value
        if (audioValue > 1200) {
            myStepper.step(STEPS_PER_REVOLUTION / 4); // Move clockwise
        } else {
            myStepper.step(-STEPS_PER_REVOLUTION / 4); // Move counterclockwise
        }


        display.setCursor(0, 30);
        display.println(changeLevel);

        if (currentMillis - previousMillis >= 180000) {
            tempSum += temp;
            audioSum += audioValue;
            count++;

            if (count == 3) {
                float avgTemp = tempSum / count;
                int avgAudio = audioSum / count;

                if (changeLevel == "Low") {
                    if (avgTemp < 25 && avgAudio < 1200) {
                        sleepQuality = "Very Good";
                    } else {
                        sleepQuality = "Good";
                    }
                } else if (changeLevel == "Medium") {
                    if (avgTemp < 25 && avgAudio < 1200) {
                        sleepQuality = "Good";
                    } else {
                        sleepQuality = "Poor";
                    }
                } else if (changeLevel == "High") {
                    sleepQuality = "Poor";
                } else {
                    sleepQuality = "Poor"; // Changed from "Poor" to "Unknown" for undetermined conditions
                }

                Serial.print("Sleep Quality: ");
                Serial.println(sleepQuality);

                tempSum = 0;
                audioSum = 0;
                count = 0;
                previousMillis = currentMillis;
            }
        }

        display.setCursor(0, 40);
        display.print("Sleep Quality: ");
        display.println(sleepQuality);

        display.display();

        delay(10000); // Wait for 10 seconds before next loop iteration
    }
}
