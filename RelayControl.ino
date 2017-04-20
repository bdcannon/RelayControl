#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "";
const char* password = "";
ESP8266WebServer server(80);

const int led = 13;
#define RELAY_0 D0
#define RELAY_1 D1

#define RELAY_NUM_KEY "relaynum"
#define MISSING_RELAY_ARG "Relay number argument missing from request"

bool relay0on = false;
bool relay1on = false;

void handleRoot() {
  digitalWrite(led, 1);
  String message;
  
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(200, "text/plain", message);
  digitalWrite(led, 0);
}

// Returns the index of the request parameter in the server args or -1 if not found
int findRequestParam(String requestKey){
  int relayArgIndex = -1;
  
  for(int i = 0; i < server.args(); i++){
    if(server.argName(i) == requestKey){
      relayArgIndex = i;
      break;
    }
   }
    return relayArgIndex;
}

void setRelay(int relayNum, bool state){
  // Turn on the relay
    switch(relayNum){
      case 0 : digitalWrite(RELAY_0, state);
               relay0on = state;
               break;
      case 1 : digitalWrite(RELAY_1, state);
               relay1on = state;
               break;
      default : break;
    }
}

void handleNotFound(){
  digitalWrite(led, 1);
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
  digitalWrite(led, 0);
}

void setup(void){
  Serial.begin(115200);
  
  // Set up pins
  pinMode(led, OUTPUT);
  pinMode(RELAY_0, OUTPUT);
  pinMode(RELAY_1, OUTPUT);

  // Start all the pins off
  digitalWrite(RELAY_0, LOW);
  digitalWrite(RELAY_1, LOW);  
  
  digitalWrite(led, 0);
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

  if (MDNS.begin("relaycontrol")) {
    Serial.println("MDNS responder started");
  }

  // Going to serve a basic control page
  server.on("/", handleRoot);
  
  ////////////////////////////////////////
  // Get the status of the relay
  server.on("/relaystatus", [](){
    int relayArgIndex = -1;
    int relayNum = -1;

    relayArgIndex = findRequestParam(RELAY_NUM_KEY);
    
    // If we found the arg in the list of arguments
    if(relayArgIndex < 0){
      server.send(400, "text/plain", MISSING_RELAY_ARG);
      return;
    }
    
    // Check we got a relay number
    relayNum = server.arg(relayArgIndex).toInt();
    if(relayNum < 0){
      server.send(400, "text/plain", MISSING_RELAY_ARG);
      return;
    }

    // Make string out of the relay status
    String relayStatus = "";
    switch(relayNum){
      case 0 : relay0on ? relayStatus+= "1\n" : relayStatus+="0\n"; break;
      case 1 : relay1on ? relayStatus+= "1\n" : relayStatus+="0\n"; break;
      default : relayStatus += "There was a problem\n"; break;
    }
    
    server.send(200, "text/plain", String(relayNum) + ":" + relayStatus);
  });
  
  ////////////////////////////////////////
  // Turning on a relay
  server.on("/relayon", [](){
    int relayArgIndex = -1;
    int relayNum = -1;

    // Find relay arg index
    relayArgIndex = findRequestParam(RELAY_NUM_KEY);

    // Check that we got a relay arg
    if(relayArgIndex < 0){
      server.send(400, "text/plain", MISSING_RELAY_ARG);
      return;
    }

    // Check we got a relay number
    relayNum = server.arg(relayArgIndex).toInt();
    if(relayNum < 0){
      server.send(400, "text/plain", MISSING_RELAY_ARG);
      return;
    }

    // Turn on the relay
    setRelay(relayNum, HIGH);
    server.send(200, "text/plain", server.arg(relayArgIndex) + ":1\n");
  });
  ////////////////////////////////////////
  // Turns the relay off
  server.on("/relayoff", [](){
    int relayArgIndex = -1;
    int relayNum = -1;

    relayArgIndex = findRequestParam(RELAY_NUM_KEY);

    // Check that we got a relay arg
    if(relayArgIndex < 0){
      server.send(400, "text/plain", MISSING_RELAY_ARG);
      return;
    }

    // Check we got a relay number
    relayNum = server.arg(relayArgIndex).toInt();
    if(relayNum < 0){
      server.send(400, "text/plain", MISSING_RELAY_ARG);
      return;
    }

    setRelay(relayNum, LOW);
    
    server.send(200, "text/plain", server.arg(relayArgIndex) + ":0\n");
  });

  server.onNotFound(handleNotFound);

  // Server is now started
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
