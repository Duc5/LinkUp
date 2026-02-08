// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>
// #include <WiFiClient.h>
// #include <ArduinoJson.h>

// const char* WIFI_SSID = "HangoutNet";
// const char* WIFI_PASS = "hangout123";

// // Safer than URL parsing:
// const char* HUB_HOST = "192.168.4.1";
// const uint16_t HUB_PORT = 80;
// const char* HUB_PATH = "/event";

// const char* SENDER_ID = "SENDER_B";

// unsigned long lastSendMs = 0;

// void printWifiStatus() {
//   Serial.print("WiFi.status="); Serial.println(WiFi.status()); // 3 = connected
//   Serial.print("IP="); Serial.println(WiFi.localIP());
//   Serial.print("GW="); Serial.println(WiFi.gatewayIP());
//   Serial.print("DNS="); Serial.println(WiFi.dnsIP());
//   Serial.print("RSSI="); Serial.println(WiFi.RSSI());
// }

// bool tcpProbe() {
//   WiFiClient c;
//   Serial.print("TCP probe to "); Serial.print(HUB_HOST); Serial.print(":"); Serial.print(HUB_PORT); Serial.print(" ... ");
//   bool ok = c.connect(HUB_HOST, HUB_PORT);
//   Serial.println(ok ? "OK" : "FAIL");
//   c.stop();
//   return ok;
// }

// bool postEvent(const char* location, int time_offset_h) {
//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("Not connected, skip POST");
//     return false;
//   }

//   // Quick connectivity check
//   if (!tcpProbe()) return false;

//   WiFiClient client;
//   HTTPClient http;

//   http.setTimeout(4000);

//   if (!http.begin(client, HUB_HOST, HUB_PORT, HUB_PATH)) {
//     Serial.println("http.begin failed");
//     return false;
//   }

//   http.addHeader("Content-Type", "application/json");

//   StaticJsonDocument<200> doc;
//   doc["location"] = location;
//   doc["time_offset_h"] = time_offset_h;
//   doc["sender_id"] = SENDER_ID;

//   String body;
//   serializeJson(doc, body);

//   Serial.print("POST body: "); Serial.println(body);

//   int code = http.POST(body);
//   String resp = http.getString();
//   http.end();

//   Serial.print("HTTP code="); Serial.print(code);
//   Serial.print(" resp="); Serial.println(resp);

//   return (code == 200);
// }

// void connectWifi() {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASS);

//   Serial.print("Connecting to "); Serial.println(WIFI_SSID);

//   unsigned long start = millis();
//   while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
//     delay(300);
//     Serial.print(".");
//   }
//   Serial.println();

//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("Connected ✅");
//     printWifiStatus();
//   } else {
//     Serial.println("WiFi connect failed ❌");
//     printWifiStatus();
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   delay(200);
//   connectWifi();
// }

// void loop() {
//   if (WiFi.status() != WL_CONNECTED) {
//     // retry occasionally
//     static unsigned long lastRetry = 0;
//     if (millis() - lastRetry > 5000) {
//       lastRetry = millis();
//       Serial.println("Retry WiFi...");
//       connectWifi();
//     }
//     return;
//   }

//   if (millis() - lastSendMs > 5000) { // every 5 seconds
//     lastSendMs = millis();
//     Serial.println("Sending event...");
//     bool ok = postEvent("LIBRARY", 2);
//     Serial.println(ok ? "Sent ✅" : "Send failed ❌");
//   }
// }
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// OLED Display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// === BUTTON PINS ===
const int BUTTON1_PIN = 0; // GPIO0 - FLASH button
const int BUTTON2_PIN = 2; // GPIO2 - D4 pin

// Button tracking
unsigned long lastButton1Press = 0;
unsigned long lastButton2Press = 0;
const unsigned long BUTTON_COOLDOWN = 600; // ms between presses
const unsigned long BOTH_HOLD_MS = 2; // how long to hold before showing XP
unsigned long bothHoldStartMs = 0;
bool bothHoldArmed = false;

