// LoRa 9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_RX

#include <SPI.h>
#include <RH_RF95.h>
#include <SoftwareSerial.h>


#define RFM95_CS 10
#define RX_PIN 6
#define TX_PIN 7


#define RFM95_RST 9
#define RFM95_INT 2
#define NODE_ID 2

#define START_CODE 1
#define REPLY_START_CODE 2
#define PACKET_CODE 3
#define FORWARD_CODE 4
#define NETWORK_CATCHUP_CODE 5
#define HEARTBEAT_OUT 6
#define HEARTBEAT_REPLY 7
#define RECALIBRATE 8

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 923.0
#define retries 10



int loops = 0;
SoftwareSerial soft_serial(0, 1);

const char* authString = "hehehoh";
bool disconnect = false;

int connectedNodes[20];
bool connectedNodesBool[20];

uint8_t noOfNodes = 0;
uint8_t pointerToPutNewNode = 0;
int packetsSent = 0;

int masterNode;

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
void startupInitializationTask(void* params);
void listenTask(void* params);


struct Nodes {
  uint8_t code;
};

struct Message {
  uint8_t code[3];  // Assuming 3 bytes for code
  uint8_t id[3];    // Assuming 3 bytes for ID
  uint8_t prev_id[3];
  uint8_t message_id[3];
  uint8_t destination[3];
  char auth[8];  // Assuming 8 bytes for authentication
  char msg[64];  // Example size for the message, adjust as needed
};



void (*resetFunc)(void) = 0;  //declare reset function at address 0

void setup() {
  // Wait until m5stick send first data
  // delay(3000);
  Serial.begin(9600);
  soft_serial.begin(115200);


  for (int i = 0; i < 20; i++) {
    connectedNodes[i] = -1;
    connectedNodesBool[i] = false;
  }

  connectedNodes[0] = NODE_ID;
  connectedNodesBool[0] = true;

  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  digitalWrite(RFM95_RST, HIGH);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1)
      ;
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 915.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1)
      ;
  }
  Serial.print("Set Freq to:");
  Serial.println(RF95_FREQ);

  // Defaults after init are 915.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(13, false);

  startupInitialization();
}



void loop() {
  Message receivedMsg;
  if (receiveMessage(&receivedMsg, 0)) {
    Serial.print("Some message!! Code :");
    Serial.println(getCode(receivedMsg.code));
    doAction(&receivedMsg, getCode(receivedMsg.code));
  } else {
    loops += +1;
  }

  if (loops == 5) {
    Serial.println("Nominating!");
    nominateMasterNode();
    // Start Nomination
  }

  if (loops > 10 && loops % 30 == 0) {
    printNetworkConnected();
    if (masterNode == NODE_ID) {
      heartbeat(&receivedMsg);
    }
  }
  if (soft_serial.available() > 0) {
    Serial.println("Got data");
    readDataFromUART();
  }
}


void sendInitMessage(Message& msg) {
  constructMessage(msg, START_CODE, NODE_ID, authString, (connectedNodesBool[0]) ? "Internet" : "No", 255, NODE_ID, ++packetsSent);
}

void sendForwardMessage(Message& msg, Message* msgForward) {
  constructMessage(
    msg,
    bytesToInt(msgForward->code, 3),
    bytesToInt(msgForward->id, 3),
    authString,
    msgForward->msg,
    bytesToInt(msgForward->destination, 3),
    NODE_ID,
    bytesToInt(msgForward->message_id, 3));
}

void sendNetworkCatchupMessage(Message& msg, int destination) {
  char result[20 * 4 + 1];  // 3 digits per integer + comma + null terminator
  result[0] = '\0';         // Initialize the string to be empty
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] != -1) {
      char temp[12];                                                                // Temporary buffer to hold the integer as a string
      sprintf(temp, "%d%s", connectedNodes[i], connectedNodesBool[i] ? "T" : "F");  // Convert integer to string
      strcat(result, temp);                                                         // Append the string to the result
      if (i < 20 - 1) {
        strcat(result, ",");  // Add a comma and space between numbers, except after the last number
      }
    }
  }


  constructMessage(msg,
                   NETWORK_CATCHUP_CODE,
                   NODE_ID,
                   authString,
                   result,
                   destination,
                   NODE_ID,
                   ++packetsSent);
}

