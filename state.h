// ╔══════════════════════════════════════════════════════════════╗
// ║                       state.h                                ║
// ║  extern declarations for all global state variables and      ║
// ║  hardware objects. Definitions live in DIYPOD.ino.           ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once

#include "config.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>
#include <Preferences.h>

// ── Hardware Objects ──────────────────────────────────────────────
extern Adafruit_SSD1306    oled;
extern Adafruit_NeoPixel   strip;
extern DFRobotDFPlayerMini df;
extern Preferences         prefs;

// ── Playback State ────────────────────────────────────────────────
extern Screen     scr;        // active screen
extern int        track;      // current track number (1-based)
extern int        trackTotal; // total tracks on SD card
extern int        vol;        // volume (0–30)
extern int        eqIdx;      // EQ preset index
extern bool       playing;    // true if DFPlayer is playing
extern RepeatMode repeatMd;   // repeat mode
extern bool       shuffle;    // shuffle enabled
extern bool       autoPlay;   // auto-advance to next track

// ── UI State ──────────────────────────────────────────────────────
extern int  menuCur;  // selected menu item
extern int  listCur;  // selected track in browse list
extern int  listOff;  // scroll offset in browse list
extern int  setCur;   // selected settings item
extern int  gameCur;  // selected game in games menu
extern bool dirty;    // true = redraw requested next loop

// ── Boot State ────────────────────────────────────────────────────
extern uint8_t  bootPhase;  // 0 = booting, 1 = ready
extern uint32_t bootStart;  // millis() at power-on

// ── Lock / Sleep ──────────────────────────────────────────────────
extern bool     locked;        // buttons locked
extern uint8_t  sleepTimeout;  // sleep timeout index (0 = off)
extern uint32_t lastActivity;  // millis() of last button press
extern bool     screenOff;     // true when display is sleeping

// ── Elapsed Time Tracking ─────────────────────────────────────────
extern uint32_t trackStart;   // millis() when current track started
extern uint32_t pausedAt;     // millis() when playback was paused
extern uint32_t pausedAccum;  // accumulated paused milliseconds

// ── Scrolling Header ──────────────────────────────────────────────
extern int16_t  scrollX;    // current scroll position in pixels
extern uint32_t lastScroll; // millis() of last scroll tick

// ── DFPlayer Init ─────────────────────────────────────────────────
extern bool dfTried;  // true once Serial1/DFPlayer init has been attempted
extern bool dfReady;  // true once DFPlayer is ready and track count is known

// ── Buttons ───────────────────────────────────────────────────────
// Btn tracks the state of a single button for short/long press detection.
struct Btn {
  uint8_t  pin;
  bool     last     = HIGH;
  uint32_t downAt   = 0;
  bool     longDone = false;
};
extern Btn  bT, bM, bB;  // top, middle, bottom buttons
extern bool midHolding;   // true while MID is held past half the long-press threshold

// ── No SD Card Prompt ─────────────────────────────────────────────
extern bool     noSDPrompt;    // true while the no-SD popup is visible
extern bool     noSDAnimOut;   // true while the dismiss animation is playing
extern uint32_t noSDAnimStart; // millis() when the dismiss animation started