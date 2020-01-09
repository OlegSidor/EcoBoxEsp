#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <regex>
extern "C" {
#include <user_interface.h>
}


#define EEPROM_SIZE 1024
#define resetPin 7
String url = "http://ecoteam.com.ua:8000";

ESP8266WebServer server(80);
IPAddress IP(192,168,5,1);
IPAddress mask(255, 255, 255, 0);
ESP8266WiFiMulti WiFiMulti;
String ssid     = "";
String password = "";
String device_id = "0";
String server_password = "";

String pm0_1        = "0";
String pm2_5        = "0";
String pm10         = "0";
String temperature  = "0";
String humidity     = "0";
String co2          = "0";
String rssi         = "0";

void setup() {
  Serial.begin(115200);
  delay(100);
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  if(resetInfo->reason == 6){
    EEPROM.begin(EEPROM_SIZE);
    for(int i=0; i < 1024; i++){
      EEPROM.write(i,0);
    }
    EEPROM.commit();
    EEPROM.end();
  }
  get_password();
  Serial.flush();
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  WiFi.softAP("Ecobox", server_password);
  WiFi.softAPConfig(IP, IP, mask);
  server.begin();
  Serial.println("Server started.");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());
  Serial.print("MAC:"); Serial.println(WiFi.softAPmacAddress());
  Serial.println(server_password);
  server.on("/", send_status);
  server.on("/setpassword", set_password);
  server.on("/connect", handle_OnConnect);
  server.on("/logout", logout);
  server.on("/changeServerPassword", changeServerPassword);
  
  if(ssid != "") wifiConnect();
  
  delay(200);
}

void loop() {
    bool StringReady = false;
    String data = "";
    
    while (Serial.available()){
     StringReady = true;
     data = Serial.readString();
    }
    if (StringReady && device_id != "0" && data.startsWith("{")){
      if(data == "CLEAR"){
        
        return;
      }
      rssi = (String) WiFi.RSSI();

      DynamicJsonDocument doc(512);
      deserializeJson(doc, data);
      JsonObject sensors = doc.as<JsonObject>();

      pm0_1 =       (const char*)sensors["pm0_1"];
      pm2_5 =       (const char*)sensors["pm2_5"];
      pm10 =        (const char*)sensors["pm10"];
      temperature = (const char*)sensors["temperature"];
      humidity =    (const char*)sensors["humidity"];
      co2 =         (const char*)sensors["co2"];
      
      String link = url+"/air/add/?lat=48.524283&lon=24.984165&sat=9&alt=245.00&speed=0.20&pm0_1="+String(pm0_1)+"&pm2_5="+String(pm2_5)+"&pm10="+String(pm10)+"&temperature="+String(temperature)+"&humidity="+String(humidity)+"&co2="+String(co2)+"&uid=0&wf="+rssi+"&device_id="+device_id;
      Serial.println(link);
        if (WiFiMulti.run() == WL_CONNECTED){
          WiFiClient client;
          HTTPClient http;
          http.begin(client, link);
          int httpCode = http.GET();
          Serial.println("ASD");
          Serial.println(httpCode);
          http.end();
        }
  }
  server.handleClient();
  
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
  server_password = (const char*)data["server_password"];
}

void handle_OnConnect() {
  server.send(200, "text/html", login_form()); 
}

String login_form(){
  String ptr = "<!DOCTYPE html> <html lang='en'> <head> <meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'> <title>EcoBox</title> </head> <style> * { padding: 0; margin: 0 } body { position: relative; height: 100vh; background: #FF5F6D; background: -webkit-linear-gradient(to left, #FFC371, #FF5F6D); background: linear-gradient(to left, #FFC371, #FF5F6D); } .wrap { width: 250px; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); text-align: center; color: white; font-family: Arial; } form { background: white; border-radius: 5px; padding: 10px; } input { margin: 5px 0; display: block; padding: 5px; width: 100%; box-sizing: border-box; } input[type=submit] { background: limegreen; border: none; border-radius: 2px; color: white; } a { float: right; font-size: 13px; margin-top: 5px; color: white; text-decoration: none; } </style> <body> <div class='wrap'> <h1>EcoBox</h1> <form action='setpassword' method='POST'> <input type='text' name='ssid' placeholder='SSID' required> <input type='password' name='password' placeholder='Password' required> <input type='number' name='device_id' placeholder='Devide ID'> <input type='submit' name='submit' value='Connect'> </form> <a href='http://ecoteam.com.ua:8000/'>EcoTeam</a> </div> </body> </html>";
  return ptr;
}

