/*
  RFID + PIN Door Lock  (Final)
  -----------------------------
  Card UID : 0x53, 0x51, 0xCA, 0x56
  PIN      : MIEA  (press S4, S8, S12, S16)
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Servo.h>

#define SS_PIN    10
#define RST_PIN    9
#define SERVO_PIN  3
#define GREEN_LED  6
#define RED_LED    7

MFRC522 rfid(SS_PIN, RST_PIN);
Servo lockServo;

const int LOCKED_ANGLE   = 0;
const int UNLOCKED_ANGLE = 90;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'A','B','C','D'},
  {'E','F','G','H'},
  {'I','J','K','L'},
  {'M','N','O','P'}
};
byte rowPins[ROWS] = {2, 4, 5, 8};
byte colPins[COLS] = {A3, A2, A1, A0};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const String CORRECT_PIN = "MIEA";

byte authorizedUIDs[][4] = {
  {0x53, 0x51, 0xCA, 0x56}
};
const int NUM_AUTHORIZED = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

String enteredPIN = "";
bool cardVerified = false;
unsigned long cardVerifiedAt = 0;
const unsigned long CARD_TIMEOUT = 30000;

int failedAttempts = 0;
unsigned long lockoutUntil = 0;
const int MAX_ATTEMPTS = 3;
const unsigned long LOCKOUT_MS = 30000;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  lockServo.attach(SERVO_PIN);
  lockServo.write(LOCKED_ANGLE);

  Serial.println(F("=== RFID + PIN Lock ready ==="));
  Serial.println(F("Tap your card."));
}

void loop() {
  if (millis() < lockoutUntil) return;

  if (cardVerified && millis() - cardVerifiedAt > CARD_TIMEOUT) {
    cardVerified = false;
    enteredPIN = "";
    Serial.println(F("Card verification timed out."));
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (isAuthorizedCard()) {
      cardVerified = true;
      cardVerifiedAt = millis();
      enteredPIN = "";
      Serial.println(F("Card OK. Enter PIN (4 buttons)."));
    } else {
      Serial.println(F("Unknown card. Access denied."));
      denyAccess();
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  char key = keypad.getKey();
  if (key) {
    enteredPIN += key;
    Serial.print(F("*"));

    if (enteredPIN.length() >= CORRECT_PIN.length()) {
      handlePINEntry();
    }
  }
}

bool isAuthorizedCard() {
  for (int i = 0; i < NUM_AUTHORIZED; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (rfid.uid.uidByte[j] != authorizedUIDs[i][j]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}

void handlePINEntry() {
  Serial.println();
  if (cardVerified && enteredPIN == CORRECT_PIN) {
    Serial.println(F("Access granted."));
    grantAccess();
    failedAttempts = 0;
  } else {
    Serial.println(F("Access denied."));
    denyAccess();
    failedAttempts++;
    if (failedAttempts >= MAX_ATTEMPTS) {
      Serial.println(F("Too many attempts. Locked out for 30s."));
      lockoutUntil = millis() + LOCKOUT_MS;
      failedAttempts = 0;
    }
  }
  enteredPIN = "";
  cardVerified = false;
}

void grantAccess() {
  digitalWrite(GREEN_LED, HIGH);
  lockServo.write(UNLOCKED_ANGLE);
  delay(3000);
  lockServo.write(LOCKED_ANGLE);
  digitalWrite(GREEN_LED, LOW);
}

void denyAccess() {
  digitalWrite(RED_LED, HIGH);
  delay(1000);
  digitalWrite(RED_LED, LOW);
}