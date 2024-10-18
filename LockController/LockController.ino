#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Your Wi-Fi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

int lockDoor() {
  Serial.println("Locking the door...");
  return 0; 
}

int unlockDoor() {
  Serial.println("Unlocking the door...");
  return 0; 
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/lock", HTTP_GET, [](AsyncWebServerRequest *request){
    int status = lockDoor();  
    request->send(200, "text/plain", "Door Locked");
  });

  server.on("/unlock", HTTP_GET, [](AsyncWebServerRequest *request){
    int status = unlockDoor(); 
    request->send(200, "text/plain", "Door Unlocked");
  });

  server.begin();
}

void loop() {}
