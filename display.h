// ╔══════════════════════════════════════════════════════════════╗
// ║                      display.h                               ║
// ║  All OLED drawing functions. Each screen has a draw*()       ║
// ║  function and matching draw*Hints() for the side panel.      ║
// ║  redraw() clears and dispatches to the active screen.        ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"
#include "battery.h"
#include "bitmaps.h"
#include <math.h>

// Game difficulty externs — defined in their respective game headers
extern uint8_t bbDifficulty;
extern uint8_t ppDifficulty;
extern uint8_t snakeDifficulty;

// ── Boot Screen ───────────────────────────────────────────────────

// 4x4 Bayer ordered dither matrix — values 0–15 map to brightness thresholds.
// Used by drawBootFade() to simulate a smooth fade on a 1-bit display.
const uint8_t DITHER[4][4] = {
  {  0,  8,  2, 10 },
  { 12,  4, 14,  6 },
  {  3, 11,  1,  9 },
  { 15,  7, 13,  5 }
};

// Draws the boot bitmap with a dithered brightness overlay.
// brightness 0.0 = fully black, 1.0 = full bitmap visible.
void drawBootFade(float brightness) {
  oled.clearDisplay();
  oled.drawBitmap(0, 0, bmp_Boot1, 128, 64, WHITE);
  int threshold = (int)((1.0f - brightness) * 16);
  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 128; x++) {
      if (DITHER[y % 4][x % 4] < threshold)
        oled.drawPixel(x, y, BLACK);
    }
  }
  oled.display();
}

// ── Shared UI Elements ────────────────────────────────────────────

// Draws the scrolling marquee header with title text and battery indicator.
// The title scrolls continuously using three copies to create a seamless loop.
void drawHeader(const char* title) {
  int textW    = strlen(title) * 6;
  uint32_t now = millis();

  // Advance scroll position every 15ms
  if (now - lastScroll >= 15) {
    lastScroll = now;
    scrollX--;
    if (scrollX <= -(textW + 32)) scrollX = 0;
  }

  // Draw three copies of the title to fill any scroll position
  oled.fillRect(0, 0, 128, 13, BLACK);
  oled.setTextWrap(false);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(scrollX, 2);                     oled.print(title);
  oled.setCursor(scrollX + textW + 32, 2);        oled.print(title);
  oled.setCursor(scrollX + (textW + 32) * 2, 2); oled.print(title);

  // Mask scroll overflow and draw the header border
  oled.fillRect(0,   0, 2,  13, BLACK);
  oled.fillRect(105, 0, 23, 13, BLACK);
  oled.drawBitmap(0, 0, bmp_Border6, 109, 13, WHITE);

  // Battery indicator: X = no divider, flashing cell = critical, filled cells = charge level
  int pct = getBatteryPct();
  oled.drawBitmap(110, 0, bmp_IconBatteryFrame, 18, 13, WHITE);
  if (pct < 0) {
    oled.drawLine(113, 2, 123, 8, WHITE);
    oled.drawLine(123, 2, 113, 8, WHITE);
  } else if (pct <= 20) {
    if ((millis() / 500) % 2 == 0)
      oled.fillRoundRect(112, 2, 3, 7, 2, WHITE);
  } else {
    int fillW = (int)(13.0f * pct / 100.0f);
    if (fillW > 0) oled.fillRoundRect(112, 2, fillW, 7, 2, WHITE);
  }
}

void drawSidePanel()      { oled.drawBitmap(110, 14, bmp_Border2, 18,  50, WHITE); }
void drawMainPanel()      { oled.drawBitmap(0,   14, bmp_Border3, 109, 50, WHITE); }
void drawSecondaryPanel() { oled.drawBitmap(0,   14, bmp_Border4, 96,  50, WHITE); }

// Draws the scrollbar panel with a proportional thumb indicator.
void drawScrollPanel(int total, int visible, int offset) {
  oled.drawBitmap(97, 14, bmp_Border5, 12, 50, WHITE);
  if (total <= 0) return;
  int thumbH = (total <= visible) ? 44 : max(4, 44 * visible / total);
  int thumbY = (total <= visible) ? 16 : 16 + (int)((long)offset * (44 - thumbH) / max(1, total - visible));
  oled.fillRoundRect(99, thumbY, 6, thumbH, 2, WHITE);
}

