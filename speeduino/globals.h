#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include "table.h"
#include <assert.h>
#include "logger.h"

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
  #define BOARD_MAX_DIGITAL_PINS 54 //digital pins +1
  #define BOARD_MAX_IO_PINS 70 //digital pins + analog channels + 1
#ifndef LED_BUILTIN
  #define LED_BUILTIN 13
#endif
  #define CORE_AVR
  #define BOARD_H "board_avr2560.h"
  #define INJ_CHANNELS 4
  #define IGN_CHANNELS 5

  #if defined(__AVR_ATmega2561__)
    //This is a workaround to avoid having to change all the references to higher ADC channels. We simply define the channels (Which don't exist on the 2561) as being the same as A0-A7
    //These Analog inputs should never be used on any 2561 board defintion (Because they don't exist on the MCU), so it will not cause any isses
    #define A8  A0
    #define A9  A1
    #define A10  A2
    #define A11  A3
    #define A12  A4
    #define A13  A5
    #define A14  A6
    #define A15  A7
  #endif

  //#define TIMER5_MICROS

#elif defined(CORE_TEENSY)
  #if defined(__MK64FX512__) || defined(__MK66FX1M0__)
    #define CORE_TEENSY35
    #define BOARD_H "board_teensy35.h"
    #define SD_LOGGING //SD logging enabled by default for Teensy 3.5 as it has the slot built in
  #elif defined(__IMXRT1062__)
    #define CORE_TEENSY41
    #define BOARD_H "board_teensy41.h"
  #endif
  #define INJ_CHANNELS 8
  #define IGN_CHANNELS 8

#elif defined(STM32_MCU_SERIES) || defined(ARDUINO_ARCH_STM32) || defined(STM32)
  #define CORE_STM32
  #if defined(STM32F407xx) //F407 can do 8x8 STM32F401/STM32F411 not
   #define INJ_CHANNELS 8
   #define IGN_CHANNELS 8
  #else
   #define INJ_CHANNELS 4
   #define IGN_CHANNELS 5
  #endif

//Select one for EEPROM,the default is EEPROM emulation on internal flash.
//#define SRAM_AS_EEPROM /*Use 4K battery backed SRAM, requires a 3V continuous source (like battery) connected to Vbat pin */
//#define USE_SPI_EEPROM PB0 /*Use M25Qxx SPI flash */
//#define FRAM_AS_EEPROM /*Use FRAM like FM25xxx, MB85RSxxx or any SPI compatible */

  #ifndef word
    #define word(h, l) ((h << 8) | l) //word() function not defined for this platform in the main library
  #endif
  
  
  #if defined(ARDUINO_BLUEPILL_F103C8) || defined(ARDUINO_BLUEPILL_F103CB) \
   || defined(ARDUINO_BLACKPILL_F401CC) || defined(ARDUINO_BLACKPILL_F411CE)
    //STM32 Pill boards
    #ifndef NUM_DIGITAL_PINS
      #define NUM_DIGITAL_PINS 35
    #endif
    #ifndef LED_BUILTIN
      #define LED_BUILTIN PB1 //Maple Mini
    #endif
  #elif defined(STM32F407xx)
    #ifndef NUM_DIGITAL_PINS
      #define NUM_DIGITAL_PINS 75
    #endif
  #endif

  #if defined(STM32_CORE_VERSION)
    #define BOARD_H "board_stm32_official.h"
  #else
    #define CORE_STM32_GENERIC
    #define BOARD_H "board_stm32_generic.h"
  #endif

  //Specific mode for Bluepill due to its small flash size. This disables a number of strings from being compiled into the flash
  #if defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB)
    #define SMALL_FLASH_MODE
  #endif

  #define BOARD_MAX_DIGITAL_PINS NUM_DIGITAL_PINS
  #define BOARD_MAX_IO_PINS NUM_DIGITAL_PINS
  #if __GNUC__ < 7 //Already included on GCC 7
  extern "C" char* sbrk(int incr); //Used to freeRam
  #endif
  #ifndef digitalPinToInterrupt
  inline uint32_t  digitalPinToInterrupt(uint32_t Interrupt_pin) { return Interrupt_pin; } //This isn't included in the stm32duino libs (yet)
  #endif
#elif defined(__SAMD21G18A__)
  #define BOARD_H "board_samd21.h"
  #define CORE_SAMD21
#else
  #error Incorrect board selected. Please select the correct board (Usually Mega 2560) and upload again
#endif

//This can only be included after the above section
#include BOARD_H //Note that this is not a real file, it is defined in globals.h. 

//Handy bitsetting macros
#define BIT_SET(a,b) ((a) |= (1U<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1U<<(b)))
#define BIT_CHECK(var,pos) !!((var) & (1U<<(pos)))
#define BIT_TOGGLE(var,pos) ((var)^= 1UL << (pos))

#define interruptSafe(c) (noInterrupts(); {c} interrupts();) //Wraps any code between nointerrupt and interrupt calls

#define MS_IN_MINUTE 60000
#define US_IN_MINUTE 60000000

//Define the load algorithm
#define LOAD_SOURCE_MAP         0
#define LOAD_SOURCE_TPS         1
#define LOAD_SOURCE_IMAPEMAP    2

//Define bit positions within engine virable
#define BIT_ENGINE_RUN      0   // Engine running
#define BIT_ENGINE_CRANK    1   // Engine cranking
#define BIT_ENGINE_ASE      2   // after start enrichment (ASE)
#define BIT_ENGINE_WARMUP   3   // Engine in warmup
#define BIT_ENGINE_ACC      4   // in acceleration mode (TPS accel)
#define BIT_ENGINE_DCC      5   // in deceleration mode
#define BIT_ENGINE_MAPACC   6   // MAP acceleration mode
#define BIT_ENGINE_MAPDCC   7   // MAP decelleration mode

//Define masks for Status1
#define BIT_STATUS1_INJ1           0  //inj1
#define BIT_STATUS1_INJ2           1  //inj2
#define BIT_STATUS1_INJ3           2  //inj3
#define BIT_STATUS1_INJ4           3  //inj4
#define BIT_STATUS1_DFCO           4  //Decelleration fuel cutoff
#define BIT_STATUS1_BOOSTCUT       5  //Fuel component of MAP based boost cut out
#define BIT_STATUS1_TOOTHLOG1READY 6  //Used to flag if tooth log 1 is ready
#define BIT_STATUS1_TOOTHLOG2READY 7  //Used to flag if tooth log 2 is ready (Log is not currently used)

//Define masks for spark variable
#define BIT_SPARK_HLAUNCH         0  //Hard Launch indicator
#define BIT_SPARK_SLAUNCH         1  //Soft Launch indicator
#define BIT_SPARK_HRDLIM          2  //Hard limiter indicator
#define BIT_SPARK_SFTLIM          3  //Soft limiter indicator
#define BIT_SPARK_BOOSTCUT        4  //Spark component of MAP based boost cut out
#define BIT_SPARK_ERROR           5  // Error is detected
#define BIT_SPARK_IDLE            6  // idle on
#define BIT_SPARK_SYNC            7  // Whether engine has sync or not

#define BIT_SPARK2_FLATSH         0  //Flat shift hard cut
#define BIT_SPARK2_FLATSS         1  //Flat shift soft cut
#define BIT_SPARK2_SPARK2_ACTIVE  2
#define BIT_SPARK2_UNUSED4        3
#define BIT_SPARK2_UNUSED5        4
#define BIT_SPARK2_UNUSED6        5
#define BIT_SPARK2_UNUSED7        6
#define BIT_SPARK2_UNUSED8        7

#define BIT_TIMER_1HZ             0
#define BIT_TIMER_4HZ             1
#define BIT_TIMER_10HZ            2
#define BIT_TIMER_15HZ            3
#define BIT_TIMER_30HZ            4

#define BIT_STATUS3_RESET_PREVENT 0 //Indicates whether reset prevention is enabled
#define BIT_STATUS3_NITROUS       1
#define BIT_STATUS3_FUEL2_ACTIVE  2
#define BIT_STATUS3_VSS_REFRESH   3
#define BIT_STATUS3_HALFSYNC      4 //shows if there is only sync from primary trigger, but not from secondary.
#define BIT_STATUS3_NSQUIRTS1     5
#define BIT_STATUS3_NSQUIRTS2     6
#define BIT_STATUS3_NSQUIRTS3     7

