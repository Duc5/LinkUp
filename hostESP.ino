#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
// #include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SH1106G display(128, 64, &Wire);
// Your mate used SDA=GPIO14, SCL=GPIO12
const int OLED_SDA = 14;
const int OLED_SCL = 12;
const uint8_t OLED_ADDR = 0x3C;

// ===== Wi-Fi AP =====
const char* AP_SSID = "HangoutNet";
const char* AP_PASS = "hangout123"; // >= 8 chars

// ===== HTTP Server =====
ESP8266WebServer server(80);



// ------------------------------------------------------------
// UI state machine: show eyes always, show event for 8s
// ------------------------------------------------------------
enum UiMode { UI_EYES, UI_SURPRISED, UI_EVENT };
UiMode uiMode = UI_EYES;

unsigned long lastEyesRenderMs = 0;
const unsigned long EYES_RENDER_PERIOD_MS = 120; // ~8 fps
unsigned long surprisedUntilMs = 0;
const unsigned long SURPRISED_DURATION_MS = 2000; // ~1.8s

unsigned long eventUntilMs = 0;
String eventLine1 = "";
String eventLine2 = "";

// forward declarations (since drawEyes is defined later)
void drawEyes();
void updateEyesAnimation();
void scheduleNextBlink(unsigned long now);
void scheduleNextMood(unsigned long now);


void drawEventScreen() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(20, 10);
  display.print("EVENT RECEIVED!!!");

  display.setCursor(20, 30);
  display.print(eventLine1);

  display.setCursor(20, 46);
  display.print(eventLine2);

  display.display();
}

// ===== EYE SYSTEM VARIABLES =====
// Eye positions
const int LEFT_EYE_X = 31;
const int RIGHT_EYE_X = 95;
const int EYE_Y = 24;
const int EYE_WIDTH = 32;
const int PUPIL_SIZE = 8;
bool happyMode = true; // true = HAPPY, false = CURIOUS

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

EyeExpression currentExpression = HAPPY;
EyeExpression previousExpression = HAPPY;
uint8_t randomCuteExpressionIndex() {
  const uint8_t opts[] = { HAPPY, CURIOUS };
  return opts[random(0, (int)(sizeof(opts) / sizeof(opts[0])))];
}


// Blinking
bool isBlinking = true;
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 3000;
const unsigned long BLINK_DURATION = 150;

// ===== Non-blocking blink + expression scheduler =====
bool blinkActive = false;
unsigned long blinkStartMs = 0;
unsigned long nextBlinkMs = 0;

unsigned long nextMoodChangeMs = 0;

unsigned long randBetween(unsigned long a, unsigned long b) {
  return a + (unsigned long)random((long)(b - a + 1));
}


void scheduleNextBlink(unsigned long now) {
  nextBlinkMs = now + randBetween(1000, 1000); // 1-2s
}

void scheduleNextMood(unsigned long now) {
  nextMoodChangeMs = now + randBetween(1000, 1000); // 6–12s
}

void updateEyesAnimation() {
  unsigned long now = millis();

  // Start blink
  if (!blinkActive && now >= nextBlinkMs) {
    blinkActive = true;
    blinkStartMs = now;
    previousExpression = currentExpression;
    currentExpression = BLINKING;
  }

  // End blink
  if (blinkActive && now - blinkStartMs >= BLINK_DURATION) {
    blinkActive = false;
    currentExpression = previousExpression;
    scheduleNextBlink(now);
  }

  // Change expression occasionally
// Alternate between HAPPY <-> CURIOUS
  if (!blinkActive && now >= nextMoodChangeMs) {
    happyMode = !happyMode;
    currentExpression = happyMode ? HAPPY : CURIOUS;
    scheduleNextMood(now);
  }

}

// Expression names
const char* expressionNames[] = {
"Neutral", "Happy", "Sad", "Angry", "Sleepy",
"Surprised", "Curious", "Love", "Winking", "Blinking"
};