// Draws the "NO SD!" popup with optional scale for the dismiss animation.
// scale 1.0 = full size, scale 0.0 = invisible.
void drawNoSD(float scale = 1.0f) {
  int bW = (int)(50 * scale);
  int bH = (int)(22 * scale);
  if (bW < 2 || bH < 2) return;
  int bX = (OLED_W - bW) / 2;
  int bY = (OLED_H - bH) / 2;
  int rx = max(0, min(3, min(bW, bH) / 2 - 1));
  oled.fillRoundRect(bX, bY, bW, bH, rx, BLACK);
  oled.drawRoundRect(bX, bY, bW, bH, rx, WHITE);
  if (scale > 0.85f) {
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(bX + 9, bY + 7);
    oled.print("NO SD!");
  }
}

// ── Now Playing ───────────────────────────────────────────────────

void drawPlayHints() {
  // During no-SD prompt — show neutral dots
  if (noSDPrompt || noSDAnimOut) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8, 8, WHITE);
    oled.drawBitmap(114, 34, bmp_HintNull, 8, 8, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8, 8, WHITE);
    return;
  }
  // Locked — show lock icon
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(112, 20, bmp_HintNext, 12, 6, WHITE);
  if (midHolding)
    oled.drawBitmap(112, 32, bmp_HintReturn,  12, 12, WHITE);
  else if (playing)
    oled.drawBitmap(112, 32, bmp_HintPause,   12, 12, WHITE);
  else
    oled.drawBitmap(112, 32, bmp_HintPlay,    12, 12, WHITE);
  oled.drawBitmap(112, 50, bmp_HintPrevious, 12, 6, WHITE);
}

void drawPlay() {
  drawHeader("NOW PLAYING");
  drawSidePanel();
  drawPlayHints();
  drawMainPanel();

  oled.drawBitmap(3, 31, bmp_IconVolEQ, 16, 28, WHITE);

  // Track number — text size adapts to digit count
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(39, 17);
  oled.print("TRACK");
  if (track > 999) {
    char tn[6]; snprintf(tn, 6, "%04d", track);
    oled.setTextSize(1); oled.setCursor(76, 17); oled.print(tn);
  } else if (track > 99) {
    char tn[5]; snprintf(tn, 5, "%03d", track);
    oled.setTextSize(2); oled.setCursor(70, 17); oled.print(tn);
  } else {
    char tn[5]; snprintf(tn, 5, "%02d", track);
    oled.setTextSize(3); oled.setCursor(71, 17); oled.print(tn);
  }
  oled.setTextSize(1);

  // EQ preset name
  oled.setCursor(21, 35);
  oled.print(EQ_NAMES[eqIdx]);

  // Elapsed time — counts up while playing, freezes when paused
  uint32_t elMs = (!dfReady) ? 0
    : playing ? (millis() - trackStart - pausedAccum)
               : (pausedAt - trackStart - pausedAccum);
  uint16_t elSec = (uint16_t)(elMs / 1000);
  char el[7]; snprintf(el, 7, "%d:%02d", elSec / 60, elSec % 60);
  int ew   = strlen(el) * 6;
  int boxW = ew + 4, boxH = 11, boxX = 104 - boxW, boxY = 48;
  oled.drawRoundRect(boxX + 1, boxY, boxW - 1, boxH, 3, WHITE);
  oled.setCursor(boxX + 3, boxY + 2);
  oled.print(el);

  // Volume bar — fills proportionally to the right of the EQ label
  int volX  = 21;
  int timeX = 104 - (ew + 4);
  int volW  = timeX - volX - 2;
  oled.drawRoundRect(volX, boxY, volW + 1, boxH, 3, WHITE);
  int fill = (int)((long)vol * (volW - 4) / 30);
  if (fill > 0) oled.fillRoundRect(volX + 2, boxY + 2, fill + 1, boxH - 4, 2, WHITE);

  // Shuffle/repeat badge — animates between icons when both are active
  if (shuffle && repeatMd != REP_OFF) {
    const int holdMs  = 1000;
    const int transMs = 50;
    uint32_t  cycle   = holdMs * 2 + transMs * 4;
    uint32_t  t       = millis() % cycle;
    const uint8_t* bmp;
    if      (t < holdMs)                    bmp = bmp_IconRepeat;
    else if (t < holdMs + transMs)          bmp = bmp_RepShuTFrame1;
    else if (t < holdMs + transMs * 2)      bmp = bmp_RepShuTFrame2;
    else if (t < holdMs * 2 + transMs * 2)  bmp = bmp_IconShuffle;
    else if (t < holdMs * 2 + transMs * 3)  bmp = bmp_RepShuTFrame2;
    else                                     bmp = bmp_RepShuTFrame1;
    oled.drawBitmap(3, 17, bmp, 24, 12, WHITE);
  } else if (shuffle) {
    oled.drawBitmap(3, 17, bmp_IconShuffle, 24, 12, WHITE);
  } else if (repeatMd != REP_OFF) {
    oled.drawBitmap(3, 17, bmp_IconRepeat, 24, 12, WHITE);
  }
}