#define VALID_MAP_MAX 1022 //The largest ADC value that is valid for the MAP sensor
#define VALID_MAP_MIN 2 //The smallest ADC value that is valid for the MAP sensor

#ifndef UNIT_TEST 
#define TOOTH_LOG_SIZE      127
#define TOOTH_LOG_BUFFER    128 //256
#else
#define TOOTH_LOG_SIZE      1
#define TOOTH_LOG_BUFFER    1 //256
#endif

#define COMPOSITE_LOG_PRI   0
#define COMPOSITE_LOG_SEC   1
#define COMPOSITE_LOG_TRIG  2
#define COMPOSITE_LOG_SYNC  3

#define INJ_PAIRED          0
#define INJ_SEMISEQUENTIAL  1
#define INJ_BANKED          2
#define INJ_SEQUENTIAL      3

#define OUTPUT_CONTROL_DIRECT   0
#define OUTPUT_CONTROL_MC33810  10

#define IGN_MODE_WASTED     0
#define IGN_MODE_SINGLE     1
#define IGN_MODE_WASTEDCOP  2
#define IGN_MODE_SEQUENTIAL 3
#define IGN_MODE_ROTARY     4

#define SEC_TRIGGER_SINGLE  0
#define SEC_TRIGGER_4_1     1

#define ROTARY_IGN_FC       0
#define ROTARY_IGN_FD       1
#define ROTARY_IGN_RX8      2

#define BOOST_MODE_SIMPLE   0
#define BOOST_MODE_FULL     1

#define WMI_MODE_SIMPLE       0
#define WMI_MODE_PROPORTIONAL 1
#define WMI_MODE_OPENLOOP     2
#define WMI_MODE_CLOSEDLOOP   3

#define HARD_CUT_FULL       0
#define HARD_CUT_ROLLING    1

#define SIZE_BYTE           8
#define SIZE_INT            16

#define EVEN_FIRE           0
#define ODD_FIRE            1

#define EGO_ALGORITHM_SIMPLE  0
#define EGO_ALGORITHM_PID     2

#define STAGING_MODE_TABLE  0
#define STAGING_MODE_AUTO   1

#define NITROUS_OFF         0
#define NITROUS_STAGE1      1
#define NITROUS_STAGE2      2
#define NITROUS_BOTH        3

#define PROTECT_CUT_OFF     0
#define PROTECT_CUT_IGN     1
#define PROTECT_CUT_FUEL    2
#define PROTECT_CUT_BOTH    3
#define PROTECT_IO_ERROR    7

#define AE_MODE_TPS         0
#define AE_MODE_MAP         1

#define AE_MODE_MULTIPLIER  0
#define AE_MODE_ADDER       1

#define KNOCK_MODE_OFF      0
#define KNOCK_MODE_DIGITAL  1
#define KNOCK_MODE_ANALOG   2

#define FUEL2_MODE_OFF      0
#define FUEL2_MODE_MULTIPLY 1
#define FUEL2_MODE_ADD      2
#define FUEL2_MODE_CONDITIONAL_SWITCH   3
#define FUEL2_MODE_INPUT_SWITCH 4

#define SPARK2_MODE_OFF      0
#define SPARK2_MODE_MULTIPLY 1
#define SPARK2_MODE_ADD      2
#define SPARK2_MODE_CONDITIONAL_SWITCH   3
#define SPARK2_MODE_INPUT_SWITCH 4

#define FUEL2_CONDITION_RPM 0
#define FUEL2_CONDITION_MAP 1
#define FUEL2_CONDITION_TPS 2
#define FUEL2_CONDITION_ETH 3

#define SPARK2_CONDITION_RPM 0
#define SPARK2_CONDITION_MAP 1
#define SPARK2_CONDITION_TPS 2
#define SPARK2_CONDITION_ETH 3

#define RESET_CONTROL_DISABLED             0
#define RESET_CONTROL_PREVENT_WHEN_RUNNING 1
#define RESET_CONTROL_PREVENT_ALWAYS       2
#define RESET_CONTROL_SERIAL_COMMAND       3

#define OPEN_LOOP_BOOST     0
#define CLOSED_LOOP_BOOST   1

#define SOFT_LIMIT_FIXED        0
#define SOFT_LIMIT_RELATIVE     1

#define VVT_MODE_ONOFF      0
#define VVT_MODE_OPEN_LOOP  1
#define VVT_MODE_CLOSED_LOOP 2
#define VVT_LOAD_MAP      0
#define VVT_LOAD_TPS      1

#define MULTIPLY_MAP_MODE_OFF   0
#define MULTIPLY_MAP_MODE_BARO  1
#define MULTIPLY_MAP_MODE_100   2

#define FOUR_STROKE         0
#define TWO_STROKE          1

#define GOING_LOW         0
#define GOING_HIGH        1

#define MAX_RPM 18000 //This is the maximum rpm that the ECU will attempt to run at. It is NOT related to the rev limiter, but is instead dictates how fast certain operations will be allowed to run. Lower number gives better performance

#define BATTV_COR_MODE_WHOLE 0
#define BATTV_COR_MODE_OPENTIME 1

#define INJ1_CMD_BIT      0
#define INJ2_CMD_BIT      1
#define INJ3_CMD_BIT      2
#define INJ4_CMD_BIT      3
#define INJ5_CMD_BIT      4
#define INJ6_CMD_BIT      5
#define INJ7_CMD_BIT      6
#define INJ8_CMD_BIT      7

#define IGN1_CMD_BIT      0
#define IGN2_CMD_BIT      1
#define IGN3_CMD_BIT      2
#define IGN4_CMD_BIT      3
#define IGN5_CMD_BIT      4
#define IGN6_CMD_BIT      5
#define IGN7_CMD_BIT      6
#define IGN8_CMD_BIT      7

#define ENGINE_PROTECT_BIT_RPM  0
#define ENGINE_PROTECT_BIT_MAP  1
#define ENGINE_PROTECT_BIT_OIL  2
#define ENGINE_PROTECT_BIT_AFR  3

//Table sizes
#define CALIBRATION_TABLE_SIZE 512
#define CALIBRATION_TEMPERATURE_OFFSET 40 // All temperature measurements are stored offset by 40 degrees. This is so we can use an unsigned byte (0-255) to represent temperature ranges from -40 to 215
#define OFFSET_FUELTRIM 127 //The fuel trim tables are offset by 128 to allow for -128 to +128 values
#define OFFSET_IGNITION 40 //Ignition values from the main spark table are offset 40 degrees downards to allow for negative spark timing

#define SERIAL_BUFFER_THRESHOLD 32 // When the serial buffer is filled to greater than this threshold value, the serial processing operations will be performed more urgently in order to avoid it overflowing. Serial buffer is 64 bytes long, so the threshold is set at half this as a reasonable figure

#ifndef CORE_TEENSY41
  #define FUEL_PUMP_ON() *pump_pin_port |= (pump_pin_mask)
  #define FUEL_PUMP_OFF() *pump_pin_port &= ~(pump_pin_mask)
#else
  //Special compatibility case for TEENSY 41 (for now)
  #define FUEL_PUMP_ON() digitalWrite(pinFuelPump, HIGH);
  #define FUEL_PUMP_OFF() digitalWrite(pinFuelPump, LOW);
#endif

extern const char TSfirmwareVersion[] PROGMEM;

extern const byte data_structure_version; //This identifies the data structure when reading / writing.
#define NUM_PAGES     15
extern const uint16_t npage_size[NUM_PAGES]; /**< This array stores the size (in bytes) of each configuration page */
#define MAP_PAGE_SIZE 288

