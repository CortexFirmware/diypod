// ╔══════════════════════════════════════════════════════════════╗
// ║                    brickbreaker.h                            ║
// ║  Classic brick breaker for DIYPOD Shuffle. TOP=right,        ║
// ║  BOT=left, MID=pause, MID hold=exit to menu.                 ║
// ║  Score and lives shown in header. High score saved to flash. ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"
#include "display.h"
#include "battery.h"

// ── Playfield ─────────────────────────────────────────────────────
#define BB_FIELD_X    3    // left edge of playfield in pixels
#define BB_FIELD_Y   15    // top edge of playfield in pixels
#define BB_FIELD_W  104    // playfield width in pixels
#define BB_FIELD_H   47    // playfield height in pixels

// ── Bricks ────────────────────────────────────────────────────────
// Bricks are 8x4px with 1px gaps. Grid offset places them inside the border.
#define BB_BRICK_COLS  11
#define BB_BRICK_ROWS   5
#define BB_BRICK_W      8
#define BB_BRICK_H      4
#define BB_BRICK_GAP    1
#define BB_BRICK_OFF_X  1  // horizontal offset of grid from field left edge
#define BB_BRICK_OFF_Y  1  // vertical offset of grid from field top edge

// ── Paddle ────────────────────────────────────────────────────────
#define BB_PADDLE_Y      (BB_FIELD_Y + BB_FIELD_H - 5)  // Y position of paddle top
#define BB_PADDLE_H       3
#define BB_PADDLE_EASY   22   // paddle width per difficulty
#define BB_PADDLE_MEDIUM 20
#define BB_PADDLE_HARD   18

// ── Ball ──────────────────────────────────────────────────────────
#define BB_BALL_SIZE      2
#define BB_SPEED_EASY   0.8f   // pixels per frame per difficulty
#define BB_SPEED_MEDIUM 1.2f
#define BB_SPEED_HARD   1.6f
#define BB_SPEED_MAX    2.5f   // ball speed cap across all levels

// ── Difficulty ────────────────────────────────────────────────────
#define BB_DIFF_EASY   0
#define BB_DIFF_MEDIUM 1
#define BB_DIFF_HARD   2

// ── Lives ─────────────────────────────────────────────────────────
#define BB_LIVES 3

// ── Power-up ──────────────────────────────────────────────────────
#define BB_PUP_SPEED   0.5f  // pixels per frame the heart falls
#define BB_PUP_CHANCE   20   // 1 in N chance of a power-up dropping from a destroyed brick

struct BBPowerUp {
  float x, y;
  bool  active;
};
static BBPowerUp bbPup = { 0, 0, false };

// ── State ─────────────────────────────────────────────────────────
// bbDifficulty is non-static so display.h and input.h can access it via extern.
uint8_t bbDifficulty = BB_DIFF_MEDIUM;

static int     bbScore        = 0;
static int     bbHighScore    = 0;
static int     bbLevel        = 1;
static uint8_t bbLives        = BB_LIVES;
static bool    bbPaused       = false;
static bool    bbBallLaunched = false;
static int     bbCombo        = 0;  // consecutive bricks hit without touching paddle
static int     bbPaddleW      = BB_PADDLE_MEDIUM;

// Brick hit points: 0 = empty, 1 = one hit, 2 = two hits, 3 = three hits
static uint8_t bbBricks[BB_BRICK_ROWS][BB_BRICK_COLS];

// Ball position and velocity
static float bbBallX, bbBallY;
static float bbBallDX, bbBallDY;
static float bbBallSpeed;

// Paddle position (left edge x)
static float bbPaddleX;

// ── Preferences ───────────────────────────────────────────────────
void bbLoadHighScore() {
  prefs.begin("diypod", true);
  bbHighScore = prefs.getInt("bbHS", 0);
  prefs.end();
}

void bbSaveHighScore() {
  prefs.begin("diypod", false);
  prefs.putInt("bbHS", bbHighScore);
  prefs.end();
}

