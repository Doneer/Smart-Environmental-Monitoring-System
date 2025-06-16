#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <avr/sleep.h>
#include <avr/power.h>

//Pin definitions
#define DHTPIN 2
#define DHTTYPE DHT11
#define LDR_PIN A0
#define JOYSTICK_X A1
#define JOYSTICK_Y A2
#define JOYSTICK_SW 3
#define GREEN_LED 5
#define YELLOW_LED 6
#define RED_LED 7
#define BUZZER_PIN 9
#define SD_CS_PIN 10

//Sensor thresholds
float tempHighThreshold = 30.0;
float tempLowThreshold = 0.0;
float humidityHighThreshold = 70.0;
float humidityLowThreshold = 30.0;
int lightHighThreshold = 80;
int lightLowThreshold = 20;

//Calibration offset constants 
float temperatureOffset = -1.5;  
float humidityOffset = 5.0;      

//Alert levels
#define ALERT_NONE 0
#define ALERT_LOW 1
#define ALERT_HIGH 2

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
File dataFile;

float temperature = 0.0;
float humidity = 0.0;
int lightLevel = 0;
byte menuPosition = 0;
byte settingsPosition = 0;
byte editPosition = 0;
bool alarmEnabled = true;
bool autoRotate = true;
bool inSettingsEdit = false;
byte currentAlertLevel = ALERT_NONE;
bool sdCardPresent = false;
bool rtcPresent = false;
bool dataLoggingEnabled = true;
unsigned long lastLogTime = 0;
int logInterval = 60;
bool sleepEnabled = true;       
byte idleTimeThreshold = 90;    
unsigned long lastActivityTime = 0; 
bool isInSleepMode = false;
unsigned long sleepStartTime = 0;
unsigned long sleepDuration = 0; 

unsigned long lastReadTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastButtonCheckTime = 0;
unsigned long lastModeChangeTime = 0;
unsigned long alertToggleTime = 0;
bool alertToggleState = false;

void setup() {
  //Initialize pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(JOYSTICK_SW, INPUT_PULLUP);
  pinMode(SD_CS_PIN, OUTPUT);

  //Initialize UART at register level
  initUART();
  
  //Setup timer interrupt 
  setupInterrupts();
  
  //Making sure buzzer is silent
  digitalWrite(BUZZER_PIN, HIGH);
  
  //Test LEDs
  digitalWrite(GREEN_LED, HIGH);
  delay(100);
  digitalWrite(YELLOW_LED, HIGH);
  delay(100);
  digitalWrite(RED_LED, HIGH);
  delay(100);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  //Test buzzer
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  
  //Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smrt Env Mntr");
  
  //Initialize DHT sensor
  dht.begin();
  
  //Initialize I2C
  Wire.begin();
  
  //Initialize RTC
  if (rtc.begin()) {
    rtcPresent = true;
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  
  //Initialize SD card
  sdCardPresent = SD.begin(SD_CS_PIN);
  
  //Create log file with header if it doesn't exist
  if (sdCardPresent && !SD.exists("log.csv")) {
    dataFile = SD.open("log.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println(F("Date,Time,T,H,L,A"));
      dataFile.close();
    }
  }
  
  //Show init complete
  lcd.setCursor(0, 1);
  lcd.print("Int cmplt");
  delay(1000);
  
  //Set initial activity time
  lastActivityTime = millis();
}

void initUART() {
  //Set baud rate to 9600 (for 16MHz clock)
  UBRR0H = 0;
  UBRR0L = 103;
  
  //Enable transmitter only (no receiver to save memory)
  UCSR0B = (1 << TXEN0);
  
  UCSR0C = (3 << UCSZ00);
}

//Send a single character via UART
void sendUARTChar(char c) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
}

//Set up a single timer interrupt
void setupInterrupts() {
  TCCR2A = (1 << WGM21); 
  TCCR2B = (7 << CS20);   
  OCR2A = 155;          
  TIMSK2 = (1 << OCIE2A); 
  sei();                 
}