void renderUi() {
  unsigned long now = millis();

  if (uiMode == UI_SURPRISED) {
    drawEyes(); // draw SURPRISED eyes

    if ((long)(now - surprisedUntilMs) >= 0) {
      // move to event screen
      uiMode = UI_EVENT;
      eventUntilMs = now + 8000;
      drawEventScreen();
    }
    return;
  }

  if (uiMode == UI_EVENT) {
    if ((long)(now - eventUntilMs) >= 0) {
      uiMode = UI_EYES; // timeout done
      // fall through to draw eyes immediately
    } else {
      // During event mode, redraw event screen occasionally
      // (or just once when event arrives; both are fine)
      drawEventScreen();
      return;
    }
  }


    // UI_EYES mode
    updateEyesAnimation();

    if (now - lastEyesRenderMs >= EYES_RENDER_PERIOD_MS) {
      lastEyesRenderMs = now;
      drawEyes();
    }

}

// ===== HTTP Handlers =====
void handleRoot() {
  server.send(200, "text/plain", "Hub A OK. POST /event with JSON.");
}

void handleEvent() {
  String body = server.arg("plain");

  // Respond immediately (prevents sender timeouts)
  server.send(200, "application/json", "{\"ok\":true}");

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.println("Bad JSON received");
    eventLine1 = "Bad JSON";
    eventLine2 = "";
    uiMode = UI_EVENT;
    eventUntilMs = millis() + 4000;
    drawEventScreen(); // show instantly
    return;
  }

  const char* location = doc["location"] | "KITCHEN";
  int timeOffset = doc["time_offset_h"] | 0;
  const char* senderId = doc["sender_id"] | "UNKNOWN";

  Serial.println("---- EVENT RECEIVED ----");
  Serial.print("sender: "); Serial.println(senderId);
  Serial.print("location: "); Serial.println(location);
  Serial.print("time_offset_h: "); Serial.println(timeOffset);
  Serial.println("------------------------");

  eventLine1 = String("From: ") + senderId;
  eventLine2 = String(location) + " in " + String(timeOffset) + "h";

  // Step 1: surprised reaction
  previousExpression = currentExpression;
  currentExpression = LOVE;

  uiMode = UI_SURPRISED;
  surprisedUntilMs = millis() + SURPRISED_DURATION_MS;

}

// ===== Setup / Loop =====
void setup() {
  Serial.begin(115200);
  delay(200);

  // OLED init
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("OLED not found!");
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.display();

  // Start in happy-eyes standby immediately
  uiMode = UI_EYES;
  drawEyes();
  randomSeed(ESP.getCycleCount());
  scheduleNextBlink(millis());
  scheduleNextMood(millis());


  // Wi-Fi AP init
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(AP_SSID, AP_PASS);

  IPAddress ip = WiFi.softAPIP();

  Serial.println();
  Serial.print("AP start: "); Serial.println(apOk ? "OK" : "FAILED");
  Serial.print("SSID: "); Serial.println(AP_SSID);
  Serial.print("IP:   "); Serial.println(ip);

  // HTTP server init
  server.on("/", HTTP_GET, handleRoot);
  server.on("/event", HTTP_POST, handleEvent);
  server.begin();

  Serial.println("HTTP server started on port 80");

  // If AP failed, show a quick message for 8s, then return to eyes
  if (!apOk) {
    eventLine1 = "WIFI FAIL";
    eventLine2 = "AP not started";
    uiMode = UI_EVENT;
    eventUntilMs = millis() + 8000;
  }
}



// ===== EYE DRAWING FUNCTIONS =====
void drawEyebrow(int centerX, int centerY, bool isAngry) {
int yOffset = isAngry ? -18 : -12;
int curve = isAngry ? -3 : 3;
display.drawLine(centerX - 12, centerY + yOffset,
centerX, centerY + yOffset + curve, SH110X_BLACK);
display.drawLine(centerX, centerY + yOffset + curve,
centerX + 12, centerY + yOffset, SH110X_BLACK);
}

