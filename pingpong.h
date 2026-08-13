// ╔══════════════════════════════════════════════════════════════╗
// ║                      pingpong.h                              ║
// ║  Ping pong game for DIYPOD Shuffle. TOP=up, BOT=down,        ║
// ║  MID=pause, MID hold=exit to menu. First to 10 wins.         ║
// ║  Three difficulty settings affect AI speed and reaction.     ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"
#include "display.h"
#include "battery.h"

// ── Playfield ─────────────────────────────────────────────────────
#define PP_FIELD_X    3    // left edge of playfield in pixels
#define PP_FIELD_Y   15    // top edge of playfield in pixels
#define PP_FIELD_W  104    // playfield width in pixels
#define PP_FIELD_H   46    // playfield height in pixels
#define PP_WIN_SCORE 10    // first to this score wins

// ── Paddle ────────────────────────────────────────────────────────
#define PP_PADDLE_W    3
#define PP_PADDLE_H   14
#define PP_PADDLE_OFF  2                                              // distance from field edge
#define PP_PLAYER_X   (PP_FIELD_X + PP_PADDLE_OFF - 3)              // player paddle left edge X
#define PP_AI_X       (PP_FIELD_X + PP_FIELD_W - PP_PADDLE_OFF - PP_PADDLE_W)  // AI paddle left edge X

// ── Ball ──────────────────────────────────────────────────────────
#define PP_BALL_SIZE    2
#define PP_SPEED_EASY   1.0f   // pixels per frame per difficulty
#define PP_SPEED_MEDIUM 1.5f
#define PP_SPEED_HARD   2.0f
#define PP_SPEED_MAX    3.5f   // ball speed cap — increases slightly each rally

// ── AI ────────────────────────────────────────────────────────────
#define PP_AI_SPEED_EASY    0.6f   // AI paddle movement speed per difficulty
#define PP_AI_SPEED_MEDIUM  1.2f
#define PP_AI_SPEED_HARD    1.8f
#define PP_AI_REACT_EASY    300    // ms between AI target updates — simulates reaction time
#define PP_AI_REACT_MEDIUM  150
#define PP_AI_REACT_HARD     40

// ── Difficulty ────────────────────────────────────────────────────
#define PP_DIFF_EASY   0
#define PP_DIFF_MEDIUM 1
#define PP_DIFF_HARD   2

// ── State ─────────────────────────────────────────────────────────
// ppDifficulty is non-static so display.h and input.h can access it via extern.
uint8_t ppDifficulty = PP_DIFF_MEDIUM;

static int      ppPlayerScore  = 0;
static int      ppAIScore      = 0;
static int      ppHighStreak   = 0;   // best consecutive win streak ever recorded
static int      ppCurStreak    = 0;   // current rally win streak
static bool     ppPaused       = false;
static bool     ppBallLaunched = false;

static float    ppPlayerY;      // player paddle top Y
static float    ppAIY;          // AI paddle top Y
static float    ppBallX, ppBallY;
static float    ppBallDX, ppBallDY;
static float    ppBallSpeed;
static float    ppAISpeed;
static uint32_t ppAIReactTime = 0;  // millis() of last AI target update
static float    ppAITargetY;        // Y position AI paddle is moving toward

// ── Preferences ───────────────────────────────────────────────────
void ppLoadHighStreak() {
  prefs.begin("diypod", true);
  ppHighStreak = prefs.getInt("ppHS", 0);
  prefs.end();
}

void ppSaveHighStreak() {
  prefs.begin("diypod", false);
  prefs.putInt("ppHS", ppHighStreak);
  prefs.end();
}

// ── Reset ─────────────────────────────────────────────────────────
// Places the ball in the center with a direction based on who serves.
// Random Y component gives variety to each serve.
void ppResetBall(bool playerServes = true) {
  ppBallX  = PP_FIELD_X + PP_FIELD_W / 2.0f;
  ppBallY  = PP_FIELD_Y + PP_FIELD_H / 2.0f;
  ppBallDX = playerServes ? ppBallSpeed : -ppBallSpeed;
  ppBallDY = (random(0, 2) == 0 ? 1.0f : -1.0f) * ppBallSpeed * 0.5f;
  float len = sqrt(ppBallDX * ppBallDX + ppBallDY * ppBallDY);
  ppBallDX = ppBallDX / len * ppBallSpeed;
  ppBallDY = ppBallDY / len * ppBallSpeed;
  ppBallLaunched = false;
}

