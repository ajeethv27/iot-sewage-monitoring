/*
 * IoT Sewage Monitoring System: sensor node (Arduino Uno)
 * B.E. project, Kumaraguru College of Technology, 2021
 *
 * Reads flow, gas, leakage and water level, shows them on a 16x2 LCD,
 * drives the pump relay from the level reading and streams a framed
 * packet to the ESP8266 gateway over the hardware UART.
 *
 * Packet format (13 bytes, ASCII):  '*' FFF GGG WWW LLL
 *   FFF = total flow [L], GGG = gas [%], WWW = leakage [%], LLL = level [%]
 *   each value zero-padded to three digits.
 */
#include <LiquidCrystal.h>

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

const int PIN_WATER = A0;   // leakage sensor
const int PIN_LEVEL = A1;   // water-level sensor
const int PIN_GAS   = A2;   // MQ-2 gas sensor (via LM741 comparator stage)
const int PIN_PUMP  = A3;   // relay driver -> pump

// Flow sensor on INT0 (digital pin 2)
const byte FLOW_INTERRUPT = 0;
const float CALIBRATION_FACTOR = 4.5;   // pulses per second per L/min

volatile byte pulseCount = 0;
float flowRate = 0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;
unsigned long oldTime = 0;

int gas, water, level;

void sendData();

void pulseCounter() { pulseCount++; }

// Print a value as three digits, e.g. 7 -> "007"
void print3(Print &out, unsigned long v) {
  if (v <= 9)       { out.print("00"); }
  else if (v <= 99) { out.print("0");  }
  out.print(v);
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(PIN_WATER, INPUT_PULLUP);
  pinMode(PIN_LEVEL, INPUT_PULLUP);
  pinMode(PIN_GAS, INPUT);
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, HIGH);

  attachInterrupt(FLOW_INTERRUPT, pulseCounter, FALLING);

  lcd.setCursor(1, 0); lcd.print("SEWAGE");
  lcd.setCursor(1, 1); lcd.print("MONITORING");
  delay(2000);
  lcd.clear();
}

void loop() {
  // --- Flow: integrate once per second ---------------------------------
  if ((millis() - oldTime) > 1000) {
    detachInterrupt(FLOW_INTERRUPT);
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / CALIBRATION_FACTOR;  // L/min
    oldTime = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;

    lcd.setCursor(0, 0); lcd.print("F:"); print3(lcd, totalMilliLitres / 1000);

    pulseCount = 0;
    attachInterrupt(FLOW_INTERRUPT, pulseCounter, FALLING);
  }

  // --- Gas ----------------------------------------------------------------
  gas = map(analogRead(PIN_GAS), 0, 1023, 0, 100);
  lcd.setCursor(0, 1); lcd.print("G:"); print3(lcd, gas);

  // --- Leakage (sensor output is inverted) ----------------------------------
  water = map(analogRead(PIN_WATER), 0, 1023, 100, 0);
  lcd.setCursor(8, 1); lcd.print("W:"); print3(lcd, water);

  // --- Level + automatic pump control ----------------------------------------
  level = map(analogRead(PIN_LEVEL), 0, 1023, 0, 100);
  lcd.setCursor(8, 0); lcd.print("L:"); print3(lcd, level);
  digitalWrite(PIN_PUMP, level <= 50 ? LOW : HIGH);

  sendData();
  delay(500);
}

void sendData() {
  Serial.print('*');
  print3(Serial, totalMilliLitres / 1000);
  print3(Serial, gas);
  print3(Serial, water);
  print3(Serial, level);
}
