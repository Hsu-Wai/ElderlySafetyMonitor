#include <Wire.h>
#include "RichShieldDHT.h"
#include "RichShieldTM1637.h"
#include "RichShieldKEY.h"
#include "RichShieldLightSensor.h"
#include "RichShieldPassiveBuzzer.h"

const int CLK = 10;
const int DIO = 11;

const int KEY1_PIN = 8;
const int KEY2_PIN = 9;

const int GREEN_LED = 5;
const int YELLOW_LED = 7;
const int RED_LED = 4;

const int BUZZER_PIN = 3;

const byte DISPLAY_TEMPERATURE = 0;
const byte DISPLAY_HUMIDITY = 1;
const byte DISPLAY_LIGHT = 2;
const byte NUMBER_OF_DISPLAY_MODES = 3;

const float TEMP_WARNING = 30;
const float TEMP_DANGER = 35;

const float HUMIDITY_LOW = 40;
const float HUMIDITY_HIGH = 80;

const float LIGHT_DARK = 200;

TM1637 disp(CLK, DIO);
Key key(KEY1_PIN, KEY2_PIN);
DHT dht;
LightSensor light;
PassiveBuzzer buzzer(BUZZER_PIN);

float temperature = 0;
float humidity = 0;
float lightValue = 0;

byte displayMode = DISPLAY_TEMPERATURE;

bool muteAlarm = false;
bool sensorValid = false;

void readSensors(void);
void printSensorReadings(void);
void updateDisplay(void);
void checkEnvironment(void);
void checkButtons(void);
void soundAlarm(void);
void switchDisplayMode(void);
void printDisplayMode(void);

bool isDanger(void);
bool isWarning(void);

void readSensors(void)
{
  float newTemperature;
  float newHumidity;

  newTemperature = dht.readTemperature();
  newHumidity = dht.readHumidity();

  lightValue = light.getRes();

  if (isnan(newTemperature) || isnan(newHumidity))
  {
    sensorValid = false;

    Serial.println("DHT Sensor Error");
    Serial.print("Light: ");
    Serial.println(lightValue);
    Serial.println("-------------------");

    return;
  }

  temperature = newTemperature;
  humidity = newHumidity;
  sensorValid = true;

  printSensorReadings();
}

void printSensorReadings(void)
{
  Serial.println("===== ROOM READINGS =====");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Light: ");
  Serial.println(lightValue);

  if (lightValue > LIGHT_DARK)
  {
    Serial.println("Room lighting: DARK");
  }
  else
  {
    Serial.println("Room lighting: BRIGHT");
  }

  Serial.println("=========================");
}

bool isDanger(void)
{
  return sensorValid && temperature >= TEMP_DANGER;
}

bool isWarning(void)
{
  return sensorValid &&
         (temperature >= TEMP_WARNING ||
          humidity < HUMIDITY_LOW ||
          humidity > HUMIDITY_HIGH);
}

void updateDisplay(void)
{
  switch (displayMode)
  {
    case DISPLAY_TEMPERATURE:

      if (sensorValid)
      {
        disp.display((int)temperature);
      }
      else
      {
        disp.display(0);
      }

      break;

    case DISPLAY_HUMIDITY:

      if (sensorValid)
      {
        disp.display((int)humidity);
      }
      else
      {
        disp.display(0);
      }

      break;

    case DISPLAY_LIGHT:
      disp.display((int)lightValue);
      break;

    default:
      displayMode = DISPLAY_TEMPERATURE;
      disp.display((int)temperature);
      break;
  }
}

void checkEnvironment(void)
{
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  if (!sensorValid)
  {
    digitalWrite(RED_LED, HIGH);
    buzzer.off();

    Serial.println("Status: SENSOR ERROR");
    return;
  }

  if (isDanger())
  {
    digitalWrite(RED_LED, HIGH);
    Serial.println("Status: DANGER");
  }
  else if (isWarning())
  {
    digitalWrite(YELLOW_LED, HIGH);
    Serial.println("Status: WARNING");
  }
  else
  {
    digitalWrite(GREEN_LED, HIGH);
    Serial.println("Status: SAFE");
  }

  soundAlarm();
}

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

void switchDisplayMode(void)
{
  displayMode++;

  if (displayMode >= NUMBER_OF_DISPLAY_MODES)
  {
    displayMode = DISPLAY_TEMPERATURE;
  }

  printDisplayMode();
  updateDisplay();
}

void printDisplayMode(void)
{
  switch (displayMode)
  {
    case DISPLAY_TEMPERATURE:
      Serial.println("Display: TEMPERATURE");
      break;

    case DISPLAY_HUMIDITY:
      Serial.println("Display: HUMIDITY");
      break;

    case DISPLAY_LIGHT:
      Serial.println("Display: LIGHT");
      break;
  }
}

void soundAlarm(void)
{
  if (muteAlarm || !sensorValid)
  {
    buzzer.off();
    return;
  }

  if (isDanger())
  {
    buzzer.playTone(1200, 500);
  }
  else if (isWarning())
  {
    buzzer.playTone(800, 200);
  }
  else
  {
    buzzer.off();
  }
}

void setup(void)
{
  Serial.begin(9600);

  dht.begin();
  disp.init();
  disp.display(0);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  buzzer.off();

  Serial.println("Smart Elderly Room Monitor Started");
  Serial.println("KEY1: Change display");
  Serial.println("KEY2: Mute or enable alarm");
}

void loop(void)
{
  checkButtons();
  readSensors();
  checkEnvironment();
  updateDisplay();
}