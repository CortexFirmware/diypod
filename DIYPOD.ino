/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║              DIYPOD Shuffle — Firmware v1.0                  ║
 * ║         github.com/cortekz/diypod  |  By Cortekz             ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * Hardware: ESP32-C3 SuperMini, DFPlayer Mini, SSD1306 OLED (128x64),
 *           TP4057 LiPo charger, 3x tactile buttons
 *
 * Required Libraries:
 *   - Adafruit SSD1306 + Adafruit GFX
 *   - DFRobotDFPlayerMini
 *   - Adafruit NeoPixel
 *   - Preferences (built-in)
 *
 * ── Wiring ───────────────────────────────────────────────────────
 *   OLED SDA   → GPIO2       OLED SCL   → GPIO3
 *   DF TX      → GPIO5       DF RX      ← GPIO4  (1kΩ on RX line)
 *   BTN TOP    → GPIO6 + GND
 *   BTN BOT    → GPIO7 + GND
 *   BTN MID    → GPIO8 + GND
 *   VBAT div   → GPIO1       (100kΩ+100kΩ voltage divider)
 *   LED strip  → GPIO21      (WS2812B, optional easter egg)
 *   Audio in   → GPIO0       (DAC_L from DFPlayer, optional easter egg)
 *
 * ── Button Actions ───────────────────────────────────────────────
 *   NOW PLAYING  TOP=Next        MID=Play/Pause   BOT=Prev
 *                TOP(hold)=Skip  MID(hold)=Menu   BOT(hold)=Skip back
 *   MENU         TOP=Up          MID=Select       BOT=Down
 *                                MID(hold)=Back
 *   BROWSE       TOP=Up          MID=Play         BOT=Down
 *                                MID(hold)=Back
 *   VOLUME       TOP=Vol+        MID=Back         BOT=Vol-
 *   EQUALIZER    TOP=Next EQ     MID=Confirm      BOT=Prev EQ
 *   SETTINGS     TOP=Up          MID=Toggle       BOT=Down
 *                                MID(hold)=Back
 *   GLOBAL       TOP+BOT(hold)=Lock/Unlock
 *                TOP+MID+BOT(hold)=DFPlayer reset + SD rescan
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>
#include <Preferences.h>

#include "config.h"
#include "state.h"
#include "bitmaps.h"
#include "battery.h"
#include "player.h"
#include "visualizer.h"
#include "input.h"
#include "display.h"
#include "brickbreaker.h"
#include "pingpong.h"
#include "snake.h"

// ── Hardware Objects ──────────────────────────────────────────────
Adafruit_SSD1306    oled(OLED_W, OLED_H, &Wire, -1);
Adafruit_NeoPixel   strip(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);
DFRobotDFPlayerMini df;
Preferences         prefs;

// ── State Definitions ─────────────────────────────────────────────
// All globals are declared here and exposed via extern in state.h

// Playback
Screen     scr        = SCR_PLAY;
int        track      = 1;
int        trackTotal = 0;
int        vol        = 20;
int        eqIdx      = 0;
bool       playing    = false;
RepeatMode repeatMd   = REP_OFF;
bool       shuffle    = false;
bool       autoPlay   = true;

// No SD card prompt
bool       noSDPrompt    = false;
bool       noSDAnimOut   = false;
uint32_t   noSDAnimStart = 0;

// UI navigation
int        menuCur    = 0;
int        listCur    = 0;
int        listOff    = 0;
int        setCur     = 0;
int        gameCur    = 0;
bool       dirty      = true;   // set true to request a redraw next loop

// Boot
uint8_t    bootPhase  = 0;      // 0 = booting, 1 = ready
uint32_t   bootStart  = 0;

// Sleep / lock
bool       locked       = false;
uint8_t    sleepTimeout = 0;
uint32_t   lastActivity = 0;
bool       screenOff    = false;

// Elapsed time tracking
uint32_t   trackStart  = 0;
uint32_t   pausedAt    = 0;
uint32_t   pausedAccum = 0;

// Scrolling header
int16_t    scrollX    = 0;
uint32_t   lastScroll = 0;

// DFPlayer init flags
bool       dfTried = false;
bool       dfReady = false;