void send_status(){
  String ptr = "<!DOCTYPE html> <html lang='en'> <head> <meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'> <meta charset='UTF-8'> <title>EcoBox</title> </head>";
  ptr += "<style> table { margin-top: 50px; border-collapse: collapse; width: 100%; border: solid rgba(0, 0, 0, 0.3) 1px; } td { text-align: left; padding: 8px; } tr:nth-child(even) { background-color: #c7c7c7; } body { width: 250px; margin: 0 auto; text-align: center; } .status { font-size: 20px; font-weight: bold; } .status.success { color: #4CAF50; } .status.error { color: #FF5F6D; } .description{ font-weight: bold; } .f-modal-alert { background-color: #fff; border-radius: 4px; margin-top: 20px; padding: 5px; } .f-modal-alert .f-modal-icon { border-radius: 50%; border: 4px solid gray; box-sizing: content-box; height: 80px; margin: 20px auto; padding: 0; position: relative; width: 80px; } .f-modal-alert .f-modal-icon.f-modal-success, .f-modal-alert .f-modal-icon.f-modal-error { border-color: #A5DC86; } .f-modal-alert .f-modal-icon.f-modal-success:after, .f-modal-alert .f-modal-icon.f-modal-success:before, .f-modal-alert .f-modal-icon.f-modal-error:after, .f-modal-alert .f-modal-icon.f-modal-error:before { background: #fff; content: ''; height: 120px; position: absolute; -webkit-transform: rotate(45deg); transform: rotate(45deg); width: 60px; } .f-modal-alert .f-modal-icon.f-modal-success:before, .f-modal-alert .f-modal-icon.f-modal-error:before { border-radius: 120px 0 0 120px; left: -33px; top: -7px; -webkit-transform-origin: 60px 60px; transform-origin: 60px 60px; -webkit-transform: rotate(-45deg); transform: rotate(-45deg); } .f-modal-alert .f-modal-icon.f-modal-success:after, .f-modal-alert .f-modal-icon.f-modal-error:after { border-radius: 0 120px 120px 0; left: 30px; top: -11px; -webkit-transform-origin: 0 60px; transform-origin: 0 60px; -webkit-transform: rotate(-45deg); transform: rotate(-45deg); } .f-modal-alert .f-modal-icon.f-modal-success .f-modal-placeholder, .f-modal-alert .f-modal-icon.f-modal-error .f-modal-placeholder { border-radius: 50%; border: 4px solid rgba(165, 220, 134, 0.2); box-sizing: content-box; height: 80px; left: -4px; position: absolute; top: -4px; width: 80px; z-index: 2; } .f-modal-alert .f-modal-icon.f-modal-success .f-modal-fix, .f-modal-alert .f-modal-icon.f-modal-error .f-modal-fix { background-color: #fff; height: 90px; left: 28px; position: absolute; top: 8px; -webkit-transform: rotate(-45deg); transform: rotate(-45deg); width: 5px; z-index: 1; } .f-modal-alert .f-modal-icon.f-modal-success .f-modal-line, .f-modal-alert .f-modal-icon.f-modal-error .f-modal-line { background-color: #A5DC86; border-radius: 2px; display: block; height: 5px; position: absolute; z-index: 2; } .f-modal-alert .f-modal-icon.f-modal-success .f-modal-line.f-modal-tip, .f-modal-alert .f-modal-icon.f-modal-error .f-modal-line.f-modal-tip { left: 14px; top: 46px; -webkit-transform: rotate(45deg); transform: rotate(45deg); width: 25px; } .f-modal-alert .f-modal-icon.f-modal-success .f-modal-line.f-modal-long, .f-modal-alert .f-modal-icon.f-modal-error .f-modal-line.f-modal-long { right: 8px; top: 38px; -webkit-transform: rotate(-45deg); transform: rotate(-45deg); width: 47px; } .f-modal-alert .f-modal-icon.f-modal-error { border-color: #F27474; } .f-modal-alert .f-modal-icon.f-modal-error .f-modal-x-mark { display: block; position: relative; z-index: 2; } .f-modal-alert .f-modal-icon.f-modal-error .f-modal-placeholder { border: 4px solid rgba(200, 0, 0, 0.2); } .f-modal-alert .f-modal-icon.f-modal-error .f-modal-line { background-color: #F27474; top: 37px; width: 47px; } .f-modal-alert .f-modal-icon.f-modal-error .f-modal-line.f-modal-left { left: 17px; -webkit-transform: rotate(45deg); transform: rotate(45deg); } .f-modal-alert .f-modal-icon.f-modal-error .f-modal-line.f-modal-right { right: 16px; -webkit-transform: rotate(-45deg); transform: rotate(-45deg); } .f-modal-alert .f-modal-icon + .f-modal-icon { margin-top: 50px; } .animateSuccessTip { -webkit-animation: animateSuccessTip .75s; animation: animateSuccessTip .75s; } .animateSuccessLong { -webkit-animation: animateSuccessLong .75s; animation: animateSuccessLong .75s; } .f-modal-icon.f-modal-success.animate:after { -webkit-animation: rotatePlaceholder 4.25s ease-in; animation: rotatePlaceholder 4.25s ease-in; } .f-modal-icon.f-modal-error.animate:after { -webkit-animation: rotatePlaceholder 4.25s ease-in; animation: rotatePlaceholder 4.25s ease-in; } .animateXLeft { -webkit-animation: animateXLeft .75s; animation: animateXLeft .75s; } .animateXRight { -webkit-animation: animateXRight .75s; animation: animateXRight .75s; }";
  ptr += "@-webkit-keyframes animateSuccessTip { 0%, 54% { width: 0; left: 1px; top: 19px; } 70% { width: 50px; left: -8px; top: 37px; } 84% { width: 17px; left: 21px; top: 48px; } 100% { width: 25px; left: 14px; top: 45px; } } @keyframes animateSuccessTip { 0%, 54% { width: 0; left: 1px; top: 19px; } 70% { width: 50px; left: -8px; top: 37px; } 84% { width: 17px; left: 21px; top: 48px; } 100% { width: 25px; left: 14px; top: 45px; } } @-webkit-keyframes animateSuccessLong { 0%, 65% { width: 0; right: 46px; top: 54px; } 84% { width: 55px; right: 0; top: 35px; } 100% { width: 47px; right: 8px; top: 38px; } } @keyframes animateSuccessLong { 0%, 65% { width: 0; right: 46px; top: 54px; } 84% { width: 55px; right: 0; top: 35px; } 100% { width: 47px; right: 8px; top: 38px; } } @-webkit-keyframes rotatePlaceholder { 0%, 5% { -webkit-transform: rotate(-45deg); transform: rotate(-45deg); } 100%, 12% { -webkit-transform: rotate(-405deg); transform: rotate(-405deg); } } @keyframes rotatePlaceholder { 0%, 5% { -webkit-transform: rotate(-45deg); transform: rotate(-45deg); } 100%, 12% { -webkit-transform: rotate(-405deg); transform: rotate(-405deg); } } @-webkit-keyframes animateErrorIcon { 0% { -webkit-transform: rotateX(100deg); transform: rotateX(100deg); opacity: 0; } 100% { -webkit-transform: rotateX(0deg); transform: rotateX(0deg); opacity: 1; } } @keyframes animateErrorIcon { 0% { -webkit-transform: rotateX(100deg); transform: rotateX(100deg); opacity: 0; } 100% { -webkit-transform: rotateX(0deg); transform: rotateX(0deg); opacity: 1; } } @-webkit-keyframes animateXLeft { 0%, 65% { left: 82px; top: 95px; width: 0; } 84% { left: 14px; top: 33px; width: 47px; } 100% { left: 17px; top: 37px; width: 47px; } } @keyframes animateXLeft { 0%, 65% { left: 82px; top: 95px; width: 0; } 84% { left: 14px; top: 33px; width: 47px; } 100% { left: 17px; top: 37px; width: 47px; } } @-webkit-keyframes animateXRight { 0%, 65% { right: 82px; top: 95px; width: 0; } 84% { right: 14px; top: 33px; width: 47px; } 100% { right: 16px; top: 37px; width: 47px; } } @keyframes animateXRight { 0%, 65% { right: 82px; top: 95px; width: 0; } 84% { right: 14px; top: 33px; width: 47px; } 100% { right: 16px; top: 37px; width: 47px; } } @-webkit-keyframes scaleWarning { 0% { -webkit-transform: scale(1); transform: scale(1); } 30% { -webkit-transform: scale(1.02); transform: scale(1.02); } 100% { -webkit-transform: scale(1); transform: scale(1); } } @keyframes scaleWarning { 0% { -webkit-transform: scale(1); transform: scale(1); } 30% { -webkit-transform: scale(1.02); transform: scale(1.02); } 100% { -webkit-transform: scale(1); transform: scale(1); } } @-webkit-keyframes pulseWarning { 0% { background-color: #fff; -webkit-transform: scale(1); transform: scale(1); opacity: 0.5; } 30% { background-color: #fff; -webkit-transform: scale(1); transform: scale(1); opacity: 0.5; } 100% { background-color: #F8BB86; -webkit-transform: scale(2); transform: scale(2); opacity: 0; } } @keyframes pulseWarning { 0% { background-color: #fff; -webkit-transform: scale(1); transform: scale(1); opacity: 0.5; } 30% { background-color: #fff; -webkit-transform: scale(1); transform: scale(1); opacity: 0.5; } 100% { background-color: #F8BB86; -webkit-transform: scale(2); transform: scale(2); opacity: 0; } } @-webkit-keyframes pulseWarningIns { 0% { background-color: #F8D486; } 100% { background-color: #F8BB86; } } @keyframes pulseWarningIns { 0% { background-color: #F8D486; } 100% { background-color: #F8BB86; } } </style>";
  ptr += "<body>";
  if(WiFiMulti.run() == WL_CONNECTED){
    ptr += "<div class='f-modal-alert'> <div class='f-modal-icon f-modal-success animate'> <span class='f-modal-line f-modal-tip animateSuccessTip'></span> <span class='f-modal-line f-modal-long animateSuccessLong'></span> <div class='f-modal-placeholder'></div> <div class='f-modal-fix'></div> </div> </div> <div class='status success'>WiFi Connected</div> <small><a href='logout'>Reset</a></small>";
  } else {
    ptr += "<div class='f-modal-alert'> <div class='f-modal-icon f-modal-error animate'> <span class='f-modal-x-mark'> <span class='f-modal-line f-modal-left animateXLeft'></span> <span class='f-modal-line f-modal-right animateXRight'></span> </span> <div class='f-modal-placeholder'></div> <div class='f-modal-fix'></div> </div> </div> <div class='status error'>Not Connected</div>";
    ptr += "<small><a href='connect'>Connect</a></small>";
  }
  ptr += "<table>";
  ptr += "<table> <tr> <td>PM 2.5</td>";
  ptr += "<td>"+pm2_5+"</td>";
  ptr += "</tr> <tr> <td>PM 0.1</td>";
  ptr += "<td>"+pm0_1+"</td>";
  ptr += "</tr> <tr> <td>PM 10</td>";
  ptr += "<td>"+pm10+"</td>";
  ptr += "</tr><tr><td>CO2</td>";
  ptr += "<td>"+co2+"</td>";
  ptr += "</tr><tr><td>Temperature</td>";
  ptr += "<td>"+temperature+"</td>";
  ptr += "</tr><tr> <td>Humidity</td>";
  ptr += "<td>"+humidity+"</td>";
  ptr += "</tr><tr> <td>WIFI</td>";
  ptr += "<td>"+rssi+"</td>";
  ptr += "</tr></table>";
  ptr += "<br> <form action='/changeServerPassword' method='POST'> <input type='string' placeholder='server password' name='server_password'> <input type='submit' value='Save'> </form></body></html>";
  server.send(200, "text/html", ptr); 
}

