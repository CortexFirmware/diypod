// ╔══════════════════════════════════════════════════════════════╗
// ║                      config.h                                ║
// ║  Hardware pin definitions, compile-time constants, enums,    ║
// ║  and lookup tables. Included by every other header.          ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once

#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>

// ── Developer Settings ────────────────────────────────────────────
// Set VBAT_SIMULATE to true to test the battery UI without hardware.
// Set VBAT_SIM_PCT to any value 0–100 to simulate that charge level.
#define VBAT_SIMULATE  false
#define VBAT_SIM_PCT   75

// ── Pin Definitions ───────────────────────────────────────────────
#define PIN_VBAT   1   // Battery voltage divider midpoint (rev2+ hardware)
#define PIN_SDA    2   // OLED I2C data
#define PIN_SCL    3   // OLED I2C clock
#define PIN_DF_RX  4   // DFPlayer RX (1kΩ resistor on this line)
#define PIN_DF_TX  5   // DFPlayer TX
#define PIN_BTN_T  6   // Top button
#define PIN_BTN_B  7   // Bottom button
#define PIN_BTN_M  8   // Middle button
#define PIN_LED    21  // WS2812B LED strip data (optional easter egg)
#define PIN_AUDIO  0   // DAC_L audio input for visualizer (optional easter egg)

// ── Display ───────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C

// ── LED Strip Visualizer ──────────────────────────────────────────
// Adjust NUM_LEDS to match your strip length.
#define NUM_LEDS        144   // Number of LEDs in your strip
#define VIS_SMOOTHING   0.7f  // Bar smoothing — higher = slower response (0–1)
#define VIS_PEAK_DECAY  0.99f // Peak dot decay — higher = slower fall (0–1)
#define VIS_SENSITIVITY 300   // Lower = more sensitive to quiet audio

// ── UI Constants ──────────────────────────────────────────────────
#define MENU_N   7    // Number of main menu items
#define EQ_N     6    // Number of EQ presets
#define LONG_MS  700  // Button hold threshold in milliseconds

// ── Screen Enum ───────────────────────────────────────────────────
// All possible screens. Games have a separate difficulty select screen.
enum Screen {
  SCR_PLAY,
  SCR_MENU,
  SCR_TRACKS,
  SCR_VOL,
  SCR_EQ,
  SCR_SETTINGS,
  SCR_GAMES,
  SCR_ABOUT,
  SCR_BB_DIFF,
  SCR_BB,
  SCR_PP_DIFF,
  SCR_PP,
  SCR_SNAKE_DIFF,
  SCR_SNAKE
};

enum RepeatMode { REP_OFF, REP_ON };

// ── Lookup Tables ─────────────────────────────────────────────────
const char*    MENU_LABELS[] = { "Now Playing","Browse Tracks","Volume","Equalizer","Games","Settings","About" };
const char*    EQ_NAMES[]    = { "NORMAL","POP","ROCK","JAZZ","CLASSIC","BASS" };
const uint8_t  EQ_VALS[]     = { DFPLAYER_EQ_NORMAL, DFPLAYER_EQ_POP, DFPLAYER_EQ_ROCK,
                                  DFPLAYER_EQ_JAZZ, DFPLAYER_EQ_CLASSIC, DFPLAYER_EQ_BASS };
const char*    REP_NAMES[]   = { "OFF","ON" };
const uint32_t SLEEP_MS[]    = { 0, 30000, 60000, 300000, 600000 };
const char*    SLEEP_NAMES[] = { "OFF","30S","1M","5M","10M" };
