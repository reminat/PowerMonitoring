#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"
#include <iostream>
#include <string>
/* credentials.h
const char *wifiPwd = "...";
const char *mqttPwd = "...";
*/
const char *device = "PZEM01";
const char *inTopic = "PZEMIN/+/+";
const char *outTopic = "PZEMOUT/data";
int interval = 5;

const char *ssid = "Reminat";
const char *mqttServer = "mosquitto.reminat.com";
const char *mqttUser = "remi";

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPwd);

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
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPwd))
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
    message.concat(",");
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
  String key = String(topic).substring(strlen(inTopic) + strlen(device) - 2, strlen(topic));
  Serial.println(key);

  if (key == "interval")
  {
    interval = atoi((char *)payload);
  }
  else if (key == "reset")
  {
    pzem.resetEnergy();
  }
  Serial.println();
};
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  randomSeed(micros());
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(handleMqttMessage);
}

void loop()
{
  if (!mqttClient.connected())
  {
    connectMqtt();
  }
  mqttClient.loop();

  Serial.print("Custom Address:");
  Serial.println(pzem.readAddress(), HEX);

  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

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
  mqttClient.publish(outTopic, format(voltage, current, power, energy, frequency, pf).c_str());

  Serial.println();
  delay(interval * 1000);
}