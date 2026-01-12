
#include "raylib.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <wchar.h>
#include <locale.h>
#include <math.h>

// Modern color palette
#define BG_DARK ((Color){15, 23, 42, 255})           // Slate-900
#define BG_CARD ((Color){30, 41, 59, 255})           // Slate-800
#define BG_CARD_HOVER ((Color){51, 65, 85, 255})     // Slate-700
#define ACCENT_PRIMARY ((Color){99, 102, 241, 255})  // Indigo-500
#define ACCENT_HOVER ((Color){79, 70, 229, 255})     // Indigo-600
#define TEXT_PRIMARY ((Color){248, 250, 252, 255})   // Slate-50
#define TEXT_SECONDARY ((Color){148, 163, 184, 255}) // Slate-400
#define SUCCESS_COLOR ((Color){34, 197, 94, 255})    // Green-500
#define ERROR_COLOR ((Color){239, 68, 68, 255})      // Red-500
#define WARNING_COLOR ((Color){251, 191, 36, 255})   // Amber-400

// Application state enum for error handling
typedef enum {
  STATE_LOADING,
  STATE_SUCCESS,
  STATE_ERROR_API_KEY,
  STATE_ERROR_NETWORK,
  STATE_ERROR_INVALID_CITY,
  STATE_ERROR_JSON_PARSE
} AppState;

struct Memory
{
  char *data;
  size_t size;
};

typedef struct weatherData
{
  Texture2D weatherlogo;
  Texture2D weatherBanner;
  char weatherName[100];
  char city[100];
  int weatherID;
  char temperature[32];
  char humidity[32];
  char country[100];
  char errorMessage[256];
  char description[256];  // Weather description
  int feelsLike;          // Feels like temperature
  int windSpeed;          // Wind speed
} weatherData;

// Animation state
typedef struct {
  float cardScale;
  float buttonScale;
  float logoRotation;
  float logoFloat;
  float fadeIn;
  float shimmerOffset;
} AnimationState;

size_t callback_func(void *ptr, size_t size, size_t num_of_members, void *userData)
{
  size_t total = size * num_of_members;
  struct Memory *mem = (struct Memory *)userData;
  char *temp = realloc(mem->data, mem->size + total + 1);
  if (temp == NULL)
  {
    return 0;
  }

  mem->data = temp;

  memcpy(&(mem->data[mem->size]), ptr, total);
  mem->size += total;
  mem->data[mem->size] = '\0';
  return total;
}

// Enhanced Button structure for UI
typedef struct {
  Rectangle bounds;
  bool isHovered;
  bool isPressed;
  float hoverProgress;
  float pressProgress;
} Button;

// Function to draw a rounded rectangle with gradient
void DrawRoundedRectangleGradient(Rectangle rec, float roundness, int segments, Color colorTop, Color colorBottom) {
  // Draw main rounded rectangle
  DrawRectangleRounded(rec, roundness, segments, colorTop);
  
  // Draw gradient overlay
  for (int i = 0; i < rec.height; i++) {
    float t = (float)i / rec.height;
    Color gradColor = (Color){
      (unsigned char)(colorTop.r + (colorBottom.r - colorTop.r) * t),
      (unsigned char)(colorTop.g + (colorBottom.g - colorTop.g) * t),
      (unsigned char)(colorTop.b + (colorBottom.b - colorTop.b) * t),
      (unsigned char)(colorTop.a + (colorBottom.a - colorTop.a) * t)
    };
    DrawRectangle(rec.x, rec.y + i, rec.width, 1, gradColor);
  }
  
  // Draw rounded corners on top
  DrawRectangleRounded(rec, roundness, segments, Fade(colorTop, 0.5f));
}