// ── Main Menu ─────────────────────────────────────────────────────

void drawMenuHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(113, 18, bmp_HintUp,     10, 11, WHITE);
  oled.drawBitmap(112, 32, bmp_HintSelect, 12, 12, WHITE);
  oled.drawBitmap(113, 47, bmp_HintDown,   10, 11, WHITE);
}

void drawMenu() {
  drawHeader("MAIN MENU");
  drawSidePanel();
  drawMenuHints();
  drawSecondaryPanel();
  // Show 4 items at a time, scrolling to keep the selection visible
  int vis = constrain(menuCur - 1, 0, MENU_N - 4);
  for (int i = 0; i < 4 && vis + i < MENU_N; i++) {
    int idx = vis + i;
    int y   = 15 + i * 11 + (i >= 1 ? 1 : 0) + (i >= 3 ? 1 : 0);
    oled.setTextSize(1);
    if (idx == menuCur) {
      oled.fillRoundRect(2, y + 1, 90, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(4, y + 2); oled.print(MENU_LABELS[idx]);
      oled.setTextColor(WHITE);
    } else {
      oled.setTextColor(WHITE);
      oled.setCursor(4, y + 2); oled.print(MENU_LABELS[idx]);
    }
  }
  drawScrollPanel(MENU_N, 4, vis);
}

// ── Browse Tracks ─────────────────────────────────────────────────

void drawTracksHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(113, 18, bmp_HintUp, 10, 11, WHITE);
  if (midHolding) {
    oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
  } else if (listCur + 1 == track) {
    // Show play/pause state for the currently loaded track
    oled.drawBitmap(112, 32, playing ? bmp_HintTrackPlay : bmp_HintTrackPause, 12, 12, WHITE);
  } else {
    oled.drawBitmap(112, 32, bmp_HintTrackStop, 12, 12, WHITE);
  }
  oled.drawBitmap(113, 47, bmp_HintDown, 10, 11, WHITE);
}

void drawTracks() {
  drawHeader("BROWSE TRACKS");
  drawSidePanel();
  drawTracksHints();
  drawSecondaryPanel();

  // Floating disc icon — sine-eased bob animation
  const int8_t BOB_FRAMES[] = {
    22, 23, 24, 25, 25, 26, 26, 26, 25, 25, 24, 23,
    22, 21, 20, 19, 19, 18, 18, 18, 19, 19, 20, 21
  };
  const uint8_t BOB_COUNT = 24;
  static uint8_t  bobFrame     = 0;
  static uint32_t lastBobFrame = 0;
  uint32_t now = millis();
  if (now - lastBobFrame >= 25) { lastBobFrame = now; bobFrame = (bobFrame + 1) % BOB_COUNT; }
  oled.drawBitmap(57, BOB_FRAMES[bobFrame], bmp_IconTracks, 30, 32, WHITE);

  // Track list — 4 visible items, scrolls to keep selection in view
  if (listCur < listOff)      listOff = listCur;
  if (listCur >= listOff + 4) listOff = listCur - 3;
  for (int i = 0; i < 4; i++) {
    int t2 = listOff + i + 1;
    if (trackTotal > 0 && t2 > trackTotal) break;
    int  y   = 15 + i * 11 + (i >= 1 ? 1 : 0) + (i >= 3 ? 1 : 0);
    char lbl[12]; snprintf(lbl, 12, "Track %d", t2);
    int  lblW = strlen(lbl) * 6 + 3;
    oled.setTextSize(1);
    if (t2 == listCur + 1) {
      oled.fillRoundRect(1, y, lblW + 2, 11, 3, BLACK);
      oled.fillRoundRect(2, y + 1, lblW, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(4, y + 2); oled.print(lbl);
      oled.setTextColor(WHITE);
    } else {
      oled.fillRoundRect(2, y + 1, lblW, 9, 2, BLACK);
      oled.setTextColor(WHITE);
      oled.setCursor(4, y + 2); oled.print(lbl);
    }
  }
  drawScrollPanel(max(trackTotal, 1), 4, listOff);
}

// ── Volume ────────────────────────────────────────────────────────

void drawVolHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(112, 17, bmp_HintPlus,   12, 12, WHITE);
  oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
  oled.drawBitmap(112, 47, bmp_HintMinus,  12, 12, WHITE);
}