void drawEye(int centerX, int centerY, EyeExpression expression, bool isLeftEye = true) {
display.fillCircle(centerX, centerY, EYE_WIDTH/2, SH110X_WHITE);
display.drawCircle(centerX, centerY, EYE_WIDTH/2, SH110X_BLACK);
int pupilX = centerX;
int pupilY = centerY;
switch(expression) {
case NEUTRAL: break;
case HAPPY:
pupilY = centerY - 3;
for (int i = -8; i <= 8; i += 4) {
display.drawPixel(centerX + i, centerY - 10, SH110X_BLACK);
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
centerX + EYE_WIDTH/2, centerY - 3, SH110X_BLACK);
display.fillCircle(centerX, centerY, PUPIL_SIZE - 2, SH110X_BLACK);
return;
case SURPRISED:
display.drawCircle(centerX, centerY, EYE_WIDTH/2 + 2, SH110X_BLACK);
display.fillCircle(centerX, centerY, PUPIL_SIZE - 3, SH110X_BLACK);
return;
case CURIOUS:
pupilX = centerX + (isLeftEye ? 6 : -6);
break;
case LOVE:
if (isLeftEye) {
display.fillCircle(centerX - 3, centerY - 3, 4, SH110X_BLACK);
display.fillCircle(centerX + 3, centerY - 3, 4, SH110X_BLACK);
display.fillTriangle(centerX - 7, centerY - 1,
centerX + 7, centerY - 1,
centerX, centerY + 8, SH110X_BLACK);
} else {
display.fillCircle(centerX - 3, centerY - 3, 4, SH110X_BLACK);
display.fillCircle(centerX + 3, centerY - 3, 4, SH110X_BLACK);
display.fillTriangle(centerX - 7, centerY - 1,
centerX + 7, centerY - 1,
centerX, centerY + 8, SH110X_BLACK);
}
return;
case WINKING:
if (isLeftEye) {
display.drawLine(centerX - EYE_WIDTH/2, centerY,
centerX + EYE_WIDTH/2, centerY, SH110X_BLACK);
return;
} else {
pupilX = centerX - 4;
}
break;
case BLINKING:
display.drawLine(centerX - EYE_WIDTH/2, centerY,
centerX + EYE_WIDTH/2, centerY, SH110X_BLACK);
return;
}
display.fillCircle(pupilX, pupilY, PUPIL_SIZE, SH110X_BLACK);
display.fillCircle(pupilX - 2, pupilY - 2, 2, SH110X_WHITE);
}

// ===== DISPLAY FUNCTIONS =====
void drawEyes() {
display.clearDisplay();
drawEye(LEFT_EYE_X, EYE_Y, currentExpression, true);
drawEye(RIGHT_EYE_X, EYE_Y, currentExpression, false);
display.setTextSize(1);
display.setTextColor(SH110X_BLACK, SH110X_WHITE);
display.setCursor(40, 55);
// display.print(expressionNames[currentExpression]);
display.display();
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
display.setTextColor(SH110X_WHITE);
display.setCursor(40, 28);
display.print("Hello!");
display.display();
delay(800);
for(int offset = -3; offset <= 3; offset++) {
display.clearDisplay();
int pupilOffset = offset * 2;
display.fillCircle(LEFT_EYE_X, EYE_Y, EYE_WIDTH/2, SH110X_WHITE);
display.drawCircle(LEFT_EYE_X, EYE_Y, EYE_WIDTH/2, SH110X_BLACK);
display.fillCircle(LEFT_EYE_X + pupilOffset, EYE_Y, PUPIL_SIZE, SH110X_BLACK);
display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_WIDTH/2, SH110X_WHITE);
display.drawCircle(RIGHT_EYE_X, EYE_Y, EYE_WIDTH/2, SH110X_BLACK);
display.fillCircle(RIGHT_EYE_X + pupilOffset, EYE_Y, PUPIL_SIZE, SH110X_BLACK);
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


void loop() {
  server.handleClient();
  yield(); // keeps ESP8266 responsive
  renderUi();
}