void sendHeartbeatOut(Message& msg, int destination, char* note) {
  constructMessage(msg,
                   HEARTBEAT_OUT,
                   NODE_ID,
                   authString,
                   note,
                   destination,
                   NODE_ID,
                   packetsSent);
}

void sendHeartBeatReply(Message& msg, int destination, char* note) {
  constructMessage(msg,
                   HEARTBEAT_REPLY,
                   NODE_ID,
                   authString,
                   note,
                   destination,
                   NODE_ID,
                   packetsSent);
}

void sendPacketMessage(Message& msg, int destination, char* note) {
  constructMessage(
    msg,
    PACKET_CODE,
    NODE_ID,
    authString,
    note,
    destination,
    NODE_ID,
    packetsSent);
}

void heartbeat(Message* receivedMsg) {
  for (int i = 0; i < 20; i++) {                                    // Assuming 20 is the maximum number of nodes
    if (connectedNodes[i] != -1 && connectedNodes[i] != NODE_ID) {  // Check if the node is connected
      bool checked = false;
      for (int d = 0; d < 5; d++) {
        if (checked) {
          break;
        }
        sendMessage(HEARTBEAT_OUT, connectedNodes[i], "HEARTBEAT CHECK", NULL);
        if (receiveMessage(receivedMsg, 0)) {  // Wait for a reply for 2000 milliseconds
          checked = true;
          Serial.println("Still in node: " + String(connectedNodes[i]));
        }
      }
      if (!checked) {
        disconnect = true;
        Serial.println("No heartbeat reply from node: " + String(connectedNodes[i]));
        removeFromNetwork(connectedNodes[i]);  // Remove the node from the network
      }
    }
  }
}



void receiveData(char* message) {
  sendMessage(PACKET_CODE, masterNode, message, NULL);
}



void readDataFromUART() {
  String incomingStr = soft_serial.readString();  // Use String type instead of char*
  Serial.println(incomingStr.c_str());
  if (masterNode != NODE_ID) {
    receiveData(incomingStr.c_str());  // Pass const char* to receiveData if it expects it
  } else {
    // Forward...
    soft_serial.write(incomingStr.c_str());  // Use c_str() to get const char* for write
  }
}

void sendMessage(int code, uint8_t destination, char* message, Message* referenceMessage) {
  Message msg;
  if (code == START_CODE) {
    // Init Message
    sendInitMessage(msg);
  } else if (code == FORWARD_CODE) {
    Serial.println("Constructing Forward Message");
    sendForwardMessage(msg, referenceMessage);
  } else if (code == NETWORK_CATCHUP_CODE) {
    Serial.print("Constructing NETWORK CATCHUP Message to");
    Serial.println(destination);
    sendNetworkCatchupMessage(msg, destination);
    Serial.print("In network");
    Serial.println(msg.msg);
    Serial.print("Message Code:");
    Serial.println(bytesToInt(msg.code, 3));
  } else if (code == HEARTBEAT_OUT) {
    Serial.println("Heartbeat Check");
    sendHeartbeatOut(msg, destination, message);
  } else if (code == HEARTBEAT_REPLY) {
    Serial.println("Heartbeat reply");
    Serial.print("MSG->");
    sendHeartBeatReply(msg, destination, message);
  }else if(code == PACKET_CODE){
    sendPacketMessage(msg, destination, message);
  }
  sendMessageOut(&msg);
}


void sendMessageOut(Message* msg) {
  size_t structSize = sizeof(*msg);
  uint8_t* structAsArray = new uint8_t[structSize];
  memcpy(structAsArray, msg, structSize);

  if (!rf95.send(structAsArray, structSize)) {
    Serial.println("Send failed");
  } else {
    Serial.println("Send initialized");
  }
  if (!rf95.waitPacketSent()) {
    Serial.println("Wait for packet to complete failed");
  } else {
    Serial.println("Sent!");
  }
  delete[] structAsArray;
}

