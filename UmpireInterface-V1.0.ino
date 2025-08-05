#include <HardwareSerial.h>
#include "BLEDevice.h"
//#include "BLEScan.h"

#define RXD2 18
#define TXD2 17

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// strike characteristic 
static BLEUUID        commandUUID    ("9f0765fd-86aa-4354-a954-807d42a350f6");
static BLEUUID        ballUUID      ("d05dc9ee-1d84-4837-a2d9-cd5654f7fc14");
static BLEUUID        outUUID       ("b4db4085-9a8b-408b-a790-b42f25935363");
static BLEUUID        inningUUID    ("bdb52ad7-5b55-441b-8473-20dd29ce92f6");
static BLEUUID        homeScoreUUID ("d679be1c-0d36-4e24-ab4b-701a2cc41184");
static BLEUUID        awayScoreUUID ("729b1861-7e35-450a-918d-c59c1925b8b1");
static BLEUUID        awayTeamUUID  ("ce426594-a606-4dcb-974c-f2fa0f8d59e4");
static BLEUUID        homeTeamUUID  ("45782257-dc6f-4b56-ba4b-39d2d1b1c890");
static BLEUUID        timeUUID      ("149e140e-1c7f-4fe6-bdad-ffbd779e403a");

static BLERemoteCharacteristic* pRemoteCharacteristic; 
static BLERemoteCharacteristic* pCommandCharacteristic;
static BLERemoteCharacteristic* pBallCharacteristic;
static BLERemoteCharacteristic* pOutCharacteristic;
static BLERemoteCharacteristic* pInningCharacteristic;
static BLERemoteCharacteristic* pHomescoreCharacteristic;
static BLERemoteCharacteristic* pAwayscoreCharacteristic;
static BLERemoteCharacteristic* pAwayteamCharacteristic;
static BLERemoteCharacteristic* pHometeamCharacteristic;
static BLERemoteCharacteristic* pTimeCharacteristic;
static BLEAdvertisedDevice* myDevice;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

unsigned long previousMillis = 0;
const unsigned long ScorelinkWriteInterval = 5000;
struct BaseballGame {
  int strikes;
  int balls;
  int outs;
  int innings;
  int homeScore;
  int awayScore;
  unsigned long time;
  char awayTeam[8];
  char hometeam[8];
};

struct ScorelinkProtocolPacket {
  // Time 
  char Clock_Min_Tens;  // 0-9 or blank
  char Clock_Min_Ones;  // 0-9 
  char Clock_Sec_Tens;  // 0-9 
  char Clock_Sec_Ones;  // 0-9 or blank

  char Balls;           // 0-4 or blank
  char Strikes;         // 0-3 or blank
  char Outs;            // 0-3 or blank
  
  // Innings
  char Inning_Tens;     // 1 or blank
  char Inning_Ones;     // 0-9


  char Guest_Runs_Tens; // 1-9 or blank
  char Guest_Runs_Ones; // 0-9
  char Home_Runs_Tens;  // 1-9 or blank
  char Home_Runs_Ones;  // 0-9 

  char Top_Or_Bottom; // T or B
  char Guest_Hits_Tens; // 1-9 or blank
  char Guest_Hits_Ones; // 0-9
  char Home_Hits_Tens;  // 1-9 or blank
  char Home_Hits_Ones;  // 0-9 
  char Guest_Errors_Ones; // 0-9
  char Home_Errors_Ones;  // 0-9
  char At_Bat_Tens;       // 1-9 or blank
  char At_Bat_Ones;       // 0-9 
  char Error_Flag;        // H (or?) E or BLANK -- doesn't actually say or but im assuming its one or the other or blank
  char Guest_Error;       // 0-9
  char Top_of_Inning;     // . or blank
  char Bottom_of_Inning;  // . or blank

  // blank packet at 28
  // blank packet at 29

  char Guest_Inning_1;  // 0-9
  char Home_Inning_1;   // 0-9
  char Guest_Inning_2;  // 0-9
  char Home_Inning_2;   // 0-9
  char Guest_Inning_3;  // 0-9
  char Home_Inning_3;   // 0-9
  char Guest_Inning_4;  // 0-9
  char Home_Inning_4;   // 0-9
  char Guest_Inning_5;  // 0-9    
  char Home_Inning_5;   // 0-9
  char Guest_Inning_6;  // 0-9
  char Home_Inning_6;   // 0-9
  char Guest_Inning_7;  // 0-9
  char Home_Inning_7;   // 0-9
  char Guest_Inning_8;  // 0-9
  char Home_Inning_8;   // 0-9
  char Guest_Inning_9;  // 0-9
  char Home_Inning_9;   // 0-9
  char Guest_Inning_10; // 0-9
  char Home_Inning_10;  // 0-9