// ── Brick Layout ──────────────────────────────────────────────────
// Generates brick hit points for the current level and difficulty.
// Higher levels and harder difficulties introduce more multi-hit bricks.
void bbGenerateLevel() {
  for (int r = 0; r < BB_BRICK_ROWS; r++) {
    for (int c = 0; c < BB_BRICK_COLS; c++) {
      int maxHits = 1;
      if (bbDifficulty == BB_DIFF_MEDIUM) {
        maxHits = (bbLevel > 2 && r < 2) ? 2 : 1;
      } else if (bbDifficulty == BB_DIFF_HARD) {
        maxHits = (r < 1) ? 3 : (r < 3) ? 2 : 1;
      }
      // Random chance of adding an extra hit point from level 4 onward
      if (bbLevel > 3 && random(0, 4) == 0) maxHits = min(3, maxHits + 1);
      bbBricks[r][c] = maxHits;
    }
  }
}

// ── Ball Launch ───────────────────────────────────────────────────
// Launches the ball from the paddle center at a slight diagonal angle.
void bbLaunchBall() {
  bbBallX  = bbPaddleX + bbPaddleW / 2.0f;
  bbBallY  = BB_PADDLE_Y - BB_BALL_SIZE - 1;
  bbBallDX = 0.6f;
  bbBallDY = -1.0f;
  float len = sqrt(bbBallDX * bbBallDX + bbBallDY * bbBallDY);
  bbBallDX = bbBallDX / len * bbBallSpeed;
  bbBallDY = bbBallDY / len * bbBallSpeed;
  bbBallLaunched = true;
}

// ── Reset ─────────────────────────────────────────────────────────
// fullReset = true: resets everything including score, lives, and level.
// fullReset = false: resets only ball and paddle position (used after losing a life).
void bbReset(bool fullReset = true) {
  if (fullReset) {
    bbScore  = 0;
    bbLives  = BB_LIVES;
    bbLevel  = 1;
    bbCombo  = 0;
    switch (bbDifficulty) {
      case BB_DIFF_EASY:   bbPaddleW = BB_PADDLE_EASY;   bbBallSpeed = BB_SPEED_EASY;   break;
      case BB_DIFF_HARD:   bbPaddleW = BB_PADDLE_HARD;   bbBallSpeed = BB_SPEED_HARD;   break;
      default:             bbPaddleW = BB_PADDLE_MEDIUM; bbBallSpeed = BB_SPEED_MEDIUM; break;
    }
  }
  bbPaddleX      = BB_FIELD_X + (BB_FIELD_W - bbPaddleW) / 2.0f;
  bbBallLaunched = false;
  bbBallX        = bbPaddleX + bbPaddleW / 2.0f;
  bbBallY        = BB_PADDLE_Y - BB_BALL_SIZE - 1;
  bbPaused       = false;
  bbPup.active   = false;
}

// Advances to the next level — increases ball speed slightly and regenerates bricks.
void bbNextLevel() {
  bbLevel++;
  bbBallSpeed = min(BB_SPEED_MAX, bbBallSpeed + 0.3f);
  bbGenerateLevel();
  bbReset(false);
}

// ── Draw Helpers ──────────────────────────────────────────────────

// Draws or erases a single brick based on its current hit points.
// hp 1 = outline, hp 2 = outline + one fill line, hp 3 = outline + two fill lines.
void bbDrawBrick(int r, int c) {
  int x     = BB_FIELD_X + BB_BRICK_OFF_X + c * (BB_BRICK_W + BB_BRICK_GAP);
  int y     = BB_FIELD_Y + BB_BRICK_OFF_Y + r * (BB_BRICK_H + BB_BRICK_GAP);
  uint8_t hp = bbBricks[r][c];
  if (hp == 0) { oled.fillRect(x, y, BB_BRICK_W, BB_BRICK_H, BLACK); return; }
  oled.drawRect(x, y, BB_BRICK_W, BB_BRICK_H, WHITE);
  if (hp >= 2) oled.fillRect(x + 2, y + 1, BB_BRICK_W - 4, 1, WHITE);
  if (hp >= 3) oled.fillRect(x + 2, y + 2, BB_BRICK_W - 4, 1, WHITE);
}

