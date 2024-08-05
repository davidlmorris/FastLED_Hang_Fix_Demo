# FastLED_Hang_Fix_Demo

 Demonstration and fix for FastLED.show() random hang.  FastLED is a library to support the display of Addressable LED which can run on a number of MCUs including the Esp32.

The Esp32 is a fast MCU, but struggles (probably because of FreeRTOS) to manage interrupts running over about 200 kilohertz.  See this [YouTube](<https://www.youtube.com/watch?v=CJhWlfkf-5M>) clip at about 8:30 for a compelling demonstration.

FastLED is reported (notably [here](https://github.com/FastLED/FastLED/issues/1438)) to hang after a random amount of time usually hours after the Esp32 has started.

## Why

The FastLed RMT driver makes extensive use of interrupts.  The comments in the FastLED driver hint at least one per 32 bits of data out.  The driver does all the sending in one hit so that even with multiple controllers (as we have in this demo) it is only when the last one is called does it finally does the work.  I am uncertain what might happen if only one controller that is not the last one is called.  (Something for a future experiment?). On a send, it waits 50 microseconds to signal a start, then starts the channels for each controller.  It then sets a semaphore which is then released when the final channel on the final controller is complete.  It appears that rarely something goes wrong here, and presumably, that interrupt is swallowed by the Esp32 system.   The result is that ESP32RMTController::showPixels() waits forever for the semaphore to be released (which it never will be) so we have a hang.  Note that this hang will occur minutes or hours or even days into a run, depending on other factors (like load or power), and is essentially out of the control of the FastLED driver.  

## The Fix

The quick and dirty fix is simply to assign some timeout value to the semaphore take rather than portMAX_DELAY (line 204 for version 3.6.0 and line 223 for version 3.7.0).  

We do this in the modified FastLED lib forked [here](https://github.com/davidlmorris/FastLED) by defining FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM to a value like (2000/portTICK_PERIOD_MS) before we call FastLED.h (see  [clockless_rmt_esp32.h](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.h)).  Alternatively, we can call GiveGTX_sem(); which has been added to [clockless_rmt_esp32.cpp](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.cpp) so we can programmatically decide on the wait time.

## Quick note on the location of Libs
Debuggery is installed in the normal way and should appear once Platformio.ini does its update thing (naturally you have to have Plafromio installed).

The modified FastLED lib (until they accept my [pull](https://github.com/FastLED/FastLED/pull/1651) request) should be located in an adjacent folder in other words `..\FastLED`.  Download it from [here](https://github.com/davidlmorris/FastLED).  In platform.ini lib_deps has been modified to assume that position.

```ini
lib_deps = davidlmorris/debuggery@^1.1.9
            ..\FastLED
```

## Testing

The sample code here relies on two libraries the modified version of [FastLED](https://github.com/davidlmorris/FastLED) and [debuggery](https://github.com/davidlmorris/debuggery) (just to be able to display information more easily).  Debuggery is registered with both Platformio and Arduino, so you can download the latest version through their normal process, or download the zip directly.  The easiest way to use this version of FastLED is simply to navigate to `FastLED\src\platforms\esp\32\` and download [clockless_rmt_esp32.cpp](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.cpp) and [clockless_rmt_esp32.h](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.h) into the same location of your local copy of the FastLED library, as these are the only files that have any modifications.

To run this code in the Arduino environment you will need to change the name of FastLED_Hang_Fix_Demo.cpp to FastLED_Hang_Fix_Demo.ino.  
Otherwise this should be set to run in the Plafromio environment as is.

You do not need to add any additional hardware to the Esp32.  

It is best to make sure you are creating a log file, and receiving data from the Serial port.  Run this overnight, or on a computer you are not using since as mentioned elsewhere this may take hours (longest I had was 17 hours) or perhaps even days to occur.

## Quick fix

If you are having the 'random hangs' problem on the Esp32 using the RMT driver then the quick fix is to change xSemaphoreTake(gTX_sem, portMAX_DELAY); to xSemaphoreTake(gTX_sem, 2000 / portTICK_PERIOD_MS); for a wait of 2 seconds rather than indefinitely for the semaphore to be freed.

You need to dive deep into the FastLED lib to \FastLED\src\platforms\esp\32\clockless_rmt_esp32.cpp line 204 for version 3.6.0 and line 223 for version 3.70.

To make it easy I've added the following to [clockless_rmt_esp32.h](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.h):

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

The 'so something' will either be a call GiveGTX_sem(); which we have added to [clockless_rmt_esp32.cpp](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.cpp) if we have set DEBUG_USE_PORT_MAX_DELAY_FOR_GTX_SEM or a wait for the time out we have set in FASTLED_RMT_MAX_TICKS_FOR_GTX_SEM (the other change we made to [clockless_rmt_esp32.cpp](https://github.com/davidlmorris/FastLED/tree/master/src/platforms/esp/32/clockless_rmt_esp32.cpp)).

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
