# 🌦️ Weather App (libcurl + raylib)

An **educational open‑source project** built to learn and get hands‑on exposure to the basics of **libcurl** (for HTTP requests) and **raylib** (for graphics/UI).

---

## 🎯 Project Goals

* Learn how to use **libcurl** to fetch data from a web API
* Learn how to use **raylib** to create a graphical window and render text
* Understand basic **API handling**, **JSON response flow**, and **program structure in C**

---

## 🛠️ Tech Stack

* **Language:** C
* **Networking:** libcurl
* **Graphics/UI:** raylib
* **API:** OpenWeatherMap (or any compatible weather API)

---

## ✨ Current Features

* **Secure HTTPS API calls** to OpenWeatherMap
* **City selection** via command-line argument
* **GUI error messages** for better user feedback
* **Refresh button** to update weather without restarting
* Displays weather information in a raylib window
* Shows temperature (Celsius), humidity, weather condition
* Dynamic weather banners and icons based on conditions
* Retro pixel-art font for aesthetic appeal

---

## 🖼️ Output Screen
 <img width="598" height="276" alt="Screenshot from 2026-01-05 22-42-01" src="https://github.com/user-attachments/assets/0f66c120-9887-44f1-aefd-4435a5ebd8e1" />

---

## 🚀 Getting Started

### Prerequisites

* GCC or Clang
* libcurl installed
* raylib installed
* OpenWeatherMap API key ([Get one free here](https://openweathermap.org/api))

### Build & Run

```bash
# Set your API key (required)
export OPENWEATHER_API_KEY="your_api_key_here"

# Build the application
./build.sh

# Run with default city (Lahore)
./weather_app

# Run with a specific city
./weather_app "London"
./weather_app "New York"
./weather_app "Tokyo"
```

### Using the Refresh Button

Once the app is running:
- Click the green **Refresh** button in the bottom-right corner
- Weather data updates automatically
- No need to restart the application

### Error Handling

The app now displays helpful error messages in the GUI:
- **Missing API Key**: Instructions to set environment variable
- **Invalid City**: Notification when city is not found
- **Network Errors**: Clear error messages for connection issues
- **Usage Hints**: Command-line usage displayed on errors
---

## 🤝 Contributing

This project is **open to beginners** and first‑time contributors.

You can help by:

* Improving UI/layout
* Adding error handling (invalid city, no internet, API errors)
* Cleaning up and refactoring code
* Adding new features (forecast, icons, units toggle)

Please check the **Issues** tab for beginner‑friendly tasks labeled:

* `good first issue`
* `help wanted`

---


## 📜 License

MIT License

---

⭐ If you find this project helpful, feel free to star the repository and share feedback!

