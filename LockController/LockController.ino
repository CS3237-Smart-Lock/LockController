#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <ESP32Servo.h>

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
// Possible PWM GPIO pins on the ESP32-S2: 0(used by on-board button),1-17,18(used by on-board LED),19-21,26,33-42
// Possible PWM GPIO pins on the ESP32-S3: 0(used by on-board button),1-21,35-45,47,48(used by on-board LED)
// Possible PWM GPIO pins on the ESP32-C3: 0(used by on-board button),1-7,8(used by on-board LED),9-10,18-21
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
int servoPin = 17;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
int servoPin = 7;
#else
int servoPin = 18;
#endif

// Your Wi-Fi credentials
const char* ssid = "";
const char* password = "";

WebServer server(80);

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void turnServo(int pos){
  myservo.write(pos);      // tell servo to go to position in variable 'pos'
  delay(15);             // waits 15ms for the servo to reach the position
}

#define SWITCH 4
byte locked = LOW;
int count = 0;

Servo myservo;

int pos = 0;
int servoPin = 18;

int lockDoor() {
  Serial.println("Locking the door...");
  turnServo(180);
  return 0; 
}

int unlockDoor() {
  Serial.println("Unlocking the door...");
  state = HIGH;
  turnServo(0);
  return 0; 
}

void setupServo(){
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object
	// using default min/max of 1000us and 2000us
	// different servos may require different min/max settings
	// for an accurate 0 to 180 sweep
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
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

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
}

void startServer(){
  server.onNotFound(handleNotFound);

  server.on("/lock", HTTP_GET, [](){
    Serial.println("Lock request received");
    int status = lockDoor();  
    server.send(200, "text/plain", "Door Locked");
  });

  server.on("/unlock", HTTP_GET, [](){
    Serial.println("Unlock request received");
    int status = unlockDoor(); 
    server.send(200, "text/plain", "Door Unlocked");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void setup() {
  Serial.begin(115200);

  setupServo();
  connectToWifi();
  startServer();

  pinMode(SWITCH, INPUT_PULLUP);
}

void loop() {
  server.handleClient();
  delay(2);  

  if (digitalRead(SWITCH) == LOW) {
    state = LOW;
    lockDoor() 
  }
  if (state) {
    Serial.println("Unlock");
    unlockDoor() 
  }
}
