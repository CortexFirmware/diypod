// ╔══════════════════════════════════════════════════════════════╗
// ║                       input.h                                ║
// ║  Button reading (readBtn), screen navigation (goTo,          ║
// ║  menuSelect), action dispatch (handleInput), and hold-to-    ║
// ║  repeat/skip logic (handleScrollRepeat).                     ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"
#include "player.h"

// Game state externs — defined in their respective game headers
extern int      gameCur;
extern uint8_t  bbDifficulty;
extern uint8_t  ppDifficulty;
extern uint8_t  snakeDifficulty;
extern uint32_t snakeSpeedMs;
extern const int SNAKE_DIFF_SPEEDS[];

// Reads a single button and returns:
//   0  = short press (released before LONG_MS)
//   1  = long press  (held for LONG_MS)
//  -1  = no event
// Debounce threshold is 30ms on release.
int readBtn(Btn &b) {
  bool raw     = digitalRead(b.pin);
  uint32_t now = millis();
  if (raw == LOW  && b.last == HIGH) { b.downAt = now; b.longDone = false; }
  if (raw == LOW  && !b.longDone && now - b.downAt >= LONG_MS) { b.longDone = true; b.last = raw; return 1; }
  if (raw == HIGH && b.last == LOW)  { b.last = raw; return (!b.longDone && now - b.downAt >= 30) ? 0 : -1; }
  b.last = raw;
  return -1;
}

// Navigates to a screen. Resets the scrolling header when changing screens.
void goTo(Screen s) {
  if (s != scr) { scrollX = 0; lastScroll = 0; }
  scr   = s;
  dirty = true;
}

// Handles selection from the main menu.
void menuSelect() {
  switch (menuCur) {
    case 0: goTo(SCR_PLAY); break;
    case 1: listCur = track - 1; listOff = constrain(listCur - 1, 0, max(0, trackTotal - 4)); goTo(SCR_TRACKS); break;
    case 2: goTo(SCR_VOL);      break;
    case 3: goTo(SCR_EQ);       break;
    case 4: goTo(SCR_GAMES);    break;
    case 5: goTo(SCR_SETTINGS); break;
    case 6: goTo(SCR_ABOUT);    break;
  }
}

// Dispatches button events to the active screen.
// T, M, B are the return values of readBtn() — 0=short, 1=long, -1=none.
// Returns immediately if the device is locked or no buttons were pressed.
void handleInput(int T, int M, int B) {
  if (locked) return;
  if (T < 0 && M < 0 && B < 0) return;
  lastActivity = millis();

  switch (scr) {
    // ── Now Playing ───────────────────────────────────────────────
    case SCR_PLAY: {
      if (T == 0) {
        int t2;
        if (shuffle) { do { t2 = random(1, trackTotal + 1); } while (t2 == track && trackTotal > 1); }
        else { t2 = track + 1; if (trackTotal > 0 && t2 > trackTotal) t2 = 1; }
        playTrack(t2);
      }
      if (M == 0) {
        if (playing) { if (dfReady) df.pause(); playing = false; pausedAt = millis(); }
        else         { if (dfReady) df.start(); playing = true;  pausedAccum += millis() - pausedAt; }
        dirty = true;
      }
      if (M == 1) goTo(SCR_MENU);
      if (B == 0) {
        int t2;
        if (shuffle) { do { t2 = random(1, trackTotal + 1); } while (t2 == track && trackTotal > 1); }
        else { t2 = track - 1; if (t2 < 1) t2 = trackTotal > 0 ? trackTotal : 1; }
        playTrack(t2);
      }
      break;
    }
    // ── Main Menu ─────────────────────────────────────────────────
    case SCR_MENU:
      if (T == 0) { menuCur = (menuCur - 1 + MENU_N) % MENU_N; dirty = true; }
      if (M == 1) goTo(SCR_PLAY);
      if (M == 0) menuSelect();
      if (B == 0) { menuCur = (menuCur + 1) % MENU_N; dirty = true; }
      break;

    // ── Browse Tracks ─────────────────────────────────────────────
    // Long press TOP/BOT skips by a proportional jump for large libraries.
    case SCR_TRACKS: {
      int jump = max(1, (trackTotal / 10 / 5) * 5);
      if (T == 0) { if (listCur > 0) { listCur--; dirty = true; } }
      if (T == 1) { listCur = max(0, listCur - jump); dirty = true; }
      if (M == 1) goTo(SCR_MENU);
      if (M == 0) { playTrack(listCur + 1); goTo(SCR_PLAY); }
      if (B == 0) { if (trackTotal == 0 || listCur < trackTotal - 1) { listCur++; dirty = true; } }
      if (B == 1) { listCur = min(trackTotal > 0 ? trackTotal - 1 : 0, listCur + jump); dirty = true; }
      break;
    }
    // ── Volume ────────────────────────────────────────────────────
    case SCR_VOL:
      if (T == 0 || T == 1) { vol = constrain(vol + 1, 0, 30); if (dfReady) df.volume(vol); dirty = true; }
      if (M == 0 || M == 1) { saveSettings(); goTo(SCR_MENU); }
      if (B == 0 || B == 1) { vol = constrain(vol - 1, 0, 30); if (dfReady) df.volume(vol); dirty = true; }
      break;

    // ── Equalizer ─────────────────────────────────────────────────
    case SCR_EQ:
      if (T == 0) { eqIdx = (eqIdx + 1 + EQ_N) % EQ_N; dirty = true; }
      if (M == 0 || M == 1) { if (dfReady) df.EQ(EQ_VALS[eqIdx]); saveSettings(); goTo(SCR_MENU); }
      if (B == 0) { eqIdx = (eqIdx - 1 + EQ_N) % EQ_N; dirty = true; }
      break;

    // ── Games Menu ────────────────────────────────────────────────
    case SCR_GAMES:
      if (T == 0) { gameCur = (gameCur - 1 + 3) % 3; dirty = true; }
      if (B == 0) { gameCur = (gameCur + 1) % 3; dirty = true; }
      if (M == 0) {
        if      (gameCur == 0) goTo(SCR_BB_DIFF);
        else if (gameCur == 1) goTo(SCR_PP_DIFF);
        else                   goTo(SCR_SNAKE_DIFF);
      }
      if (M == 1) goTo(SCR_MENU);
      break;

    // ── Brick Breaker Difficulty ──────────────────────────────────
    case SCR_BB_DIFF:
      if (T == 0) { bbDifficulty = (bbDifficulty + 2) % 3; dirty = true; }
      if (B == 0) { bbDifficulty = (bbDifficulty + 1) % 3; dirty = true; }
      if (M == 0) goTo(SCR_BB);
      if (M == 1) goTo(SCR_GAMES);
      break;

    // ── Ping Pong Difficulty ──────────────────────────────────────
    case SCR_PP_DIFF:
      if (T == 0) { ppDifficulty = (ppDifficulty + 2) % 3; dirty = true; }
      if (B == 0) { ppDifficulty = (ppDifficulty + 1) % 3; dirty = true; }
      if (M == 0) goTo(SCR_PP);
      if (M == 1) goTo(SCR_GAMES);
      break;

    // ── Snake Difficulty ──────────────────────────────────────────
    case SCR_SNAKE_DIFF:
      if (T == 0) { snakeDifficulty = (snakeDifficulty + 2) % 3; dirty = true; }
      if (B == 0) { snakeDifficulty = (snakeDifficulty + 1) % 3; dirty = true; }
      if (M == 0) {
        snakeSpeedMs = SNAKE_DIFF_SPEEDS[snakeDifficulty];
        goTo(SCR_SNAKE);
      }
      if (M == 1) goTo(SCR_GAMES);
      break;

    // ── Settings ──────────────────────────────────────────────────
    case SCR_SETTINGS:
      if (T == 0) { setCur = (setCur - 1 + 4) % 4; dirty = true; }
      if (M == 1) { saveSettings(); goTo(SCR_MENU); }
      if (M == 0) {
        if      (setCur == 0) shuffle      = !shuffle;
        else if (setCur == 1) repeatMd     = (RepeatMode)((repeatMd + 1) % 2);
        else if (setCur == 2) autoPlay     = !autoPlay;
        else                  sleepTimeout = (sleepTimeout + 1) % 5;
        dirty = true;
      }
      if (B == 0) { setCur = (setCur + 1) % 4; dirty = true; }
      break;

    // ── About ─────────────────────────────────────────────────────
    case SCR_ABOUT:
      if (M == 0 || M == 1) goTo(SCR_MENU);
      break;
  }
}