// fullReset = true: resets scores, streak, and speed. Used at game start.
// fullReset = false: resets only paddle and ball positions. Used after a point.
void ppReset(bool fullReset = true) {
  if (fullReset) {
    ppPlayerScore = 0;
    ppAIScore     = 0;
    ppCurStreak   = 0;
    switch (ppDifficulty) {
      case PP_DIFF_EASY:   ppBallSpeed = PP_SPEED_EASY;   ppAISpeed = PP_AI_SPEED_EASY;   break;
      case PP_DIFF_HARD:   ppBallSpeed = PP_SPEED_HARD;   ppAISpeed = PP_AI_SPEED_HARD;   break;
      default:             ppBallSpeed = PP_SPEED_MEDIUM; ppAISpeed = PP_AI_SPEED_MEDIUM; break;
    }
  }
  ppPlayerY = PP_FIELD_Y + (PP_FIELD_H - PP_PADDLE_H) / 2.0f;
  ppAIY     = PP_FIELD_Y + (PP_FIELD_H - PP_PADDLE_H) / 2.0f;
  ppPaused  = false;
  ppResetBall(true);
}

// ── Draw Helpers ──────────────────────────────────────────────────
void ppDrawPaddle(int x, float y, bool clear = false) {
  if (clear)
    oled.fillRect(x, (int)y, PP_PADDLE_W, PP_PADDLE_H, BLACK);
  else
    oled.fillRoundRect(x, (int)y, PP_PADDLE_W, PP_PADDLE_H, 1, WHITE);
}

void ppDrawBall(bool clear = false) {
  oled.fillRect((int)ppBallX, (int)ppBallY, PP_BALL_SIZE, PP_BALL_SIZE, clear ? BLACK : WHITE);
}

// ── Header ────────────────────────────────────────────────────────
// Shows player score on the left and AI score on the right.
// AI score shifts left automatically as it gains digits.
void ppDrawHeader() {
  oled.fillRect(0, 0, 128, 13, BLACK);
  oled.drawBitmap(0, 0, bmp_Border6, 109, 13, WHITE);
  oled.drawBitmap(110, 0, bmp_IconBatteryFrame, 18, 13, WHITE);

  int pct = getBatteryPct();
  if (pct < 0) {
    oled.drawLine(113, 2, 123, 8, WHITE);
    oled.drawLine(123, 2, 113, 8, WHITE);
  } else if (pct <= 20) {
    if ((millis() / 500) % 2 == 0) oled.fillRoundRect(112, 2, 3, 7, 2, WHITE);
  } else {
    int fillW = (int)(13.0f * pct / 100.0f);
    if (fillW > 0) oled.fillRoundRect(112, 2, fillW, 7, 2, WHITE);
  }

  oled.setTextSize(1);
  oled.setTextColor(WHITE);

  char pbuf[4]; snprintf(pbuf, 4, "%d", ppPlayerScore);
  char abuf[4]; snprintf(abuf, 4, "%d", ppAIScore);
  int  aScoreW = strlen(abuf) * 6;

  oled.setCursor(2, 2);
  oled.print(pbuf);
  oled.setCursor(100 - (aScoreW - 6), 2);
  oled.print(abuf);
}

// ── Hints ─────────────────────────────────────────────────────────
// paused = true shows play icon, holding = true shows return icon.
void ppDrawHints(bool paused, bool holding) {
  oled.fillRect(110, 14, 18, 50, BLACK);
  drawSidePanel();
  oled.drawBitmap(113, 18, bmp_HintUp,   10, 11, WHITE);
  if (holding)
    oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
  else if (paused)
    oled.drawBitmap(112, 32, bmp_HintPlay,   12, 12, WHITE);
  else
    oled.drawBitmap(112, 32, bmp_HintPause,  12, 12, WHITE);
  oled.drawBitmap(113, 47, bmp_HintDown, 10, 11, WHITE);
}

// ── Full Screen Draw ──────────────────────────────────────────────
void ppDrawScreen() {
  oled.clearDisplay();
  ppDrawHeader();
  drawSidePanel();
  drawMainPanel();
  ppDrawHints(false, false);
  ppDrawPaddle(PP_PLAYER_X, ppPlayerY);
  ppDrawPaddle(PP_AI_X, ppAIY);
  ppDrawBall();
  oled.display();
}