extern struct table3D fuelTable; //16x16 fuel map
extern struct table3D fuelTable2; //16x16 fuel map
extern struct table3D ignitionTable; //16x16 ignition map
extern struct table3D ignitionTable2; //16x16 ignition map
extern struct table3D afrTable; //16x16 afr target map
extern struct table3D stagingTable; //8x8 fuel staging table
extern struct table3D boostTable; //8x8 boost map
extern struct table3D vvtTable; //8x8 vvt map
extern struct table3D wmiTable; //8x8 wmi map
extern struct table3D trim1Table; //6x6 Fuel trim 1 map
extern struct table3D trim2Table; //6x6 Fuel trim 2 map
extern struct table3D trim3Table; //6x6 Fuel trim 3 map
extern struct table3D trim4Table; //6x6 Fuel trim 4 map
extern struct table3D dwellTable; //4x4 Dwell map
extern struct table2D taeTable; //4 bin TPS Acceleration Enrichment map (2D)
extern struct table2D maeTable;
extern struct table2D WUETable; //10 bin Warm Up Enrichment map (2D)
extern struct table2D ASETable; //4 bin After Start Enrichment map (2D)
extern struct table2D ASECountTable; //4 bin After Start duration map (2D)
extern struct table2D PrimingPulseTable; //4 bin Priming pulsewidth map (2D)
extern struct table2D crankingEnrichTable; //4 bin cranking Enrichment map (2D)
extern struct table2D dwellVCorrectionTable; //6 bin dwell voltage correction (2D)
extern struct table2D injectorVCorrectionTable; //6 bin injector voltage correction (2D)
extern struct table2D injectorAngleTable; //4 bin injector timing curve (2D)
extern struct table2D IATDensityCorrectionTable; //9 bin inlet air temperature density correction (2D)
extern struct table2D baroFuelTable; //8 bin baro correction curve (2D)
extern struct table2D IATRetardTable; //6 bin ignition adjustment based on inlet air temperature  (2D)
extern struct table2D idleTargetTable; //10 bin idle target table for idle timing (2D)
extern struct table2D idleAdvanceTable; //6 bin idle advance adjustment table based on RPM difference  (2D)
extern struct table2D CLTAdvanceTable; //6 bin ignition adjustment based on coolant temperature  (2D)
extern struct table2D rotarySplitTable; //8 bin ignition split curve for rotary leading/trailing  (2D)
extern struct table2D flexFuelTable;  //6 bin flex fuel correction table for fuel adjustments (2D)
extern struct table2D flexAdvTable;   //6 bin flex fuel correction table for timing advance (2D)
extern struct table2D flexBoostTable; //6 bin flex fuel correction table for boost adjustments (2D)
extern struct table2D fuelTempTable;  //6 bin fuel temperature correction table for fuel adjustments (2D)
extern struct table2D knockWindowStartTable;
extern struct table2D knockWindowDurationTable;
extern struct table2D oilPressureProtectTable;
extern struct table2D wmiAdvTable; //6 bin wmi correction table for timing advance (2D)

//These are for the direct port manipulation of the injectors, coils and aux outputs
extern volatile PORT_TYPE *inj1_pin_port;
extern volatile PINMASK_TYPE inj1_pin_mask;
extern volatile PORT_TYPE *inj2_pin_port;
extern volatile PINMASK_TYPE inj2_pin_mask;
extern volatile PORT_TYPE *inj3_pin_port;
extern volatile PINMASK_TYPE inj3_pin_mask;
extern volatile PORT_TYPE *inj4_pin_port;
extern volatile PINMASK_TYPE inj4_pin_mask;
extern volatile PORT_TYPE *inj5_pin_port;
extern volatile PINMASK_TYPE inj5_pin_mask;
extern volatile PORT_TYPE *inj6_pin_port;
extern volatile PINMASK_TYPE inj6_pin_mask;
extern volatile PORT_TYPE *inj7_pin_port;
extern volatile PINMASK_TYPE inj7_pin_mask;
extern volatile PORT_TYPE *inj8_pin_port;
extern volatile PINMASK_TYPE inj8_pin_mask;

extern volatile PORT_TYPE *ign1_pin_port;
extern volatile PINMASK_TYPE ign1_pin_mask;
extern volatile PORT_TYPE *ign2_pin_port;
extern volatile PINMASK_TYPE ign2_pin_mask;
extern volatile PORT_TYPE *ign3_pin_port;
extern volatile PINMASK_TYPE ign3_pin_mask;
extern volatile PORT_TYPE *ign4_pin_port;
extern volatile PINMASK_TYPE ign4_pin_mask;
extern volatile PORT_TYPE *ign5_pin_port;
extern volatile PINMASK_TYPE ign5_pin_mask;
extern volatile PORT_TYPE *ign6_pin_port;
extern volatile PINMASK_TYPE ign6_pin_mask;
extern volatile PORT_TYPE *ign7_pin_port;
extern volatile PINMASK_TYPE ign7_pin_mask;
extern volatile PORT_TYPE *ign8_pin_port;
extern volatile PINMASK_TYPE ign8_pin_mask;

extern volatile PORT_TYPE *tach_pin_port;
extern volatile PINMASK_TYPE tach_pin_mask;
extern volatile PORT_TYPE *pump_pin_port;
extern volatile PINMASK_TYPE pump_pin_mask;

extern volatile PORT_TYPE *flex_pin_port;
extern volatile PINMASK_TYPE flex_pin_mask;

extern volatile PORT_TYPE *triggerPri_pin_port;
extern volatile PINMASK_TYPE triggerPri_pin_mask;
extern volatile PORT_TYPE *triggerSec_pin_port;
extern volatile PINMASK_TYPE triggerSec_pin_mask;

//These need to be here as they are used in both speeduino.ino and scheduler.ino
extern bool channel1InjEnabled;
extern bool channel2InjEnabled;
extern bool channel3InjEnabled;
extern bool channel4InjEnabled;
extern bool channel5InjEnabled;
extern bool channel6InjEnabled;
extern bool channel7InjEnabled;
extern bool channel8InjEnabled;

extern int ignition1EndAngle;
extern int ignition2EndAngle;
extern int ignition3EndAngle;
extern int ignition4EndAngle;
extern int ignition5EndAngle;
extern int ignition6EndAngle;
extern int ignition7EndAngle;
extern int ignition8EndAngle;

extern int ignition1StartAngle;
extern int ignition2StartAngle;
extern int ignition3StartAngle;
extern int ignition4StartAngle;
extern int ignition5StartAngle;
extern int ignition6StartAngle;
extern int ignition7StartAngle;
extern int ignition8StartAngle;

//These are variables used across multiple files
extern const byte PROGMEM fsIntIndex[31];
extern bool initialisationComplete; //Tracks whether the setup() function has run completely
extern byte fpPrimeTime; //The time (in seconds, based on currentStatus.secl) that the fuel pump started priming
extern volatile uint16_t mainLoopCount;
extern unsigned long revolutionTime; //The time in uS that one revolution would take at current speed (The time tooth 1 was last seen, minus the time it was seen prior to that)
extern volatile unsigned long timer5_overflow_count; //Increments every time counter 5 overflows. Used for the fast version of micros()
extern volatile unsigned long ms_counter; //A counter that increments once per ms
extern uint16_t fixedCrankingOverride;
extern bool clutchTrigger;
extern bool previousClutchTrigger;
extern volatile uint32_t toothHistory[TOOTH_LOG_BUFFER];
extern volatile uint8_t compositeLogHistory[TOOTH_LOG_BUFFER];
extern volatile bool fpPrimed; //Tracks whether or not the fuel pump priming has been completed yet
extern volatile unsigned int toothHistoryIndex;
extern volatile byte toothHistorySerialIndex;
extern unsigned long currentLoopTime; /**< The time (in uS) that the current mainloop started */
extern unsigned long previousLoopTime; /**< The time (in uS) that the previous mainloop started */
extern volatile uint16_t ignitionCount; /**< The count of ignition events that have taken place since the engine started */
extern byte primaryTriggerEdge;
extern byte secondaryTriggerEdge;
extern int CRANK_ANGLE_MAX;
extern int CRANK_ANGLE_MAX_IGN;
extern int CRANK_ANGLE_MAX_INJ; //The number of crank degrees that the system track over. 360 for wasted / timed batch and 720 for sequential
extern volatile uint32_t runSecsX10; /**< Counter of seconds since cranking commenced (similar to runSecs) but in increments of 0.1 seconds */
extern volatile uint32_t seclx10; /**< Counter of seconds since powered commenced (similar to secl) but in increments of 0.1 seconds */
extern volatile byte HWTest_INJ; /**< Each bit in this variable represents one of the injector channels and it's HW test status */
extern volatile byte HWTest_INJ_50pc; /**< Each bit in this variable represents one of the injector channels and it's 50% HW test status */
extern volatile byte HWTest_IGN; /**< Each bit in this variable represents one of the ignition channels and it's HW test status */
extern volatile byte HWTest_IGN_50pc; /**< Each bit in this variable represents one of the ignition channels and it's 50% HW test status */