// Function to draw enhanced button with animations
void DrawEnhancedButton(Button *btn, const char *text, Font font, int fontSize, AnimationState *anim) {
  // Update hover animation
  if (btn->isHovered) {
    btn->hoverProgress = fminf(btn->hoverProgress + 0.1f, 1.0f);
  } else {
    btn->hoverProgress = fmaxf(btn->hoverProgress - 0.1f, 0.0f);
  }
  
  // Update press animation
  if (btn->isPressed) {
    btn->pressProgress = fminf(btn->pressProgress + 0.2f, 1.0f);
  } else {
    btn->pressProgress = fmaxf(btn->pressProgress - 0.2f, 0.0f);
  }
  
  // Calculate button scale
  float scale = 1.0f - (btn->pressProgress * 0.05f);
  Rectangle scaledBounds = {
    btn->bounds.x + (btn->bounds.width * (1 - scale)) / 2,
    btn->bounds.y + (btn->bounds.height * (1 - scale)) / 2,
    btn->bounds.width * scale,
    btn->bounds.height * scale
  };
  
  // Draw shadow
  Rectangle shadowRect = {scaledBounds.x + 2, scaledBounds.y + 4, scaledBounds.width, scaledBounds.height};
  DrawRectangleRounded(shadowRect, 0.3f, 16, Fade(BLACK, 0.3f));
  
  // Draw button background with gradient
  Color btnColor = btn->isHovered ? ACCENT_HOVER : ACCENT_PRIMARY;
  Color btnColorDark = (Color){btnColor.r - 30, btnColor.g - 30, btnColor.b - 30, 255};
  
  DrawRectangleRounded(scaledBounds, 0.3f, 16, btnColor);
  
  // Draw shimmer effect on hover
  if (btn->hoverProgress > 0) {
    Rectangle shimmerRect = {
      scaledBounds.x + anim->shimmerOffset - 50,
      scaledBounds.y,
      50,
      scaledBounds.height
    };
    DrawRectangleRounded(shimmerRect, 0.3f, 16, Fade(WHITE, 0.2f * btn->hoverProgress));
  }
  
  // Draw border
  DrawRectangleRoundedLines(scaledBounds, 0.3f, 16, Fade(WHITE, 0.2f + btn->hoverProgress * 0.3f));
  
  // Draw text
  Vector2 textSize = MeasureTextEx(font, text, fontSize, 1);
  Vector2 textPos = {
    scaledBounds.x + (scaledBounds.width - textSize.x) / 2,
    scaledBounds.y + (scaledBounds.height - textSize.y) / 2
  };
  DrawTextEx(font, text, textPos, fontSize, 1, WHITE);
}

// Function to draw a card with shadow and rounded corners
void DrawCard(Rectangle bounds, float roundness, Color color, float shadowIntensity) {
  // Draw shadow
  Rectangle shadowRect = {bounds.x + 4, bounds.y + 6, bounds.width, bounds.height};
  DrawRectangleRounded(shadowRect, roundness, 16, Fade(BLACK, shadowIntensity));
  
  // Draw card
  DrawRectangleRounded(bounds, roundness, 16, color);
  
  // Draw subtle border
  DrawRectangleRoundedLines(bounds, roundness, 16, Fade(WHITE, 0.1f));
}