// ── Pause ─────────────────────────────────────────────────────────
// Blocking pause loop. Short MID press resumes, long MID press exits to menu.
void ppPause() {
  ppPaused = true;
  ppDrawHints(true, false);
  oled.display();

  uint32_t pressTime  = 0;
  bool     showReturn = false;

  while (true) {
    bool midDown = digitalRead(PIN_BTN_M) == LOW;
    if (midDown) {
      if (pressTime == 0) pressTime = millis();
      if (!showReturn && millis() - pressTime > LONG_MS / 2) {
        showReturn = true;
        ppDrawHints(true, true);
        oled.display();
      }
      if (millis() - pressTime > LONG_MS) {
        ppPaused = false;
        oled.fillRect(110, 14, 18, 50, BLACK);
        drawSidePanel();
        oled.display();
        goTo(SCR_GAMES);
        return;
      }
    } else {
      if (pressTime > 0) {
        ppPaused = false;
        ppDrawHints(false, false);
        oled.display();
        break;
      }
    }
    delay(10);
  }
  delay(200);
}

// ── Point Scored ──────────────────────────────────────────────────
// Updates scores and streak, flashes the display, then resets the ball.
// The scorer serves next.
void ppPointScored(bool playerScored) {
  if (playerScored) {
    ppPlayerScore++;
    ppCurStreak++;
    if (ppCurStreak > ppHighStreak) { ppHighStreak = ppCurStreak; ppSaveHighStreak(); }
  } else {
    ppAIScore++;
    ppCurStreak = 0;
  }

  oled.invertDisplay(true);  delay(60);
  oled.invertDisplay(false); delay(60);

  ppResetBall(playerScored);
  ppDrawHeader();
  oled.display();
  delay(500);
}

// ── Win Screen ────────────────────────────────────────────────────
// Shows the result, final score, and win streak.
// Short press retries, long press exits to menu.
void ppWinScreen(bool playerWon) {
  oled.clearDisplay();
  ppDrawHeader();
  drawSidePanel();
  drawMainPanel();

  oled.drawBitmap(114, 19, bmp_HintNull, 8, 8, WHITE);
  oled.drawBitmap(114, 34, bmp_HintNull, 8, 8, WHITE);
  oled.drawBitmap(114, 49, bmp_HintNull, 8, 8, WHITE);

  oled.setTextSize(1);
  oled.setTextColor(WHITE);

  oled.fillRoundRect(27, 16, 52, 12, 2, WHITE);
  oled.setTextColor(BLACK);
  oled.setCursor(30, 18);
  oled.print(playerWon ? F("YOU WIN!") : F("AI WINS!"));
  oled.setTextColor(WHITE);

  oled.setCursor(4, 32); oled.print(F("Score: "));       oled.print(ppPlayerScore); oled.print(F(" - ")); oled.print(ppAIScore);
  oled.setCursor(4, 42); oled.print(F("Win streak: "));  oled.print(ppCurStreak > 0 ? ppCurStreak : 0);
  oled.setCursor(4, 52); oled.print(F("Best streak: ")); oled.print(ppHighStreak);
  oled.display();

  delay(500);
  uint32_t pressTime  = 0;
  bool     showReturn = false;
  while (true) {
    bool anyDown = digitalRead(PIN_BTN_T) == LOW ||
                   digitalRead(PIN_BTN_M) == LOW ||
                   digitalRead(PIN_BTN_B) == LOW;
    if (anyDown) {
      if (pressTime == 0) pressTime = millis();
      if (!showReturn && millis() - pressTime > LONG_MS / 2) {
        showReturn = true;
        oled.fillRect(110, 14, 18, 50, BLACK);
        drawSidePanel();
        oled.drawBitmap(114, 19, bmp_HintNull,   8,  8, WHITE);
        oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
        oled.drawBitmap(114, 49, bmp_HintNull,   8,  8, WHITE);
        oled.display();
      }
      if (millis() - pressTime > LONG_MS) {
        goTo(SCR_GAMES);
        while (digitalRead(PIN_BTN_T) == LOW ||
               digitalRead(PIN_BTN_M) == LOW ||
               digitalRead(PIN_BTN_B) == LOW) { delay(10); }
        delay(200);
        return;
      }
    } else {
      if (pressTime > 0) break;
      pressTime = 0;
    }
    delay(10);
  }
  delay(200);
}

