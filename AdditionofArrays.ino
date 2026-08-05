#include <Wire.h>
#include "RichShieldDHT.h"
#include "RichShieldTM1637.h"
#include "RichShieldKEY.h"
#include "RichShieldLightSensor.h"
#include "RichShieldPassiveBuzzer.h"

// ==================================================
// Hardware pin assignments
// ==================================================

const int CLK = 10;
const int DIO = 11;

const int KEY1_PIN = 8;
const int KEY2_PIN = 9;

const int GREEN_LED = 5;
const int YELLOW_LED = 7;
const int RED_LED = 4;

const int BUZZER_PIN = 3;

// ==================================================
// Rich Shield objects
// ==================================================

TM1637 disp(CLK, DIO);
Key key(KEY1_PIN, KEY2_PIN);
DHT dht;
LightSensor light;
PassiveBuzzer buzzer(BUZZER_PIN);

// ==================================================
// Sensor array indexes
// ==================================================

const byte TEMP_INDEX = 0;
const byte HUMIDITY_INDEX = 1;
const byte LIGHT_INDEX = 2;

const byte SENSOR_COUNT = 3;

// Store all three readings in one array.
float sensorValues[SENSOR_COUNT] = {0, 0, 0};

// Each row is a C-style string.
// "Temperature" has 11 characters, so 12 spaces
// are needed to include the '\0' terminator.
const char SENSOR_NAMES[SENSOR_COUNT][12] =
{
  "Temperature",
  "Humidity",
  "Light"
};

// Units for each sensor value.
const char SENSOR_UNITS[SENSOR_COUNT][4] =
{
  " C",
  " %",
  ""
};

// ==================================================
// LED arrays and indexes
// ==================================================

const byte GREEN_LED_INDEX = 0;
const byte YELLOW_LED_INDEX = 1;
const byte RED_LED_INDEX = 2;

const byte LED_COUNT = 3;

const int LED_PINS[LED_COUNT] =
{
  GREEN_LED,
  YELLOW_LED,
  RED_LED
};

// ==================================================
// Environment status indexes
// ==================================================

const byte STATUS_SENSOR_ERROR = 0;
const byte STATUS_DANGER = 1;
const byte STATUS_WARNING = 2;
const byte STATUS_SAFE = 3;

const byte STATUS_COUNT = 4;

// "SENSOR ERROR" has 12 characters, so each row
// must have space for at least 13 characters.
const char STATUS_NAMES[STATUS_COUNT][13] =
{
  "SENSOR ERROR",
  "DANGER",
  "WARNING",
  "SAFE"
};

// This array maps each status to the correct LED.
const byte STATUS_LED_INDEX[STATUS_COUNT] =
{
  RED_LED_INDEX,       // SENSOR ERROR
  RED_LED_INDEX,       // DANGER
  YELLOW_LED_INDEX,    // WARNING
  GREEN_LED_INDEX      // SAFE
};

// ==================================================
// Threshold values
// ==================================================

const float TEMP_WARNING = 30;
const float TEMP_DANGER = 35;

const float HUMIDITY_LOW = 40;
const float HUMIDITY_HIGH = 80;

const float LIGHT_DARK = 200;

// ==================================================
// Program variables
// ==================================================

// The display mode uses the same indexes as sensorValues.
byte displayMode = TEMP_INDEX;

bool muteAlarm = false;
bool sensorValid = false;

// ==================================================
// Function prototypes
// ==================================================

void readSensors(void);
void printSensorReadings(void);
void updateDisplay(void);
void checkEnvironment(void);
void checkButtons(void);
void switchDisplayMode(void);
void soundAlarm(byte status);

byte getEnvironmentStatus(void);

// ==================================================
// Read the sensors
// ==================================================

void readSensors(void)
{
  float newTemperature;
  float newHumidity;

  newTemperature = dht.readTemperature();
  newHumidity = dht.readHumidity();

  // The light sensor is stored at index 2.
  sensorValues[LIGHT_INDEX] = light.getRes();

  // Check whether either DHT reading is invalid.
  if (isnan(newTemperature) || isnan(newHumidity))
  {
    sensorValid = false;

    Serial.println("DHT Sensor Error");

    Serial.print(SENSOR_NAMES[LIGHT_INDEX]);
    Serial.print(": ");
    Serial.println(sensorValues[LIGHT_INDEX]);

    Serial.println("-------------------");

    return;
  }

  // Store the valid readings in the array.
  sensorValues[TEMP_INDEX] = newTemperature;
  sensorValues[HUMIDITY_INDEX] = newHumidity;

  sensorValid = true;

  printSensorReadings();
}