//This needs to be here because using the config page directly can prevent burning the setting
extern byte resetControl;

extern volatile byte TIMER_mask;
extern volatile byte LOOP_TIMER;

//These functions all do checks on a pin to determine if it is already in use by another (higher importance) function
#define pinIsInjector(pin)  ( ((pin) == pinInjector1) || ((pin) == pinInjector2) || ((pin) == pinInjector3) || ((pin) == pinInjector4) || ((pin) == pinInjector5) || ((pin) == pinInjector6) || ((pin) == pinInjector7) || ((pin) == pinInjector8) )
#define pinIsIgnition(pin)  ( ((pin) == pinCoil1) || ((pin) == pinCoil2) || ((pin) == pinCoil3) || ((pin) == pinCoil4) || ((pin) == pinCoil5) || ((pin) == pinCoil6) || ((pin) == pinCoil7) || ((pin) == pinCoil8) )
#define pinIsSensor(pin)    ( ((pin) == pinCLT) || ((pin) == pinIAT) || ((pin) == pinMAP) || ((pin) == pinTPS) || ((pin) == pinO2) || ((pin) == pinBat) )
#define pinIsOutput(pin)    ( ((pin) == pinFuelPump) || ((pin) == pinFan) || ((pin) == pinVVT_1) || ((pin) == pinVVT_2) || ((pin) == pinBoost) || ((pin) == pinIdle1) || ((pin) == pinIdle2) || ((pin) == pinTachOut) )
#define pinIsUsed(pin)      ( pinIsInjector((pin)) || pinIsIgnition((pin)) || pinIsSensor((pin)) || pinIsOutput((pin)) || pinIsReserved((pin)) )

//The status struct contains the current values for all 'live' variables
//In current version this is 64 bytes
struct statuses {
  volatile bool hasSync;
  uint16_t RPM;
  byte RPMdiv100;
  long longRPM;
  int mapADC;
  int baroADC;
  long MAP; //Has to be a long for PID calcs (Boost control)
  int16_t EMAP;
  int16_t EMAPADC;
  byte baro; //Barometric pressure is simply the inital MAP reading, taken before the engine is running. Alternatively, can be taken from an external sensor
  byte TPS; /**< The current TPS reading (0% - 100%). Is the tpsADC value after the calibration is applied */
  byte tpsADC; /**< 0-255 byte representation of the TPS. Downsampled from the original 10-bit reading, but before any calibration is applied */
  byte tpsDOT; /**< TPS delta over time. Measures the % per second that the TPS is changing. Value is divided by 10 to be stored in a byte */
  byte mapDOT; /**< MAP delta over time. Measures the kpa per second that the MAP is changing. Value is divided by 10 to be stored in a byte */
  volatile int rpmDOT;
  byte VE; /**< The current VE value being used in the fuel calculation. Can be the same as VE1 or VE2, or a calculated value of both */
  byte VE1; /**< The VE value from fuel table 1 */
  byte VE2; /**< The VE value from fuel table 2, if in use (and required conditions are met) */
  byte O2;
  byte O2_2;
  int coolant;
  int cltADC;
  int IAT;
  int iatADC;
  int batADC;
  int O2ADC;
  int O2_2ADC;
  int dwell;
  byte dwellCorrection; /**< The amount of correction being applied to the dwell time. */
  byte battery10; /**< The current BRV in volts (multiplied by 10. Eg 12.5V = 125) */
  int8_t advance; /**< The current advance value being used in the spark calculation. Can be the same as advance1 or advance2, or a calculated value of both */
  int8_t advance1; /**< The advance value from ignition table 1 */
  int8_t advance2; /**< The advance value from ignition table 2 */
  uint16_t corrections; /**< The total current corrections % amount */
  uint16_t AEamount; /**< The amount of accleration enrichment currently being applied. 100=No change. Varies above 255 */
  byte egoCorrection; /**< The amount of closed loop AFR enrichment currently being applied */
  byte wueCorrection; /**< The amount of warmup enrichment currently being applied */
  byte batCorrection; /**< The amount of battery voltage enrichment currently being applied */
  byte iatCorrection; /**< The amount of inlet air temperature adjustment currently being applied */
  byte baroCorrection; /**< The amount of correction being applied for the current baro reading */
  byte launchCorrection; /**< The amount of correction being applied if launch control is active */
  byte flexCorrection; /**< Amount of correction being applied to compensate for ethanol content */
  byte fuelTempCorrection; /**< Amount of correction being applied to compensate for fuel temperature */
  int8_t flexIgnCorrection; /**< Amount of additional advance being applied based on flex. Note the type as this allows for negative values */
  byte afrTarget;
  byte idleDuty; /**< The current idle duty cycle amount if PWM idle is selected and active */
  byte CLIdleTarget; /**< The target idle RPM (when closed loop idle control is active) */
  bool idleUpActive; /**< Whether the externally controlled idle up is currently active */
  bool CTPSActive; /**< Whether the externally controlled closed throttle position sensor is currently active */
  bool fanOn; /**< Whether or not the fan is turned on */
  volatile byte ethanolPct; /**< Ethanol reading (if enabled). 0 = No ethanol, 100 = pure ethanol. Eg E85 = 85. */
  volatile int8_t fuelTemp;
  unsigned long AEEndTime; /**< The target end time used whenever AE is turned on */
  volatile byte status1;
  volatile byte spark;
  volatile byte spark2;
  uint8_t engine;
  unsigned int PW1; //In uS
  unsigned int PW2; //In uS
  unsigned int PW3; //In uS
  unsigned int PW4; //In uS
  unsigned int PW5; //In uS
  unsigned int PW6; //In uS
  unsigned int PW7; //In uS
  unsigned int PW8; //In uS
  volatile byte runSecs; /**< Counter of seconds since cranking commenced (Maxes out at 255 to prevent overflow) */
  volatile byte secl; /**< Counter incrementing once per second. Will overflow after 255 and begin again. This is used by TunerStudio to maintain comms sync */
  volatile uint32_t loopsPerSecond; /**< A performance indicator showing the number of main loops that are being executed each second */ 
  bool launchingSoft; /**< Indicator showing whether soft launch control adjustments are active */
  bool launchingHard; /**< Indicator showing whether hard launch control adjustments are active */
  uint16_t freeRAM;
  unsigned int clutchEngagedRPM; /**< The RPM at which the clutch was last depressed. Used for distinguishing between launch control and flat shift */ 
  bool flatShiftingHard;
  volatile uint32_t startRevolutions; /**< A counter for how many revolutions have been completed since sync was achieved. */
  uint16_t boostTarget;
  byte testOutputs;
  bool testActive;
  uint16_t boostDuty; //Percentage value * 100 to give 2 points of precision
  byte idleLoad; /**< Either the current steps or current duty cycle for the idle control. */
  uint16_t canin[16];   //16bit raw value of selected canin data for channel 0-15
  uint8_t current_caninchannel = 0; /**< Current CAN channel, defaults to 0 */
  uint16_t crankRPM = 400; /**< The actual cranking RPM limit. This is derived from the value in the config page, but saves us multiplying it everytime it's used (Config page value is stored divided by 10) */
  volatile byte status3;
  int16_t flexBoostCorrection; /**< Amount of boost added based on flex */
  byte nitrous_status;
  byte nSquirts;
  byte nChannels; /**< Number of fuel and ignition channels.  */
  int16_t fuelLoad;
  int16_t fuelLoad2;
  int16_t ignLoad;
  bool fuelPumpOn; /**< Indicator showing the current status of the fuel pump */
  byte syncLossCounter;
  byte knockRetard;
  bool knockActive;
  bool toothLogEnabled;
  bool compositeLogEnabled;
  //int8_t vvt1Angle;
  long vvt1Angle;
  byte vvt1TargetAngle;
  byte vvt1Duty;
  uint16_t injAngle;
  byte ASEValue;
  uint16_t vss; /**< Current speed reading. Natively stored in kph and converted to mph in TS if required */
  bool idleUpOutputActive; /**< Whether the idle up output is currently active */
  byte gear; /**< Current gear (Calculated from vss) */
  byte fuelPressure; /**< Fuel pressure in PSI */
  byte oilPressure; /**< Oil pressure in PSI */
  byte engineProtectStatus;
  byte wmiPW;
  bool wmiEmpty;
  long vvt2Angle;
  byte vvt2TargetAngle;
  byte vvt2Duty;
  byte outputsStatus;
  byte TS_SD_Status; //TunerStudios SD card status
};

