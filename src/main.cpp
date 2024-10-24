#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "ButtonClass.h"

const int gameLed = 12,
          sensors[4][2] = {
            {4, 5},
            {6, 7},
            {8, 9},
            {10, 11}
          };

Button buttonSelect(2);
Button buttonConfirm(3);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void handleTimeUp();
void handleStartGame();
void handleDetected();
void sortSensor();
void handleDetection();
void handleLapResult();
void handleEndGame();
void handleEndGameResults();
void blinkGameLed();
void resetLap();
void handleDetectionTimeCounting();
void printResultsSerial();
void printResultsLCD();
void turnOffSensorLeds();
void turnOnSensorLeds();
void initLCD();
void initPins();
void initGame();
void handleButtonSelect();
void handleButtonConfirm();
void printGameParams();
void printGameOption();
void handleGameReset();
void resetGameProps();
void lcdPrint(String text, int row, bool clear);

const int MAX_ROUNDS = 100;
long startMillis,
  endMillis,
  responseTime,
  sequence[MAX_ROUNDS],
  results[MAX_ROUNDS],
  avarage,
  fastest,
  slowest;
unsigned long startGameMillis = 0, gameParams = 0, gameParamsSettings[3][2] = {{10, MAX_ROUNDS}, {30000, 180000}, {0, 0}};
int selectedSensor,
  lastSelectedSensor,
  gameLength,
  gameLap = 0,
  totGames = 3,
  selectedGame = -1; // 0 = REACT, 1 = SPEED, 3 = MEMORY
bool detecting, detected, countingDetection, gameOn, isGameSelected = false;

void setup() {
  initLCD();
  initPins();
  initGame();
  Serial.begin(9600);
}

void loop() {
  handleTimeUp();
  handleButtonSelect();
  handleButtonConfirm();

  if (detecting == true) {
    handleDetectionTimeCounting();
    handleDetection();
    return;
  }

  if (detected == true) {
    handleDetected();
    return;
  }

  if (gameOn == true && gameLap == 0) {
    handleStartGame();
  }
}

void handleDetected() {
  resetLap();
  if (selectedGame == 0) {
    if (gameLap == gameLength) {
      handleEndGame();
      return;
    }
    delay(random(500, 3500));
  }
}

void handleStartGame() {
    delay(3000);
    blinkGameLed();
    delay(500);
    digitalWrite(gameLed, HIGH);
    lcdPrint("    GAME ON!    ", 0, true);
    if (selectedGame == 1) {
      startGameMillis = millis();
    }
    detecting = true;
}

void handleTimeUp() {
  if (selectedGame == 1 && startGameMillis > 0 && millis() - startGameMillis > gameParams) {
    handleEndGame();
  }
}

void sortSensor() {
  selectedSensor = random(4);
  if (selectedGame == 1) {
    while (selectedSensor == lastSelectedSensor) {
      selectedSensor = random(4);
    };
    lastSelectedSensor = selectedSensor;
  }
  digitalWrite(sensors[selectedSensor][1], HIGH);
}

void handleDetection() {
  if (!digitalRead(sensors[selectedSensor][0])) {
    digitalWrite(sensors[selectedSensor][1], LOW);
    handleLapResult();
    detected = true;
    detecting = false;
  }
}

void handleLapResult() {
  endMillis = millis();
  if (selectedGame == 0) {
    responseTime = endMillis - startMillis;
    results[gameLap] = responseTime;
  }
}

void handleEndGame() {
  digitalWrite(gameLed, LOW);
  delay(200);
  lcdPrint("   GAME OVER    ", 0, true);
  blinkGameLed();
  handleEndGameResults();
  printResultsLCD();
  printResultsSerial();
  resetGameProps();
}

void resetGameProps() {
  gameOn = false;
  detecting = false;
  detected = false;
  slowest = 0;
  fastest = 0;
  avarage = 0;
  startGameMillis = 0;
  countingDetection = false;
  startMillis = 0;
  endMillis = 0;
  isGameSelected = false;
  selectedGame = -1;
}

void handleEndGameResults() {
  if (selectedGame == 0) {
    long totalResponseTime = 0;
    for (int i = 0; i < gameLap; i++) {
      long result = results[i];
      if (i == 0) {
        slowest = result;
        fastest = result;
      }
      if (result > slowest) {
        slowest = result;
      }
      if (result < fastest) {
        fastest = result;
      }
      totalResponseTime = totalResponseTime + result;
    }
    avarage = totalResponseTime / gameLap;
  }
}

void blinkGameLed() {
  digitalWrite(gameLed, HIGH);
  turnOnSensorLeds();
  delay(400);
  digitalWrite(gameLed, LOW);
  turnOffSensorLeds();
  delay(300);
  digitalWrite(gameLed, HIGH);
  turnOnSensorLeds();
  delay(400);
  digitalWrite(gameLed, LOW);
  turnOffSensorLeds();
  delay(300);
  digitalWrite(gameLed, HIGH);
  turnOnSensorLeds();
  delay(400 );
  digitalWrite(gameLed, LOW);
  turnOffSensorLeds();
}

void resetLap() {
  detected = false;
  countingDetection = false;
  detecting = true;
  gameLap++;
}

void handleDetectionTimeCounting() {
  if (countingDetection == false) {
    sortSensor();
    startMillis = millis();
    countingDetection = true;
  }
}

