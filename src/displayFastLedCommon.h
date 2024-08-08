#ifndef _DISPLAY_FAST_LED_COMMON_H_
#define _DISPLAY_FAST_LED_COMMON_H_

#include <Arduino.h>
#include "debug_conditionals.h"





// THIS was ONLY USED for testing and relied on the following code
// being added to the FastLED lib in FastLED\src\platforms\esp\32\clockless_rmt_esp32.cpp
// void IRAM_ATTR GiveGTX_sem()
//     {
//     if (gTX_sem != NULL)
//         {
//         xSemaphoreGive(gTX_sem);
//         }
//     }
// Note also that FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM is used in 
// FastLED\src\platforms\esp\32\clockless_rmt_esp32.cpp
// to replace:         xSemaphoreTake(gTX_sem, portMAX_DELAY);
// with:               xSemaphoreTake(gTX_sem, FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM);
#define DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM true
#if DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM == false
# define FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM  (2000/portTICK_PERIOD_MS)
#endif


// Default settings for reference: (Things I've seen people try to address Esp32 RMT driver issues).
// #define FASTLED_RMT_MAX_CHANNELS 8
// #define FASTLED_RMT_BUILTIN_DRIVER false
// #define FASTLED_ESP32_FLASH_LOCK
// #define FASTLED_RMT_SERIAL_DEBUG
// #define FASTLED_ALLOW_INTERRUPTS 1
// #define INTERRUPT_THRESHOLD 0
// #define FASTLED_INTERRUPT_RETRY_COUNT 2
// #define FASTLED_FORCE_SOFTWARE_SPI
// end of default reference

// Likely Optimum settings for this code
//#define FASTLED_RMT_MAX_CHANNELS 4      // Only four outputs so would 4 be better the default 8?
// #define FASTLED_RMT_BUILTIN_DRIVER false
// #define FASTLED_RMT_SERIAL_DEBUG
//#define FASTLED_ESP32_FLASH_LOCK
//#define FASTLED_ALLOW_INTERRUPTS 1
//#define INTERRUPT_THRESHOLD 0
//#define FASTLED_INTERRUPT_RETRY_COUNT 2 // default is effectively 2
//#define FASTLED_FORCE_SOFTWARE_SPI      //The default is undefined as shown.
// #define FASTLED_RMT_SERIAL_DEBUG  1

// But see <FastLED_NeoMatrix.h> if FastLED_NeoMatrix is updated.
// and delete the settings in <FastLED_NeoMatrix.h>.


// #define FASTLED_ESP32_I2S 1  //  FastLED experiment which proved to be slower 
                                // ~1900 cycles v ~2700 for one example.


// Attemps to force jam more often (that didn't actually work!!!)
// #define FASTLED_ALLOW_INTERRUPTS 1
// #define INTERRUPT_THRESHOLD 4
// #define FASTLED_INTERRUPT_RETRY_COUNT 1 // default is effectively 2
// #define FASTLED_RMT_MAX_CHANNELS 1          // Only four outputs so would 4 be better?
// #define FASTLED_FORCE_SOFTWARE_SPI          // The default is undefined as shown.
// #define FASTLED_RMT_BUILTIN_DRIVER 1        // This would slow things up a tiny tiny bit, but I can't tell.
// #define FASTLED_ESP32_FLASH_LOCK  1
// #define FASTLED_RMT_SERIAL_DEBUG  1

// https://github.com/FastLED/FastLED/issues/1438

// Current debug settings
//#define FASTLED_RMT_BUILTIN_DRIVER 1
//#define FASTLED_RMT_MAX_CHANNELS 4

// #define DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM false
//  Note that DELAY_FOR_GTX_SEM must be defined before fastled.h, which in our case
// is defined in #include <FastLED_NeoMatrix.h> which can be found in "displayFastLedCommon.h".


// #define DELAY_FOR_GTX_SEM  (2000/portTICK_PERIOD_MS)


// Note that <FastLED_NeoMatrix.h> repeats or changes most of the settings above.  
// Might be useful to look there first.  Note that I have deleted setting there
// ... but be careful of updates.