// Function to fetch weather data for a given city
AppState fetchWeatherData(const char *city, const char *API_KEY, weatherData *myData, 
                          const char **fullPath_of_WeatherBanner, const char **fullPath_of_WeatherLogo,
                          const char *basePath) {
  AppState appState = STATE_LOADING;
  char url[256] = {0};
  snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", city, API_KEY);

  struct Memory chunk = {0};
  chunk.data = malloc(1);
  if (chunk.data == NULL) {
    fprintf(stderr, "malloc failed to allocate data for the chunk");
    snprintf(myData->errorMessage, sizeof(myData->errorMessage), 
             "Memory Error\nFailed to allocate memory");
    return STATE_ERROR_NETWORK;
  }
  chunk.size = 0;
  cJSON *json = NULL;

  CURL *curl;
  CURLcode result = curl_global_init(CURL_GLOBAL_ALL);

  if (result != CURLE_OK) {
    appState = STATE_ERROR_NETWORK;
    snprintf(myData->errorMessage, sizeof(myData->errorMessage), 
             "Network Error\nFailed to initialize CURL");
    free(chunk.data);
    return appState;
  }
  
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
      appState = STATE_ERROR_NETWORK;
      snprintf(myData->errorMessage, sizeof(myData->errorMessage), 
               "Network Error\n%s", curl_easy_strerror(result));
    } else {
      json = cJSON_Parse(chunk.data);
      if (json != NULL) {
        // Check if API returned an error (e.g., city not found)
        cJSON *cod = cJSON_GetObjectItemCaseSensitive(json, "cod");
        if (cJSON_IsNumber(cod) && cod->valueint == 404) {
          appState = STATE_ERROR_INVALID_CITY;
          snprintf(myData->errorMessage, sizeof(myData->errorMessage), 
                   "City Not Found\nPlease check the city name");
        } else if (cJSON_IsString(cod) && strcmp(cod->valuestring, "404") == 0) {
          appState = STATE_ERROR_INVALID_CITY;
          snprintf(myData->errorMessage, sizeof(myData->errorMessage), 
                   "City Not Found\nPlease check the city name");
        } else {
          // Parse all weather data
          cJSON *location = cJSON_GetObjectItemCaseSensitive(json, "name");
          if (cJSON_IsString(location) && strlen(location->valuestring) < sizeof(myData->city)) {
            strcpy(myData->city, location->valuestring);
          }
          
          cJSON *sys = cJSON_GetObjectItemCaseSensitive(json, "sys");
          if (cJSON_IsObject(sys)) {
            cJSON *country = cJSON_GetObjectItemCaseSensitive(sys, "country");
            if (cJSON_IsString(country) && strlen(country->valuestring) < sizeof(myData->country)) {
              strcpy(myData->country, country->valuestring);
            }
          }
          
          cJSON *weather_obj = cJSON_GetObjectItemCaseSensitive(json, "weather");
          cJSON *firstITEM = cJSON_GetArrayItem(weather_obj, 0);
          cJSON *weather_name = cJSON_GetObjectItemCaseSensitive(firstITEM, "main");
          if (cJSON_IsString(weather_name) && strlen(weather_name->valuestring) < sizeof(myData->weatherName)) {
            strcpy(myData->weatherName, weather_name->valuestring);
          }
          
          // Get weather description
          cJSON *weather_desc = cJSON_GetObjectItemCaseSensitive(firstITEM, "description");
          if (cJSON_IsString(weather_desc) && strlen(weather_desc->valuestring) < sizeof(myData->description)) {
            strcpy(myData->description, weather_desc->valuestring);
            // Capitalize first letter
            if (myData->description[0] >= 'a' && myData->description[0] <= 'z') {
              myData->description[0] = myData->description[0] - 32;
            }
          }
          
          cJSON *temperature_obj = cJSON_GetObjectItemCaseSensitive(json, "main");
          cJSON *temperature = cJSON_GetObjectItemCaseSensitive(temperature_obj, "temp");
          if (cJSON_IsNumber(temperature)) {
            snprintf(myData->temperature, sizeof(myData->temperature), "%d°C", (int)(temperature->valuedouble - 273.15));
          }
          
          // Get feels like temperature
          cJSON *feels_like = cJSON_GetObjectItemCaseSensitive(temperature_obj, "feels_like");
          if (cJSON_IsNumber(feels_like)) {
            myData->feelsLike = (int)(feels_like->valuedouble - 273.15);
          }
          
          cJSON *humidity = cJSON_GetObjectItemCaseSensitive(temperature_obj, "humidity");
          if (cJSON_IsNumber(humidity)) {
            snprintf(myData->humidity, sizeof(myData->humidity), "%d%%", (int)(humidity->valuedouble));
          }
          
          // Get wind speed
          cJSON *wind_obj = cJSON_GetObjectItemCaseSensitive(json, "wind");
          if (cJSON_IsObject(wind_obj)) {
            cJSON *wind_speed = cJSON_GetObjectItemCaseSensitive(wind_obj, "speed");
            if (cJSON_IsNumber(wind_speed)) {
              myData->windSpeed = (int)(wind_speed->valuedouble * 3.6); // Convert m/s to km/h
            }
          }
          
          cJSON *weatherID = cJSON_GetObjectItemCaseSensitive(firstITEM, "id");
          if (cJSON_IsNumber(weatherID)) {
            myData->weatherID = weatherID->valuedouble;
          }

          // Determining weather banner and logo
          if (myData->weatherID >= 200 && myData->weatherID <= 232) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/thunderStorm.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/thunderStorm.png", basePath);
          } else if (myData->weatherID >= 300 && myData->weatherID <= 321) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/rain.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/rain.png", basePath);
          } else if (myData->weatherID >= 500 && myData->weatherID <= 531) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/rain.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/rain.png", basePath);
          } else if (myData->weatherID >= 600 && myData->weatherID <= 622) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/snow.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/snow.png", basePath);
          } else if (myData->weatherID >= 701 && myData->weatherID <= 781) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/fog.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/fog.png", basePath);
          } else if (myData->weatherID == 800) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/clear.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/sunny.png", basePath);
          } else if (myData->weatherID > 800 && myData->weatherID <= 804) {
            *fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/clouds.jpg", basePath);
            *fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/clouds.png", basePath);
          }
          
          appState = STATE_SUCCESS;
        }
        cJSON_Delete(json);
      } else {
        appState = STATE_ERROR_JSON_PARSE;
        snprintf(myData->errorMessage, sizeof(myData->errorMessage), 
                 "Parse Error\nFailed to parse API response");
      }
    }
    curl_easy_cleanup(curl);
  }
  
  free(chunk.data);
  curl_global_cleanup();
  return appState;
}