void loop() {
  checkSleepMode();
  
  if (isInSleepMode) {
    return;
  }
  
  //Read sensors (every 2 seconds)
  if (millis() - lastReadTime > 2000) {
    readSensors();
    checkAlarms();
    lastReadTime = millis();
  }
  
  //Update display (every 500ms)
  if (millis() - lastDisplayTime > 500) {
    updateDisplay();
    lastDisplayTime = millis();
  }
  
  //Check joystick (every 200ms)
  if (millis() - lastButtonCheckTime > 200) {
    checkJoystick();
    lastButtonCheckTime = millis();
  }
  
  //Auto-rotate
  if (autoRotate && millis() - lastModeChangeTime > 3000 && !inSettingsEdit) {
    menuPosition = (menuPosition + 1) % 3; 
    lastModeChangeTime = millis();
  }
  
  //Log data at specified interval
  if (sdCardPresent && dataLoggingEnabled && rtcPresent) {
    if (millis() - lastLogTime > (logInterval * 1000UL)) {
      logDataToSD();
      lastLogTime = millis();
    }
  }
  
  //Handle alert sounds
  handleAlertSounds();
}

void readSensors() {
  //Read DHT sensor
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  
  //Only update if readings are valid
  if (!isnan(newHumidity)) {
    humidity = newHumidity + humidityOffset;
    if (humidity > 100.0) humidity = 100.0;
    if (humidity < 0.0) humidity = 0.0;
  }
  
  if (!isnan(newTemperature)) {
    temperature = newTemperature + temperatureOffset;
  }
  
  //Read light sensor 
  int rawLightLevel = analogRead(LDR_PIN);
  
  //Convert to percentage
  lightLevel = convertLightToPercent(rawLightLevel);
}

void checkAlarms() {
  int newAlertLevel = ALERT_NONE;
  
  //Reset LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  //Check temperature
  if (!isnan(temperature)) {
    if (temperature > tempHighThreshold) {
      newAlertLevel = ALERT_HIGH;
    } else if (temperature < tempLowThreshold) {
      newAlertLevel = max(newAlertLevel, ALERT_LOW);
    }
  }
  
  //Check humidity
  if (!isnan(humidity)) {
    if (humidity > humidityHighThreshold || humidity < humidityLowThreshold) {
      newAlertLevel = max(newAlertLevel, ALERT_LOW);
    }
  }
  
  //Check light level
  if (lightLevel > lightHighThreshold || lightLevel < lightLowThreshold) {
    newAlertLevel = max(newAlertLevel, ALERT_LOW);
  }
  
  //Set LEDs based on alert level
  if (newAlertLevel == ALERT_HIGH) {
    digitalWrite(RED_LED, HIGH);
  } else if (newAlertLevel == ALERT_LOW) {
    digitalWrite(YELLOW_LED, HIGH);
  } else {
    digitalWrite(GREEN_LED, HIGH);
  }
  
  //Update current alert level
  if (currentAlertLevel != newAlertLevel) {
    currentAlertLevel = newAlertLevel;

    //Add minimal UART output - just one character per alert level
    if (newAlertLevel == ALERT_HIGH) {
      sendUARTChar('H');  //High alert
    } else if (newAlertLevel == ALERT_LOW) {
      sendUARTChar('L');  //Low alert
    } else {
      sendUARTChar('N');  //Normal
    }
    sendUARTChar('\r');
    sendUARTChar('\n');
    
    //Sound the buzzer immediately if there's an alert
    if (newAlertLevel > ALERT_NONE && alarmEnabled) {
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
    }
    
    //Reset alert timing
    alertToggleState = true;
    alertToggleTime = millis();
    
    //Active alert counts as activity (prevents sleep during alerts)
    if (newAlertLevel > ALERT_NONE) {
      lastActivityTime = millis();
    }
  }
}

void handleAlertSounds() {
  if (!alarmEnabled) {
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }
  
  switch (currentAlertLevel) {
    case ALERT_HIGH:
      //Rapid toggling 
      if (millis() - alertToggleTime > 200) {
        alertToggleState = !alertToggleState;
        digitalWrite(BUZZER_PIN, alertToggleState ? LOW : HIGH);
        alertToggleTime = millis();
      }
      break;
      
    case ALERT_LOW:
      //Slower toggling
      if (alertToggleState) {
        if (millis() - alertToggleTime > 500) {
          digitalWrite(BUZZER_PIN, HIGH);
          alertToggleState = false;
          alertToggleTime = millis();
        }
      } else {
        if (millis() - alertToggleTime > 2000) {
          digitalWrite(BUZZER_PIN, LOW);
          alertToggleState = true;
          alertToggleTime = millis();
        }
      }
      break;
      
    default:
      digitalWrite(BUZZER_PIN, HIGH);
      break;
  }
}

