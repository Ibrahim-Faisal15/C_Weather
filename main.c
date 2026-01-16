
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "raylib.h"

#define MAIN_GRAY ((Color){52, 52, 52, 255})

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
  char errorMessage[256];  // Store error messages for GUI display

} weatherData;

size_t callback_func(void *ptr, size_t size, size_t num_of_members, void *userData)
{
  /*ptr is the temp packet holder from internet and we hold it in the userdata, userda
    is not converted to our requried struct and we call it mem now, we created a temp variable
    and reallocated memory.
  */
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

// Button structure for UI
typedef struct {
  Rectangle bounds;
  bool isHovered;
} Button;

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
          
          cJSON *temperature_obj = cJSON_GetObjectItemCaseSensitive(json, "main");
          cJSON *temperature = cJSON_GetObjectItemCaseSensitive(temperature_obj, "temp");
          if (cJSON_IsNumber(temperature)) {
            snprintf(myData->temperature, sizeof(myData->temperature), "%dC", (int)(temperature->valuedouble - 273.15));
          }
          
          cJSON *humidity = cJSON_GetObjectItemCaseSensitive(temperature_obj, "humidity");
          if (cJSON_IsNumber(humidity)) {
            snprintf(myData->humidity, sizeof(myData->humidity), "%d%%", (int)(humidity->valuedouble));
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
  const int winWidth = 600;
  const int winHeight = 250;
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

  InitWindow(winWidth, winHeight, "Weather App");
  
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
  
  const char *fontPath = TextFormat("%sassets/font/PressStart2P-Regular.ttf", basePath);
  const int fontSize = 35;
  Rectangle recSrc = {0.0f, 0.0f, (float)myData.weatherBanner.width, (float)myData.weatherlogo.height};
  Rectangle recDest = {0.0f, 0.0f, winWidth, 100.0f};
  // printf("%d qlrj2olthi23thoyn2y",myData.weatherBanner.width);
  Font customFont = LoadFontEx(fontPath, fontSize, NULL, 0);

  // Create refresh button
  Button refreshButton = {0};
  refreshButton.bounds = (Rectangle){winWidth - 110, winHeight - 40, 100, 30};
  refreshButton.isHovered = false;

  // RenderTexture2D miniWIN = LoadRenderTexture(600, 120);

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    // Update button hover state
    Vector2 mousePos = GetMousePosition();
    refreshButton.isHovered = CheckCollisionPointRec(mousePos, refreshButton.bounds);
    
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
        // Update rectangle source dimensions
        recSrc = (Rectangle){0.0f, 0.0f, (float)myData.weatherBanner.width, (float)myData.weatherlogo.height};
      }
    }

    // BeginTextureMode(miniWIN);
    // ClearBackground(BLACK);
    // EndTextureMode();

    BeginDrawing();
    ClearBackground(MAIN_GRAY);
    
    // Display content based on application state
    if (appState == STATE_SUCCESS) {
      // Draw normal weather display
      DrawTextEx(customFont, myData.weatherName, (Vector2){100, 135}, (float)fontSize, 0.0f, WHITE);
      DrawText(TextFormat("%s, %s", myData.city, myData.country), 100, 170, 20, GRAY);
      DrawTextEx(customFont, myData.temperature, (Vector2){448, 135}, (float)fontSize, 0.0f, WHITE);
      DrawText(TextFormat("%s", myData.humidity), 460, 170, 20, GRAY);
      DrawLine(430, 130, 430, 190, GRAY);

      DrawTexturePro(myData.weatherBanner, recSrc, recDest, (Vector2){0, 0}, 0.20f, WHITE);
      DrawTextureEx(myData.weatherlogo, (Vector2){-30, 130}, 0.0f, 0.25f, WHITE);
    } else {
      // Display error message
      const char *errorTitle = "Error";
      Color errorColor = RED;
      
      switch(appState) {
        case STATE_LOADING:
          errorTitle = "Loading...";
          errorColor = YELLOW;
          break;
        case STATE_ERROR_API_KEY:
          errorTitle = "API Key Error";
          break;
        case STATE_ERROR_NETWORK:
          errorTitle = "Network Error";
          break;
        case STATE_ERROR_INVALID_CITY:
          errorTitle = "Invalid City";
          break;
        case STATE_ERROR_JSON_PARSE:
          errorTitle = "Data Error";
          break;
        default:
          errorTitle = "Unknown Error";
      }
      
      // Draw error title
      int titleWidth = MeasureText(errorTitle, 30);
      DrawText(errorTitle, (winWidth - titleWidth) / 2, 60, 30, errorColor);
      
      // Draw error message (multi-line support)
      DrawText(myData.errorMessage, 50, 110, 20, WHITE);
      
      // Draw usage hint
      DrawText("Usage: ./weather_app [city_name]", 50, 200, 16, GRAY);
    }
    
    // Draw refresh button (always visible)
    Color buttonColor = refreshButton.isHovered ? DARKGREEN : GREEN;
    if (!API_KEY) buttonColor = GRAY;  // Disabled if no API key
    
    DrawRectangleRec(refreshButton.bounds, buttonColor);
    DrawRectangleLinesEx(refreshButton.bounds, 2, refreshButton.isHovered ? WHITE : LIGHTGRAY);
    
    const char *buttonText = "Refresh";
    int textWidth = MeasureText(buttonText, 16);
    DrawText(buttonText, 
             refreshButton.bounds.x + (refreshButton.bounds.width - textWidth) / 2,
             refreshButton.bounds.y + 7, 
             16, WHITE);

    // DrawTextureRec(miniWIN.texture, (Rectangle){0, 0, miniWIN.texture.width, miniWIN.texture.height}, (Vector2){0, 0}, WHITE);

    EndDrawing();
  }

  // Cleanup
  UnloadFont(customFont);
  if (myData.weatherBanner.id != 0) UnloadTexture(myData.weatherBanner);
  if (myData.weatherlogo.id != 0) UnloadTexture(myData.weatherlogo);
  // UnloadRenderTexture(miniWIN);
  CloseWindow();

  return 0;
}
