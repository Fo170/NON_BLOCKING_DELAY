//====================================================================
// NON_BLOCKING_DELAY
// Délai non bloquant, compatible : ESP8266 / ESP32 / AVR / PIC / …
// En environnement Arduino (busy-wait + yield) ET FreeRTOS (vTaskDelay).
//
// Auteur : Olivier FOURNET — org Fo170
// Licence : GPL-3.0-only
// https://github.com/Fo170/NON_BLOCKING_DELAY
//
// --- Principe ---
// * Branche FreeRTOS : vTaskDelay() — la tâche se met en sommeil ; les
//   tâches de plus basse priorité (WiFi, watchdog, idle…) continuent.
// * Branche Arduino / libre : busy-wait `while (now() - t0 <= duree)`,
//   condition overflow-safe (jamais de saturation de millis()), avec
//   yield() pour laisser tourner la pile (ex. WiFi ESP8266).
//
// --- Détection automatique ---
// La branche RTOS est choisie si l'API FreeRTOS est visible
// (configTICK_RATE_HZ + portTICK_PERIOD_MS) OU si plateforme ESP32
// (ESP_PLATFORM), qui embarque toujours FreeRTOS (Arduino ou ESP-IDF).
// Sans cela, et si ARDUINO est défini → branche busy-wait.
// Pour forcer une branche, définir NON_BLOCKING_DELAY_USE_RTOS avant
// l'include.
//
// --- Macros de configuration (optionnelles) ---
// NON_BLOCKING_DELAY_USE_RTOS : force la branche FreeRTOS.
// NON_BLOCKING_DELAY_NOW()    : « horloge » — défaut millis() sur Arduino,
//                               à fournir hors Arduino (autre RTOS, bare metal).
// NON_BLOCKING_DELAY_YIELD()  : cède la main — défaut yield() sur ESP8266/ESP32,
//                               rien ailleurs (AVR, PIC, …).
//====================================================================
#ifndef NON_BLOCKING_DELAY_H
#define NON_BLOCKING_DELAY_H


// Version de la librairie (source UNIQUE — agrégée par liste_lib.h)
#define __NON_BLOCKING_DELAY_H_VERSION__ "1.0.1"
//----- Détection environnement -------------------------------------
#if defined(configTICK_RATE_HZ) && defined(portTICK_PERIOD_MS)
  #define NON_BLOCKING_DELAY_USE_RTOS 1
#elif defined(ESP_PLATFORM)
  #define NON_BLOCKING_DELAY_USE_RTOS 1
#endif

//----- Configuration utilisateur -----------------------------------
#ifndef NON_BLOCKING_DELAY_NOW
  #if defined(ARDUINO)
    #define NON_BLOCKING_DELAY_NOW()  millis()
  #endif
#endif

#ifndef NON_BLOCKING_DELAY_YIELD
  #if defined(ESP8266) || defined(ESP32) || defined(ESP_PLATFORM)
    #define NON_BLOCKING_DELAY_YIELD()  yield()  // alimente WiFi / boucle ESP
  #else
    #define NON_BLOCKING_DELAY_YIELD()  ((void)0)
  #endif
#endif

//===== Branche FreeRTOS ============================================
#ifdef NON_BLOCKING_DELAY_USE_RTOS
inline void NON_BLOCKING_DELAY(unsigned long duree)
{
  TickType_t ticks = (TickType_t)(duree / portTICK_PERIOD_MS);
  if (duree % portTICK_PERIOD_MS)  ++ticks;  // arrondi supérieur
  if (!ticks)  ticks = 1;                    // vTaskDelay(0) = simple yield
  vTaskDelay(ticks);                         // la tâche dort → non bloquant
}

//===== Branche Arduino / libre (busy-wait + yield) =================
#else
  #if defined(NON_BLOCKING_DELAY_NOW)
inline void NON_BLOCKING_DELAY(unsigned long duree)
{
  if (duree < 1) duree = 1;                       // min. une période d'horloge
  unsigned long t0 = NON_BLOCKING_DELAY_NOW();    // instant de départ
  while (NON_BLOCKING_DELAY_NOW() - t0 <= duree)  // overflow-safe
  {
    NON_BLOCKING_DELAY_YIELD();                   // laisse tourner la pile
  }
}
  #else
    #error "NON_BLOCKING_DELAY : environnement non reconnu. Inclure Arduino.h ou definir NON_BLOCKING_DELAY_USE_RTOS / NON_BLOCKING_DELAY_NOW()."
  #endif
#endif

#endif // NON_BLOCKING_DELAY_H