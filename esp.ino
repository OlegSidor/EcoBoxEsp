#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <SPI.h>

extern "C" {
#include <user_interface.h>
}


#define EEPROM_SIZE 512
#define resetPin 7
char url[] = "http://ecoteam.com.ua:8000";

ESP8266WebServer server(80);
IPAddress IP(192,168,5,1);
IPAddress mask = (255, 255, 255, 0);

String ssid     = "";
String password = "";
String device_id = "0";
bool send_data = false;
bool started = false;
void setup() {
  Serial.begin(115200);
  delay(100);
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  if(resetInfo->reason == 6){
    EEPROM.begin(EEPROM_SIZE);
    for(int i=0; i < 1026; i++){
      EEPROM.write(i,0);
    }
    EEPROM.commit();
    EEPROM.end();
  }
  get_password();
  if(ssid != ""){
    if(password != "*NULL*"){
      WiFi.begin(ssid, password);
    } else WiFi.begin(ssid);
    Serial.print("Connecting to ");
    Serial.print(ssid); Serial.println(" ...");
  
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
      delay(1000);
      Serial.print(++i); Serial.print(' ');
      if(i >= 10){
        break;
        startServer();
      }
    }
    
    WiFi.softAPdisconnect(true);
    Serial.println('\n');
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    Serial.print("Devide_id:");
    Serial.println(device_id);
    send_data = true;
  } else {
    startServer();
  }
  delay(200);
}
WiFiClient client;
void loop() {
  if (WiFi.status() != WL_CONNECTED && !started)
  {
    startServer();
    send_data = false;
  }
  if(send_data){
    bool StringReady = false;
    String data = "";
    
    while (Serial.available()){
     StringReady = true;
     data = Serial.readString();
    }
    if (StringReady && device_id != "0"){
      if(data == "CLEAR"){
        
        return;
      }
      HTTPClient http;
      long rssi = WiFi.RSSI();
      String link = url+data+"&wf="+rssi+"&device_id="+device_id;
      Serial.println(link);
      http.begin(link);
  
      int httpCode = http.GET();
    }
  } else {
    server.handleClient();
  }
}


void get_password()
{
  
  DynamicJsonDocument doc(1024);
  
  int len=0;
  char c;
  String json_string = "";
  
  EEPROM.begin(EEPROM_SIZE);
  c = EEPROM.read(len);
  while(c != '\0' && len<1025){
    c = EEPROM.read(len);
    json_string += c;
    len++;
  }
  EEPROM.end();

  deserializeJson(doc, json_string);
  JsonObject data = doc.as<JsonObject>();
  
  ssid      = (const char*)data["ssid"];
  password  = (const char*)data["password"];
  device_id = (const char*)data["device_id"];

}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML(ssid, password, device_id == "0" ? "" : device_id)); 
}

String SendHTML(String s, String p, String d){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>EcoBox</title>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<form action='setpassword' method='POST'>\n";
  ptr +="<input type='text' name='ssid' placeholder='ssid'><br>\n";
  ptr +="<input type='password' name='password' placeholder='password'><br>\n";
  ptr +="<input type='text' name='device_id' placeholder='devide_id'><br>\n";
  ptr +="<input type='submit' name='submit' value='Submit'><br>\n";
  ptr +="</form>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void set_password(){
  EEPROM.begin(EEPROM_SIZE);
  String ssid_ = "";
  String password_ = "*NULL*\0";
  String device_id_ = "";
  if (server.hasArg("ssid")){
      ssid_ = server.arg("ssid")+'\0';
  }
  if (server.hasArg("password") && server.arg("password") != ""){
     password_ = server.arg("password")+'\0';
  }
  if (server.hasArg("device_id")){
     device_id_ =  server.arg("device_id")+'\0';
  }

  String json_string = "";
  DynamicJsonDocument doc(1024);
  JsonObject data = doc.to<JsonObject>();
  
  data["ssid"] = ssid_;
  data["password"] = password_;
  data["device_id"] = device_id_;

  serializeJson(doc, json_string);

  Serial.println(json_string);
  
  json_string += '\0';

  for(int i = 0; i < json_string.length(); i++){
    EEPROM.write(i,json_string[i]);
  }
  
  EEPROM.commit();
  EEPROM.end();
  get_password();
  ESP.restart();
}
void startServer(){
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Ecobox", NULL);
    WiFi.softAPConfig(IP, IP, mask);
    server.begin();
    Serial.println("Server started.");
    Serial.print("IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("MAC:"); Serial.println(WiFi.softAPmacAddress());
    server.on("/", handle_OnConnect);
    server.on("/setpassword", set_password);
    started = true;
}
