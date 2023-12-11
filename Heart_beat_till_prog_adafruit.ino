#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT  64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define GRAPH_WIDTH   SCREEN_WIDTH
#define GRAPH_HEIGHT  SCREEN_HEIGHT - 10 // Leave some space for labels

#define HISTORY_SIZE  100 // Number of historical values to store
#define DISPLAY_VALUES  50 // Number of values to display on the x-axis
#define UPDATE_DELAY   200 // Delay between each iteration in milliseconds
#define LOCK_DURATION  5000 // Duration (in milliseconds) to lock the y-axis if values are consistent

int sensorHistory[HISTORY_SIZE];
int historyIndex = 0;

int highestValue;
int lowestValue;

unsigned long lastUpdateMillis = 0;
bool lockYAxis = false;

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ; // Don't proceed, loop forever
  }

  // Other setup code as needed
  delay(1000);
}

void loop() {
  // Clear the display at the beginning of each loop
  display.clearDisplay();

  // Read pulse sensor data
  int sensorValue = analogRead(A0); // Change A0 to the correct pin if needed

  // Store the current sensor value in the history array
  sensorHistory[historyIndex] = sensorValue;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  // Update the highest and lowest values within the current history window
  updateMinMaxValues();

  // Calculate the centering offsets
  int xOffset = SCREEN_WIDTH / 2 - GRAPH_WIDTH / 2;
  int yOffset = SCREEN_HEIGHT / 2 + 20; // Move down by 20 pixels

  // Draw the history values on the graph with the applied scale factor and centering
  for (int i = 0; i < DISPLAY_VALUES; i++) {
    int arrayIndex = map(i, 0, DISPLAY_VALUES, 0, HISTORY_SIZE);
    int x = map(i, 0, DISPLAY_VALUES, 0, GRAPH_WIDTH);
    int y = map(sensorHistory[(historyIndex + arrayIndex) % HISTORY_SIZE], lowestValue, highestValue, GRAPH_HEIGHT, 0);

    // Move by 1 pixel for each unit difference in sensor readings
    y = yOffset - y;

    // Apply centering offsets
    x += xOffset;

    display.drawPixel(x, y, WHITE);
  }

  // Draw a line connecting all the points with the applied scale factor and centering
  for (int i = 1; i < DISPLAY_VALUES; i++) {
    int arrayIndex1 = map(i - 1, 0, DISPLAY_VALUES, 0, HISTORY_SIZE);
    int arrayIndex2 = map(i, 0, DISPLAY_VALUES, 0, HISTORY_SIZE);
    int x1 = map(i - 1, 0, DISPLAY_VALUES, 0, GRAPH_WIDTH);
    int y1 = map(sensorHistory[(historyIndex + arrayIndex1) % HISTORY_SIZE], lowestValue, highestValue, GRAPH_HEIGHT, 0);
    int x2 = map(i, 0, DISPLAY_VALUES, 0, GRAPH_WIDTH);
    int y2 = map(sensorHistory[(historyIndex + arrayIndex2) % HISTORY_SIZE], lowestValue, highestValue, GRAPH_HEIGHT, 0);

    // Move by 1 pixel for each unit difference in sensor readings
    y1 = yOffset - y1;
    y2 = yOffset - y2;

    // Apply centering offsets
    x1 += xOffset;
    x2 += xOffset;

    // Draw the line segment
    display.drawLine(x1, y1, x2, y2, WHITE);
  }

  // Show the display buffer on the screen
  display.display();

  // Print the sensor reading to Serial
  Serial.println(sensorValue);

  // Check if the y-axis should be locked
  if (millis() - lastUpdateMillis > LOCK_DURATION && isValuesConsistent()) {
    lockYAxis = true;
  }

  // Update the last update time
  lastUpdateMillis = millis();

  // Delay for a longer interval
  delay(UPDATE_DELAY);
}

// Function to update the highest and lowest values within the current history window
void updateMinMaxValues() {
  highestValue = sensorHistory[0];
  lowestValue = sensorHistory[0];
  for (int i = 1; i < HISTORY_SIZE; i++) {
    int value = sensorHistory[(historyIndex + i) % HISTORY_SIZE];
    if (value > highestValue) {
      highestValue = value;
    }
    if (value < lowestValue) {
      lowestValue = value;
    }
  }
}

// Function to check if values are consistent for a specified duration
bool isValuesConsistent() {
  int referenceValue = sensorHistory[historyIndex];
  for (int i = 1; i < HISTORY_SIZE; i++) {
    int value = sensorHistory[(historyIndex + i) % HISTORY_SIZE];
    if (abs(value - referenceValue) > 10) { // Adjust the threshold as needed
      return false;
    }
  }
  return true;
}