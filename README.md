# FastLED_Hang_Fix_Demo

#### Quick note.. this issue below has been fixed with by the option of adding `#define FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM (2000/portTICK_PERIOD_MS)` (just) before you include "FastLED.h" from FastLED version [3.7.1](https://github.com/FastLED/FastLED/releases/tag/3.7.1). See below for more.

 Demonstration and fix for FastLED.show() random hang.  FastLED is a library to support the display of Addressable LED which can run on a number of MCUs including the Esp32.

The Esp32 is a fast MCU, but struggles (probably because of FreeRTOS) to manage interrupts running over about 200 kilohertz.  See this [YouTube clip](<https://www.youtube.com/watch?v=CJhWlfkf-5M>) at about 8:30 for a compelling demonstration.

FastLED is reported (notably [here](https://github.com/FastLED/FastLED/issues/1438)) to hang after a random amount of time usually hours after the Esp32 has started.

## Why

The FastLed RMT driver makes extensive use of interrupts.  The comments in the FastLED driver hint at least one per 32 bits of data out.  The driver does all the sending in one hit so that even with multiple controllers (as we have in this demo) it is only when the last one is called does it finally does the work.  I am uncertain what might happen if only one controller that is not the last one is called.  (Something for a future experiment?). On a send, it waits 50 microseconds to signal a start, then starts the channels for each controller.  It then sets a semaphore which is then released when the final channel on the final controller is complete.  It appears that rarely something goes wrong here, and presumably, that interrupt is swallowed by the Esp32 system.   The result is that ESP32RMTController::showPixels() waits forever for the semaphore to be released (which it never will be) so we have a hang.  Note that this hang will occur minutes or hours or even days into a run, depending on other factors (like load or power), and is essentially out of the control of the FastLED driver.  

## The Fix

The quick and dirty fix is simply to assign some timeout value to the semaphore take rather than portMAX_DELAY (line 204 for version 3.6.0 and line 223 for version 3.7.0).  

We do this in FastLED version [3.7.1](https://github.com/FastLED/FastLED/releases/tag/3.7.1) (added August 8 2024) by changing FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM to a value like (2000/portTICK_PERIOD_MS) before we call FastLED.h (see  [clockless_rmt_esp32.h](https://github.com/FastLED/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.h)).  Alternatively, we can call GiveGTX_sem(); which has been added to [clockless_rmt_esp32.cpp](https://github.com/FastLED/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.cpp) so we can programmatically decide on the wait time.

(cf Pull request... https://github.com/FastLED/FastLED/pull/1651. )

## Quick note on the location of Libs
Debuggery is installed in the normal way and should appear once Platformio.ini does its update thing (naturally you have to have Plafromio installed).

~~The modified FastLED lib (until they accept my [pull](https://github.com/FastLED/FastLED/pull/1651) request) should be located in an adjacent folder in other words `..\FastLED`.  Download it from [here](https://github.com/davidlmorris/FastLED).~~  

```ini
lib_deps = davidlmorris/debuggery@^1.1.9
            FastLED/FastLED@^3.7.1
```

## Testing

The sample code here relies on two libraries [FastLED 3.7.1](https://github.com/FastLED/FastLED/releases/tag/3.7.1) (which includes the fix) and [debuggery](https://github.com/davidlmorris/debuggery) (just to be able to display information more easily).  Debuggery is registered with both Platformio and Arduino, so you can download the latest version through their normal process, or download the zip directly.

To run this code in the Arduino environment you will need to change the name of FastLED_Hang_Fix_Demo.cpp to FastLED_Hang_Fix_Demo.ino. Otherwise, this should be set to run in the Plafromio environment as is.

You do not need to add any additional hardware to the Esp32.

It is best to make sure you are creating a log file, and receiving data from the Serial port.  Run this overnight, or on a computer you are not using since as mentioned elsewhere this may take hours (longest I had was 17 hours) or perhaps even days to occur.

## Quick fix

If you are having the 'random hangs' problem on the Esp32 using the RMT driver then the quick fix is to change xSemaphoreTake(gTX_sem, portMAX_DELAY); to xSemaphoreTake(gTX_sem, 2000 / portTICK_PERIOD_MS); for a wait of 2 seconds rather than indefinitely for the semaphore to be freed.

You need to dive deep into the FastLED lib to \FastLED\src\platforms\esp\32\clockless_rmt_esp32.cpp line 204 for version 3.6.0 and line 223 for version 3.70.

To make it easy I've added the following to [clockless_rmt_esp32.h](https://github.com/FastLED/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.h):

```cpp
// This is work-around for the issue of random fastLed freezes randomly sometimes minutes
// but usually hours after the start, probably caused by interrupts 
// being swallowed by the system so that the gTX_sem semaphore is never released
// by the RMT interrupt handler causing FastLED.Show() never to return.

// The default is never return (or max ticks aka portMAX_DELAY).  
// To resolve this we need to set a maximum time to hold the semaphore.
// For example: To wait a maximum of two seconds (enough time for the Esp32 to have sorted 
// itself out and fast enough that it probably won't be greatly noticed by the audience)
// use:
// # define FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM (2000/portTICK_PERIOD_MS)
// (Place this in your code directly before the first call to FastLED.h)
#ifndef FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM  
#define FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM (portMAX_DELAY)
#endif
```

## How the Demo works

FastLED_Hang_Fix_Demo sets up a moderately pathological timer interrupt to give us some background interrupt contention.

Once setup is complete (printing out some useful background information to the serial port, setting up the show leds task, and setting up FastLED itself) the loop starts.

Every 15 minutes it prints a 'Running continuously for' block to we know it is still alive without filling up our event log.  Every loops it print random colours to all the Leds we have allocated to FastLED and shows them.

To show the Leds we have a task running (on core 1, the same as FastLed and the loop code) that wraps the FastLED.show between global bools NotShowing = false; and NotShowing = true; There are lots of yields so other tasks get a chance to run since FastLED.show might have kept the system busy.  This task is triggered once per loop with the following code, every n milliseconds where n is the slowest framerate of a FastLED pin rounded down if NotShowing is true (which avoids tripping over FastLED.Show() calling it more often than the relevant framerate, and also allows us to spend more time on other tasks).

Otherwise, after a second if it is still blocked (NotShowing =false:- meaning FastLED.Show() has jammed) it will display a message.  After another second, the 'so something' to the RMT driver from the message above will activate, and if that fails in another 13 seconds it will reboot the Esp32.

The 'so something' will either be a call GiveGTX_sem(); which has been added to `clockless_rmt_esp32.cpp` if we have set DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM or a wait for the time out we have set in FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM (the other change we made to `clockless_rmt_esp32.cpp`).

```cpp
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
```

## Serial Monitor sample result

In the sample below we were lucky to get a 'jam' at  9.27 minutes (note this is decimal minutes) into the run, and there were several that followed later.  However after 2:00 am things seemed to settle down, and it ran continuously for more than 5 hours (before a manual termination).

I trust you are able to replicate these results in some fashion or other.

```text
22:30:14.441 > rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
22:30:14.446 > configsip: 0, SPIWP:0xee
22:30:14.446 > clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
22:30:14.452 > mode:DIO, clock div:2
22:30:14.458 > load:0x3fff0030,len:1184
22:30:14.458 > load:0x40078000,len:13232
22:30:14.463 > load:0x40080400,len:3028
22:30:14.463 > entry 0x400805e4
22:30:17.689 > 
22:30:18.689 > 
22:30:19.694 > code-lights : 'src/FastLED_Hang_Fix_Demo.cpp Built: Aug  3 2024 22:29:54'
22:30:19.700 > BOARD_NAME selected is Esp32 doitESP32devkitV1.
22:30:19.700 > Starting comms 5.10 seconds after boot.
22:30:19.755 > Main App running on core 1 at 240 MHz.
22:30:19.909 > controllers[0]->size() = 256.
22:30:19.909 > controllers[1]->size() = 256.
22:30:19.909 > controllers[2]->size() = 470.
22:30:19.914 > controllers[3]->size() = 470.
22:30:19.914 > FastLED.count() = 4.
22:30:19.959 > fastLedShowHandlerTask running on core 1.
22:30:20.062 > FastLED Lowest frame rate in use is 70 (14 ms per frame).
22:30:20.211 > Initialisation Complete 5.63 seconds after boot.
22:39:30.793 > 
22:39:30.794 > FastLED.Show() has jammed after 9.27 minutes since boot!
22:39:30.857 > Forcing semaphore reset try.
22:39:30.930 > 
22:45:14.631 > Running continuously for 15.00 minutes(s) after boot.
23:00:14.487 > Running continuously for 30.00 minutes(s) after boot.
23:15:14.498 > Running continuously for 45.00 minutes(s) after boot.
23:18:33.731 > 
23:18:33.732 > FastLED.Show() has jammed after 48.32 minutes since boot!
23:18:33.808 > Forcing semaphore reset try.
23:18:33.872 > 
23:30:14.372 > Running continuously for 1.00 hour(s) after boot.
23:42:29.876 > 
23:42:29.877 > FastLED.Show() has jammed after 72.26 minutes since boot!
23:42:29.955 > Forcing semaphore reset try.
23:42:30.017 > 
23:45:14.270 > Running continuously for 75.00 minutes(s) after boot.
23:50:03.661 > 
23:50:03.662 > FastLED.Show() has jammed after 79.82 minutes since boot!
23:50:03.739 > Forcing semaphore reset try.
23:50:03.801 > 
00:00:14.250 > Running continuously for 90.00 minutes(s) after boot.
00:14:00.040 > 
00:14:00.040 > FastLED.Show() has jammed after 103.77 minutes since boot!
00:14:00.119 > Forcing semaphore reset try.
00:14:00.183 > 
00:15:14.116 > Running continuously for 105.00 minutes(s) after boot.
00:21:33.798 > 
00:21:33.799 > FastLED.Show() has jammed after 111.33 minutes since boot!
00:21:33.875 > Forcing semaphore reset try.
00:21:33.939 > 
00:30:14.140 > Running continuously for 2.00 hour(s) after boot.
00:45:13.972 > Running continuously for 135.00 minutes(s) after boot.
01:00:13.944 > Running continuously for 150.00 minutes(s) after boot.
01:15:13.894 > Running continuously for 165.00 minutes(s) after boot.
01:30:13.850 > Running continuously for 3.00 hour(s) after boot.
01:45:13.712 > Running continuously for 195.00 minutes(s) after boot.
01:48:28.158 > 
01:48:28.158 > FastLED.Show() has jammed after 198.24 minutes since boot!
01:48:28.236 > Forcing semaphore reset try.
01:48:28.299 > 
01:56:01.753 > 
01:56:01.754 > FastLED.Show() has jammed after 205.80 minutes since boot!
01:56:01.831 > Forcing semaphore reset try.
01:56:01.894 > 
02:00:13.670 > Running continuously for 210.00 minutes(s) after boot.
02:15:13.621 > Running continuously for 225.00 minutes(s) after boot.
02:30:13.592 > Running continuously for 4.00 hour(s) after boot.
02:45:13.472 > Running continuously for 255.00 minutes(s) after boot.
03:00:13.458 > Running continuously for 270.00 minutes(s) after boot.
03:15:13.329 > Running continuously for 285.00 minutes(s) after boot.
03:30:13.326 > Running continuously for 5.00 hour(s) after boot.
03:45:13.151 > Running continuously for 315.00 minutes(s) after boot.
04:00:13.160 > Running continuously for 330.00 minutes(s) after boot.
04:15:13.028 > Running continuously for 345.00 minutes(s) after boot.
04:30:13.031 > Running continuously for 6.00 hour(s) after boot.
04:45:12.952 > Running continuously for 375.00 minutes(s) after boot.
05:00:12.834 > Running continuously for 390.00 minutes(s) after boot.
05:15:12.787 > Running continuously for 405.00 minutes(s) after boot.
05:30:12.733 > Running continuously for 7.00 hour(s) after boot.
05:45:12.623 > Running continuously for 435.00 minutes(s) after boot.
06:00:12.601 > Running continuously for 450.00 minutes(s) after boot.
06:15:12.545 > Running continuously for 465.00 minutes(s) after boot.
06:30:12.454 > Running continuously for 8.00 hour(s) after boot.
06:45:12.335 > Running continuously for 495.00 minutes(s) after boot.
07:00:12.257 > Running continuously for 510.00 minutes(s) after boot.
07:15:12.191 > Running continuously for 525.00 minutes(s) after boot.
07:30:12.115 > Running continuously for 9.00 hour(s) after boot.
07:45:12.060 > Running continuously for 555.00 minutes(s) after boot.
```

The following run was for more than 13 hours overnight, and yet had zero hangs, so presumably there are times when this can go for a long time without causing a hangs.  (Longest I've noticed was 17 hours... but I am sure you can do better!)

(Note the small display changes, though the code is still running exactly the same pathological timer interrupts.)
```text
17:27:20.765 > ets Jun  8 2016 00:22:57
17:27:20.765 >
17:27:20.765 > rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
17:27:20.771 > configsip: 0, SPIWP:0xee
17:27:20.771 > clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
17:27:20.776 > mode:DIO, clock div:2
17:27:20.781 > load:0x3fff0030,len:1184
17:27:20.781 > load:0x40078000,len:13232
17:27:20.786 > load:0x40080400,len:3028 
17:27:20.786 > entry 0x400805e4
17:27:24.014 > 
17:27:25.015 > 
17:27:26.019 > FastLED_Hang_Fix_Demo 'src/FastLED_Hang_Fix_Demo.cpp' Built: Aug  4 2024 17:27:04.
17:27:26.040 > BOARD_NAME selected is Esp32 doitESP32devkitV1.
17:27:26.040 > Starting comms 5.10 seconds after boot.
17:27:26.082 > Main App running on core 1 at 240 MHz.
17:27:26.235 > FastLED controllers[0]->size() = 256.
17:27:26.235 > FastLED controllers[1]->size() = 256.
17:27:26.241 > FastLED controllers[2]->size() = 470.
17:27:26.241 > FastLED controllers[3]->size() = 470.
17:27:26.246 > FastLED.count() = 4.
17:27:26.297 > fastLedShowHandlerTask running on core 1.
17:27:26.400 > FastLED Lowest frame rate in use is 70 (14 ms per frame).
17:27:26.556 > Initialisation Complete 5.64 seconds after boot.
17:27:26.699 > Running continuously for 00:05 seconds(s) after boot at an average of 0.00 loops per second.
17:42:20.881 > Running continuously for 15:00 minutes(s) after boot at an average of 21.62 loops per second.
17:57:20.851 > Running continuously for 30:00 minutes(s) after boot at an average of 21.77 loops per second.
18:12:20.770 > Running continuously for 45:00 minutes(s) after boot at an average of 21.78 loops per second.
18:27:20.719 > Running continuously for  1:00:00 hour(s) after boot at an average of 21.75 loops per second.
18:42:20.643 > Running continuously for  1:15:00 hour(s) after boot at an average of 21.77 loops per second.
18:57:20.536 > Running continuously for  1:30:00 hour(s) after boot at an average of 21.76 loops per second.
19:12:20.508 > Running continuously for  1:45:00 hour(s) after boot at an average of 21.81 loops per second.
19:27:20.428 > Running continuously for  2:00:00 hour(s) after boot at an average of 21.78 loops per second.
19:42:20.386 > Running continuously for  2:15:00 hour(s) after boot at an average of 21.73 loops per second.
19:57:20.305 > Running continuously for  2:30:00 hour(s) after boot at an average of 21.76 loops per second.
20:12:20.206 > Running continuously for  2:45:00 hour(s) after boot at an average of 21.81 loops per second.
20:27:20.183 > Running continuously for  3:00:00 hour(s) after boot at an average of 21.75 loops per second.
20:42:20.112 > Running continuously for  3:15:00 hour(s) after boot at an average of 21.75 loops per second.
20:57:20.033 > Running continuously for  3:30:00 hour(s) after boot at an average of 21.78 loops per second.
21:12:19.932 > Running continuously for  3:45:00 hour(s) after boot at an average of 21.78 loops per second.
21:27:19.873 > Running continuously for  4:00:00 hour(s) after boot at an average of 21.75 loops per second.
21:42:19.814 > Running continuously for  4:15:00 hour(s) after boot at an average of 21.79 loops per second.
21:57:19.724 > Running continuously for  4:30:00 hour(s) after boot at an average of 21.76 loops per second.
22:12:19.666 > Running continuously for  4:45:00 hour(s) after boot at an average of 21.76 loops per second.
22:27:19.599 > Running continuously for  5:00:00 hour(s) after boot at an average of 21.73 loops per second.
22:42:19.538 > Running continuously for  5:15:00 hour(s) after boot at an average of 21.68 loops per second.
22:57:19.501 > Running continuously for  5:30:00 hour(s) after boot at an average of 21.70 loops per second.
23:12:19.402 > Running continuously for  5:45:00 hour(s) after boot at an average of 21.61 loops per second.
23:27:19.319 > Running continuously for  6:00:00 hour(s) after boot at an average of 21.67 loops per second.
23:42:19.257 > Running continuously for  6:15:00 hour(s) after boot at an average of 21.69 loops per second.
23:57:19.180 > Running continuously for  6:30:00 hour(s) after boot at an average of 21.67 loops per second.
00:12:19.153 > Running continuously for  6:45:00 hour(s) after boot at an average of 21.65 loops per second.
00:27:19.067 > Running continuously for  7:00:00 hour(s) after boot at an average of 21.70 loops per second.
00:42:18.966 > Running continuously for  7:15:00 hour(s) after boot at an average of 21.65 loops per second.
00:57:18.895 > Running continuously for  7:30:00 hour(s) after boot at an average of 21.69 loops per second.
01:12:18.826 > Running continuously for  7:45:00 hour(s) after boot at an average of 21.69 loops per second.
01:27:18.794 > Running continuously for  8:00:00 hour(s) after boot at an average of 21.67 loops per second.
01:42:18.695 > Running continuously for  8:15:00 hour(s) after boot at an average of 21.70 loops per second.
01:57:18.664 > Running continuously for  8:30:00 hour(s) after boot at an average of 21.70 loops per second.
02:12:18.543 > Running continuously for  8:45:00 hour(s) after boot at an average of 21.69 loops per second.
02:27:18.494 > Running continuously for  9:00:00 hour(s) after boot at an average of 21.66 loops per second.
02:42:18.436 > Running continuously for  9:15:00 hour(s) after boot at an average of 21.69 loops per second.
02:57:18.357 > Running continuously for  9:30:00 hour(s) after boot at an average of 21.67 loops per second.
03:12:18.288 > Running continuously for  9:45:00 hour(s) after boot at an average of 21.65 loops per second.
03:27:18.221 > Running continuously for 10:00:00 hour(s) after boot at an average of 21.67 loops per second.
03:42:18.121 > Running continuously for 10:15:00 hour(s) after boot at an average of 21.70 loops per second.
03:57:18.081 > Running continuously for 10:30:00 hour(s) after boot at an average of 21.70 loops per second.
04:12:18.017 > Running continuously for 10:45:00 hour(s) after boot at an average of 21.69 loops per second.
04:27:17.949 > Running continuously for 11:00:00 hour(s) after boot at an average of 21.70 loops per second.
04:42:17.895 > Running continuously for 11:15:00 hour(s) after boot at an average of 21.67 loops per second.
04:57:17.818 > Running continuously for 11:30:00 hour(s) after boot at an average of 21.66 loops per second.
05:12:17.775 > Running continuously for 11:45:00 hour(s) after boot at an average of 21.69 loops per second.
05:27:17.681 > Running continuously for 12:00:00 hour(s) after boot at an average of 21.73 loops per second.
05:42:17.593 > Running continuously for 12:15:00 hour(s) after boot at an average of 21.67 loops per second.
05:57:17.532 > Running continuously for 12:30:00 hour(s) after boot at an average of 21.70 loops per second.
06:12:17.426 > Running continuously for 12:45:00 hour(s) after boot at an average of 21.69 loops per second.
06:27:17.388 > Running continuously for 13:00:00 hour(s) after boot at an average of 21.71 loops per second.
06:42:17.285 > Running continuously for 13:15:00 hour(s) after boot at an average of 21.69 loops per second.
06:57:17.274 > Running continuously for 13:30:00 hour(s) after boot at an average of 21.73 loops per second.
```

## Update 1.1.1 - 2024-08-05 Forcing the error

Upped the pathology a tad! (Ok at severe risk of guru meditation errors or worse lockups.)  But at least you get a chance to see a fastled hang earlier than typical...  finding the error sweet spot requires a bit of finessing, and you might want to play with the code a bit, since who knows, each Esp32 or board might be slightly different.

Go back to 1.1.0 for a more normal result, or comment out `digitalWrite(DIO_TM1637_DIGIT_DISPLAY, outVal);` lines, and/or play around with the `void IRAM_ATTR onTimer()` call or vTaskDelay to see if you can tweak the number of hangs.

## Update 1.1.2 - 2024-08-05 Forcing the error for the even more impatient

- Upped the pathology even more!  This should give Jams within a minute or 10.  And only the occasional Guru Meditation.
- Changed the update time ('I'm still alive message') from 15 mins to 1 min.

```text
12:34:42.509 > FastLED_Hang_Fix_Demo 1.1.1 'src/FastLED_Hang_Fix_Demo.cpp' Built: Aug  5 2024 12:34:20.
12:34:42.515 > BOARD_NAME selected is Esp32 doitESP32devkitV1.
12:34:42.520 > Starting comms 5.10 seconds after boot.
12:34:42.571 > Main App running on core 1 at 240 MHz.
12:34:42.724 > FastLED controllers[0]->size() = 256.
12:34:42.725 > FastLED controllers[1]->size() = 256.
12:34:42.730 > FastLED controllers[2]->size() = 470.
12:34:42.730 > FastLED controllers[3]->size() = 470.
12:34:42.735 > FastLED.count() = 4.
12:34:42.786 > fastLedShowHandlerTask running on core 1.
12:34:42.885 > FastLED Lowest frame rate in use is 70 (14 ms per frame).
12:34:43.098 > Initialisation Complete 5.72 seconds after boot.
12:34:44.053 > Running continuously for 00:06 seconds(s) after boot. Speed 0.02 loops per sec (int = 2704.78/sec).
12:35:38.369 > Running continuously for 01:00 minutes(s) after boot. Speed 1.23 loops per sec (int = 158436.83/sec).
12:36:38.529 > Running continuously for 02:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175498.93/sec).
12:37:38.654 > Running continuously for 03:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175398.53/sec).
12:38:38.820 > Running continuously for 04:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175516.65/sec).
12:39:38.267 > Running continuously for 05:00 minutes(s) after boot. Speed 1.35 loops per sec (int = 173415.20/sec).
12:40:37.814 > Running continuously for 06:00 minutes(s) after boot. Speed 1.45 loops per sec (int = 173636.58/sec).
12:41:05.083 > 
12:41:05.086 > FastLED.Show() has jammed after 6.46 minutes since boot!
12:41:05.184 > Forcing semaphore reset try.
12:41:05.260 > 
12:41:37.886 > Running continuously for 07:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174777.00/sec).
12:41:46.381 > 
12:41:46.383 > FastLED.Show() has jammed after 7.15 minutes since boot!
12:41:46.482 > Forcing semaphore reset try.
12:41:46.556 > 
12:42:12.599 > 
12:42:12.600 > FastLED.Show() has jammed after 7.59 minutes since boot!
12:42:12.698 > Forcing semaphore reset try.
12:42:12.773 > 
12:42:37.946 > Running continuously for 08:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174738.18/sec).
12:43:06.819 > 
12:43:06.821 > FastLED.Show() has jammed after 8.49 minutes since boot!
12:43:06.920 > Forcing semaphore reset try.
12:43:06.993 > 
12:43:37.999 > Running continuously for 09:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174708.20/sec).
12:43:52.325 > 
12:43:52.327 > FastLED.Show() has jammed after 9.25 minutes since boot!
12:43:52.427 > Forcing semaphore reset try.
12:43:52.501 > 
12:44:28.207 > 
12:44:28.209 > FastLED.Show() has jammed after 9.85 minutes since boot!
12:44:28.307 > Forcing semaphore reset try.
12:44:28.383 > 
12:44:37.730 > Running continuously for 10:00 minutes(s) after boot. Speed 2.97 loops per sec (int = 173782.18/sec).
12:44:54.964 > 
12:44:54.965 > FastLED.Show() has jammed after 10.29 minutes since boot!
12:44:55.066 > Forcing semaphore reset try.
12:44:55.142 > 
12:45:24.400 > 
12:45:24.408 > FastLED.Show() has jammed after 10.78 minutes since boot!
12:45:24.499 > Forcing semaphore reset try.
12:45:24.575 > 
12:45:37.787 > Running continuously for 11:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174721.70/sec).
12:46:29.925 > 
12:46:29.927 > FastLED.Show() has jammed after 11.88 minutes since boot!
12:46:30.028 > Forcing semaphore reset try.
12:46:30.102 > 
12:46:37.819 > Running continuously for 12:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174653.53/sec).
12:47:07.664 > 
12:47:07.665 > FastLED.Show() has jammed after 12.51 minutes since boot!
12:47:07.764 > Forcing semaphore reset try.
12:47:07.839 > 
12:47:35.794 > 
12:47:35.796 > FastLED.Show() has jammed after 12.97 minutes since boot!
12:47:35.896 > Forcing semaphore reset try.
12:47:35.971 > 
12:47:37.865 > Running continuously for 13:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174690.52/sec).
12:48:15.478 > 
12:48:15.479 > FastLED.Show() has jammed after 13.64 minutes since boot!
12:48:15.580 > Forcing semaphore reset try.
12:48:15.654 > 
12:48:37.917 > Running continuously for 14:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174712.72/sec).
12:49:28.455 > 
12:49:28.457 > FastLED.Show() has jammed after 14.85 minutes since boot!
12:49:28.557 > Forcing semaphore reset try.
12:49:28.631 > 
12:49:37.969 > Running continuously for 15:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174704.87/sec).
12:50:13.940 > 
12:50:13.942 > FastLED.Show() has jammed after 15.61 minutes since boot!
12:50:14.042 > Forcing semaphore reset try.
12:50:14.116 > 
12:50:26.871 > 
12:50:26.873 > FastLED.Show() has jammed after 15.83 minutes since boot!
12:50:26.974 > Forcing semaphore reset try.
12:50:27.048 > 
12:50:33.016 > 
12:50:33.018 > FastLED.Show() has jammed after 15.93 minutes since boot!
12:50:33.117 > Forcing semaphore reset try.
12:50:33.191 > 
12:50:37.676 > Running continuously for 16:00 minutes(s) after boot. Speed 2.92 loops per sec (int = 173722.02/sec).
12:50:49.751 > 
12:50:49.752 > FastLED.Show() has jammed after 16.21 minutes since boot!
12:50:49.853 > Forcing semaphore reset try.
12:50:49.928 > 
12:51:37.771 > Running continuously for 17:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174838.70/sec).
12:52:05.693 > 
12:52:05.696 > FastLED.Show() has jammed after 17.47 minutes since boot!
12:52:05.795 > Forcing semaphore reset try.
12:52:05.870 > 
12:52:37.853 > Running continuously for 18:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174793.22/sec).
12:53:29.030 > 
12:53:29.031 > FastLED.Show() has jammed after 18.86 minutes since boot!
12:53:29.131 > Forcing semaphore reset try.
12:53:29.205 > 
12:53:37.894 > Running continuously for 19:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174675.67/sec).
12:53:49.958 > 
12:53:49.960 > FastLED.Show() has jammed after 19.21 minutes since boot!
12:53:50.060 > Forcing semaphore reset try.
12:53:50.134 > 
12:53:55.791 > 
12:53:55.793 > FastLED.Show() has jammed after 19.31 minutes since boot!
12:53:55.894 > Forcing semaphore reset try.
12:53:55.969 > 
12:54:23.271 > 
12:54:23.273 > FastLED.Show() has jammed after 19.77 minutes since boot!
12:54:23.373 > Forcing semaphore reset try.
12:54:23.448 > 
12:54:26.188 > 
12:54:26.190 > FastLED.Show() has jammed after 19.82 minutes since boot!
12:54:26.290 > Forcing semaphore reset try.
12:54:26.366 > 
12:54:36.224 > 
12:54:36.226 > FastLED.Show() has jammed after 19.98 minutes since boot!
12:54:36.325 > Forcing semaphore reset try.
12:54:36.400 > 
12:54:37.646 > Running continuously for 20:00 minutes(s) after boot. Speed 2.82 loops per sec (int = 173866.67/sec).
12:54:58.763 > 
12:54:58.764 > FastLED.Show() has jammed after 20.36 minutes since boot!
12:54:58.864 > Forcing semaphore reset try.
12:54:58.939 > 
12:55:19.130 > 
12:55:19.133 > FastLED.Show() has jammed after 20.70 minutes since boot!
12:55:19.232 > Forcing semaphore reset try.
12:55:19.306 > 
12:55:37.697 > Running continuously for 21:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174716.12/sec).
12:55:52.980 > 
12:55:52.980 > FastLED.Show() has jammed after 21.26 minutes since boot!
12:55:53.081 > Forcing semaphore reset try.
12:55:53.154 > 
12:56:27.931 > 
12:56:27.933 > FastLED.Show() has jammed after 21.84 minutes since boot!
12:56:28.035 > Forcing semaphore reset try.
12:56:28.109 > 
12:56:37.768 > Running continuously for 22:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174779.80/sec).
12:57:37.808 > Running continuously for 23:00 minutes(s) after boot. Speed 3.08 loops per sec (int = 174660.20/sec).
12:58:01.835 > 
12:58:01.835 > FastLED.Show() has jammed after 23.41 minutes since boot!
12:58:01.937 > Forcing semaphore reset try.
12:58:02.013 > 
12:58:37.880 > Running continuously for 24:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174760.48/sec).
12:59:07.740 > 
12:59:07.742 > FastLED.Show() has jammed after 24.51 minutes since boot!
12:59:07.840 > Forcing semaphore reset try.
12:59:07.917 > 
12:59:21.654 > 
12:59:21.656 > FastLED.Show() has jammed after 24.74 minutes since boot!
12:59:21.757 > Forcing semaphore reset try.
12:59:21.831 > 
12:59:37.636 > Running continuously for 25:00 minutes(s) after boot. Speed 2.97 loops per sec (int = 173864.68/sec).
13:00:23.341 > 
13:00:23.343 > FastLED.Show() has jammed after 25.77 minutes since boot!
13:00:23.443 > Forcing semaphore reset try.
13:00:23.518 > 
13:00:37.709 > Running continuously for 26:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174762.83/sec).
13:01:37.786 > Running continuously for 27:00 minutes(s) after boot. Speed 3.08 loops per sec (int = 174775.97/sec).
13:01:45.316 > 
13:01:45.318 > FastLED.Show() has jammed after 27.13 minutes since boot!
13:01:45.416 > Forcing semaphore reset try.
13:01:45.489 > 
13:02:15.698 > 
13:02:15.699 > FastLED.Show() has jammed after 27.64 minutes since boot!
13:02:15.799 > Forcing semaphore reset try.
13:02:15.873 > 
13:02:37.832 > Running continuously for 28:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174698.00/sec).
13:03:26.137 > 
13:03:26.138 > FastLED.Show() has jammed after 28.81 minutes since boot!
13:03:26.238 > Forcing semaphore reset try.
13:03:26.312 > 
13:03:37.916 > Running continuously for 29:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174804.00/sec).
13:04:29.467 > 
13:04:29.468 > FastLED.Show() has jammed after 29.87 minutes since boot!
13:04:29.569 > Forcing semaphore reset try.
13:04:29.644 > 
13:04:37.685 > Running continuously for 30:00 minutes(s) after boot. Speed 3.02 loops per sec (int = 173883.68/sec).
13:04:50.396 > 
13:04:50.397 > FastLED.Show() has jammed after 30.22 minutes since boot!
13:04:50.496 > Forcing semaphore reset try.
13:04:50.572 > 
13:05:37.764 > Running continuously for 31:00 minutes(s) after boot. Speed 3.03 loops per sec (int = 174779.97/sec).
13:05:43.354 > 
13:05:43.355 > FastLED.Show() has jammed after 31.10 minutes since boot!
13:05:43.456 > Forcing semaphore reset try.
13:05:43.530 > 
13:05:46.594 > 
13:05:46.596 > FastLED.Show() has jammed after 31.16 minutes since boot!
13:05:46.696 > Forcing semaphore reset try.
13:05:46.771 > 
13:06:37.829 > Running continuously for 32:00 minutes(s) after boot. Speed 2.98 loops per sec (int = 174757.83/sec).
13:07:37.868 > Running continuously for 33:00 minutes(s) after boot. Speed 3.08 loops per sec (int = 174655.65/sec).
13:08:37.621 > Running continuously for 34:00 minutes(s) after boot. Speed 3.07 loops per sec (int = 173827.33/sec).
13:09:21.541 > Guru Meditation Error: Core  1 panic'ed (Interrupt wdt timeout on CPU1). 
13:09:21.547 >
13:09:21.547 > Core  1 register dump:
13:09:21.547 > PC      : 0x400813fb  PS      : 0x00050035  A0      : 0x40084de0  A1      : 0x3ffbf23c
13:09:21.553 > A2      : 0x00000008  A3      : 0x3ff56000  A4      : 0x01000000  A5      : 0x3ffc1c8c  
13:09:21.564 > A6      : 0x00000008  A7      : 0x00000001  A8      : 0x3ffc1cc8  A9      : 0x00000012
13:09:21.569 > A10     : 0x00000000  A11     : 0x00000000  A12     : 0x00000001  A13     : 0x00000004  
13:09:21.581 > A14     : 0x00060a23  A15     : 0x00000001  SAR     : 0x0000001e  EXCCAUSE: 0x00000006
13:09:21.586 > EXCVADDR: 0x00000000  LBEG    : 0x40084775  LEND    : 0x4008477d  LCOUNT  : 0x00000027
13:09:21.597 > Core  1 was running in ISR context:
13:09:21.597 > EPC1    : 0x400de31f  EPC2    : 0x00000000  EPC3    : 0x4000bff0  EPC4    : 0x00000000
13:09:21.603 >
13:09:21.608 >
13:09:21.609 > Backtrace: 0x400813f8:0x3ffbf23c |<-CORRUPTED
13:09:21.609 >
13:09:21.609 >
13:09:21.609 > Core  0 register dump:
13:09:21.614 > PC      : 0x400f4fb6  PS      : 0x00060535  A0      : 0x800dd20d  A1      : 0x3ffbc7a0
13:09:21.619 > A2      : 0x00000000  A3      : 0x40085d14  A4      : 0x00060520  A5      : 0x3ffbbc70
13:09:21.631 > A6      : 0x007bf388  A7      : 0x003fffff  A8      : 0x800dce0e  A9      : 0x3ffbc770
13:09:21.636 > A10     : 0x00000000  A11     : 0x3ffbf384  A12     : 0x3ffbf384  A13     : 0x00000000
13:09:21.642 > A14     : 0x00060520  A15     : 0x00000000  SAR     : 0x0000001d  EXCCAUSE: 0x00000006  
13:09:21.652 > EXCVADDR: 0x00000000  LBEG    : 0x00000000  LEND    : 0x00000000  LCOUNT  : 0x00000000  
13:09:21.658 >
13:09:21.658 >
13:09:21.658 > Backtrace: 0x400f4fb3:0x3ffbc7a0 0x400dd20a:0x3ffbc7c0 0x400898f9:0x3ffbc7e0
13:09:21.663 >
13:09:21.663 >
13:09:21.663 >
13:09:21.667 >
13:09:21.667 > ELF file SHA256: 24c55f94355192d9
13:09:21.667 >
13:09:21.786 > Rebooting...
13:09:21.792 > ets Jun  8 2016 00:22:57
13:09:21.792 >
13:09:21.792 > rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
13:09:21.798 > configsip: 0, SPIWP:0xee
13:09:21.798 > clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
13:09:21.803 > mode:DIO, clock div:2
13:09:21.809 > load:0x3fff0030,len:1184
13:09:21.809 > load:0x40078000,len:13232
13:09:21.814 > load:0x40080400,len:3028
13:09:21.814 > entry 0x400805e4
13:09:25.043 > 
13:09:26.043 > 
13:09:27.048 > FastLED_Hang_Fix_Demo 1.1.1 'src/FastLED_Hang_Fix_Demo.cpp' Built: Aug  5 2024 12:34:20.
13:09:27.054 > BOARD_NAME selected is Esp32 doitESP32devkitV1.
13:09:27.059 > Starting comms 5.10 seconds after boot.
13:09:27.110 > Main App running on core 1 at 240 MHz.
13:09:27.263 > FastLED controllers[0]->size() = 256.
13:09:27.264 > FastLED controllers[1]->size() = 256.
13:09:27.269 > FastLED controllers[2]->size() = 470.
13:09:27.269 > FastLED controllers[3]->size() = 470.
13:09:27.274 > FastLED.count() = 4.
13:09:27.326 > fastLedShowHandlerTask running on core 1.
13:09:27.425 > FastLED Lowest frame rate in use is 70 (14 ms per frame).
13:09:27.636 > Initialisation Complete 5.72 seconds after boot.
13:09:28.592 > Running continuously for 00:06 seconds(s) after boot. Speed 0.02 loops per sec (int = 2721.12/sec).
13:10:22.897 > Running continuously for 01:00 minutes(s) after boot. Speed 1.23 loops per sec (int = 158394.22/sec).
13:11:23.044 > Running continuously for 02:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175449.75/sec).
13:12:23.179 > Running continuously for 03:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175433.62/sec).
13:13:23.349 > Running continuously for 04:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175529.77/sec).
13:14:22.790 > Running continuously for 05:00 minutes(s) after boot. Speed 1.35 loops per sec (int = 173406.73/sec).
13:15:22.928 > Running continuously for 06:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175436.35/sec).
13:16:23.094 > Running continuously for 07:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175515.72/sec).
13:17:23.240 > Running continuously for 08:01 minutes(s) after boot. Speed 1.37 loops per sec (int = 175449.45/sec).
13:18:22.668 > Running continuously for 09:00 minutes(s) after boot. Speed 1.35 loops per sec (int = 173368.93/sec).
```