// ===== SYSTEM STATES =====
enum SystemState {
EYE_MODE, // Default mode - showing eye expressions
EVENT_TIME_SETTING, // Setting event time (hours)
EVENT_LOCATION, // Choosing location
EVENT_CONFIRMATION, // Confirming event details
EVENT_CREATED, // Event created successfully
XP_VIEW
};

// ===== WIFI / HUB SETTINGS =====
const char* WIFI_SSID = "HangoutNet";
const char* WIFI_PASS = "hangout123";
const char* HUB_HOST  = "192.168.4.1";
const uint16_t HUB_PORT = 80;
const char* HUB_PATH = "/event";
const char* SENDER_ID = "Alex";

// Connect to HangoutNet (STA)
bool connectToHubWifi(unsigned long timeoutMs = 15000) {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);     // helps stability
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(200);
  }

  return (WiFi.status() == WL_CONNECTED);
}

// POST the event to Hub A
bool postEventToHub(const char* location, int time_offset_h) {
  if (!connectToHubWifi()) return false;

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(10000); // prevent -11 timeouts

  // Safer than a full URL string on ESP8266
  if (!http.begin(client, HUB_HOST, HUB_PORT, HUB_PATH)) {
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["location"] = location;
  doc["time_offset_h"] = time_offset_h;
  doc["sender_id"] = SENDER_ID;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.print("POST code="); Serial.print(code);
  Serial.print(" resp="); Serial.println(resp);

  return (code == 200);
}
void displaySending(const char* location, int hrs) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 8);
  display.print("Sending to Hub...");
  display.setCursor(0, 26);
  display.print("Loc: ");
  display.print(location);
  display.setCursor(0, 42);
  display.print("In: ");
  display.print(hrs);
  display.print("h");
  display.display();
}

void displaySendResult(bool ok) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 22);
  display.print(ok ? "SENT!" : "FAILED");
  display.display();
}


SystemState currentState = EYE_MODE;
SystemState prevState = EYE_MODE;

// ===== EVENT PLANNING VARIABLES =====
int eventHours = 0; // Hours until event (0-23)
int selectedLocation = 0; // Current location index

// Location options
const char* locations[] = {
"Kitchen",
"Hallway",
"Room",
"SU",
"Canteen"
};
const int LOCATION_COUNT = 5;

// ===== GAMIFICATION =====
int socialXP = 0;                 // 0..100
const int XP_PER_EVENT_OK = 10;   // fill amount when event sent successfully
const int XP_PER_EVENT_FAIL = 3;  // optional
unsigned long lastDecayMs = 0;
const unsigned long DECAY_PERIOD_MS = 60UL * 60UL * 1000UL; // 1 hour
const int DECAY_AMOUNT = 5;

bool xpViewActive() {
  return currentState == XP_VIEW;
}

// ===== EYE SYSTEM VARIABLES =====
// Eye positions
const int LEFT_EYE_X = 31;
const int RIGHT_EYE_X = 95;
const int EYE_Y = 24;
const int EYE_WIDTH = 32;
const int PUPIL_SIZE = 8;

// Eye expressions
enum EyeExpression {
NEUTRAL,
HAPPY,
SAD,
ANGRY,
SLEEPY,
SURPRISED,
CURIOUS,
LOVE,
WINKING,
BLINKING,
TOTAL_EXPRESSIONS
};

EyeExpression currentExpression = NEUTRAL;
EyeExpression previousExpression = NEUTRAL;

// Blinking
bool isBlinking = false;
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 2000;
const unsigned long BLINK_DURATION = 250;

// Expression names
const char* expressionNames[] = {
"Neutral", "Happy", "Sad", "Angry", "Sleepy",
"Surprised", "Curious", "Love", "Winking", "Blinking"
};

// ===== BUTTON FUNCTIONS =====
bool checkButtonPress(int pin, unsigned long &lastPressTime) {
if (digitalRead(pin) == LOW) {
if (millis() - lastPressTime > BUTTON_COOLDOWN) {
lastPressTime = millis();
return true;
}
}
return false;
}