void drawVol() {
  drawHeader("VOLUME");
  drawSidePanel();
  drawVolHints();
  drawMainPanel();
  oled.drawBitmap(5, 18, bmp_IconWarning, 96, 27, WHITE);
  int barX = 3, barY = 47, barW = 101, barH = 12;
  oled.drawRoundRect(barX, barY, barW, barH, 3, WHITE);
  int fill = (int)((long)vol * (barW - 4) / 30);
  if (fill > 0) oled.fillRoundRect(barX + 2, barY + 2, fill, barH - 4, 2, WHITE);
}

// ── Equalizer ─────────────────────────────────────────────────────

void drawEQHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(112, 20, bmp_HintRight, 12, 8, WHITE);
  oled.drawBitmap(112, 32, midHolding ? bmp_HintReturn : bmp_HintSelect, 12, 12, WHITE);
  oled.drawBitmap(112, 48, bmp_HintLeft,  12, 8, WHITE);
}

void drawEQ() {
  drawHeader(EQ_NAMES[eqIdx]);
  drawSidePanel();
  drawEQHints();
  drawMainPanel();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  int nw = strlen(EQ_NAMES[eqIdx]) * 12;
  oled.setCursor(3 + (104 - nw) / 2, 21);
  oled.print(EQ_NAMES[eqIdx]);
  oled.setTextSize(1);
  // Row of selection dots — one per preset, filled dot = active
  int fW = 12, fH = 12, fY = 47;
  int totalW = EQ_N * fW + (EQ_N - 1) * 3;
  int startX = 3 + (104 - totalW) / 2;
  for (int i = 0; i < EQ_N; i++) {
    int px = startX + i * (fW + 3);
    oled.drawRoundRect(px, fY, fW, fH, 3, WHITE);
    if (i == eqIdx) oled.fillRoundRect(px + 2, fY + 2, fW - 4, fH - 4, 1, WHITE);
  }
}

// ── Settings ──────────────────────────────────────────────────────

void drawSettingsHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(113, 18, bmp_HintUp, 10, 11, WHITE);
  oled.drawBitmap(112, 32, midHolding ? bmp_HintReturn : bmp_HintSelect, 12, 12, WHITE);
  oled.drawBitmap(113, 47, bmp_HintDown, 10, 11, WHITE);
}

void drawSettings() {
  drawHeader("SETTINGS");
  drawSidePanel();
  drawSettingsHints();
  drawMainPanel();
  const char* labs[] = { "Shuffle","Repeat","Auto-Play","Sleep" };
  for (int i = 0; i < 4; i++) {
    int  y = 15 + i * 11 + (i >= 1 ? 1 : 0) + (i >= 3 ? 1 : 0);
    char val[6];
    if      (i == 0) snprintf(val, 6, "%s", shuffle  ? "ON" : "OFF");
    else if (i == 1) snprintf(val, 6, "%s", REP_NAMES[repeatMd]);
    else if (i == 2) snprintf(val, 6, "%s", autoPlay ? "ON" : "OFF");
    else             snprintf(val, 6, "%s", SLEEP_NAMES[sleepTimeout]);
    oled.setTextSize(1);
    if (i == setCur) {
      oled.fillRoundRect(2, y + 1, 103, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(4, y + 2); oled.print(labs[i]);
      int vw = strlen(val) * 6;
      oled.setCursor(105 - vw, y + 2); oled.print(val);
      oled.setTextColor(WHITE);
    } else {
      oled.setTextColor(WHITE);
      oled.setCursor(4, y + 2); oled.print(labs[i]);
      int vw = strlen(val) * 6;
      oled.setCursor(105 - vw, y + 2); oled.print(val);
    }
  }
}

// ── Games Menu ────────────────────────────────────────────────────

void drawGamesHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(113, 18, bmp_HintUp,   10, 11, WHITE);
  oled.drawBitmap(112, 32, midHolding ? bmp_HintReturn : bmp_HintSelect, 12, 12, WHITE);
  oled.drawBitmap(113, 47, bmp_HintDown, 10, 11, WHITE);
}

