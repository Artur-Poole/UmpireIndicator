#include <TFT_eSPI.h>
#include <Arduino.h>
#include <Bounce2.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


TFT_eSPI tft = TFT_eSPI();

// Bluetooth Definitions
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define COMMAND_UUID    "9f0765fd-86aa-4354-a954-807d42a350f6"
#define BALL_UUID       "d05dc9ee-1d84-4837-a2d9-cd5654f7fc14"
#define OUT_UUID        "b4db4085-9a8b-408b-a790-b42f25935363"
#define INNING_UUID     "bdb52ad7-5b55-441b-8473-20dd29ce92f6"
#define HOMESCORE_UUID  "d679be1c-0d36-4e24-ab4b-701a2cc41184"
#define AWAYSCORE_UUID  "729b1861-7e35-450a-918d-c59c1925b8b1"
#define AWAYTEAM_UUID   "ce426594-a606-4dcb-974c-f2fa0f8d59e4"
#define HOMETEAM_UUID   "45782257-dc6f-4b56-ba4b-39d2d1b1c890"
#define TIME_UUID       "149e140e-1c7f-4fe6-bdad-ffbd779e403a"

BLECharacteristic *pCommandCharacteristic = nullptr;
BLECharacteristic *pBallCharacteristic    = nullptr;
BLECharacteristic *pOutCharacteristic = nullptr;
BLECharacteristic *pInningCharacteristic = nullptr;
BLECharacteristic *pHomescoreCharacteristic = nullptr;
BLECharacteristic *pAwayscoreCharacteristic = nullptr;
BLECharacteristic *pAwayteamCharacteristic = nullptr;
BLECharacteristic *pHometeamCharacteristic = nullptr;
BLECharacteristic *ptimeCharacteristic = nullptr;

// https://rgbcolorpicker.com/565
// Used to convert colors

// fonts to use 1, 2, 4

#define TFTC_DARKGRAY   0x10a2
#define TFTC_GREEN      0x5da4 
#define TFTC_GREEN      0x5da4 
#define TFTC_RED        0xfaab

bool SIMPLIFIED_3VARIABLES = true;

const int buttonPins[6] = { 5, 6, 7, 15, 16, 17 };
Bounce buttons[6];
bool buttonStates[6] = { false };
bool lastButtonStates[6] = { false };
unsigned long debounceDelay = 100;
unsigned long lastDebounceTime = 0;

// GAME CLOCK STUFF -- NEEDS TO BE MOVED TO STRUCT EVENTUALLY. 

const unsigned long HOUR = 3600000; // 1 hour in milliseconds 
const unsigned long MINUTE = 60000; // 1 minute in milliseconds
const unsigned long TIMER_DURATION = HOUR + (20 * MINUTE); // Total duration of timer
unsigned long startTime; // Variable to store the start time of the timer
unsigned long lastTime;

bool AUTOMATE = false;

int hx = 16; // home box x
int hy = 84; // home box y
int ax = 154; // away box x
int ay = hy; // awawy box y
int haw = 70; // width of home/away boxes
int hah = 30; // height of home/away boxes
int sx = 8;
int sy = 210;
int sw = 72;
int sh = 23;
int sd = 4;
int pushUp = 30;

int radius = 7;
int circleSpacer = (sw/8)*2.5;

