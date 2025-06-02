#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Pin definitions
#define DHTPIN 2       //Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  //DHT 11
#define LDR_PIN A0     //Analog pin for light sensor
#define JOYSTICK_X A1  //Joystick X-axis
#define JOYSTICK_Y A2  //Joystick Y-axis
#define JOYSTICK_SW 3  //Joystick button
#define GREEN_LED 5    //Green LED pin
#define YELLOW_LED 6   //Yellow LED pin
#define RED_LED 7      //Red LED pin
#define BUZZER_PIN 9   //Buzzer pin (PWM capable)

float tempHighThreshold = 30.0;  
float tempLowThreshold = 0.0;   
float humidityHighThreshold = 70.0;  
float humidityLowThreshold = 30.0;   
int lightHighThreshold = 800;
int lightLowThreshold = 200;

#define ALERT_NONE 0
#define ALERT_LOW 1    
#define ALERT_HIGH 2  

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
int currentAlertLevel = ALERT_NONE;

#define TONE_NONE 0
#define TONE_LOW 100    
#define TONE_MEDIUM 400 
#define TONE_HIGH 800  

unsigned long lastReadTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastButtonCheckTime = 0;
unsigned long lastModeChangeTime = 0;
unsigned long lastAlertTime = 0;
unsigned long alertToggleTime = 0;
bool alertToggleState = false;

const char* teamName = "Team 9";
const char* teamMembers = "DZ,KM,MT,WW,MS";

void setup() {
  Serial.begin(9600);
  
  Serial.println(F("Smart Environmental Monitoring System"));
  Serial.println(F("Initializing..."));
  
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
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH); 
  delay(100);
  digitalWrite(BUZZER_PIN, LOW); 
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH); 

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Env Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  dht.begin();
  
  Serial.println(F("System initialization complete"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("Current Settings:"));
  Serial.print(F("Temperature Thresholds: ")); 
  Serial.print(tempLowThreshold); Serial.print(F("C - ")); Serial.print(tempHighThreshold); Serial.println(F("C"));
  Serial.print(F("Humidity Thresholds: ")); 
  Serial.print(humidityLowThreshold); Serial.print(F("% - ")); Serial.print(humidityHighThreshold); Serial.println(F("%"));
  Serial.print(F("Light Thresholds: ")); 
  Serial.print(lightLowThreshold); Serial.print(F(" - ")); Serial.println(lightHighThreshold);
  Serial.println(F("----------------------------------------"));
  
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
  
  handleAlertSounds();
}

void readSensors() {
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  
  if (!isnan(newHumidity)) {
    humidity = newHumidity;
  } else {
    Serial.println(F("ERROR: Failed to read humidity!"));
  }
  
  if (!isnan(newTemperature)) {
    temperature = newTemperature;
  } else {
    Serial.println(F("ERROR: Failed to read temperature!"));
  }
  
  lightLevel = analogRead(LDR_PIN);
  
  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.print(F("°C, Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%, Light: "));
  Serial.println(lightLevel);
}

