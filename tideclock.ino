#include <ArduinoBLE.h>
#include <FlashStorage.h>
#include <Stepper.h>
#include <SPI.h>
#include <WiFiNINA.h>

#include "utility/wifi_drv.h"

#define LED_PIN 2
#define INTERRUPT_PIN 10
#define STEPPER_PIN_1 A7
#define STEPPER_PIN_2 A6
#define STEPPER_PIN_3 A5
#define STEPPER_PIN_4 A4
#define HIGH_PIN 11
#define MOVE_SPEED 10

#define CYCLE_DELAY 100 // How long to pause after each loop
#define REQUEST_COUNTDOWN (1000 * 60 * 5) / CYCLE_DELAY // 5 minutes

#define stepsPerRevolution 2048

int wifiStatus = WL_IDLE_STATUS;

char hostName[] = "example.org";

int prevStepperPos = 0;
int stepperPos = 0;
int refreshCountdown = 0;

bool initialised = false;
bool buttonPressed = false;
bool buttonReleased = false;
bool winkState = true;

String dataInputString = "";

typedef struct {
  boolean valid;
  char ssid[20];
  char pass[20];
} WifiCredentials;

BLEService configService("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE WiFi Service
BLEStringCharacteristic ssidCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite, 20);
BLEStringCharacteristic passCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1215", BLERead | BLEWrite, 20);
FlashStorage(flash_store, WifiCredentials);
Stepper stepper = Stepper(stepsPerRevolution, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4);
WiFiSSLClient client;

void setup() {
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), button, CHANGE);
  pinMode(HIGH_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  stepper.setSpeed(MOVE_SPEED);

  ssidCharacteristic.writeValue("SSID");
  passCharacteristic.writeValue("PASSWORD");

  resetConnection();
}

void loop() {
  digitalWrite(11, HIGH);
  if(buttonPressed) {
    stepper.setSpeed(MOVE_SPEED / 2);
    while(!buttonReleased){
      stepper.step(-1 * MOVE_SPEED);
    }
    buttonReleased = false;
    prevStepperPos = 0;
    refreshCountdown = 0;
    stepper.setSpeed(MOVE_SPEED);
  }
  if(refreshCountdown == 0) {
    Serial.println("fetching data");
    httpRequest();
    refreshCountdown = REQUEST_COUNTDOWN;
  } else {
    refreshCountdown--;
  }
  delay(100);
}

void button() {
  Serial.println("press button");
  if(digitalRead(INTERRUPT_PIN) == LOW){
    buttonPressed = true;
    buttonReleased = false;
  } else {
    buttonReleased = true;
    buttonPressed = false;
  }
}

void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();
  dataInputString = "";

  wifiStatus = WiFi.status();

  if (wifiStatus != WL_CONNECTED) {
    resetConnection();
  }

  if (client.connect(hostName, 443)) {
    Serial.println("connecting...");
    // send the HTTP GET request:
    // remember to put the path in!
    client.println("GET / HTTP/1.1");
    client.println("Host: example.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
    }
    Serial.println("get");
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
        Serial.print(c);
        if (isDigit(c)) {
          dataInputString += c;
        }
      }
    }
    stepperPos = dataInputString.toInt();
    updateAngle();
    Serial.println("");
    Serial.print("Value:");
    Serial.println(stepperPos);
  } else {
    Serial.println("connection failed");
  }
}

void updateAngle() {
  if (stepperPos == prevStepperPos){
    return;
  }else if (stepperPos > prevStepperPos) {
    Serial.print("Stepping forward:");
    Serial.println(stepperPos - prevStepperPos);
    stepper.step(-1 * (stepperPos - prevStepperPos));
  } else {
    Serial.print("Stepping forward (back):");
    Serial.println(stepperPos - prevStepperPos);
    stepper.step(-1 * ((4096 - prevStepperPos) + stepperPos));
  }
  prevStepperPos = stepperPos;
}

void enableBLE() {
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("TIDE CLOCK WIFI CONFIG");
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
  while(!BLE.central().connected()) {
    if (winkState) {
      digitalWrite(LED_PIN, LOW);
    } else {
      digitalWrite(LED_PIN, HIGH);
    }
    winkState = !winkState;
    delay(100);
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
  wifiStatus = WL_IDLE_STATUS;

  WifiCredentials creds;

  ssidCharacteristic.value().toCharArray(creds.ssid, 20);
  passCharacteristic.value().toCharArray(creds.pass, 20);
  creds.valid = true;

  flash_store.write(creds);
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
  creds = flash_store.read();

  while ( wifiStatus != WL_CONNECTED && wifiStatus != WL_CONNECT_FAILED ) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.print(creds.ssid);
    Serial.print(", pass (");
    Serial.print(creds.pass);
    Serial.println(")");

    wifiStatus = WiFi.begin(creds.ssid, creds.pass);

    // wait 5 seconds for connection:
    delay(5000);
  }
}

char* string2char(String input){
    if(input.length() != 0){
        char *output = const_cast<char*>(input.c_str());
        return output;
    }
}

void resetConnection() {

  WifiCredentials creds;
  creds = flash_store.read();

  if (creds.valid) {
    connectWifi();
  }

  while (wifiStatus == WL_CONNECT_FAILED || wifiStatus == WL_IDLE_STATUS) {
    // enable bluetooth and wait to try again
    enableBLE();
    connectWifi();
  }

  Serial.println("WiFi Connected");
}