void drawGames() {
  drawHeader("GAMES");
  drawSidePanel();
  drawGamesHints();
  drawMainPanel();
  oled.setTextSize(1);
  const char* games[] = { "Brick Breaker", "Ping Pong", "Snake" };
  for (int i = 0; i < 3; i++) {
    int y = 16 + i * 11;
    if (i == gameCur) {
      oled.fillRoundRect(2, y, 103, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(4, y + 1); oled.print(games[i]);
      oled.setTextColor(WHITE);
    } else {
      oled.setCursor(4, y + 1); oled.print(games[i]);
    }
  }
}

// ── About ─────────────────────────────────────────────────────────

void drawAboutHints() {
  if (locked) {
    oled.drawBitmap(114, 19, bmp_HintNull, 8,  8,  WHITE);
    oled.drawBitmap(112, 30, bmp_HintLock, 12, 16, WHITE);
    oled.drawBitmap(114, 49, bmp_HintNull, 8,  8,  WHITE);
    return;
  }
  oled.drawBitmap(114, 19, bmp_HintNull,   8,  8,  WHITE);
  oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
  oled.drawBitmap(114, 49, bmp_HintNull,   8,  8,  WHITE);
}

void drawAbout() {
  drawHeader("ABOUT");
  drawSidePanel();
  drawAboutHints();
  drawMainPanel();
  oled.drawBitmap(76, 19, bmp_IconFile, 24, 20, WHITE);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.drawBitmap(4, 19, bmp_IconArrow, 67, 10, WHITE);
  oled.setCursor(4, 32); oled.print("github.com/");
  oled.setCursor(4, 42); oled.print("CortexFirmware/");
  oled.setCursor(4, 52); oled.print("diypod");
}

// ── Game Difficulty Screens ───────────────────────────────────────
// All three games use the same layout — header, hints, and a 3-item
// difficulty list with score multiplier or speed label on the right.

void drawBBDiff() {
  drawHeader("BRICK BREAKER");
  drawSidePanel();
  drawGamesHints();
  drawMainPanel();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(3, 17);
  oled.print(F("SELECT DIFFICULTY:"));
  const char* labels[] = { "Easy          +1x", "Normal        +2x", "Hard          +3x" };
  for (int i = 0; i < 3; i++) {
    int y = 31 + i * 10;
    if (i == bbDifficulty) {
      oled.fillRoundRect(2, y, 103, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(3, y + 1); oled.print(labels[i]);
      oled.setTextColor(WHITE);
    } else {
      oled.setCursor(3, y + 1); oled.print(labels[i]);
    }
  }
}

void drawPPDiff() {
  drawHeader("PING PONG");
  drawSidePanel();
  drawGamesHints();
  drawMainPanel();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(3, 17);
  oled.print(F("SELECT DIFFICULTY:"));
  const char* labels[] = { "Easy", "Normal", "Hard" };
  for (int i = 0; i < 3; i++) {
    int y = 31 + i * 10;
    if (i == ppDifficulty) {
      oled.fillRoundRect(2, y, 103, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(3, y + 1); oled.print(labels[i]);
      oled.setTextColor(WHITE);
    } else {
      oled.setCursor(3, y + 1); oled.print(labels[i]);
    }
  }
}

void drawSnakeDiff() {
  drawHeader("SNAKE");
  drawSidePanel();
  drawGamesHints();
  drawMainPanel();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(3, 17);
  oled.print(F("SELECT DIFFICULTY:"));
  const char* labels[] = { "Easy           +1", "Normal        +10", "Hard         +100" };
  for (int i = 0; i < 3; i++) {
    int y = 31 + i * 10;
    if (i == snakeDifficulty) {
      oled.fillRoundRect(2, y, 103, 9, 2, WHITE);
      oled.setTextColor(BLACK);
      oled.setCursor(3, y + 1); oled.print(labels[i]);
      oled.setTextColor(WHITE);
    } else {
      oled.setCursor(3, y + 1); oled.print(labels[i]);
    }
  }
}

// ── Redraw ────────────────────────────────────────────────────────

// Clears the display and dispatches to the active screen's draw function.
// Games manage their own draw loop and are skipped here.
void redraw() {
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(1);
  switch (scr) {
    case SCR_PLAY:       drawPlay();      break;
    case SCR_MENU:       drawMenu();      break;
    case SCR_TRACKS:     drawTracks();    break;
    case SCR_VOL:        drawVol();       break;
    case SCR_EQ:         drawEQ();        break;
    case SCR_SETTINGS:   drawSettings();  break;
    case SCR_GAMES:      drawGames();     break;
    case SCR_ABOUT:      drawAbout();     break;
    case SCR_BB_DIFF:    drawBBDiff();    break;
    case SCR_PP_DIFF:    drawPPDiff();    break;
    case SCR_SNAKE_DIFF: drawSnakeDiff(); break;
    case SCR_BB:                          break;  // game manages its own drawing
    case SCR_PP:                          break;
    case SCR_SNAKE:                       break;
  }
  oled.display();
  dirty = false;
}
