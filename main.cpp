#include "mbed.h"

// Shift register pins
DigitalOut latchPin(D4);  // ST_CP
DigitalOut clockPin(D7);  // SH_CP
DigitalOut dataPin(D8);   // DS

// Buttons (active LOW)
DigitalIn s1(A1), s2(A2), s3(A3);

// Potentiometer for voltage measurement
AnalogIn potentiometer(A0);  // Potentiometer connected to A0

// Common ANODE patterns (INVERTED from common cathode)
const uint8_t digitPattern[10] = {
    static_cast<uint8_t>(~0x3F), // 0 → 0xC0
    static_cast<uint8_t>(~0x06), // 1 → 0xF9
    static_cast<uint8_t>(~0x5B), // 2 → 0xA4
    static_cast<uint8_t>(~0x4F), // 3 → 0xB0
    static_cast<uint8_t>(~0x66), // 4 → 0x99
    static_cast<uint8_t>(~0x6D), // 5 → 0x92
    static_cast<uint8_t>(~0x7D), // 6 → 0x82
    static_cast<uint8_t>(~0x07), // 7 → 0xF8
    static_cast<uint8_t>(~0x7F), // 8 → 0x80
    static_cast<uint8_t>(~0x6F)  // 9 → 0x90
};

// Digit positions (left to right, 1=enable)
const uint8_t digitPos[4] = {
    0x01, // Leftmost digit (00000001)
    0x02, // Second digit (00000010)
    0x04, // Third digit (00000100)
    0x08  // Rightmost digit (00001000)
};

volatile int seconds = 0, minutes = 0;
Ticker timerTicker;

void updateTime() {
    seconds++;
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 100) minutes = 0;
    }
}

void shiftOutMSBFirst(uint8_t value) {
    for (int i = 7; i >= 0; i--) {
        dataPin = (value & (1 << i)) ? 1 : 0;
        clockPin = 1;
        clockPin = 0;
    }
}

void writeToShiftRegister(uint8_t segments, uint8_t digit) {
    latchPin = 0;
    shiftOutMSBFirst(segments);  // Send segments first
    shiftOutMSBFirst(digit);     // Then digit position
    latchPin = 1;
}

// Display a number with optional decimal point
void displayNumber(int number, bool showDecimalPoint = false, int decimalPosition = 1) {
    int digits[4] = {
        (number / 1000) % 10,
        (number / 100) % 10,
        (number / 10) % 10,
        number % 10
    };
    
    for (int i = 0; i < 4; i++) {
        uint8_t segments = digitPattern[digits[i]];
        
        // Add decimal point if needed (common anode: 0 = ON, 1 = OFF)
        // Assuming decimal point is bit 7 (0x80) in the segment pattern
        if (showDecimalPoint && i == decimalPosition) {
            segments &= ~0x80;  // Turn on decimal point (clear bit 7)
        }
        
        writeToShiftRegister(segments, digitPos[i]);
        ThisThread::sleep_for(2ms);
    }
}

int main() {
    // Initialize pins
    latchPin = 0;
    clockPin = 0;
    dataPin = 0;
    
    // Configure button inputs with pull-ups
    s1.mode(PullUp);
    s2.mode(PullUp);
    s3.mode(PullUp);
    
    // Start the timer
    timerTicker.attach(&updateTime, 1.0f);
    
    while(1) {
        // Reset clock if S1 is pressed
        if (!s1) {
            seconds = minutes = 0;
            ThisThread::sleep_for(200ms);  // Debounce
        }
        
        // Display voltage when S3 is pressed, otherwise display time
        if (!s3) {
            // Read potentiometer voltage (0-1 float representing 0-3.3V)
            float voltage = potentiometer.read_u16() / 65535.0f * 3.3f;
            
            // Convert to integer value (e.g., 2.75V becomes 2750)
            int voltageValue = (int)(voltage * 1000);
            
            // Display with decimal point after first digit (e.g., "2.750")
            displayNumber(voltageValue, true, 0);
        } else {
            // Display time (MM:SS) with decimal point after minutes as a separator
            int timeValue = minutes * 100 + seconds;
            displayNumber(timeValue, true, 1);
        }
    }
  }