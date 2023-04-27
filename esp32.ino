#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RXD2 16
#define TXD2 17

/* WiFi/MQTT Setup */
const char* ssid = "Aaboenet";
const char* pass =  "12341234";

const char* mqtt_server = "109.189.16.89";
const char* mqtt_id = "zumo";

WiFiClient espClient;
PubSubClient client(espClient);
/* --------------- */

const char* sensor;
int speed;
int turn;

void setup_wifi() //connect to WiFi
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  delay(2000);
  WiFi.begin(ssid, pass);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("\nConnected to ");
  Serial.println(ssid);
}

void setup_mqtt() //connect to mosquitto broker
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if(client.connect("zumo")) {
    Serial.println("Connected as zumo");
    if(client.subscribe(mqtt_id)) {
      Serial.print("Subscribed to ");
      Serial.println(mqtt_id);
    } else {
      Serial.println("Subscribe failed.");
    }
  } else {
    client.state();
  }  
}

void callback(char* topic, byte* message, unsigned int length) //callback for incoming data via mqtt
{ 
  String messageTemp;  
  
  /* --- Print incoming data to serial --- */
  
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println("");
    
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, messageTemp);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  serializeJson(doc, Serial2);
  
  sensor = doc["sensor"];
  speed = doc["speed"];
  turn = doc["turn"];
}

void reconnect() //connect to WiFi/MQTT if disconnected
{
  while (!client.connected()) {

    //check if connected to WiFi
    if(WiFi.status() != WL_CONNECTED)
    {
      setup_wifi();
    }

    //attemt to connect to MQTT server
    Serial.println("Attempting MQTT connection");
    while (!client.connect("ESP32_test"))
    { 
      Serial.print("\r                                             ");
      Serial.print("\rFailed, rc=");
      Serial.print(client.state());
      Serial.print(" trying again in 3 seconds");
      delay(1000);
      Serial.print(".");
      delay(1000);
      Serial.print(".");
      delay(1000);
      Serial.print(".");
    } 

    Serial.println("Connected");
    client.subscribe("esp32/controllerOut");
  }
}

void info(void * parameter) {
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  analogReadResolution(9);
  
  setup_wifi();
  setup_mqtt();


}
 
void loop() {
  reconnect();
  client.loop(); //Keeps MQTT checking for new data on the subscribed topic.
}