void printResultsSerial() {
  if (selectedGame == 0) {
    Serial.println();
    Serial.println();
    Serial.println("*************************");
    Serial.println("**    FINAL RESULTS    **");
    Serial.println("*************************");
    Serial.println();
    for (int i = 0; i < gameLap; i++) {
      Serial.print(i + 1);
      Serial.print(" - ");
      Serial.print(results[i]);
      Serial.println("ms");
    }
    Serial.println();
    Serial.print("avarage - ");
    Serial.print(avarage);
    Serial.println("ms");
    Serial.println();
    Serial.print("slowest - ");
    Serial.print(slowest);
    Serial.println("ms");
    Serial.print("fastest - ");
    Serial.print(fastest);
    Serial.println("ms");
    Serial.println();
    Serial.println("*************************");
    Serial.println();
    Serial.println();
  }
}

void printResultsLCD() {
  String lcdRow0, lcdRow1;
  switch (selectedGame) {
    case 0:
      lcdRow0 = "Media: ";
      lcdRow0.concat(avarage);
      lcdRow0.concat("ms");
      lcdRow1 = "-: ";
      lcdRow1.concat(fastest);
      lcdRow1.concat("  +: ");
      lcdRow1.concat(slowest);
      break;
    case 1:
      lcdRow0 = "Time: ";
      lcdRow0.concat(gameParams / 1000);
      lcdRow0.concat(" secs    ");
      lcdRow1 = "Total: ";
      lcdRow1.concat(gameLap);
      lcdRow1.concat(" hits   ");
      break;
    case 2:
      lcdRow0 = "SPEED ln 1";
      lcdRow1 = "SPEED ln 1";
  }
  lcdPrint(lcdRow0, 0, true);
  lcdPrint(lcdRow1, 1, false);
}

void turnOffSensorLeds() {
  digitalWrite(sensors[0][1], LOW);
  digitalWrite(sensors[1][1], LOW);
  digitalWrite(sensors[2][1], LOW);
  digitalWrite(sensors[3][1], LOW);
}

void turnOnSensorLeds() {
  digitalWrite(sensors[0][1], HIGH);
  digitalWrite(sensors[1][1], HIGH);
  digitalWrite(sensors[2][1], HIGH);
  digitalWrite(sensors[3][1], HIGH);
}

void initLCD() {
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcdPrint("  Press SELECT  ", 0, false);
  lcdPrint("    to start    ", 1, false);
}

void initPins() {
  pinMode(buttonSelect.getPin(), INPUT_PULLUP);
  pinMode(buttonConfirm.getPin(), INPUT_PULLUP);
  pinMode(sensors[0][0], INPUT);
  pinMode(sensors[0][1], OUTPUT);
  pinMode(sensors[1][0], INPUT);
  pinMode(sensors[1][1], OUTPUT);
  pinMode(sensors[2][0], INPUT);
  pinMode(sensors[2][1], OUTPUT);
  pinMode(sensors[3][0], INPUT);
  pinMode(sensors[3][1], OUTPUT);
  pinMode(gameLed, OUTPUT);
}

void initGame() {
  size_t const address {0};
  unsigned int seed {};
  EEPROM.get(address, seed);
  randomSeed(seed);
  EEPROM.put(address, seed + 1);
  detecting = false;
  detected = false;
  countingDetection = false;
  gameOn = false;
}

void handleButtonSelect() {
  buttonSelect.read();
  if (buttonSelect.wasPressed()) {
    handleGameReset();
    if (isGameSelected) {
      if (gameParams >= gameParamsSettings[selectedGame][1]) {
        gameParams = gameParamsSettings[selectedGame][0];
      } else {
        gameParams = gameParams + gameParamsSettings[selectedGame][0];
      }
      printGameParams();
      return;
    }
    selectedGame++;
    if (selectedGame == totGames) {
      selectedGame = 0;
    }
    lcdPrint("  Select mode:  ", 0, true);
    printGameOption();
  }
}

void handleButtonConfirm() {
  if (!gameOn) {
    buttonConfirm.read();
    if (buttonConfirm.wasPressed()) {
      if (isGameSelected) {
        String txt;
        gameLength = gameParams;
        isGameSelected = false;
        gameOn = true;
        if (selectedGame == 1) {
          txt = "SPEED ";
          txt.concat(gameParams / 1000);
          txt.concat(" secs:  ");
        } else {
          txt = "REACT ";
          txt.concat(gameParams);
          txt.concat(" rounds:");
        }
        lcdPrint(txt, 0, false);
        lcdPrint("3 blinks n start", 1, false);
        return;
      }
      isGameSelected = true;
      switch (selectedGame) {
        case 0:
          lcdPrint("Mode: REACT", 0, true);
          break;
        case 1:
          lcdPrint("Mode: SPEED", 0, true);
          break;
        case 2:
          isGameSelected = false;
          gameOn = true;
          return;
      }
      gameParams = gameParamsSettings[selectedGame][0];
      printGameParams();
    }
  }
}

void printGameParams() {
  lcd.setCursor(0, 1);
  String txt;
  if (selectedGame == 1) {
    txt = "set time > ";
    txt.concat(gameParams / 1000);
    txt.concat(" s ");
    lcdPrint(txt, 1, false);
    return;
  }
  txt = "set rounds > ";
  txt.concat(gameParams);
  txt.concat(" ");
  lcdPrint(txt, 1, false);
}

void printGameOption() {
  switch (selectedGame) {
    case  0:
      lcdPrint("---> REACT <----", 1, false);
      return;
    case 1:
      lcdPrint("---> SPEED <----", 1, false);
      return;
    case 2:
      lcdPrint("---> MEMORY <---", 1, false);
      return;
  }
  
}

void handleGameReset() {
    gameOn = false;
    turnOffSensorLeds();
    detecting = false;
    slowest = 0;
    fastest = 0;
    avarage = 0;
    memset(results, 0, sizeof(results));
    gameLap = 0;
}

void lcdPrint(String text, int row, bool clear) {
  if (clear) {
    lcd.clear();
  }
  lcd.setCursor(0, row);
  lcd.print(text);
}
