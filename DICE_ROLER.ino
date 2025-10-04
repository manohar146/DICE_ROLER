#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
#define BUTTON_PIN 14
#define BUZZER_PIN 25
#define LED_PIN    26

bool buttonPressed = false;
unsigned long lastInputTime = 0;
bool inScreensaver = false;
const int idleTimeout = 15000;

// 8x8 star bitmap for screensaver
const unsigned char starBitmap[] PROGMEM = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
  0b00000000
};

// Dice dot layout (3x3) - true = dot present
bool diceDots[6][9] = {
  {0,0,0, 0,1,0, 0,0,0},  // 1
  {1,0,0, 0,0,0, 0,0,1},  // 2
  {1,0,0, 0,1,0, 0,0,1},  // 3
  {1,0,1, 0,0,0, 1,0,1},  // 4
  {1,0,1, 0,1,0, 1,0,1},  // 5
  {1,0,1, 1,0,1, 1,0,1}   // 6
};

// Draw rotated square (approximated angular effect)
void drawRotatedSquare(int cx, int cy, int size, bool rotate) {
  int half = size / 2;

  if (!rotate) {
    display.drawRect(cx - half, cy - half, size, size, WHITE);
  } else {
    // Rotate corners by 45° approx (square to diamond)
    int x0 = cx;
    int y0 = cy - half;
    int x1 = cx + half;
    int y1 = cy;
    int x2 = cx;
    int y2 = cy + half;
    int x3 = cx - half;
    int y3 = cy;

    display.drawLine(x0, y0, x1, y1, WHITE);
    display.drawLine(x1, y1, x2, y2, WHITE);
    display.drawLine(x2, y2, x3, y3, WHITE);
    display.drawLine(x3, y3, x0, y0, WHITE);
  }
}

// Draw single dice face with optional rotation
void drawDiceFace(int number, bool rotate = false) {
  display.clearDisplay();
  int diceSize = 40;
  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2;

  // Draw square or rotated border
  drawRotatedSquare(cx, cy, diceSize, rotate);

  // Dot spacing and radius
  int spacing = 12;
  int dotRadius = 3;
  int dx[] = {-spacing, 0, spacing};
  int dy[] = {-spacing, 0, spacing};

  for (int i = 0; i < 9; i++) {
    if (diceDots[number - 1][i]) {
      int row = i / 3;
      int col = i % 3;

      int x = cx + dx[col];
      int y = cy + dy[row];

      // Apply 45° rotation (approximation)
      if (rotate) {
        int temp = x;
        x = cx + (y - cy);
        y = cy + (temp - cx);
      }

      display.fillCircle(x, y, dotRadius, WHITE);
    }
  }

  display.display();
}

// Screensaver
void showScreensaver() {
  static int yOffset = -8;
  display.clearDisplay();
  for (int i = 0; i < 6; i++) {
    int x = 10 + i * 20;
    int y = (yOffset + i * 7) % SCREEN_HEIGHT;
    display.drawBitmap(x, y, starBitmap, 8, 8, WHITE);
  }
  display.display();
  yOffset += 2;
  if (yOffset > SCREEN_HEIGHT) yOffset = -8;
  delay(50);
}

// Rolling animation with border rotation
void animateDiceRoll() {
  Serial.println("Rolling dice...");
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);

  for (int i = 0; i < 10; i++) {
    int temp = random(1, 7);
    bool rotate = (i % 2 == 1);  // Alternate rotation
    drawDiceFace(temp, rotate);
    delay(100);
  }

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

// Welcome screen
void showWelcomeScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  for (int y = -8; y < SCREEN_HEIGHT + 8; y += 2) {
    display.clearDisplay();
    for (int i = 0; i < 6; i++) {
      int x = 10 + i * 20;
      int yOffset = (y + i * 7) % SCREEN_HEIGHT;
      display.drawBitmap(x, yOffset, starBitmap, 8, 8, WHITE);
    }
    display.display();
    delay(50);
  }

  display.clearDisplay();
  display.setCursor(20, 25);
  display.println("  Welcome to");
  display.setCursor(20, 40);
  display.println("  Dice Roller");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("Dice Roller starting...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (1);
  }

  randomSeed(analogRead(0));
  showWelcomeScreen();
  drawDiceFace(1);
  lastInputTime = millis();
}

void loop() {
  bool state = digitalRead(BUTTON_PIN);

  if (inScreensaver && state == HIGH) {
    Serial.println("Exiting screensaver...");
    inScreensaver = false;
    drawDiceFace(1);
    delay(300);
  }

  if (inScreensaver) {
    showScreensaver();
    return;
  }

  if (state == HIGH && !buttonPressed) {
    buttonPressed = true;
    lastInputTime = millis();

    animateDiceRoll();
    int result = random(1, 7);
    drawDiceFace(result);
    Serial.print("Final Dice Face: ");
    Serial.println(result);
  }

  if (state == LOW && buttonPressed) {
    buttonPressed = false;
  }

  if (!buttonPressed && millis() - lastInputTime > idleTimeout) {
    Serial.println("Entering screensaver...");
    inScreensaver = true;
  }
}