int main(int argc, char *argv[])
{
  setlocale(LC_ALL, "");
  const int winWidth = 800;
  const int winHeight = 500;
  const char *basePath = GetApplicationDirectory();
  const char *fullPath_of_WeatherBanner = NULL;
  const char *fullPath_of_WeatherLogo = NULL;

  // Get city from command-line argument or use default
  const char *city = "Lahore";  // Default city
  if (argc > 1) {
    city = argv[1];
  }

  weatherData myData = {0};
  AppState appState = STATE_LOADING;
  
  const char *API_KEY = getenv("OPENWEATHER_API_KEY");
  if (!API_KEY || API_KEY[0] == '\0') {
      appState = STATE_ERROR_API_KEY;
      snprintf(myData.errorMessage, sizeof(myData.errorMessage), 
               "Missing API Key\nSet OPENWEATHER_API_KEY environment variable");
      printf("Missing API KEY. Set OPENWEATHER_API_KEY\n");
  } else {
    // Fetch weather data on startup
    appState = fetchWeatherData(city, API_KEY, &myData, &fullPath_of_WeatherBanner, &fullPath_of_WeatherLogo, basePath);
  }

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(winWidth, winHeight, "Weather App - Modern UI");
  SetWindowMinSize(800, 500);
  
  // Only load textures if we have successful data
  if (appState == STATE_SUCCESS) {
    if(fullPath_of_WeatherBanner ==NULL){
        fprintf(stderr, "Failed to load path for weather banner");
    } else {
      myData.weatherBanner = LoadTexture(fullPath_of_WeatherBanner);
      if(myData.weatherBanner.id==0){
          fprintf(stderr, "unable to load weather banner texture");
      }
    }

    if(fullPath_of_WeatherLogo==NULL){
        fprintf(stderr, "Failed to load path for weather logo");
    } else {
      myData.weatherlogo = LoadTexture(fullPath_of_WeatherLogo);
      if(myData.weatherlogo.id==0){
          fprintf(stderr, "unable to load weather logo texture");
      }
    }
  }
  
  // Load fonts - using default font for better readability
  Font customFont = GetFontDefault();
  Font regularFont = GetFontDefault();
  
  // Initialize animation state
  AnimationState anim = {0};
  anim.fadeIn = 0.0f;
  anim.cardScale = 0.8f;
  anim.buttonScale = 0.8f;
  anim.shimmerOffset = 0.0f;

  // Create refresh button
  Button refreshButton = {0};
  refreshButton.bounds = (Rectangle){winWidth - 160, winHeight - 70, 140, 50};
  refreshButton.isHovered = false;
  refreshButton.hoverProgress = 0.0f;
  refreshButton.pressProgress = 0.0f;

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    // Update window dimensions
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    
    // Update button position based on window size
    refreshButton.bounds.x = currentWidth - 160;
    refreshButton.bounds.y = currentHeight - 70;
    
    // Update animations
    anim.fadeIn = fminf(anim.fadeIn + 0.02f, 1.0f);
    anim.cardScale = fminf(anim.cardScale + 0.02f, 1.0f);
    anim.buttonScale = fminf(anim.buttonScale + 0.02f, 1.0f);
    anim.logoFloat = sinf(GetTime() * 2.0f) * 5.0f;
    anim.logoRotation = sinf(GetTime() * 0.5f) * 2.0f;
    anim.shimmerOffset += 3.0f;
    if (anim.shimmerOffset > currentWidth + 100) anim.shimmerOffset = -100;
    
    // Update button hover state
    Vector2 mousePos = GetMousePosition();
    refreshButton.isHovered = CheckCollisionPointRec(mousePos, refreshButton.bounds);
    refreshButton.isPressed = refreshButton.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    
    // Handle refresh button click
    if (refreshButton.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && API_KEY) {
      // Unload old textures if they exist
      if (appState == STATE_SUCCESS) {
        if (myData.weatherBanner.id != 0) UnloadTexture(myData.weatherBanner);
        if (myData.weatherlogo.id != 0) UnloadTexture(myData.weatherlogo);
      }
      
      // Reset data and fetch new weather
      memset(&myData, 0, sizeof(weatherData));
      appState = fetchWeatherData(city, API_KEY, &myData, &fullPath_of_WeatherBanner, &fullPath_of_WeatherLogo, basePath);
      
      // Load new textures if successful
      if (appState == STATE_SUCCESS) {
        if (fullPath_of_WeatherBanner) {
          myData.weatherBanner = LoadTexture(fullPath_of_WeatherBanner);
        }
        if (fullPath_of_WeatherLogo) {
          myData.weatherlogo = LoadTexture(fullPath_of_WeatherLogo);
        }
      }
      
      // Reset animations
      anim.fadeIn = 0.0f;
      anim.cardScale = 0.8f;
    }

    BeginDrawing();
    ClearBackground(BG_DARK);
    
    // Draw background pattern
    for (int i = 0; i < currentWidth; i += 40) {
      for (int j = 0; j < currentHeight; j += 40) {
        DrawCircle(i, j, 1, Fade(TEXT_SECONDARY, 0.05f));
      }
    }
    
    // Display content based on application state
    if (appState == STATE_SUCCESS) {
      // Main weather card
      Rectangle mainCard = {
        40 + (1 - anim.cardScale) * 200,
        40 + (1 - anim.cardScale) * 100,
        currentWidth - 80,
        currentHeight - 140
      };
      mainCard.width *= anim.cardScale;
      mainCard.height *= anim.cardScale;
      
      // Draw shadow for the entire card
      Rectangle shadowRect = {mainCard.x + 4, mainCard.y + 6, mainCard.width, mainCard.height};
      DrawRectangleRounded(shadowRect, 0.05f, 16, Fade(BLACK, 0.4f));
      
      // Draw weather banner with rounded top corners
      if (myData.weatherBanner.id != 0) {
        Rectangle bannerRect = {mainCard.x, mainCard.y, mainCard.width, 200};
        Rectangle srcRect = {0, 0, (float)myData.weatherBanner.width, (float)myData.weatherBanner.height};
        
        // Draw the banner image with rounded top corners - no fade animation
        DrawTexturePro(myData.weatherBanner, srcRect, bannerRect, (Vector2){0, 0}, 0, WHITE);
        
        // Light overlay for better text readability
        DrawRectangleRounded(bannerRect, 0.05f, 16, Fade((Color){0, 0, 0, 60}, 0.8f));
      }
      
      // Draw the bottom part of the card (below the banner)
      Rectangle bottomCard = {mainCard.x, mainCard.y + 200, mainCard.width, mainCard.height - 200};
      DrawRectangle(bottomCard.x, bottomCard.y, bottomCard.width, bottomCard.height, BG_CARD);
      
      // Draw rounded bottom corners
      DrawRectangleRounded((Rectangle){mainCard.x, mainCard.y + mainCard.height - 20, mainCard.width, 20}, 0.5f, 16, BG_CARD);
      
      // Draw subtle border around entire card
      DrawRectangleRoundedLines(mainCard, 0.05f, 16, Fade(WHITE, 0.1f));
      
      // City name and country
      Vector2 cityPos = {mainCard.x + 30, mainCard.y + 30};
      DrawTextEx(regularFont, TextFormat("%s, %s", myData.city, myData.country), 
                 cityPos, 32, 2, Fade(TEXT_PRIMARY, anim.fadeIn));
      
      // Weather description
      Vector2 descPos = {mainCard.x + 30, mainCard.y + 70};
      DrawTextEx(regularFont, myData.description, descPos, 20, 1, Fade(TEXT_SECONDARY, anim.fadeIn));
      
      // Temperature (large) - Draw with black rounded background
      char tempStr[64];
      snprintf(tempStr, sizeof(tempStr), "%s", myData.temperature);
      Vector2 tempSize = MeasureTextEx(customFont, tempStr, 96, 3);
      Vector2 tempPos = {mainCard.x + 30, mainCard.y + 110};
      
      // Draw black rounded background for temperature with no white corners
      Rectangle tempBg = {
        tempPos.x - 15, 
        tempPos.y - 10, 
        tempSize.x + 30, 
        tempSize.y + 20
      };
      
      // Draw filled rounded rectangle with higher segment count for smoother corners
      DrawRectangleRounded(tempBg, 0.2f, 32, Fade(BLACK, anim.fadeIn * 0.8f));
      
      // Draw temperature text
      DrawTextEx(customFont, tempStr, tempPos, 96, 3, Fade(TEXT_PRIMARY, anim.fadeIn));
      
      // Weather icon with animation
      if (myData.weatherlogo.id != 0) {
        float logoScale = 0.4f;
        float logoX = mainCard.x + mainCard.width - 200;
        float logoY = mainCard.y + 50 + anim.logoFloat;
        
        DrawTextureEx(myData.weatherlogo, 
                     (Vector2){logoX, logoY}, 
                     anim.logoRotation, 
                     logoScale, 
                     Fade(WHITE, anim.fadeIn));
      }
      
      // Info cards section
      float cardY = mainCard.y + 250;
      float cardSpacing = 20;
      float cardWidth = (mainCard.width - 90) / 3;
      
      // Feels like card
      Rectangle feelsLikeCard = {mainCard.x + 30, cardY, cardWidth, 100};
      DrawCard(feelsLikeCard, 0.08f, BG_CARD_HOVER, 0.2f);
      DrawTextEx(regularFont, "FEELS LIKE", 
                 (Vector2){feelsLikeCard.x + 20, feelsLikeCard.y + 20}, 
                 14, 1, Fade(TEXT_SECONDARY, anim.fadeIn));
      DrawTextEx(regularFont, TextFormat("%d°C", myData.feelsLike), 
                 (Vector2){feelsLikeCard.x + 20, feelsLikeCard.y + 50}, 
                 32, 2, Fade(TEXT_PRIMARY, anim.fadeIn));
      
      // Humidity card
      Rectangle humidityCard = {mainCard.x + 30 + cardWidth + cardSpacing, cardY, cardWidth, 100};
      DrawCard(humidityCard, 0.08f, BG_CARD_HOVER, 0.2f);
      DrawTextEx(regularFont, "HUMIDITY", 
                 (Vector2){humidityCard.x + 20, humidityCard.y + 20}, 
                 14, 1, Fade(TEXT_SECONDARY, anim.fadeIn));
      DrawTextEx(regularFont, myData.humidity, 
                 (Vector2){humidityCard.x + 20, humidityCard.y + 50}, 
                 32, 2, Fade(TEXT_PRIMARY, anim.fadeIn));
      
      // Wind speed card
      Rectangle windCard = {mainCard.x + 30 + (cardWidth + cardSpacing) * 2, cardY, cardWidth, 100};
      DrawCard(windCard, 0.08f, BG_CARD_HOVER, 0.2f);
      DrawTextEx(regularFont, "WIND SPEED", 
                 (Vector2){windCard.x + 20, windCard.y + 20}, 
                 14, 1, Fade(TEXT_SECONDARY, anim.fadeIn));
      DrawTextEx(regularFont, TextFormat("%d km/h", myData.windSpeed), 
                 (Vector2){windCard.x + 20, windCard.y + 50}, 
                 32, 2, Fade(TEXT_PRIMARY, anim.fadeIn));
      
    } else {
      // Error state - centered card
      Rectangle errorCard = {
        currentWidth / 2 - 300,
        currentHeight / 2 - 150,
        600,
        300
      };
      
      DrawCard(errorCard, 0.05f, BG_CARD, 0.4f);
      
      const char *errorTitle = "Error";
      Color errorColor = ERROR_COLOR;
      const char *errorIcon = "✕";
      
      switch(appState) {
        case STATE_LOADING:
          errorTitle = "Loading...";
          errorColor = WARNING_COLOR;
          errorIcon = "⟳";
          break;
        case STATE_ERROR_API_KEY:
          errorTitle = "API Key Error";
          errorIcon = "🔑";
          break;
        case STATE_ERROR_NETWORK:
          errorTitle = "Network Error";
          errorIcon = "📡";
          break;
        case STATE_ERROR_INVALID_CITY:
          errorTitle = "Invalid City";
          errorIcon = "📍";
          break;
        case STATE_ERROR_JSON_PARSE:
          errorTitle = "Data Error";
          errorIcon = "⚠";
          break;
        default:
          errorTitle = "Unknown Error";
      }
      
      // Draw error icon
      DrawTextEx(regularFont, errorIcon, 
                 (Vector2){errorCard.x + errorCard.width / 2 - 30, errorCard.y + 40}, 
                 60, 2, errorColor);
      
      // Draw error title
      Vector2 titleSize = MeasureTextEx(regularFont, errorTitle, 32, 2);
      DrawTextEx(regularFont, errorTitle, 
                 (Vector2){errorCard.x + (errorCard.width - titleSize.x) / 2, errorCard.y + 120}, 
                 32, 2, TEXT_PRIMARY);
      
      // Draw error message
      Vector2 msgSize = MeasureTextEx(regularFont, myData.errorMessage, 18, 1);
      DrawTextEx(regularFont, myData.errorMessage, 
                 (Vector2){errorCard.x + (errorCard.width - msgSize.x) / 2, errorCard.y + 170}, 
                 18, 1, TEXT_SECONDARY);
      
      // Draw usage hint
      const char *hint = "Usage: ./weather_app [city_name]";
      Vector2 hintSize = MeasureTextEx(regularFont, hint, 14, 1);
      DrawTextEx(regularFont, hint, 
                 (Vector2){errorCard.x + (errorCard.width - hintSize.x) / 2, errorCard.y + 240}, 
                 14, 1, Fade(TEXT_SECONDARY, 0.6f));
    }
    
    // Draw refresh button with enhanced styling
    DrawEnhancedButton(&refreshButton, "Refresh", regularFont, 18, &anim);
    
    // Draw app title in bottom left
    DrawTextEx(regularFont, "Weather App", 
               (Vector2){20, currentHeight - 30}, 
               16, 1, Fade(TEXT_SECONDARY, 0.5f));

    EndDrawing();
  }

  // Cleanup
  if (myData.weatherBanner.id != 0) UnloadTexture(myData.weatherBanner);
  if (myData.weatherlogo.id != 0) UnloadTexture(myData.weatherlogo);
  CloseWindow();

  return 0;
}
