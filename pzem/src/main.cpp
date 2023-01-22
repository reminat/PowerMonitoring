#include "ArduinoOTA.h"
#include <ESP8266WiFi.h>
#include <iostream>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <string>
#include <map>

const char *inTopic = "PZEMIN/+/+";
const char *outTopic = "PZEMOUT/" DEVICENAME;
int interval = INTERVAL;

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
uint8_t addr[] = ADDRESSES;
int addrLength = sizeof(addr) / sizeof(addr[0]);
std::map<uint8_t, PZEM004Tv30> pzem;

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
    float pf,
    uint8_t addr)
{
  String message = "{addr: ";
  message.concat(addr);
  message.concat(",");
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
    message.concat(energy * 1000);
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

    pzem.find(atoi((char *)payload))->second.resetEnergy();
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

  for (size_t i = 0; i < 6; i++)
  {
    mqttClient.publish("PZEM99/Serial", "ok");
    uint8_t ad = addr[i];
    pzem.insert({ad, PZEM004Tv30(pzemSWSerial, ad)});
  }
}

void loop()
{
  // Serial.println();
  if (!mqttClient.connected())
  {
    connectMqtt();
  }
  mqttClient.loop();
  ArduinoOTA.handle();

  for (auto itr = pzem.begin(); itr != pzem.end(); ++itr)
  {

    delay(100);
    Serial.println("==========");

    Serial.print("Custom Address:");

    uint8_t currentAddr = itr->second.readAddress();
    Serial.println(currentAddr, HEX);

    float voltage = itr->second.voltage();
    float current = itr->second.current();
    float power = itr->second.power();
    float energy = itr->second.energy();
    float frequency = itr->second.frequency();
    float pf = itr->second.pf();

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
    String topic = outTopic;
    topic.concat("/");
    topic.concat(itr->first);
    mqttClient.publish(topic.c_str(), format(voltage, current, power, energy, frequency, pf, currentAddr).c_str());

    Serial.println();
  }

  delay(interval * 1000);
}