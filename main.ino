#include <WiFi.h>
#include <Servo.h>
#include <PubSubClient.h>
int trig_pin = 2;
int echo_pin = 4;
int addDevice_pin = 0;
int reset_pin = 0;
int pin_servo = 9;
const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *idDevice = "30t41975";

//***Set server***
const char *mqttServer = "broker.mqtt-dashboard.com";
int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

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
    if (client.connect(idDevice)))
      {

        //***Subscribe all topic you need***
        client.subscribe("binhluuluong/");
      }
    else
    {
      delay(5000);
    }
  }
}

void setup()
{
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  wifiConnect();
  client.setServer(mqttServer, port);
  client.setCallback(callback);

  myservo.attach(pin_servo);
}

// MQTT Receiver
void callback(char *topic, byte *message, unsigned int length)
{
  String strMsg;
  for (int i = 0; i < length; i++)
  {
    strMsg += (char)message[i];
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

long volume(){
  long h = 60 - getDistance();
  long v = 0;
  if (h > 20) {
    v += (h-20)*1256;
    h = 20;
  }
  v += 25120 * h * h * h / 24000;
  return v;
}

void throwOut(long v){
  long v_after = volume() - v;
  if (v_after < 0) v_after = 0;
  turnServo(true);
  while(volume() > v_after);
  turnServo(false);
}

void loop()
{
  if (!client.connected())
  {
    mqttReconnect();
  }
  client.loop();
}
