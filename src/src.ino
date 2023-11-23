#define ABG_IMPLEMENTATION
#define ABG_TIMER1
#define ABG_SYNC_PARK_ROW

#include "ArduboyG.h"
ArduboyGBase_Config<ABG_Mode::L4_Triplane> a;

#include "Arduboy2.h"
Arduboy2 arduboy;

struct Ball {
  float x, y;
  float vx, vy;
  bool enabled;
};

struct PowerUp {
  int x, y;
  bool enabled;
};

struct Brick {
  byte hp;
  bool enabled;
};

const int PADDLE_WIDTH = 16;
const int PADDLE_HEIGHT = 3;
const int BALL_SIZE = 2;
const int BALL_MAX = 4;
const int BRICK_WIDTH = 16;
const int BRICK_HEIGHT = 4;
const int BRICK_STARTING_ROWS = 4;
const int BRICK_SHIFT_ROWS = 6;
const int BRICK_ROWS = HEIGHT / BRICK_HEIGHT;
const int BRICK_COLUMNS = WIDTH / BRICK_WIDTH;
const int BRICK_SPAWN_CHANCE = 70;
const int POWERUP_MAX = 2;
const int POWERUP_CHANCE = 10;
const int POWERUP_SIZE = 4;

int paddleX;
// float ballX[BALL_MAX], ballY[BALL_MAX];
// float ballVelocityX[BALL_MAX], ballVelocityY[BALL_MAX];
Ball balls[BALL_MAX];
PowerUp powerUps[POWERUP_MAX];
int ballsInPlay;
Brick bricks[BRICK_ROWS][BRICK_COLUMNS];
int score = 0;
bool gameRunning = false;
bool gameOver = false;


void setup() {
  a.boot();
  a.startGray();
  resetGame();
}

void loop() {
  a.waitForNextPlane(BLACK);
  if (a.needsUpdate())
    update();
  render();
}

void update() {
  a.pollButtons();

  if (gameOver) {
    displayGameOverScreen();
    if (a.justPressed(A_BUTTON)) {
      resetGame();
      gameOver = false;
    }
  } else {
    if (!gameRunning) {
      if (a.justPressed(A_BUTTON)) {
        launchBall(0);
      }
    } else {
      movePaddle();
      movePowerUps();
      moveBall();
      checkCollisions();
      // checkWinCondition();
      checkGameOver();
    }
  } 
}

void render() {
  if (gameOver) {
    displayGameOverScreen();
  } else {
    drawBricks();
    drawPaddle();
    drawBalls();
    drawPowerUps();
    drawScore();
  }
}

void resetGame() {
  ballsInPlay = 1;
  paddleX = (WIDTH - PADDLE_WIDTH) / 2;
  balls[0].x = paddleX + PADDLE_WIDTH / 2;
  balls[0].y = HEIGHT - 10;
  balls[0].enabled = true;

  for (int i = 1; i < BALL_MAX; i++) {
    balls[i].enabled = false;
  }

  for (int i = 0; i < POWERUP_MAX; i++) {
    powerUps[i].enabled = false;
  }
  
  memset(bricks, false, sizeof(bricks));
  for (int i = 0; i < BRICK_STARTING_ROWS; i++) {
    for (int j = 0; j < BRICK_COLUMNS; j++) {
      bricks[i][j].enabled = true;
      bricks[i][j].hp = 1;
    }
  }
  score = 0;
  gameRunning = false;
}

void launchBall(int ball) {
  balls[ball].vx = (float)random(-10, 20) / 20.0; // Random horizontal direction
  balls[ball].vy = -0.5;            // Always upwards
  balls[ball].x = paddleX + PADDLE_WIDTH / 2;
  balls[ball].y = HEIGHT - 10;
  balls[ball].enabled = true;
  gameRunning = true;
}