// Draws or clears the paddle. Clear uses a slightly larger rect to clean up
// rounded corner pixels left by fillRoundRect.
void bbDrawPaddle(bool clear = false) {
  if (clear)
    oled.fillRect((int)bbPaddleX - 1, BB_PADDLE_Y - 1, bbPaddleW + 2, BB_PADDLE_H + 2, BLACK);
  else
    oled.fillRoundRect((int)bbPaddleX, BB_PADDLE_Y, bbPaddleW, BB_PADDLE_H, 1, WHITE);
}

void bbDrawBall(bool clear = false) {
  oled.fillRect((int)bbBallX, (int)bbBallY, BB_BALL_SIZE, BB_BALL_SIZE, clear ? BLACK : WHITE);
}

// Draws or clears the power-up heart. When clearing, redraws any bricks
// that overlap the erased area to prevent them from being wiped.
void bbDrawPup(bool clear = false) {
  if (!bbPup.active) return;
  int px = (int)bbPup.x;
  int py = (int)bbPup.y;
  if (clear) {
    oled.fillRect(px, py, 7, 6, BLACK);
    for (int r = 0; r < BB_BRICK_ROWS; r++) {
      for (int c = 0; c < BB_BRICK_COLS; c++) {
        int bx = BB_FIELD_X + BB_BRICK_OFF_X + c * (BB_BRICK_W + BB_BRICK_GAP);
        int by = BB_FIELD_Y + BB_BRICK_OFF_Y + r * (BB_BRICK_H + BB_BRICK_GAP);
        if (bx < px + 7 && bx + BB_BRICK_W > px &&
            by < py + 6 && by + BB_BRICK_H > py)
          bbDrawBrick(r, c);
      }
    }
  } else {
    oled.drawBitmap(px, py, bmp_IconHeart, 7, 6, WHITE);
  }
}

// ── Header ────────────────────────────────────────────────────────
// Draws the game-specific header with score, battery indicator, and
// a heart icon + lives count that shifts left as digits increase.
void bbDrawHeader() {
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

  oled.setCursor(2, 2);
  oled.print(F("SCORE:"));
  oled.print(bbScore);

  // Lives — heart bitmap followed by digit, both shift left as digits grow
  char livesBuf[3];
  snprintf(livesBuf, 3, "%d", bbLives);
  int livesW = strlen(livesBuf) * 6;
  int heartX = 106 - livesW - 7 - 2;
  oled.drawBitmap(heartX, 3, bmp_IconHeart, 7, 6, WHITE);
  oled.setCursor(heartX + 9, 2);
  oled.print(livesBuf);
}

// ── Hints ─────────────────────────────────────────────────────────
// paused = true shows play icon, holding = true shows return icon.
void bbDrawHints(bool paused, bool holding) {
  oled.fillRect(110, 14, 18, 50, BLACK);
  drawSidePanel();
  oled.drawBitmap(112, 20, bmp_HintRight, 12, 8,  WHITE);
  if (holding)
    oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
  else if (paused)
    oled.drawBitmap(112, 32, bmp_HintPlay,   12, 12, WHITE);
  else
    oled.drawBitmap(112, 32, bmp_HintPause,  12, 12, WHITE);
  oled.drawBitmap(112, 48, bmp_HintLeft, 12, 8, WHITE);
}

// ── Full Screen Draw ──────────────────────────────────────────────
// Used on init, level clear, and after losing a life to redraw everything.
void bbDrawScreen() {
  oled.clearDisplay();
  bbDrawHeader();
  drawSidePanel();
  drawMainPanel();
  bbDrawHints(false, false);
  for (int r = 0; r < BB_BRICK_ROWS; r++)
    for (int c = 0; c < BB_BRICK_COLS; c++)
      bbDrawBrick(r, c);
  bbDrawPaddle();
  bbDrawBall();
  oled.display();
}