void updateDisplay() {
  lcd.clear();
  
  if (menuPosition <= 1) { //Sensor data screens
    switch (menuPosition) {
      case 0: //Temperature and Humidity
        lcd.setCursor(0, 0);
        lcd.print("T:");
        lcd.print(temperature, 1);
        lcd.print("C");
        lcd.print(" ");
        if (temperature < tempLowThreshold) {
          lcd.print("(L)");
        } else if (temperature > tempHighThreshold) {
          lcd.print("(H)");
        } else {
          lcd.print("(N)");
        }

        lcd.setCursor(0, 1);
        lcd.print("H:");
        lcd.print(humidity, 1);
        lcd.print("%");
        lcd.print(" ");
        if (humidity < humidityLowThreshold) {
          lcd.print("(L)");
        } else if (humidity > humidityHighThreshold) {
          lcd.print("(H)");
        } else {
          lcd.print("(N)");
        }
        break;
        
      case 1: //Light level
        lcd.setCursor(0, 0);
        lcd.print("L:");
        lcd.setCursor(0, 1);
        lcd.print(lightLevel);
        lcd.print("%");
        
        lcd.print(" ");
        
        if (lightLevel < lightLowThreshold) {
          lcd.print("(L)");
        } else if (lightLevel > lightHighThreshold) {
          lcd.print("(H)");
        } else {
          lcd.print("(N)");
        }
        break;
    }
  } else if (menuPosition == 2) { //Settings menu
    if (!inSettingsEdit) {
      switch (settingsPosition) {
        case 0: //Main settings
          lcd.setCursor(0, 0);
          lcd.print("Alrm:");
          lcd.print(alarmEnabled ? "ON" : "OFF");
          lcd.setCursor(0, 1);
          lcd.print("Rot:");
          lcd.print(autoRotate ? "ON" : "OFF");
          break;
          
        case 1: //Temperature thresholds
          lcd.setCursor(0, 0);
          lcd.print("T Hgh:");
          lcd.print(tempHighThreshold, 1);
          lcd.print("C");
          lcd.setCursor(0, 1);
          lcd.print("T Low:");
          lcd.print(tempLowThreshold, 1);
          lcd.print("C");
          break;
          
        case 2: //Humidity thresholds
          lcd.setCursor(0, 0);
          lcd.print("H Hgh:");
          lcd.print(humidityHighThreshold, 0);
          lcd.print("%");
          lcd.setCursor(0, 1);
          lcd.print("H Low:");
          lcd.print(humidityLowThreshold, 0);
          lcd.print("%");
          break;
          
        case 3: //Light thresholds
          lcd.setCursor(0, 0);
          lcd.print("L Hgh:");
          lcd.print(lightHighThreshold);
          lcd.print("%");
          lcd.setCursor(0, 1);
          lcd.print("L Low:");
          lcd.print(lightLowThreshold);
          lcd.print("%");
          break;
          
        case 4: //Data logging
          lcd.setCursor(0, 0);
          lcd.print("Log:");
          lcd.print(dataLoggingEnabled ? "ON" : "OFF");
          lcd.setCursor(0, 1);
          lcd.print("Int:");
          lcd.print(logInterval);
          lcd.print("s");
          break;
          
        case 5: //Sleep settings
          lcd.setCursor(0, 0);
          lcd.print("Slp:");
          lcd.print(sleepEnabled ? "ON" : "OFF");
          lcd.setCursor(0, 1);
          lcd.print("Idle:");
          lcd.print(idleTimeThreshold);
          lcd.print("s");
          break;

        case 6: //Calibration
          lcd.setCursor(0, 0);
          lcd.print("T Offset:");
          lcd.print(temperatureOffset, 1);
          lcd.print("C");
          lcd.setCursor(0, 1);
          lcd.print("H Offset:");
          lcd.print(humidityOffset, 1);
          lcd.print("%");
          break;
      }
    } else {
      //Edit mode
      switch (settingsPosition) {
        case 0: //Main settings
          lcd.setCursor(0, 0);
          lcd.print("Edit Stg:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">Alrm:");
            lcd.print(alarmEnabled ? "ON" : "OFF");
          } else {
            lcd.print(">Rot:");
            lcd.print(autoRotate ? "ON" : "OFF");
          }
          break;
          
        case 1: //Temperature thresholds
          lcd.setCursor(0, 0);
          lcd.print("Edit T:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">Hgh:");
            lcd.print(tempHighThreshold, 1);
          } else {
            lcd.print(">Low:");
            lcd.print(tempLowThreshold, 1);
          }
          break;
          
        case 2: //Humidity thresholds
          lcd.setCursor(0, 0);
          lcd.print("Edit H:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">Hgh:");
            lcd.print(humidityHighThreshold, 0);
          } else {
            lcd.print(">Low:");
            lcd.print(humidityLowThreshold, 0);
          }
          break;
          
        case 3: //Light thresholds
          lcd.setCursor(0, 0);
          lcd.print("Edit L:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">Hgh:");
            lcd.print(lightHighThreshold);
            lcd.print("%");
          } else {
            lcd.print(">Low:");
            lcd.print(lightLowThreshold);
            lcd.print("%");
          }
          break;
          
        case 4: //Data logging
          lcd.setCursor(0, 0);
          lcd.print("Edit Log:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">Enbld:");
            lcd.print(dataLoggingEnabled ? "ON" : "OFF");
          } else {
            lcd.print(">Intr:");
            lcd.print(logInterval);
          }
          break;
          
        case 5: //Sleep settings
          lcd.setCursor(0, 0);
          lcd.print("Edit Slp:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">Mode:");
            lcd.print(sleepEnabled ? "ON" : "OFF");
          } else {
            lcd.print(">Idle Time:");
            lcd.print(idleTimeThreshold);
          }
          break;

        case 6: //Calibration
          lcd.setCursor(0, 0);
          lcd.print("Calibr:");
          lcd.setCursor(0, 1);
          
          if (editPosition == 0) {
            lcd.print(">T Offset:");
            lcd.print(temperatureOffset, 1);
          } else {
            lcd.print(">H Offset:");
            lcd.print(humidityOffset, 1);
          }
          break;
      }
    }
  }
}