void handleBothButtonsHold() {
  bool b1 = (digitalRead(BUTTON1_PIN) == LOW) && (currentState == EYE_MODE || currentState == XP_VIEW);
  bool b2 = true;

  unsigned long now = millis();

  if (b1 && b2) {
    if (!bothHoldArmed) {
      bothHoldArmed = true;
      bothHoldStartMs = now;
    }

    // if held long enough, enter XP view
    if (bothHoldArmed && (now - bothHoldStartMs >= BOTH_HOLD_MS)) {
      if (currentState != XP_VIEW) {
        prevState = currentState;
        currentState = XP_VIEW;
        displayXPStatus();
      } else {
        // keep refreshing while held (optional)
        displayXPStatus();
      }
      return;
    }
  } else {
    // released: if we were in XP_VIEW, return to previous screen
    bothHoldArmed = false;

    if (currentState == XP_VIEW) {
      currentState = prevState;

      // redraw the previous state's screen
      switch (currentState) {
        case EYE_MODE: drawEyes(); break;
        case EVENT_TIME_SETTING: displayEventTimeSetting(); break;
        case EVENT_LOCATION: displayLocationSelection(); break;
        case EVENT_CONFIRMATION: displayEventConfirmation(); break;
        case EVENT_CREATED: displayEventCreated(); break;
        default: drawEyes(); break;
      }
    }
  }
}

// ===== EYE DRAWING FUNCTIONS =====
void drawEyebrow(int centerX, int centerY, bool isAngry) {
int yOffset = isAngry ? -18 : -12;
int curve = isAngry ? -3 : 3;
display.drawLine(centerX - 12, centerY + yOffset,
centerX, centerY + yOffset + curve, BLACK);
display.drawLine(centerX, centerY + yOffset + curve,
centerX + 12, centerY + yOffset, BLACK);
}

void drawEye(int centerX, int centerY, EyeExpression expression, bool isLeftEye = true) {
display.fillCircle(centerX, centerY, EYE_WIDTH/2, WHITE);
display.drawCircle(centerX, centerY, EYE_WIDTH/2, BLACK);
int pupilX = centerX;
int pupilY = centerY;
switch(expression) {
case NEUTRAL: break;
case HAPPY:
pupilY = centerY - 3;
for (int i = -8; i <= 8; i += 4) {
display.drawPixel(centerX + i, centerY - 10, BLACK);
}
break;
case SAD:
pupilY = centerY + 4;
drawEyebrow(centerX, centerY, false);
break;
case ANGRY:
pupilX = centerX + (isLeftEye ? 3 : -3);
pupilY = centerY - 2;
drawEyebrow(centerX, centerY, true);
break;
case SLEEPY:
display.drawLine(centerX - EYE_WIDTH/2, centerY - 3,
centerX + EYE_WIDTH/2, centerY - 3, BLACK);
display.fillCircle(centerX, centerY, PUPIL_SIZE - 2, BLACK);
return;
case SURPRISED:
display.drawCircle(centerX, centerY, EYE_WIDTH/2 + 2, BLACK);
display.fillCircle(centerX, centerY, PUPIL_SIZE - 3, BLACK);
return;
case CURIOUS:
pupilX = centerX + (isLeftEye ? 6 : -6);
break;
case LOVE:
if (isLeftEye) {
display.fillCircle(centerX - 3, centerY - 3, 4, BLACK);
display.fillCircle(centerX + 3, centerY - 3, 4, BLACK);
display.fillTriangle(centerX - 7, centerY - 1,
centerX + 7, centerY - 1,
centerX, centerY + 8, BLACK);
} else {
display.fillCircle(centerX - 3, centerY - 3, 4, BLACK);
display.fillCircle(centerX + 3, centerY - 3, 4, BLACK);
display.fillTriangle(centerX - 7, centerY - 1,
centerX + 7, centerY - 1,
centerX, centerY + 8, BLACK);
}
return;
case WINKING:
if (isLeftEye) {
display.drawLine(centerX - EYE_WIDTH/2, centerY,
centerX + EYE_WIDTH/2, centerY, BLACK);
return;
} else {
pupilX = centerX - 4;
}
break;
case BLINKING:
display.drawLine(centerX - EYE_WIDTH/2, centerY,
centerX + EYE_WIDTH/2, centerY, BLACK);
return;
}
display.fillCircle(pupilX, pupilY, PUPIL_SIZE, BLACK);
display.fillCircle(pupilX - 2, pupilY - 2, 2, WHITE);
}