// ── AI Update ─────────────────────────────────────────────────────
// AI tracks the ball with a delayed target update to simulate reaction time.
// On Easy, it occasionally aims at a random Y to simulate misses.
void ppUpdateAI(uint32_t now) {
  int reactMs;
  switch (ppDifficulty) {
    case PP_DIFF_EASY:   reactMs = PP_AI_REACT_EASY;   break;
    case PP_DIFF_HARD:   reactMs = PP_AI_REACT_HARD;   break;
    default:             reactMs = PP_AI_REACT_MEDIUM; break;
  }

  if (now - ppAIReactTime > (uint32_t)reactMs) {
    ppAIReactTime = now;
    ppAITargetY   = ppBallY - PP_PADDLE_H / 2.0f;
    if (ppDifficulty == PP_DIFF_EASY && random(0, 4) == 0)
      ppAITargetY = PP_FIELD_Y + random(0, PP_FIELD_H - PP_PADDLE_H);  // intentional miss
  }

  float aiPaddleCX = ppAIY + PP_PADDLE_H / 2.0f;
  float targetCX   = ppAITargetY + PP_PADDLE_H / 2.0f;

  if      (aiPaddleCX < targetCX - 1) ppAIY += ppAISpeed;
  else if (aiPaddleCX > targetCX + 1) ppAIY -= ppAISpeed;

  ppAIY = constrain(ppAIY, PP_FIELD_Y + 1, PP_FIELD_Y + PP_FIELD_H - PP_PADDLE_H - 1);
}

