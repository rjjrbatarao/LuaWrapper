/*
This example shows how to expose arduino class to lua classes neatly
Please refer to LuaBridge documentations
*/

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <LuaWrapper.h>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
using namespace luabridge;
LuaWrapper lua;


const char* ssid = "wifi";
const char* password = "";
unsigned long command_timer = 0;
String script = "";
String script_tmp = "";

class coreHttpClient {
  public:
  coreHttpClient(){
    _http = new HTTPClient();
  }
  void begin(const char* url){
    _http->begin(url);
  }
  void end(){
    _http->end();
  }  
  void addHeader(const char* name, const char* value){
    _http->addHeader(name, value);
  }
  int POST(const char* url){
    return _http->POST(url);
  }
  int GET(){
    return _http->GET();
  }
  std::string getString(){
    return _http->getString().c_str();
  }
  bool connected(){
    return _http->connected();
  }
  private:
   HTTPClient* _http = NULL;
};

void HttpClientClass(){
  getGlobalNamespace(lua.lua_state)
  .beginClass<coreHttpClient>("httpClient")
  .addConstructor <void (*) (void)> ()
  .addFunction("begin", &coreHttpClient::begin)
  .addFunction("endRequest", &coreHttpClient::end)
  .addFunction("addHeader", &coreHttpClient::addHeader)
  .addFunction("POST", &coreHttpClient::POST)
  .addFunction("GET", &coreHttpClient::GET)
  .addFunction("getString", &coreHttpClient::getString)
  .addFunction("connected", &coreHttpClient::connected)
  .endClass();  
}

void setup(){
	Serial.begin(9600);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	
	HttpClientClass();
}

void loop(){
  script_tmp = "";
  while (Serial.available() > 0) {
    command_timer = millis();
    char c = Serial.read();
    script += c;
    script_tmp += c;
  }
  if (script_tmp.length() > 0) {
    Serial.print(">>");
    Serial.println(script_tmp);
  }
  if (script.length() > 0) {
    if ((millis() - command_timer) >= 500) {
      Serial.println();
      Serial.println("Executing Lua");
      Serial.println(lua.Lua_dostring(&script));
      lua_gc(lua.lua_state, LUA_GCCOLLECT, 0);
      script = "";
    }
  }
}

