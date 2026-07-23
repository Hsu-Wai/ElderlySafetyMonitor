#include <Wire.h>
#include "RichShieldDHT.h"
#include "RichShieldTM1637.h"
#include "RichShieldKEY.h"
#include "RichShieldLightSensor.h"
#include "RichShieldPassiveBuzzer.h"

#define CLK 10
#define DIO 11
TM1637 disp(CLK, DIO);
#define KEY1_PIN 8
#define KEY2_PIN 9
Key key(KEY1_PIN, KEY2_PIN);
DHT dht;
LightSensor light;
#define GREEN_LED 5
#define YELLOW_LED 7
#define RED_LED 4
PassiveBuzzer buzzer(3);
float temperature = 0;
float humidity = 0;
float lightValue = 0;
byte displayMode = 0;
bool muteAlarm = false;
const int TEMP_WARNING = 30;
const int TEMP_DANGER = 35;
const int HUMIDITY_LOW = 40;
const int HUMIDITY_HIGH = 80;
const int LIGHT_DARK = 200;
void readSensors()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("Sensor Error");
    return;
  }
  lightValue = light.getRes();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Light: ");
  Serial.println(lightValue);
  Serial.println("-------------------");
}
void updateDisplay()
{
  switch(displayMode)
  {
    case 0:
    disp.display((int)temperature);
    break;
    case 1:
    disp.display((int)humidity);
    break;
    case 2:
    disp.display((int)lightValue);
    break;
  }
}
void checkEnvironment()
{
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  if (temperature >= TEMP_DANGER)
  {
    digitalWrite(RED_LED, HIGH);
    delay(100);
    digitalWrite(RED_LED, LOW);
    delay(100);
    soundAlarm();
  }
  else if (temperature >= TEMP_WARNING || humidity < HUMIDITY_LOW || humidity > HUMIDITY_HIGH)
  {
    digitalWrite(YELLOW_LED, HIGH);
    if (!muteAlarm)
    {
      soundAlarm();
    }
  }
  else
  {
    digitalWrite(GREEN_LED, HIGH);
  }
  Serial.print("Status: ");
  if (temperature >= TEMP_DANGER)
  {
    Serial.println("DANGER");
  }
  else if (temperature >= TEMP_WARNING || humidity < HUMIDITY_LOW || humidity > HUMIDITY_HIGH)
  {
    Serial.println("WARNING");
  }
  else
  {
    Serial.println("SAFE");
  }
  if(lightValue > LIGHT_DARK)
  {
    Serial.println("Room is dark");
  }
}
void checkButtons()
{
  byte keyValue = key.get();
  if (keyValue == 1)
  {
    displayMode++;
    if (displayMode > 2)
    displayMode = 0;
    switch(displayMode)
    {
      case 0:
      Serial.println("Display: Temperature");
      break;
      case 1:
      Serial.println("Display: Humidity");
      break;
      case 2:
      Serial.println("Display: Light");
      break;
    }
  }
  if (keyValue == 2)
  {
    muteAlarm = !muteAlarm;
    if (muteAlarm)
    {
      buzzer.off();
      Serial.println("Alarm Muted");
    }
    else
    {
      Serial.println("Alarm Enabled");
    }
  }
}
void soundAlarm()
{
  if (muteAlarm)
  return;
  if (temperature >= TEMP_DANGER)
  {
   buzzer.playTone(1200, 500);
  }
  else if (temperature >= TEMP_WARNING || humidity < HUMIDITY_LOW || humidity > HUMIDITY_HIGH)
  {
    buzzer.playTone(800, 200);
  }
  else 
  {
    buzzer.off();
  }
}
void setup()
{
  Serial.begin(9600);
  dht.begin();
  disp.init();
  disp.display(0);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  Serial.println("Smart Elderly Room Monitor Started");
}
void loop()
{
  checkButtons();
  readSensors();
  checkEnvironment();
  updateDisplay();
}