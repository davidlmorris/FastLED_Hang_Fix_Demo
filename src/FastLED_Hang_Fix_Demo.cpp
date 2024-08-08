#include <Arduino.h>
#include "debug_conditionals.h"
#include "displayFastLedCommon.h"
#include <freertos/portmacro.h>
#include "FastLED_Hang_Fix_Demo.h"

/// NO NEED TO HOOK THIS UP
/// Just run it on an isolated Esp32.


#ifndef ESP32
# error "This code requires an ESP32"
#endif

// This pathological timer interrupt borrowed (and then so heavily modified you wouldn't know it) from https://github.com/SensorsIot/ESP32-Interrupts-deepsleep/blob/master/Frequency_Counter_with_Timer_Interrupt/Frequency_Counter_with_Timer_Interrupt.ino
// because I need some interrupts to do something...

volatile unsigned long count = 0;
volatile unsigned long frequency;
volatile bool outVal = true;
hw_timer_t* timer = NULL;


portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
    {
    portENTER_CRITICAL_ISR(&timerMux);
    digitalWrite(DIO_TM1637_DIGIT_DISPLAY, outVal);
    outVal = !outVal;
    frequency++;
    digitalWrite(DIO_TM1637_DIGIT_DISPLAY, outVal);
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
        DEBUG_PROGANNOUNCE("FastLED_Hang_Fix_Demo " VERSION_FASTLED_HANG_FIX_DEMO, "'" __FILE__ "'"  " Built: " __DATE__ " " __TIME__ ".");
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


// Some pathological interrupt stuff to run in the background
    pinMode(DIO_TM1637_DIGIT_DISPLAY, OUTPUT);
    timer = timerBegin(0, 91, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 5, true);
    timerAlarmEnable(timer);
// end of pathological interrupt stuff.

    fastLedPostInit();
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
    count = 0;
    frequency = count;
    }


#define MINUTES_BETWEEN_REPORTS 1

void loop(void)
    {
#if DEBUG_ON    
    static bool bReport = true;
    static unsigned long loopTime = 0;
#endif    

    vTaskDelay(xTickATinyBit);
    paint_random_leds(); // Add some random data to the LEDs
    vTaskDelay(pdMS_TO_TICKS(1));
    FastLEDshow(); // Now show the LEDs

#if DEBUG_ON    
    loopTime++;
    if (bReport)
        {
        DEBUG_START_SEMAPHORE_BLOCK
            {
            debugDisplaySeconds("Running continuously for ", 
                                uint32_t (esp_timer_get_time() / 1000000.0));
            DEBUG_PRINT("after boot. Speed ");
            DEBUG_PRINT((float) (loopTime / (MINUTES_BETWEEN_REPORTS * 60.0)),2);
            DEBUG_PRINT(" loops per sec (int = ");
            DEBUG_PRINT(frequency/(MINUTES_BETWEEN_REPORTS * 60.0));
            DEBUG_PRINTLN("/sec).");
            loopTime = 0;
            frequency = 0;
            DEBUG_DELAY(xTickATinyBit);
            DEBUG_SEMAPHORE_RELEASE;
            }
        bReport = false;
        }
    EVERY_N_MINUTES(MINUTES_BETWEEN_REPORTS)
        {
        bReport = true;
        }
#endif
    vTaskDelay(xTickATinyBit);
    taskYIELD();
    }