void movePaddle() {
  int newPaddleX = paddleX;
  if (a.pressed(LEFT_BUTTON) && paddleX > 0) {
    newPaddleX -= 1;
  }
  if (a.pressed(RIGHT_BUTTON) && paddleX < (WIDTH - PADDLE_WIDTH)) {
    newPaddleX += 1;
  }

  bool collision = false;
  for (int j = 0; j < BRICK_COLUMNS; j++) {
    if (bricks[BRICK_ROWS - 1][j].enabled && newPaddleX <= (j + 1) * BRICK_WIDTH - 1 && newPaddleX + PADDLE_WIDTH >= j * BRICK_WIDTH) {
      collision = true;
      return;
    }
  }
  if (!collision) {
    paddleX = newPaddleX;
  }
}

void moveBall() {
  for (int i = 0; i < BALL_MAX; i++) {
    if (!balls[i].enabled)
      continue;

    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;

    // Wall collision
    if (balls[i].x <= 0 || balls[i].x >= WIDTH - BALL_SIZE) {
      balls[i].vx = -balls[i].vx;
    }
    if (balls[i].y <= 0) {
      balls[i].vy = -balls[i].vy;
    }

    // Paddle collision
    if (balls[i].y >= HEIGHT - PADDLE_HEIGHT - BALL_SIZE && balls[i].x >= paddleX && balls[i].x <= paddleX + PADDLE_WIDTH) {
      balls[i].vy = -balls[i].vy;
      // Adjust ball angle based on collision point
      float hitPoint = (balls[i].x - (float)paddleX) - (PADDLE_WIDTH / 2.0);
      balls[i].vx = hitPoint / 4.0; // The further from center, the more angle
    }

    // Bottom screen collision
    if (balls[i].y >= HEIGHT) {
      if (ballsInPlay == 1) {
        gameOver = true;
      } else {
        ballsInPlay--;
        balls[i].enabled = false;
      }
    }
  }
}

void movePowerUps() {
  for (int i = 0; i < POWERUP_MAX; i++) {
    if (!powerUps[i].enabled)
      continue;

    powerUps[i].y++;
    if (powerUps[i].y > HEIGHT) {
      powerUps[i].enabled = false;
    }

    if (powerUps[i].x + POWERUP_SIZE > paddleX && powerUps[i].x < paddleX + PADDLE_WIDTH
        && powerUps[i].y + POWERUP_SIZE > HEIGHT - PADDLE_HEIGHT) {
      powerUps[i].enabled = false;
      for (int i = 0; i < BALL_MAX; i++) {
        if (balls[i].enabled)
          continue;
        else {
          launchBall(i);
          ballsInPlay++;
          break;
        }
      }
    }
  }
}

void checkCollisions() {
  for (int i = 0; i < BRICK_ROWS; i++) {
    for (int j = 0; j < BRICK_COLUMNS; j++) {
      if (bricks[i][j].enabled) {
        int brickX = j * BRICK_WIDTH;
        int brickY = i * BRICK_HEIGHT;
        for (int b = 0; b < BALL_MAX; b++) {
          if (balls[b].enabled && balls[b].x + BALL_SIZE >= brickX && balls[b].x <= brickX + BRICK_WIDTH &&
              balls[b].y + BALL_SIZE >= brickY && balls[b].y <= brickY + BRICK_HEIGHT) {
            
            // Bounce the ball
            balls[b].vy = -balls[b].vy;

            // Check brick not broken yet
            if (bricks[i][j].hp > 1) {
              bricks[i][j].hp--;
              continue;
            }

            // Break the brick
            bricks[i][j].enabled = false;
            score++;

            // Spawn powerup with POWERUP_CHANCE
            if (random(0, 100) < POWERUP_CHANCE) {
              for (int p = 0; p < POWERUP_MAX; p++) {
                if (powerUps[p].enabled)
                  continue;
                else {
                  powerUps[p].enabled = true;
                  powerUps[p].x = brickX + BRICK_WIDTH / 2 - POWERUP_SIZE / 2;
                  powerUps[p].y = brickY;
                  break;
                }
              }
            }

            if (i < BRICK_SHIFT_ROWS) {
              checkBrickRow(i);
            }
          }
        }
      }
    }
  }
}

