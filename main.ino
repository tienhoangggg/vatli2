#include <WiFi.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include "time.h"
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int trig_pin = 19;
int echo_pin = 18;
int addDevice_pin = 5;
int pin_servo = 13;
int key = 0;
bool state = true;
int state_v = 0;
struct tm timeinfo;
const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *idDevice = "30t41975";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * 7;
const int daylightOffset_sec = 3600;
struct schedule
{
	long long time;
	int ml;
	schedule *next = nullptr;
};
class list
{
	schedule *pHead = nullptr;

public:
	bool isEmpty()
	{
		return pHead == nullptr;
	}
	void insert(long long time, int ml);
	void pop();
	schedule get();
};
list l;
void list::insert(long long time, int ml)
{
	schedule *pNew = new schedule;
	pNew->time = time;
	pNew->ml = ml;
	pNew->next = pHead;
	if (pHead == nullptr)
	{
		pHead = pNew;
		return;
	}
	if (pHead->time > pNew->time)
	{
		pNew->next = pHead;
		pHead = pNew;
		return;
	}
	schedule *cur = pHead;
	while (cur->next != nullptr)
	{
		if (cur->next->time < pNew->time)
		{
			cur = cur->next;
			break;
		}
	}
	pNew->next = cur->next;
	cur->next = pNew;
}

void list::pop()
{
	schedule *pDel = pHead;
	pHead = pHead->next;
	delete pDel;
}

schedule list::get()
{
	return *pHead;
}

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
			client.subscribe("binhluuluong_esp32_device");
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
	pinMode(pin_servo, OUTPUT);
	wifiConnect();
	client.setServer(mqttServer, port);
	client.setCallback(callback);
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	myservo.attach(pin_servo);
	srand(time(NULL));
	lcd.init();
	lcd.backlight();
	lcd.setCursor(0, 0);
	lcd.print("ID: ");
	lcd.setCursor(4, 0);
	lcd.print(idDevice);
	myservo.write(90);
	Serial.begin(115200);
	Serial.println("");
}

void addDevice(String strMsg)
{
	String strPass = "", strUser = "", strID = "";
	int index = 0;
	while (strMsg[index] != ' ')
	{
		strID += strMsg[index];
		index++;
	}
	if (strID != idDevice)
		return;
	index++;
	while (strMsg[index] != ' ')
	{
		strUser += strMsg[index];
		index++;
	}
	while (index < strMsg.length() - 1)
	{
		index++;
		strPass += strMsg[index];
	}
	char **e;
	if (key == strtol(strPass.c_str(), e, 10))
		strPass = " T";
	else
		strPass = " F";
	strMsg = strUser + " " + idDevice + strPass;
	client.publish("binhluuluong_nodered_addDevice", strMsg.c_str());
}

void setSchedule(String strMsg)
{
	String time = "", ml = "", strID = "";
	int index = 0;
	while (strMsg[index] != ' ')
	{
		strID += strMsg[index];
		index++;
	}
	if (strID != idDevice)
		return;
	index++;
	while (strMsg[index] != ' ')
	{
		time += strMsg[index];
		index++;
	}
	while (index < strMsg.length() - 1)
	{
		index++;
		ml += strMsg[index];
	}
	char **e;
	l.insert(strtol(time.c_str(), e, 10), strtol(ml.c_str(), e, 10));
}
void sendReserves(String strMsg)
{
	if (strMsg != idDevice)
	{
		return;
	}
	strMsg += " ";
	strMsg += std::to_string(state_v).c_str();
	client.publish("binhluuluong_nodered_reserves", strMsg.c_str());
	return;
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
	if (String(topic) == "binhluuluong_esp32_device")
	{
		sendReserves(strMsg);
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
	int distanceCm = duration * 0.034 / 2;
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
		myservo.write(90);
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

void throwOut(int v)
{
	Serial.println("");
	int v_after = volume() - v;
	if (v_after < 0)
		v_after = 0;
	turnServo(true);
	while (volume() > v_after)
		;
	turnServo(false);
}

void check_schedule()
{
	getLocalTime(&timeinfo);
	time_t now = mktime(&timeinfo);
	if (!l.isEmpty())
		if (l.get().time <= now)
		{
			throwOut(l.get().ml);
			l.pop();
		}
}

void check_button()
{
	bool state_be = state;
	if (digitalRead(addDevice_pin) == HIGH)
	{
		state = !state;
	}
	if (state == true)
	{
		long v = volume();
		if (state_v < v - 1256 || state_v > v || state_be != state)
		{
			state_v = v;
			lcd.setCursor(0, 1);
			lcd.print("                        ");
			lcd.setCursor(0, 1);
			lcd.print(v);
			lcd.setCursor(8, 1);
			lcd.print("ml");
			if (state_v < 300)
				sendReserves(idDevice);
		}
	}
	else if (state_be != state)
	{
		key = random();
		lcd.setCursor(0, 1);
		lcd.print("             ");
		lcd.setCursor(0, 1);
		lcd.print(key);
	}
}

void loop()
{
	if (!client.connected())
	{
		mqttReconnect();
	}
	check_button();
	client.loop();
	check_schedule();
}