void setupScreen() {
  tft.fillScreen(TFTC_DARKGRAY);


 
  int txtYDif = 84-71;
  tft.fillRect(hx, hy, haw, hah, TFTC_GREEN);
  tft.fillRect(ax, ay, haw, hah, TFTC_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFTC_DARKGRAY, true);
  tft.drawString("HOME", 51 , 71, 4); // big
  tft.drawString("AWAY", 189 , 71, 4); // big

    // tft.fillRect(hx, hy, haw, hah, TFTC_GREEN); // HOME SCORE AND BOX
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFTC_DARKGRAY, TFTC_GREEN, true);
  tft.drawString(String(0), hx + haw/2, hy + hah/2, 4); // big

  // tft.fillRect(ax, ay, haw, hah, TFTC_RED); // AWAY SCORE AND BOX
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFTC_DARKGRAY, TFTC_RED, true);  
  tft.drawString(String(0), ax + haw/2, ay + hah/2, 4); // big

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFTC_DARKGRAY, true);
  tft.drawString("1", 108 , 98, 4); // big 
  int differenceToInningText = 16;
  int innTextX = 130;
  int innTextY = 88;
  tft.drawString("top", innTextX , innTextY, 2); // small
  tft.drawString("inn", innTextX , innTextY + differenceToInningText, 2); // small
  // tft.drawString("H:MM:SS", 120 , 30, 4); // big


  
  
  int txtHeightChange = 201 - 188;
  if (!SIMPLIFIED_3VARIABLES) {
    tft.drawString("Balls", sx + sw/2, sy - txtHeightChange, 4); // big
    tft.drawString("Strikes", sx + sw + sd + sw/2, sy - txtHeightChange - pushUp, 4); // big
    tft.drawString("Outs",  sx + 2*sw + 2*sd + sw/2 , sy - txtHeightChange, 4); // bi
    for (int i =0; i < 3; i++) {
      if (i != 1) { 
        if (i != 0) { // outs
        tft.fillRect(sx + i*sw + i*sd, sy, sw, sh, TFT_BLACK);
          for (int j = -1; j < 2; j++) {
            tft.fillCircle(((sx + i*sw + i*sd) + sw/2) + j*circleSpacer, sy + sh/2, radius, TFT_BLUE);
          }
        } else { // balls
          tft.fillRect(sx + i*sw + i*sd, sy, sw + int(sw/3), sh, TFT_BLACK);
          for (int j = -1; j < 3; j++) {
            tft.fillCircle(((sx + i*sw + i*sd) + sw/2) + j*circleSpacer, sy + sh/2, radius, TFT_BLUE);
          }
        }
      } else {  // strikes
        tft.fillRect(sx + i*sw + i*sd, sy - pushUp, sw, sh, TFT_BLACK);
        for (int j = -1; j < 2; j++) {
          tft.fillCircle(((sx + i*sw + i*sd) + sw/2) + j*circleSpacer, sy + sh/2 - pushUp, radius, TFT_BLUE);
        }
      }
    }
  } else {
    // Full Inning Data
    int startInningX = 40;
    int xStep = int( (240 - 2*20) / 9);
    int startInningY = 140;
    int startInningY2 = startInningY + 25;
    tft.drawString(String("H"), startInningX/2, startInningY, 2); // big
    tft.drawString(String("A"), startInningX/2 , startInningY2, 2); // big
      // Sample runs scored by the home team in each inning
    int home_team_runs[] = {2, 0, 1, 3, 0, 2, 1, 4, 0};

    for (int i = 0; i < sizeof(home_team_runs) / sizeof(home_team_runs[0]); i++) {
      tft.drawString(String(home_team_runs[i]), startInningX + xStep * i, startInningY, 2); // big
    }

    // Sample runs scored by the away team in each inning
    int away_team_runs[] = {1, 3, 0, 0, 2, 1, 0, 2, 1};

    for (int i = 0; i < sizeof(away_team_runs) / sizeof(away_team_runs[0]); i++) {
      tft.drawString(String(away_team_runs[i]), startInningX + xStep * i, startInningY2, 2); // big
    }


    sy = 210 + 5;
    tft.drawString("Balls", sx + sw/2, sy - txtHeightChange, 4); // big
    tft.drawString(String(0), sx + sw/2, sy + txtHeightChange/1.5 + 4, 4); // big

    tft.drawString("Strikes", sx + sw + sd + sw/2, sy - txtHeightChange, 4); // big
    tft.drawString(String(0), sx + sw + sd + sw/2, sy + txtHeightChange/1.5 + 4, 4); // big

    tft.drawString("Outs",  sx + 2*sw + 2*sd + sw/2 , sy - txtHeightChange, 4); // bi
    tft.drawString(String(0),  sx + 2*sw + 2*sd + sw/2 , sy + txtHeightChange/1.5 + 4 , 4); // bi
    
  }

  
  

  // tft.drawString("HOME", 90, 60, 1); // super small
  // tft.drawString("HOME", 24, 100, 2); // less small
}



struct BaseballGame {
  int strikes;
  int balls;
  int outs;
  int innings;
  int homeScore;
  int awayScore;
  unsigned long longTime;
  char time[9]; // time string array to represent time remaining in human readable form
  char awayTeam[30];  // idk for now 6 characters in each team name
  char homeTeam[30];
  int onBat; // 0 represents home on bat, 1 represents away on bat
  int topOfInning; // 

