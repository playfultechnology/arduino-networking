/*
 * UIPEthernet TCPServer example.
 *
 * For use with a enc28j60 based Ethernet-shield.
 */

// INCLUDES
#include <UIPEthernet.h>
#include "Ringbuffer.h"

// CONSTANTS
const byte mac[6] = {0xba,0xba,0xba,0xba,0xba,0xba};
const IPAddress ip(192,168,1,40);
const uint16_t port = 8081;

// GLOBALS
// The type of HTTP request received (GET/POST)
enum Method { Undefined, GET, POST };
Method method = Method::Undefined;
// Keep track of the part of the request being processed
enum MessagePart { Header, Body, PostContent };
MessagePart messagePart = MessagePart::Header;
// Use a ring buffer to store the HTTP request
RingBuffer buf(16);
// The server object, and the port on which to start the server listening
EthernetServer server = EthernetServer(port);
int reqCount = 0;                // number of requests received

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);  
  // Connect to the network
  // Note that you can leave IP blank to have it assigned by DHCP, but that makes Arduino sketch occupy more memory
  Ethernet.begin(mac, ip);
  Serial.print("LAN IP: ");
  Serial.println(Ethernet.localIP());
  // Start the server
  server.begin();
}

void loop() {
  EthernetClient client = server.available();
  if(client){
    Serial.println("New client connected");
    buf.init();
    // Initialise the variables we know about this connection
    method = Method::Undefined;
    messagePart = MessagePart::Header;
    int messageLength;
    // https://stackoverflow.com/questions/14944773/receiving-a-http-post-request-on-arduino
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while(client.connected()) {
      // There's some data to process
      if (client.available()) {
        // Read the next character
        char c = client.read();
        // Write it to the serial monitor for debugging
        Serial.write(c);
        // Add it on to the ring buffer
        buf.push(c);

        switch(method) {
          case Method::Undefined :
            if (buf.endsWith("GET /")) {
              method = Method::GET;
            }
            else if(buf.endsWith("POST /")) {
              method = Method::POST;
            }
            break;
          case Method::GET:
            // Check to see if the client request was "GET /H" or "GET /L":
            if (buf.endsWith("GET /H")) {
              Serial.println("Turn led ON");
              digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
            }
            else if (buf.endsWith("GET /L")) {
              Serial.println("Turn led OFF");
              digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
            }
            // you got two newline characters in a row
            // that's the end of the HTTP request, so send a response
            else if (buf.endsWith("\r\n\r\n")) {
              Serial.println("Sending response to GET request");
              sendResponse(client);
            }
            break;
          case Method::POST:
            // Scan the ring buffer to find out how long the message body is
            if(messagePart==MessagePart::Header && buf.endsWith("Content-Length: ")) {
              // Retrieve the next characters from the request to get the value of the content length
              char contentLength[8];
              for(int c=0; c<7; c++) {
                char n = client.read();
                buf.push(n);
                if(n=='\r') { break; }
                contentLength[c] = n;
                contentLength[c+1] = 0;
              }
              // Convert to an int
              messageLength = atoi(contentLength);
              Serial.print("messageLength:");
              Serial.println(messageLength);
            }
            // If we've got to the end of the header
            else if (messagePart == MessagePart::Header && buf.endsWith("\r\n\r\n")) {
              messagePart = MessagePart::Body;
            }
            // If we're in the message body itself
            else if(messagePart == MessagePart::Body) {
              // Read the known number of characters in the message body
              for(int c=0; c<messageLength; c++) {
                char n = client.read();
                Serial.print(n);
                buf.push(n);
              }
              Serial.println("Sending response to POST request");
              sendResponse(client);
              break;      
            }
            break;
        }
      }
    }
    client.stop();
  }
}

void sendResponse(EthernetClient client) {
  // Send a standard http response header
  // use \r\n instead of many println statements to speedup data send
  client.print(F("HTTP/1.1 200 OK\r\n"));
  client.print(F("Content-type:text/html\r\n"));
  client.print(F("Connection: close\r\n"));
  client.print(F("\r\n"));
  client.print(F("<!DOCTYPE HTML>\r\n"));
  client.print(F("<html>"));
  client.print(F("<h1>Hello World!</h1>"));
  client.print(F("Requests received: "));
  client.print(++reqCount);
  client.print(F("<br>"));
  client.print(F("Analog input A0: "));
  client.print(analogRead(A0));
  client.print(F("<br>"));
  //form added to send data from browser and view received data in serial monitor         
  client.println(F("<form action='\' method='POST'>"));
  client.println(F("Value: <input type='text' name='Name' value='' size='4' maxlength='4'><br>"));
  client.println(F("<input type='submit'>"));
  client.println(F("</form>"));
  client.println(F("Click <a href=\"/H\">here</a> turn the LED on<br>"));
  client.println(F("Click <a href=\"/L\">here</a> turn the LED off<br>"));
  client.print(F("</html>"));
}
