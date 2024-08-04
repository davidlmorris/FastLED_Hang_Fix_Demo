#include "debuggery.h"


/// @brief Private function for shared initialisation tasks.
/// @param bAllowColour true to allow, false to forbid.
void Debuggery_::_init(bool bAllowColour)
    {
    _bAllowColour = bAllowColour;
#ifdef ESP32 
    delay(1000); // Wait a sec for the serial port to catch up
#endif
    }


/// @brief Overloaded function to initialise Debuggery.
/// @param bAllowColour Allow colour is true.
/// Use false if in the Arduino ID as the serial monitor does not support colour.
/// USe true in the VSCODE IDE.
void Debuggery_::initialise(bool bAllowColour)
    {
    if (Serial)
        {
        Serial.flush();
        //Serial.end();
        }
    Serial.begin(115200);
    _init(bAllowColour);
    }


/// @brief Overloaded function to initialise Debuggery.
/// @param bAllowColour Allow colour is true.
/// Use false if in the Arduino ID as the serial monitor does not support colour.
/// USe true in the VSCODE IDE.
/// @param speed Sets the baud rate of the serial port
void Debuggery_::initialise(bool bAllowColour, unsigned long speed)
    {
    if (Serial)
        {
        Serial.flush();
        //Serial.end();
        }
    Serial.begin(speed);
    _init(bAllowColour);
    }


/// @brief Overloaded function to initialise Debuggery.
/// @param bAllowColour Allow colour is true.
/// Use false if in the Arduino ID as the serial monitor does not support colour.
/// USe true in the VSCODE IDE.
/// @param speed Bits per second (baud).
/// @param config Sets data, parity, and stop bits. Valid values are listed
/// at https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/
void Debuggery_::initialise(bool bAllowColour, unsigned long speed, uint32_t config)
    {
    if (Serial)
        {
        Serial.flush();
        //Serial.end();
        }
    Serial.begin(speed, config);
    _init(bAllowColour);
    }


/// @brief Overloaded operator to test if the serial port has started.  Returns true if the specified serial port is available.
/// noting that some boards will return true regardless.  See https://www.arduino.cc/reference/en/language/functions/communication/serial/ifserial/
Debuggery_::operator bool()
    {
    return(Serial);
    }


/// @brief Used in conjuction with the debug_assert macro.
/// @param func (May be NULL) the name of the function where the assert was called
/// @param file The name of the file where the assert was called
/// @param line The line number in the source file of the failed assertion.
/// @param failedExpr An expression that evaluates to nonzero (true) or 0 expressed as a string
void Debuggery_::__assert(const char* func, const char* file, int line, const char* failedExpr)
    {
    setColor(93, 41); // yellow fg, dull red bg
    print(F("Assertion failed: ("));
    print(failedExpr);
    if (func != NULL)
        {
        print(F("), function:"));
        print(func);
        }
    print(F(", file:"));
    print(file);
    print(F(", line:"));
    print(line);
    print(F("."));
    resetColour();
    println(F(""));
    flush(); // make sure it is all printed before we stop
    delay(100); // and some space just in case.
#ifdef ESP32 // esp32 keeps restarting on abort
    while (true);
#endif
    abort(); // end the program.
    }


/// @brief Displays the Board name, progName and a greeting 
/// usually used in setup when establishing the app as a start up message.
/// @param progName Name of the application
/// @param greeting A greeting or version number as a char*.
void Debuggery_::progAnnounce(const char* progName, const char* greeting)
    {
    _progAnnounce(progName, greeting);
    }



/// @brief Displays the  Board name, progName and a greeting as a hello message
/// usually used in setup when establishing the app as a start up message.
/// @param progName A string expressing the name of the program.
void Debuggery_::progAnnounce(const char* progName)
    {
    _progAnnounce(progName, NULL);
    }


void Debuggery_::_progAnnounce(const char* progName, const char* greeting)
    {
    setColour(94); // bright blue
    print(progName);
    resetColour();
    if (greeting == NULL)
        {
        println(F(": 'Hello!'"));
        }
    else
        {
        print(F(" "));
        print(greeting);
        println(F(""));
        }
    setColour(95); // bright magenta
    print(F("BOARD_NAME selected is "));
    setColour(96); // bright cyan
    print(BOARD_NAME);
    setColour(95); // bright magenta
    println(".");
    resetColour();
    }

