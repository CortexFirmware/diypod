// ╔══════════════════════════════════════════════════════════════╗
// ║                       snake.h                                ║
// ║  Classic snake game for DIYPOD Shuffle. TOP turns right,     ║
// ║  BOT turns left, MID pauses, MID hold exits to menu.         ║
// ║  Score shown in header. High score saved to Preferences.     ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"
#include "display.h"
#include "battery.h"

// ── Game Constants ────────────────────────────────────────────────
// Each cell is 2x2 pixels. Playfield sits inside Border3 interior.
#define SNAKE_OFFSET_X     4    // pixel X of the playfield left edge
#define SNAKE_OFFSET_Y    16    // pixel Y of the playfield top edge
#define SNAKE_GAME_W      50    // playfield width in cells
#define SNAKE_GAME_H      22    // playfield height in cells
#define SNAKE_MAX_LENGTH 256    // maximum snake body length in cells
#define SNAKE_START_SIZE   3    // starting snake length

#define SNAKE_SPEED_EASY   150  // ms per game tick per difficulty
#define SNAKE_SPEED_MEDIUM 100
#define SNAKE_SPEED_HARD    60

// ── Types ─────────────────────────────────────────────────────────

// SnakePosition represents a grid cell coordinate.
// Supports equality comparison and in-place addition for movement.
struct SnakePosition {
  int8_t x, y;
  bool operator==(const SnakePosition& o) const { return x == o.x && y == o.y; }
  SnakePosition& operator+=(const SnakePosition& o) { x += o.x; y += o.y; return *this; }
};

// Direction vectors: up, right, down, left
const SnakePosition kSnakeDirs[] = { {0,-1}, {1,0}, {0,1}, {-1,0} };

// ── Player ────────────────────────────────────────────────────────
// SnakePlayer stores the head position, direction, and a packed tail history.
// The tail array encodes the direction taken at each step as 2 bits,
// packed 4 directions per byte — allowing the full body to be reconstructed
// for drawing and self-collision detection using pixel sampling.
struct SnakePlayer {
  SnakePosition pos;
  uint8_t       tail[SNAKE_MAX_LENGTH];
  uint8_t       direction;
  int           size, moved;

  void reset() {
    pos       = { SNAKE_GAME_W / 2, SNAKE_GAME_H / 2 };
    direction = 1;  // start moving right
    size      = SNAKE_START_SIZE;
    memset(tail, 0, sizeof(tail));
    moved = 0;
  }
  void turnLeft()  { direction = (direction + 3) % 4; }
  void turnRight() { direction = (direction + 1) % 4; }

  // Shifts the tail history and advances the head by one cell.
  void update() {
    for (int i = SNAKE_MAX_LENGTH - 1; i > 0; --i)
      tail[i] = tail[i] << 2 | ((tail[i-1] >> 6) & 3);
    tail[0] = tail[0] << 2 | ((direction + 2) % 4);
    pos += kSnakeDirs[direction];
    if (moved < size) moved++;
  }
} snakePlayer;

struct SnakeItem { SnakePosition pos; } snakeItem;

// ── State ─────────────────────────────────────────────────────────
static bool snakePaused    = false;
static int  snakeHighScore = 0;
static int  snakeScore     = 0;

// snakeDifficulty and snakeSpeedMs are non-static so input.h can access them via extern.
uint8_t  snakeDifficulty = 1;
uint32_t snakeSpeedMs    = SNAKE_SPEED_MEDIUM;

const char* SNAKE_DIFF_LABELS[] = { "EASY", "MEDIUM", "HARD" };
const int   SNAKE_DIFF_SPEEDS[] = { SNAKE_SPEED_EASY, SNAKE_SPEED_MEDIUM, SNAKE_SPEED_HARD };
const int   SNAKE_DIFF_POINTS[] = { 1, 10, 100 };  // score per food eaten per difficulty

// ── Preferences ───────────────────────────────────────────────────
void snakeLoadHighScore() {
  prefs.begin("diypod", true);
  snakeHighScore = prefs.getInt("snakeHS", 0);
  prefs.end();
}

void snakeSaveHighScore(int score) {
  prefs.begin("diypod", false);
  prefs.putInt("snakeHS", score);
  prefs.end();
}

// ── Cell Draw ─────────────────────────────────────────────────────

