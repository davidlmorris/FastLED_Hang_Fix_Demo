
#ifndef ESP32
#error "This code requires an ESP32"
#endif
#include "FastLED_Hang_Fix_Demo.h"
#include "debug_conditionals.h"


#include "displayFastLedCommon.h" // here is where we call FastLED.h


// FastLED controller stuff
bool bFastLedReady = false;
bool bFastLedInitialised = false;

CLEDController* controllers[NUM_FASTLED_CONTROLLERS] = {NULL};


static volatile bool NotShowing = true;
uint8_t FastLedCommonDitherMode = 0;

// This is global to block updates when needed
volatile bool bFastLEDShowWait = false;
TaskHandle_t FastLedShowHandlerTaskSignal = NULL;
float lowestFrameRateInUse = 400;
uint16_t frameRateInMilliseconds = 400;

 void IRAM_ATTR fastLedShowHandlerTask(void* param);
 void setupFastLedShowHandlerTask(void);

#define WRITE_FASTLED_SHOW_PRIORITY 1
#define WRITE_FASTLED_SHOW_CORE 1
// Note increasing WRITE_FASTLED_SHOW_PRIORITY beyond 1 seems to add stutter
// to the system and reduce overall throughput.  0 seems to hang it!
// And FastLED HAS to operate on Core 1.
// If we use core 0 which on the surface seems sensible 
// weird things (like flickering and partial writes) seem to happen, and running slow and hangs, so
// it must be core 1 the same as the mainloop.



#define STRAND_SIZE1 256
#define STRAND_SIZE2 256
#define STRAND_SIZE3 470
#define STRAND_SIZE4 470

struct CRGB ledStrand1[STRAND_SIZE1] = { 0};
struct CRGB ledStrand2[STRAND_SIZE2] = { 0};

uint8_t uiBrightness = 255;
// Mixing the variables up to show that the LED strands do not have to occupy contiguous memory.

struct CRGB ledStrand3[STRAND_SIZE3] = { 0};
struct CRGB ledStrand4[STRAND_SIZE4] = { 0};

void clear_all_leds(void)
    {
    for(int i = 0; i < STRAND_SIZE1;i++)
        {
        ledStrand1[i] = CRGB::Black;
        }
    for(int i = 0; i < STRAND_SIZE2;i++)
        {
        ledStrand2[i] = CRGB::Black;
        }
    for(int i = 0; i < STRAND_SIZE3;i++)
        {
        ledStrand3[i] = CRGB::Black;
        }
    for(int i = 0; i < STRAND_SIZE4;i++)
        {
        ledStrand4[i] = CRGB::Black;
        }
    }

void paint_random_leds(void)
    {
    for(int i = 0; i < STRAND_SIZE1;i++)
        {
        ledStrand1[i].r = random8(255);  
        ledStrand1[i].g = random8(255);  
        ledStrand1[i].b = random8(255);  
        }
    for(int i = 0; i < STRAND_SIZE2;i++)
        {
        ledStrand2[i].r = random8(255);  
        ledStrand2[i].g = random8(255);  
        ledStrand2[i].b = random8(255);  
        }
    for(int i = 0; i < STRAND_SIZE3;i++)
        {
        ledStrand3[i].r = random8(255);  
        ledStrand3[i].g = random8(255);  
        ledStrand3[i].b = random8(255);  
        }
    for(int i = 0; i < STRAND_SIZE4;i++)
        {
        ledStrand4[i].r = random8(255);  
        ledStrand4[i].g = random8(255);  
        ledStrand4[i].b = random8(255);  
        }
    }


