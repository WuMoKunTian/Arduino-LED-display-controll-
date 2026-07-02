/*
    Arduino Uno + MAX7219 LED Matrix shock animation.

    Original behavior preserved from the 2016 sketch:
    - no pulse: clear the display
    - medium pulse: draw expanding procedural circle rings
    - strong pulse: play a fixed frame animation
*/

#include <LedControl.h>

const int SHOCK_PIN = 8;

const int DIN_PIN = 2;
const int CS_PIN = 3;
const int CLK_PIN = 4;

const int MATRIX_DEVICE = 0;
const int MATRIX_COUNT = 1;
const int MATRIX_WIDTH = 8;
const int MATRIX_HEIGHT = 8;
const int MATRIX_INTENSITY = 15;

const int SERIAL_BAUD_RATE = 9600;

const long NO_SHOCK_PULSE = 0;
const long CIRCLE_MIN_SHOCK_PULSE = 1000;
const long FRAME_ANIMATION_MIN_SHOCK_PULSE = 10000;

const unsigned long SHOCK_READ_DELAY_MS = 10;
const unsigned long CIRCLE_FRAME_DELAY_MS = 21;
const unsigned long FRAME_ANIMATION_DELAY_MS = 42;

const int RANDOM_CENTER_MARGIN = 2;
const int RANDOM_CENTER_MARGIN_SIDES = 2;
const int RANDOM_CENTER_MIN_OFFSET = 0;

const int CIRCLE_INITIAL_RADIUS = 0;
const int CIRCLE_INITIAL_CENTER = 0;

const float LED_HALF_SIZE = 0.5;

const byte LED_ON_BIT = 1;
const byte LED_OFF_BIT = 0;
const byte BIT_SHIFT_STEP = 1;

const int ANIMATION_FRAME_COUNT = 11;
const byte FRAME_ANIMATION[ANIMATION_FRAME_COUNT][MATRIX_HEIGHT] = {
    {
        B00000000,
        B00000000,
        B00000000,
        B00001000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
    },
    {
        B00000000,
        B00000000,
        B00001000,
        B00010100,
        B00001000,
        B00000000,
        B00000000,
        B00000000,
    },
    {
        B00000000,
        B00001000,
        B00010100,
        B00100010,
        B00010100,
        B00001000,
        B00000000,
        B00000000,
    },
    {
        B00001000,
        B00010100,
        B00100010,
        B01000001,
        B00100010,
        B00010100,
        B00001000,
        B00000000,
    },
    {
        B00010100,
        B00100010,
        B01100001,
        B10000000,
        B01000001,
        B00100010,
        B00010100,
        B00001000,
    },
    {
        B00100010,
        B01000001,
        B10000000,
        B10000000,
        B01000000,
        B00100001,
        B00010010,
        B00001100,
    },
    {
        B01000001,
        B10000000,
        B00000000,
        B00001000,
        B10000000,
        B01000000,
        B00100001,
        B00010010,
    },
    {
        B10000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B10000000,
        B01000000,
        B00100001,
    },
    {
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B10000000,
        B01000000,
    },
    {
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B10000000,
    },
    {
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
    },
};

struct CircleState
{
    int radius;
    int centerX;
    int centerY;
};

LedControl matrix = LedControl(DIN_PIN, CLK_PIN, CS_PIN, MATRIX_COUNT);
CircleState circle = {CIRCLE_INITIAL_RADIUS, CIRCLE_INITIAL_CENTER, CIRCLE_INITIAL_CENTER};

void clearMatrix()
{
    matrix.clearDisplay(MATRIX_DEVICE);
}

long readShockPulse()
{
    delay(SHOCK_READ_DELAY_MS);
    return pulseIn(SHOCK_PIN, HIGH);
}

void printShockPulse(long shockPulse)
{
    Serial.print("measurement = ");
    Serial.println(shockPulse);
    Serial.println("________________________________");
}

void displayFrame(const byte frame[MATRIX_HEIGHT])
{
    for (int row = 0; row < MATRIX_HEIGHT; row++)
    {
        matrix.setRow(MATRIX_DEVICE, row, frame[row]);
    }
}

