#ifndef _DEBUG_CONDITIONALS_H_
# define _DEBUG_CONDITIONALS_H_

#include <Arduino.h>

// Why is this file here:
// We want to have an independent file we can use for different apps
// and for different files including other cpp file in the same app.
// We define DEBUG_ON to true to show 'we are debugging, and what we are debugging'
// defines which may be different in each app so we can application independence.

// Why not just in the main INO file?  Basically because while other INO
// files are concatenated with the main INO file before preprocessing
// any C++ file.
// Some of the issues are 10 years old... or more.
// cf https://github.com/arduino/Arduino/issues/1841
// and here a solution of sorts
//  http://www.gammon.com.au/forum/?id=12625
// which I am sorely tempted to follow!

// DEBUG defines 
// This is the key one:

// ************************* here **************************
# define DEBUG_ON true // comment this out to remove debuggery altogether.


# ifdef NDEBUG
#  undef DEBUG_ON
# endif

extern const unsigned long xTickATinyBit;
extern const unsigned long xTickHalfASec;
extern const unsigned long xTickFullSec;
extern const unsigned long xTickFiveSecs;

# ifdef DEBUG_ON
# define FASTLED_RMT_SERIAL_DEBUG  1

// Debugging defines:
# define NUMBER_BUFFER 33
extern  void debugDisplaySeconds(const char* sMessage, uint32_t seconds_run_time);

# define DEBUG_FASTLED_JAM true
# define DEBUG_UPDATE_FREQUENCY false  // Show number of loops per second every minute.

#  include <debuggery.h>
extern void initSemaphore(void);
extern bool takeSemaphore(void);
extern void giveSemaphore(void);

#  define DEBUG_INIT_SEMAPHORE              initSemaphore()
#  define DEBUG_START_SEMAPHORE_BLOCK       if(takeSemaphore())
#  define DEBUG_SEMAPHORE_RELEASE           giveSemaphore()
#  define DEBUG_DELAY(y)                    vTaskDelay(y)
# else
# define FASTLED_RMT_SERIAL_DEBUG  0
#  include <not_debuggery.h>
#  define DEBUG_INIT_SEMAPHORE              ((void)0)
#  define DEBUG_START_SEMAPHORE_BLOCK       ((void)0);  // note the semi-colon is require here as we have statement not an if.
#  define DEBUG_SEMAPHORE_RELEASE           ((void)0)
#  define DEBUG_DELAY(y)                    ((void)0)
# endif



#endif /* _DEBUG_CONDITIONALS_H_ */
