
/*
 * Smart Attendance System using ESP32 and RFID
 * 
 * COMPONENTS:
 * - ESP32 DevKit v1: Main microcontroller
 * - RC522 RFID Reader: Reads MIFARE RFID cards/tags (13.56MHz)
 * - LCD 1602 I2C: 16x2 character display for student name and status
 * - Green LED: Indicates successful attendance marking
 * - Red LED: Indicates access denied
 * - Piezo Buzzer: Audio feedback for attendance events
 * - 2x 220Ω Resistors: Current limiting for LEDs
 * 
 * PIN ASSIGNMENTS:
 * RC522 RFID Module (SPI):
 *   - VCC    → ESP32 3V3 (3.3V power)
 *   - GND    → ESP32 GND
 *   - MOSI   → ESP32 D23 (GPIO23)
 *   - MISO   → ESP32 D19 (GPIO19)
 *   - SCK    → ESP32 D18 (GPIO18)
 *   - SDA/SS → ESP32 D5  (GPIO5, chip select)
 *   - RST    → ESP32 D4  (GPIO4, reset pin)
 * 
 * LCD 1602 I2C:
 *   - VCC → ESP32 3V3
 *   - GND → ESP32 GND
 *   - SDA → ESP32 D21 (GPIO21)
 *   - SCL → ESP32 D22 (GPIO22)
 * 
 * Green LED:
 *   - Anode  → ESP32 D25 (GPIO25)
 *   - Cathode → 220Ω resistor → GND
 * 
 * Red LED:
 *   - Anode  → ESP32 D26 (GPIO26)
 *   - Cathode → 220Ω resistor → GND
 * 
 * Buzzer:
 *   - Pin 1 (signal) → ESP32 D27 (GPIO27)
 *   - Pin 2 (GND)    → ESP32 GND
 * 
 * BEHAVIOR:
 * 1. System waits for RFID card to be tapped on the RC522 reader.
 * 2. When a card is detected, the UID is read and compared against the database.
 * 3. If UID matches a registered student:
 *    - Display student name on LCD line 1
 *    - Display "Attendance Marked" on LCD line 2
 *    - Turn ON green LED for 2 seconds
 *    - Buzzer beeps once (1000Hz for 200ms)
 * 4. If UID is not registered:
 *    - Display "Access Denied" on LCD
 *    - Turn ON red LED for 2 seconds
 *    - Buzzer beeps twice (1000Hz, 200ms on, 100ms off, 200ms on)
 * 5. After 2 seconds, LEDs turn off and LCD clears, ready for next card.
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin definitions for RC522 RFID module
#define SS_PIN    5   // SDA/SS pin (chip select) on GPIO5
#define RST_PIN   4   // RST pin on GPIO4

// Pin definitions for LEDs
#define GREEN_LED 25  // Green LED on GPIO25
#define RED_LED   26  // Red LED on GPIO26

// Pin definition for buzzer
#define BUZZER    27  // Buzzer signal pin on GPIO27

// Create MFRC522 instance
MFRC522 rfid(SS_PIN, RST_PIN);

// Create LCD instance (I2C address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Student database: UID to Name mapping
// Each UID is 4 bytes (for MIFARE Classic 1K cards)
// Replace these sample UIDs with your actual card UIDs
struct Student {
  byte uid[4];
  String name;
};

// Sample student records
// To get your card's UID, tap it and check the Serial Monitor
Student students[] = {
  {{0xAA, 0xBB, 0xCC, 0xDD}, "Rahul"},   // Sample UID for Rahul
  {{0x11, 0x22, 0x33, 0x44}, "Aditya"},  // Sample UID for Aditya
  {{0x55, 0x66, 0x77, 0x88}, "Yogi"}     // Sample UID for Yogi
};

const int numStudents = sizeof(students) / sizeof(students[0]);

// Timer variables for LED control (non-blocking)
unsigned long ledOffAt = 0;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  Serial.println("Smart Attendance System Initializing...");
  
  // Initialize SPI bus for RC522
  SPI.begin();
  
  // Initialize RC522 RFID reader
  rfid.PCD_Init();
  Serial.println("RC522 RFID Reader initialized.");
  
  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Attendance Sys");
  lcd.setCursor(0, 1);
  lcd.print("Ready...");
  Serial.println("LCD initialized.");
  
  // Configure LED pins as outputs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  // Configure buzzer pin as output
  pinMode(BUZZER, OUTPUT);
  
  Serial.println("System ready. Tap your RFID card...");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap Your Card");
}

void loop() {
  // Non-blocking LED timer: turn off LEDs after the display period
  if (ledOffAt != 0 && millis() >= ledOffAt) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    ledOffAt = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tap Your Card");
  }
  
  // Check if a new RFID card is present
  if (!rfid.PICC_IsNewCardPresent()) {
    return; // No card detected, continue loop
  }
  
  // Attempt to read the card's UID
  if (!rfid.PICC_ReadCardSerial()) {
    return; // Failed to read card, continue loop
  }
  
  // Card detected and UID read successfully
  Serial.print("Card detected! UID: ");
  printUID(rfid.uid.uidByte, rfid.uid.size);
  
  // Search for the UID in the student database
  int studentIndex = findStudent(rfid.uid.uidByte, rfid.uid.size);
  
  if (studentIndex >= 0) {
    // UID found: registered student
    String studentName = students[studentIndex].name;
    Serial.print("Access granted for: ");
    Serial.println(studentName);
    
    // Display student name and attendance status on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(studentName);
    lcd.setCursor(0, 1);
    lcd.print("Attendance OK");
    
    // Turn on green LED for 2 seconds
    digitalWrite(GREEN_LED, HIGH);
    ledOffAt = millis() + 2000;
    
    // Buzzer beeps once (1000Hz for 200ms)
    tone(BUZZER, 1000, 200);
    
  } else {
    // UID not found: unregistered card
    Serial.println("Access denied: UID not registered.");
    
    // Display "Access Denied" on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Unknown Card");
    
    // Turn on red LED for 2 seconds
    digitalWrite(RED_LED, HIGH);
    ledOffAt = millis() + 2000;
    
    // Buzzer beeps twice (1000Hz, 200ms on, 100ms off, 200ms on)
    tone(BUZZER, 1000, 200);
    delay(300); // 200ms beep + 100ms silence
    tone(BUZZER, 1000, 200);
  }
  
  // Halt PICC (stop communication with the card)
  rfid.PICC_HaltA();
  
  // Stop encryption on PCD (the reader)
  rfid.PCD_StopCrypto1();
}

// Helper function: search for a UID in the student database
// Returns the index of the student if found, or -1 if not found
int findStudent(byte *uid, byte uidSize) {
  for (int i = 0; i < numStudents; i++) {
    // Compare the first 4 bytes of the UID
    if (memcmp(uid, students[i].uid, 4) == 0) {
      return i; // Match found
    }
  }
  return -1; // No match
}

// Helper function: print UID to Serial Monitor in hex format
void printUID(byte *uid, byte uidSize) {
  for (byte i = 0; i < uidSize; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
  Serial.println();
}