/**
 * @brief This mostly covers off variables that are required for fuel
 * 
 * See the ini file for further reference
 * 
 */
struct config2 {

  byte aseTaperTime;
  byte aeColdPct;  //AE cold clt modifier %
  byte aeColdTaperMin; //AE cold modifier, taper start temp (full modifier), was ASE in early versions
  byte aeMode : 2; /**< Acceleration Enrichment mode. 0 = TPS, 1 = MAP. Values 2 and 3 reserved for potential future use (ie blended TPS / MAP) */
  byte battVCorMode : 1;
  byte SoftLimitMode : 1;
  byte useTachoSweep : 1;
  byte aeApplyMode : 1; //0 = Multiply | 1 = Add
  byte multiplyMAP : 2; //0 = off | 1 = baro | 2 = 100
  byte wueValues[10]; //Warm up enrichment array (10 bytes)
  byte crankingPct; //Cranking enrichment
  byte pinMapping; // The board / ping mapping to be used
  byte tachoPin : 6; //Custom pin setting for tacho output
  byte tachoDiv : 2; //Whether to change the tacho speed
  byte tachoDuration; //The duration of the tacho pulse in mS
  byte maeThresh; /**< The MAPdot threshold that must be exceeded before AE is engaged */
  byte taeThresh; /**< The TPSdot threshold that must be exceeded before AE is engaged */
  byte aeTime;

  //Display config bits
  byte displayType : 3; //21
  byte display1 : 3;
  byte display2 : 2;

  byte display3 : 3;    //22
  byte display4 : 2;
  byte display5 : 3;

  byte displayB1 : 4;   //23
  byte displayB2 : 4;

  byte reqFuel;       //24
  byte divider;
  byte injTiming : 1;
  byte multiplyMAP_old : 1;
  byte includeAFR : 1;
  byte hardCutType : 1;
  byte ignAlgorithm : 3;
  byte indInjAng : 1;
  byte injOpen; //Injector opening time (ms * 10)
  uint16_t injAng[4];

  //config1 in ini
  byte mapSample : 2;
  byte strokes : 1;
  byte injType : 1;
  byte nCylinders : 4; //Number of cylinders

  //config2 in ini
  byte fuelAlgorithm : 3;
  byte fixAngEnable : 1; //Whether fixed/locked timing is enabled
  byte nInjectors : 4; //Number of injectors


  //config3 in ini
  byte engineType : 1;
  byte flexEnabled : 1;
  byte legacyMAP  : 1;
  byte baroCorr : 1;
  byte injLayout : 2;
  byte perToothIgn : 1;
  byte dfcoEnabled : 1; //Whether or not DFCO is turned on

  byte aeColdTaperMax;  //AE cold modifier, taper end temp (no modifier applied), was primePulse in early versions
  byte dutyLim;
  byte flexFreqLow; //Lowest valid frequency reading from the flex sensor
  byte flexFreqHigh; //Highest valid frequency reading from the flex sensor

  byte boostMaxDuty;
  byte tpsMin;
  byte tpsMax;
  int8_t mapMin; //Must be signed
  uint16_t mapMax;
  byte fpPrime; //Time (In seconds) that the fuel pump should be primed for on power up
  byte stoich;
  uint16_t oddfire2; //The ATDC angle of channel 2 for oddfire
  uint16_t oddfire3; //The ATDC angle of channel 3 for oddfire
  uint16_t oddfire4; //The ATDC angle of channel 4 for oddfire

  byte idleUpPin : 6;
  byte idleUpPolarity : 1;
  byte idleUpEnabled : 1;

  byte idleUpAdder;
  byte aeTaperMin;
  byte aeTaperMax;

  byte iacCLminDuty;
  byte iacCLmaxDuty;
  byte boostMinDuty;

  int8_t baroMin; //Must be signed
  uint16_t baroMax;

  int8_t EMAPMin; //Must be signed
  uint16_t EMAPMax;

  byte fanWhenOff : 1;      // Only run fan when engine is running
  byte fanWhenCranking : 1;      //**< Setting whether the fan output will stay on when the engine is cranking */ 
  byte useDwellMap : 1;  // Setting to change between fixed dwell value and dwell map
  byte fanUnused : 2;
  byte rtc_mode : 2;
  byte incorporateAFR : 1;  //Incorporate AFR
  byte asePct[4];  //Afterstart enrichment (%)
  byte aseCount[4]; //Afterstart enrichment cycles. This is the number of ignition cycles that the afterstart enrichment % lasts for
  byte aseBins[4]; //Afterstart enrichment temp axis
  byte primePulse[4]; //Priming pulsewidth
  byte primeBins[4]; //Priming temp axis

  byte CTPSPin : 6;
  byte CTPSPolarity : 1;
  byte CTPSEnabled : 1;

  byte idleAdvEnabled : 2;
  byte idleAdvAlgorithm : 1;
  byte IdleAdvDelay : 5;
  
  byte idleAdvRPM;
  byte idleAdvTPS;

  byte injAngRPM[4];

  byte idleTaperTime;
  byte dfcoDelay;
  byte dfcoMinCLT;

  //VSS Stuff
  byte vssMode : 2;
  byte vssPin : 6;
  
  uint16_t vssPulsesPerKm;
  byte vssSmoothing;
  uint16_t vssRatio1;
  uint16_t vssRatio2;
  uint16_t vssRatio3;
  uint16_t vssRatio4;
  uint16_t vssRatio5;
  uint16_t vssRatio6;

  byte idleUpOutputEnabled : 1;
  byte idleUpOutputInv : 1;
  byte idleUpOutputPin  : 6;

  byte tachoSweepMaxRPM;
  byte primingDelay;

  byte iacTPSlimit;
  byte iacRPMlimitHysteresis;

  int8_t rtc_trim;

  byte unused2_95[4];

#if defined(CORE_AVR)
  };
#else
  } __attribute__((__packed__)); //The 32 bi systems require all structs to be fully packed
#endif

//Page 4 of the config - See the ini file for further reference
//This mostly covers off variables that are required for ignition
struct config4 {

  int16_t triggerAngle;
  int8_t FixAng; //Negative values allowed
  byte CrankAng;
  byte TrigAngMul; //Multiplier for non evenly divisible tooth counts.

  byte TrigEdge : 1;
  byte TrigSpeed : 1;
  byte IgInv : 1;
  byte TrigPattern : 5;

  byte TrigEdgeSec : 1;
  byte fuelPumpPin : 6;
  byte useResync : 1;

  byte sparkDur; //Spark duration in ms * 10
  byte trigPatternSec; //Mode for Missing tooth secondary trigger.  Either single tooth cam wheel or 4-1
  uint8_t bootloaderCaps; //Capabilities of the bootloader over stock. e.g., 0=Stock, 1=Reset protection, etc.

  byte resetControlConfig : 2; //Which method of reset control to use (0=None, 1=Prevent When Running, 2=Prevent Always, 3=Serial Command)
  byte resetControlPin : 6;

  byte StgCycles; //The number of initial cycles before the ignition should fire when first cranking