void checkAlarms() {
  int newAlertLevel = ALERT_NONE;
  bool tempHigh = false;
  bool tempLow = false;
  bool humidityOutOfRange = false;
  bool lightOutOfRange = false;
  
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  if (!isnan(temperature)) {
    if (temperature > tempHighThreshold) {
      tempHigh = true;
      newAlertLevel = ALERT_HIGH; 
    } else if (temperature < tempLowThreshold) {
      tempLow = true;
      newAlertLevel = max(newAlertLevel, ALERT_LOW);  
    }
  }
  
  if (!isnan(humidity)) {
    if (humidity > humidityHighThreshold || humidity < humidityLowThreshold) {
      humidityOutOfRange = true;
      newAlertLevel = max(newAlertLevel, ALERT_LOW);  
    }
  }
  
  if (lightLevel > lightHighThreshold || lightLevel < lightLowThreshold) {
    lightOutOfRange = true;
    newAlertLevel = max(newAlertLevel, ALERT_LOW); 
  }
  
  if (newAlertLevel == ALERT_HIGH) {
    digitalWrite(RED_LED, HIGH);
  } else if (newAlertLevel == ALERT_LOW) {
    digitalWrite(YELLOW_LED, HIGH);
  } else {
    digitalWrite(GREEN_LED, HIGH); 
  }
  
  if (currentAlertLevel != newAlertLevel) {
    currentAlertLevel = newAlertLevel;
    
    if (newAlertLevel > ALERT_NONE && alarmEnabled) {
      digitalWrite(BUZZER_PIN, LOW);  
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH); 
    }
    
    if (newAlertLevel == ALERT_HIGH) {
      Serial.println(F("ALERT: High-priority alert triggered!"));
      if (tempHigh) Serial.println(F("  - Temperature above threshold"));
    } else if (newAlertLevel == ALERT_LOW) {
      Serial.println(F("ALERT: Low-priority alert triggered!"));
      if (tempLow) Serial.println(F("  - Temperature below threshold"));
      if (humidityOutOfRange) Serial.println(F("  - Humidity out of range"));
      if (lightOutOfRange) Serial.println(F("  - Light level out of range"));
    } else {
      Serial.println(F("INFO: All readings within normal ranges"));
    }
    
    lastAlertTime = millis();
    alertToggleState = true;
  }
}

void handleAlertSounds() {
  if (!alarmEnabled) {
    playTone(TONE_NONE);
    return;
  }
  
  switch (currentAlertLevel) {
    case ALERT_HIGH:
      if (millis() - alertToggleTime > 200) {
        alertToggleState = !alertToggleState;
        alertToggleTime = millis();
        
        if (alertToggleState) {
          playTone(TONE_HIGH);
        } else {
          playTone(TONE_NONE);
        }
      }
      break;
      
    case ALERT_LOW:
      if (alertToggleState) {
        if (millis() - alertToggleTime > 500) {
          playTone(TONE_NONE);
          alertToggleState = false;
          alertToggleTime = millis();
        }
      } else {
        if (millis() - alertToggleTime > 2000) {
          playTone(TONE_LOW);
          alertToggleState = true;
          alertToggleTime = millis();
        }
      }
      break;
      
    default:
      playTone(TONE_NONE);
      break;
  }
}

