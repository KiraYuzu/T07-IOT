#include <BLEDevice.h>
#include "Wire.h"
#include "Adafruit_TCS34725.h"
#include <Arduino.h>  // Include the Arduino core library for access to Serial1
#include <M5StickCPlus.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>



// Unique service UUID for our BLE network
#define SERVICE_UUID "0000XXXX-0000-1000-8000-00805F9B34FB"
#define SERIAL_TX 25
#define SERIAL_RX 26

#define SCL_PIN 32
#define SDA_PIN 33

// Counter to store the number of devices seen

struct DataToSend {
  uint16_t deviceCount;
  uint16_t batteryLevel;
  uint16_t ledState;
  uint16_t brightness;
  uint16_t id;
};
int deviceCount;

DataToSend data;
const char *ssid = "testntwork";
const char *password = "password123";
const char *serverName = "http://192.168.109.46:3001/update";

String receivedString = "";

const uint standardizedBaudRate = 9600;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

void doBLEScan(void *);
void colorSensing(void *);
void uartSend(void *);
void batteryRead(void *);
void ledTask(void *);

// Callback to handle BLE advertising packets
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    deviceCount++;
  }
};

void postDataToServer() {
  Serial.println("Posting JSON data to server...");
  // if (wifiMulti.run() == WL_CONNECTED) {
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<20> doc;
  doc["deviceCount"] = data.deviceCount;
  doc["batteryLevel"] = data.batteryLevel;
  doc["ledState"] = data.ledState;
  doc["brightness"] = data.brightness;
  doc["id"] = data.id;

  String requestBody;
  serializeJson(doc, requestBody);


  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.printf("Error occurred while sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  // }
}

void postStringToServer() {
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<20> doc;
  doc["data"] = receivedString;
  String requestBody;
  serializeJson(doc, requestBody);

  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.printf("Error occurred while sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
  }
}

void wifiSetup() {
  WiFi.begin(ssid, password);
  WiFi.setHostname("Node ");
}

// Function to perform BLE scanning
void doBLEScan(void *parameter) {
  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);

  while (true) {
    Serial.printf("Nearby Bluetooth Devices found: %d\n", deviceCount);
    data.deviceCount = deviceCount;
    deviceCount = 0;                   // Reset device count
    pBLEScan->start(5);                // Scan for 5 seconds
    vTaskDelay(pdMS_TO_TICKS(10000));  // Wait for 10 seconds before next scan
  }
}

void colorSensing(void *parameter) {
  Wire.begin(SDA_PIN, SCL_PIN);  // Initialize I2C with specified SDA and SCL pins

  if (tcs.begin()) {
    Serial.println(F("Found sensor"));
  } else {
    Serial.println(F("No TCS34725 found ... check your connections"));
    while (1)
      ;  // halt!
  }

  while (true) {
    uint16_t clear, red, green, blue;
    tcs.getRawData(&red, &green, &blue, &clear);

    // Calculate color temperature (in Kelvin)
    float temperature = tcs.calculateColorTemperature(red, green, blue);

    // Calculate light intensity (in Lux)
    float lux = tcs.calculateLux(red, green, blue);
    data.brightness = static_cast<uint16_t>(lux);
    Serial.print("Color Temp: ");
    Serial.print(temperature, 2);
    Serial.print(" K ");
    Serial.print("Lux: ");
    Serial.println(lux, 2);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void uartSend(void *parameter) {
  static int counter = 0;
  while (true) {
    Serial.println("Sending over UART");
    Serial.println(
      serializeDataToSend()
    );
    Serial1.write(serializeDataToSend().c_str());
    // postDataToServer();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void uartRead(void *parameter) {
  while (true) {
    if (Serial1.available() > 0) {
      // Directly get char* from Serial1

      receivedString = Serial1.readStringUntil('\n');
      Serial.print(receivedString);
      Serial.println();  // Optionally, print a newline character after sending
      postStringToServer();
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void batteryRead(void *parameter) {
  while (true) {
    float voltage = M5.Axp.GetBatVoltage();
    int batteryLevel = map(voltage, 3.0, 4.2, 0, 100);
    data.batteryLevel = batteryLevel;
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void ledTask(void *parameter) {

  while (true) {
    float luxRange = static_cast<double>(data.brightness) / 300;
    int brightness = 800 - 800 * luxRange;
    M5.Lcd.fillScreen(BLACK);
    if (1 - luxRange > 0.66) {
      M5.Lcd.drawRect(0, 0, 135, 240, WHITE);  // BRIGHTEST
      M5.Lcd.fillRect(0, 0, 135, 240, WHITE);
    } else if (1 - luxRange > 0.33) {
      M5.Lcd.drawRect(16, 32, 101, 180, WHITE);
      M5.Lcd.fillRect(16, 32, 101, 180, WHITE);
    } else {
      M5.Lcd.drawRect(32, 64, 67, 120, WHITE);
      M5.Lcd.fillRect(32, 64, 67, 120, WHITE);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

String serializeDataToSend() {
  static String jsonString;
  StaticJsonDocument<20> doc;
  doc["deviceCount"] = data.deviceCount;
  doc["batteryLevel"] = data.batteryLevel;
  doc["ledState"] = data.ledState;
  doc["brightness"] = rand()%(100-20 + 1) + 20;
  doc["id"] = data.id;
  serializeJson(doc, jsonString);
  return jsonString + "**";
}

void setup() {
  Serial.begin(9600);
  data.id = 1;
  wifiSetup();
  Serial1.begin(115200, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
  M5.begin();
  M5.lcd.fillScreen(BLACK);

  // Start the BLE scan on a separate thread
  xTaskCreate(doBLEScan, "BLEScanTask", 8192, NULL, 1, NULL);
  // xTaskCreate(colorSensing, "ColorSensingTask", 8192, NULL, 1, NULL);
  // xTaskCreate(batteryRead, "BatteryTask", 8192, NULL, 1, NULL);
  xTaskCreate(ledTask, "LedTask", 8192, NULL, 1, NULL);
  xTaskCreate(uartSend, "UartTask", 8192, NULL, 1, NULL);
  xTaskCreate(uartRead, "UartRecv", 8192, NULL, 1, NULL);
}

void loop() {
  // Your main loop code here
  // Serial.printf("Loop hehe");
}
