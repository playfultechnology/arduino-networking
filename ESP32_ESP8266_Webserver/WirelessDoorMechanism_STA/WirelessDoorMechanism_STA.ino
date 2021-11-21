/**
 * Wireless door controller, which allows a relay module
 * to be driven HIGH or LOW by clients making HTTP requests
 * to /H or /L URLs. 
 * This version uses STA mode to join an existing Wi-Fi 
 * network
*/
// INCLUDES
#include <ESP8266WiFi.h> // For network connection
#include <ESP8266WebServer.h> // For acting as a server to deliver webpages to clients

// CONSTANTS
// The SSID of the Wi-Fi network to join
const char ssid[] = "ENTER YOUR SSID HERE";
// Password required to join network
const char password[] = "ENTER PASSWORD HERE";
// Signal pin that will drive the relay
const int relayPin = D1;
// After what time will door automatically re-lock
const unsigned long relockDelay = 5000;

// GLOBALS
ESP8266WebServer server(80);
// Timestamp at which door was last unlocked
unsigned long lastUnlockTime;
enum State { Locked, Unlocked };
State state = State::Locked;

// CALLBACKS
void displayPage() {
  server.send(200, "text/html", "<h1>Relay Controller</h1>Relay <a href=\"/H\">HIGH</a><br>Relay <a href=\"/L\">LOW</a>");
}
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  // Start serial connection for debugging
  Serial.begin(115200);
  Serial.println(__FILE__ __DATE__);

  // Initialise relay  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Initialise the network
  WiFi.mode(WIFI_STA);  
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  // Print assigned IP address
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());

  // Define the different resources which the user can request
  server.on("/", []() {
    displayPage();
  });
  server.on("/H", [](){
    Serial.println(F("Unlocking"));
    digitalWrite(relayPin, HIGH);
    lastUnlockTime = millis();
    state = State::Unlocked;
    displayPage();
  });
  server.on("/L", [](){
    Serial.println(F("Locking"));
    digitalWrite(relayPin, LOW);
    state = State::Locked;
    displayPage();
  });
  server.onNotFound(handleNotFound);
 
  // Start the server
  server.begin();
  Serial.println(F("HTTP server started"));
}

void loop() {
  server.handleClient();
  if(state == State::Unlocked && millis() - lastUnlockTime > relockDelay) {
    Serial.println(F("Auto Re-locking"));
    digitalWrite(relayPin, LOW);
    state = State::Locked;
  }
}
