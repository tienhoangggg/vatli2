#include <WiFi.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include "time.h"
#include <LiquidCrystal_I2C.h>
#include <string>
int trig_pin = 2;
int echo_pin = 4;
int addDevice_pin = 5;
int pin_servo = 18;
int key = 0;
int countTime = 0;
struct tm timeinfo;
const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *idDevice = "30t41975";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * 7;
const int daylightOffset_sec = 3600;
String schedule[10];

//***Set server***
const char *mqttServer = "broker.mqtt-dashboard.com";
int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
void wifiConnect()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

void mqttReconnect()
{
  while (!client.connected())
  {
    if (client.connect(idDevice))
    {
      //***Subscribe all topic you need***
      client.subscribe("binhluuluong_esp32_key");
      client.subscribe("binhluuluong_esp32_schedule");
    }
    else
    {
      delay(2000);
    }
  }
}

void setup()
{
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  pinMode(addDevice_pin, INPUT);
  wifiConnect();
  client.setServer(mqttServer, port);
  client.setCallback(callback);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  myservo.attach(pin_servo);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ID: 29t41975 ");
  lcd.setCursor(0, 1);
  lcd.print(volume());
  lcd.setCursor(8, 1);
  lcd.print("ml");
  Serial.begin(115200);
}

void addDevice(String strMsg)
{
  String strKey = "", strPass = "", strUser = "", strID = "";
  while (key > 0)
  {
    strKey = (char)(key % 10 + 48) + strKey;
    key /= 10;
  }
  int index2 = strMsg.length() - 1, index1 = 0;
  while (strMsg[index1] != ' ')
    index1++;
  for (int i = 0; i < index1; i++)
    strID += strMsg[i];
  if (strID != idDevice)
    return;
  while (strMsg[index2] != ' ')
    index2--;
  for (int i = index1 + 1; i < index2; i++)
    strUser += strMsg[i];
  for (int i = index2 + 1; i < strMsg.length(); i++)
    strPass += strMsg[i];
  if (strKey == strPass)
    strPass = " T";
  else
    strPass = " F";
  strMsg = strUser + " " + idDevice + strPass;
  client.publish("binhluuluong_nodered_addDevice", strMsg.c_str());
}

void setSchedule(String strMsg)
{
  for (int i = 1; i < 10; i++)
    schedule[i] = schedule[i - 1];
  schedule[0] = strMsg;
}
// MQTT Receiver
void callback(char *topic, byte *message, unsigned int length)
{
  String strMsg;
  for (int i = 0; i < length; i++)
  {
    strMsg += (char)message[i];
  }
  if (String(topic) == "binhluuluong_esp32_key")
  {
    addDevice(strMsg);
    return;
  }
  if (String(topic) == "binhluuluong_esp32_schedule")
  {
    setSchedule(strMsg);
    return;
  }
}

long getDistance()
{
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);
  long duration = pulseIn(echo_pin, HIGH);
  long distanceCm = duration * 0.034 / 2;
  return distanceCm;
}

void turnServo(bool s)
{
  if (s == true)
  {
    myservo.write(180);
  }
  else if (s == false)
  {
    myservo.write(0);
  }
}

long volume()
{
  long h = 50 - getDistance();
  long v = 0;
  if (h > 20)
  {
    v += (h - 20) * 1256;
    h = 20;
  }
  v += 25120 * h * h * h / 24000;
  return v;
}

void throwOut(long v)
{
  long v_after = volume() - v;
  if (v_after < 0)
    v_after = 0;
  turnServo(true);
  while (volume() > v_after)
    ;
  turnServo(false);
}

void loop()
{
  if (!client.connected())
  {
    mqttReconnect();
  }
  if (digitalRead(addDevice_pin) == HIGH)
  {
    key = random();
    lcd.setCursor(0, 1);
    lcd.print("             ");
    lcd.setCursor(0, 1);
    lcd.print(random());
    delay(30000);

    if (digitalRead(addDevice_pin) == LOW)
    {
      lcd.setCursor(0, 1);
      lcd.print("                        ");
      lcd.setCursor(0, 1);
      lcd.print(volume());
      lcd.setCursor(8, 1);
      lcd.print("ml");
    }
    Serial.println(key);
  }

  getLocalTime(&timeinfo);
  Serial.println(timeinfo.tm_sec);
  client.loop();
}