bool receiveMessage(Message* receivedMsg, uint16_t millis) {
  uint16_t millie = 650;
  if (millis != 0) {
    millie = millis;
  }
  if (rf95.waitAvailableTimeout(millie)) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      memcpy(receivedMsg, buf, sizeof(Message));
      printMessage(receivedMsg);
      if (verifyPacket(receivedMsg)) {
        printMessage(receivedMsg);
        return true;
      } else {
        Serial.println("Drop Packet");
      }
    } else {
      Serial.print("????");
    }
  }
  return false;
}

void startupInitialization() {
  // Create RTOS tasks
  // TaskHandle_t listen_task;
  Message receivedMsg;
  for (int i = 0; i < retries; i++) {
    sendMessage(START_CODE, 255, (connectedNodesBool[0]) ? "Internet" : "No", NULL);
    for (int i = 0; i < 3; i++) {
      if (receiveMessage(&receivedMsg, 0)) {
        Serial.print("Some message!! Code :");
        Serial.println(getCode(receivedMsg.code));
        doAction(&receivedMsg, getCode(receivedMsg.code));
      };
    }
  }
};






void printMessage(Message* receivedMsg) {
  Serial.print("Code: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(receivedMsg->code[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("ID: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(receivedMsg->id[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("Desintation: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(receivedMsg->destination[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("Auth: ");
  Serial.println(receivedMsg->auth);
  Serial.print("Message: ");
  Serial.println(receivedMsg->msg);
}



bool verifyPacket(Message* receivedMsg) {
  Serial.print("Received Message: ");
  Serial.println(receivedMsg->auth);
  Serial.print("Auth Message: ");
  Serial.println(authString);
  Serial.print("Destination: ");
  Serial.println((bytesToInt(receivedMsg->destination, 3)));
  bool originDuplicationCheck = bytesToInt(receivedMsg->id, 3) != NODE_ID;
  bool authenticationCheck = strcmp_custom(receivedMsg->auth, authString) == 0;
  bool destinationCheck = ((bytesToInt(receivedMsg->destination, 3) == NODE_ID) || (bytesToInt(receivedMsg->destination, 3) == 255));
  if (!originDuplicationCheck) {
    Serial.println("Dropping due to origin duplication");
  }
  if (!authenticationCheck) {
    Serial.println("Dropping due to auth failure");
  }
  if (!destinationCheck) {
    Serial.println("Dropping due to desination failure");
  }
  return originDuplicationCheck && authenticationCheck && destinationCheck;
}


void doAction(Message* message, uint32_t code) {
  // START CODE == Node joining network
  if (code == START_CODE) {
    uint32_t node_id = bytesToInt(message->id, 3);
    Serial.println(node_id);
    if (!existsInNetwork(node_id)) {
      bool internetConnected = strcmp_custom(message->msg, "Internet") == 0;
      addToNetwork(node_id, internetConnected);
      Serial.print("Node ID: ");
      Serial.print(node_id);
      Serial.print("-");
      Serial.print(internetConnected);
      Serial.println(" joining");
      printNetworkConnected();
      if (loops >= 5) {
        delay(2000);

        // Initial Nomination over
        for (int i = 0; i < 9; i++) {
          delay(500);
          sendMessage(NETWORK_CATCHUP_CODE, node_id, "", NULL);
        }
      }
      loops = 0;  // Reset Wait for Nomination, someone still joining
    }
    // sendForwardMessage(Message &msg, Message *msgForward)
    sendMessage(FORWARD_CODE, 255, "", message);
  }
  if (code == NETWORK_CATCHUP_CODE) {
    int number;
    bool isNumber = false;
    bool network = false;
    for (int i = 0; message->msg[i] != '\0';) {
      int d = 0;
      while (message->msg[i + d] != ',' && message->msg[i + d] != '\0') {
        if (message->msg[i + d] >= '0' && message->msg[i + d] <= '9') {
          // Extracting the number
          number = number * 10 + (message->msg[i + d] - '0');
          isNumber = true;
        }
        d++;
      }
      if (isNumber) {
        if (message->msg[i + d - 1] == 'T') {
          network = true;
        } else if (message->msg[i + d - 1] == 'F') {
          network = false;
        }
        if (!existsInNetwork(number)) {
          addToNetwork(number, network);
        }


        number = 0;  // Reset the number for the next segment
        isNumber = false;
        network = false;
      }
      i += d + 1;  // Increment by d+1 to move past the number segment and the character after it
      // Increment by d+1 to move past the comma
    }
    Serial.println("Network Catchup!!");
  }

  if (code == HEARTBEAT_REPLY) {
    if (masterNode == NODE_ID) {
      Serial.println("Heartbeat reply received");
    }
  }
  if (code == HEARTBEAT_OUT) {
    if (masterNode != NODE_ID) {
      Serial.print("Replying to ");
      Serial.println(bytesToInt(message->id, 3));
      for (int d = 0; d < 3; d++) {
        delay(650);
        sendMessage(HEARTBEAT_REPLY, bytesToInt(message->id, 3), "HEARTBEAT REPLY", message);
      }
    }
  }

  if(code == PACKET_CODE){
    if(NODE_ID == masterNode){
      soft_serial.write(message->msg);
    }
  }
}


// Nomination

void nominateMasterNode() {
  Serial.print("Out of the networks");
  printNetworkConnected();
  int suggested = 255;
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] < suggested && connectedNodesBool[i]) {
      suggested = connectedNodes[i];
    }
  }


  masterNode = suggested;
  Serial.print("Node ");
  Serial.print(masterNode);
  Serial.println("Nominated, all will go through him");
}

// Network Operations

void addToNetwork(uint32_t code, bool connectedToNetwork) {
  bool insert = true;
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] == -1) {
      if (insert) {
        connectedNodes[i] = code;
        connectedNodesBool[i] = connectedToNetwork;
        noOfNodes += 1;
        insert = false;
      } else {
        pointerToPutNewNode = i;
        break;
      }
    }
  }
}

