#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <NewPing.h>
#include "ButtonClass.h"

const int GAME_LED = 16,
  sensors[4][3] = {
    {2, 3, 4},
    {5, 6, 7},
    {8, 9, 10},
    {11, 12, 13}
  },
  MAX_ROUNDS = 100,
  DETECTION_RANGE = 12,
  MAX_DETECTION_RANGE = 100,
  AMOUNT_OF_MODES = 3;

unsigned long startGameMillis = 0, gameParams = 0, gameParamsSettings[3][2] = {{10, MAX_ROUNDS}, {30000, 180000}, {0, 0}};

long startMillis,
  endMillis,
  responseTime,
  sequence[MAX_ROUNDS],
  results[MAX_ROUNDS],
  avarage,
  fastest,
  slowest;

int selectedSensor,
  lastSelectedSensor,
  gameLength,
  gameLap = 0,
  selectedGame = -1; // 0 = REACT, 1 = SPEED, 3 = MEMORY, 4 = PURSUIT

bool detecting,
  detected,
  countingDetection,
  gameOn,
  isGameSelected = false;

Button buttonSelect(14);
Button buttonConfirm(15);
LiquidCrystal_I2C lcd(0x27, 16, 2);

NewPing sonar[4] = {
  NewPing(sensors[0][0], sensors[0][1], 100),
  NewPing(sensors[1][0], sensors[1][1], 100),
  NewPing(sensors[2][0], sensors[2][1], 100),
  NewPing(sensors[3][0], sensors[3][1], 100)
};

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
void handleDetectionStart();
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
void startRoutine();
void lcdPrint(String text, int row, bool clear);
bool senseSomething(int sensor);
bool handleSensorStartRoutine(int sensor);

void setup() {
  initLCD();
  initPins();
  startRoutine();
  initGame();
  Serial.begin(9600);
}

