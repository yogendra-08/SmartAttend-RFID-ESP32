The ESP32 continuously polls the RC522 RFID reader over SPI. When a card is tapped, the firmware reads its UID and searches a hardcoded student database (Rahul, Aditya, Yogi with sample UIDs). On a match, the LCD displays the student's name and "Attendance OK," the green LED lights for 2 seconds, and the buzzer beeps once at 1000Hz for 200ms. For an unregistered UID, the LCD shows "Access Denied," the red LED lights for 2 seconds, and the buzzer beeps twice (200ms on, 100ms off, 200ms on). A non-blocking timer (millis) controls the 2-second LED display period so the loop remains responsive. After each scan, the system halts the card and clears the display, ready for the next tap. To use with real cards, tap each card once, note its UID from the Serial Monitor, and update the students array with the actual 4-byte UIDs.

# Build Guide

1. Connect ESP32
2. Connect RC522 RFID
3. Connect LCD
4. Connect LEDs
5. Connect Buzzer
6. Upload Arduino Code
7. Scan RFID Card
8. Verify Attendance Output