// Buttons
Btn        bT{PIN_BTN_T}, bM{PIN_BTN_M}, bB{PIN_BTN_B};
bool       midHolding = false;

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
  pinMode(PIN_BTN_T, INPUT_PULLUP);
  pinMode(PIN_BTN_M, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);  // I2C fast mode for smoother OLED updates

  strip.begin();
  strip.setBrightness(50);
  strip.show();

  oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  oled.clearDisplay();
  oled.display();

  bootStart    = millis();
  randomSeed(analogRead(A0));
  loadSettings();

  trackStart   = millis();
  pausedAt     = millis();
  lastActivity = millis();
  playing      = false;
  dirty        = true;
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
  uint32_t now = millis();

  // ── Boot sequence ─────────────────────────────────────────────
  // Shows a dithered fade-in/out of the boot bitmap while the
  // DFPlayer initialises in the background.
  if (bootPhase < 1) {
    uint32_t elapsed = now - bootStart;

    const uint32_t FADE_IN_MS  = 400;
    const uint32_t HOLD_MS     = 2700;
    const uint32_t FADE_OUT_MS = 400;
    const uint32_t FADE_TOTAL  = FADE_IN_MS + HOLD_MS + FADE_OUT_MS;

    if (elapsed < FADE_TOTAL) {
      float brightness;
      if (elapsed < FADE_IN_MS) {
        brightness = (float)elapsed / FADE_IN_MS;
      } else if (elapsed < FADE_IN_MS + HOLD_MS) {
        brightness = 1.0f;
      } else {
        brightness = 1.0f - (float)(elapsed - FADE_IN_MS - HOLD_MS) / FADE_OUT_MS;
        brightness = max(0.0f, brightness);
      }
      drawBootFade(brightness);
    }

    // Clear display after fade completes
    if (elapsed >= FADE_TOTAL && elapsed < FADE_TOTAL + 50) {
      oled.clearDisplay();
      oled.display();
    }

    // Start DFPlayer init at 2s (gives it time to stabilise after power-on)
    if (elapsed >= 2000 && !dfTried) {
      dfTried = true;
      Serial1.begin(9600, SERIAL_8N1, PIN_DF_RX, PIN_DF_TX);
      df.begin(Serial1, false, false);
      df.setTimeOut(500);
      df.volume(vol);
      df.EQ(EQ_VALS[eqIdx]);
      df.outputDevice(DFPLAYER_DEVICE_SD);
    }

    if (elapsed >= 3500) { bootPhase = 1; }
    if (bootPhase < 1) return;
  }

  // ── DFPlayer ready ────────────────────────────────────────────
  // Runs once after boot. Reads the SD track count with retries,
  // then starts playback. Shows a "NO SD!" prompt if no card found.
  if (dfTried && !dfReady && now > 3500) {
    dfReady = true;
    trackStart = millis(); pausedAccum = 0;

    for (int i = 0; i < 5; i++) {
      trackTotal = df.readFileCounts(DFPLAYER_DEVICE_SD);
      if (trackTotal > 1) break;
      delay(200);
    }

    if (trackTotal < 1) {
      noSDPrompt = true;
      trackTotal = 1;
    }

    df.play(1); track = 1; playing = true;
    if (!noSDPrompt) dirty = true;
  }

  // ── Volume init ───────────────────────────────────────────────
  // Set volume once after DFPlayer has fully settled
  static bool volSet = false;
  if (dfReady && !volSet && now > 4200) { df.volume(vol); volSet = true; }

  // ── Dirty flag ────────────────────────────────────────────────
  // Forced every loop to keep battery meter and animations smooth.
  // Suppressed during the no-SD prompt to prevent flickering.
  static uint32_t lastTick = 0;
  if (!noSDPrompt && !noSDAnimOut) {
    if (playing && now - lastTick >= 1000) { lastTick = now; dirty = true; }
    dirty = true;
  }

  // ── DFPlayer events ───────────────────────────────────────────
  if (dfReady && df.available()) {
    uint8_t type = df.readType();
    if (type == DFPlayerPlayFinished) {
      static uint32_t lastFinish = 0;
      if (now - lastFinish > 1000) { lastFinish = now; onFinished(); }
    } else {
      df.read();
    }
  }

  int T = readBtn(bT), M = readBtn(bM), B = readBtn(bB);
  midHolding = (digitalRead(PIN_BTN_M) == LOW && (millis() - bM.downAt) >= (LONG_MS / 2));

  // ── Wake from sleep ───────────────────────────────────────────
  // Any button held for LONG_MS wakes the display.
  static uint32_t wakePressTime = 0;
  if (screenOff) {
    bool anyDown = digitalRead(PIN_BTN_T) == LOW ||
                   digitalRead(PIN_BTN_M) == LOW ||
                   digitalRead(PIN_BTN_B) == LOW;
    if (anyDown) {
      if (wakePressTime == 0) wakePressTime = now;
      if (now - wakePressTime > LONG_MS) {
        screenOff = false;
        oled.ssd1306_command(SSD1306_DISPLAYON);
        lastActivity = now;
        bT.last = LOW; bT.longDone = true;
        bM.last = LOW; bM.longDone = true;
        bB.last = LOW; bB.longDone = true;
        dirty = true;
        wakePressTime = 0;
      }
    } else {
      wakePressTime = 0;
    }
    return;
  }

  handleScrollRepeat(now);

  // ── Button lock (TOP + BOT hold 500ms) ────────────────────────
  static uint32_t lockPressTime = 0;
  static bool     lockFired     = false;
  if (digitalRead(PIN_BTN_T) == LOW && digitalRead(PIN_BTN_B) == LOW) {
    if (lockPressTime == 0) lockPressTime = now;
    if (now - lockPressTime > 500 && !lockFired) {
      locked = !locked; lockFired = true; dirty = true;
      bT.last = LOW; bT.longDone = true;
      bB.last = LOW; bB.longDone = true;
      bM.last = LOW; bM.longDone = true;
    }
  } else { lockPressTime = 0; lockFired = false; }

  // ── SD rescan (TOP + MID + BOT hold 1s) ──────────────────────
  // Resets the DFPlayer and re-reads the SD card. Useful if a card
  // is inserted after boot or files are added without power cycling.
  static uint32_t triplePressTime = 0;
  static bool     tripleFired     = false;
  if (digitalRead(PIN_BTN_T) == LOW &&
      digitalRead(PIN_BTN_M) == LOW &&
      digitalRead(PIN_BTN_B) == LOW) {
    if (triplePressTime == 0) triplePressTime = now;
    if (now - triplePressTime > 1000 && !tripleFired) {
      tripleFired = true;
      df.reset(); delay(3000);
      df.volume(vol); df.EQ(EQ_VALS[eqIdx]);
      for (int i = 0; i < 5; i++) {
        trackTotal = df.readFileCounts(DFPLAYER_DEVICE_SD);
        if (trackTotal > 1) break;
        delay(200);
      }
      dirty = true;
    }
  } else { triplePressTime = 0; tripleFired = false; }

  // ── Sleep timeout ─────────────────────────────────────────────
  if (sleepTimeout > 0 && !screenOff && now - lastActivity > SLEEP_MS[sleepTimeout]) {
    screenOff = true;
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  // ── Low battery auto-pause ────────────────────────────────────
  // Pauses playback at 0% to prevent SD card corruption on sudden
  // power loss. Only fires if a voltage divider is connected (pct != -1).
  if (getBatteryPct() == 0 && playing && dfReady) {
    df.pause(); playing = false; pausedAt = millis(); dirty = true;
  }

  // ── No SD card prompt ─────────────────────────────────────────
  // Displays an animated popup over the now playing screen when no
  // SD card is detected. Any button press dismisses it with an
  // outro shrink animation.
  if (noSDPrompt || noSDAnimOut) {
    const uint32_t ANIM_MS = 150;
    float scale = 1.0f;

    if (noSDAnimOut) {
      float t = (float)(now - noSDAnimStart) / ANIM_MS;
      scale = max(0.0f, 1.0f - t);
      if (t >= 1.0f) { noSDAnimOut = false; dirty = true; }
    }

    if (noSDPrompt || noSDAnimOut) {
      static uint32_t lastNoSDDraw = 0;
      if (now - lastNoSDDraw >= 16) {
        lastNoSDDraw = now;
        oled.clearDisplay();
        drawPlay();
        drawNoSD(scale);
        oled.display();
      }
      if (noSDPrompt && (T == 0 || M == 0 || B == 0)) {
        noSDPrompt    = false;
        noSDAnimOut   = true;
        noSDAnimStart = now;
      }
      return;
    }
  }

  // ── Input and game routing ────────────────────────────────────
  // Games handle their own draw loop and return early so the main
  // redraw() is not called while a game is active.
  handleInput(T, M, B);

  if (scr == SCR_BB) {
    bbUpdate(T, M, B);
    return;
  }
  if (scr == SCR_PP) {
    ppUpdate(T, M, B);
    return;
  }
  if (scr == SCR_SNAKE) {
    snakeUpdate(T, M, B);
    return;
  }

  handleVisualizer();

  if (dirty && !noSDPrompt && !noSDAnimOut) redraw();
  delay(10);
}
