#include <Arduino.h>
#include <Stream.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <TimedAction.h>

TimedAction timedAction = TimedAction(1000,sendmessage);

//AWS
#include "sha256.h"
#include "Utils.h"


//WEBSockets
#include <Hash.h>
#include <WebSocketsClient.h>

//MQTT PUBSUBCLIENT LIB 
#include <PubSubClient.h>

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"

extern "C" {
  #include "user_interface.h"
}

//AWS IOT config, change these:
const char * wifi_ssid       = "INFINET";
const char * wifi_password   = "satuduatiga";
const char * aws_endpoint    = "a1gw4ay7j34fac-ats.iot.us-east-1.amazonaws.com";
const char * aws_key         = "AKIAJHCHOJHQKFU5QR3Q";
const char * aws_secret      = "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";
const char * aws_region      = "us-east-1";
const char * aws_topic       = "$aws/things/device-6e5c4b13-357f-4a56-abb9-a22a23f2e505/shadow/update";
int port = 443;

//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

ESP8266WiFiMulti WiFiMulti;

AWSWebSocketClient awsWSclient(1000);

PubSubClient client(awsWSclient);

//# of connections
long connection = 0;

//generate random mqtt clientID
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1)
    cID[i]=(char)random(1, 256);
  return cID;
}

//count messages arrived
int arrivedcount = 0;

//callback to handle mqtt messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//connects to websocket layer and mqtt layer
bool connect () {



    if (client.connected()) {    
        client.disconnect ();
    }  
    //delay is not necessary... it just help us to get a "trustful" heap space value
    delay (1000);
    Serial.print (millis ());
    Serial.print (" - conn: ");
    Serial.print (++connection);
    Serial.print (" - (");
    Serial.print (ESP.getFreeHeap ());
    Serial.println (")");


    //creating random client id
    char* clientID = generateClientID ();
    
    client.setServer(aws_endpoint, port);
    if (client.connect(clientID)) {
      Serial.println("connected");     
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      return false;
    }
    
}

//subscribe to a mqtt topic
void subscribe () {
    client.setCallback(callback);
    client.subscribe(aws_topic);
   //subscript to a topic
    Serial.println("MQTT subscribed");
}

//send a message to a mqtt topic
void sendmessage () {
    if (awsWSclient.connected ()) {   
      client.publish(aws_topic, "{\"message\":\"Hello from ESP8266\"}");
    }
}


void setup() {
    wifi_set_sleep_type(NONE_SLEEP_T);
    Serial.begin (115200);
    delay (2000);
    Serial.setDebugOutput(1);

    //fill with ssid and wifi password
    WiFiMulti.addAP(wifi_ssid, wifi_password);
    Serial.println ("connecting to wifi");
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print (".");
    }
    Serial.println ("\nconnected");

    //fill AWS parameters    
    awsWSclient.setAWSRegion(aws_region);
    awsWSclient.setAWSDomain(aws_endpoint);
    awsWSclient.setAWSKeyID(aws_key);
    awsWSclient.setAWSSecretKey(aws_secret);
    awsWSclient.setUseSSL(true);

    if (connect ()){
      subscribe ();
    }

}

void loop() {
    timedAction.check();
  //keep the mqtt up and running
  if (awsWSclient.connected ()) {    
      client.loop ();
  } else {
    //handle reconnection
    if (connect ()){
      subscribe ();      
    }
  }

}
