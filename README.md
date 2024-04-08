# LoRaMesh.ino README

## Introduction
This C code provides an example sketch demonstrating how to create a simple messaging client (transmitter) using LoRa technology. It utilizes the RH_RF95 class for communication. Please note that RH_RF95 does not provide addressing or reliability features, so it's suitable for basic messaging needs.

## Requirements
- Arduino board compatible with LoRa technology
- RH_RF95 library
- SoftwareSerial library (if applicable)
- SPI library

## Hardware Setup
- Connect the LoRa module according to the pin configurations defined in the code.
- Ensure proper connections and power supply.

## Usage
1. Upload the code to your Arduino board.
2. Ensure that all necessary libraries are installed.
3. Run the code and observe the serial monitor for messages and interactions.

## Code Overview
- The code initializes the LoRa module and sets up necessary configurations.
- It defines message structures and functions for sending and receiving messages.
- Network operations such as node addition, removal, and nomination of a master node are implemented.
- Heartbeat functionality is included for monitoring node connectivity.
- UART communication is handled for receiving external data.

## Functions
- **setup()**: Initializes necessary components and configurations.
- **loop()**: Continuously listens for incoming messages and performs necessary actions.
- Various helper functions for message handling, network operations, and UART communication.

# LoRaWAN Client README

## Introduction
This Arduino sketch demonstrates a LoRaWAN client using the LMIC (LoraWAN-in-C) library. It sends data packets over a LoRaWAN network at regular intervals. The sketch is designed to work with The Things Network (TTN) and requires appropriate configuration for device EUI, application EUI, and application key.

## Requirements
- Arduino board compatible with LMIC library
- LMIC library
- SPI library
- The Things Network (TTN) account for network configuration

## Hardware Setup
- Connect your LoRa module according to the pin mappings defined in the code.
- Ensure proper connections and power supply.

## Usage
1. Configure your TTN account with appropriate device EUI, application EUI, and application key.
2. Upload the sketch to your Arduino board.
3. Monitor the serial output for debugging information.
4. Verify data transmission on your TTN console.

## Code Overview
- The code initializes LMIC and sets up necessary configurations for LoRaWAN communication.
- It defines device EUI, application EUI, and application key obtained from TTN.
- Data packets are sent at regular intervals defined by `TX_INTERVAL`.
- Serial communication is used for debugging and receiving input data from the user.

## Functions
- **setup()**: Initializes necessary components and configurations.
- **loop()**: Runs the main program loop, handling LoRaWAN events and data transmission.
- **onEvent()**: Callback function to handle LoRaWAN events.
- **do_send()**: Sends data packets over the LoRaWAN network.
- **stringToBytes()**: Utility function to convert a String object to a byte array.

# M5Stick BLE and Sensor Communication README

## Introduction
This Arduino sketch demonstrates communication with BLE (Bluetooth Low Energy) devices, sensor data reading, and HTTP POST requests to a server. It utilizes the M5StickC Plus development board, Adafruit TCS34725 color sensor, and ESP32's BLE capabilities.

## Requirements
- M5StickC Plus development board
- Adafruit TCS34725 color sensor
- Access to a BLE network for scanning nearby devices
- WiFi connection for HTTP POST requests
- Development environment with Arduino IDE or PlatformIO

## Hardware Setup
- Connect the Adafruit TCS34725 color sensor to the appropriate pins on the M5StickC Plus board.
- Ensure proper connections and power supply.

## Usage
1. Upload the sketch to your M5StickC Plus board.
2. Monitor the serial output for debugging information.
3. The sketch will perform BLE scanning, color sensing, battery level reading, and LED control tasks concurrently.
4. Data collected from sensors will be sent over UART and posted to a server via HTTP requests.

## Configuration
- Modify the `ssid`, `password`, and `serverName` variables to match your WiFi network credentials and server address.
- Customize the BLE service UUID (`SERVICE_UUID`) as needed for your application.

## Code Overview
- The code initializes BLE scanning, color sensing, battery reading, LED control, and UART communication tasks.
- BLE scanning is performed in the background to count nearby devices.
- Color sensing task reads data from the Adafruit TCS34725 color sensor and updates brightness information.
- Battery reading task monitors the battery level of the M5StickC Plus board.
- LED task adjusts the brightness of the built-in LED based on sensor data.
- UART communication sends sensor data over serial communication for external processing.
- HTTP POST requests are used to send sensor data to a server for further analysis.

## Functions
- **setup()**: Initializes necessary components and configurations.
- **loop()**: Contains the main program loop (currently empty).
- **doBLEScan()**: Performs BLE scanning to count nearby devices.
- **colorSensing()**: Reads color data from the TCS34725 sensor and calculates brightness.
- **batteryRead()**: Reads the battery level of the M5StickC Plus board.
- **ledTask()**: Controls the built-in LED brightness based on sensor data.
- **uartSend()**: Sends sensor data over UART communication.
- **uartRead()**: Reads data from UART for server communication.
- **postDataToServer()**: Sends sensor data to the specified server via HTTP POST request.
- **serializeDataToSend()**: Serializes sensor data into a JSON format for transmission.

## Notes
- This code is intended for educational and demonstrative purposes.
- Customize and extend the code as per your project requirements.
- Ensure compatibility with your hardware setup and LoRa module specifications.

## Author
This code was authored by Team 07 and is provided as-is without any warranties.