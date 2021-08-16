#include <ArduinoBLE.h>
#include <FlashStorage.h>
#include <Stepper.h>
#include <SPI.h>
#include <WiFiNINA.h>

#include "utility/wifi_drv.h"

#define LIMIT_COM 5
#define LIMIT_SWITCH 4
#define LED_PIN 2
#define ACTIVITY_PIN 7
#define LOW_PIN 6
#define INTERRUPT_PIN 10
#define STEPPER_PIN_1 A7
#define STEPPER_PIN_2 A6
#define STEPPER_PIN_3 A5
#define STEPPER_PIN_4 A4
#define HIGH_PIN 11

#define CYCLE_DELAY 100
#define REQUEST_COUNTDOWN (1000 * 60 * 5) / CYCLE_DELAY

#define stepsPerRevolution 2048

int status = WL_IDLE_STATUS;  // the WiFi radio's status

char hostName[] = "tides.mamota.net";

int prevStepperPos = 0;
int stepperPos = 0;
int dataCountdown = 0;
int blinkCountdown = 1000;
bool blinkState = true;
long bounceTime = 0;
bool buttonPressed = true;
bool waitingSecondButtonPress = false;
long secondPressTimer = 0;
bool winkState = true;
bool triedDefaultCreds = false;
bool activityLED = true;

String inString = "";

typedef struct {
  boolean valid;
  char ssid[20];
  char pass[20];
} WifiCredentials;

BLEService configService("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE WiFi Service
BLEStringCharacteristic ssidCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite, 20);
BLEStringCharacteristic passCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1215", BLERead | BLEWrite, 20);
FlashStorage(my_flash_store, WifiCredentials);
Stepper stepper = Stepper(stepsPerRevolution, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4);
WiFiClient client;

void setup() {
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), button, CHANGE);
  pinMode(HIGH_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(ACTIVITY_PIN, OUTPUT);
  pinMode(LOW_PIN, OUTPUT);
  pinMode(LIMIT_COM, OUTPUT);
  pinMode(LIMIT_SWITCH, INPUT_PULLUP);
  Serial.begin(9600);
  stepper.setSpeed(10);
  delay(200);
  Serial.println("boot");
  digitalWrite(LOW_PIN, LOW);
  digitalWrite(ACTIVITY_PIN, HIGH);
  digitalWrite(HIGH_PIN, HIGH);
  digitalWrite(LIMIT_COM, LOW);
  
  ssidCharacteristic.writeValue("");
  passCharacteristic.writeValue("");

  resetConnection();
}

void loop() {
  if(waitingSecondButtonPress) {
    if(buttonPressed) {
      buttonPressed = false;
      waitingSecondButtonPress = false;
      dataCountdown = REQUEST_COUNTDOWN;
      Serial.println("Finish calib");
      httpRequest();
      digitalWrite(ACTIVITY_PIN, LOW);
      stepper.setSpeed(10);
    } else {
      if(blinkCountdown-- <= 0){
        blinkState = !blinkState;
        if(blinkState){
          Serial.println("wink");
          digitalWrite(ACTIVITY_PIN, HIGH);  
        } else {
          Serial.println("wonk");
          digitalWrite(ACTIVITY_PIN, LOW);  
        }
        blinkCountdown = 500000;
        long timeWaited = millis() - secondPressTimer;
        if(timeWaited < 0 || timeWaited > 10 * 1000) {
          Serial.println("timed out waiting!");
          buttonPressed = true;
        }
      }
      return;
    }
  }
  if(buttonPressed) {
    Serial.println("calibrate");
    digitalWrite(ACTIVITY_PIN, HIGH);
    while(digitalRead(LIMIT_SWITCH) == LOW){
      stepper.step(1);
    }
    while(true){
      stepper.step(1);
      if(digitalRead(LIMIT_SWITCH) == LOW){
        break;
      }
    }
    prevStepperPos = 0;
    buttonPressed = false;
    waitingSecondButtonPress = true;
    secondPressTimer = millis();
    Serial.println("awaiting second press...");
    return;
  }
  if(dataCountdown == 0) {
    httpRequest();
    dataCountdown = REQUEST_COUNTDOWN;
  } else {
    dataCountdown--;
  }
  delay(100);
}

void button() {
  if((millis() - bounceTime) < 500) {
    Serial.println("bounce");
    return;
  }
  bounceTime = millis();
  Serial.println("press");
  buttonPressed = true;
}

void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();
  inString = "";

  if (status != WL_CONNECTED) {
    resetConnection();
  }
  
  // if there's a successful connection:
  if (client.connect(hostName, 80)) {
    Serial.print("connecting...");
    // send the HTTP GET request:
    client.println("GET /seamills HTTP/1.1");
    client.println("Host: tides.mamota.net");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
    }
    Serial.println("response received!");
    char prevchar1 = 0;
    char prevchar2 = 0;
    char prevchar3 = 0;
    bool inbody = false;
    while (client.available()) {
      char c = client.read();
      if (c == '\n' && prevchar1 == '\r' && prevchar2 == '\n' && prevchar3 == '\r'){
        inbody = true;
      }
      prevchar3 = prevchar2;
      prevchar2 = prevchar1;
      prevchar1 = c;
      if(inbody){
        if (isDigit(c)) {
          inString += c;
        }
      }
    }
    stepperPos = inString.toInt();
    updateAngle();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    status = WiFi.status();
    if(status == WL_CONNECTION_LOST) {
      WiFi.disconnect();
    }
    printStatus();
  }
}