  byte boostType : 1; //Open or closed loop boost control
  byte useDwellLim : 1; //Whether the dwell limiter is off or on
  byte sparkMode : 3; //Spark output mode (Eg Wasted spark, single channel or Wasted COP)
  byte triggerFilter : 2; //The mode of trigger filter being used (0=Off, 1=Light (Not currently used), 2=Normal, 3=Aggressive)
  byte ignCranklock : 1; //Whether or not the ignition timing during cranking is locked to a CAS pulse. Only currently valid for Basic distributor and 4G63.

  byte dwellCrank; //Dwell time whilst cranking
  byte dwellRun; //Dwell time whilst running
  byte triggerTeeth; //The full count of teeth on the trigger wheel if there were no gaps
  byte triggerMissingTeeth; //The size of the tooth gap (ie number of missing teeth)
  byte crankRPM; //RPM below which the engine is considered to be cranking
  byte floodClear; //TPS value that triggers flood clear mode (No fuel whilst cranking)
  byte SoftRevLim; //Soft rev limit (RPM/100)
  byte SoftLimRetard; //Amount soft limit retards (degrees)
  byte SoftLimMax; //Time the soft limit can run
  byte HardRevLim; //Hard rev limit (RPM/100)
  byte taeBins[4]; //TPS based acceleration enrichment bins (%/s)
  byte taeValues[4]; //TPS based acceleration enrichment rates (% to add)
  byte wueBins[10]; //Warmup Enrichment bins (Values are in configTable1)
  byte dwellLimit;
  byte dwellCorrectionValues[6]; //Correction table for dwell vs battery voltage
  byte iatRetBins[6]; // Inlet Air Temp timing retard curve bins
  byte iatRetValues[6]; // Inlet Air Temp timing retard curve values
  byte dfcoRPM; //RPM at which DFCO turns off/on at
  byte dfcoHyster; //Hysteris RPM for DFCO
  byte dfcoTPSThresh; //TPS must be below this figure for DFCO to engage

  byte ignBypassEnabled : 1; //Whether or not the ignition bypass is enabled
  byte ignBypassPin : 6; //Pin the ignition bypass is activated on
  byte ignBypassHiLo : 1; //Whether this should be active high or low.

  byte ADCFILTER_TPS;
  byte ADCFILTER_CLT;
  byte ADCFILTER_IAT;
  byte ADCFILTER_O2;
  byte ADCFILTER_BAT;
  byte ADCFILTER_MAP; //This is only used on Instantaneous MAP readings and is intentionally very weak to allow for faster response
  byte ADCFILTER_BARO;
  
  byte cltAdvBins[6]; /**< Coolant Temp timing advance curve bins */
  byte cltAdvValues[6]; /**< Coolant timing advance curve values. These are translated by 15 to allow for negative values */

  byte maeBins[4]; /**< MAP based AE MAPdot bins */
  byte maeRates[4]; /**< MAP based AE values */

  int8_t batVoltCorrect; /**< Battery voltage calibration offset */

  byte baroFuelBins[8];
  byte baroFuelValues[8];

  byte idleAdvBins[6];
  byte idleAdvValues[6];

  byte engineProtectMaxRPM;

  byte unused4_120[7];

#if defined(CORE_AVR)
  };
#else
  } __attribute__((__packed__)); //The 32 bi systems require all structs to be fully packed
#endif

//Page 6 of the config - See the ini file for further reference
//This mostly covers off variables that are required for AFR targets and closed loop
struct config6 {

  byte egoAlgorithm : 2;
  byte egoType : 2;
  byte boostEnabled : 1;
  byte vvtEnabled : 1;
  byte engineProtectType : 2;

  byte egoKP;
  byte egoKI;
  byte egoKD;
  byte egoTemp; //The temperature above which closed loop functions
  byte egoCount; //The number of ignition cylces per step
  byte vvtMode : 2; //Valid VVT modes are 'on/off', 'open loop' and 'closed loop'
  byte vvtLoadSource : 2; //Load source for VVT (TPS or MAP)
  byte vvtPWMdir : 1; //VVT direction (normal or reverse)
  byte vvtCLUseHold : 1; //Whether or not to use a hold duty cycle (Most cases are Yes)
  byte vvtCLAlterFuelTiming : 1;
  byte boostCutEnabled : 1;
  byte egoLimit; //Maximum amount the closed loop will vary the fueling
  byte ego_min; //AFR must be above this for closed loop to function
  byte ego_max; //AFR must be below this for closed loop to function
  byte ego_sdelay; //Time in seconds after engine starts that closed loop becomes available
  byte egoRPM; //RPM must be above this for closed loop to function
  byte egoTPSMax; //TPS must be below this for closed loop to function
  byte vvt1Pin : 6;
  byte useExtBaro : 1;
  byte boostMode : 1; //Simple of full boost control
  byte boostPin : 6;
  byte VVTasOnOff : 1; //Whether or not to use the VVT table as an on/off map
  byte useEMAP : 1;
  byte voltageCorrectionBins[6]; //X axis bins for voltage correction tables
  byte injVoltageCorrectionValues[6]; //Correction table for injector PW vs battery voltage
  byte airDenBins[9];
  byte airDenRates[9];
  byte boostFreq; //Frequency of the boost PWM valve
  byte vvtFreq; //Frequency of the vvt PWM valve
  byte idleFreq;

  byte launchPin : 6;
  byte launchEnabled : 1;
  byte launchHiLo : 1;

  byte lnchSoftLim;
  int8_t lnchRetard; //Allow for negative advance value (ATDC)
  byte lnchHardLim;
  byte lnchFuelAdd;

  //PID values for idle needed to go here as out of room in the idle page
  byte idleKP;
  byte idleKI;
  byte idleKD;

  byte boostLimit; //Is divided by 2, allowing kPa values up to 511
  byte boostKP;
  byte boostKI;
  byte boostKD;

  byte lnchPullRes : 2;
  byte fuelTrimEnabled : 1;
  byte flatSEnable : 1;
  byte baroPin : 4;
  byte flatSSoftWin;
  byte flatSRetard;
  byte flatSArm;

  byte iacCLValues[10]; //Closed loop target RPM value
  byte iacOLStepVal[10]; //Open loop step values for stepper motors
  byte iacOLPWMVal[10]; //Open loop duty values for PMWM valves
  byte iacBins[10]; //Temperature Bins for the above 3 curves
  byte iacCrankSteps[4]; //Steps to use when cranking (Stepper motor)
  byte iacCrankDuty[4]; //Duty cycle to use on PWM valves when cranking
  byte iacCrankBins[4]; //Temperature Bins for the above 2 curves

  byte iacAlgorithm : 3; //Valid values are: "None", "On/Off", "PWM", "PWM Closed Loop", "Stepper", "Stepper Closed Loop"
  byte iacStepTime : 3; //How long to pulse the stepper for to ensure the step completes (ms)
  byte iacChannels : 1; //How many outputs to use in PWM mode (0 = 1 channel, 1 = 2 channels)
  byte iacPWMdir : 1; //Direction of the PWM valve. 0 = Normal = Higher RPM with more duty. 1 = Reverse = Lower RPM with more duty

  byte iacFastTemp; //Fast idle temp when using a simple on/off valve

  byte iacStepHome; //When using a stepper motor, the number of steps to be taken on startup to home the motor
  byte iacStepHyster; //Hysteresis temperature (*10). Eg 2.2C = 22

  byte fanInv : 1;        // Fan output inversion bit
  byte fanEnable : 1;     // Fan enable bit. 0=Off, 1=On/Off
  byte fanPin : 6;
  byte fanSP;             // Cooling fan start temperature
  byte fanHyster;         // Fan hysteresis
  byte fanFreq;           // Fan PWM frequency
  byte fanPWMBins[4];     //Temperature Bins for the PWM fan control

#if defined(CORE_AVR)
  };
#else
  } __attribute__((__packed__)); //The 32 bit systems require all structs to be fully packed
#endif