// ── Brick Collision ───────────────────────────────────────────────
// Checks all bricks for overlap with the ball. On hit: decrements hp,
// updates score and combo, reflects ball, and may spawn a power-up.
// Only one brick is processed per frame to keep physics predictable.
bool bbBrickCollision() {
  bool hit = false;
  for (int r = 0; r < BB_BRICK_ROWS; r++) {
    for (int c = 0; c < BB_BRICK_COLS; c++) {
      if (bbBricks[r][c] == 0) continue;
      int bx = BB_FIELD_X + BB_BRICK_OFF_X + c * (BB_BRICK_W + BB_BRICK_GAP);
      int by = BB_FIELD_Y + BB_BRICK_OFF_Y + r * (BB_BRICK_H + BB_BRICK_GAP);

      if ((int)bbBallX + BB_BALL_SIZE > bx &&
          (int)bbBallX < bx + BB_BRICK_W &&
          (int)bbBallY + BB_BALL_SIZE > by &&
          (int)bbBallY < by + BB_BRICK_H) {

        bbBricks[r][c]--;
        bbCombo++;
        int points = (10 * bbLevel) * bbCombo;
        if (bbDifficulty == BB_DIFF_HARD)   points *= 3;
        if (bbDifficulty == BB_DIFF_MEDIUM) points *= 2;
        bbScore = min(bbScore + points, 9999990);
        if (bbScore > bbHighScore) bbHighScore = bbScore;

        bbDrawBrick(r, c);

        // 1 in BB_PUP_CHANCE chance of dropping a power-up on full brick destroy
        if (bbBricks[r][c] == 0 && !bbPup.active && random(0, BB_PUP_CHANCE) == 0) {
          bbPup.x = bx + (BB_BRICK_W - 7) / 2.0f;
          bbPup.y = by + BB_BRICK_H;
          bbPup.active = true;
        }

        // Reflect ball — use dominant axis to determine direction
        float centerBallX = bbBallX + BB_BALL_SIZE / 2.0f;
        float centerBallY = bbBallY + BB_BALL_SIZE / 2.0f;
        float brickCX     = bx + BB_BRICK_W / 2.0f;
        float brickCY     = by + BB_BRICK_H / 2.0f;
        if (abs(centerBallX - brickCX) / BB_BRICK_W > abs(centerBallY - brickCY) / BB_BRICK_H)
          bbBallDX = -bbBallDX;
        else
          bbBallDY = -bbBallDY;

        hit = true;
        break;
      }
    }
    if (hit) break;
  }
  return hit;
}

// Returns true when all bricks have been cleared.
bool bbAllBricksCleared() {
  for (int r = 0; r < BB_BRICK_ROWS; r++)
    for (int c = 0; c < BB_BRICK_COLS; c++)
      if (bbBricks[r][c] > 0) return false;
  return true;
}

// ── Pause ─────────────────────────────────────────────────────────
// Blocking pause loop. Short MID press resumes, long MID press exits to menu.
void bbPause() {
  bbPaused = true;
  bbDrawHints(true, false);
  oled.display();

  uint32_t pressTime  = 0;
  bool     showReturn = false;

  while (true) {
    bool midDown = digitalRead(PIN_BTN_M) == LOW;
    if (midDown) {
      if (pressTime == 0) pressTime = millis();
      if (!showReturn && millis() - pressTime > LONG_MS / 2) {
        showReturn = true;
        bbDrawHints(true, true);
        oled.display();
      }
      if (millis() - pressTime > LONG_MS) {
        bbPaused = false;
        oled.fillRect(110, 14, 18, 50, BLACK);
        drawSidePanel();
        oled.display();
        goTo(SCR_GAMES);
        return;
      }
    } else {
      if (pressTime > 0) {
        bbPaused = false;
        bbDrawHints(false, false);
        oled.display();
        break;
      }
    }
    delay(10);
  }
  delay(200);
}

// ── Life Lost ─────────────────────────────────────────────────────
void bbLostBall() {
  bbLives--;
  bbCombo = 0;
  bbDrawPup(true);       // erase power-up from display before reset
  bbPup.active = false;

  oled.invertDisplay(true);  delay(60);
  oled.invertDisplay(false); delay(60);

  if (bbLives <= 0) return;  // game over handled by caller

  bbDrawPaddle(true);
  drawMainPanel();
  bbReset(false);
  bbDrawHeader();
  bbDrawPaddle();
  bbDrawBall();
  oled.display();
}