// ==================================================
// Print every sensor reading
// ==================================================

void printSensorReadings(void)
{
  Serial.println("===== ROOM READINGS =====");

  // i changes from 0 to 2.
  // Each repetition prints one sensor.
  for (byte i = 0; i < SENSOR_COUNT; i++)
  {
    Serial.print(SENSOR_NAMES[i]);
    Serial.print(": ");
    Serial.print(sensorValues[i]);
    Serial.println(SENSOR_UNITS[i]);
  }

  if (sensorValues[LIGHT_INDEX] > LIGHT_DARK)
  {
    Serial.println("Room lighting: DARK");
  }
  else
  {
    Serial.println("Room lighting: BRIGHT");
  }

  Serial.println("=========================");
}

// ==================================================
// Determine the current environment status
// ==================================================

byte getEnvironmentStatus(void)
{
  if (!sensorValid)
  {
    return STATUS_SENSOR_ERROR;
  }

  if (sensorValues[TEMP_INDEX] >= TEMP_DANGER)
  {
    return STATUS_DANGER;
  }

  if (sensorValues[TEMP_INDEX] >= TEMP_WARNING ||
      sensorValues[HUMIDITY_INDEX] < HUMIDITY_LOW ||
      sensorValues[HUMIDITY_INDEX] > HUMIDITY_HIGH)
  {
    return STATUS_WARNING;
  }

  return STATUS_SAFE;
}

// ==================================================
// Update the four-digit display
// ==================================================

void updateDisplay(void)
{
  // Temperature and humidity cannot be displayed
  // when the DHT reading is invalid.
  if (!sensorValid && displayMode != LIGHT_INDEX)
  {
    disp.display(0);
    return;
  }

  // The display mode is also the sensor array index.
  disp.display((int)sensorValues[displayMode]);
}

// ==================================================
// Control the LEDs and alarm
// ==================================================

void checkEnvironment(void)
{
  byte currentStatus;

  currentStatus = getEnvironmentStatus();

  // Switch all LEDs off.
  for (byte i = 0; i < LED_COUNT; i++)
  {
    digitalWrite(LED_PINS[i], LOW);
  }

  // Use the status-to-LED mapping array.
  digitalWrite(
    LED_PINS[STATUS_LED_INDEX[currentStatus]],
    HIGH
  );

  Serial.print("Status: ");
  Serial.println(STATUS_NAMES[currentStatus]);

  if (sensorValues[LIGHT_INDEX] > LIGHT_DARK)
  {
    Serial.println("Room is dark");
  }

  soundAlarm(currentStatus);
}

// ==================================================
// Check the two buttons
// ==================================================

void checkButtons(void)
{
  byte keyValue;

  keyValue = key.get();

  switch (keyValue)
  {
    case 1:
      switchDisplayMode();
      break;

    case 2:
      muteAlarm = !muteAlarm;

      if (muteAlarm)
      {
        buzzer.off();
        Serial.println("Alarm: MUTED");
      }
      else
      {
        Serial.println("Alarm: ENABLED");
      }

      break;
  }
}

// ==================================================
// Change display mode
// ==================================================

void switchDisplayMode(void)
{
  displayMode++;

  if (displayMode >= SENSOR_COUNT)
  {
    displayMode = TEMP_INDEX;
  }

  Serial.print("Display: ");
  Serial.println(SENSOR_NAMES[displayMode]);

  updateDisplay();
}

// ==================================================
// Control the buzzer
// ==================================================

void soundAlarm(byte status)
{
  if (muteAlarm || status == STATUS_SENSOR_ERROR)
  {
    buzzer.off();
    return;
  }

  if (status == STATUS_DANGER)
  {
    buzzer.playTone(1200, 500);
  }
  else if (status == STATUS_WARNING)
  {
    buzzer.playTone(800, 200);
  }
  else
  {
    buzzer.off();
  }
}

// ==================================================
// Initial setup
// ==================================================

void setup(void)
{
  Serial.begin(9600);

  dht.begin();

  disp.init();
  disp.display(0);

  // Configure all LED pins using an array and loop.
  for (byte i = 0; i < LED_COUNT; i++)
  {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  buzzer.off();

  Serial.println("Smart Elderly Room Monitor Started");
  Serial.println("KEY1: Change display");
  Serial.println("KEY2: Mute or enable alarm");
}

// ==================================================
// Main program loop
// ==================================================

void loop(void)
{
  checkButtons();
  readSensors();
  checkEnvironment();
  updateDisplay();
}