/// @brief When inserted (once only) in a loop as the last item, will report 
/// the number of loops per second displaying every 'reportEvery' number of seconds.
/// @param reportEvery const uint8_t Frequency of a 'report display' in seconds.
void Debuggery_::speedTest(const uint8_t reportEvery)
    {
    speedTest(reportEvery, "");
    }

void Debuggery_::speedTest(const uint8_t reportEvery, const char * extraText)
    {
    speedTest(reportEvery, extraText, "");
    }

void Debuggery_::speedTest(const uint8_t reportEvery, const char * extraText, const char * moreExtraText)
    {
    static unsigned long timeReport = 0;
    static unsigned long loopCount = 0;
    static unsigned long loopReportCount = 0;
    static unsigned long loopTime = 0;

    unsigned long nowTime = millis();
    loopCount++; // since we have just completed one loop, since we are the last item in that loop.
    if ((loopTime + 1000) < nowTime)
        {
        loopTime = nowTime;
        loopReportCount = loopReportCount + loopCount;
        if ((timeReport + (reportEvery * 1000)) < nowTime)
            {
            loopReportCount = loopReportCount / reportEvery;
            print(F("Loop counts per second: "));
            print(loopReportCount);
            print(F(" (reported every "));
            print(reportEvery);
            print(F(" secs)"));
            print(extraText);
            print(moreExtraText);
            println(F("."));
            loopReportCount = 0;
            timeReport = nowTime;
            }
        loopCount = 0;
        }
    }


/// @brief Write a byte to the the serial port
/// @param val byte to send
/// @return size_t number of bytes written.
size_t Debuggery_::write(uint8_t val)
    {
    return(Serial.write(val));
    // TODO It might be faster to go directly to softwareSerial.
    }


/// @brief Changes the serial monitor display colour.
/// @param colour colour to use (see https://en.wikipedia.org/wiki/ANSI_escape_code).
/// @note FG    Colour Name
/// @note 30 	Black 
/// @note 31 	Red 
/// @note 32 	Green 
/// @note 33 	Yellow
/// @note 34 	Blue 
/// @note 35 	Magenta
/// @note 36 	Cyan 	
/// @note 37 	White 	
/// @note 90 	Bright Black (Gray) 	
/// @note 91 	Bright Red 	
/// @note 92 	Bright Green
/// @note 93 	Bright Yellow
/// @note 94 	Bright Blue 
/// @note 95 	Bright Magenta
/// @note 96 	Bright Cyan 
/// @note 97 	Bright White     
void Debuggery_::setColour(uint8_t colour)
    {
    if (_bAllowColour)
        {
        print("\x1B[");
        print(colour);
        print("m");
        }
    }


/// @brief Changes the serial monitor foreground and background display colour.
///  (see https://en.wikipedia.org/wiki/ANSI_escape_code) for the colours.
/// @note FG  BG      Colour Name
/// @note 30 	40 	    Black 
/// @note 31 	41 	    Red 
/// @note 32 	42 	    Green 
/// @note 33 	43 	    Yellow
/// @note 34 	44 	    Blue 
/// @note 35 	45 	    Magenta
/// @note 36 	46 	    Cyan 	
/// @note 37 	47 	    White 	
/// @note 90 	100 	Bright Black (Gray) 	
/// @note 91 	101 	Bright Red 	
/// @note 92 	102 	Bright Green
/// @note 93 	103 	Bright Yellow
/// @note 94 	104 	Bright Blue 
/// @note 95 	105 	Bright Magenta
/// @note 96 	106 	Bright Cyan 
/// @note 97 	107 	Bright White     
/// @param fgColour foreground display colour.
/// @param bgColour background display colour.
void Debuggery_::setColour(uint8_t fgColour, uint8_t bgColour)
    {
    if (_bAllowColour)
        {
        print("\x1B[");
        print(fgColour);
        print(";");
        print(bgColour);
        print("m");
        }
    }


/// @brief Resets the colour to the default value
void Debuggery_::resetColour(void)
    {
    if (_bAllowColour)
        {
        // Serial.print("\x1B[0m");
        print("\x1B[39;49m");
        }
    }


/// @brief Object to provided Debug information printing in exactly the same
/// was at the serial port (with the same print functions).
Debuggery_ Debuggery;
