/*
 * ESP32/ESP8266 WebServer
 * 
 * Demonstrates how to create functionality that can be controlled over the internet
 * via an exposed web interface
 */

// INCLUDES
// Load the appropriate libraries depending on platform
#if defined(ESP32)
  #include <WiFi.h> // For network connection
  #include <WebServer.h> // For acting as a server to deliver webpages to clients
  #include <HTTPClient.h> // For acting as client to retrieve webpages from the internet
#elif defined(ESP8266)
  #include <ESP8266WiFi.h> // For network connection
  #include <ESP8266WebServer.h> // For acting as a server to deliver webpages to clients
  #include <ESP8266HTTPClient.h> // For acting as client to retrieve webpages from the internet
#endif
#include <WiFiClient.h>

// CONSTANTS
// The SSID of the Wi-Fi network to join
const char ssid[] = "VodafoneConnect53686628";
// Password required to join network
const char password[] = "8p2ty6329x2mk6v";
// Port on which server will listen to requests
const uint16_t port = 2002;
const byte relayPin = 7;

// GLOBALS
#if defined(ESP32)
  // The webserver object
  WebServer server(port);
#elif defined(ESP8266)
  ESP8266WebServer server(port);
#endif

// CALLBACKS
void displayPage() {
  server.send(200, "text/html", "<h1>Escape Room Controller</h1><form action='/' method='GET'>Password: <input type='text' name='p' value='' size='4' maxlength='4'><input type='submit'></form>Click <a href=\"/H\">here</a> turn the LED on<br>Click <a href=\"/L\">here</a> turn the LED off");
}
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  // For debugging purposes
  Serial.begin(115200);

  // Initialise the network  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print config to serial monitor
  Serial.print(F("SSID: "));
  Serial.println(ssid);
  Serial.print(F("LAN IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("MAC: "));
  Serial.println(WiFi.macAddress());

  // Retrieve our WAN address from the ident.me service
  HTTPClient http;
  http.begin("http://ident.me/"); 
  int httpCode = http.GET();
  if(httpCode > 0) {
    String response = http.getString();
    Serial.print(F("WAN IP: "));
    Serial.println(response);    
  }
  // Free up resources  
  http.end();

  // Define the different resources which the user can request
  server.on("/", []() {
    if(server.hasArg("p") && server.arg("p") == "1234") {
      digitalWrite(relayPin, HIGH);
      delay(500);
      digitalWrite(relayPin, LOW);
      Serial.println((" - CORRECT PASSWORD ENTERED!"));
    }
    displayPage();
  });
  server.on("/H", [](){
    Serial.println(F(" - Activating relay"));
    digitalWrite(relayPin, HIGH);
    displayPage();
  });
  server.on("/L", [](){
    Serial.println(F(" - Activating relay"));
    digitalWrite(relayPin, LOW);
    displayPage();
  });
  server.onNotFound(handleNotFound);
  // Start the server
  server.begin();
  Serial.println(F("HTTP server started"));

  delay(2000);
}

void loop() {
  server.handleClient();
}