void playFrameAnimation()
{
    for (int frameIndex = 0; frameIndex < ANIMATION_FRAME_COUNT; frameIndex++)
    {
        displayFrame(FRAME_ANIMATION[frameIndex]);
        delay(FRAME_ANIMATION_DELAY_MS);
    }
}

bool isInsideCircle(float x, float y, int centerX, int centerY, float radius)
{
    float dx = x - centerX;
    float dy = y - centerY;

    return (dx * dx + dy * dy < radius * radius);
}

bool isOutsideCircle(float x, float y, int centerX, int centerY, float radius)
{
    return !isInsideCircle(x, y, centerX, centerY, radius);
}

bool circleIntersectsLed(int row, int column, int centerX, int centerY, float radius)
{
    float x0 = row - LED_HALF_SIZE;
    float x1 = row + LED_HALF_SIZE;
    float y0 = column - LED_HALF_SIZE;
    float y1 = column + LED_HALF_SIZE;

    bool allCornersInside = (
        isInsideCircle(x0, y0, centerX, centerY, radius) &&
        isInsideCircle(x0, y1, centerX, centerY, radius) &&
        isInsideCircle(x1, y0, centerX, centerY, radius) &&
        isInsideCircle(x1, y1, centerX, centerY, radius)
    );

    bool allCornersOutside = (
        isOutsideCircle(x0, y0, centerX, centerY, radius) &&
        isOutsideCircle(x0, y1, centerX, centerY, radius) &&
        isOutsideCircle(x1, y0, centerX, centerY, radius) &&
        isOutsideCircle(x1, y1, centerX, centerY, radius)
    );

    return !allCornersInside && !allCornersOutside;
}

void drawCircleRing(int radiusStep, int centerX, int centerY)
{
    float radius = radiusStep - LED_HALF_SIZE;

    for (int row = 0; row < MATRIX_HEIGHT; row++)
    {
        byte rowValue = LED_OFF_BIT;

        for (int column = 0; column < MATRIX_WIDTH; column++)
        {
            rowValue = rowValue << BIT_SHIFT_STEP;

            if (circleIntersectsLed(row, column, centerX, centerY, radius))
            {
                rowValue = rowValue | LED_ON_BIT;
            }
        }

        matrix.setRow(MATRIX_DEVICE, row, rowValue);
    }
}

void randomCenter(CircleState &currentCircle)
{
    int centerRangeWidth = MATRIX_WIDTH - (RANDOM_CENTER_MARGIN * RANDOM_CENTER_MARGIN_SIDES);
    int centerRangeHeight = MATRIX_HEIGHT - (RANDOM_CENTER_MARGIN * RANDOM_CENTER_MARGIN_SIDES);

    currentCircle.centerX = RANDOM_CENTER_MARGIN + random(RANDOM_CENTER_MIN_OFFSET, centerRangeWidth);
    currentCircle.centerY = RANDOM_CENTER_MARGIN + random(RANDOM_CENTER_MIN_OFFSET, centerRangeHeight);
}

void updateCircleAnimation()
{
    if (circle.radius % MATRIX_WIDTH == CIRCLE_INITIAL_RADIUS)
    {
        randomCenter(circle);
        circle.radius = CIRCLE_INITIAL_RADIUS;
    }

    delay(CIRCLE_FRAME_DELAY_MS);
    drawCircleRing(circle.radius, circle.centerX, circle.centerY);
    circle.radius++;
}

void setup()
{
    pinMode(SHOCK_PIN, INPUT);

    matrix.shutdown(MATRIX_DEVICE, false);
    matrix.setIntensity(MATRIX_DEVICE, MATRIX_INTENSITY);
    clearMatrix();

    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("____________________Here we go!!_________________");
}

void loop()
{
    long shockPulse = readShockPulse();
    printShockPulse(shockPulse);

    if (shockPulse == NO_SHOCK_PULSE)
    {
        clearMatrix();
    }

    if (shockPulse > CIRCLE_MIN_SHOCK_PULSE && shockPulse <= FRAME_ANIMATION_MIN_SHOCK_PULSE)
    {
        updateCircleAnimation();
        return;
    }

    if (shockPulse > FRAME_ANIMATION_MIN_SHOCK_PULSE)
    {
        playFrameAnimation();
        return;
    }
}