void playTone(int frequency) {
  if (frequency == TONE_NONE) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
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
          
        case 4: 
          lcd.setCursor(0, 0);
          lcd.print(teamName);
          lcd.setCursor(0, 1);
          lcd.print(teamMembers);
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
          
        case 4: 
          lcd.setCursor(0, 0);
          lcd.print(teamName);
          lcd.setCursor(0, 1);
          lcd.print(teamMembers);
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
      Serial.println(F("Joystick button pressed"));
      
      if (menuPosition == 2) { 
        if (!inSettingsEdit) {
          if (settingsPosition != 4) {
            inSettingsEdit = true;
            editPosition = 0; 
            Serial.println(F("Entered settings edit mode"));
          } else {
            Serial.println(F("Team info screen - button press ignored"));
          }
        } else {
          inSettingsEdit = false;
          Serial.println(F("Exited settings edit mode"));
        }
      } else {
        autoRotate = !autoRotate;
        Serial.print(F("Auto-rotate: "));
        Serial.println(autoRotate ? F("ON") : F("OFF"));
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    }
    
    if (xValue < 300 && lastXValue >= 300) { 
      Serial.println(F("Joystick moved left"));
      
      if (!inSettingsEdit) {
        if (menuPosition > 0) {
          menuPosition--;
        } else {
          menuPosition = 2; 
        }
        autoRotate = false;
        lastModeChangeTime = millis();
        settingsPosition = 0; 
        
        Serial.print(F("Changed to menu position: "));
        Serial.println(menuPosition);
      } else if (menuPosition == 2) {
        editPosition = (editPosition == 1) ? 0 : 1;
        Serial.print(F("Changed edit position to: "));
        Serial.println(editPosition);
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    } 
    else if (xValue > 700 && lastXValue <= 700) { 
      Serial.println(F("Joystick moved right"));
      
      if (!inSettingsEdit) {
        if (menuPosition < 2) {
          menuPosition++;
        } else {
          menuPosition = 0; 
        }
        autoRotate = false;
        lastModeChangeTime = millis();
        settingsPosition = 0; 
        
        Serial.print(F("Changed to menu position: "));
        Serial.println(menuPosition);
      } else if (menuPosition == 2) {
        editPosition = (editPosition == 1) ? 0 : 1;
        Serial.print(F("Changed edit position to: "));
        Serial.println(editPosition);
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    }
    
    if (yValue < 300 && lastYValue >= 300) { 
      Serial.println(F("Joystick moved up"));
      
      if (menuPosition == 2) { 
        if (!inSettingsEdit) {
          if (settingsPosition > 0) {
            settingsPosition--;
          } else {
            settingsPosition = 4; 
          }
          Serial.print(F("Changed settings position to: "));
          Serial.println(settingsPosition);
        } else {
          Serial.print(F("Increasing setting value: "));
          adjustSettingValue(1);
        }
      }
      
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      
      lastJoystickActionTime = millis();
    } 
    else if (yValue > 700 && lastYValue <= 700) { 
      Serial.println(F("Joystick moved down"));
      
      if (menuPosition == 2) { 
        if (!inSettingsEdit) {
          if (settingsPosition < 4) {
            settingsPosition++;
          } else {
            settingsPosition = 0; 
          }
          Serial.print(F("Changed settings position to: "));
          Serial.println(settingsPosition);
        } else {
          Serial.print(F("Decreasing setting value: "));
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
        Serial.print(F("Alarm enabled: "));
        Serial.println(alarmEnabled ? F("YES") : F("NO"));
      } else { 
        autoRotate = !autoRotate;
        Serial.print(F("Auto-rotate: "));
        Serial.println(autoRotate ? F("ON") : F("OFF"));
      }
      break;
      
    case 1: 
      if (editPosition == 0) { 
        tempHighThreshold += direction * 0.5;
        Serial.print(F("Temperature high threshold: "));
        Serial.println(tempHighThreshold);
      } else { 
        tempLowThreshold += direction * 0.5;
        Serial.print(F("Temperature low threshold: "));
        Serial.println(tempLowThreshold);
      }
      break;
      
    case 2: 
      if (editPosition == 0) { 
        humidityHighThreshold += direction * 1.0;
        Serial.print(F("Humidity high threshold: "));
        Serial.println(humidityHighThreshold);
      } else { 
        humidityLowThreshold += direction * 1.0;
        Serial.print(F("Humidity low threshold: "));
        Serial.println(humidityLowThreshold);
      }
      break;
      
    case 3: 
      if (editPosition == 0) { 
        lightHighThreshold += direction * 10;
        Serial.print(F("Light high threshold: "));
        Serial.println(lightHighThreshold);
      } else { 
        lightLowThreshold += direction * 10;
        Serial.print(F("Light low threshold: "));
        Serial.println(lightLowThreshold);
      }
      break;
  }
  
  validateThresholds();
}

void validateThresholds() {
  if (tempHighThreshold <= tempLowThreshold) {
    tempHighThreshold = tempLowThreshold + 0.5;
    Serial.println(F("Adjusted temperature thresholds to ensure High > Low"));
  }
  
  if (humidityHighThreshold <= humidityLowThreshold) {
    humidityHighThreshold = humidityLowThreshold + 1.0;
    Serial.println(F("Adjusted humidity thresholds to ensure High > Low"));
  }
  
  if (lightHighThreshold <= lightLowThreshold) {
    lightHighThreshold = lightLowThreshold + 10;
    Serial.println(F("Adjusted light thresholds to ensure High > Low"));
  }
  
  if (tempHighThreshold > 50.0) tempHighThreshold = 50.0;
  if (tempLowThreshold < -20.0) tempLowThreshold = -20.0;
  
  if (humidityHighThreshold > 100.0) humidityHighThreshold = 100.0;
  if (humidityLowThreshold < 0.0) humidityLowThreshold = 0.0;
  
  if (lightHighThreshold > 1023) lightHighThreshold = 1023;
  if (lightLowThreshold < 0) lightLowThreshold = 0;
}