// Draws or erases a 2x2 pixel cell at the given grid position.
void snakeDrawCell(SnakePosition pos, int color = WHITE) {
  oled.fillRect(SNAKE_OFFSET_X + pos.x * 2, SNAKE_OFFSET_Y + pos.y * 2, 2, 2, color);
}

// Returns true if the pixel at the given grid cell is WHITE.
// Used for self-collision detection — reads directly from the display buffer.
bool snakeCheckBody(SnakePosition pos) {
  return oled.getPixel(SNAKE_OFFSET_X + pos.x * 2, SNAKE_OFFSET_Y + pos.y * 2) == WHITE;
}

// ── Header ────────────────────────────────────────────────────────
// Game-specific header — shows score in place of the scrolling title.
void snakeDrawHeader() {
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

  char buf[6]; snprintf(buf, 6, "%d", snakeScore);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(2, 2);
  oled.print(F("SCORE:"));
  oled.print(buf);
}

// ── Hints ─────────────────────────────────────────────────────────
// paused = true shows play icon, holding = true shows return icon.
void snakeDrawHints(bool paused, bool holding) {
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

// ── Item Spawn ────────────────────────────────────────────────────
// Spawns food at a random position not occupied by the snake body.
// Keeps food away from edges until the snake is long enough to fill
// the inner area, then allows the full playfield.
void snakeSpawnItem() {
  do {
    if (snakePlayer.size > 333) {
      snakeItem.pos.x = random(0, SNAKE_GAME_W);
      snakeItem.pos.y = random(0, SNAKE_GAME_H);
    } else {
      snakeItem.pos.x = random(1, SNAKE_GAME_W - 1);
      snakeItem.pos.y = random(1, SNAKE_GAME_H - 1);
    }
  } while (snakeCheckBody(snakeItem.pos) ||
           (snakeItem.pos.x == snakePlayer.pos.x &&
            snakeItem.pos.y == snakePlayer.pos.y));
}

// ── Screen Draw ───────────────────────────────────────────────────
// Draws the UI frame — header, hints, and border. Does not clear the
// playfield so existing snake and food pixels are preserved.
void snakeDrawScreen(bool paused = false, bool holding = false) {
  snakeDrawHeader();
  snakeDrawHints(paused, holding);
  drawMainPanel();
}

// ── Render ────────────────────────────────────────────────────────
// Draws the new head position and erases the tail tip to simulate movement.
// Only erases the tail once the snake has grown to its full starting size.
void snakeRenderPlayer() {
  snakeDrawCell(snakePlayer.pos);
  if (snakePlayer.moved < snakePlayer.size) return;
  SnakePosition tailPos = snakePlayer.pos;
  for (int i = 0; i < snakePlayer.size; ++i)
    tailPos += kSnakeDirs[(snakePlayer.tail[i >> 2] >> ((i & 3) * 2)) & 3];
  snakeDrawCell(tailPos, BLACK);
}

void snakeRenderItem() {
  snakeDrawCell(snakeItem.pos);
}

// ── Reset ─────────────────────────────────────────────────────────
// Full game reset — clears the display, resets state, and draws the initial frame.
void snakeReset() {
  snakeScore  = 0;
  snakePaused = false;
  oled.clearDisplay();
  snakePlayer.reset();
  snakeDrawScreen();
  oled.fillRect(SNAKE_OFFSET_X, SNAKE_OFFSET_Y, SNAKE_GAME_W * 2, SNAKE_GAME_H * 2, BLACK);
  snakeSpawnItem();
  snakeRenderItem();
  oled.display();
}

// ── Pause ─────────────────────────────────────────────────────────
// Blocking pause loop. Short MID press resumes, long MID press exits to menu.
void snakePause() {
  snakePaused = true;
  snakeDrawHints(true, false);
  oled.display();

  uint32_t pressTime  = 0;
  bool     showReturn = false;

  while (true) {
    bool midDown = digitalRead(PIN_BTN_M) == LOW;
    if (midDown) {
      if (pressTime == 0) pressTime = millis();
      if (!showReturn && millis() - pressTime > LONG_MS / 2) {
        showReturn = true;
        snakeDrawHints(true, true);
        oled.display();
      }
      if (millis() - pressTime > LONG_MS) {
        snakePaused = false;
        oled.fillRect(110, 14, 18, 50, BLACK);
        drawSidePanel();
        oled.display();
        goTo(SCR_GAMES);
        return;
      }
    } else {
      if (pressTime > 0) {
        snakePaused = false;
        snakeDrawHints(false, false);
        snakeRenderPlayer();
        snakeRenderItem();
        oled.display();
        break;
      }
    }
    delay(10);
  }
  delay(200);
}

// ── Game Over ─────────────────────────────────────────────────────
// Flashes the display, shows score and high score, waits for input.
// Short press retries, long press exits to menu.
void snakeGameOver() {
  bool newHigh = snakeScore > snakeHighScore;
  if (newHigh) { snakeHighScore = snakeScore; snakeSaveHighScore(snakeScore); }

  for (int i = 0; i < 3; i++) {
    oled.invertDisplay(true);  delay(60);
    oled.invertDisplay(false); delay(60);
  }

  oled.clearDisplay();
  snakeDrawHeader();
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
  oled.setCursor(4, 30); oled.print(F("Score: ")); oled.print(snakeScore);
  oled.setCursor(4, 40); oled.print(F("Best:  ")); oled.print(snakeHighScore);
  if (newHigh) { oled.setCursor(4, 50); oled.print(F("NEW BEST!")); }
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

// ── Main Update ───────────────────────────────────────────────────
// Called every loop iteration when scr == SCR_SNAKE.
// Handles input, game ticks, collision detection, and end conditions.
void snakeUpdate(int T, int M, int B) {
  static uint32_t lastTick      = 0;
  static bool     snakeInitDone = false;

  // Initialise on first entry
  if (!snakeInitDone) {
    snakeLoadHighScore();
    snakeReset();
    lastTick      = millis();
    snakeInitDone = true;
    return;
  }

  if (scr != SCR_SNAKE) { snakeInitDone = false; return; }

  // MID short = pause, MID long = exit
  if (M == 0) {
    snakePause();
    if (scr != SCR_SNAKE) { snakeInitDone = false; return; }
    lastTick = millis();
    return;
  }
  if (M == 1) {
    snakeInitDone = false;
    goTo(SCR_GAMES);
    return;
  }

  // Show return hint while MID is held during gameplay
  static bool showExitHint = false;
  bool midHeld = digitalRead(PIN_BTN_M) == LOW;
  if (midHeld && !showExitHint && !snakePaused) {
    showExitHint = true;
    oled.fillRect(110, 14, 18, 50, BLACK);
    drawSidePanel();
    oled.drawBitmap(112, 20, bmp_HintRight,  12, 8,  WHITE);
    oled.drawBitmap(112, 32, bmp_HintReturn, 12, 12, WHITE);
    oled.drawBitmap(112, 48, bmp_HintLeft,   12, 8,  WHITE);
    oled.display();
  } else if (!midHeld && showExitHint) {
    showExitHint = false;
    oled.fillRect(110, 14, 18, 50, BLACK);
    drawSidePanel();
    oled.drawBitmap(112, 20, bmp_HintRight,  12, 8,  WHITE);
    oled.drawBitmap(112, 32, bmp_HintPause,  12, 12, WHITE);
    oled.drawBitmap(112, 48, bmp_HintLeft,   12, 8,  WHITE);
    oled.display();
  }

  // TOP = turn right, BOT = turn left
  if (T == 0) snakePlayer.turnRight();
  if (B == 0) snakePlayer.turnLeft();

  // ── Game tick ─────────────────────────────────────────────────
  uint32_t now = millis();
  if (now - lastTick >= snakeSpeedMs) {
    lastTick = now;
    snakePlayer.update();

    if (snakePlayer.pos == snakeItem.pos) {
      // Food eaten — grow and spawn new food
      snakePlayer.size++;
      snakeScore += SNAKE_DIFF_POINTS[snakeDifficulty];
      snakeSpawnItem();
    } else if (
      snakePlayer.pos.x < -1 || snakePlayer.pos.x >= SNAKE_GAME_W ||
      snakePlayer.pos.y <  0  || snakePlayer.pos.y >= SNAKE_GAME_H ||
      (snakePlayer.moved >= snakePlayer.size && snakeCheckBody(snakePlayer.pos))
    ) {
      // Wall or self collision — game over
      snakeGameOver();
      snakeInitDone = false;
      snakeReset();
      lastTick = millis();
      return;
    }

    snakeRenderPlayer();
    snakeRenderItem();
    snakeDrawHeader();  // redraws score and battery every tick
    oled.display();
  }
}