  void initialize(const char* home, const char* away, unsigned long t) {
    strikes = 0;
    balls = 0;
    outs = 0;
    innings = 0; // 0 2 4 6 8 10 12 14 16 18 Top of Inning, 1, 2, 3
    homeScore = 0;
    awayScore = 0;
    longTime = t;
    strncpy(awayTeam, away, sizeof(awayTeam));
    strncpy(homeTeam, home, sizeof(homeTeam));
    onBat = 0;
    topOfInning = true; // Game starts with the top of the inning
  }

  void addStrike() {
    if (AUTOMATE == false) {
      if (strikes + 1 > 3) {
        strikes = 0;
        addOut();
      } else {
        Serial.println("Adding Strike - Not Automated");
        strikes = strikes + 1;
        updateScreen(0);
      }

      // send strike to ble
      // Update the BLE characteristic
      char strikesValue[2];                         // Assuming maximum of 3 digits for strikes count and a null terminator
      char sendValue[3];
      sendValue[0] = 'S';
      itoa(strikes, &sendValue[1], 10);  // Convert integer to string

      Serial.println(sendValue);
      pCommandCharacteristic->setValue(sendValue);

      // Notify connected clients about the update
      pCommandCharacteristic->notify();

    } else {
      if (strikes + 1 >= 3) {
        strikes = 0;
        addOut();
      } else {
        Serial.println("Adding Strike - Automated");
        strikes + 1;
        updateScreen(0);
      }
    }
  }

  void addBall() {
    if (AUTOMATE == false) {
      if (balls + 1 > 3) {
        balls = 0;
      } else {
        Serial.println("Adding Strike - Not Automated");
        balls = balls + 1;
        updateScreen(1);
      }
    } else {
      if (strikes + 1 >= 3) {
        strikes = 0;
        addOut();
      } else {
        Serial.println("Adding Strike - Automated");
        strikes + 1;
        updateScreen(0);
      }
    }
  }

  void addOut() {
    if (AUTOMATE == false) {
      if (outs + 1 > 3) {
        Serial.println("Here in outs");
        outs = 0;
        changeFielding();
      } else {
        Serial.println("Adding Out - Not Automated");
        outs = outs + 1;
        updateScreen(2);
      }
    }
  }

  void changeFielding() {
    // idk if automate here
    Serial.println("Changing Fielding");
    if (onBat) {
      onBat = 0;
    } else {
      onBat = 1;
    }
    outs = 0;
    balls = 0;
    strikes = 0;

    if (innings % 2 != 0) {
      topOfInning = false;
      innings = innings + 1;
    } else {
      innings = innings + 1;
    }
    Serial.println(innings);
    updateScreen(6);
    Serial.println("After update screen");
  }
  
};

BaseballGame currentGame;

struct UmpireIndicator {
  BaseballGame game;
  bool connectedToInterface;
  bool startedGame;

  void initalize(BaseballGame gme) {
    game = gme;
    connectedToInterface = false;
    startedGame = false;
  }

  void startGame() {
    startedGame = true;
  }

  // void disconnect()

  // void reconnect()

  // void saveDataToDevice()

  // void loadDataToDevice()


  void setupScreen() {
    tft.fillScreen(TFTC_DARKGRAY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFTC_DARKGRAY, true);
    tft.drawString("Umpire Indicator", 120 , 70, 4); // big v1.2 
    tft.drawString("Mike Zirinsky, Michael Connel", 120 , 86, 2); // big
    tft.drawString("v3.3 Designed by Artur Poole", 120 , 98, 1); // big
  }
  void updateScreen(char input) {
    switch (input) {
      case 'L': {
        int lBarMax = 120;
        int lBarProg = 0;

        // tft.fillRect(180, int32_t y, int32_t w, int32_t h, uint32_t color)
        break;
      }
      default: {

        break;
      }
    }
  }
};

UmpireIndicator indicator;

