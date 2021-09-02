#include <ConfigManager.h>
#include <Stepper.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define LIMIT_COM 27
#define LIMIT_SWITCH 26
#define STEPPER_PIN_1 16
#define STEPPER_PIN_2 17
#define STEPPER_PIN_3 5
#define STEPPER_PIN_4 18
#define LED_PIN 15

#define CYCLE_DELAY 1
#define REQUEST_COUNTDOWN (1000 * 60 * 5) / CYCLE_DELAY
#define stepsPerRevolution 2048
#define CALIBRATED_POSITION 130

struct Config {
    char name[20];
    char password[20];
} config;

int prevStepperPos = 0;
int stepperPos = 0;
int dataCountdown = 0;
int previousBlink = 0;
int blinkDuration = 500;
bool ledOn = false;

Stepper stepper = Stepper(stepsPerRevolution, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4);
ConfigManager configManager;
String serverPath = "http://tides.mamota.net/itchenor";
bool calibrated = false;

void setup() {
  DEBUG_MODE = true;
  pinMode(STEPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_PIN_3, OUTPUT);
  pinMode(STEPPER_PIN_4, OUTPUT);
  pinMode(LIMIT_COM, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LIMIT_SWITCH, INPUT_PULLDOWN);
  
  Serial.begin(115200);
  stepper.setSpeed(10);

  digitalWrite(LIMIT_COM, HIGH);
  digitalWrite(LED_PIN, HIGH);
  
  // put your setup code here, to run once:
  configManager.setAPName("Tide Clock");
  configManager.setAPFilename("/index.html");
  configManager.addParameter("name", config.name, 20);
  configManager.addParameter("password", config.password, 20, set);
  
  configManager.setAPICallback(APICallback);
  
  configManager.begin(config);  
//  configManager.clearWifiSettings(false);
}

bool requested = false;

void loop() {

  calibrateStepper();
  
  configManager.loop();
  if(configManager.wifiConnected()){
    digitalWrite(LED_PIN, LOW);
    if(!calibrated) {
      calibrateStepper();
    }
    
    if(dataCountdown == 0) {
      httpRequest();
      dataCountdown = REQUEST_COUNTDOWN;
    } else {
      dataCountdown--;
    }
  } else {
    // Blink the LED
    int timeNow = millis();
    if ((timeNow - previousBlink > blinkDuration) || (previousBlink > timeNow)) {
      if(ledOn) {
        digitalWrite(LED_PIN, LOW);
      } else {
        digitalWrite(LED_PIN, HIGH);
      }
      ledOn = !ledOn;
      previousBlink = timeNow;
    }
  }
}

void httpRequest() {
  Serial.println("Requesting...");
  HTTPClient http;
  http.begin(serverPath.c_str());
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.print("Payload: ");
    Serial.println(payload);
    stepperPos = payload.toInt();
    updateAngle();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}


void updateAngle() {
  Serial.print("Stepper Pos: ");
  Serial.println(stepperPos);
  Serial.print("Previous Stepper Pos: ");
  Serial.println(prevStepperPos);
  if (stepperPos == prevStepperPos){
    return;
  }else if (stepperPos > prevStepperPos) {
    Serial.print("Stepping forward:");
    Serial.println(stepperPos - prevStepperPos);
    stepper.step(stepperPos - prevStepperPos);
  } else {
    Serial.print("Stepping forward (positive to negative):");
    Serial.println(stepperPos - prevStepperPos);
    stepper.step((stepsPerRevolution - prevStepperPos) + stepperPos);
  }
  prevStepperPos = stepperPos;
}

void calibrateStepper() {
  Serial.println("Calibrate");
  while(digitalRead(LIMIT_SWITCH) == LOW){
//    Serial.print("L");
    stepper.step(1);
  }
  while(true){
    stepper.step(1);
//    Serial.print("H");
    if(digitalRead(LIMIT_SWITCH) == LOW){
//      Serial.println("L");
      break;
    }
  }
//  Serial.println("ZERO");
//  while(true){
//    if(digitalRead(LIMIT_SWITCH) == LOW){
//      Serial.print("L");
//    } else {
//      Serial.println("H");
//    }
//    delay(500);
//  }
  prevStepperPos = CALIBRATED_POSITION;
//  stepperPos = 0;
//  updateAngle();
//  Serial.println("Zero position");
//  delay(100000);
//  stepper.step(stepsPerRevolution);
//  Serial.println("Full rotation");
//  delay(5000);
  calibrated = true;
//  digitalWrite(LED_PIN, LOW);
}

void APICallback(WebServer *server) {
  configManager.stopWebserver();
}
