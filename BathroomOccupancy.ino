#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "Bond";
const char* password = "5mmm575b";
const char* host = "35.167.48.154";
const int port = 8080;

IPAddress ip(192,168,9,251); //Requested static IP address for the ESP
IPAddress router(192,168,9,1); // IP address for the Wifi router
IPAddress netmask(255,255,255,0);

ESP8266WebServer server(80);

//the time we give the sensor to calibrate (10-60 secs according to the datasheet)
int calibrationTime = 30;

//the time when the sensor outputs a low impulse
long unsigned int lowIn;
long unsigned int prevUpdateTime = 0;   //the prev time update that esp is running is sent to the server
long unsigned int checkUpdate = 120000; //1200000; //the esp sends updates to server every 10 mins to tell its active

//the amount of milliseconds the sensor has to be low 
//before we assume all motion has stopped
long unsigned int pause = 10000;
long unsigned int last_motion_at = 0; 

boolean lockLow = true;
boolean takeLowTime;

int pirPin = D2;    //the digital pin connected to the PIR sensor's output
int ledPin = D7;

int clientTimeout = 5;
boolean previousBathroomStatus = false; // false => bathroom is not free , true = bathroom is free

void handleRoot() {
    server.send(200, "text/html", "Welcome to nodemcu server. This nodemcu is dedicated for bathroom occupancy");
}

void handleNotFound(){
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void setup(){
    Serial.begin(115200);

    WiFi.config(ip,router,netmask);
    WiFi.begin(ssid, password);
    Serial.println("");
  
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }
  
    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");

    pinMode(pirPin, INPUT);
    pinMode(ledPin, OUTPUT);
    digitalWrite(pirPin, LOW);

    //give the sensor some time to calibrate
    Serial.println();
    Serial.print("calibrating sensor ");
    for(int i = 0; i < calibrationTime; i++) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(" done");
    Serial.println("SENSOR ACTIVE");
    delay(50);
}

void loop(){
     if(digitalRead(pirPin) == HIGH){
         digitalWrite(ledPin, HIGH);   //the led visualizes the sensors output pin state
         if(lockLow){
             //makes sure we wait for a transition to LOW before any further output is made:
             lockLow = false;
             Serial.println("---");
             Serial.print("motion detected at ");
             Serial.print(millis()/1000);
             Serial.println(" sec");
             delay(50);
         }
         takeLowTime = true;
     }

     if(digitalRead(pirPin) == LOW) {
         digitalWrite(ledPin, LOW);  //the led visualizes the sensors output pin state

         if(takeLowTime){
          lowIn = millis();          //save the time of the transition from high to LOW
          takeLowTime = false;       //make sure this is only done at the start of a LOW phase
          }
         //if the sensor is low for more than the given pause, 
         //we assume that no more motion is going to happen
         if(!lockLow && millis() - lowIn > pause) {
             //makes sure this block of code is only executed again after
             //a new motion sequence has been detected
             lockLow = true;
             Serial.print("motion ended at ");      //output
             last_motion_at = (millis() - pause)/1000;
             Serial.print((millis() - pause)/1000);
             Serial.println(" sec");
             delay(50);
         }
     }
     boolean isFree = false;
     if(((millis()/1000) - last_motion_at > 5) && !!lockLow) {
         isFree = true;
     }
     else {
         isFree = false;
     }
     if(isFree != previousBathroomStatus) {
        String text = (!!isFree) ? " free " : " occupied ";
        Serial.println("bathroom is" + text + "connecting to aws server");
        WiFiClient client;
        while (!client.connect(host, port) && clientTimeout > 0) {
          Serial.println("connection failed");
          clientTimeout -= 1;
          return;
        }
        client.print("GET /updateBathroomStatus?status=" + String(isFree) +" HTTP/1.1\r\n Host: " + String(host) + "\r\n" +
               "Connection: close\r\n\r\n");
        delay(10);
        while (client.available()) {
          String line = client.readStringUntil('\r');
          Serial.print(line);
        }
        Serial.println();
        Serial.println("closing connection");
        Serial.println();
        previousBathroomStatus = isFree;
        prevUpdateTime = millis();
     }
     if (millis() - prevUpdateTime > checkUpdate){
        String text = "1";
        Serial.print(text);
        Serial.println(" sending device is active update to aws server");
        WiFiClient client;
        while (!client.connect(host, port) && clientTimeout > 0) {
          Serial.println("connection failed");
          clientTimeout -= 1;
          return;
        }
        client.print("GET /updateDeviceStatus?status=" + String(text) +" HTTP/1.1\r\n Host: " + String(host) + "\r\n" +
               "Connection: close\r\n\r\n");
        delay(10);
        while (client.available()) {
          String line = client.readStringUntil('\r');
          Serial.print(line);
        }
        Serial.println();
        Serial.println("closing connection");
        Serial.println();
        prevUpdateTime = millis();
     }
     server.handleClient();
}


