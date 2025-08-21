# _Custom Datalogger_

## Overview
Custom Datalogger is an ESP32-based embedded system that uses a set of modular, custom drivers to read and display temperature and humidity data from a DHT11 sensor. Readings are displayed on both a web interface and an LCD screen with multiple display modes. The system includes IR and button controls, audio feedback, Wi-Fi connectivity, and status LEDs to give users real-time system feedback.

## Features
- **Sensor Data Collection:** Temperature and humidity readings using a DHT11 sensor  
- **LCD Display Modes:** Switch between temperature, humidity, and time since last read  
- **Control Options:** IR remote and physical button to switch display modes or trigger a reading  
- **Web Interface:** Hosts a simple web server displaying live data and allowing manual readings  
- **Speaker Feedback:** Plays a sound when a new reading is taken  
- **Status LED:**  
  - Green: Ready  
  - Yellow: Setup in progress  
  - Blinking Red: Critical error  
- **Time Management:** Internal timekeeping to track time since last read, adjustable via the `timeset` driver  
- **Auto & Manual Reads:** Automatically takes a reading every minute, or instantly on-demand via web or IR  

## Project Structure

The project **datalogger** contains one source file in C++ language [main.cpp](main/main.cpp). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── components
│   └── button
│       ├── CMakeLists.txt
│       ├── button.c
│       ├── button.h
│       ├── button_task.cpp
│       └── button_task.hpp
│   └── dht11
│       ├── CMakeLists.txt
│       ├── dht11.c
│       ├── dht11.h
│       ├── dht11_task.cpp
│       └── dht11_task.hpp
│   └── irdecoder
│       ├── CMakeLists.txt
│       ├── irdecoder.c
│       ├── irdecoder.h
│       ├── irdecoder.cpp
│       └── irdecoder.hpp
│   └── lcd
│       ├── CMakeLists.txt
│       ├── lcd_i2c.c
│       ├── lcd_i2c.h
│       ├── lcd_task.cpp
│       └── lcd_task.hpp
│   └── speaker
│       ├── CMakeLists.txt
│       ├── audio_data_generator.py
│       ├── speaker.c
│       ├── speaker.h
│       ├── speaker_task.cpp
│       ├── speaker_task.hpp
│   └── statusled
│       ├── CMakeLists.txt
│       ├── statusled.c
│       └── statusled.h
│   └── timeset
│       ├── CMakeLists.txt
│       └── timeset.c
│       └── timeset.h
│   └── webserver
│       ├── CMakeLists.txt
│       ├── webserver.cpp
│       ├── webserver.hpp
│       ├── index.html
│       ├── style.css
│       └── script.js
│   └── wifi
│       ├── CMakeLists.txt
│       ├── wifi.c
│       └── wifi.h
├── main
│   ├── CMakeLists.txt
│   └── main.cpp
└── README.md                  This is the file you are currently reading
```
### Special Files

- `speaker/audio_data_generator.py`: Converts wav files to a header files for embedding audio (e.g. converting power_on.wav, power_off.wav, reading_taken.wav to power_on.h, power_off.h, and reading_taken.h respectively)
- `webserver/index.html`, `style.css`, `script.js`: Embedded in the firmware for hosting the web UI  