void fastLedSetup(void)
    {
    FastLedCommonDitherMode = DISABLE_DITHER;
    // BINARY_DITHER is sometimes annoying a low light levels...  and DISABLE_DITHER is probably good enough in any case!
    // Especially since our frame rate is limited.
    // We set this in the controller when we initialise the stands or matrix.
    FastLED.setBrightness(127);  // Half brightness to start.
    FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);
    // setMaxPowerInVoltsAndMilliamps is probably NOT going 
    // to work well here since we won't have (temporal) dither.

    // Add four clockless based CLEDController instances 2 for the stands 2 for the matrixes. 
    controllers[FASTLED_STRAND_LEFT] = &FastLED.addLeds<LED_CHIPSET_STRAND, LEFT_OUT_LED_STRAND_PIN, COLOR_ORDER_STRAND>(ledStrand1, STRAND_SIZE1);
    controllers[FASTLED_STRAND_RIGHT] = &FastLED.addLeds<LED_CHIPSET_STRAND, RIGHT_OUT_LED_STRAND_PIN, COLOR_ORDER_STRAND>(ledStrand2, STRAND_SIZE2);
    controllers[FASTLED_MATRIX_LEFT] = &FastLED.addLeds<LED_CHIPSET_MATRIX, LEFT_OUT_LED_MATRIX_PIN, COLOR_ORDER_MATRIX>(ledStrand3, STRAND_SIZE3);
    controllers[FASTLED_MATRIX_RIGHT] = &FastLED.addLeds<LED_CHIPSET_MATRIX, RIGHT_OUT_LED_MATRIX_PIN, COLOR_ORDER_MATRIX>(ledStrand4, STRAND_SIZE4);

    controllers[FASTLED_STRAND_LEFT]->setCorrection(TypicalLEDStrip);
    controllers[FASTLED_STRAND_RIGHT]->setCorrection(TypicalLEDStrip);
    controllers[FASTLED_MATRIX_LEFT]->setCorrection(TypicalLEDStrip);
    controllers[FASTLED_MATRIX_RIGHT]->setCorrection(TypicalLEDStrip);

    controllers[FASTLED_STRAND_LEFT]->setDither(FastLedCommonDitherMode);
    controllers[FASTLED_STRAND_RIGHT]->setDither(FastLedCommonDitherMode);
    controllers[FASTLED_MATRIX_LEFT]->setDither(FastLedCommonDitherMode);
    controllers[FASTLED_MATRIX_RIGHT]->setDither(FastLedCommonDitherMode);
    setupFastLedShowHandlerTask();

    fastLedCalcFrameRate(STRAND_SIZE4); // pick the biggest

    }


/// @brief Returns a maximum framerate given the number of LEDS and
/// also stores a lowestFrameRateInUse to determine the maximum frequency
/// that FastLED.Show should be called.
/// @param numberOfLeds Number of LEDs on a single pin or controller.
/// @return max frame Rate for the number of LEDs on that pin.
float fastLedCalcFrameRate(uint16_t numberOfLeds)
    {
  // assume WS2812 or WS2812B at 800 kilobits/sec 
    float frameRate = (800000.0 / 24.0 / (float) numberOfLeds) - (50.0 / 1000000.0);
    if (frameRate < lowestFrameRateInUse)
        {
        lowestFrameRateInUse = frameRate;
        }
    return(frameRate);
    }

/// @brief Used after all FastLED controller have been initialised
/// so we can calculate framerate frequency (and anything else we might need later).
/// @param  
void fastLedPostInit(void)
    {
    frameRateInMilliseconds = (uint16_t) floor(1000.0 / lowestFrameRateInUse);
    uint16_t uiLowestFrameRateInUse = floor(lowestFrameRateInUse);
    FastLED.setMaxRefreshRate(uiLowestFrameRateInUse, true);
    // This will force a delay in FastLED.show.
    // Needs to be set after adding all the LEDs to the controllers.
    //  see https://forum.makerforums.info/t/today-i-learned-fastled-show-will-automatically-wait-delay-if-you-have-set-a-refresh-rate/64631

    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_PRINT("FastLED Lowest frame rate in use is ");
        DEBUG_PRINT(uiLowestFrameRateInUse);
        DEBUG_PRINT(" (");
        DEBUG_PRINT(frameRateInMilliseconds);
        DEBUG_PRINT(" ms per frame)");
        DEBUG_PRINTLN(".");
        DEBUG_DELAY(xTickATinyBit);
        DEBUG_SEMAPHORE_RELEASE;
        }

// http://fastled.io/docs/class_c_fast_l_e_d.html#a1f39e8404db214bbd6a776f52a77d8b1
// cf https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
//     4 x channels  at  800 KHz / 24 bits / LED  - 50us reset time

    // Channel                     Pixels (LEDS)           Frame Rates
    // Matrix Left                 256                     130.2082833
    // Matrix Right                256                     130.2082833
    // Strands Left                270                     123.4567401  (Assuming not using the extra 20 meter strand of 200 LEDS - which it now does)
    // Strands Right               470                     70.92193582
    //     (with 200 extra)
    // Total                       1252                    26.62401816

// Ideally, we won't get below a frame rate of 60hz.
    }