//Page 9 of the config mostly deals with CANBUS control
//See ini file for further info (Config Page 10 in the ini)
struct config9 {
  byte enable_secondarySerial:1;            //enable secondary serial
  byte intcan_available:1;                     //enable internal can module
  byte enable_intcan:1;
  byte caninput_sel[16];                    //bit status on/Can/analog_local/digtal_local if input is enabled
  uint16_t caninput_source_can_address[16];        //u16 [15] array holding can address of input
  uint8_t caninput_source_start_byte[16];     //u08 [15] array holds the start byte number(value of 0-7)
  uint16_t caninput_source_num_bytes;     //u16 bit status of the number of bytes length 1 or 2
  byte unused10_67;
  byte unused10_68;
  byte enable_candata_out : 1;
  byte canoutput_sel[8];
  uint16_t canoutput_param_group[8];
  uint8_t canoutput_param_start_byte[8];
  byte canoutput_param_num_bytes[8];

  byte unused10_110;
  byte unused10_111;
  byte unused10_112;
  byte unused10_113;
  byte speeduino_tsCanId:4;         //speeduino TS canid (0-14)
  uint16_t true_address;            //speeduino 11bit can address
  uint16_t realtime_base_address;   //speeduino 11 bit realtime base address
  uint16_t obd_address;             //speeduino OBD diagnostic address
  uint8_t Auxinpina[16];            //analog  pin number when internal aux in use
  uint8_t Auxinpinb[16];            // digital pin number when internal aux in use

  byte iacStepperInv : 1;  //stepper direction of travel to allow reversing. 0=normal, 1=inverted.
  byte iacCoolTime : 3; // how long to wait for the stepper to cool between steps

  byte iacMaxSteps; // Step limit beyond which the stepper won't be driven. Should always be less than homing steps. Stored div 3 as per home steps.

  byte unused10_155;
  byte unused10_156;
  byte unused10_157;
  byte unused10_158;
  byte unused10_159;
  byte unused10_160;
  byte unused10_161;
  byte unused10_162;
  byte unused10_163;
  byte unused10_164;
  byte unused10_165;
  byte unused10_166;
  byte unused10_167;
  byte unused10_168;
  byte unused10_169;
  byte unused10_170;
  byte unused10_171;
  byte unused10_172;
  byte unused10_173;
  byte unused10_174;
  byte unused10_175;
  byte unused10_176;
  byte unused10_177;
  byte unused10_178;
  byte unused10_179;
  byte unused10_180;
  byte unused10_181;
  byte unused10_182;
  byte unused10_183;
  byte unused10_184;
  byte unused10_185;
  byte unused10_186;
  byte unused10_187;
  byte unused10_188;
  byte unused10_189;
  byte unused10_190;
  byte unused10_191;
  
#if defined(CORE_AVR)
  };
#else
  } __attribute__((__packed__)); //The 32 bit systems require all structs to be fully packed
#endif

/*
Page 10 - No specific purpose. Created initially for the cranking enrich curve
192 bytes long
See ini file for further info (Config Page 11 in the ini)
*/
struct config10 {
  byte crankingEnrichBins[4]; //Bytes 0-4
  byte crankingEnrichValues[4]; //Bytes 4-7

  //Byte 8
  byte rotaryType : 2;
  byte stagingEnabled : 1;
  byte stagingMode : 1;
  byte EMAPPin : 4;

  byte rotarySplitValues[8]; //Bytes 9-16
  byte rotarySplitBins[8]; //Bytes 17-24

  uint16_t boostSens; //Bytes 25-26
  byte boostIntv; //Byte 27
  uint16_t stagedInjSizePri; //Bytes 28-29
  uint16_t stagedInjSizeSec; //Bytes 30-31
  byte lnchCtrlTPS; //Byte 32

  uint8_t flexBoostBins[6]; //Byets 33-38
  int16_t flexBoostAdj[6];  //kPa to be added to the boost target @ current ethanol (negative values allowed). Bytes 39-50
  uint8_t flexFuelBins[6]; //Bytes 51-56
  uint8_t flexFuelAdj[6];   //Fuel % @ current ethanol (typically 100% @ 0%, 163% @ 100%). Bytes 57-62
  uint8_t flexAdvBins[6]; //Bytes 63-68
  uint8_t flexAdvAdj[6];    //Additional advance (in degrees) @ current ethanol (typically 0 @ 0%, 10-20 @ 100%). NOTE: THIS SHOULD BE A SIGNED VALUE BUT 2d TABLE LOOKUP NOT WORKING WITH IT CURRENTLY!
                            //And another three corn rows die.
                            //Bytes 69-74

  //Byte 75
  byte n2o_enable : 2;
  byte n2o_arming_pin : 6;
  byte n2o_minCLT; //Byte 76
  byte n2o_maxMAP; //Byte 77
  byte n2o_minTPS; //Byte 78
  byte n2o_maxAFR; //Byte 79

  //Byte 80
  byte n2o_stage1_pin : 6;
  byte n2o_pin_polarity : 1;
  byte n2o_stage1_unused : 1;
  byte n2o_stage1_minRPM; //Byte 81
  byte n2o_stage1_maxRPM; //Byte 82
  byte n2o_stage1_adderMin; //Byte 83
  byte n2o_stage1_adderMax; //Byte 84
  byte n2o_stage1_retard; //Byte 85

  //Byte 86
  byte n2o_stage2_pin : 6;
  byte n2o_stage2_unused : 2;
  byte n2o_stage2_minRPM; //Byte 87
  byte n2o_stage2_maxRPM; //Byte 88
  byte n2o_stage2_adderMin; //Byte 89
  byte n2o_stage2_adderMax; //Byte 90
  byte n2o_stage2_retard; //Byte 91

  //Byte 92
  byte knock_mode : 2;
  byte knock_pin : 6;

  //Byte 93
  byte knock_trigger : 1;
  byte knock_pullup : 1;
  byte knock_limiterDisable : 1;
  byte knock_unused : 2;
  byte knock_count : 3;

  byte knock_threshold; //Byte 94
  byte knock_maxMAP; //Byte 95
  byte knock_maxRPM; //Byte 96
  byte knock_window_rpms[6]; //Bytes 97-102
  byte knock_window_angle[6]; //Bytes 103-108
  byte knock_window_dur[6]; //Bytes 109-114

  byte knock_maxRetard; //Byte 115
  byte knock_firstStep; //Byte 116
  byte knock_stepSize; //Byte 117
  byte knock_stepTime; //Byte 118
        
  byte knock_duration; //Time after knock retard starts that it should start recovering. Byte 119
  byte knock_recoveryStepTime; //Byte 120
  byte knock_recoveryStep; //Byte 121

  //Byte 122
  byte fuel2Algorithm : 3;
  byte fuel2Mode : 3;
  byte fuel2SwitchVariable : 2;

  //Bytes 123-124
  uint16_t fuel2SwitchValue;

  //Byte 125
  byte fuel2InputPin : 6;
  byte fuel2InputPolarity : 1;
  byte fuel2InputPullup : 1;

  byte vvtCLholdDuty; //Byte 126
  byte vvtCLKP; //Byte 127
  byte vvtCLKI; //Byte 128
  byte vvtCLKD; //Byte 129
  int16_t vvtCLMinAng; //Bytes 130-131
  int16_t vvtCLMaxAng; //Bytes 132-133

  byte crankingEnrichTaper; //Byte 134

  byte fuelPressureEnable : 1;
  byte oilPressureEnable : 1;
  byte oilPressureProtEnbl : 1;
  byte unused10_135 : 5;

  byte fuelPressurePin : 4;
  byte oilPressurePin : 4;

  int8_t fuelPressureMin;
  byte fuelPressureMax;
  int8_t oilPressureMin;
  byte oilPressureMax;

  byte oilPressureProtRPM[4];
  byte oilPressureProtMins[4];

  byte wmiEnabled : 1; // Byte 149
  byte wmiMode : 6;
  
  byte wmiAdvEnabled : 1;

  byte wmiTPS; // Byte 150
  byte wmiRPM; // Byte 151
  byte wmiMAP; // Byte 152
  byte wmiMAP2; // Byte 153
  byte wmiIAT; // Byte 154
  int8_t wmiOffset; // Byte 155

  byte wmiIndicatorEnabled : 1; // 156
  byte wmiIndicatorPin : 6;
  byte wmiIndicatorPolarity : 1;

  byte wmiEmptyEnabled : 1; // 157
  byte wmiEmptyPin : 6;
  byte wmiEmptyPolarity : 1;

