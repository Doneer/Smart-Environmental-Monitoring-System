#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2       //Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  //DHT 11
#define LDR_PIN A0     //Analog pin for light sensor
#define JOYSTICK_X A1  //Joystick X-axis
#define JOYSTICK_Y A2  //Joystick Y-axis
#define JOYSTICK_SW 3  //Joystick button
#define GREEN_LED 5    //Green LED pin
#define YELLOW_LED 6   //Yellow LED pin
#define RED_LED 7      //Red LED pin
#define BUZZER_PIN 9   //Buzzer pin

const float tempHighThreshold = 30.0;  
const float tempLowThreshold = 0.0;   
const float humidityHighThreshold = 70.0;  
const float humidityLowThreshold = 30.0;   
const int lightHighThreshold = 800;
const int lightLowThreshold = 200;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

float temperature = 0.0;
float humidity = 0.0;
int lightLevel = 0;
int menuPosition = 0;
bool alarmEnabled = true;
bool autoRotate = true;
unsigned long lastReadTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastButtonCheckTime = 0;
unsigned long lastModeChangeTime = 0;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Smart Environmental Monitoring System"));

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(JOYSTICK_SW, INPUT_PULLUP); 
  
  digitalWrite(BUZZER_PIN, HIGH);
  
  digitalWrite(GREEN_LED, HIGH);
  delay(300);
  digitalWrite(YELLOW_LED, HIGH);
  delay(300);
  digitalWrite(RED_LED, HIGH);
  delay(300);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  digitalWrite(BUZZER_PIN, LOW);
  delay(200);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  delay(200);
  digitalWrite(BUZZER_PIN, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Env Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  dht.begin();
  
  delay(2000);
}

void loop() {
  if (millis() - lastReadTime > 2000) {
    readSensors();
    checkAlarms();
    lastReadTime = millis();
  }
  
  if (millis() - lastDisplayTime > 500) {
    updateDisplay();
    lastDisplayTime = millis();
  }
  
  if (millis() - lastButtonCheckTime > 200) {
    checkJoystick();
    lastButtonCheckTime = millis();
  }
  
  if (autoRotate && millis() - lastModeChangeTime > 3000) {
    menuPosition = (menuPosition + 1) % 3;
    lastModeChangeTime = millis();
  }
}

void readSensors() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  lightLevel = analogRead(LDR_PIN);
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Light: ");
  Serial.println(lightLevel);
}

void checkAlarms() {
  bool isAlert = false;
  
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
 
  if (!isnan(temperature)) {
    if (temperature > tempHighThreshold) {
      digitalWrite(RED_LED, HIGH);
      isAlert = true;
    } else if (temperature < tempLowThreshold) {
      digitalWrite(YELLOW_LED, HIGH);
      isAlert = true;
    } else {
      digitalWrite(GREEN_LED, HIGH);
    }
  }
  
  if (!isnan(humidity)) {
    if (humidity > humidityHighThreshold || humidity < humidityLowThreshold) {
      digitalWrite(YELLOW_LED, HIGH);
      isAlert = true;
    }
  }
  
  if (lightLevel > lightHighThreshold || lightLevel < lightLowThreshold) {
    digitalWrite(YELLOW_LED, HIGH);
    isAlert = true;
  }
  
  if (isAlert && alarmEnabled) {
    digitalWrite(BUZZER_PIN, LOW); 
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH); 
  }
}

void updateDisplay() {
  lcd.clear();
  
  switch (menuPosition) {
    case 0: 
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print(" C");
      lcd.setCursor(0, 1);
      lcd.print("Humidity: ");
      lcd.print(humidity, 1);
      lcd.print("%");
      break;
      
    case 1: 
      lcd.setCursor(0, 0);
      lcd.print("Light Level:");
      lcd.setCursor(0, 1);
      lcd.print(lightLevel);
      if (lightLevel < lightLowThreshold) {
        lcd.print(" (Low)");
      } else if (lightLevel > lightHighThreshold) {
        lcd.print(" (High)");
      } else {
        lcd.print(" (Normal)");
      }
      break;
      
    case 2: 
      lcd.setCursor(0, 0);
      lcd.print("Alarm: ");
      lcd.print(alarmEnabled ? "ON" : "OFF");
      lcd.setCursor(0, 1);
      lcd.print("Auto-rotate: ");
      lcd.print(autoRotate ? "ON" : "OFF");
      break;
  }
}

void checkJoystick() {
  int xValue = analogRead(JOYSTICK_X);
  int yValue = analogRead(JOYSTICK_Y);
  int buttonState = digitalRead(JOYSTICK_SW);
  
  static int lastXValue = 512;
  static int lastYValue = 512;
  static bool lastButtonState = HIGH;
  static unsigned long lastJoystickActionTime = 0;
  
  if (millis() - lastJoystickActionTime > 300) {
    if (xValue < 300 && lastXValue >= 300) {
      if (menuPosition > 0) {
        menuPosition--;
        autoRotate = false;
        lastModeChangeTime = millis();

        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
        digitalWrite(BUZZER_PIN, HIGH);
        
        lastJoystickActionTime = millis();
      }
    } 
    else if (xValue > 700 && lastXValue <= 700) {
      if (menuPosition < 2) {
        menuPosition++;
        autoRotate = false;
        lastModeChangeTime = millis();

        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
        digitalWrite(BUZZER_PIN, HIGH);
        
        lastJoystickActionTime = millis();
      }
    }
    
    if (menuPosition == 2) { 
      if (yValue < 300 && lastYValue >= 300) { 
        alarmEnabled = true;
        
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
        digitalWrite(BUZZER_PIN, HIGH);
        
        lastJoystickActionTime = millis();
      } 
      else if (yValue > 700 && lastYValue <= 700) { 
        alarmEnabled = false;
        
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
        digitalWrite(BUZZER_PIN, HIGH);
        
        lastJoystickActionTime = millis();
      }
    }
    
    if (buttonState == LOW && lastButtonState == HIGH) { 
      autoRotate = !autoRotate;

      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    }
  }

  lastXValue = xValue;
  lastYValue = yValue;
  lastButtonState = buttonState;
}