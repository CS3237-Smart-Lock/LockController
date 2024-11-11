#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include <ESP32Servo.h>
#define LOCKED 1
#define UNLOCKED 0

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

#define OUTSIDE_PIN 13
#define INSIDE_PIN 14
//#define LED_LOCK 5
//#define LED_UNLOCK 23

int count = 0;
byte state = LOCKED;
volatile bool pressed_outside = false;
volatile bool pressed_inside = false;

// Your Wi-Fi credentials
const char* ssid = "iPhone von Luis";
const char* password = "12345678";

WebServer server(80);

hd44780_I2Cexp lcd;  // Declare lcd object: auto-locates address

void setup_lcd() {
  
  int status = lcd.begin(16, 2);  // Adjust to 20,4 if using a 20x4 display
  delay(500);
  if (status) {                   // Check if initialization was successful
    Serial.println("LCD initialization failed");
    return;
  }
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.cursor();    // Display the cursor
  lcd.blink();     // Set cursor to blink
  lcd.print("Hello, ESP32!");
}

void IRAM_ATTR isr_out() {
  pressed_outside = true;
  
}

void IRAM_ATTR isr_in() {
  pressed_inside = true;
  
}

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
  delay(15);               // waits 15ms for the servo to reach the position
}

int lockDoor() {
  Serial.println("Locking the door...");
  turnServo(180);
  state = LOCKED;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LOCKED...");
  //digitalWrite(LED_UNLOCK, LOW);
  //digitalWrite(LED_LOCK, HIGH);
  pressed_outside = false;
  pressed_inside = false;
  return 0; 
}

int unlockDoor() {
  Serial.println("Unlocking the door...");
  turnServo(0);
  state = UNLOCKED;
  //digitalWrite(LED_LOCK, LOW);
  //digitalWrite(LED_UNLOCK, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UNLOCKED!!!");
  pressed_outside = false;
  pressed_inside = false;
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

esp_err_t init_wifi(const char* hostname = "esp32", int maxRetries = 20) {
  WiFi.begin(ssid, password);
  Serial.println("Starting WiFi");

  int retryCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retryCount++;

    if (retryCount >= maxRetries) {
      Serial.println("\nWiFi connection failed. Restarting ESP...");
      return ESP_FAIL;
    }
  }

  Serial.println("\nWiFi connected");

  // Start mDNS
  if (!MDNS.begin(hostname)) {
    Serial.println("Error starting mDNS");
    return ESP_FAIL;
  }
  Serial.print("mDNS responder started with hostname: ");
  Serial.println(hostname);

  return ESP_OK;
}

void startServer(){
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, [](){
    server.send(200, "text/plain", "Hello World!");
  });

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
  delay(1000);

  setupServo();
  delay(1000);

  if (init_wifi("lockController") == ESP_FAIL){
    ESP.restart();
  };

  delay(1000);
  startServer();

  setup_lcd();


  //setup the LEDS
  //pinMode(LED_LOCK, OUTPUT);
  //pinMode(LED_UNLOCK, OUTPUT);

  //set up the outside pin
  pinMode(OUTSIDE_PIN, INPUT_PULLUP);
  attachInterrupt(OUTSIDE_PIN, isr_out, FALLING);

  //set up the inside pin
  pinMode(INSIDE_PIN, INPUT_PULLUP);
  attachInterrupt(INSIDE_PIN, isr_in, FALLING);


  //digitalWrite(LED_UNLOCK, LOW);
  //digitalWrite(LED_LOCK, HIGH);
}

void loop() {
  server.handleClient();
  delay(2);

  if(pressed_outside) {
    delay(250);
    lockDoor();
  } else if(pressed_inside) {
    delay(250);
    if(state==LOCKED) {
      unlockDoor();
    } else {
      lockDoor();
    }
  }
}