int convertLightToPercent(int adcValue) {
  const int darkADC = 100;  
  const int brightADC = 900;  
  
  int constrained = constrain(adcValue, darkADC, brightADC);
  
  return map(constrained, darkADC, brightADC, 100, 0);
}

void checkJoystick() {
  int xValue = analogRead(JOYSTICK_X);
  int yValue = analogRead(JOYSTICK_Y);
  int buttonState = digitalRead(JOYSTICK_SW);
  
  static int lastXValue = 512;
  static int lastYValue = 512;
  static bool lastButtonState = HIGH;
  static unsigned long lastJoystickActionTime = 0;
  
  if (millis() - lastJoystickActionTime < 300) {
    lastXValue = xValue;
    lastYValue = yValue;
    lastButtonState = buttonState;
    return;
  }
  
  //Check for any joystick movement
  bool joystickMoved = (abs(xValue - 512) > 200) || (abs(yValue - 512) > 200) || (buttonState != lastButtonState);
  
  if (joystickMoved) {
    //Update activity time whenever joystick is moved
    lastActivityTime = millis();
  }
  
  //Button press
  if (buttonState == LOW && lastButtonState == HIGH) {
    if (menuPosition == 2) { //In settings menu
      if (!inSettingsEdit) {
        inSettingsEdit = true;
        editPosition = 0;
      } else {
        inSettingsEdit = false;
      }
    } else {
      autoRotate = !autoRotate;
    }
    
    //Sound feedback
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    
    lastJoystickActionTime = millis();
  }
  
  //Left movement
  if (xValue < 300 && lastXValue >= 300) {
    if (!inSettingsEdit) {
      if (menuPosition > 0) menuPosition--;
      else menuPosition = 2;
      
      autoRotate = false;
      lastModeChangeTime = millis();
      settingsPosition = 0;
    } else if (menuPosition == 2) {
      editPosition = (editPosition == 1) ? 0 : 1;
    }
    
    //Sound feedback
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    
    lastJoystickActionTime = millis();
  } 
  
  //Right movement
  if (xValue > 700 && lastXValue <= 700) {
    if (!inSettingsEdit) {
      if (menuPosition < 2) menuPosition++;
      else menuPosition = 0;
      
      autoRotate = false;
      lastModeChangeTime = millis();
      settingsPosition = 0;
    } else if (menuPosition == 2) {
      editPosition = (editPosition == 1) ? 0 : 1;
    }
    
    //Sound feedback
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    
    lastJoystickActionTime = millis();
  }
  
  //Up movement
  if (yValue < 300 && lastYValue >= 300) {
    if (menuPosition == 2) { //In settings menu
      if (!inSettingsEdit) {
        if (settingsPosition > 0) settingsPosition--;
        else settingsPosition = 6; 
      } else {
        adjustSettingValue(1);
      }
    }
    
    //Sound feedback
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    
    lastJoystickActionTime = millis();
  } 
  
  //Down movement
  if (yValue > 700 && lastYValue <= 700) {
    if (menuPosition == 2) { //In settings menu
      if (!inSettingsEdit) {
        if (settingsPosition < 6) settingsPosition++; 
        else settingsPosition = 0;
      } else {
        adjustSettingValue(-1);
      }
    }
    
    //Sound feedback
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    
    lastJoystickActionTime = millis();
  }
  
  //Remember states
  lastXValue = xValue;
  lastYValue = yValue;
  lastButtonState = buttonState;
}

