#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <NewPing.h>
#include "ButtonClass.h"

const int AMOUNT_OF_SENSORS = 4,
  GAME_LED = 16,
  MAX_ROUNDS = 100,
  DETECTION_RANGE = 12,
  MAX_DETECTION_RANGE_CM = 100,
  AMOUNT_OF_MODES = 2;

const int SENSORS[AMOUNT_OF_SENSORS][3] = {
  {2, 3, 4},
  {5, 6, 7},
  {8, 9, 10},
  {11, 12, 13}
};

unsigned long startModeMillis = 0,
  modeParams = 0,
  modeParamsSettings[AMOUNT_OF_MODES][2] = {
    {30000, 180000},
    {10, MAX_ROUNDS}
  };

unsigned int lastDisplayedTime = 0;

long startMillis,
  responseTime,
  results[MAX_ROUNDS],
  avarage,
  fastest,
  slowest;

int selectedSensor,
  lastSelectedSensor,
  modeLength,
  gameLap,
  selectedMode = -1; // 0 = SPEED, 1 = REACT

bool detecting,
  detected,
  countingDetection,
  gameOn,
  isModeSelected;

String sensorReportMsg[2] = {"", ""};

Button buttonSelect(14);
Button buttonConfirm(15);
LiquidCrystal_I2C lcd(0x27, 16, 2);

NewPing sonars[AMOUNT_OF_SENSORS] = {
  NewPing(SENSORS[0][0], SENSORS[0][1], MAX_DETECTION_RANGE_CM),
  NewPing(SENSORS[1][0], SENSORS[1][1], MAX_DETECTION_RANGE_CM),
  NewPing(SENSORS[2][0], SENSORS[2][1], MAX_DETECTION_RANGE_CM),
  NewPing(SENSORS[3][0], SENSORS[3][1], MAX_DETECTION_RANGE_CM)
};

void handleTimeUp();
void handleStartMode();
void handleDetected();
void sortSensor();
void handleDetection();
void handleLapResult();
void handleEndMode();
void handleEndModeResults();
void blinkModeLed();
void resetLap();
void handleDetectionStart();
void printResultsSerial();
void printResultsLCD();
void turnOffSensorLeds();
void turnOnSensorLeds();
void initLCD();
void initPins();
void initRandomSeed();
void handleButtonSelect();
void handleButtonConfirm();
void printModeParams();
void printModeOption();
void handleModeReset();
void resetModeProps();
void checkSensors();
void printSensorsReport();
void printMainMenu();
void handleDisplayInfo();
void lcdPrint(String text, int row, bool clear);
bool senseSomething(int sensor);
bool handleSensorStartRoutine(int sensor);

void setup() {
  initLCD();
  initPins();
  initRandomSeed();
  checkSensors();
  // printSensorsReport();
  resetModeProps();
  printMainMenu();
  // Serial.begin(9600);
}

void loop() {
  if (gameOn && selectedMode == 0) {
    handleDisplayInfo();
    handleTimeUp();
  }

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
    handleStartMode();
  }
}

void handleDetected() {
  resetLap();
  if (selectedMode == 1) {
    if (gameLap == modeLength) {
      handleEndMode();
      return;
    }
    delay(random(500, 3500));
  }
}

void handleStartMode() {
    delay(3000);
    blinkModeLed();
    delay(500);
    digitalWrite(GAME_LED, HIGH);
    lcdPrint("    GAME ON!    ", 0, true);
    if (selectedMode == 0) {
      startModeMillis = millis();
    }
    detecting = true;
}

void handleTimeUp() {
  if (selectedMode == 0 && startModeMillis > 0 && millis() - startModeMillis > modeParams) {
    handleEndMode();
  }
}

void sortSensor() {
  selectedSensor = random(AMOUNT_OF_SENSORS);
  Serial.println();
  Serial.println(selectedSensor);
  if (selectedMode == 0) {
    while (selectedSensor == lastSelectedSensor) {
      selectedSensor = random(AMOUNT_OF_SENSORS);
    };
    lastSelectedSensor = selectedSensor;
  }
  digitalWrite(SENSORS[selectedSensor][2], HIGH);
}