//#define FASTLED_ALLOW_INTERRUPTS 1
//#define FASTLED_INTERRUPT_RETRY_COUNT 1
//#define INTERRUPT_THRESHOLD 1
//#define FASTLED_ESP32_I2S true
//#define FASTLED_ESP32_FLASH_LOCK 1
//#define FASTLED_RMT_MAX_CHANNELS 1

// cf https://www.reddit.com/r/FastLED/comments/f1cmjo/another_small_push_to_the_esp32_support/
/*(2) An option to lock out SPI flash access (e.g., using SPIFFS) during a call to show().
SPI flash access on the ESP32 requires a lockdown of the memory bus. To enforce this requirement,
the ESP-IDF system software disables all tasks on *both* cores until flash operations complete.
If this lockout happens during a call to show(), the timing gets off, and the LEDs look flashy.
I added a switch that causes the driver to acquire a lock on the flash system, effectively
delaying the flash operations until show() is complete. You can enable it by adding the
following line *before* you include FastLED.h:  #define FASTLED_ESP32_FLASH_LOCK 1
*/

// #define FASTLED_INTERRUPT_RETRY_COUNT 1
//#define FASTLED_ALLOW_INTERRUPTS 0
//#define NO_DITHERING 1
// #define FASTLED_RMT_MAX_CHANNELS 4
// #define FASTLED_INTERRUPT_RETRY_COUNT 4
//#define FASTLED_ESP32_FLASH_LOCK 1

//#define FASTLED_RMT_BUILTIN_DRIVER 1
//#define FASTLED_RMT_SERIAL_DEBUG 1
#define FASTLED_INTERNAL
#include <FastLED.h> // here is where we call FastLED.h

// Universal defines
#define LEFT_CHANNEL 0   // Left channel name
#define RIGHT_CHANNEL 1  // Right channel name
#define STEREO_DISPLAY true                     // If true, L&R channels are independent. 
                                                // If false, both L&R outputs display same 
                                                // data from L audio channel.
#define FASTLED_STUTTER_REDUCTION   true

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)


// FastLED defines
#define COLOR_ORDER_MATRIX GRB   // If colours look wrong, play with this
#define COLOR_ORDER_STRAND GRB   // If colours look wrong, play with this
#define LED_CHIPSET_MATRIX          WS2812B  // LED type  WS2812B
#define LED_CHIPSET_STRAND          WS2812B  // WS2812? or WS2812B (or am I using both?) // LED string type [WS2812B]

// This is define in the main app!  NOT HERE!
// #define MAX_MILLIAMPS 500  // Careful with the amount of power here if running off USB port
#define LED_VOLTS 5  // Usually 5 or 12


// Important!!!!
// Careful with the amount of power (especially if running off USB port - make it 100)
// HOWEVER, note that since are not using dithering because
// our updates are so slow this setting may not have the desired
// affect.  (i.e. Don't use USB ports on a PC to power LEDs!)
// (In my case I am using a 5V 10 amp power supply for the LED strips)
#define MAX_MILLIAMPS 10000                  
// #define MAX_MILLIAMPS 500                     



// FastLED Matrix stuff
#define FASTLED_STRAND_LEFT 0
#define FASTLED_STRAND_RIGHT 1
#define FASTLED_MATRIX_LEFT 2
#define FASTLED_MATRIX_RIGHT 3

#define NUM_FASTLED_CONTROLLERS 4

extern CLEDController* controllers[NUM_FASTLED_CONTROLLERS];

extern bool bFastLedReady;
extern bool bFastLedInitialised;
extern uint8_t FastLedCommonDitherMode;
extern volatile bool bFastLEDShowWait;

extern void fastLedSetup(void);
extern float fastLedCalcFrameRate(uint16_t numberOfLeds);
extern void fastLedPostInit(void);
extern void FastLEDshow(void);


extern void clear_all_leds(void);
extern void paint_random_leds(void);

#endif /* _DISPLAY_FAST_LED_COMMON_H_ */
