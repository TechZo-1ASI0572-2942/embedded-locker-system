#include <WiFi.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <HX711.h>

#define RED_LED_PIN 18
#define GREEN_LED_PIN 5
#define HX_DT_PIN 35
#define HX_SCK_PIN 16

//HX711
HX711 scale;

// Define keypad layout and pins
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Define potentiometer pin
const int potPin = A0;

// Define servo settings
Servo myServo;
const int servoPin = 19;
const int unlockPosition = 90; // Set servo position to -90 degrees
const int lockPosition = 0;    // Set servo position to +90 degrees

// Define LCD settings
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
bool locked = true;
char enteredCode[5]; // To store the entered code
int codeIndex = 0;

// Timing variables for non-blocking delays
unsigned long lastActionTime = 0;
bool showingMessage = false;
bool ledState = false;
unsigned long ledStartTime = 0;
int currentState = 0; // 0=normal, 1=success, 2=error
unsigned long messageStartTime = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin(21, 22); 
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  scale.begin(HX_DT_PIN, HX_SCK_PIN);
  // Inicializar balanza una sola vez
  scale.set_scale(); // Puedes poner un valor de escala real aquí si sabes cuál es
  scale.tare(); // Establecer el peso actual como 0

  myServo.attach(servoPin);
  myServo.write(lockPosition);

  lcd.init();
  lcd.backlight();

  updateLockStatus();
}

void loop() {

  // Handle timed events (messages, LEDs, etc.)
  handleTimedEvents();

  // Handle keypad input
  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
  }
}

void handleTimedEvents() {
  unsigned long currentTime = millis();
  
  // Handle message display timeout
  if (showingMessage && (currentTime - messageStartTime >= 2000)) {
    showingMessage = false;
    updateLockStatus();
  }
  
  // Handle LED timeout
  if (ledState && (currentTime - ledStartTime >= 3000)) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    ledState = false;
  }
}

void updateLockStatus() {
  if (!showingMessage) {
    lcd.clear();
    if (locked) {
      lcd.print("Door Locked");
    } else {
      lcd.print("Door Unlocked");
    }
  }
}

void showMessage(String message, int duration = 2000) {
  lcd.clear();
  lcd.print(message);
  showingMessage = true;
  messageStartTime = millis();
}


void activateLED(int pin, unsigned long duration = 3000) {
  digitalWrite(pin, HIGH);
  ledState = true;
  ledStartTime = millis();
}

void handleKeypadInput(char key) {
  if (locked) {
    if (key == '#' && codeIndex > 0) { // Check for Enter key
      enteredCode[codeIndex] = '\0'; // Null-terminate the entered code
      
      if (strcmp(enteredCode, "1234") == 0) { // Check if the entered code is correct
        unlockDoor();
        showMessage("Door Unlocked");
        activateLED(GREEN_LED_PIN);
      } else {
        showMessage("Incorrect Pin!");
        activateLED(RED_LED_PIN);
      }
      
      // Clear entered code
      memset(enteredCode, 0, sizeof(enteredCode));
      codeIndex = 0;
      
    } else if (key == 'C' && codeIndex > 0) { // Check for 'C' key to delete
      codeIndex--;
      enteredCode[codeIndex] = '\0';
      lcd.setCursor(codeIndex, 1);
      lcd.print(' ');
      
    } else if (key != '#' && key != 'C' && codeIndex < sizeof(enteredCode) - 1) {
      enteredCode[codeIndex] = key;
      lcd.setCursor(codeIndex, 1);
      lcd.print('*');
      codeIndex++;
    }
  } else {
    if (key == '*') {
      if (scale.is_ready()) {
        long weight = scale.get_units(1); // Reduced from 10 to 1 for faster response
        if (weight > 0) {
          Serial.println(weight);
          myServo.write(0);
          lockDoor();
          showMessage("Door Locked");
          activateLED(RED_LED_PIN);
        }else{
          showMessage("Empty locker!");
        }
      }
    }
  }
}

void unlockDoor() {
  locked = false;
  myServo.write(unlockPosition);
}

void lockDoor() {
  locked = true;
  myServo.write(lockPosition);
}