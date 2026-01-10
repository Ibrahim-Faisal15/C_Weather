
#include "raylib.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <wchar.h>
#include <locale.h>

#define MAIN_GRAY ((Color){52, 52, 52, 255})

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

int main()
{
  setlocale(LC_ALL, "");
  const int winWidth = 600;
  const int winHeight = 250;
  const char *basePath = GetApplicationDirectory();
  const char *fullPath_of_WeatherBanner = NULL;
  const char *fullPath_of_WeatherLogo = NULL;

  weatherData myData = {0};
  const char *API_KEY = getenv("OPENWEATHER_API_KEY");
  if (!API_KEY || API_KEY[0] == '\0') {
      printf("Missing API KEY. Set OPENWEATHER_API_KEY\n");
      return 1;
  }
  char url[256] = {0};
  snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/weather?q=Lahore&appid=%s", API_KEY);

  struct Memory chunk ={0};
  chunk.data = malloc(1);
  if (chunk.data==NULL){
      fprintf(stderr, "malloc failed to allocate data for the chunk");
      return 1;
  }
  chunk.size = 0;
  cJSON *json = NULL;

  CURL *curl;
  CURLcode result = curl_global_init(CURL_GLOBAL_ALL);

  if (result != CURLE_OK)
  {
    printf("Initialization of CURL failed.");
    return 1;
  }
  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    result = curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
      printf("Error: %s\n", curl_easy_strerror(result));
    }
    else
    {
      // printf("Data: %s", chunk.data);
      json = cJSON_Parse(chunk.data);
      if (json != NULL)
      {
        // Get the city name
        cJSON *location = cJSON_GetObjectItemCaseSensitive(json, "name");
        if (cJSON_IsString(location))
        {
            if(strlen(location->valuestring) >= sizeof(myData.city)){
                fprintf(stderr, "Buffer overflow detected in location string. location length: %zu, buffer length: %zu\n", 
                        strlen(location->valuestring), sizeof(myData.city));
                return 1;
            } 
                strcpy(myData.city, location->valuestring);
        }
        cJSON *sys = cJSON_GetObjectItemCaseSensitive(json, "sys");
        if(!cJSON_IsObject(sys)){
            fprintf(stderr, "Failed to parse object \"sys\"");
            return 1;
        }
        cJSON *country = cJSON_GetObjectItemCaseSensitive(sys, "country");
        if (cJSON_IsString(country))
        {
            if(strlen(country->valuestring) >= sizeof(myData.country)){
                fprintf(stderr, "Buffer overflow detected in country string. country length: %zu, buffer length: %zu\n", 
                        strlen(country->valuestring), sizeof(myData.country));
                return 1;
            } 
                strcpy(myData.country, country->valuestring);
        }
        // Get what weather it is
        cJSON *weather_obj = cJSON_GetObjectItemCaseSensitive(json, "weather");
        cJSON *firstITEM = cJSON_GetArrayItem(weather_obj, 0);
        cJSON *weather_name = cJSON_GetObjectItemCaseSensitive(firstITEM, "main");
        if (cJSON_IsString(weather_name))
        {
            if(strlen(weather_name->valuestring) >= sizeof(myData.weatherName)){
                fprintf(stderr, "Buffer overflow detected in weather_name string. name length: %zu, buffer length: %zu\n", 
                        strlen(weather_name->valuestring), sizeof(myData.weatherName));
                return 1;
            }
                strcpy(myData.weatherName, weather_name->valuestring);
        }
        // Get the temperature
        cJSON *temperature_obj = cJSON_GetObjectItemCaseSensitive(json, "main");
        cJSON *temperature = cJSON_GetObjectItemCaseSensitive(temperature_obj, "temp");
        if (cJSON_IsNumber(temperature))
        {

          snprintf(myData.temperature, sizeof(myData.temperature), "%dC", (int)(temperature->valuedouble - 273.15));
        }
        // Get the humidity level
        cJSON *humidity = cJSON_GetObjectItemCaseSensitive(temperature_obj, "humidity");
        if (cJSON_IsNumber(humidity))
        {

          snprintf(myData.humidity, sizeof(myData.humidity), "%d%%", (int)(humidity->valuedouble));
        }
        // Get the weather ID
        cJSON *weatherID_obj = cJSON_GetObjectItemCaseSensitive(json, "weather");
        cJSON *weatherID = cJSON_GetObjectItemCaseSensitive(firstITEM, "id");
        if (cJSON_IsNumber(weatherID))
        {
          myData.weatherID = weatherID->valuedouble;
        }

        // Determining weather banner and logo
        if (myData.weatherID >= 200 && myData.weatherID <= 232)
        {
          printf("ThunderStorm");
          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/thunderStorm.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/thunderStorm.png", basePath);
        }
        else if (myData.weatherID >= 300 && myData.weatherID <= 321)
        {
          printf("Drizzle");

          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/rain.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/rain.png", basePath);
        }
        else if (myData.weatherID >= 500 && myData.weatherID <= 531)
        {
          printf("Rain");
          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/rain.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/rain.png", basePath);
        }
        else if (myData.weatherID >= 600 && myData.weatherID <= 622)
        {
          printf("Snow");
          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/snow.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/snow.png", basePath);
        }
        else if (myData.weatherID >= 701 && myData.weatherID <= 781)
        {
          printf("Atmosphere");

          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/fog.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/fog.png", basePath);
        }
        else if (myData.weatherID == 800)
        {
          printf("clear");

          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/clear.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/sunny.png", basePath);
        }
        else if (myData.weatherID > 800 && myData.weatherID <= 804)
        {
          printf("clouds");

          fullPath_of_WeatherBanner = TextFormat("%sassets/weatherBanner/clouds.jpg", basePath);
          fullPath_of_WeatherLogo = TextFormat("%sassets/weatherLogos/clouds.png", basePath);
        }

        // printf("City: %s\nWeather: %s\nTemperature: %d\nHumidity: %d\nLogo: %s\nBanner: %s\n",
        //        myData.city, myData.weatherName, myData.temperature, myData.humidity,
        //        myData.weatherlogo, myData.weatherBanner);
      }
    }
  }

  curl_global_cleanup();

  InitWindow(winWidth, winHeight, "Weather App");
  if(fullPath_of_WeatherBanner ==NULL){
      fprintf(stderr, "Failed to load path for weather banner");
  }
  myData.weatherBanner = LoadTexture(fullPath_of_WeatherBanner);
  if(myData.weatherBanner.id==0){
      fprintf(stderr, "unable to load weather banner texture");
      return 1;
  }

  if(fullPath_of_WeatherLogo==NULL){
      fprintf(stderr, "Failed to load path for weather logo");
  }
  myData.weatherlogo = LoadTexture(fullPath_of_WeatherLogo);
  if(myData.weatherlogo.id==0){
      fprintf(stderr, "unable to load weather logo texture");
      return 1;
  }
  const char *fontPath = TextFormat("%sassets/font/PressStart2P-Regular.ttf", basePath);
  const int fontSize = 35;
  Rectangle recSrc = {0.0f, 0.0f, (float)myData.weatherBanner.width, (float)myData.weatherlogo.height};
  Rectangle recDest = {0.0f, 0.0f, winWidth, 100.0f};
  // printf("%d qlrj2olthi23thoyn2y",myData.weatherBanner.width);
  Font customFont = LoadFontEx(fontPath, fontSize, NULL, 0);

  // RenderTexture2D miniWIN = LoadRenderTexture(600, 120);

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {

    // BeginTextureMode(miniWIN);
    // ClearBackground(BLACK);
    // EndTextureMode();

    BeginDrawing();
    // DrawText(TextFormat("%s", myData.weatherName), 100, 135, 50, WHITE);
    DrawTextEx(customFont, myData.weatherName, (Vector2){100, 135}, (float)fontSize, 0.0f, WHITE);
    DrawText(TextFormat("%s, %s", myData.city, myData.country), 100, 170, 20, GRAY);
    // DrawText(TextFormat("%d", myData.temperature), 450, 135, 50, WHITE);
    DrawTextEx(customFont, myData.temperature, (Vector2){448, 135}, (float)fontSize, 0.0f, WHITE);
    DrawText(TextFormat("%s", myData.humidity), 460, 170, 20, GRAY);
    DrawLine(430, 130, 430, 190, GRAY);
    // DrawText(TextFormat("%s", myData.city), 100, 170, 20, GRAY);

    DrawTexturePro(myData.weatherBanner, recSrc, recDest, (Vector2){0, 0}, 0.20f, WHITE);
    DrawTextureEx(myData.weatherlogo, (Vector2){-30, 130}, 0.0f, 0.25f, WHITE);

    ClearBackground(MAIN_GRAY);
    // DrawTextureRec(miniWIN.texture, (Rectangle){0, 0, miniWIN.texture.width, miniWIN.texture.height}, (Vector2){0, 0}, WHITE);

    EndDrawing();
  }

  cJSON_Delete(json);
  free(chunk.data);
  curl_easy_cleanup(curl);
  UnloadFont(customFont);
  UnloadTexture(myData.weatherBanner);
  UnloadTexture(myData.weatherlogo);
  // UnloadRenderTexture(miniWIN);
  CloseWindow();

  return 0;
}