void updateScreen(int button) {
  char inningBuffer[2];
  int txtHeightChange = 201 - 188;
  switch (button) {
    case 0: { // updates 
      if (!SIMPLIFIED_3VARIABLES) {
        tft.fillRect(sx + sw + sd, sy - pushUp, sw, sh, TFT_BLACK); // strikes
        for (int j = -1; j < 2; j++) {
          if (j < currentGame.strikes - 1) {
            tft.fillCircle(((sx + sw + sd) + sw/2) + j*circleSpacer, sy + sh/2 - pushUp, radius, TFT_RED);
          } else {
            tft.fillCircle(((sx + sw + sd) + sw/2) + j*circleSpacer, sy + sh/2 - pushUp, radius, TFT_BLUE);
          }
        }
      } else {
        tft.drawString("Strikes", sx + sw + sd + sw/2, sy - txtHeightChange, 4); // big
        tft.drawString(String(currentGame.strikes), sx + sw + sd + sw/2, sy + txtHeightChange/1.5 + 4, 4); // big
      }
      
      break;
    } case 1: {
      if (!SIMPLIFIED_3VARIABLES) {
        tft.fillRect(sx, sy, sw, sh, TFT_BLACK);
        for (int j = -1; j < 3; j++) { // balls
          if (j < currentGame.balls - 1) {
            tft.fillCircle(((sx) + sw/2) + j*circleSpacer, sy + sh/2, radius, TFTC_RED);
          } else {
            tft.fillCircle(((sx) + sw/2) + j*circleSpacer, sy + sh/2, radius, TFT_BLUE);
          }
        }
      } else {
        tft.drawString("Balls", sx + sw/2, sy - txtHeightChange, 4); // big
        tft.drawString(String(currentGame.balls), sx + sw/2, sy + txtHeightChange/1.5 + 4, 4); // big
      }

      
      break;
    } case 2: {
      if (!SIMPLIFIED_3VARIABLES) {
        for (int j = -1; j < 2; j++) {
          if (j < currentGame.outs - 1) { // outs
            tft.fillCircle(((sx + 2*sw + 2*sd) + sw/2) + j*circleSpacer, sy + sh/2, radius, TFTC_RED);
          } else {
            tft.fillCircle(((sx + 2*sw + 2*sd) + sw/2) + j*circleSpacer, sy + sh/2, radius, TFT_BLUE);
          }
        }
      } else {
        tft.drawString("Outs",  sx + 2*sw + 2*sd + sw/2 , sy - txtHeightChange, 4); // bi
        tft.drawString(String(currentGame.outs),  sx + 2*sw + 2*sd + sw/2 , sy + txtHeightChange/1.5 + 4 , 4); // bi
      }
      break; // anything after case 5 can be used for example updating innings can send i = 6
    } case 6: {// case 6 is from baseball game struct to reset values
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_WHITE, TFTC_DARKGRAY, true);
      Serial.println("bruuh in change inning");
      int differenceToInningText = 16;
      int innTextX = 130;
      int innTextY = 88;
      String topOrBottom = "top";
      if (currentGame.innings % 2 != 0) {
        topOrBottom = "bot";
      }
      int displayInning = int((currentGame.innings / 2) + 1);
      Serial.println("Display inning");
      Serial.println(displayInning);
        // tft.drawString("1", 108 , 98, 4); // big 
      tft.drawString(String(displayInning), 108 , 98, 4); // big 
      tft.drawString(String(topOrBottom), innTextX , innTextY, 2); // small
      tft.drawString("inn", innTextX , innTextY + differenceToInningText, 2); // small
      
      break;
    } case 7: { // Time has changed
      // tft.fillRect(80, 10, 160, 40, TFTC_DARKGRAY);
      tft.drawString(String(currentGame.time), 120 , 30, 4); // big
      break;
    } default: {
      Serial.println("Not programmed");
      break;
    }
  }
}

// Function to update the value of strikes and notify clients
void updateStrikes(int newStrikes) {
// if (pCommandCharacteristic != nullptr) {
  // Make sure new strikes value is within bounds
  newStrikes = newStrikes % 4;
  if (newStrikes >= 0 && newStrikes <= 3) {
    // Update the value of strikes
    currentGame.strikes = newStrikes;
  }
    // Update the BLE characteristic
    // char strikesValue[4];                         // Assuming maximum of 3 digits for strikes count and a null terminator
    // itoa(currentGame.strikes, strikesValue, 10);  // Convert integer to string
    // pCommandCharacteristic->setValue(strikesValue);

    // Notify connected clients about the update
    // pCommandCharacteristic->notify();
  
// } else {
//   Serial.println("Error: pStrikeCharactaeristic == null");
// }
}

void updateBalls(int newBalls) {
  newBalls = newBalls % 5;
  if (newBalls >= 0 && newBalls <= 4) {
    // Update the value of strikes
    currentGame.balls = newBalls;
    Serial.println(currentGame.balls);
  }
}

void updateOuts(int newOuts) {
  newOuts = newOuts % 4;
    if (newOuts >= 0 && newOuts <= 3) {
      // Update the value of strikes
      currentGame.outs = newOuts;
    }
}