  char Guest_Team_Name[8]; // ASCII A-Z Takes up from Position [50, 57]
  char Home_Team_Name[8]; // ASCII A-Z Takes up from Position [58, 65]
  char OnFirst;     // . or blank
  char OnSecond;    // . or blank
  char OnThird;     // . or blank
  char Pitch_Speed_100s;  // 0-9 or blank
  char Pitch_Speed_10s;   // 0-9 or blank
  char Pitch_Speed_1s;    // 0-9
  char Pitch_Count_100s;  // 0-9 or blank
  char Pitch_Count_10s;   // 0-9 or blank
  char Pitch_Count_1s;    // 0-9

  char Layer;
  char Command_Type;
  char Ad_CMD;
  char Ad_Number;
  char CMD;
  char Data_10s;
  char Data_1s;

  char Clock_Status;
  char Clock_Mode;
  char PlayClock_Status;

  char Inbound_Channel_Number; // blank 0-9
  char Bot_MSB[6]; // hex
  char TDateTime[12]; // 12 char hex
};

char blankByte = ' ';
ScorelinkProtocolPacket default_start;
ScorelinkProtocolPacket scoreLink1;
BaseballGame game1;


void commandParser(uint8_t* pData, size_t length) {
  char command = (char)pData[0]; // Cast the first byte to char
  int rawValue = pData[1]; // Access the second byte directly as an integer
  char formatValue = (char)pData[1]; // Cast the first byte to char

  Serial.println("Command and Value");
  Serial.write(command);
  Serial.write(formatValue);
  Serial.println();

  switch (command) {
    case 'S' : {
      scoreLink1.Strikes = formatValue;
      Serial.println("Added to strike in command parser");
      break;

    }
    default : {
      Serial.println("Unrecognized Command");
      Serial.write(command);
      Serial.println();
      break;
    }

    Serial2.println(getPacketData(scoreLink1));

  }
}

