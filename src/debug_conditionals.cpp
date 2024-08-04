#include <Arduino.h>
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
#endif