/// @brief Used as a replacement for FastLED.Show() to minimise hangs and crashes
// and to not call FastLED.Show() more often than it can do an update.
void FastLEDshow(void)
    {
    static uint32_t restartCount = 0;
    static uint64_t restartNextUs = esp_timer_get_time() + 1000000;
    // This is an attempt to reduce contention issues with FastLED and
    // the rest of the code for the random rare hangs...  

    if (NotShowing)
        {
   // An attempt to resolve Esp32/FastLED timing issues
   // basically no point in calling this more than the frame rate
   // that the slowest FastLED channel can deliver.
   // I'm not entirely sure this actually has a noticeable effect, but 
   // my very subjective opinion is it slightly reduced some stuttering, that
   // I could only really see in scrolling text.

#if FASTLED_STUTTER_REDUCTION
        EVERY_N_MILLISECONDS(frameRateInMilliseconds)
#endif
            {
            xTaskNotifyGive(FastLedShowHandlerTaskSignal);
            }
        restartCount = 0;
        }
    else
        {
        if (esp_timer_get_time() > restartNextUs)
            {
            // We get here every second if we are blocked because FastLED.Show has jammed...
            // First time is likely to be at 0 seconds as restartNextUs was (hopefully)
            // some time ago.
            restartNextUs = esp_timer_get_time() + 1000000;
            if (restartCount == 1)   /// If we have jammed for 1 second(s).
                {
# if DEBUG_FASTLED_JAM
                DEBUG_START_SEMAPHORE_BLOCK
                    {
                    DEBUG_PRINTLN("");
                    DEBUG_PRINT("FastLED.Show() has jammed after ");
                    DEBUG_PRINT((esp_timer_get_time() / 1000000.0) / 60.0);
                    DEBUG_PRINT(" minutes since boot!");
                    DEBUG_PRINTLN("");
                    DEBUG_DELAY(xTickATinyBit);
    # if DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM
                    DEBUG_PRINTLN("Forcing semaphore reset try.");
    # else                
                    DEBUG_PRINTLN("Waiting another second while gTX_sem times out.");
    # endif                
                    DEBUG_DELAY(xTickATinyBit);
                    DEBUG_PRINTLN("");
                    DEBUG_SEMAPHORE_RELEASE;
                }
                vTaskDelay(pdMS_TO_TICKS(500));
# endif            
# if DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM
                // This triggers FastLED RMT driver to reset its blocking semaphore.
                GiveGTX_sem();
# endif                
                vTaskDelay(pdMS_TO_TICKS(1));
                                // Now we trigger our FastLED task to clear our lock.
                xTaskNotifyGive(FastLedShowHandlerTaskSignal);
                vTaskDelay(pdMS_TO_TICKS(50));
                }

            // If this doesn't work something else is broken so 
            // let's just re-boot after 15 secs.
            if (restartCount > 15)
                {
# if DEBUG_FASTLED_JAM
                DEBUG_START_SEMAPHORE_BLOCK
                    {
                    DEBUG_PRINTLN("");
                    DEBUG_PRINT("FastLED.Show() has jammed after ");
                    DEBUG_PRINT((esp_timer_get_time() / 1000000.0) / 60.0);
                    DEBUG_PRINT(" minutes since boot!");
                    DEBUG_PRINTLN("");
                    DEBUG_DELAY(xTickATinyBit);
                    DEBUG_PRINTLN("");
                    DEBUG_PRINTLN("");
                    DEBUG_PRINTLN("");
                    DEBUG_SEMAPHORE_RELEASE;
                }
                vTaskDelay(pdMS_TO_TICKS(500));
# endif            
                ESP.restart();
                }
            restartCount++;
            }
        }
    }


void setupFastLedShowHandlerTask(void)
    {
    int stackSizeInWords = configMINIMAL_STACK_SIZE + 10000;
    xTaskCreatePinnedToCore(
        fastLedShowHandlerTask,
        "fastLedShowHandlerTask",
        stackSizeInWords,
        NULL,
        WRITE_FASTLED_SHOW_PRIORITY,
        &FastLedShowHandlerTaskSignal,
        WRITE_FASTLED_SHOW_CORE);
    }


/// @brief FastLED.show task.  Trigger with xTaskNotifyGive(FastLedShowHandlerTaskSignal)
/// @param  param unused.
void IRAM_ATTR fastLedShowHandlerTask(void* param)
    {
    DEBUG_START_SEMAPHORE_BLOCK
        {
        DEBUG_PRINT("fastLedShowHandlerTask running on core ");
        DEBUG_PRINT(xPortGetCoreID());
        DEBUG_PRINTLN(".");
        DEBUG_SEMAPHORE_RELEASE;
        }
    vTaskDelay(pdMS_TO_TICKS(100));
    bFastLedInitialised = true;
    bFastLedReady = true;
    while (true)
        {
        // Sleep until the ISR gives us something to do, or for our 
        // timeout period in ticks (which is basically for ever).
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        NotShowing = false;
        yield();
        while (bFastLEDShowWait)
            {
            yield();
            }
        DEBUG_ASSERT(FastLED.size() > 0);
        DEBUG_ASSERT(FastLED.count() == 4);
        FastLED.show(uiBrightness);
        yield();
        NotShowing = true;
        }
    }