void adjustSettingValue(int direction) {
  switch (settingsPosition) {
    case 0: //Main settings
      if (editPosition == 0) {
        alarmEnabled = !alarmEnabled;
      } else {
        autoRotate = !autoRotate;
      }
      break;
      
    case 1: //Temperature thresholds
      if (editPosition == 0) {
        tempHighThreshold += direction * 0.5;
      } else {
        tempLowThreshold += direction * 0.5;
      }
      break;
      
    case 2: //Humidity thresholds
      if (editPosition == 0) {
        humidityHighThreshold += direction * 1.0;
      } else {
        humidityLowThreshold += direction * 1.0;
      }
      break;
      
    case 3: //Light thresholds
      if (editPosition == 0) {
        lightHighThreshold += direction * 10;
      } else {
        lightLowThreshold += direction * 10;
      }
      break;
      
    case 4: //Data logging settings
      if (editPosition == 0) {
        dataLoggingEnabled = !dataLoggingEnabled;
      } else {
        logInterval += direction * 10;
        if (logInterval < 10) logInterval = 10;
        if (logInterval > 3600) logInterval = 3600;
      }
      break;
      
    case 5: //Sleep settings
      if (editPosition == 0) {
        sleepEnabled = !sleepEnabled;
      } else {
        idleTimeThreshold += direction * 5;
        if (idleTimeThreshold < 10) idleTimeThreshold = 10;
        if (idleTimeThreshold > 120) idleTimeThreshold = 120;
      }
      break;

    case 6: //Calibration settings
      if (editPosition == 0) {
        temperatureOffset += direction * 0.5;
        //Limit reasonable offset range
        if (temperatureOffset > 10.0) temperatureOffset = 10.0;
        if (temperatureOffset < -10.0) temperatureOffset = -10.0;
      } else {
        humidityOffset += direction * 1.0;
        //Limit reasonable offset range
        if (humidityOffset > 20.0) humidityOffset = 20.0;
        if (humidityOffset < -20.0) humidityOffset = -20.0;
      }
      break;
  }
  
  validateThresholds();
}

void validateThresholds() {
  //Make sure high thresholds are higher than low thresholds
  if (tempHighThreshold <= tempLowThreshold)
    tempHighThreshold = tempLowThreshold + 0.5;
  
  if (humidityHighThreshold <= humidityLowThreshold)
    humidityHighThreshold = humidityLowThreshold + 1.0;
  
  if (lightHighThreshold <= lightLowThreshold)
    lightHighThreshold = lightLowThreshold + 10;
  
  //Set reasonable limits
  if (tempHighThreshold > 50.0) tempHighThreshold = 50.0;
  if (tempLowThreshold < -20.0) tempLowThreshold = -20.0;
  
  if (humidityHighThreshold > 100.0) humidityHighThreshold = 100.0;
  if (humidityLowThreshold < 0.0) humidityLowThreshold = 0.0;
  
  if (lightHighThreshold > 1023) lightHighThreshold = 1023;
  if (lightLowThreshold < 0) lightLowThreshold = 0;
}

void logDataToSD() {
  if (!sdCardPresent || !rtcPresent) return;
  
  DateTime now = rtc.now();
  dataFile = SD.open("log.csv", FILE_WRITE);
  
  if (dataFile) {
    //Date in MM/DD format
    dataFile.print(now.month());
    dataFile.print('/');
    dataFile.print(now.day());
    dataFile.print(',');
    
    //Time in HH:MM format
    if (now.hour() < 10) dataFile.print('0');
    dataFile.print(now.hour());
    dataFile.print(':');
    if (now.minute() < 10) dataFile.print('0');
    dataFile.print(now.minute());
    dataFile.print(',');
    
    //Sensor data
    dataFile.print(temperature);
    dataFile.print(',');
    dataFile.print(humidity);
    dataFile.print(',');
    dataFile.print(lightLevel);
    dataFile.print(',');
    
    //Alert level
    dataFile.println(currentAlertLevel);
    
    dataFile.close();
  }
}