void buttonHandler() {
  for (int i = 0; i < 6; i++) {
    bool reading = digitalRead(buttonPins[i]);

    if (reading != lastButtonStates[i]) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;

        if (buttonStates[i] == HIGH) {  // Button was pressed and without noise or debounce
          // Serial.println(i);
          // printBaseballGame();

          // for button remappings set it off the defualt: Slot 0 or ButtonPin[0] is strike so in ur matching array have 0 point to something else for example balls
          // sampleMap = [1, 0, x, x, x, ] --> Can change controls this way instead of changing code. Can also have dynamic controls by having different modes which have different correspoonding functions for different button presses.

          switch (i) {
            case 0:
              // Serial.println("Hit button 0 -strike-");
              currentGame.addStrike();
              // if (buttonStates[1] == HIGH) {
              //   Serial.println("Ball or 2nd button also pressed"); // MUlti button inputs -- For setting up and advanced stuff like connecting -- should eventually change buttons to interrupts
              // }
              break;
            case 1:
              // Serial.println("Hit Button 1 -ball-");
              currentGame.addBall();
              break;
            case 2:
              // Serial.println("Hit Button 2 -Outs-");;
              currentGame.addOut();
              break;
            case 3:
              // Serial.println("Hit Button 3 -Inning-");

              break;
            case 4:
              // Serial.println("Hit Button 4 -Start-");

              break;
            case 5:
              // Serial.println("Hit Button 5 -Undo-");
              break;
            default:
              Serial.println("Bad Button");
              break;
          }
          // updateScreen(i);
        }
      }
    }
    lastButtonStates[i] = reading;
  }
}

void startScreenButtonHandler() {
  for (int i = 0; i < 6; i++) {
    bool reading = digitalRead(buttonPins[i]);

    if (reading != lastButtonStates[i]) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;

        if (buttonStates[i] == HIGH) {  // Button was pressed and without noise or debounce
          // Serial.println(i);
          // printBaseballGame();

          
          Serial.println("Someone touched a button");
          switch (i) {
            case 0:
              Serial.println("Select Current / connect");
              Serial.println("Switching screens");
              indicator.startedGame = true;
              setupScreen();
              startTime = millis();
              break;
            case 1:
              Serial.println("Cursor Down");
              break;
            case 2:
              Serial.println("Cursor Up");
              break;
            case 3:
              Serial.println("Reconnect");

              break;
            default:
              Serial.println("Not Programmed or bad button");
              break;
          }
          // updateScreen(i);
        }
      }
    }
    lastButtonStates[i] = reading;
  }
}

bool anyButtonPress() {
  for (int i = 0; i < 6; i++) {
    bool reading = digitalRead(buttonPins[i]);

    if (reading != lastButtonStates[i]) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;

        if (buttonStates[i] == HIGH) {  // Button was pressed and without noise or debounce
          return true;
        }
      }
    }
    lastButtonStates[i] = reading;
  }
  return false;
}

