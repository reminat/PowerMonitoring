#include "ArduinoOTA.h"
#include <ESP8266WiFi.h>
#include <iostream>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <string>

const char *inTopic = "PZEMIN/+/+";
const char *outTopic = "PZEMOUT/data/PZEM99";
int interval = INTERVAL;

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem1(pzemSWSerial, 0x01);
PZEM004Tv30 pzem2(pzemSWSerial, 0x02);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFISSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFISSID, WIFIPASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wifi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMqtt()
{
  while (!mqttClient.connected())
  {
    Serial.println();
    Serial.print("Attempting MQTT connection ...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTTUSER, MQTTPASSWORD))
    {
      Serial.println("MQTT connected");
      mqttClient.subscribe(inTopic);
    }
    else
    {
      Serial.println();
      Serial.print("Failed, rc=");
      Serial.println(mqttClient.state());
      Serial.println("Try again in 5 sec");
      delay(5000);
    }
  }
}
String format(
    float voltage,
    float current,
    float power,
    float energy,
    float frequency,
    float pf)
{
  String message = "{";
  if (!isnan(voltage))
  {
    message.concat("\"voltage\": ");
    message.concat(voltage);
    message.concat(",");
  }
  if (!isnan(current))
  {
    message.concat("\"current\": ");
    message.concat(current);
    message.concat(",");
  }
  if (!isnan(power))
  {
    message.concat("\"power\": ");
    message.concat(power);
    message.concat(",");
  }
  if (!isnan(energy))
  {
    message.concat("\"energy\": ");
    message.concat(energy);
    message.concat(",");
  }
  if (!isnan(frequency))
  {
    message.concat("\"frequency\": ");
    message.concat(frequency);
    message.concat(",");
  }
  if (!isnan(pf))
  {
    message.concat("\"pf\": ");
    message.concat(pf);
  }
  message.concat("}");
  return message;
}
void handleMqttMessage(char *topic, byte *payload, unsigned int length)
{
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String key = String(topic).substring(strlen(inTopic) + strlen(DEVICENAME) - 2, strlen(topic));
  Serial.println(key);

  if (key == "interval")
  {
    interval = atoi((char *)payload);
  }
  else if (key == "reset")
  {
    pzem1.resetEnergy();
  }
  Serial.println();
};
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  ArduinoOTA.begin();
  ArduinoOTA.setHostname(DEVICENAME);
  ArduinoOTA.setPassword(MQTTPASSWORD);
  randomSeed(micros());
  mqttClient.setServer(MQTTSERVER, 1883);
  mqttClient.setCallback(handleMqttMessage);
}

void loop()
{
  if (!mqttClient.connected())
  {
    connectMqtt();
  }
  mqttClient.loop();
  ArduinoOTA.handle();
  Serial.println("==========");

  Serial.print("Custom Address:");

  Serial.println(pzem1.readAddress(), HEX);

  float voltage = pzem1.voltage();
  float current = pzem1.current();
  float power = pzem1.power();
  float energy = pzem1.energy();
  float frequency = pzem1.frequency();
  float pf = pzem1.pf();

  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println("V");
  Serial.print("Current: ");
  Serial.print(current);
  Serial.println("A");
  Serial.print("Power: ");
  Serial.print(power);
  Serial.println("W");
  Serial.print("Energy: ");
  Serial.print(energy, 3);
  Serial.println("kWh");
  Serial.print("Frequency: ");
  Serial.print(frequency, 1);
  Serial.println("Hz");
  Serial.print("PF: ");
  Serial.println(pf);
  mqttClient.publish("test/1", format(voltage, current, power, energy, frequency, pf).c_str());

  Serial.println();
  // delay(interval * 1000);

  Serial.print("Custom Address:");

  Serial.println(pzem2.readAddress(), HEX);

  float voltage2 = pzem2.voltage();
  float current2 = pzem2.current();
  float power2 = pzem2.power();
  float energy2 = pzem2.energy();
  float frequency2 = pzem2.frequency();
  float pf2 = pzem2.pf();
  Serial.println();
  Serial.println();

  Serial.print("Voltage 2: ");
  Serial.print(voltage2);
  Serial.println("V");
  Serial.print("Current 2: ");
  Serial.print(current2);
  Serial.println("A");
  Serial.print("Power 2: ");
  Serial.print(power2);
  Serial.println("W");
  Serial.print("Energy 2: ");
  Serial.print(energy2, 3);
  Serial.println("kWh");
  Serial.print("Frequency 2: ");
  Serial.print(frequency2, 1);
  Serial.println("Hz");
  Serial.print("PF 2: ");
  Serial.println(pf2);
  mqttClient.publish("test/2", format(voltage2, current2, power2, energy2, frequency2, pf2).c_str());

  Serial.println();
  delay(interval * 1000);
}