void checkSleepMode() {
  //If already in sleep mode, check if we should wake up
  if (isInSleepMode) {
    //First check if we need to read sensors during sleep
    if (millis() - lastReadTime > 2000) {
      //Read sensors while in sleep mode
      readSensors();
      
      //Check if any sensors have crossed thresholds
      int newAlertLevel = checkAlertsQuietly();
      
      //If alert level has changed and there's an alert, wake up
      if (newAlertLevel != currentAlertLevel && newAlertLevel != ALERT_NONE) {
        currentAlertLevel = newAlertLevel;
        exitSleepMode();
        //Sound the alert now that we're awake
        if (alarmEnabled) {
          digitalWrite(BUZZER_PIN, LOW);
          delay(100);
          digitalWrite(BUZZER_PIN, HIGH);
        }
        return;
      }
      
      lastReadTime = millis();
    }
    
    //Check for user input to wake up
    int xValue = analogRead(JOYSTICK_X);
    int yValue = analogRead(JOYSTICK_Y);
    int buttonState = digitalRead(JOYSTICK_SW);
    
    bool joystickMoved = (abs(xValue - 512) > 200) || (abs(yValue - 512) > 200) || (buttonState == LOW);
    
    if (joystickMoved) {
      //Wake up if the joystick is moved
      exitSleepMode();
      return;
    }
    
    //Stay in sleep mode (skip normal loop processing)
    delay(100); //Reduced polling interval while sleeping
    return;
  }
  
  //Only check entering sleep if enabled and no alerts are active
  if (!sleepEnabled || currentAlertLevel != ALERT_NONE) {
    return;
  }
  
  //Check if system has been idle long enough
  if ((millis() - lastActivityTime) > (idleTimeThreshold * 1000UL)) {
    enterSleepMode();
  }
}

int checkAlertsQuietly() {
  int newAlertLevel = ALERT_NONE;
  
  //Check temperature
  if (!isnan(temperature)) {
    if (temperature > tempHighThreshold) {
      newAlertLevel = ALERT_HIGH;
    } else if (temperature < tempLowThreshold) {
      newAlertLevel = max(newAlertLevel, ALERT_LOW);
    }
  }
  
  //Check humidity
  if (!isnan(humidity)) {
    if (humidity > humidityHighThreshold || humidity < humidityLowThreshold) {
      newAlertLevel = max(newAlertLevel, ALERT_LOW);
    }
  }
  
  //Check light level
  if (lightLevel > lightHighThreshold || lightLevel < lightLowThreshold) {
    newAlertLevel = max(newAlertLevel, ALERT_LOW);
  }
  
  return newAlertLevel;
}

void enterSleepMode() {
  //Prepare for sleep: turn off LEDs, LCD backlight, etc.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, HIGH); 
  lcd.noBacklight();
  
  isInSleepMode = true;
  sleepStartTime = millis();
}

void exitSleepMode() {
  //Restore LCD backlight
  lcd.backlight();
  
  //Turn on the appropriate LED based on current alert level
  if (currentAlertLevel == ALERT_HIGH) {
    digitalWrite(RED_LED, HIGH);
  } else if (currentAlertLevel == ALERT_LOW) {
    digitalWrite(YELLOW_LED, HIGH);
  } else {
    digitalWrite(GREEN_LED, HIGH);
  }
  
  //Mark as no longer in sleep mode
  isInSleepMode = false;

  //Log data immediately upon waking 
  if (sdCardPresent && dataLoggingEnabled && rtcPresent) {
    logDataToSD();
    lastLogTime = millis();  
  }
  
  //Reset activity time
  lastActivityTime = millis();
  
  //Reset timing variables to avoid immediate actions after wake-up
  lastDisplayTime = millis();
  lastButtonCheckTime = millis();
  lastModeChangeTime = millis();
  alertToggleTime = millis();
}

//Timer2 interrupt service routine
ISR(TIMER2_COMPA_vect) {
  static byte counter = 0;
  counter++;
  
  //Blink the LED every ~5 seconds only when no alerts
  if (counter >= 50 && currentAlertLevel == ALERT_NONE && !isInSleepMode) {
    digitalWrite(GREEN_LED, !digitalRead(GREEN_LED));
    counter = 0;
  }
}