void printStatus() {
  Serial.print("Wifi status: ");
  switch(status){
    case WL_IDLE_STATUS:
      Serial.println("Idle"); break;
    case WL_NO_SSID_AVAIL:
      Serial.println("SSID not found"); break;
    case WL_SCAN_COMPLETED:
      Serial.println("Scan complete"); break;
    case WL_CONNECTED:
      Serial.println("Connected"); break;
    case WL_CONNECT_FAILED:
      Serial.println("Conneciton Failed"); break;
    case WL_CONNECTION_LOST:
      Serial.println("Connection Lost"); break;
    case WL_DISCONNECTED:
      Serial.println("Disconnected"); break;
    default:
      Serial.print("Unknown - ");
      Serial.println(status);
  }
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

void enableBLE() {
  buttonPressed = false;
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("ETA WIFI CONFIG");
  BLE.setAdvertisedService(configService);

  BLEDescriptor ssidDescriptor("2901", "SSID");
  BLEDescriptor passDescriptor("2901", "Password");

  ssidCharacteristic.addDescriptor(ssidDescriptor);
  passCharacteristic.addDescriptor(passDescriptor);

  // add the characteristic to the service
  configService.addCharacteristic(ssidCharacteristic);
  configService.addCharacteristic(passCharacteristic);

  // add service
  BLE.addService(configService);

  // start advertising
  BLE.advertise();
  Serial.println("BLE started advertising");

  // enable winking
  digitalWrite(ACTIVITY_PIN, LOW);
  while(!BLE.central().connected()) {
    if (winkState) {
      digitalWrite(LED_PIN, LOW);
    } else {
      digitalWrite(LED_PIN, HIGH);
    }
    winkState = !winkState;
    delay(100);
    if(buttonPressed) {
      buttonPressed = false;
      BLE.stopAdvertise();
      BLE.end();
      status = WL_IDLE_STATUS;
      digitalWrite(LED_PIN, LOW);

      // Re-initialize the WiFi driver
      // This is currently necessary to switch from BLE to WiFi
      wiFiDrv.wifiDriverDeinit();
      wiFiDrv.wifiDriverInit();
      
      delay(100); // delay to wait for BLE to stop
      return;
    }
  }
  digitalWrite(LED_PIN, HIGH);
  while (BLE.central().connected()) {
    // if the remote device wrote to the characteristic,
    // use the value to control the LED:
    if (ssidCharacteristic.written()) {
      Serial.println(ssidCharacteristic.value());
    }
    if (passCharacteristic.written()) {
      Serial.println(passCharacteristic.value());
    }
  }
  digitalWrite(LED_PIN, LOW);
  Serial.println("BLE disconnected");
  BLE.stopAdvertise();
  BLE.end();
  status = WL_IDLE_STATUS;

  WifiCredentials creds;

  ssidCharacteristic.value().toCharArray(creds.ssid, 20);
  passCharacteristic.value().toCharArray(creds.pass, 20);
  creds.valid = true;
  
  my_flash_store.write(creds);
  Serial.println("Wrote creds to storage");

  // Re-initialize the WiFi driver
  // This is currently necessary to switch from BLE to WiFi
  wiFiDrv.wifiDriverDeinit();
  wiFiDrv.wifiDriverInit();
  
  delay(100); // delay to wait for BLE to stop
}

void connectWifi() {
  Serial.println("Wifi connection");

  WifiCredentials creds;
  creds = my_flash_store.read();

  if (!triedDefaultCreds && !creds.valid) {
    creds = defaultCreds();
    triedDefaultCreds = true;
    Serial.println("Trying default creds");
  }
  
  while ( status != WL_CONNECTED && status != WL_CONNECT_FAILED && status != WL_NO_SSID_AVAIL) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.print(creds.ssid);
    Serial.print(", pass (");
    Serial.print(creds.pass);
    Serial.println(")");
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(creds.ssid, creds.pass);

    // wait 5 seconds for connection:
    printStatus();
    delay(5000);
  }
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void resetConnection() {

  WifiCredentials creds;
  creds = my_flash_store.read();

  if (creds.valid || !triedDefaultCreds) {
    connectWifi();
    
  }

  while (status == WL_CONNECT_FAILED || status == WL_IDLE_STATUS || status == WL_NO_SSID_AVAIL) {
    // enable bluetooth and wait to try again
    enableBLE();
    connectWifi();
  }
  triedDefaultCreds = false;
  Serial.println("WiFi Connected");
}

WifiCredentials defaultCreds () {
  WifiCredentials creds;
  strcpy(creds.ssid, "Kimbonet 2G");
  strcpy(creds.pass, "shdbzqwa");
  creds.valid = true;
  return creds;
}
