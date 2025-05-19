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

float tempHighThreshold = 30.0;  
float tempLowThreshold = 0.0;   
float humidityHighThreshold = 70.0;  
float humidityLowThreshold = 30.0;   
int lightHighThreshold = 800;
int lightLowThreshold = 200;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

float temperature = 0.0;
float humidity = 0.0;
int lightLevel = 0;
int menuPosition = 0;
int settingsPosition = 0; 
int editPosition = 0;    
bool alarmEnabled = true;
bool autoRotate = true;
bool inSettingsEdit = false; 

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
  
  if (autoRotate && millis() - lastModeChangeTime > 3000 && !inSettingsEdit) {
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
  
  if (menuPosition <= 1) {
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
    }
  } else if (menuPosition == 2) { 
    if (!inSettingsEdit) {
      switch (settingsPosition) {
        case 0: 
          lcd.setCursor(0, 0);
          lcd.print("Alarm: ");
          lcd.print(alarmEnabled ? "ON" : "OFF");
          lcd.setCursor(0, 1);
          lcd.print("Auto-rotate: ");
          lcd.print(autoRotate ? "ON" : "OFF");
          break;
          
        case 1: 
          lcd.setCursor(0, 0);
          lcd.print("Temp High: ");
          lcd.print(tempHighThreshold, 1);
          lcd.setCursor(0, 1);
          lcd.print("Temp Low: ");
          lcd.print(tempLowThreshold, 1);
          break;
          
        case 2: 
          lcd.setCursor(0, 0);
          lcd.print("Humid High: ");
          lcd.print(humidityHighThreshold, 0);
          lcd.setCursor(0, 1);
          lcd.print("Humid Low: ");
          lcd.print(humidityLowThreshold, 0);
          break;
          
        case 3: 
          lcd.setCursor(0, 0);
          lcd.print("Light High: ");
          lcd.print(lightHighThreshold);
          lcd.setCursor(0, 1);
          lcd.print("Light Low: ");
          lcd.print(lightLowThreshold);
          break;
      }
    } else {
      switch (settingsPosition) {
        case 0: 
          lcd.setCursor(0, 0);
          lcd.print("Edit Settings:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) { 
            lcd.print("> Alarm: ");
            lcd.print(alarmEnabled ? "ON" : "OFF");
          } else { 
            lcd.print("> Auto: ");
            lcd.print(autoRotate ? "ON" : "OFF");
          }
          break;
          
        case 1: 
          lcd.setCursor(0, 0);
          lcd.print("Edit Temperature:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print("> High: ");
            lcd.print(tempHighThreshold, 1);
            lcd.print("C");
          } else {
            lcd.print("> Low: ");
            lcd.print(tempLowThreshold, 1);
            lcd.print("C");
          }
          break;
          
        case 2: 
          lcd.setCursor(0, 0);
          lcd.print("Edit Humidity:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print("> High: ");
            lcd.print(humidityHighThreshold, 0);
            lcd.print("%");
          } else {
            lcd.print("> Low: ");
            lcd.print(humidityLowThreshold, 0);
            lcd.print("%");
          }
          break;
          
        case 3: 
          lcd.setCursor(0, 0);
          lcd.print("Edit Light:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print("> High: ");
            lcd.print(lightHighThreshold);
          } else {
            lcd.print("> Low: ");
            lcd.print(lightLowThreshold);
          }
          break;
      }
    }
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
   
    if (buttonState == LOW && lastButtonState == HIGH) {
      if (menuPosition == 2) { 
        if (!inSettingsEdit) {
          inSettingsEdit = true;
          editPosition = 0; 
        } else {
          inSettingsEdit = false;
        }
      } else {
        autoRotate = !autoRotate;
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    }
    
    if (xValue < 300 && lastXValue >= 300) { 
      if (!inSettingsEdit) {
        if (menuPosition > 0) {
          menuPosition--;
        } else {
          menuPosition = 2; 
        }
        autoRotate = false;
        lastModeChangeTime = millis();
        settingsPosition = 0; 
      } else if (menuPosition == 2) {
        editPosition = (editPosition == 1) ? 0 : 1;
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    } 
    else if (xValue > 700 && lastXValue <= 700) { 
      if (!inSettingsEdit) {
        if (menuPosition < 2) {
          menuPosition++;
        } else {
          menuPosition = 0; 
        }
        autoRotate = false;
        lastModeChangeTime = millis();
        settingsPosition = 0; 
      } else if (menuPosition == 2) {
        editPosition = (editPosition == 1) ? 0 : 1;
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    }
    
    if (yValue < 300 && lastYValue >= 300) { 
      if (menuPosition == 2) { 
        if (!inSettingsEdit) {
          if (settingsPosition > 0) {
            settingsPosition--;
          } else {
            settingsPosition = 3; 
          }
        } else {
          adjustSettingValue(1);
        }
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    } 
    else if (yValue > 700 && lastYValue <= 700) { 
      if (menuPosition == 2) { 
        if (!inSettingsEdit) {
          if (settingsPosition < 3) {
            settingsPosition++;
          } else {
            settingsPosition = 0; 
          }
        } else {
          adjustSettingValue(-1);
        }
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    }
  }
  
  lastXValue = xValue;
  lastYValue = yValue;
  lastButtonState = buttonState;
}

void adjustSettingValue(int direction) {
  switch (settingsPosition) {
    case 0: 
      if (editPosition == 0) { 
        alarmEnabled = !alarmEnabled;
      } else { 
        autoRotate = !autoRotate;
      }
      break;
      
    case 1: 
      if (editPosition == 0) { 
        tempHighThreshold += direction * 0.5;
      } else { 
        tempLowThreshold += direction * 0.5;
      }
      break;
      
    case 2: 
      if (editPosition == 0) { 
        humidityHighThreshold += direction * 1.0;
      } else { 
        humidityLowThreshold += direction * 1.0;
      }
      break;
      
    case 3: 
      if (editPosition == 0) { 
        lightHighThreshold += direction * 10;
      } else { 
        lightLowThreshold += direction * 10;
      }
      break;
  }
  
  validateThresholds();
}

void validateThresholds() {
  if (tempHighThreshold <= tempLowThreshold) {
    tempHighThreshold = tempLowThreshold + 0.5;
  }
  
  if (humidityHighThreshold <= humidityLowThreshold) {
    humidityHighThreshold = humidityLowThreshold + 1.0;
  }
  
  if (lightHighThreshold <= lightLowThreshold) {
    lightHighThreshold = lightLowThreshold + 10;
  }
  
  if (tempHighThreshold > 50.0) tempHighThreshold = 50.0;
  if (tempLowThreshold < -20.0) tempLowThreshold = -20.0;
  
  if (humidityHighThreshold > 100.0) humidityHighThreshold = 100.0;
  if (humidityLowThreshold < 0.0) humidityLowThreshold = 0.0;
  
  if (lightHighThreshold > 1023) lightHighThreshold = 1023;
  if (lightLowThreshold < 0) lightLowThreshold = 0;
}