// Handles continuous actions while TOP or BOT is held.
// Fires at 150ms intervals after the initial LONG_MS hold threshold.
//
// Now Playing: accelerating track skip —
//   0–5s held = skip by 1, 5–10s = skip by 10, 10s+ = skip by 100
// All other screens: scroll or adjust at a fixed 150ms interval.
void handleScrollRepeat(uint32_t now) {
  static uint32_t last      = 0;
  static uint32_t heldSince = 0;

  bool tDown = digitalRead(PIN_BTN_T) == LOW;
  bool bDown = digitalRead(PIN_BTN_B) == LOW;

  if (!tDown && !bDown) { heldSince = 0; return; }
  if (heldSince == 0) heldSince = now;
  if (now - heldSince < LONG_MS) return;  // wait for initial hold threshold
  if (now - last < 150) return;            // rate limit repeat actions
  last = now;

  if (scr == SCR_PLAY) {
    uint32_t held = now - heldSince;
    int jump = held < 5000 ? 1 : held < 10000 ? 10 : 100;
    if (tDown) playTrack(constrain(track + jump, 1, trackTotal > 0 ? trackTotal : 9999));
    if (bDown) playTrack(constrain(track - jump, 1, trackTotal > 0 ? trackTotal : 9999));
  } else if (scr == SCR_MENU) {
    if (tDown) { menuCur = (menuCur - 1 + MENU_N) % MENU_N; dirty = true; }
    if (bDown) { menuCur = (menuCur + 1) % MENU_N; dirty = true; }
  } else if (scr == SCR_TRACKS) {
    if (tDown) { if (listCur > 0) { listCur--; dirty = true; } }
    if (bDown) { if (trackTotal == 0 || listCur < trackTotal - 1) { listCur++; dirty = true; } }
  } else if (scr == SCR_SETTINGS) {
    if (tDown) { setCur = (setCur - 1 + 4) % 4; dirty = true; }
    if (bDown) { setCur = (setCur + 1) % 4; dirty = true; }
  } else if (scr == SCR_EQ) {
    if (tDown) { eqIdx = (eqIdx + 1 + EQ_N) % EQ_N; dirty = true; }
    if (bDown) { eqIdx = (eqIdx - 1 + EQ_N) % EQ_N; dirty = true; }
  } else if (scr == SCR_VOL) {
    if (tDown) { vol = constrain(vol + 1, 0, 30); if (dfReady) df.volume(vol); dirty = true; }
    if (bDown) { vol = constrain(vol - 1, 0, 30); if (dfReady) df.volume(vol); dirty = true; }
  }
}