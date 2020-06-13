#if defined(CORE_STM32_GENERIC)
#include "board_stm32_generic.h"
#include "globals.h"
#include "auxiliaries.h"
#include "idle.h"
#include "scheduler.h"
#include "HardwareTimer.h"

void initBoard()
{
    /*
    ***********************************************************************************************************
    * General
    */
    #ifndef FLASH_LENGTH
      #define FLASH_LENGTH 8192
    #endif
    delay(10);
    /*
    ***********************************************************************************************************
    * Idle
    */
    if( (configPage6.iacAlgorithm == IAC_ALGORITHM_PWM_OL) || (configPage6.iacAlgorithm == IAC_ALGORITHM_PWM_CL) )
    {
        idle_pwm_max_count = 1000000L / (TIMER_RESOLUTION * configPage6.idleFreq * 2); //Converts the frequency in Hz to the number of ticks (at 2uS) it takes to complete 1 cycle. Note that the frequency is divided by 2 coming from TS to allow for up to 5KHz
    } 

    //This must happen at the end of the idle init
    Timer1.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer1.attachInterrupt(4, idleInterrupt); //on first flash the configPage4.iacAlgorithm is invalid


    /*
    ***********************************************************************************************************
    * Timers
    */
    #if defined(ARDUINO_BLUEPILL_F103C8) || defined(ARDUINO_BLUEPILL_F103CB)
        Timer4.setPeriod(1000);  // Set up period
        Timer4.setMode(1, TIMER_OUTPUT_COMPARE);
        Timer4.attachInterrupt(1, oneMSInterval);
        Timer4.resume(); //Start Timer
    #else
        Timer11.setPeriod(1000);  // Set up period
        Timer11.setMode(1, TIMER_OUTPUT_COMPARE);
        Timer11.attachInterrupt(1, oneMSInterval);
        Timer11.resume(); //Start Timer
    #endif
    pinMode(LED_BUILTIN, OUTPUT); //Visual WDT

    /*
    ***********************************************************************************************************
    * Auxilliaries
    */
    //2uS resolution Min 8Hz, Max 5KHz
    boost_pwm_max_count = 1000000L / (TIMER_RESOLUTION * configPage6.boostFreq * 2); //Converts the frequency in Hz to the number of ticks (at 2uS) it takes to complete 1 cycle. The x2 is there because the frequency is stored at half value (in a byte) to allow freqneucies up to 511Hz
    vvt_pwm_max_count = 1000000L / (TIMER_RESOLUTION * configPage6.vvtFreq * 2); //Converts the frequency in Hz to the number of ticks (at 2uS) it takes to complete 1 cycle

    //Need to be initialised last due to instant interrupt
    Timer1.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer1.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer1.attachInterrupt(2, boostInterrupt);
    Timer1.attachInterrupt(3, vvtInterrupt);

    /*
    ***********************************************************************************************************
    * Schedules
    */
    #if defined(ARDUINO_BLUEPILL_F103C8) || defined(ARDUINO_BLUEPILL_F103CB)
        //(CYCLES_PER_MICROSECOND == 72, APB2 at 72MHz, APB1 at 36MHz).
        //Timer2 to 4 is on APB1, Timer1 on APB2.   www.st.com/resource/en/datasheet/stm32f103cb.pdf sheet 12
        Timer1.setPrescaleFactor(((Timer1.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1); //2us resolution
        Timer2.setPrescaleFactor(((Timer2.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1); //2us resolution
        Timer3.setPrescaleFactor(((Timer3.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1); //2us resolution
    #else
        //(CYCLES_PER_MICROSECOND == 168, APB2 at 84MHz, APB1 at 42MHz).
        //Timer2 to 14 is on APB1, Timers 1, 8, 9 and 10 on APB2.   www.st.com/resource/en/datasheet/stm32f407vg.pdf sheet 120
        Timer1.setPrescaleFactor(((Timer1.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1); //2us resolution
        Timer2.setPrescaleFactor(((Timer2.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1);  //2us resolution
        Timer3.setPrescaleFactor(((Timer3.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1);  //2us resolution
        Timer4.setPrescaleFactor(((Timer4.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1);  //2us resolution
        Timer5.setPrescaleFactor(((Timer5.getBaseFrequency()/1000000) * TIMER_RESOLUTION)-1);  //2us resolution
    #endif
    Timer2.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer2.setMode(4, TIMER_OUTPUT_COMPARE);

    Timer3.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer3.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer1.setMode(1, TIMER_OUTPUT_COMPARE);

    //Attach interupt functions
    //Injection
    Timer2.attachInterrupt(1, fuelSchedule1Interrupt);
    Timer2.attachInterrupt(2, fuelSchedule2Interrupt);
    Timer2.attachInterrupt(3, fuelSchedule3Interrupt);
    Timer2.attachInterrupt(4, fuelSchedule4Interrupt);
    #if (INJ_CHANNELS >= 5)
    Timer5.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer5.attachInterrupt(1, fuelSchedule5Interrupt);
    #endif
    #if (INJ_CHANNELS >= 6)
    Timer5.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer5.attachInterrupt(2, fuelSchedule6Interrupt);
    #endif
    #if (INJ_CHANNELS >= 7)
    Timer5.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer5.attachInterrupt(3, fuelSchedule7Interrupt);
    #endif
    #if (INJ_CHANNELS >= 8)
    Timer5.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer5.attachInterrupt(4, fuelSchedule8Interrupt);
    #endif

    //Ignition
    Timer3.attachInterrupt(1, ignitionSchedule1Interrupt); 
    Timer3.attachInterrupt(2, ignitionSchedule2Interrupt);
    Timer3.attachInterrupt(3, ignitionSchedule3Interrupt);
    Timer3.attachInterrupt(4, ignitionSchedule4Interrupt);
    #if (IGN_CHANNELS >= 5)
    Timer4.setMode(1, TIMER_OUTPUT_COMPARE);
    Timer4.attachInterrupt(1, ignitionSchedule5Interrupt);
    #endif
    #if (IGN_CHANNELS >= 6)
    Timer4.setMode(2, TIMER_OUTPUT_COMPARE);
    Timer4.attachInterrupt(2, ignitionSchedule6Interrupt);
    #endif
    #if (IGN_CHANNELS >= 7)
    Timer4.setMode(3, TIMER_OUTPUT_COMPARE);
    Timer4.attachInterrupt(3, ignitionSchedule7Interrupt);
    #endif
    #if (IGN_CHANNELS >= 8)
    Timer4.setMode(4, TIMER_OUTPUT_COMPARE);
    Timer4.attachInterrupt(4, ignitionSchedule8Interrupt);
    #endif

    Timer1.setOverflow(0xFFFF);
    //Timer1.resume();
    Timer2.setOverflow(0xFFFF);
    //Timer2.resume();
    Timer3.setOverflow(0xFFFF);
    //Timer3.resume();
    #if (IGN_CHANNELS >= 5)
    Timer4.setOverflow(0xFFFF);
    //Timer4.resume();
    #endif
    #if (INJ_CHANNELS >= 5)
    Timer5.setOverflow(0xFFFF);
    //Timer5.resume();
    #endif
}

uint16_t freeRam()
{
    char top = 't';
    return &top - reinterpret_cast<char*>(sbrk(0));
}

#endif