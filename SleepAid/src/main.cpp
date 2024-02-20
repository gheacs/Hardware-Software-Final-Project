#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Stepper.h>

#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8

#define BME_SCK    13
#define BME_MISO   12
#define BME_MOSI   11
#define BME_CS     7

#define MIC_ANALOG_PIN A0

#define IN1 A0  // Motor driver input pins
#define IN2 A3
#define IN3 A1
#define IN4 A2

const int stepsPerRevolution = 2048;  // Change this to fit the number of steps per revolution for your motor
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4); // initialize the stepper library

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

Adafruit_BME280 bme; // I2C

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize display
  tft.begin();
  tft.setRotation(3); // Adjust rotation if needed

  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280 sensor, check wiring!");
    while (1);
  }

  // Set up the Stepper motor
  myStepper.setSpeed(60); // set the speed at 60 rpm

  Serial.println("BME280 sensor found!");
}

void loop() {
  // Clear previous readings
  tft.fillScreen(ILI9341_BLACK);

  // Reading BME280 data
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("Temperature = ");
  tft.print(bme.readTemperature());
  tft.println(" *C");

  tft.print("Pressure = ");
  tft.print(bme.readPressure() / 100.0F);
  tft.println(" hPa");

  tft.print("Humidity = ");
  tft.print(bme.readHumidity());
  tft.println(" %");

  // Reading audio data from MAX9814
  int audioValue = analogRead(MIC_ANALOG_PIN);
  tft.setCursor(0, 100);
  tft.print("Audio Value: ");
  tft.println(audioValue);

  // Step the motor
  Serial.println("clockwise");
  myStepper.step(stepsPerRevolution);
  delay(500);
  Serial.println("counterclockwise");
  myStepper.step(-stepsPerRevolution);
  delay(500);
}
