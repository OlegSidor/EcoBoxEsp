#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
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
bool send_data = false;

void setup() {
  Serial.begin(115200);
  delay(100);
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  if(resetInfo->reason == 6){
    EEPROM.begin(EEPROM_SIZE);
    for(int i=0; i < 500; i++){
      EEPROM.write(i,0);
    }
    EEPROM.commit();
    EEPROM.end();
  }
  get_password();
  if(ssid != ""){
    if(password != ""){
      WiFi.begin(ssid, password);
    } else WiFi.begin(ssid);
    Serial.print("Connecting to ");
    Serial.print(ssid); Serial.println(" ...");
  
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
      delay(1000);
      Serial.print(++i); Serial.print(' ');
    }
    
    WiFi.softAPdisconnect(true);
    Serial.println('\n');
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    send_data = true;
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Ecobox", NULL);
    WiFi.softAPConfig(IP, IP, mask);
    server.begin();
    Serial.println("Server started.");
    Serial.print("IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("MAC:"); Serial.println(WiFi.softAPmacAddress());
    server.on("/", handle_OnConnect);
    server.on("/setpassword", set_password);
  }
  delay(200);
}
WiFiClient client;
void loop() {
  if(send_data){
    bool StringReady = false;
    String data = "";
    
    while (Serial.available()){
     StringReady = true;
     data = Serial.readString();
    }
    if (StringReady){
      if(data == "CLEAR"){
        
        return;
      }
      HTTPClient http;
      String link = url+data;
      http.begin(link);
  
      int httpCode = http.GET();
    }
  } else {
    server.handleClient();
  }
}


void get_password()
{
  EEPROM.begin(EEPROM_SIZE);
  int len=0;
  char s;
  char p;
  s = char(EEPROM.read(len));
  while(s != '\0' && len<500)
  {    
    s = EEPROM.read(len);
    ssid += char(EEPROM.read(len));
    len++;
  }
  Serial.println("");
  if(len >= 499){
    ssid = "";
    return;
  }
  p = EEPROM.read(len);
  while(p != '\0' && len<500){
    p = EEPROM.read(len);
    password += char(EEPROM.read(len));
    len++;
  }
  
  if(len >= 499){
    password = "";
    return;
  }
  EEPROM.end();
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}


String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>EcoBox</title>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<form action='setpassword' method='POST'>\n";
  ptr +="<input type='text' name='ssid' placeholder='ssid'><br>\n";
  ptr +="<input type='password' name='password' placeholder='password'><br>\n";
  ptr +="<input type='submit' name='submit' value='Submit'><br>\n";
  ptr +="</form>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void set_password(){
  EEPROM.begin(EEPROM_SIZE);
  String ssid_ = "";
  String password_ = "";
  if (server.hasArg("ssid")){
      ssid_ = server.arg("ssid")+'\0';
  }
  if (server.hasArg("password")){
     password_ = server.arg("password")+'\0';
  }
  
  int _size_s = ssid_.length();
  int _size_p = password_.length();
  int i;
  for(i=0;i<_size_s;i++)
  {
    EEPROM.write(i,ssid_[i]);
  }
  EEPROM.write(i,'\0');
  i++;
  int j;
  for(j=0;j<_size_p;j++)
  {
    EEPROM.write(i+j,password_[j]);
  }
  EEPROM.write(i+j,'\0');
  EEPROM.commit();
  EEPROM.end();
  get_password();
  ESP.restart();
}