// ===== DISPLAY FUNCTIONS =====
void drawEyes() {
display.clearDisplay();
drawEye(LEFT_EYE_X, EYE_Y, currentExpression, true);
drawEye(RIGHT_EYE_X, EYE_Y, currentExpression, false);
display.setTextSize(1);
display.setTextColor(BLACK, WHITE);
display.setCursor(40, 55);
// display.print(expressionNames[currentExpression]);
// drawProgressBar(10, 58, 108, 6, socialXP);

display.display();
}
void drawProgressBar(int x, int y, int w, int h, int percent) {
  percent = constrain(percent, 0, 100);
  display.drawRect(x, y, w, h, WHITE);
  int fillW = (w - 2) * percent / 100;
  display.fillRect(x + 1, y + 1, fillW, h - 2, WHITE);
}

void displayEventTimeSetting() {
display.clearDisplay();
// Title
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(20, 5);
display.print("Set Event Time");
// Time display (00:00 format)
display.setTextSize(2);
display.setCursor(40, 25);
// Format hours as 2 digits
if (eventHours < 10) {
display.print("0");
}
display.print(eventHours);
display.print(":00");
// Instructions
display.setTextSize(1);
display.setCursor(10, 50);
display.print("Btn1:+1hr Btn2:OK");
// Social XP bar at bottom
// drawProgressBar(10, 58, 108, 6, socialXP);

display.display();
}

void displayLocationSelection() {
display.clearDisplay();
// Title
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(30, 5);
display.print("Select Location");
// Current location
display.setTextSize(2);
display.setCursor(20, 25);
display.print(locations[selectedLocation]);
// Instructions
display.setTextSize(1);
display.setCursor(10, 50);
display.print("Btn1:Next Btn2:OK");
display.display();
}

void displayEventConfirmation() {
display.clearDisplay();
// Title
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(25, 5);
display.print("Confirm Event");
// Time
display.setTextSize(1);
display.setCursor(10, 20);
display.print("Time: ");
display.print(eventHours);
display.print(" hours");
// Location
display.setCursor(10, 35);
display.print("Location: ");
display.print(locations[selectedLocation]);
// Instructions
display.setCursor(10, 50);
display.print("Btn1:Cancel Btn2:OK");
display.setCursor(10, 50);
display.print("Btn1:Cancel Btn2:OK");

// XP bar at very bottom
// drawProgressBar(10, 58, 108, 6, socialXP);

display.display();
}

void displayEventCreated() {
display.clearDisplay();
// Success message
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(20, 20);
display.print("Event Created!");
display.setCursor(15, 35);
display.print("Notifying others...");
// Instructions
display.setCursor(30, 50);
display.print("Press any button");
display.display();
// drawProgressBar(10, 58, 108, 6, socialXP);

}
void displayXPStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 6);
  display.print("SOCIAL XP");

  // percentage text
  display.setTextSize(2);
  display.setCursor(0, 22);
  display.print(socialXP);
  display.print("%");

  // progress bar
  drawProgressBar(10, 54, 108, 8, socialXP);

  // hint
  display.setTextSize(1);
  display.setCursor(0, 46);
  // display.print("Hold both buttons");

  display.display();
}


