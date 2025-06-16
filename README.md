# Smart Environmental Monitoring System

[![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)](https://www.arduino.cc/)
[![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

> **Professional-grade embedded system for real-time environmental monitoring with data logging, multi-level alerts, and power management capabilities.**

## 📋 Project Overview

The Smart Environmental Monitoring System is an advanced Arduino-based embedded application that continuously monitors environmental conditions including temperature, humidity, and ambient light levels. The system features a sophisticated user interface, comprehensive data logging, intelligent alert system, and power-efficient sleep mode operation.

**🎓 Academic Project**: Developed for Embedded Systems course, demonstrating professional embedded programming practices and system integration.

## ✨ Key Features

### 🌡️ **Environmental Monitoring**
- **Temperature & Humidity**: DHT11 sensor with user-configurable calibration offsets
- **Light Level**: LDR sensor with percentage-based readings and professional calibration
- **Real-time Display**: 16x2 LCD with automatic screen rotation between sensor readings
- **Multi-level Alerts**: Visual (LED) and audible (buzzer) notifications with configurable thresholds

### 💾 **Data Management**
- **SD Card Logging**: CSV format data storage with timestamps
- **Real-time Clock**: DS3231 RTC module for accurate timestamping
- **Configurable Intervals**: User-adjustable logging frequency (10-3600 seconds)
- **Data Persistence**: Non-volatile storage with error handling and recovery

### 🔧 **Advanced System Features**
- **Power Management**: Intelligent sleep mode with configurable idle timeout
- **Wake-on-Alert**: Automatic wake-up when sensor thresholds are exceeded
- **Professional UI**: Seven-screen settings interface with joystick navigation
- **Sensor Calibration**: User-adjustable temperature and humidity offsets
- **Custom UART**: Register-level implementation for external communication

### 🛡️ **Reliability & Safety**
- **Error Handling**: Comprehensive sensor validation and graceful degradation
- **Threshold Monitoring**: Configurable high/low limits for all environmental parameters
- **Status Indication**: Color-coded LED system (Green/Yellow/Red) for immediate status recognition
- **Robust Operation**: Continues monitoring even with individual component failures

## 🔧 Hardware Requirements

### **Core Components**
- **Arduino Uno/Nano** (ATmega328P microcontroller)
- **DHT11** Temperature & Humidity Sensor
- **LDR** Light-Dependent Resistor (Photoresistor)
- **16x2 LCD Display** with I2C interface
- **DS3231 RTC Module** with battery backup
- **Micro SD Card Module**
- **Analog Joystick Module**

### **User Interface Components**
- **LEDs**: Green, Yellow, Red status indicators
- **Passive Buzzer** for audio alerts
- **220Ω Resistors** for LED current limiting
- **Breadboard & Jumper Wires**

### **Storage & Power**
- **Micro SD Card** (formatted FAT16/FAT32)
- **CR2032 Battery** for RTC backup
- **5V Power Supply** (USB or external adapter)

## 📊 Pin Configuration

| Component | Arduino Pin | Type | Description |
|-----------|-------------|------|-------------|
| DHT11 Data | Pin 2 | Digital I/O | Temperature/Humidity sensor |
| LDR Signal | A0 | Analog Input | Light level reading |
| Joystick X | A1 | Analog Input | X-axis position |
| Joystick Y | A2 | Analog Input | Y-axis position |
| Joystick Button | Pin 3 | Digital Input | Navigation button |
| Green LED | Pin 5 | Digital Output | Normal status indicator |
| Yellow LED | Pin 6 | Digital Output | Warning status indicator |
| Red LED | Pin 7 | Digital Output | Critical status indicator |
| Buzzer | Pin 9 | PWM Output | Audio alerts |
| SD Card CS | Pin 10 | Digital Output | SPI chip select |
| SD Card MOSI | Pin 11 | Digital Output | SPI data out |
| SD Card MISO | Pin 12 | Digital Input | SPI data in |
| SD Card SCK | Pin 13 | Digital Output | SPI clock |
| LCD SDA | A4 | I2C Data | Display communication |
| LCD SCL | A5 | I2C Clock | Display communication |
| RTC SDA | A4 | I2C Data | Real-time clock (shared) |
| RTC SCL | A5 | I2C Clock | Real-time clock (shared) |

## 🚀 Quick Start

### **1. Hardware Setup**
1. Connect all components according to the pin configuration table
2. Insert formatted micro SD card into SD module
3. Ensure RTC module has CR2032 battery installed
4. Connect Arduino to 5V power source

### **2. Software Installation**
1. Install Arduino IDE (version 2.0+)
2. Install required libraries:
   ```
   - DHT sensor library by Adafruit
   - LiquidCrystal_I2C
   - RTClib by Adafruit
   - SD (included with Arduino IDE)
   - SPI (included with Arduino IDE)
   - Wire (included with Arduino IDE)
   ```
3. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/smart-environmental-monitor.git
   ```
4. Open `Smart_Env_Monitor_V5.ino` in Arduino IDE
5. Upload to your Arduino board

### **3. Initial Configuration**
1. Power on the system - wait for "Init cmplt" message
2. Navigate to Settings menu using joystick
3. Configure alert thresholds for your environment
4. Enable data logging if desired
5. Set appropriate sleep timeout

## 🎮 User Interface

### **Navigation Controls**
- **Joystick Left/Right**: Switch between main display screens
- **Joystick Up/Down**: Navigate settings menus or adjust values
- **Joystick Button**: Enter/exit settings mode, toggle auto-rotation

### **Display Screens**
1. **Temperature/Humidity**: Current readings with status indicators (L/N/H)
2. **Light Level**: Ambient light percentage with status indicators
3. **Settings Menu**: Seven configuration categories

### **Settings Categories**
1. **Main Settings**: Alarm enable/disable, auto-rotation control
2. **Temperature Thresholds**: High/low limits (adjustable in 0.5°C steps)
3. **Humidity Thresholds**: High/low limits (adjustable in 1% steps)
4. **Light Thresholds**: High/low limits (adjustable in 10% steps)
5. **Data Logging**: Enable/disable, interval configuration (10-3600s)
6. **Sleep Settings**: Sleep mode enable, idle timeout (10-120s)
7. **Calibration**: Temperature/humidity offset adjustments (±10°C, ±20%)

### **Status Indicators**
- **🟢 Green LED**: All parameters normal
- **🟡 Yellow LED**: Warning conditions (parameters approaching limits)
- **🔴 Red LED**: Critical conditions (parameters outside safe ranges)
- **🔊 Buzzer Patterns**:
  - Single beep: Navigation feedback
  - Rapid beeping: High alert condition
  - Slow beeping: Low alert condition

## 💾 Data Logging

### **Log File Format**
Data is automatically saved to `log.csv` on the SD card in the following format:

```csv
Date,Time,T,H,L,A
5/26,08:30,23.5,45.2,75,0
5/26,09:30,24.1,43.8,68,1
```

**Columns:**
- **Date**: MM/DD format
- **Time**: HH:MM format (24-hour)
- **T**: Temperature (°C)
- **H**: Humidity (%)
- **L**: Light level (%)
- **A**: Alert level (0=Normal, 1=Low, 2=High)

### **Data Retrieval**
1. Power down system safely
2. Remove SD card and insert into computer
3. Open `log.csv` with any spreadsheet application
4. Analyze environmental trends and alert patterns

## ⚡ Power Management

### **Sleep Mode Operation**
- **Automatic Sleep**: Enters low-power mode after configurable idle period
- **Wake Conditions**: Joystick movement, button press, or sensor alert
- **Power Savings**: LCD backlight off, reduced system activity
- **Continuous Monitoring**: Sensors checked periodically during sleep
- **Immediate Logging**: Data logged immediately upon wake-up

### **Wake-up Behavior**
1. LCD backlight restored
2. Appropriate status LED activated
3. Data logging resumed
4. 60-second logging interval restarted

## 🔧 Technical Specifications

### **MCU Functionalities Implemented (15 total)**
- **Digital I/O**: LED control, buzzer, joystick button
- **ADC**: Light sensor, joystick position reading
- **PWM**: Buzzer tone generation with varied patterns
- **UART**: Custom register-level implementation
- **Timer/Interrupts**: Background LED management
- **I2C**: LCD and RTC communication
- **SPI**: SD card data storage
- **Additional Peripherals**: DHT11, RTC, SD card, joystick, multi-color LEDs

### **Communication Protocols**
- **I2C Bus**: LCD (0x27) + RTC (0x68) - shared bus
- **SPI Bus**: SD card module - dedicated bus
- **1-Wire**: DHT11 sensor proprietary protocol
- **Custom UART**: Register-level 9600 baud implementation

### **Performance Characteristics**
- **Sensor Reading**: Every 2 seconds
- **Display Update**: 500ms refresh rate
- **User Input**: 200ms joystick polling
- **Data Logging**: User-configurable (10-3600 seconds)
- **Alert Response**: <100ms for threshold violations

## 🛠️ Advanced Features

### **Register-Level Programming**
The system demonstrates professional embedded programming with direct register manipulation:

```cpp
//Custom UART implementation
void initUART() {
    UBRR0H = 0; UBRR0L = 103;        //9600 baud calculation
    UCSR0B = (1 << TXEN0);           //Enable transmitter
    UCSR0C = (3 << UCSZ00);          //8N1 format
}

//Timer interrupt configuration
void setupInterrupts() {
    TCCR2A = (1 << WGM21);           //CTC mode
    TCCR2B = (7 << CS20);            //1024 prescaler
    OCR2A = 155;                     //Compare value
    TIMSK2 = (1 << OCIE2A);          //Enable interrupt
}
```

### **Professional Calibration System**
- **Temperature Offset**: ±10°C adjustment range
- **Humidity Offset**: ±20% adjustment range
- **Light Sensor Calibration**: Configurable dark/bright reference points
- **User-Friendly Interface**: Real-time adjustment via settings menu

## 📚 Documentation

### **Complete Documentation Package**
- **📄 Technical Report**: Comprehensive 16-page project documentation
- **🔧 Pin Connection Table**: Detailed hardware specifications
- **📋 User Manual**: Complete operation and troubleshooting guide
- **🔬 Failure Mode Analysis**: Professional reliability assessment
- **📊 Component Datasheets**: Complete hardware documentation

### **Code Quality**
- **Professional Structure**: Modular design with clear function separation
- **Comprehensive Comments**: Detailed code documentation throughout
- **Error Handling**: Robust operation with graceful degradation
- **Industry Standards**: Consistent naming conventions and formatting

## 🤝 Team & Contributions

**Team Members:**
- **Daniyar Zhumatayev (253857)** - Team Leader, Sensors & Data Acquisition (20%)
- **Kuzma Martysiuk (253854)** - Display & User Interface (20%)
- **Maksym Tsyhypalo (253845)** - Data Logging & Storage (20%)
- **Mateusz Siwocha (250355)** - Communication & Alert System (20%)
- **Wojciech Wojcieszek (247048)** - Power Management & System Integration (20%)

## 🚨 Troubleshooting

### **Common Issues**

**No Display Output:**
- Check LCD I2C connections (SDA→A4, SCL→A5)
- Verify I2C address (default: 0x27)
- Ensure 5V power supply stability

**Incorrect Sensor Readings:**
- Allow 30-second stabilization after power-on
- Use calibration settings to adjust sensor offsets
- Check sensor connections and environmental conditions

**Data Logging Problems:**
- Verify SD card is formatted (FAT16/FAT32)
- Check SD card module SPI connections
- Ensure data logging is enabled in settings menu
- Verify RTC battery is installed and functional

**Sleep Mode Issues:**
- Check sleep mode is enabled in settings
- Verify idle timeout configuration
- Ensure no continuous alerts preventing sleep

## 📈 Future Enhancements

### **Potential Improvements**
- **Wireless Connectivity**: WiFi/Bluetooth modules for remote monitoring
- **Additional Sensors**: Soil moisture, air quality, atmospheric pressure
- **Data Visualization**: Graphical display or web interface
- **IoT Integration**: Cloud connectivity for remote data access
- **Mobile App**: Smartphone interface for system control

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Arduino Community** for excellent libraries and documentation
- **Adafruit Industries** for high-quality sensor libraries
- **Embedded Systems Course** for project guidance and requirements
- **Open Source Community** for tools and resources

## 📞 Support

For technical support or questions about this project:
- **Create an issue** in this repository
- **Check the documentation** in the `/docs` folder
- **Review the troubleshooting guide** above

---

**⭐ If you find this project useful, please consider giving it a star!**

**🔗 Related Projects**: [Arduino Weather Station](https://github.com/topics/arduino-weather-station) | [Embedded Systems](https://github.com/topics/embedded-systems) | [IoT Sensors](https://github.com/topics/iot-sensors)