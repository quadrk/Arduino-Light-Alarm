#include <LiquidCrystal.h> // library for LCD

// ---------------------- LCD pin settings ----------------------
LiquidCrystal lcd(2, 3, 4, 5, 6, 1);
// (rs=2, e=3, d4=4, d5=5, d6=6, d7=1)

// ---------------------- Bar Graph (inversed) ---------------------
int barPins[10] = {8, 9, 10, 11, A1, A2, A3, A4, A5, 7};

// ---------------------- Other pins -------------------------------
const int ldrPin     = A0;   // Fororesistor
const int buttonPin  = 13;   // Push button
const int buzzerPin  = 12;   // Buzzer

// if "brightness" > ALARM_THRESHOLD and alarmStatus == AWAY
// starts 5 sec enteringDelay, then alarm
const int ALARM_THRESHOLD = 800; 

// test
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 250;

// Alarm modes
enum AlarmMode {
  OFF,  // Alarm is OFF
  AWAY  // Alarm is ON (after 5 sec leaving)
};
AlarmMode alarmStatus = OFF;

// Flags and timers
bool alarmActive = false;     // If true, alarm is active
bool leavingDelay = false;    // 5 sec on leaving (when the button in OFF -> AWAY is pressed)
bool enteringDelay = false;   // 5s to enter (when it became light)

unsigned long leavingStartTime = 0;   // time of start leavingDelay
unsigned long enteringStartTime = 0;  // time of start enteringDelay

// --------------------------------------------------------------------
//                      F U N C T I O N S
// --------------------------------------------------------------------

// Start 5s leaving delay
void startLeavingDelay() {
  leavingDelay = true;
  leavingStartTime = millis();
}

// Process "leaving" (leavingDelay) — every second without beep
void handleLeavingDelay() {
  unsigned long elapsed = (millis() - leavingStartTime) / 1000;
  if (elapsed < 5) {
    // editable - 5s, without "beeps"
    static unsigned long lastSec = 0;
    if (elapsed != lastSec) {
      lastSec = elapsed;
      beepOnce();  // short beep
    }
  } else {
    // 5s passed, the persone is "away", the alarm is fully active
    leavingDelay = false;
  }
}

// Start 5s entering delay
void startEnteringDelay() {
  enteringDelay = true;
  enteringStartTime = millis();
}

// Process "entry"
void handleEnteringDelay() {
  unsigned long elapsed = (millis() - enteringStartTime) / 1000;
  if (elapsed < 5) {
    // Wait 5s, without "beeps"
  } else {
    // 5s passed, status is AWAY == ALARM ON
    if (alarmStatus == AWAY) {
      alarmActive = true;
    }
    enteringDelay = false;
  }
}

// Short beep (100ms)
void beepOnce() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

// Alarm siren (200ms on, 200ms off)
void alarmSiren() {
  static unsigned long lastToggle = 0;
  static bool buzzerState = false;
  if (millis() - lastToggle >= 200) {
    lastToggle = millis();
    buzzerState = !buzzerState;
    digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);
  }
}

// Bar Graph (Inversed fixed) - better cabling
void updateBarGraph(int brightness) {
  // If brightness=300 (dark) -> level 0
  // If brightness=1000 (bright) -> level 10
  int level = map(brightness, 300, 1000, 0, 10);
  if (level < 0)   level = 0;
  if (level > 10)  level = 10;

  for (int i = 0; i < 10; i++) {
    if (i < level) {
      digitalWrite(barPins[9 - i], HIGH);
    } else {
      digitalWrite(barPins[9 - i], LOW);
    }
  }
}

// Displaying information on LCD (reprint the second line again) [fixed bugs]
void displayInfoOnLCD(int brightness) {
  // First line: Light=xxx
  lcd.setCursor(0, 0);
  lcd.print("Light=");
  lcd.print(brightness);
  lcd.print("    "); // clr the rest

  // Generate the text for the second line
  // First clear the entire second line
  lcd.setCursor(0, 1);
  lcd.print("                "); // 16 spaces -- clr

  // Then print the actual text
  lcd.setCursor(0, 1);

  if (leavingDelay) {
    // "Leaving: Xs"
    unsigned long elapsed = (millis() - leavingStartTime) / 1000;
    int remain = 5 - elapsed;
    if (remain < 0) remain = 0;
    lcd.print("Leaving: ");
    lcd.print(remain);
    lcd.print("s");
    return;
  }
  if (enteringDelay) {
    // "Enter: Xs"
    unsigned long elapsed = (millis() - enteringStartTime) / 1000;
    int remain = 5 - elapsed;
    if (remain < 0) remain = 0;
    lcd.print("Enter: ");
    lcd.print(remain);
    lcd.print("s");
    return;
  }
  // If alarm
  if (alarmActive) {
    lcd.print("ALARM!!!");
    return;
  }
  // Else print the status
  if (alarmStatus == OFF) {
    lcd.print("Alarm=OFF");
  } else {
    lcd.print("Alarm=AWAY");
  }
}

// Check the button
void checkButton() {
  if (millis() - lastButtonPress < debounceDelay) return;
  if (digitalRead(buttonPin) == LOW) {
    lastButtonPress = millis();

    if (alarmStatus == OFF) {
      // Going away with delay
      alarmStatus = AWAY;
      startLeavingDelay();
    } else {
      // Elsee OFF
      alarmStatus = OFF;
      // Reset all flags
      leavingDelay = false;
      enteringDelay = false;
      alarmActive = false;
    }
  }
}

// --------------------------------------------------------------------
//                            S E T U P   &   L O O P
// --------------------------------------------------------------------
void setup() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("System Start...");

  for (int i = 0; i < 10; i++) {
    pinMode(barPins[i], OUTPUT);
    digitalWrite(barPins[i], LOW);
  }

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pinMode(buttonPin, INPUT_PULLUP);

  delay(1000);
  lcd.clear();
}

void loop() {
  // 1) Read the LDR (correcting the “inversion”)
  int rawVal = analogRead(ldrPin);
  int brightness = map(1023 - rawVal, 0, 1023, 300, 1000);

  // 2) Updating Bar Graph
  updateBarGraph(brightness);

  // 3) Leaving
  if (leavingDelay) handleLeavingDelay();

  // 4) Entering (enteringDelay) - now without “peaks”
  if (enteringDelay) handleEnteringDelay();

  // 5) Check the alarm status
  if (alarmStatus == OFF) {
    // OFF
    alarmActive = false;
    enteringDelay = false;
  } else {
    // AWAY
    // If there is no leavingDelay, there is no enteringDelay and the alarm is not yet active
    if (!leavingDelay && !enteringDelay && !alarmActive) {
      // Checking to see if it's too bright
      if (brightness > ALARM_THRESHOLD) {
        // Run enteringDelay, but WITHOUT peaks
        startEnteringDelay();
      }
    }
  }

  // 6) If alarmActive, start the siren
  if (alarmActive) {
    alarmSiren();
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  // 7)  Putting everything on the screen
  displayInfoOnLCD(brightness);

  // 8) Button
  checkButton();

  delay(100);
}