void set_password(){
  server.send(200, "text/html", "<!DOCTYPE html> <html> <head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'> <title>EcoBox</title> <meta http-equiv='refresh' content='10; url=/'> </head><style>*{padding: 0; margin: 0;} .lds-roller { display: inline-block; position: absolute; width: 80px; height: 80px;top: 50%;left: 50%;transform: translate(-50%, -50%); } .lds-roller div { animation: lds-roller 1.2s cubic-bezier(0.5, 0, 0.5, 1) infinite; transform-origin: 40px 40px; } .lds-roller div:after { content: " "; display: block; position: absolute; width: 7px; height: 7px; border-radius: 50%; background: #f5ba3b; margin: -4px 0 0 -4px; } .lds-roller div:nth-child(1) { animation-delay: -0.036s; } .lds-roller div:nth-child(1):after { top: 63px; left: 63px; } .lds-roller div:nth-child(2) { animation-delay: -0.072s; } .lds-roller div:nth-child(2):after { top: 68px; left: 56px; } .lds-roller div:nth-child(3) { animation-delay: -0.108s; } .lds-roller div:nth-child(3):after { top: 71px; left: 48px; } .lds-roller div:nth-child(4) { animation-delay: -0.144s; } .lds-roller div:nth-child(4):after { top: 72px; left: 40px; } .lds-roller div:nth-child(5) { animation-delay: -0.18s; } .lds-roller div:nth-child(5):after { top: 71px; left: 32px; } .lds-roller div:nth-child(6) { animation-delay: -0.216s; } .lds-roller div:nth-child(6):after { top: 68px; left: 24px; } .lds-roller div:nth-child(7) { animation-delay: -0.252s; } .lds-roller div:nth-child(7):after { top: 63px; left: 17px; } .lds-roller div:nth-child(8) { animation-delay: -0.288s; } .lds-roller div:nth-child(8):after { top: 56px; left: 12px; } @keyframes lds-roller { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }</style><body><div class='lds-roller'><div></div><div></div><div></div><div></div><div></div><div></div><div></div><div></div></div></body></html>");
  
  EEPROM.begin(EEPROM_SIZE);
  String ssid_ = "";
  String password_ = "";
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
  data["server_password"] = server_password;
  serializeJson(doc, json_string);

  json_string += '\0';

  for(int i = 0; i < json_string.length(); i++){
    EEPROM.write(i,json_string[i]);
  }
  
  EEPROM.commit();
  EEPROM.end();
  ssid     = ssid_;
  password = password_;
  device_id = device_id_;
  
  wifiConnect();
}

