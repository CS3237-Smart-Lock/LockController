#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>

// Your Wi-Fi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

#define SWITCH 4
byte locked = LOW;
int count = 0;

Servo myservo;

int pos = 0;
int servoPin = 18;

int lockDoor() {
  Serial.println("Locking the door...");
  return 0; 
}

int unlockDoor() {
  Serial.println("Unlocking the door...");
  state = HIGH;
  return 0; 
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myservo.setPeriodHertz(50);
  myservo.attach(servoPin, 1000, 2000);

  pinMode(SWITCH, INPUT_PULLUP);

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

void loop() {
  if (digitalRead(SWITCH) == LOW) {
    state = LOW;
    Serial.println("Locked");
    myservo.write(0);
    delay(100);
  }
  if (state) {
    Serial.println("Unlock");
    myservo.write(180);
  }
}
