#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"

const char *device = "PZEM01";
const char *topic = "PZEM/";
int interval = 2;

/* creentials.h
const char *wifiPwd = "...";
const char *mqttPwd = "...";
*/
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
      mqttClient.subscribe(topic);
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

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  randomSeed(micros());
  mqttClient.setServer(mqttServer, 1883);
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

  if (isnan(voltage))
  {
    Serial.println("Error reading voltage");
  }
  else if (isnan(current))
  {
    Serial.println("Error reading current");
  }
  else if (isnan(power))
  {
    Serial.println("Error reading power");
  }
  else if (isnan(energy))
  {
    Serial.println("Error reading energy");
  }
  else if (isnan(frequency))
  {
    Serial.println("Error reading frequency");
  }
  else if (isnan(pf))
  {
    Serial.println("Error reading power factor");
  }
  else
  {

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
  }

  Serial.println();
  delay(interval * 1000);
}