static void notifyCallback( // this functino can handle all notificaitons, however need to check which remoteCharacteristic is notifying
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    BLEUUID uuid = pBLERemoteCharacteristic->getUUID();
    
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
    if (strcmp(uuid.toString().c_str(), commandUUID.toString().c_str()) == 0) {
      // Serial.println("Should not be here if we are pressing button 1");
      Serial.println("In Command");
      // int test = (int)(*pData) - 48;
      // Serial.write(pData, length);
      commandParser(pData,length);
      // char character = '0' + test;    // Convert integer to character
      // scoreLink1.Strikes = character;
    }else if (strcmp(uuid.toString().c_str(), ballUUID.toString().c_str()) == 0) {
      Serial.println("In balllslslsl");
      int test = (int)(*pData) - 48;
      char character = '0' + test;    // Convert integer to character
      scoreLink1.Balls = character;
    } else if (strcmp(uuid.toString().c_str(), outUUID.toString().c_str()) == 0) {
      Serial.println("In outststs");
      int test = (int)(*pData) - 48;
      char character = '0' + test;    // Convert integer to character
      scoreLink1.Outs = character;
    } else if (strcmp(uuid.toString().c_str(), inningUUID.toString().c_str()) == 0) {
      int test = (int)(*pData) - 48;
      Serial.println("In inninignngngns");
      game1.innings = test;
    } else {
      Serial.println("UUIDS do not match"); 
      Serial.println(uuid.toString().c_str());
      Serial.println(commandUUID.toString().c_str());
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    pCommandCharacteristic = pRemoteService->getCharacteristic(commandUUID);
    pBallCharacteristic = pRemoteService->getCharacteristic(ballUUID);
    pOutCharacteristic = pRemoteService->getCharacteristic(outUUID);
    pInningCharacteristic = pRemoteService->getCharacteristic(inningUUID);
    // pHomescoreCharacteristic = pRemoteService->getCharacteristic(homeScoreUUID);
    // pAwayscoreCharacteristic = pRemoteService->getCharacteristic(awayScoreUUID);
    // pAwayteamCharacteristic = pRemoteService->getCharacteristic(awayTeamUUID);
    // pHometeamCharacteristic = pRemoteService->getCharacteristic(homeTeamUUID);
    // pTimeCharacteristic = pRemoteService->getCharacteristic(timeUUID);

    // add the other characteristics using the above method

    // Use a checker

    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our regular characteristic");

    if (pCommandCharacteristic == nullptr) {
      Serial.print("Failed to find our strike UUID: ");
      Serial.println(commandUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our strike characteristic");

    // Read the value of the characteristic.
    // if(pRemoteCharacteristic->canRead()) {
    //   std::string value = pRemoteCharacteristic->readValue();
    //   Serial.print("The characteristic value was: ");
    //   Serial.println(value.c_str());
    // }

    // Read the value of the characteristic.
    // if(pCommandCharacteristic->canRead()) {
    //   std::string value = pCommandCharacteristic->readValue();
    //   Serial.print("The strike characteristic value was: ");
    //   Serial.println(value.c_str());
    // }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server. // We can implement to add more for each type of bluetooth server?
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void copyDefaultSettings() {
  default_start.Clock_Min_Tens    = '0';  // 0-9 or blank
  default_start.Clock_Min_Ones    = '0';  // 0-9 
  default_start.Clock_Sec_Tens    = '0';  // 0-9 
  default_start.Clock_Sec_Ones    = '0';  // 0-9 or blank

  default_start.Balls             = '0';           // 0-4 or blank
  default_start.Strikes           = '0';         // 0-3 or blank
  default_start.Outs              = '0';            // 0-3 or blank
  
  // Innings
  default_start.Inning_Tens       = '0';     // 1 or blank
  default_start.Inning_Ones       = '0';     // 0-9


  default_start.Guest_Runs_Tens   = '0'; // 1-9 or blank
  default_start.Guest_Runs_Ones   = '0'; // 0-9
  default_start.Home_Runs_Tens    = '0';  // 1-9 or blank
  default_start.Home_Runs_Ones    = '0';  // 0-9 

  default_start.Top_Or_Bottom     = 'T'; // T or B
  default_start.Guest_Hits_Tens   = '0'; // 1-9 or blank
  default_start.Guest_Hits_Ones   = '0'; // 0-9
  default_start.Home_Hits_Tens    = '0';  // 1-9 or blank
  default_start.Home_Hits_Ones    = '0';  // 0-9 
  default_start.Guest_Errors_Ones = '0'; // 0-9
  default_start.Home_Errors_Ones  = '0';  // 0-9
  default_start.At_Bat_Tens       = '0';       // 1-9 or blank
  default_start.At_Bat_Ones       = '0';       // 0-9 
  default_start.Error_Flag        = '0';        // H (or?) E or BLANK -- doesn't actually say or but im assuming its one or the other or blank
  default_start.Guest_Error       = '0';       // 0-9
  default_start.Top_of_Inning     = '.';     // . or blank
  default_start.Bottom_of_Inning  = '0';  // . or blank

  // blank packet at 28
  // blank packet at 29

  default_start.Guest_Inning_1    = '0';  // 0-9
  default_start.Home_Inning_1     = '0';   // 0-9
  default_start.Guest_Inning_2    = '0';  // 0-9
  default_start.Home_Inning_2     = '0';   // 0-9
  default_start.Guest_Inning_3    = '0';  // 0-9
  default_start.Home_Inning_3     = '0';   // 0-9
  default_start.Guest_Inning_4    = '0';  // 0-9
  default_start.Home_Inning_4     = '0';   // 0-9
  default_start.Guest_Inning_5    = '0';  // 0-9    
  default_start.Home_Inning_5     = '0';   // 0-9
  default_start.Guest_Inning_6    = '0';  // 0-9
  default_start.Home_Inning_6     = '0';   // 0-9
  default_start.Guest_Inning_7    = '0';  // 0-9
  default_start.Home_Inning_7     = '0';   // 0-9
  default_start.Guest_Inning_8    = '0';  // 0-9
  default_start.Home_Inning_8     = '0';   // 0-9
  default_start.Guest_Inning_9    = '0';  // 0-9
  default_start.Home_Inning_9     = '0';   // 0-9
  default_start.Guest_Inning_10   = '0'; // 0-9
  default_start.Home_Inning_10    = '0';  // 0-9

  for (int i = 0; i < 8; i++) {
    default_start.Guest_Team_Name[i] = "Lehigh  "[i];
  }
  // strcpy(default_start.Home_Team_Name, "Lafayett"); // ASCII A-Z Takes up from Position [58, 65]
  for (int i = 0; i < 8; i++) {
    default_start.Home_Team_Name[i] = "Lafayett"[i];
  }
  default_start.OnFirst           = '0';     // . or blank
  default_start.OnSecond          = '0';    // . or blank
  default_start.OnThird           = '0';     // . or blank
  default_start.Pitch_Speed_100s  = '0';  // 0-9 or blank
  default_start.Pitch_Speed_10s   = '0';   // 0-9 or blank
  default_start.Pitch_Speed_1s    = '0';    // 0-9
  default_start.Pitch_Count_100s  = '0';  // 0-9 or blank
  default_start.Pitch_Count_10s   = '0';   // 0-9 or blank
  default_start.Pitch_Count_1s    = '0';    // 0-9

  default_start.Layer = blankByte;
  default_start.Command_Type = blankByte;
  default_start.Ad_CMD = blankByte;
  default_start.Ad_Number = '0';
  default_start.CMD = '0';
  default_start.Data_10s = '0';
  default_start.Data_1s = '0';

  default_start.Clock_Status = 'S';
  default_start.Clock_Mode = ':';
  default_start.PlayClock_Status = 'S';
  default_start.Inbound_Channel_Number = '0';
  unsigned long decimalNumber = 61750; // Use unsigned long for a 6-byte representation of scorelink bot number -- thanks chatGPT 
  // Doesn't work to do decimal number>> 40 - 8*i
  // so 61750 is 1C1C in hex add two leading 0s to the front to make it work??
  default_start.Bot_MSB[0] = '0';
  default_start.Bot_MSB[1] = '0';
  default_start.Bot_MSB[2] = '1';
  default_start.Bot_MSB[3] = 'C';
  default_start.Bot_MSB[4] = '1';
  default_start.Bot_MSB[5] = 'C';


  for (int i = 0; i < 12; i++) { // for no cause idk what TDateTime is and how to use it
    default_start.TDateTime[i] = 'k';
  }

}

String stringToHex(String str) {
  String hexString = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    hexString += String(c, HEX);
  }
  return hexString;
}

void assignBasicParamsToDefualtStart() {
  default_start.Clock_Min_Tens    = '1';  // 0-9 or blank
  default_start.Clock_Min_Ones    = '5';  // 0-9 
  default_start.Clock_Sec_Tens    = '6';  // 0-9 
  default_start.Clock_Sec_Ones    = '0';  // 0-9 or blank

  default_start.Balls             = '1';           // 0-4 or blank
  default_start.Strikes           = '2';         // 0-3 or blank
  default_start.Outs              = '1';            // 0-3 or blank
  
  // Innings
  default_start.Inning_Tens       = '1';     // 1 or blank
  default_start.Inning_Ones       = '8';     // 0-9


  default_start.Guest_Runs_Tens   = ' '; // 1-9 or blank
  default_start.Guest_Runs_Ones   = '3'; // 0-9
  default_start.Home_Runs_Tens    = '0';  // 1-9 or blank
  default_start.Home_Runs_Ones    = '0';  // 0-9 

  default_start.Top_Or_Bottom     = 'T'; // T or B
  default_start.Guest_Hits_Tens   = ' '; // 1-9 or blank
  default_start.Guest_Hits_Ones   = ' '; // 0-9
  default_start.Home_Hits_Tens    = ' ';  // 1-9 or blank
  default_start.Home_Hits_Ones    = ' ';  // 0-9 
  default_start.Guest_Errors_Ones = ' '; // 0-9
  default_start.Home_Errors_Ones  = ' ';  // 0-9
  default_start.At_Bat_Tens       = ' ';       // 1-9 or blank
  default_start.At_Bat_Ones       = ' ';       // 0-9 
  default_start.Error_Flag        = ' ';        // H (or?) E or BLANK -- doesn't actually say or but im assuming its one or the other or blank
  default_start.Guest_Error       = ' ';       // 0-9
  default_start.Top_of_Inning     = '.';     // . or blank
  default_start.Bottom_of_Inning  = ' ';  // . or blank

  // blank packet at 28
  // blank packet at 29

  default_start.Guest_Inning_1    = '0';  // 0-9
  default_start.Home_Inning_1     = '0';   // 0-9
  default_start.Guest_Inning_2    = '0';  // 0-9
  default_start.Home_Inning_2     = '0';   // 0-9
  default_start.Guest_Inning_3    = '0';  // 0-9
  default_start.Home_Inning_3     = '0';   // 0-9
  default_start.Guest_Inning_4    = '0';  // 0-9
  default_start.Home_Inning_4     = '0';   // 0-9
  default_start.Guest_Inning_5    = '0';  // 0-9    
  default_start.Home_Inning_5     = '0';   // 0-9
  default_start.Guest_Inning_6    = '0';  // 0-9
  default_start.Home_Inning_6     = '0';   // 0-9
  default_start.Guest_Inning_7    = '0';  // 0-9
  default_start.Home_Inning_7     = '0';   // 0-9
  default_start.Guest_Inning_8    = '0';  // 0-9
  default_start.Home_Inning_8     = '0';   // 0-9
  default_start.Guest_Inning_9    = '0';  // 0-9
  default_start.Home_Inning_9     = '0';   // 0-9
  default_start.Guest_Inning_10   = '0'; // 0-9
  default_start.Home_Inning_10    = '0';  // 0-9

  String lehigh = "Lehigh  ";
  String laf = "Lafayette";

  // String hexLehigh = stringToHex(lehigh);
  // String hexLafayette = stringToHex(laf);

  for (int i = 0; i < 8; i++) {
    default_start.Guest_Team_Name[i] = laf[i];
  }
  // strcpy(default_start.Home_Team_Name, "Lafayett"); // ASCII A-Z Takes up from Position [58, 65]
  for (int i = 0; i < 8; i++) {
    default_start.Home_Team_Name[i] = lehigh[i];
  }
  default_start.OnFirst           = ' ';     // . or blank
  default_start.OnSecond          = ' ';    // . or blank
  default_start.OnThird           = ' ';     // . or blank
  default_start.Pitch_Speed_100s  = ' ';  // 0-9 or blank
  default_start.Pitch_Speed_10s   = ' ';   // 0-9 or blank
  default_start.Pitch_Speed_1s    = ' ';    // 0-9
  default_start.Pitch_Count_100s  = ' ';  // 0-9 or blank
  default_start.Pitch_Count_10s   = ' ';   // 0-9 or blank
  default_start.Pitch_Count_1s    = ' ';    // 0-9

  default_start.Layer = blankByte; // what do these following sections mean?
  default_start.Command_Type = blankByte;
  default_start.Ad_CMD = blankByte;
  default_start.Ad_Number = '0';
  default_start.CMD = '0';
  default_start.Data_10s = '0';
  default_start.Data_1s = '0';

  default_start.Clock_Status = 'S'; // R or S, reset or start?
  default_start.Clock_Mode = ':'; // what does clock mode represent?
  default_start.PlayClock_Status = 'S'; 
  default_start.Inbound_Channel_Number = '0';
  // unsigned long decimalNumber = 61750; // Decimal number to convert // old one
  unsigned long decimalNumber = 61801; // Decimal number to convert // new scorelink number
  char Bot_MSB[6]; // Array to store 6-byte hexadecimal representation

  // Convert the unsigned long number to a 6-character hexadecimal string
  sprintf(Bot_MSB, "%06lX", decimalNumber);
  default_start.Bot_MSB[0] = Bot_MSB[0];
  default_start.Bot_MSB[1] = Bot_MSB[1];
  default_start.Bot_MSB[2] = Bot_MSB[2];
  default_start.Bot_MSB[3] = Bot_MSB[3];
  default_start.Bot_MSB[4] = Bot_MSB[4];
  default_start.Bot_MSB[5] = Bot_MSB[5];


  // can use the sprintf to format the TDateTime, but I don't believe this device is capable of getting the TDateTime without internet connection
  for (int i = 0; i < 12; i++) { //  Updates by itself in MQTT Viewer
    default_start.TDateTime[i] = 'k';
  }

}

String getPacketData(ScorelinkProtocolPacket game) {
  String packetData = "";
  
  packetData += '0'; // start bit // packet position 0
  packetData += '1'; // second bit # matching sbdata on emulator
  packetData += game.Clock_Min_Tens;
  packetData += game.Clock_Min_Ones;
  packetData += game.Clock_Sec_Tens;
  packetData += game.Clock_Sec_Ones;
  packetData += game.Balls;
  packetData += game.Strikes;
  packetData += game.Outs;
  packetData += game.Inning_Tens;
  packetData += game.Inning_Ones; // packet position 10
  packetData += game.Guest_Runs_Tens;
  packetData += game.Guest_Runs_Ones;
  packetData += game.Home_Runs_Tens;
  packetData += game.Home_Runs_Ones;
  packetData += game.Top_Or_Bottom;
  packetData += game.Guest_Hits_Tens;
  packetData += game.Guest_Hits_Ones;
  packetData += game.Home_Hits_Tens;
  packetData += game.Home_Hits_Ones;
  packetData += game.Guest_Errors_Ones; // packet position 20
  packetData += game.Home_Errors_Ones;
  packetData += game.At_Bat_Tens;
  packetData += game.At_Bat_Ones;
  packetData += game.Error_Flag;
  packetData += game.Guest_Error;
  packetData += game.Top_of_Inning;
  packetData += game.Bottom_of_Inning; // packet position 27
  packetData += blankByte;             // packet position 28 blank
  packetData += blankByte;             // packet position 29 blank
  packetData += game.Guest_Inning_1;                 // packet position 30
  packetData += game.Home_Inning_1;
  packetData += game.Guest_Inning_2;
  packetData += game.Home_Inning_2;
  packetData += game.Guest_Inning_3;
  packetData += game.Home_Inning_3;
  packetData += game.Guest_Inning_4;
  packetData += game.Home_Inning_4;
  packetData += game.Guest_Inning_5;
  packetData += game.Home_Inning_5;
  packetData += game.Guest_Inning_6;     // packet position 40
  packetData += game.Home_Inning_6;
  packetData += game.Guest_Inning_7;
  packetData += game.Home_Inning_7;
  packetData += game.Guest_Inning_8;
  packetData += game.Home_Inning_8;
  packetData += game.Guest_Inning_9;
  packetData += game.Home_Inning_9;
  packetData += game.Guest_Inning_10;
  packetData += game.Home_Inning_10;    // packet position 49
  for (int i = 0; i < 8; i++) { // [50, 57]
    packetData += game.Guest_Team_Name[i];
  }
  for (int i = 0; i < 8; i++) { // [58, 65]
    packetData += game.Home_Team_Name[i];
  }
  
  packetData += game.OnFirst; // position 66
  packetData += game.OnSecond;
  packetData += game.OnThird;
  packetData += game.Pitch_Speed_100s;
  packetData += game.Pitch_Speed_10s; // position 70
  packetData += game.Pitch_Speed_1s;
  packetData += game.Pitch_Count_100s;
  packetData += game.Pitch_Count_10s;
  packetData += game.Pitch_Count_1s; // position 74
  for (int i = 0; i < 23; i++) {
    packetData += blankByte; // [75 - 97 blank] 
  }


  char Bot_MSB[6]; // hex
  char TDateTime[12]; // 12 char hex
  
  // Not sure what to do for these asks technical support or president for help
  packetData += game.Layer; // layer position 98
  packetData += game.Command_Type; // Command type (blank for Ad)
  packetData += game.Ad_CMD; // Ad Cmd (All printable ASCII)
  packetData += game.Ad_Number; // Ad # (All Printable ASCII)
  packetData += game.CMD; // Position 102 Cmd: 0-9, A-Z or blank
  packetData += game.Data_10s; // Data 10s: 0-9;
  packetData += game.Data_1s; // Data 1s: 0-9;

  for (int i = 0; i < 11; i++) {
    packetData += blankByte; // [105 - 116 blank] 
  }

  packetData += game.Clock_Status;
  packetData += game.Clock_Mode;
  packetData += game.PlayClock_Status;

  for (int i = 0; i < 12; i++) {
    packetData += blankByte; // [105 - 116 blank] 
  }
  packetData += game.Inbound_Channel_Number;
  for (int i = 0; i < 6; i++) {
    packetData += game.Bot_MSB[i];
  }
  for (int i = 0; i < 12; i++) {
    packetData += game.TDateTime[i]; // Currently just 'k' repeated, but in MQTT viewer it the TDateTime section seems to update itself. 
  }
  
  packetData += "\n";
  return packetData;
}

void writeScorelinkData(ScorelinkProtocolPacket game) {
  Serial.write('0' ); // start bit // packet position 0
  Serial.write('1' ); // second bit # matching sbdata on emulator
  Serial.write(game.Clock_Min_Tens );
  Serial.write(game.Clock_Min_Ones );
  Serial.write(game.Clock_Sec_Tens );
  Serial.write(game.Clock_Sec_Ones );
  Serial.write(game.Balls );
  Serial.write(game.Strikes );
  Serial.write(game.Outs );
  Serial.write(game.Inning_Tens );
  Serial.write(game.Inning_Ones ); // packet position 10
  Serial.write(game.Guest_Runs_Tens );
  Serial.write(game.Guest_Runs_Ones );
  Serial.write(game.Home_Runs_Tens );
  Serial.write(game.Home_Runs_Ones );
  Serial.write(game.Top_Or_Bottom );
  Serial.write(game.Guest_Hits_Tens );
  Serial.write(game.Guest_Hits_Ones );
  Serial.write(game.Home_Hits_Tens );
  Serial.write(game.Home_Hits_Ones );
  Serial.write(game.Guest_Errors_Ones ); // packet position 20
  Serial.write(game.Home_Errors_Ones );
  Serial.write(game.At_Bat_Tens );
  Serial.write(game.At_Bat_Ones );
  Serial.write(game.Error_Flag );
  Serial.write(game.Guest_Error );
  Serial.write(game.Top_of_Inning );
  Serial.write(game.Bottom_of_Inning ); // packet position 27
  Serial.write(blankByte );             // packet position 28 blank
  Serial.write(blankByte );             // packet position 29 blank
  Serial.write(game.Guest_Inning_1 );                 // packet position 30
  Serial.write(game.Home_Inning_1 );
  Serial.write(game.Guest_Inning_2 );
  Serial.write(game.Home_Inning_2 );
  Serial.write(game.Guest_Inning_3 );
  Serial.write(game.Home_Inning_3 );
  Serial.write(game.Guest_Inning_4 );
  Serial.write(game.Home_Inning_4 );
  Serial.write(game.Guest_Inning_5 );
  Serial.write(game.Home_Inning_5 );
  Serial.write(game.Guest_Inning_6 );     // packet position 40
  Serial.write(game.Home_Inning_6 );
  Serial.write(game.Guest_Inning_7 );
  Serial.write(game.Home_Inning_7 );
  Serial.write(game.Guest_Inning_8 );
  Serial.write(game.Home_Inning_8 );
  Serial.write(game.Guest_Inning_9 );
  Serial.write(game.Home_Inning_9 );
  Serial.write(game.Guest_Inning_10 );
  Serial.write(game.Home_Inning_10 );    // packet position 49
   for (int i = 0; i < 8; i++) { // [50, 57]
    Serial.write(game.Guest_Team_Name[i]);
  }
  for (int i = 0; i < 8; i++) { // [58, 65]
    Serial.write(game.Home_Team_Name[i]);
  }
  
  Serial.write(game.OnFirst ); // position 66
  Serial.write(game.OnSecond );
  Serial.write(game.OnThird );
  Serial.write(game.Pitch_Speed_100s );
  Serial.write(game.Pitch_Speed_10s ); // position 70
  Serial.write(game.Pitch_Speed_1s );
  Serial.write(game.Pitch_Count_100s );
  Serial.write(game.Pitch_Count_10s );
  Serial.write(game.Pitch_Count_1s ); // position 74
  for (int i = 0; i < 23; i++) {
    Serial.write(blankByte ); // [75 - 97 blank] 
  }


  char Bot_MSB[6]; // hex
  char TDateTime[12]; // 12 char hex
  
  // Not sure what to do for these asks technical support or president for help
  Serial.write(game.Layer); // layer position 98
  Serial.write(game.Command_Type); // Command type (blank for Ad)
  Serial.write(game.Ad_CMD); // Ad Cmd (All printable ASCII)
  Serial.write(game.Ad_Number); // Ad # (All Printable ASCII)
  Serial.write(game.CMD); // Position 102 Cmd: 0-9, A-Z or blank
  Serial.write(game.Data_10s); // Data 10s: 0-9;
  Serial.write(game.Data_1s); // Data 1s: 0-9;

  for (int i = 0; i < 11; i++) {
    Serial.write(blankByte ); // [105 - 116 blank] 
  }

  Serial.write(game.Clock_Status );
  Serial.write(game.Clock_Mode );
  Serial.write(game.PlayClock_Status );

  for (int i = 0; i < 12; i++) {
    Serial.write(blankByte ); // [105 - 116 blank] 
  }
  Serial.write(game.Inbound_Channel_Number);
  for (int i = 0; i < 6; i++) {
    Serial.write(game.Bot_MSB[i]);
  }
  for (int i = 0; i < 12; i++) {
    Serial.write(game.TDateTime[i]);
  }

}


void convertTime(ScorelinkProtocolPacket game) {
  unsigned long currentMillis = millis();
  unsigned long totalSeconds = (currentMillis / 1000) % 60;
  unsigned long totalMinutes = (currentMillis / 60000) % 60;

  game.Clock_Min_Tens = totalMinutes / 10;
  game.Clock_Min_Ones = totalMinutes % 10;
  game.Clock_Sec_Tens = totalSeconds / 10;
  game.Clock_Sec_Ones = totalSeconds % 10;
}

void convertInning(ScorelinkProtocolPacket game) {
  // Innings
  // default_start.Inning_Tens       = 0;     // 1 or blank
  // default_start.Inning_Ones       = 0;     // 0-9

  // 0 2 4 6 8 10 12 14 16 18 E: 20 22= Top 1st, Top 2nd, Top 3rd, Top 4th, Top 5th, Top 6th, Top 7th, Top 8th, Top 9th, Extra: Top 10th, Top 11th
  // 1 3 5 7 9 11 13 15 17 19 E: 21 23= Bot 1st, Bot 2nd, Bot 3rd, Bot 4th, Bot 5th, Bot 6th, Bot 7th, Bot 8th, Bot 9th Extra: Bot 10th, Bot 11th
  int innings = 4;
  bool top = innings % 2 == 0;
  int properInning = 0;
  if (top) {
    properInning = innings / 2;
    game.Top_Or_Bottom = 'T';
    game.Top_of_Inning = '.';
    game.Bottom_of_Inning = blankByte;
  } else {
    properInning = (innings - 1) / 2 ;
    game.Top_Or_Bottom = 'B';
    game.Bottom_of_Inning = '.';
    game.Top_of_Inning = blankByte;

  }

  game.Inning_Tens = innings / 10;
  game.Inning_Ones = innings % 10;

}

char c;
String readString;

void setup() {
   // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(115200);


  BLEDevice::init("UMPI-Interface");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  // while (!Serial) {}
  
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2); // WORKED with the CODE below 
  // Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // invalid
  // while (!Serial2) {}
  assignBasicParamsToDefualtStart();
  BaseballGame bas;
  bas.balls = 0;
  bas.outs = 0;
  bas.innings = 0;
  bas.homeScore = 0;
  bas.awayScore = 0;
  bas.time = millis();
  strcpy(bas.hometeam, "Lehigh");
  strcpy(bas.awayTeam, "Lafayette");

  game1 = bas;

  scoreLink1 = default_start;
  // Serial.println("Fuck this");
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // String newValue = "Time since boot: " + String(millis()/1000);
    // Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    pCommandCharacteristic->registerForNotify(notifyCallback);
    pBallCharacteristic->registerForNotify(notifyCallback);
    pOutCharacteristic->registerForNotify(notifyCallback);
    pInningCharacteristic->registerForNotify(notifyCallback);
    // pHomescoreCharacteristic->registerForNotify(notifyCallback);
    // pAwayscoreCharacteristic->registerForNotify(notifyCallback);
    // pAwayteamCharacteristic->registerForNotify(notifyCallback);
    // pHometeamCharacteristic->registerForNotify(notifyCallback);
    // pTimeCharacteristic->registerForNotify(notifyCallback);
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= ScorelinkWriteInterval) {
      convertTime(scoreLink1);
      convertInning(scoreLink1);
      Serial2.println(getPacketData(scoreLink1));
      previousMillis = currentMillis;
    }
    // register the other characateristics
    // Serial.print("Outs ");
    // Serial.println(game1.outs);
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
}