// ── Game Over ─────────────────────────────────────────────────────
void bbGameOver() {
  bool newHigh = bbScore > 0 && bbScore >= bbHighScore;
  bbHighScore  = max(bbHighScore, bbScore);
  bbSaveHighScore();

  for (int i = 0; i < 3; i++) {
    oled.invertDisplay(true);  delay(60);
    oled.invertDisplay(false); delay(60);
  }

  oled.clearDisplay();
  bbDrawHeader();
  drawSidePanel();
  drawMainPanel();

  oled.drawBitmap(114, 19, bmp_HintNull, 8, 8, WHITE);
  oled.drawBitmap(114, 34, bmp_HintNull, 8, 8, WHITE);
  oled.drawBitmap(114, 49, bmp_HintNull, 8, 8, WHITE);

  oled.setTextSize(1);
  oled.fillRoundRect(25, 16, 59, 12, 2, WHITE);
  oled.setTextColor(BLACK);
  oled.setCursor(28, 18); oled.print(F("GAME OVER"));
  oled.setTextColor(WHITE);
  oled.setCursor(4, 30); oled.print(F("Score: ")); oled.print(bbScore);
  oled.setCursor(4, 40); oled.print(F("Best:  ")); oled.print(bbHighScore);
  if (newHigh) { oled.setCursor(4, 50); oled.print(F("NEW BEST!")); }
  oled.display();

  // Wait for button input — short press retries, long press exits to menu
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

// ── Main Update ───────────────────────────────────────────────────
// Called every loop iteration when scr == SCR_BB.
// Handles input, physics, collision, power-ups, and end conditions.
void bbUpdate(int T, int M, int B) {
  static bool     bbInitDone   = false;
  static uint32_t lastFrame    = 0;
  static bool     showExitHint = false;

  // Initialise on first entry
  if (!bbInitDone) {
    bbLoadHighScore();
    bbReset(true);
    bbGenerateLevel();
    bbDrawScreen();
    lastFrame  = millis();
    bbInitDone = true;
    return;
  }

  if (scr != SCR_BB) { bbInitDone = false; return; }

  // MID short = pause, MID long = exit
  if (M == 0) {
    bbPause();
    if (scr != SCR_BB) { bbInitDone = false; return; }
    lastFrame = millis();
    return;
  }
  if (M == 1) {
    bbInitDone = false;
    while (digitalRead(PIN_BTN_T) == LOW ||
           digitalRead(PIN_BTN_M) == LOW ||
           digitalRead(PIN_BTN_B) == LOW) { delay(10); }
    delay(200);
    goTo(SCR_GAMES);
    return;
  }

  // Show return hint while MID is held during gameplay
  bool midHeld = digitalRead(PIN_BTN_M) == LOW;
  if (midHeld && !showExitHint && !bbPaused) {
    showExitHint = true;
    bbDrawHints(false, true);
    oled.display();
  } else if (!midHeld && showExitHint) {
    showExitHint = false;
    bbDrawHints(false, false);
    oled.display();
  }

  // Frame cap ~60fps
  uint32_t now = millis();
  if (now - lastFrame < 16) return;
  lastFrame = now;

  // ── Paddle movement ───────────────────────────────────────────
  float paddleSpeed = 2.5f;
  bbDrawPaddle(true);
  if (T == 0 || digitalRead(PIN_BTN_T) == LOW) {
    bbPaddleX += paddleSpeed;
    if (bbPaddleX + bbPaddleW > BB_FIELD_X + BB_FIELD_W - 2)
      bbPaddleX = BB_FIELD_X + BB_FIELD_W - bbPaddleW - 2;
  }
  if (B == 0 || digitalRead(PIN_BTN_B) == LOW) {
    bbPaddleX -= paddleSpeed;
    if (bbPaddleX < BB_FIELD_X - 1) bbPaddleX = BB_FIELD_X - 1;
  }
  bbDrawPaddle();
  bbDrawBall(true);

  // ── Ball pre-launch ───────────────────────────────────────────
  // Ball follows paddle center until any button is pressed to launch.
  if (!bbBallLaunched) {
    bbBallX = bbPaddleX + bbPaddleW / 2.0f;
    bbDrawBall(true);
    bbDrawBall();
    if (digitalRead(PIN_BTN_T) == LOW ||
        digitalRead(PIN_BTN_B) == LOW ||
        digitalRead(PIN_BTN_M) == LOW) bbLaunchBall();
    oled.display();
    return;
  }

  // ── Ball movement ─────────────────────────────────────────────
  bbDrawBall(true);
  bbDrawPaddle();  // restore paddle pixels that may have been erased by ball clear
  bbBallX += bbBallDX;
  bbBallY += bbBallDY;

  // Wall collisions
  if (bbBallX <= BB_FIELD_X) {
    bbBallX  = BB_FIELD_X;
    bbBallDX = abs(bbBallDX);
  }
  if (bbBallX + BB_BALL_SIZE >= BB_FIELD_X + BB_FIELD_W) {
    bbBallX  = BB_FIELD_X + BB_FIELD_W - BB_BALL_SIZE;
    bbBallDX = -abs(bbBallDX);
  }
  if (bbBallY <= BB_FIELD_Y) {
    bbBallY  = BB_FIELD_Y;
    bbBallDY = abs(bbBallDY);
  }

  // Paddle collision — angle of reflection varies based on hit position
  if (bbBallY + BB_BALL_SIZE >= BB_PADDLE_Y &&
      bbBallY + BB_BALL_SIZE <= BB_PADDLE_Y + BB_PADDLE_H + 2 &&
      bbBallX + BB_BALL_SIZE >= bbPaddleX &&
      bbBallX <= bbPaddleX + bbPaddleW) {
    bbCombo = 0;  // reset combo on paddle touch
    bbBallDY = -abs(bbBallDY);
    float hitPos = (bbBallX + BB_BALL_SIZE / 2.0f - bbPaddleX) / bbPaddleW;
    hitPos   = constrain(hitPos, 0.05f, 0.95f);
    bbBallDX = bbBallSpeed * (hitPos - 0.5f) * 2.0f;
    float len = sqrt(bbBallDX * bbBallDX + bbBallDY * bbBallDY);
    bbBallDX = bbBallDX / len * bbBallSpeed;
    bbBallDY = bbBallDY / len * bbBallSpeed;
  }

  bbBrickCollision();

  // ── Power-up ──────────────────────────────────────────────────
  if (bbPup.active) {
    bbDrawPup(true);
    bbPup.y += BB_PUP_SPEED;

    if ((int)bbPup.y + 6 >= BB_PADDLE_Y &&
        (int)bbPup.x + 7 >= bbPaddleX &&
        (int)bbPup.x <= bbPaddleX + bbPaddleW) {
      // Caught — award an extra life (max 9)
      bbPup.active = false;
      bbLives      = min(9, bbLives + 1);
      bbDrawHeader();
    } else if (bbPup.y > BB_FIELD_Y + BB_FIELD_H) {
      bbPup.active = false;  // fell past paddle
    } else {
      bbDrawPup();
    }
  }

  // ── End conditions ────────────────────────────────────────────
  if (bbBallY > BB_FIELD_Y + BB_FIELD_H) {
    bbLostBall();
    if (bbLives <= 0) {
      bbGameOver();
      bbInitDone = false;
      bbReset(true);
      bbGenerateLevel();
      bbDrawScreen();
      lastFrame = millis();
      return;
    }
    lastFrame = millis();
    return;
  }

  if (bbAllBricksCleared()) {
    oled.invertDisplay(true);  delay(100);
    oled.invertDisplay(false); delay(100);
    oled.invertDisplay(true);  delay(100);
    oled.invertDisplay(false);
    bbNextLevel();
    bbDrawScreen();
    lastFrame = millis();
    return;
  }

  drawMainPanel();
  bbDrawBall();
  bbDrawHeader();
  oled.display();
}