// ===== EVENT PLANNING FUNCTIONS =====
void startEventCreation() {
Serial.println("Starting event creation...");
currentState = EVENT_TIME_SETTING;
eventHours = 0; // Reset to 0 hours
displayEventTimeSetting();
}

void handleTimeSetting() {
// Button 1: Increase hours
if (digitalRead(BUTTON1_PIN) == LOW) {
if (millis() - lastButton1Press > BUTTON_COOLDOWN) {
lastButton1Press = millis();
eventHours = (eventHours + 1) % 24; // Cycle 0-23
Serial.print("Event hours: ");
Serial.println(eventHours);
displayEventTimeSetting();
}
}
// Button 2: Confirm time, move to location
if (digitalRead(BUTTON2_PIN) == LOW) {
if (millis() - lastButton2Press > BUTTON_COOLDOWN) {
lastButton2Press = millis();
Serial.println("Time confirmed, moving to location selection");
currentState = EVENT_LOCATION;
selectedLocation = 0; // Start with first location
displayLocationSelection();
}
}
}

void handleLocationSelection() {
// Button 1: Cycle locations
if (digitalRead(BUTTON1_PIN) == LOW) {
if (millis() - lastButton1Press > BUTTON_COOLDOWN) {
lastButton1Press = millis();
selectedLocation = (selectedLocation + 1) % LOCATION_COUNT;
Serial.print("Selected location: ");
Serial.println(locations[selectedLocation]);
displayLocationSelection();
}
}
// Button 2: Confirm location, move to confirmation
if (digitalRead(BUTTON2_PIN) == LOW) {
if (millis() - lastButton2Press > BUTTON_COOLDOWN) {
lastButton2Press = millis();
Serial.println("Location confirmed, moving to final confirmation");
currentState = EVENT_CONFIRMATION;
displayEventConfirmation();
}
}
}

void handleEventConfirmation() {
  // Button 1: Confirm and create event
  if (digitalRead(BUTTON2_PIN) == LOW) {
    if (millis() - lastButton2Press > BUTTON_COOLDOWN) {
      lastButton2Press = millis();
      Serial.println("Event confirmed! Posting to Hub...");

      const char* loc = locations[selectedLocation];

      displaySending(loc, eventHours);
      bool ok = postEventToHub(loc, eventHours);
      if (ok) socialXP = min(100, socialXP + XP_PER_EVENT_OK);
      else    socialXP = min(100, socialXP + XP_PER_EVENT_FAIL);

      displaySendResult(ok);

      Serial.println(ok ? "Sent to Hub ✅" : "Send to Hub ❌");

      // Now show your existing success screen regardless
      currentState = EVENT_CREATED;
      delay(600);                 // short pause so user sees result
      displayEventCreated();
    }
  }

  // Button 2: Cancel and return to eye mode
  if (digitalRead(BUTTON1_PIN) == LOW) {
    if (millis() - lastButton1Press > BUTTON_COOLDOWN) {
      lastButton1Press = millis();
      Serial.println("Event cancelled, returning to eye mode");
      currentState = EYE_MODE;
      drawEyes();
    }
  }
}


void handleEventCreated() {
// Any button returns to eye mode
if ((digitalRead(BUTTON1_PIN) == LOW || digitalRead(BUTTON2_PIN) == LOW)) {
unsigned long now = millis();
if (now - lastButton1Press > BUTTON_COOLDOWN && now - lastButton2Press > BUTTON_COOLDOWN) {
Serial.println("Returning to eye mode");
currentState = EYE_MODE;
drawEyes();
}
}
}

// ===== EYE MODE FUNCTIONS =====
void performBlink() {
if (isBlinking) {
currentExpression = BLINKING;
drawEyes();
delay(BLINK_DURATION);
currentExpression = previousExpression;
isBlinking = false;
drawEyes();
}
}

void checkAutoBlink() {
unsigned long currentTime = millis();
if (currentTime - lastBlinkTime > BLINK_INTERVAL) {
isBlinking = true;
previousExpression = currentExpression;
lastBlinkTime = currentTime;
}
}