void handleDetection() {
  if (senseSomething(selectedSensor)) {
    digitalWrite(SENSORS[selectedSensor][2], LOW);
    handleLapResult();
    detected = true;
    detecting = false;
  }
}

void handleLapResult() {
  if (selectedMode == 1) {
    responseTime = millis() - startMillis;
    results[gameLap] = responseTime;
  }
}

void handleEndMode() {
  digitalWrite(GAME_LED, LOW);
  delay(200);
  lcdPrint("   GAME OVER    ", 0, true);
  blinkModeLed();
  handleEndModeResults();
  printResultsLCD();
  // printResultsSerial();
  resetModeProps();
}

void resetModeProps() {
  gameOn = false;
  detecting = false;
  detected = false;
  slowest = 0;
  fastest = 0;
  avarage = 0;
  startModeMillis = 0;
  countingDetection = false;
  startMillis = 0;
  isModeSelected = false;
  gameLap = 0;
  selectedMode = -1;
  memset(results, 0, sizeof(results));
}

void handleEndModeResults() {
  if (selectedMode == 1) {
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

void blinkModeLed() {
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
  if (selectedMode == 1) {
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
  switch (selectedMode) {
    case 0:
      lcdRow0 = "Time: ";
      lcdRow0.concat(modeParams / 1000);
      lcdRow0.concat(" secs    ");
      lcdRow1 = "Total: ";
      lcdRow1.concat(gameLap);
      lcdRow1.concat(" hits   ");
      break;
    case 1:
      lcdRow0 = "Media: ";
      lcdRow0.concat(avarage);
      lcdRow0.concat("ms");
      lcdRow1 = "-: ";
      lcdRow1.concat(fastest);
      lcdRow1.concat("  +: ");
      lcdRow1.concat(slowest);
      break;
    case 2:
      lcdRow0 = "SPEED ln 1";
      lcdRow1 = "SPEED ln 1";
  }
  lcdPrint(lcdRow0, 0, true);
  lcdPrint(lcdRow1, 1, false);
}

void turnOffSensorLeds() {
  digitalWrite(SENSORS[0][2], LOW);
  digitalWrite(SENSORS[1][2], LOW);
  digitalWrite(SENSORS[2][2], LOW);
  digitalWrite(SENSORS[3][2], LOW);
}

void turnOnSensorLeds() {
  digitalWrite(SENSORS[0][2], HIGH);
  digitalWrite(SENSORS[1][2], HIGH);
  digitalWrite(SENSORS[2][2], HIGH);
  digitalWrite(SENSORS[3][2], HIGH);
}

void initLCD() {
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcdPrint("   Starting...  ", 0, true);
}

void initPins() {
  pinMode(buttonSelect.getPin(), INPUT_PULLUP);
  pinMode(buttonConfirm.getPin(), INPUT_PULLUP);
  pinMode(GAME_LED, OUTPUT);
  for (int i = 0; i < AMOUNT_OF_SENSORS; i++) {
    pinMode(SENSORS[i][2], OUTPUT);
  }
}

void initRandomSeed() {
  size_t const address {0};
  unsigned int seed {};
  EEPROM.get(address, seed);
  randomSeed(seed);
  EEPROM.put(address, seed + 1);
}

void handleButtonSelect() {
  buttonSelect.read();
  if (buttonSelect.wasPressed()) {
    if (gameOn) {
      handleModeReset();
      printMainMenu();
      return;
    }
    if (isModeSelected) {
      if (modeParams >= modeParamsSettings[selectedMode][1]) {
        modeParams = modeParamsSettings[selectedMode][0];
      } else {
        modeParams = modeParams + modeParamsSettings[selectedMode][0];
      }
      printModeParams();
      return;
    }
    selectedMode++;
    if (selectedMode == AMOUNT_OF_MODES) {
      selectedMode = 0;
    }
    lcdPrint("  Select mode:  ", 0, true);
    printModeOption();
  }
}

void handleButtonConfirm() {
  if (!gameOn) {
    buttonConfirm.read();
    if (buttonConfirm.wasPressed()) {
      if (isModeSelected) {
        String txt;
        modeLength = modeParams;
        isModeSelected = false;
        gameOn = true;
        if (selectedMode == 0) {
          txt = "SPEED ";
          txt.concat(modeParams / 1000);
          txt.concat(" secs:  ");
        } else {
          txt = "REACT ";
          txt.concat(modeParams);
          txt.concat(" rounds:");
        }
        lcdPrint(txt, 0, false);
        lcdPrint("3 blinks n start", 1, false);
        return;
      }
      isModeSelected = true;
      lcdPrint((selectedMode == 0) ? "Mode: SPEED" : "Mode: REACT", 0, true);
      modeParams = modeParamsSettings[selectedMode][0];
      printModeParams();
    }
  }
}

void printModeParams() {
  lcd.setCursor(0, 1);
  String txt;
  if (selectedMode == 0) {
    txt = "set time > ";
    txt.concat(modeParams / 1000);
    txt.concat(" s ");
    lcdPrint(txt, 1, false);
    return;
  }
  txt = "set rounds > ";
  txt.concat(modeParams);
  txt.concat(" ");
  lcdPrint(txt, 1, false);
}

void printModeOption() {
  if (selectedMode == 0) {
    lcdPrint("---> SPEED <----", 1, false);
    return;
  }
  lcdPrint("---> REACT <----", 1, false);
}

void handleModeReset() {
  if (gameOn) {
    digitalWrite(GAME_LED, LOW);
    turnOffSensorLeds();
    resetModeProps();
  }
}

void lcdPrint(String text, int row, bool clear) {
  if (clear) {
    lcd.clear();
  }
  lcd.setCursor(0, row);
  lcd.print(text);
}

void checkSensors() {
  int currentRow = 0;
  String reportMsgRow1Final;
  sensorReportMsg[0] = "STATUS:";
  for (int i = 0; i < AMOUNT_OF_SENSORS; i++) {
    if (i < 2) {
      currentRow = 0;
    } else {
      currentRow = 1;
    }
    sensorReportMsg[currentRow].concat(" ");
    sensorReportMsg[currentRow].concat(i);
    digitalWrite(SENSORS[i][2], HIGH);
    if (handleSensorStartRoutine(i) == true) {
      sensorReportMsg[currentRow].concat(i);
      sensorReportMsg[currentRow].concat(":B");
    } else {
      sensorReportMsg[currentRow].concat(":G");
    }
    digitalWrite(SENSORS[i][2], LOW);
    delay(100);
  }
}

bool senseSomething(int sensor) {
  int distance = sonars[sensor].ping() / US_ROUNDTRIP_CM;
  return (distance > 0 && distance < DETECTION_RANGE);
};

bool handleSensorStartRoutine(int sensor) {
  long startedAt = millis();
  bool moveOn = false, error = false;
  digitalWrite(SENSORS[sensor][2], HIGH);
  while (moveOn == false) {
    moveOn = senseSomething(sensor);
    if (millis() - startedAt > 15000) {
      moveOn = true;
      error = true;
    }
  }
  return error;
}

void printSensorsReport() {
  String reportMsgRow1Final;
  for (int i = 5; i > 0; i--) {
    lcdPrint(sensorReportMsg[0], 0, false);
    reportMsgRow1Final = sensorReportMsg[1];
    reportMsgRow1Final.concat("    (");
    reportMsgRow1Final.concat(i);
    reportMsgRow1Final.concat(")");
    lcdPrint(reportMsgRow1Final, 1, false);
    delay(1000);
  }
}

void printMainMenu() {
  lcdPrint("WHITE to CHANGE", 0, true);
  lcdPrint("BLUE to CHOOSE", 1, false);
}

void handleDisplayInfo() {
  String time;
  unsigned int timeDiff;
  time = "Hits: ";
  time.concat(gameLap);
  time.concat(" - ");
  if (lastDisplayedTime == 0) {
    lastDisplayedTime = modeParams / 1000;
    time.concat(lastDisplayedTime);
  } else {
    timeDiff = millis() - startModeMillis;
    timeDiff = timeDiff / 1000;
    if (timeDiff != lastDisplayedTime) {
      lastDisplayedTime = modeParams / 1000 - timeDiff;
      time.concat(lastDisplayedTime);
    }
  }
  time.concat("s    ");
  lcdPrint(time, 1, false);
}
