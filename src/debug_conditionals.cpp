
#include "debug_conditionals.h"


const unsigned long xTickATinyBit = (50 / portTICK_PERIOD_MS);
const unsigned long xTickHalfASec = (500 / portTICK_PERIOD_MS);
const unsigned long xTickFullSec = (1000 / portTICK_PERIOD_MS);
const unsigned long xTickFiveSecs = (5000 / portTICK_PERIOD_MS);

#ifdef DEBUG_ON

#ifdef ESP32
StaticSemaphore_t xSemaphoreSerialPortBuffer;
#endif
SemaphoreHandle_t xBinarySemaphoreSerialPort = NULL;

void initSemaphore(void)
    {
 #ifdef ESP32
     xBinarySemaphoreSerialPort = xSemaphoreCreateBinaryStatic(&xSemaphoreSerialPortBuffer);
//     // xSemaphoreCreateBinaryStatic is undefined in <Arduino_FreeRTOS.h>
//     // Need #define configSUPPORT_STATIC_ALLOCATION 1 for FreeRTOS code
//     // cf https://github.com/earlephilhower/arduino-pico/issues/564
 #else    
    xBinarySemaphoreSerialPort = xSemaphoreCreateBinary();
 #endif    
//    xSemaphoreTake(xBinarySemaphoreSerialPort, (TickType_t) xTickFullSec) ;
    xSemaphoreGive(xBinarySemaphoreSerialPort);
    vTaskDelay(xTickATinyBit);
    }

bool takeSemaphore(void)
    {
    return (xSemaphoreTake(xBinarySemaphoreSerialPort, (TickType_t) xTickFullSec ) == pdTRUE);
    }

void giveSemaphore(void)
    {
    vTaskDelay(xTickATinyBit); // Allow a little time for the serial port to do its print.
    xSemaphoreGive(xBinarySemaphoreSerialPort);
    }

void debugDisplaySeconds(const char* sMessage, uint32_t seconds_run_time)
    {
    char buffer[NUMBER_BUFFER + 1];        

    uint32_t tm_Hours = (seconds_run_time / 3600);
    uint32_t tm_Minutes = (seconds_run_time / 60) % 60;
    uint32_t tm_Seconds = seconds_run_time % 60;
    DEBUG_PRINT(sMessage);
    if (tm_Hours > 0)
        {
        snprintf(buffer, NUMBER_BUFFER, "%u:", tm_Hours);
        DEBUG_PRINT(buffer);
        }
    snprintf(buffer, NUMBER_BUFFER, "%02.2u:", tm_Minutes);
    DEBUG_PRINT(buffer);
    snprintf(buffer, NUMBER_BUFFER, "%02.2u", tm_Seconds);
    DEBUG_PRINT(buffer);
    if (tm_Hours > 0)
        {
        DEBUG_PRINT(" hour(s) ");
        }
    else
        {
        if (tm_Minutes > 0)
            {
            DEBUG_PRINT(" minutes(s) ");
            }
        else
            {
            DEBUG_PRINT(" seconds(s) ");
            }
        }
    }

#endif
