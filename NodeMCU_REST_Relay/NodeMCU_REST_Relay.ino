#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define DEBUG 0

#include "secrets.h"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;
/*
IPAddress staticIP(192, 168, 1, 199);  // Set your desired static IP address
IPAddress gateway(192, 168, 1, 1);      // Set your router's IP address
IPAddress subnet(255, 255, 255, 0);     // Set your subnet mask
*/

const int relayPin = 14;  // Define the pin connected to the relay, GPIO14 = D5

ESP8266WebServer server(80);

void setRelay(bool active){
  digitalWrite(relayPin, active ? LOW : HIGH); // since it is NC, it is reversed
}

bool isRelayActive(){
  return digitalRead(relayPin) == 0;
}

// Base64-encoded favicon image data
const char *faviconBase64 = "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAA2FBMVEVHcEwlpVslp1wRTyvhTj3nTDwnrmAnrmAkolknrmAOQSQKLxolqFwmq14NPCEQSSgjnVYLNR0Ybz1XlVY9HRRak1Z9KSBCIBYadEDLQzTORDWEKyIlpVshk1Elp1whllMdhEkmrF8UWzInrmAYbj0mrV8eiEsbe0QimFMnrmAnrmAJKxcfjk4lplseikwVYjYdg0gnrmAKLxonrmAadEAnrmAnrmAnrmAjnFYnrmAmrV8imVUmql4mrF8hlFIjnVYkpFsmq14imFQjnlcmql0kpVshlVIhl1Pwn0+zAAAAOXRSTlMAosQX4dv8+6LeDgfBxw4Xygg8owejMwc8T08zpHbsdl3+K/RVxGFIl/HhCGrzdyxN5Qn9WvLI9p1SZGE4AAAAi0lEQVQY01WLVxaCQBAEVwVWFHOOmHNOOEMw6/1vJMqKTP1VvW7GPIo1RqjIcpUWSaKeL+RKJGQ5Lwc9qQKosUCIgkvo7ynNQryEI36IJ84n00xn6AOg/vMGCHQRWs27jWhb84EIs/bjdn06zmvh+ZB/94igrD7eGYPPfumGaW/Un3SVrbE+HHcb9gZRvRQKqdxDQwAAAABJRU5ErkJggg==";

String htmlContent;  // Variable to store the HTML content

void setup() {
  pinMode(relayPin, OUTPUT);
  setRelay(false);
  Serial.begin(115200);

  // Connect to Wi-Fi with a static IP
  //WiFi.config(staticIP, gateway, subnet);
  Serial.println("Connecting to WiFi...");
  connectToWiFi();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(" .");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Load HTML content from the file
  loadHtmlContent();

  // Define the root endpoint for serving the "hello" page
  server.on("/", HTTP_GET, handleRoot);

  // Define the endpoint for serving the favicon
  server.on("/favicon.ico", HTTP_GET, handleFavicon);

  // Define the endpoint for handling JSON POST requests
  server.on("/control", HTTP_POST, handleControl);

  // Start the server
  server.begin();
}

void loop() {
  // Handle incoming client requests
  server.handleClient();

  // Check WiFi connection and reconnect if necessary
  if (WiFi.status() != WL_CONNECTED) {
    if (DEBUG) Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
}

void loadHtmlContent() {
  // Load HTML content from the file
  if (SPIFFS.begin()) {
    File file = SPIFFS.open("/home.html", "r");
    if (file) {
      while (file.available()) {
        htmlContent += (char)file.read();
      }
      file.close();
    } else {
      Serial.println("Failed to open content.html");
    }
    SPIFFS.end();
  } else {
    Serial.println("Failed to mount file system");
  }
}

void connectToWiFi() {
  // Attempt to connect to WiFi
  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    if (DEBUG) Serial.print(" .");
    attempts++;
  }
  if (DEBUG) Serial.println("");
}

void handleRoot() {
  // Handle requests to the root ("/") path
  String html = htmlContent;
  
  // Check the current state of the relay and update the checkbox accordingly
  html.replace("%s_checked", isRelayActive() ? "checked" : "");

  // Replace %s in the favicon link with the actual base64-encoded favicon data
  html.replace("%s_favicon", faviconBase64);
  
  server.send(200, "text/html", html);
}

void handleFavicon() {
  // Serve the embedded favicon
  server.send(200, "image/x-icon", faviconBase64);
}

void handleControl() {
  if (DEBUG) Serial.println("-> POST on /control");
  // Handle JSON POST request to /control

  // Check if the request has a JSON payload
  if (server.hasArg("plain")) {
    // Parse JSON payload
    String jsonPayload = server.arg("plain");
    if (DEBUG) Serial.println("Received JSON: " + jsonPayload);

    // Parse JSON and extract the 'relay' state
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonPayload);

    if (error) {
      if (DEBUG) Serial.println("Failed to parse JSON");
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to parse JSON\"}");
      return;
    }

    // Check if the JSON contains the 'relay' key
    if (doc.containsKey("relay")) {
      // Get the relay state (true or false)
      bool relayState = doc["relay"];

      // Set the relay state
      setRelay(relayState);

      // Respond with success
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Relay state changed\"}");
    } else {
      // Respond with an error if the 'relay' key is missing
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"'relay' key missing in JSON\"}");
    }
  } else {
    // Respond with an error if there is no JSON payload
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No JSON payload in the request\"}");
  }
}