void checkBrickRow(int row) {
  int bricksInRow = 0;
  for (int j = 0; j < BRICK_COLUMNS; j++) {
    if (bricks[row][j].enabled) {
      bricksInRow++;
    }
  }
  // If 75% of bricks are destroyed, shift all rows down
  if (bricksInRow <= BRICK_COLUMNS / 4) {
    shiftBricksDown();
  }
}

void shiftBricksDown() {
  for (int i = BRICK_ROWS - 1; i > 0; i--) {
    for (int j = 0; j < BRICK_COLUMNS; j++) {
      bricks[i][j].enabled = bricks[i - 1][j].enabled;
      bricks[i][j].hp = bricks[i - 1][j].hp;
    }
  }
  // Add new row at the top
  for (int j = 0; j < BRICK_COLUMNS; j++) {
    if (random(0, 100) < BRICK_SPAWN_CHANCE) {
      bricks[0][j].enabled = true;
      bricks[0][j].hp = 1 + random(0, 3);
    }
  }
  // Check for game over condition
  for (int j = 0; j < BRICK_COLUMNS; j++) {
    if (bricks[BRICK_ROWS - 1][j].enabled && paddleX <= (j + 1) * BRICK_WIDTH - 1 && paddleX + PADDLE_WIDTH >= j * BRICK_WIDTH) {
      gameOver = true;
      return;
    }
  }
}

void checkGameOver() {
  // for (int i = 0; i < BRICK_ROWS; i++) {
  //   if (bricks[i][0]) {
  //     if ((i * BRICK_HEIGHT) >= (HEIGHT - PADDLE_HEIGHT)) {
  //       gameOver = true;
  //       return;
  //     }
  //   }
  // }
}

void drawPaddle() {
  a.fillRect(paddleX, HEIGHT - PADDLE_HEIGHT, PADDLE_WIDTH, PADDLE_HEIGHT, LIGHT_GRAY);
}

void drawBalls() {
  for (int i = 0; i < BALL_MAX; i++) {
    if (!balls[i].enabled)
      continue;

    a.fillRect(balls[i].x, balls[i].y, BALL_SIZE, BALL_SIZE, DARK_GRAY);
    a.drawPixel(balls[i].x + 1, balls[i].y, WHITE);
  }
}

void drawPowerUps() {
  for (int i = 0; i < POWERUP_MAX; i++) {
    if (!powerUps[i].enabled)
      continue;

    a.fillRect(powerUps[i].x, powerUps[i].y, POWERUP_SIZE, POWERUP_SIZE, powerUps[i].y % 4);
  }
}

void drawBricks() {
  for (int i = 0; i < BRICK_ROWS; i++) {
    for (int j = 0; j < BRICK_COLUMNS; j++) {
      if (bricks[i][j].enabled) {
        a.drawRect(j * BRICK_WIDTH, i * BRICK_HEIGHT, BRICK_WIDTH, BRICK_HEIGHT, DARK_GRAY);
        a.drawRect(j * BRICK_WIDTH + 1, i * BRICK_HEIGHT + 1, BRICK_WIDTH - 2, BRICK_HEIGHT - 2, bricks[i][j].hp-1);
        // a.drawRect(j * BRICK_WIDTH, i * BRICK_HEIGHT, BRICK_WIDTH - 1, BRICK_HEIGHT - 1, LIGHT_GRAY);
        // a.drawFastHLine(j * BRICK_WIDTH + 1, i * BRICK_HEIGHT + 1, BRICK_WIDTH - 2, WHITE);
      }
    }
  }
}

void drawScore() {
  arduboy.setCursor(WIDTH - 30, 0);
  arduboy.print(score);
}

void displayGameOverScreen() {
  arduboy.setCursor(WIDTH / 2 - 12, HEIGHT / 2 - 4);
  arduboy.print(F("GAME OVER"));
  arduboy.setCursor(WIDTH / 2 - 12, HEIGHT / 2 + 4);
  arduboy.print(F("Score: "));
  arduboy.print(score);
}