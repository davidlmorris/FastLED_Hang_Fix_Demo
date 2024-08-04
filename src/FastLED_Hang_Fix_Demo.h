

// Showing #defines at compile time cf https://stackoverflow.com/a/10791845/3137
// #define XSTR(x) STR(x)
// #define STR(x) #x
#define NO_OF_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

#if defined(ESP32)

// START OF PINS 
// This is on DOIT ESP32 DEVKIT V1 (In this case a Lonely Binary clone... but I assume they are all the same, right?)
// Analog Pins   (Left hand side if USB is at the bottom).
#define NOT_USED_EN                  0 
#define POT_BRIGHTNESS_PIN          36  //  VP GPIO 36 Input only  // Brightness potentiometer in
#define UNUSED39                    39  //  VN GPIO 39 Input only  
#define CLK_ROT_ENC                 34  //  GPIO 34 Input only // CLK_ROT_ENC Rotary encoder
#define DT_ROT_ENC                  35  //  GPIO 35 Input only // DT_ROT_ENC Rotary encoder
#define LEFT_OUT_LED_STRAND_PIN     32  //  TX2 GPIO 17  // Left channel data out pin to LEDs On VU stick [6]
#define RIGHT_OUT_LED_STRAND_PIN    33  //  RX2 GPIO 16  // Right channel data out pin to LEDs On VU stick [5]
#define LEFT_OUT_LED_MATRIX_PIN     25  //  GPIO 25 // Left channel data out pin to LED MATRIX [5]
#define RIGHT_OUT_LED_MATRIX_PIN    26  //  GPIO 26 // Right channel data out pin to LED MATRIX [5]
#define BTN_MODE_CHANGE_PIN         27  //  GPIO 27 //  modeBut input pin
#define GPIO_HPSI_CS                14  //  GPIO 14 // Slave SPI
#define GPIO_HPSI_MOSI              12  //  GPIO 12 // Slave SPI 
                                        // (must be LOW during boot == boot fail if GPIO 12 pulled high)
#define GPIO_HPSI_MISO              13  //  GPIO 13 // Slave SPI
#define NOT_USED_GND                0 
#define NOT_USED_VIN                0 

// Other side (Right hand side if USB is at the bottom).
#define GPIO_VPSI_MOSI              23  //  GPIO 23  // SI (Data in to 74HC595)
#define DIO_TM1637_DIGIT_DISPLAY    22  //  GPIO 22  // DIO
#define DONT_USE_1                  1   //  TX GPIO 1  UART TX debug output at boot (These are used by the the serial port (and effectively the USB).)
#define DONT_USE_3                  3   //  RX GPIO 3  UART RX HIGH at boot 
#define CLK_TM1637_DIGIT_DISPLAY    21  //  GPIO 22  // CLK
#define GPIO_VPSI_MISO              19  //  GPIO 19 // MISO analogDisplay
#define GPIO_VPSI_CLK               18  //  GPIO 18 // CLK -> SCK
#define GPIO_VPSI_CS                5   //  GPIO 5  // SS -> RCLK = CS/Latch // ( outputs PWM signal at boot)
#define DMX_TX                      17  //  TX2 GPIO 17 
#define DMX_RTS                     16  //  RX2 GPIO 16
#define GPIO_HPSI_HANDSHAKE         4   //  GPIO 2  // Slave SPI
#define DMX_RX                      2   //  GPIO 2 (LED)
#define GPIO_HPSI_SCLK              15  //  GPIO 15 // Slave SPI
#define NOT_USED_GND                0 
#define NOT_USED_3V3                0 
// END OF PINS


// Note: DO NOT USE:
// GPIO 6 (SCK/CLK)
// GPIO 7 (SDO/SD0)
// GPIO 8 (SDI/SD1)
// GPIO 9 (SHD/SD2)
// GPIO 10 (SWP/SD3)
// GPIO 11 (CSC/CMD)

// And now it looks like you can't use GPIO12 either!

// cf https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

/*
The ESP32 chip has the following strapping pins:
GPIO 0.
GPIO 2.
GPIO 4.
GPIO 5 (must be HIGH during boot)
GPIO 12 (must be LOW during boot)
GPIO 15 (must be HIGH during boot)

// So maybe these last 3 can be buttons but not much else, right?

// Pretty sure these are ok since we have not got the extra 4MB PSRAM on a plain old Esp32.
// GPIO 16 PSRAM for the WROVER
// GPIO 17 PSRAM for the WROVER

*/

// Also these are input ony which is fine as we are using thing them for
// analogue read. GPIO 34 GPIO 35 GPIO 36 GPIO 39

// ADC1 Channel Pin equiv
// #define ADC1_POT_BRIGHTNESS_PIN      ADC1_CHANNEL_0  // GPIO 36 // Brightness potentiometer in

#else
# error "MCU needs to be an Esp32"
#endif