void removeFromNetwork(uint32_t code) {
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] == code) {
      connectedNodes[i] = -1;
      noOfNodes -= 1;
      pointerToPutNewNode = i;
    }
  }
}


bool existsInNetwork(uint32_t code) {
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] == code) {
      return true;
    }
  }
  return false;
}

int indexInNetwork(uint32_t code) {
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] == code) {
      return true;
    }
  }
  return -1;
}


void printNetworkConnected() {
  Serial.print("{");
  for (int i = 0; i < 20; i++) {
    if (connectedNodes[i] != -1) {
      Serial.print(connectedNodes[i]);
      Serial.print("|");
    }
  }
  Serial.println("}");
}

// Helpers
uint32_t getCode(uint8_t* code) {
  return bytesToInt(code, 3);
}

void constructMessage(Message& msg, int code, int id, const char* auth, const char* message, int destination, int prev_id, int msg_id) {
  // Convert code and ID to bytes and store in the structure
  msg.code[0] = (code / 100) % 10;
  msg.code[1] = (code / 10) % 10;
  msg.code[2] = code % 10;

  msg.id[0] = (id / 100) % 10;
  msg.id[1] = (id / 10) % 10;
  msg.id[2] = id % 10;

  msg.destination[0] = (destination / 100) % 10;
  msg.destination[1] = (destination / 10) % 10;
  msg.destination[2] = destination % 10;

  msg.prev_id[0] = (prev_id / 100) % 10;
  msg.prev_id[1] = (prev_id / 10) % 10;
  msg.prev_id[2] = prev_id % 10;

  msg.message_id[0] = (msg_id / 100) % 10;
  msg.message_id[1] = (msg_id / 10) % 10;
  msg.message_id[2] = msg_id % 10;

  // Copy authentication and message into the structure
  strncpy(msg.auth, auth, sizeof(msg.auth));
  strncpy(msg.msg, message, sizeof(msg.msg));
}

bool strcmp_custom(const char* str1, const char* str2) {
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

// Function to convert byte array to integer
uint32_t bytesToInt(uint8_t* bytes, int length) {
  uint32_t result = 0;
  return (bytes[0] * 100 + bytes[1] * 10 + bytes[2]);
}