void setupBluetooth() {
   Serial.println("Starting Bluetooth Low Energy");

  BLEDevice::init("Umpire Indicator 1");
  BLEServer *pServer = BLEDevice::createServer();

  // Regular Charactaretstic
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  // Strikes Bluetooth Characteristic
  pCommandCharacteristic = pService->createCharacteristic(
    COMMAND_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  // Balls Bluetooth Characteristic
  pBallCharacteristic = pService->createCharacteristic(
    BALL_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // Outs Bluetooth Characteristic
  pOutCharacteristic = pService->createCharacteristic(
    OUT_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // Innings Bluetooth Characteristic
  pInningCharacteristic = pService->createCharacteristic(
    INNING_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // Homescore Bluetooth Characteristic
  pHomescoreCharacteristic = pService->createCharacteristic(
    HOMESCORE_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // AWAy SCORE Bluetooth Characteristic
  pAwayscoreCharacteristic = pService->createCharacteristic(
    AWAYSCORE_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // AWAYTEAM NAME Bluetooth Characteristic
  pAwayteamCharacteristic = pService->createCharacteristic(
    AWAYTEAM_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // Home team name Bluetooth Characteristic
  pHometeamCharacteristic = pService->createCharacteristic(
    HOMETEAM_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  
  // Time Bluetooth Characteristic
  ptimeCharacteristic = pService->createCharacteristic(
    TIME_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pService->start();
  startScan();
}

void startScan() {
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Bluetooth Advertising Started!");
}

double LOADING_TIME_MIL = 500.0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  tft.init();

  // initialize bluetooth
  setupBluetooth();

  // buton setup
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  // BaseballGame bas;  // Used chat gpt to generate library of these that we can
  // bas.balls = 0;
  // bas.strikes = 1;
  // bas.outs = 2;
  // bas.innings = 5;
  // bas.homeScore = 3;
  // bas.awayScore = 2;
  // bas.time = millis();

  // strcpy(bas.homeTeam, "Lehigh");
  // strcpy(bas.awayTeam, "Lafayette");
  // bas.topOfInning = 1;
  // bas.onBat = 0; // Lehigh home team starts at bat
  BaseballGame game;
  game.initialize("Lehigh", "Lafayette", 0);
  currentGame = game;
  UmpireIndicator ind;
  ind.initalize(game);
  indicator = ind;
  startTime = millis(); // Record the start time
  indicator.setupScreen();
}
bool pressToStart = false;
void loop() {
  if (indicator.startedGame) {
    buttonHandler();
    // Serial.println("Bruh");
    unsigned long currentTime = millis(); // Get the current time
    unsigned long elapsedTime = currentTime - startTime; // Calculate elapsed time
    // Serial.println(lastTime);
    // Serial.println(currentTime);
    // Serial.println(lastTime - currentTime);
    if (currentTime - lastTime >= 1000) {
      lastTime = currentTime;
        // Calculate remaining time
      unsigned long remainingTime = TIMER_DURATION - elapsedTime;
      
      // Convert remaining time to hours, minutes, and seconds
      unsigned long remainingHours = remainingTime / HOUR;
      unsigned long remainingMinutes = (remainingTime % HOUR) / MINUTE;
      unsigned long remainingSeconds = ((remainingTime % HOUR) % MINUTE) / 1000;

      char buffer[9]; // H:MM:SS + '\0'
      sprintf(buffer, "%lu:%02lu:%02lu", remainingHours, remainingMinutes, remainingSeconds);
      // currentGame.time = buffer
      strcpy(currentGame.time, buffer);
      updateScreen(7);
    }
  } else {
    // Serial.println("test");
    unsigned long currentTime = millis(); // Get the current time
    unsigned long elapsedTime = currentTime - startTime; // Calculate elapsed time
    // Serial.println(lastTime);
    // Serial.println(currentTime);
    // Serial.println(lastTime - currentTime);
    // Serial.println("Loading...");
    if (elapsedTime >= LOADING_TIME_MIL) {
      if (anyButtonPress()) {
        pressToStart = true;
        tft.fillRect(0, 110, 240, 240-110, TFTC_DARKGRAY);
      }
      if (pressToStart) {
        if (currentTime - lastTime >= 50) { // update this screen
          lastTime = currentTime;
          int yStart = 120;
          int yJump = 20;
          tft.setTextDatum(MC_DATUM);
          tft.setTextColor(TFT_WHITE, TFTC_DARKGRAY, true);
          tft.drawString("Umpire - Field A (Open)", 120, yStart, 2); // big
          tft.drawString("Umpire - Field B (Full)", 120,  yStart + yJump , 2); // big
          tft.drawString("Umpire - Field C (Secondary Umpire)", 120, yStart + yJump*2 , 2); // big
          tft.drawString("GameTime - Coach Donald", 120, yStart + yJump*3, 2); // big
        }
        startScreenButtonHandler();
      }
    } else if (currentTime - lastTime > 5) {
      lastTime = currentTime;
      int progress = int((elapsedTime / LOADING_TIME_MIL) * 100.0);
      int progMaxWidth = 180;
      int progWidth = int((progress/100.0) * progMaxWidth);
      int progHeight = 30;
      int ProgX = int(240/2 - progMaxWidth/2);
      int ProgY = 110;
      tft.fillRect(ProgX, ProgY, progWidth, progHeight, TFTC_GREEN);
      // Serial.println(progress);
      if (progress == 99) {
        lastTime = currentTime + 20; // So it can skip thee one or two other times its at 99
        // Serial.println("At the end of the road");
        tft.fillRect(ProgX, ProgY, progMaxWidth, progHeight, TFTC_DARKGRAY);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFTC_GREEN, TFTC_DARKGRAY, true);
        tft.drawString("Press any Button", 120, 130, 4); // big
        tft.drawString("To Continue", 120,  144 , 2); // big
      }
    }
  }

  // put your main code here, to run repeatedly:
  // tft.fillScreen(TFTC_DARKGRAY);
  // delay(1000);
  // Serial.println("Hi");
  // tft.fillScreen(TFTC_GREEN);
  // delay(1000);
}
