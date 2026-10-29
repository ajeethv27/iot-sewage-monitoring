/*
 * IoT Sewage Monitoring System: Wi-Fi gateway (NodeMCU / ESP8266)
 * B.E. project, Kumaraguru College of Technology, 2021
 *
 * Receives the framed packet from the Arduino Uno over UART, publishes the
 * four readings to Blynk virtual pins V0..V3 and raises push notifications
 * when a threshold is crossed.
 *
 * Written for the legacy Blynk platform (library 0.6.x, 2021). Porting to
 * Blynk IoT needs BLYNK_TEMPLATE_ID / BLYNK_TEMPLATE_NAME and
 * Blynk.logEvent() in place of Blynk.notify().
 */
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// ---- Fill in before flashing (never commit real credentials) ----
char auth[] = "YOUR_BLYNK_AUTH_TOKEN";
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

const unsigned int PACKET_LEN = 13;   // '*' + 4 x 3 digits
unsigned char buf[PACKET_LEN];
unsigned int idx = 0;
unsigned int flow = 0, gas = 0, water = 0, level = 0;

unsigned int digits3(const unsigned char *p) {
  return (p[0] - '0') * 100 + (p[1] - '0') * 10 + (p[2] - '0');
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
}

void loop() {
  // Re-synchronise on the '*' start byte and collect one packet
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '*') { idx = 0; }
    if (idx < PACKET_LEN) { buf[idx++] = c; }
  }

  if (idx == PACKET_LEN && buf[0] == '*') {
    flow  = digits3(&buf[1]);
    gas   = digits3(&buf[4]);
    water = digits3(&buf[7]);
    level = digits3(&buf[10]);
    idx = 0;
  }

  Blynk.virtualWrite(V0, flow);  delay(100);
  Blynk.virtualWrite(V1, gas);   delay(100);
  if (gas >= 50)   { Blynk.notify("GAS LEVEL EXCEEDED"); }
  Blynk.virtualWrite(V2, water); delay(100);
  if (water >= 50) { Blynk.notify("WATER LEAKAGE DETECTED"); }
  Blynk.virtualWrite(V3, level); delay(100);
  if (level <= 50) { Blynk.notify("WATER LEVEL LOW"); }
  delay(100);

  Blynk.run();
}