void logout(){
  ssid     = "";
  password = "";
  device_id = "0";
  server.send(200, "text/html", "<!DOCTYPE html> <html> <head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'> <title>EcoBox</title> <meta http-equiv='refresh' content='0; url=/'> </head>"); 
  WiFi.disconnect();
}
void changeServerPassword(){
  String pswd = "";
  if (server.hasArg("server_password")){
      pswd = server.arg("server_password")+'\0';
  }

  EEPROM.begin(EEPROM_SIZE);
  for(int i=0; i < 1024; i++){
    EEPROM.write(i,0);
  }
  EEPROM.commit();

  String json_string = "";
  DynamicJsonDocument doc(1024);
  JsonObject data = doc.to<JsonObject>();
  
  data["ssid"] = ssid;
  data["password"] = password;
  data["device_id"] = device_id;
  data["server_password"] = pswd;

  serializeJson(doc, json_string);

  json_string += '\0';

  for(int i = 0; i < json_string.length(); i++){
    EEPROM.write(i,json_string[i]);
  }
  
  EEPROM.commit();
  EEPROM.end();

  WiFi.softAP("Ecobox", pswd);
}
void wifiConnect(){
    if(password != ""){
      WiFiMulti.addAP(ssid.c_str(), password.c_str());
    } else WiFiMulti.addAP(ssid.c_str());
    
    Serial.print("Connecting to ");
    Serial.print(ssid); Serial.println(" ...");
  
    int i = 0;
    while (WiFiMulti.run() != WL_CONNECTED) {
      delay(1000);
      Serial.print(++i); Serial.print(' ');
      if(i >= 10){
        break;
      }
    }
    if(WiFiMulti.run() == WL_CONNECTED){
      Serial.println("Connection established!");  
      Serial.print("IP address:\t");
      Serial.println(WiFi.localIP());
      Serial.print("Devide_id:");
      Serial.println(device_id);
    } else {
      Serial.println("Connection error");
    }
}