// ── Main Update ───────────────────────────────────────────────────
// Called every loop iteration when scr == SCR_PP.
// Handles input, physics, collision, scoring, and win detection.
void ppUpdate(int T, int M, int B) {
  static bool     ppInitDone   = false;
  static uint32_t lastFrame    = 0;
  static bool     showExitHint = false;

  // Initialise on first entry
  if (!ppInitDone) {
    ppLoadHighStreak();
    ppReset(true);
    ppDrawScreen();
    lastFrame  = millis();
    ppInitDone = true;
    return;
  }

  if (scr != SCR_PP) { ppInitDone = false; return; }

  // MID short = pause, MID long = exit
  if (M == 0) {
    ppPause();
    if (scr != SCR_PP) { ppInitDone = false; return; }
    lastFrame = millis();
    return;
  }
  if (M == 1) {
    ppInitDone = false;
    while (digitalRead(PIN_BTN_T) == LOW ||
           digitalRead(PIN_BTN_M) == LOW ||
           digitalRead(PIN_BTN_B) == LOW) { delay(10); }
    delay(200);
    goTo(SCR_GAMES);
    return;
  }

  // Show return hint while MID is held during gameplay
  bool midHeld = digitalRead(PIN_BTN_M) == LOW;
  if (midHeld && !showExitHint && !ppPaused) {
    showExitHint = true;
    ppDrawHints(false, true);
    oled.display();
  } else if (!midHeld && showExitHint) {
    showExitHint = false;
    ppDrawHints(false, false);
    oled.display();
  }

  // Frame cap ~60fps
  uint32_t now = millis();
  if (now - lastFrame < 16) return;
  lastFrame = now;

  // ── Player paddle movement ────────────────────────────────────
  ppDrawPaddle(PP_PLAYER_X, ppPlayerY, true);
  if (T == 0 || digitalRead(PIN_BTN_T) == LOW) {
    ppPlayerY -= 2.0f;
    if (ppPlayerY < PP_FIELD_Y + 1) ppPlayerY = PP_FIELD_Y + 1;
  }
  if (B == 0 || digitalRead(PIN_BTN_B) == LOW) {
    ppPlayerY += 2.0f;
    if (ppPlayerY + PP_PADDLE_H > PP_FIELD_Y + PP_FIELD_H - 1)
      ppPlayerY = PP_FIELD_Y + PP_FIELD_H - PP_PADDLE_H - 1;
  }
  ppDrawPaddle(PP_PLAYER_X, ppPlayerY);

  // ── AI paddle movement ────────────────────────────────────────
  ppDrawPaddle(PP_AI_X, ppAIY, true);
  ppUpdateAI(now);
  ppDrawPaddle(PP_AI_X, ppAIY);

  // ── Ball pre-launch ───────────────────────────────────────────
  // Ball sits still until TOP or BOT is pressed.
  if (!ppBallLaunched) {
    ppDrawBall();
    if (T == 0 || B == 0) ppBallLaunched = true;
    oled.display();
    return;
  }

  // ── Ball movement ─────────────────────────────────────────────
  // Erase ball, redraw paddles to restore any erased pixels, then move.
  ppDrawBall(true);
  ppDrawPaddle(PP_PLAYER_X, ppPlayerY);
  ppDrawPaddle(PP_AI_X, ppAIY);
  ppBallX += ppBallDX;
  ppBallY += ppBallDY;

  // Top/bottom wall collisions
  if (ppBallY <= PP_FIELD_Y + 1) {
    ppBallY  = PP_FIELD_Y + 1;
    ppBallDY = abs(ppBallDY);
  }
  if (ppBallY + PP_BALL_SIZE >= PP_FIELD_Y + PP_FIELD_H - 1) {
    ppBallY  = PP_FIELD_Y + PP_FIELD_H - PP_BALL_SIZE - 1;
    ppBallDY = -abs(ppBallDY);
  }

  // Player paddle collision — angle varies based on hit position
  if (ppBallDX < 0 &&
      ppBallX <= PP_PLAYER_X + PP_PADDLE_W &&
      ppBallX >= PP_PLAYER_X &&
      ppBallY + PP_BALL_SIZE >= ppPlayerY &&
      ppBallY <= ppPlayerY + PP_PADDLE_H) {
    ppBallDX = abs(ppBallDX);
    float hitPos = constrain((ppBallY + PP_BALL_SIZE / 2.0f - ppPlayerY) / PP_PADDLE_H, 0.1f, 0.9f);
    ppBallDY    = ppBallSpeed * (hitPos - 0.5f) * 2.0f;
    float len   = sqrt(ppBallDX * ppBallDX + ppBallDY * ppBallDY);
    ppBallDX    = ppBallDX / len * ppBallSpeed;
    ppBallDY    = ppBallDY / len * ppBallSpeed;
    ppBallSpeed = min(PP_SPEED_MAX, ppBallSpeed + 0.05f);  // slight speed increase per rally
  }

  // AI paddle collision
  if (ppBallDX > 0 &&
      ppBallX + PP_BALL_SIZE >= PP_AI_X &&
      ppBallX + PP_BALL_SIZE <= PP_AI_X + PP_PADDLE_W + 2 &&
      ppBallY + PP_BALL_SIZE >= ppAIY &&
      ppBallY <= ppAIY + PP_PADDLE_H) {
    ppBallDX = -abs(ppBallDX);
    float hitPos = constrain((ppBallY + PP_BALL_SIZE / 2.0f - ppAIY) / PP_PADDLE_H, 0.1f, 0.9f);
    ppBallDY    = ppBallSpeed * (hitPos - 0.5f) * 2.0f;
    float len   = sqrt(ppBallDX * ppBallDX + ppBallDY * ppBallDY);
    ppBallDX    = ppBallDX / len * ppBallSpeed;
    ppBallDY    = ppBallDY / len * ppBallSpeed;
    ppBallSpeed = min(PP_SPEED_MAX, ppBallSpeed + 0.05f);
  }

  // ── Scoring ───────────────────────────────────────────────────
  // Ball past player paddle = AI scores. Ball past AI paddle = player scores.
  if (ppBallX < PP_PLAYER_X - 1) {
    ppPointScored(false);
    if (ppAIScore >= PP_WIN_SCORE) {
      ppWinScreen(false);
      ppInitDone = false;
      ppReset(true);
      ppDrawScreen();
      lastFrame = millis();
      return;
    }
    lastFrame = millis();
    return;
  }
  if (ppBallX + PP_BALL_SIZE > PP_AI_X + 5) {
    ppPointScored(true);
    if (ppPlayerScore >= PP_WIN_SCORE) {
      ppWinScreen(true);
      ppInitDone = false;
      ppReset(true);
      ppDrawScreen();
      lastFrame = millis();
      return;
    }
    lastFrame = millis();
    return;
  }

  ppDrawBall();
  ppDrawHeader();
  drawMainPanel();
  oled.display();
}