void changeExpression() {
int nextExpr = (int)currentExpression + 1;
if (nextExpr >= TOTAL_EXPRESSIONS - 1) {
nextExpr = NEUTRAL;
}
currentExpression = (EyeExpression)nextExpr;
drawEyes();
}

void triggerGreeting() {
EyeExpression savedExpression = currentExpression;
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(40, 28);
display.print("Hello!");
display.display();
delay(800);
for(int offset = -3; offset <= 3; offset++) {
display.clearDisplay();
int pupilOffset = offset * 2;
display.fillCircle(LEFT_EYE_X, EYE_Y, EYE_WIDTH/2, WHITE);
display.drawCircle(LEFT_EYE_X, EYE_Y, EYE_WIDTH/2, BLACK);
display.fillCircle(LEFT_EYE_X + pupilOffset, EYE_Y, PUPIL_SIZE, BLACK);
display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_WIDTH/2, WHITE);
display.drawCircle(RIGHT_EYE_X, EYE_Y, EYE_WIDTH/2, BLACK);
display.fillCircle(RIGHT_EYE_X + pupilOffset, EYE_Y, PUPIL_SIZE, BLACK);
display.display();
delay(80);
}
currentExpression = WINKING;
drawEyes();
delay(400);
currentExpression = HAPPY;
drawEyes();
delay(800);
currentExpression = savedExpression;
drawEyes();
}

// ===== SETUP =====
void setup() {
Serial.begin(115200);
Serial.println("Starting Social Robot Pet...");
pinMode(BUTTON1_PIN, INPUT_PULLUP);
pinMode(BUTTON2_PIN, INPUT_PULLUP);
Wire.begin(14, 12);
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
Serial.println("OLED not found!");
while(1);
}
Serial.println("OLED initialized!");
// Connect to Hub Wi-Fi once at startup (optional but nice)
bool wifiOk = connectToHubWifi();
Serial.println(wifiOk ? "WiFi connected to HangoutNet" : "WiFi NOT connected");

display.clearDisplay();
display.display();
delay(100);
// Startup animation
for(int i = 0; i < 3; i++) {
display.clearDisplay();
display.setTextSize(2);
display.setTextColor(WHITE);
display.setCursor(30, 20);
display.print("Hi!");
display.display();
delay(300);
display.clearDisplay();
display.display();
delay(200);
}
currentExpression = HAPPY;
drawEyes();
delay(1000);
currentExpression = NEUTRAL;
drawEyes();
Serial.println("Ready!");
Serial.println("EYE MODE: Btn1=Change expression, Btn2=Start event");
}

// ===== MAIN LOOP =====
void loop() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 50) {
  handleBothButtonsHold();
  if (currentState == XP_VIEW) {
    lastCheck = millis();
    return; // don't run other UI logic while XP screen is active
  }
  // Handle different states
  switch(currentState) {
  case EYE_MODE:
  // In eye mode, Button 2 starts event creation
  if (digitalRead(BUTTON2_PIN) == LOW) {
  if (millis() - lastButton2Press > BUTTON_COOLDOWN) {
  lastButton2Press = millis();
  startEventCreation();
  }
  }
  // Button 1 changes expression
  if (digitalRead(BUTTON1_PIN) == LOW) {
  if (millis() - lastButton1Press > BUTTON_COOLDOWN) {
  lastButton1Press = millis();
  Serial.println("Changing expression");
  changeExpression();
  }
  }
  // Automatic blinking
  checkAutoBlink();
  if (isBlinking) {
  performBlink();
  }
  break;
  case EVENT_TIME_SETTING:
  handleTimeSetting();
  break;
  case EVENT_LOCATION:
  handleLocationSelection();
  break;
  case EVENT_CONFIRMATION:
  handleEventConfirmation();
  break;
  case EVENT_CREATED:
  handleEventCreated();
  break;
  }
  lastCheck = millis();
  }
  delay(10);
}