# NON_BLOCKING_DELAY — délai non bloquant pour Arduino & FreeRTOS

Bibliothèque **header-only** (un seul fichier `Delay.h`, pas de `.cpp`) définissant une fonction `NON_BLOCKING_DELAY(unsigned long)` qui remplace `delay()` sans bloquer le système :

- **FreeRTOS / ESP32** : `vTaskDelay()` — la tâche se met en sommeil, les tâches de plus basse priorité (WiFi, idle, watchdog…) continuent.
- **Arduino classique (ESP8266, AVR, PIC…)** : busy-wait `millis()` **overflow-safe** avec `yield()` pour laisser tourner la pile (WiFi ESP8266…).

## Fonctionnement

```cpp
NON_BLOCKING_DELAY(1000);   // attend ~1 s sans bloquer le reste du système
```

| Plateforme | Branche utilisée | Mécanisme |
|---|---|---|
| ESP32 (Arduino ou ESP-IDF) | **FreeRTOS** | `vTaskDelay()` |
| FreeRTOS nu (STM32, etc.) | **FreeRTOS** | `vTaskDelay()` |
| ESP8266 (Arduino) | **Busy-wait** | `millis()` + `yield()` |
| AVR / Nano / Uno | **Busy-wait** | `millis()` (pas de `yield()` utile) |
| PIC8 (HAL_Arduino_PIC8) | **Busy-wait** | `millis()` seule |

La détection est **automatique** : branche FreeRTOS si l'API est visible (`configTICK_RATE_HZ` + `portTICK_PERIOD_MS`) ou si `ESP_PLATFORM` (ESP32). Forçable avec `#define NON_BLOCKING_DELAY_USE_RTOS` avant l'include.

## Points d'attention (à ne pas « simplifier »)

- **Dépassement de `millis()`** : sur une cible 32 bits, le compteur repart de zéro après ~49,7 jours (`2^32` ms). La condition `while (now - t0 <= duree)` est **sûre** : la soustraction est calculée modulo `2^32` et reste correcte même en traversant la remise à zéro. La version naïve `while (now <= t0 + duree)` **déborde** : `t0 + duree` s'enroule sur une petite valeur → `now` est presque toujours supérieur à la borne et le délai **s'arrête aussitôt (durée ≈ 0)** au lieu d'attendre la durée demandée. La garde reste valide tant que la durée demandée est très inférieure à `2^32` ms (toujours le cas en pratique).
- **Conversion ms→tick** : arrondie **au supérieur** et **minimum 1 tick** — ex. 5 ms avec `portTICK_PERIOD_MS = 10` attend 10 ms. La conversion se fait par division (`duree / portTICK_PERIOD_MS`) + reste, **sans multiplication** : aucune saturation pour les longues durées (contrairement à `duree * configTICK_RATE_HZ / 1000`).

## Paramètres de configuration (macros, redéfinissables avant l'include)

| Macro | Défaut | Rôle |
|---|---|---|
| `NON_BLOCKING_DELAY_USE_RTOS` | détection auto | Forcer la branche FreeRTOS |
| `NON_BLOCKING_DELAY_NOW()` | `millis()` (si `ARDUINO`) | Horloge à fournir hors Arduino |
| `NON_BLOCKING_DELAY_YIELD()` | `yield()` sur ESP8266/ESP32, sinon rien | Cède la main |

## Installation

### PlatformIO

```ini
lib_deps =
    https://github.com/Fo170/NON_BLOCKING_DELAY.git@^1.0.0
```

### Arduino IDE

Copier `Delay.h` dans le dossier du sketch et `#include "Delay.h"`.

## Exemple

```cpp
#include "Delay.h"

unsigned long derniereMesure = 0;

void loop() {
  if (millis() - derniereMesure >= 5000) {   // travail toutes les 5 s
    derniereMesure = millis();
    // ... mesure, envoi réseau, etc.
  }

  NON_BLOCKING_DELAY(200);   // attend 200 ms sans bloquer WiFi/idle
}
```

> Dans une tâche FreeRTOS, `NON_BLOCKING_DELAY` laisse la main aux autres tâches. En Arduino simple, le busy-wait ne se termine qu'après la durée demandée (pas de multitâche) — mais `yield()` maintient WiFi actif sur ESP8266.

## Licence

GPL-3.0-only — voir `LICENSE`. Auteur : Olivier FOURNET (org [Fo170](https://github.com/Fo170)).