  byte wmiEnabledPin; // 158

  byte wmiAdvBins[6]; //Bytes 159-164
  byte wmiAdvAdj[6];  //Additional advance (in degrees)
                      //Bytes 165-170
  byte vvtCLminDuty;
  byte vvtCLmaxDuty;
  byte vvt2Pin : 6;
  byte unused11_174_1 : 1;
  byte unused11_174_2 : 1;

  byte fuelTempBins[6];
  byte fuelTempValues[6]; //180

  //Byte 186
  byte spark2Algorithm : 3;
  byte spark2Mode : 3;
  byte spark2SwitchVariable : 2;

  //Bytes 187-188
  uint16_t spark2SwitchValue;

  //Byte 189
  byte spark2InputPin : 6;
  byte spark2InputPolarity : 1;
  byte spark2InputPullup : 1;

  byte unused11_187_191[2]; //Bytes 187-191

#if defined(CORE_AVR)
  };
#else
  } __attribute__((__packed__)); //The 32 bit systems require all structs to be fully packed
#endif

struct cmpOperation{
  uint8_t firstCompType : 3;
  uint8_t secondCompType : 3;
  uint8_t bitwise : 2;
};

/*
Page 13 - Programmable outputs conditions.
128 bytes long
*/
struct config13 {
  uint8_t outputInverted;
  uint8_t unused12_1;
  uint8_t outputPin[8];
  uint8_t outputDelay[8]; //0.1S
  uint8_t firstDataIn[8];
  uint8_t secondDataIn[8];
  uint8_t unused_13[16];
  int16_t firstTarget[8];
  int16_t secondTarget[8];
  //89bytes
  struct cmpOperation operation[8];

  uint16_t candID[8]; //Actual CAN ID need 16bits, this is a placeholder

  byte unused12_106_127[22];

#if defined(CORE_AVR)
  };
#else
  } __attribute__((__packed__)); //The 32 bit systems require all structs to be fully packed
#endif

extern byte pinInjector1; //Output pin injector 1
extern byte pinInjector2; //Output pin injector 2
extern byte pinInjector3; //Output pin injector 3
extern byte pinInjector4; //Output pin injector 4
extern byte pinInjector5; //Output pin injector 5
extern byte pinInjector6; //Output pin injector 6
extern byte pinInjector7; //Output pin injector 7
extern byte pinInjector8; //Output pin injector 8
extern byte injectorOutputControl; //Specifies whether the injectors are controlled directly (Via an IO pin) or using something like the MC33810
extern byte pinCoil1; //Pin for coil 1
extern byte pinCoil2; //Pin for coil 2
extern byte pinCoil3; //Pin for coil 3
extern byte pinCoil4; //Pin for coil 4
extern byte pinCoil5; //Pin for coil 5
extern byte pinCoil6; //Pin for coil 6
extern byte pinCoil7; //Pin for coil 7
extern byte pinCoil8; //Pin for coil 8
extern byte ignitionOutputControl; //Specifies whether the coils are controlled directly (Via an IO pin) or using something like the MC33810
extern byte pinTrigger; //The CAS pin
extern byte pinTrigger2; //The Cam Sensor pin
extern byte pinTrigger3;	//the 2nd cam sensor pin
extern byte pinTPS;//TPS input pin
extern byte pinMAP; //MAP sensor pin
extern byte pinEMAP; //EMAP sensor pin
extern byte pinMAP2; //2nd MAP sensor (Currently unused)
extern byte pinIAT; //IAT sensor pin
extern byte pinCLT; //CLS sensor pin
extern byte pinO2; //O2 Sensor pin
extern byte pinO2_2; //second O2 pin
extern byte pinBat; //Battery voltage pin
extern byte pinDisplayReset; // OLED reset pin
extern byte pinTachOut; //Tacho output
extern byte pinFuelPump; //Fuel pump on/off
extern byte pinIdle1; //Single wire idle control
extern byte pinIdle2; //2 wire idle control (Not currently used)
extern byte pinIdleUp; //Input for triggering Idle Up
extern byte pinIdleUpOutput; //Output that follows (normal or inverted) the idle up pin
extern byte pinCTPS; //Input for triggering closed throttle state
extern byte pinFuel2Input; //Input for switching to the 2nd fuel table
extern byte pinSpark2Input; //Input for switching to the 2nd ignition table
extern byte pinSpareTemp1; // Future use only
extern byte pinSpareTemp2; // Future use only
extern byte pinSpareOut1; //Generic output
extern byte pinSpareOut2; //Generic output
extern byte pinSpareOut3; //Generic output
extern byte pinSpareOut4; //Generic output
extern byte pinSpareOut5; //Generic output
extern byte pinSpareOut6; //Generic output
extern byte pinSpareHOut1; //spare high current output
extern byte pinSpareHOut2; // spare high current output
extern byte pinSpareLOut1; // spare low current output
extern byte pinSpareLOut2; // spare low current output
extern byte pinSpareLOut3;
extern byte pinSpareLOut4;
extern byte pinSpareLOut5;
extern byte pinBoost;
extern byte pinVVT_1;		// vvt output 1
extern byte pinVVT_2;		// vvt output 2
extern byte pinFan;       // Cooling fan output
extern byte pinStepperDir; //Direction pin for the stepper motor driver
extern byte pinStepperStep; //Step pin for the stepper motor driver
extern byte pinStepperEnable; //Turning the DRV8825 driver on/off
extern byte pinLaunch;
extern byte pinIgnBypass; //The pin used for an ignition bypass (Optional)
extern byte pinFlex; //Pin with the flex sensor attached
extern byte pinVSS; 
extern byte pinBaro; //Pin that an external barometric pressure sensor is attached to (If used)
extern byte pinResetControl; // Output pin used control resetting the Arduino
extern byte pinFuelPressure;
extern byte pinOilPressure;
extern byte pinWMIEmpty; // Water tank empty sensor
extern byte pinWMIIndicator; // No water indicator bulb
extern byte pinWMIEnabled; // ON-OFF ouput to relay/pump/solenoid 
extern byte pinMC33810_1_CS;
extern byte pinMC33810_2_CS;
#ifdef USE_SPI_EEPROM
  extern byte pinSPIFlash_CS;
#endif


/* global variables */ // from speeduino.ino
//#ifndef UNIT_TEST

//#endif

extern struct statuses currentStatus; //The global status object
extern struct config2 configPage2;
extern struct config4 configPage4;
extern struct config6 configPage6;
extern struct config9 configPage9;
extern struct config10 configPage10;
extern struct config13 configPage13;
//extern byte cltCalibrationTable[CALIBRATION_TABLE_SIZE]; /**< An array containing the coolant sensor calibration values */
//extern byte iatCalibrationTable[CALIBRATION_TABLE_SIZE]; /**< An array containing the inlet air temperature sensor calibration values */
//extern byte o2CalibrationTable[CALIBRATION_TABLE_SIZE]; /**< An array containing the O2 sensor calibration values */

extern uint16_t cltCalibration_bins[32];
extern uint16_t cltCalibration_values[32];
extern uint16_t iatCalibration_bins[32];
extern uint16_t iatCalibration_values[32];
extern uint16_t o2Calibration_bins[32];
extern uint8_t  o2Calibration_values[32]; // Note 8-bit values
extern struct table2D cltCalibrationTable; /**< A 32 bin array containing the coolant temperature sensor calibration values */
extern struct table2D iatCalibrationTable; /**< A 32 bin array containing the inlet air temperature sensor calibration values */
extern struct table2D o2CalibrationTable; /**< A 32 bin array containing the O2 sensor calibration values */

static_assert(sizeof(struct config2) == 128, "configPage2 size is not 128");
static_assert(sizeof(struct config4) == 128, "configPage4 size is not 128");
static_assert(sizeof(struct config6) == 128, "configPage6 size is not 128");
static_assert(sizeof(struct config9) == 192, "configPage9 size is not 192");
static_assert(sizeof(struct config10) == 192, "configPage10 size is not 192");
static_assert(sizeof(struct config13) == 128, "configPage13 size is not 128");
#endif // GLOBALS_H
