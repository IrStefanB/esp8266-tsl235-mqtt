
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//Delete the include and uncomment to define MQ server, MQ port , ssid and password
#include "setup.h"
//#define MQTT_SERVER ""
//const int mqttPort = 1882;
//const char* ssid = "";
//const char* password = "";


//LED on ESP8266 GPIO2
const int lightPin = 2;

//char* lightTopic = "/yourTopic";

volatile unsigned long cnt = 0;
unsigned long oldcnt = 0;
unsigned long t = 0;
unsigned long last;

void irq1()
{
  cnt++;
}

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, mqttPort, callback, wifiClient);

void setup() {
  pinMode(lightPin, INPUT);
  digitalWrite(lightPin, HIGH);
  attachInterrupt(0, irq1, RISING);
  //start the serial line for debugging
  Serial.begin(115200);
  delay(100);


  //start wifi subsystem
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);
}



void loop(){

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

  sensorReading();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10000); 

  
}

void sensorReading() {
    if (millis() - last > 1000){
      last = millis();
      t = cnt;
      unsigned long hz = t - oldcnt;

      if(((hz+50)/100) > 1200) {
          client.publish(lightTopic, "bright");
        }else if(((hz+50)/100) > 600) {
          client.publish(lightTopic, "light");
        }else if(((hz+50)/100) > 250) {
          client.publish(lightTopic, "dim");
        }else if(((hz+50)/100) > 0) {
          client.publish(lightTopic, "low");
        }else {
          client.publish(lightTopic, "off");
        }
      Serial.print("FREQ: "); 
      Serial.print(hz);
      Serial.print("\t = "); 
      Serial.print((hz+50)/100);  // +50 == rounding last digit
      Serial.println(" mW/m2");
      oldcnt = t;
    }
  }

//For subscribing purposes
void callback(char* topic, byte* payload, unsigned int length) {
  //convert topic to string to make it easier to work with
  String topicStr = topic; 

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
}

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMQTT Connected");
        //client.publish(lightTopic);
      }

      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
    }
  }
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}
