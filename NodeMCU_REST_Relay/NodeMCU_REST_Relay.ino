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

  // Define the root endpoint for serving the "hello" page
  server.on("/", HTTP_GET, handleRoot);

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


// Define the HTML content for the root ("/") path
const char *htmlContent = R"(
  <html>
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
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
          <input type='checkbox' name='relay' id='relay' onchange='sendFormData()' %s>
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

void handleRoot() {
  // Handle requests to the root ("/") path
  String html = String(htmlContent);
  
  // Check the current state of the relay and update the checkbox accordingly
  html.replace("%s", isRelayActive() ? "checked" : "");
  
  server.send(200, "text/html", html);
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