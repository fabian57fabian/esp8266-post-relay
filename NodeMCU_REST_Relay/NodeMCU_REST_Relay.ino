#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define DEBUG 0

#include "secrets.h"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// If needed, set the static IP
bool USE_STATIC_IP = true;
IPAddress staticIP(192, 168, 1, 57);    // Set your desired static IP address
IPAddress gateway(192, 168, 1, 1);      // Set your router's IP address
IPAddress subnet(255, 255, 255, 0);     // Set your subnet mask

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

const char *htmlContent = R"(<html>
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <link rel="icon" href="data:image/x-icon;base64,%s_favicon" type="image/x-icon">
      <style>
        body {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          height: 100vh;
          margin: 0;
        }
        h1, form {
          text-align: center;
        }
        .toggle {
          position: relative;
          display: inline-block;
          width: 60px;
          height: 34px;
        }
        .toggle input {
          opacity: 0;
          width: 0;
          height: 0;
        }
        .slider {
          position: absolute;
          cursor: pointer;
          top: 0;
          left: 0;
          right: 0;
          bottom: 0;
          background-color: #ccc;
          -webkit-transition: .4s;
          transition: .4s;
        }
        .slider:before {
          position: absolute;
          content: '';
          height: 26px;
          width: 26px;
          left: 4px;
          bottom: 4px;
          background-color: white;
          -webkit-transition: .4s;
          transition: .4s;
        }
        .toggle input:checked + .slider {
          background-color: #2196F3;  /* Changed color when checked to blue */
        }
        input:focus + .slider {
          box-shadow: 0 0 1px #ccc;
        }
        input:checked + .slider:before {
          -webkit-transform: translateX(26px);
          -ms-transform: translateX(26px);
          transform: translateX(26px);
        }
      </style>
    </head>
    <body>
      <h1>Albero di Natale</h1>
      <form id='controlForm'>
        <label class='toggle'>
          <input type='checkbox' name='relay' id='relay' onchange='sendFormData()' %s_checked>
          <span class='slider'></span>
        </label>
      </form>
      <script>
        function sendFormData() {
          var form = document.getElementById('controlForm');
          var formData = {
            relay: form.relay.checked
          };

          fetch('/control', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json'
            },
            body: JSON.stringify(formData)
          });
        }
      </script>
    </body>
  </html>
)";

void setup() {
  pinMode(relayPin, OUTPUT);
  setRelay(false);
  Serial.begin(115200);

  // Connect to Wi-Fi with a static IP
  if (USE_STATIC_IP){
    WiFi.config(staticIP, gateway, subnet);
  }
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

  // Define the root endpoint for serving the "hello" page
  server.on("/", HTTP_GET, handleRoot);

  // Define the endpoint for serving the favicon
  server.on("/favicon.ico", HTTP_GET, handleFavicon);

  // Define the endpoint for handling JSON POST control
  server.on("/control", HTTP_POST, handleControl);

  // Define the endpoint for handling JSON GET status
  server.on("/status", HTTP_GET, handleStatus);

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
  String html = String(htmlContent);
  
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

void handleStatus(){
  if (DEBUG) Serial.println("-> GET on /status");
  if (isRelayActive()){
    server.send(200, "application/json", "{\"status\":\"on\"}");
  }else{
    server.send(200, "application/json", "{\"status\":\"off\"}");
  }
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