void loop() {
  handleTimeUp();
  handleButtonSelect();
  handleButtonConfirm();

  if (detecting == true) {
    handleDetectionStart();
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
  Serial.println("pegou");
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
    digitalWrite(GAME_LED, HIGH);
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
  Serial.println();
  Serial.println(selectedSensor);
  if (selectedGame == 1) {
    while (selectedSensor == lastSelectedSensor) {
      selectedSensor = random(4);
    };
    lastSelectedSensor = selectedSensor;
  }
  digitalWrite(sensors[selectedSensor][2], HIGH);
}

void handleDetection() {
  if (senseSomething(selectedSensor)) {
    digitalWrite(sensors[selectedSensor][2], LOW);
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
  digitalWrite(GAME_LED, LOW);
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
  gameLap = 0;
  selectedGame = -1;
  memset(results, 0, sizeof(results));
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
  digitalWrite(GAME_LED, HIGH);
  turnOnSensorLeds();
  delay(400);
  digitalWrite(GAME_LED, LOW);
  turnOffSensorLeds();
  delay(300);
  digitalWrite(GAME_LED, HIGH);
  turnOnSensorLeds();
  delay(400);
  digitalWrite(GAME_LED, LOW);
  turnOffSensorLeds();
  delay(300);
  digitalWrite(GAME_LED, HIGH);
  turnOnSensorLeds();
  delay(400 );
  digitalWrite(GAME_LED, LOW);
  turnOffSensorLeds();
}

void resetLap() {
  detected = false;
  countingDetection = false;
  detecting = true;
  gameLap++;
}

void handleDetectionStart() {
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
  digitalWrite(sensors[0][2], LOW);
  digitalWrite(sensors[1][2], LOW);
  digitalWrite(sensors[2][2], LOW);
  digitalWrite(sensors[3][2], LOW);
}

void turnOnSensorLeds() {
  digitalWrite(sensors[0][2], HIGH);
  digitalWrite(sensors[1][2], HIGH);
  digitalWrite(sensors[2][2], HIGH);
  digitalWrite(sensors[3][2], HIGH);
}

void initLCD() {
  lcd.init();
  lcd.clear();
  lcd.backlight();
}

void initPins() {
  pinMode(buttonSelect.getPin(), INPUT_PULLUP);
  pinMode(buttonConfirm.getPin(), INPUT_PULLUP);
  pinMode(sensors[0][2], OUTPUT);
  pinMode(sensors[1][2], OUTPUT);
  pinMode(sensors[2][2], OUTPUT);
  pinMode(sensors[3][2], OUTPUT);
  pinMode(GAME_LED, OUTPUT);
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
  lcdPrint("WHITE to CHANGE", 0, true);
  lcdPrint("BLUE to CHOOSE", 1, false);
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
    // if (selectedGame == AMOUNT_OF_MODES) {
    // Hiding Memory mode for now
    if (selectedGame == AMOUNT_OF_MODES - 1) {
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
  if (gameOn) {
    digitalWrite(GAME_LED, LOW);
    turnOffSensorLeds();
    resetGameProps();
  }
}

void lcdPrint(String text, int row, bool clear) {
  if (clear) {
    lcd.clear();
  }
  lcd.setCursor(0, row);
  lcd.print(text);
}

void startRoutine() {
  int report[4];
  String reportMsgRow0, reportMsgRow1, reportMsgRow1Final;
  
  lcdPrint("Starting...", 0, true);
  digitalWrite(sensors[0][2], HIGH);
  report[0] = handleSensorStartRoutine(0);

  delay(300);
  digitalWrite(sensors[0][2], LOW);
  delay(100);

  lcdPrint("Starting........", 0, true);
  report[1] = handleSensorStartRoutine(1);

  delay(300);
  digitalWrite(sensors[1][2], LOW);
  delay(100);

  lcdPrint("........", 1, false);
  digitalWrite(sensors[2][2], HIGH);
  report[2] = handleSensorStartRoutine(2);

  delay(300);
  digitalWrite(sensors[2][2], LOW);
  delay(100);

  lcdPrint("................", 1, false);
  digitalWrite(sensors[3][2], HIGH);
  report[3] = handleSensorStartRoutine(3);
  
  delay(300);
  digitalWrite(sensors[3][2], LOW);
  delay(100);

  reportMsgRow0 = "STATUS:";

  for (int i = 0; i < 4; i++) {
    if (i < 2) {
      reportMsgRow0.concat(" ");
      reportMsgRow0.concat(i);
      if (report[i] == true) {
        reportMsgRow0.concat(":B");
        continue;
      }
      reportMsgRow0.concat(":G");
      continue;
    }
    reportMsgRow1.concat(" ");
    reportMsgRow1.concat(i);
    if (report[i] == true) {
      reportMsgRow1.concat(":B");
      continue;
    }
    reportMsgRow1.concat(":G");
  }

  lcdPrint(reportMsgRow0, 0, true);
  reportMsgRow1Final = reportMsgRow1;
  reportMsgRow1Final.concat("    (5)");
  lcdPrint(reportMsgRow1Final, 1, false);
  delay(1000);
  reportMsgRow1Final = reportMsgRow1;
  reportMsgRow1Final.concat("    (4)");
  lcdPrint(reportMsgRow1Final, 1, false);
  delay(1000);
  reportMsgRow1Final = reportMsgRow1;
  reportMsgRow1Final.concat("    (3)");
  lcdPrint(reportMsgRow1Final, 1, false);
  delay(1000);
  reportMsgRow1Final = reportMsgRow1;
  reportMsgRow1Final.concat("    (2)");
  lcdPrint(reportMsgRow1Final, 1, false);
  delay(1000);
  reportMsgRow1Final = reportMsgRow1;
  reportMsgRow1Final.concat("    (1)");
  lcdPrint(reportMsgRow1Final, 1, false);
  delay(1000);
}

bool senseSomething(int sensor) {
  int distance = sonar[sensor].ping() / US_ROUNDTRIP_CM;
  return (distance > 0 && distance < DETECTION_RANGE);
};

bool handleSensorStartRoutine(int sensor) {
  long startedAt = millis();
  bool moveOn = false, error = false;
  digitalWrite(sensors[sensor][2], HIGH);
  while (moveOn == false) {
    moveOn = senseSomething(sensor);
    if (millis() - startedAt > 15000) {
      moveOn = true;
      error = true;
    }
  }
  return error;
}
