#include <Arduino.h>
#include <WiFi.h>
#include "debug_conditionals.h"
#include "displayFastLedCommon.h"
#include <freertos/portmacro.h>
#include "FastLED_Hang_Fix_Demo.h"

/// NO NEED TO HOOK THIS UP
/// Just run it on an isolated Esp32.

#ifndef ESP32
# error "This code requires an ESP32"
#endif

// this bit from https://github.com/SensorsIot/ESP32-Interrupts-deepsleep/blob/master/Frequency_Counter_with_Timer_Interrupt/Frequency_Counter_with_Timer_Interrupt.ino
// because I need some interrupts to do something...

volatile int count = 0;
volatile int frequency;

int tst;

hw_timer_t* timer = NULL;


portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
    {
    portENTER_CRITICAL_ISR(&timerMux);
    frequency = count;
    count++;
    portEXIT_CRITICAL_ISR(&timerMux);
    }

void setup(void)
    {
    DEBUG_INITIALISE(false, 115200, SERIAL_8N1);
    // WiFi.mode( WIFI_OFF );
    // btStop();
    DEBUG_INIT_SEMAPHORE;
    vTaskDelay(xTickFullSec);
    DEBUG_RESETCOLOUR();
    DEBUG_DELAY(xTickFullSec);
    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_PRINTLN("");
        DEBUG_DELAY(xTickFullSec);
        DEBUG_PRINTLN("");
        DEBUG_DELAY(xTickFullSec);
        DEBUG_PROGANNOUNCE("FastLED_Hang_Fix_Demo", "'" __FILE__ "'"  " Built: " __DATE__ " " __TIME__ ".");
        DEBUG_PRINT("Starting comms ");
        DEBUG_PRINT(esp_timer_get_time() / 1000000.0);
        DEBUG_PRINT(" seconds after boot");
        DEBUG_PRINTLN(".");
        DEBUG_DELAY(xTickATinyBit);
        DEBUG_PRINT("Main App running on core ");
        DEBUG_PRINT(xPortGetCoreID());
        DEBUG_PRINT(" at ");
        DEBUG_PRINT(getCpuFrequencyMhz());
        DEBUG_PRINT(" MHz");
        DEBUG_PRINTLN(".");
        DEBUG_DELAY(xTickATinyBit);
        DEBUG_SEMAPHORE_RELEASE;
        }
    DEBUG_DELAY(xTickATinyBit);

    fastLedSetup();
    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_PRINT("FastLED controllers[0]->size() = ");
        DEBUG_PRINT(controllers[0]->size());
        DEBUG_PRINTLN(".");
        DEBUG_PRINT("FastLED controllers[1]->size() = ");
        DEBUG_PRINT(controllers[1]->size());
        DEBUG_PRINTLN(".");
        DEBUG_PRINT("FastLED controllers[2]->size() = ");
        DEBUG_PRINT(controllers[2]->size());
        DEBUG_PRINTLN(".");
        DEBUG_PRINT("FastLED controllers[3]->size() = ");
        DEBUG_PRINT(controllers[3]->size());
        DEBUG_PRINTLN(".");
        DEBUG_PRINT("FastLED.count() = ");
        DEBUG_PRINT(FastLED.count());
        DEBUG_PRINTLN(".");
        DEBUG_SEMAPHORE_RELEASE;
        }
    DEBUG_DELAY(xTickATinyBit);
    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_ASSERT(controllers[0]->size() > 0);
        DEBUG_ASSERT(FastLED.count() == 4);
        DEBUG_SEMAPHORE_RELEASE;
        }

    fastLedPostInit();


// Some interrupt stuff to run in the background
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 5, true);
    timerAlarmEnable(timer);

    clear_all_leds();
    vTaskDelay(pdMS_TO_TICKS(1));
    FastLEDshow();
    DEBUG_DELAY(xTickATinyBit);

    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_PRINT("Initialisation Complete ");
        DEBUG_PRINT(esp_timer_get_time() / 1000000.0);
        DEBUG_PRINT(" seconds after boot");
        DEBUG_PRINTLN(".");
        DEBUG_DELAY(xTickATinyBit);
        DEBUG_SEMAPHORE_RELEASE;
        }
    }




void loop(void)
    {
#if DEBUG_ON        
    static uint8_t HourCount = 0;

    EVERY_N_MINUTES(15)
        {
        HourCount++;
        DEBUG_START_SEMAPHORE_BLOCK
            {
            DEBUG_PRINT("Running continuously for ");
            if (HourCount >= 4)
                {
                HourCount = 0;
                DEBUG_PRINT((esp_timer_get_time() / 1000000.0) / 3600.0);
                DEBUG_PRINT(" hour(s) after boot");
                }
            else
                {
                DEBUG_PRINT((esp_timer_get_time() / 1000000.0) / 60.0);
                DEBUG_PRINT(" minutes(s) after boot");
                }
            DEBUG_PRINTLN(".");
            DEBUG_DELAY(xTickATinyBit);
            DEBUG_SEMAPHORE_RELEASE;
            }
        }
#endif
    paint_random_leds();
    vTaskDelay(pdMS_TO_TICKS(1));
  // Now show the LEDs
    FastLEDshow();

#if DEBUG_ON && DEBUG_UPDATE_FREQUENCY
    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_SPEEDTEST(60);
        DEBUG_SEMAPHORE_RELEASE;
        }
#endif
    taskYIELD();
    }

