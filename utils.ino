/*
Returns how much free dynamic memory exists (between heap and stack)
*/
#include "utils.h"

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setPinMapping(byte boardID)
{
  switch(boardID)
  {
    case 0:
      //Pin mappings as per the v0.1 shield
      pinInjector1 = 8; //Output pin injector 1 is on
      pinInjector2 = 9; //Output pin injector 2 is on
      pinInjector3 = 11; //Output pin injector 3 is on
      pinInjector4 = 10; //Output pin injector 4 is on
      pinCoil1 = 6; //Pin for coil 1
      pinCoil2 = 7; //Pin for coil 2
      pinCoil3 = 12; //Pin for coil 3
      pinCoil4 = 13; //Pin for coil 4
      pinTrigger = 2; //The CAS pin
      pinTPS = A0; //TPS input pin
      pinMAP = A1; //MAP sensor pin
      pinIAT = A2; //IAT sensor pin
      pinCLT = A3; //CLS sensor pin
      pinO2 = A4; //O2 Sensor pin
      break;
    case 1:
      //Pin mappings as per the v0.2 shield
      pinInjector1 = 8; //Output pin injector 1 is on
      pinInjector2 = 9; //Output pin injector 2 is on
      pinInjector3 = 10; //Output pin injector 3 is on
      pinInjector4 = 11; //Output pin injector 4 is on
      pinCoil1 = 28; //Pin for coil 1
      pinCoil2 = 24; //Pin for coil 2
      pinCoil3 = 40; //Pin for coil 3
      pinCoil4 = 36; //Pin for coil 4
      pinTrigger = 20; //The CAS pin
      pinTrigger2 = 21; //The Cam Sensor pin
      pinTPS = A2; //TPS input pin
      pinMAP = A3; //MAP sensor pin
      pinIAT = A0; //IAT sensor pin
      pinCLT = A1; //CLS sensor pin
      pinO2 = A8; //O2 Sensor pin
      pinBat = A4; //O2 Sensor pin
      pinDisplayReset = 48; // OLED reset pin
      break;
    case 2:
      //Pin mappings as per the v0.3 shield
      pinInjector1 = 8; //Output pin injector 1 is on
      pinInjector2 = 9; //Output pin injector 2 is on
      pinInjector3 = 10; //Output pin injector 3 is on
      pinInjector4 = 11; //Output pin injector 4 is on
      pinCoil1 = 28; //Pin for coil 1
      pinCoil2 = 24; //Pin for coil 2
      pinCoil3 = 40; //Pin for coil 3
      pinCoil4 = 36; //Pin for coil 4
      pinTrigger = 18; //The CAS pin
      pinTrigger2 = 19; //The Cam Sensor pin
      pinTPS = A2;//TPS input pin
      pinMAP = A3; //MAP sensor pin
      pinIAT = A0; //IAT sensor pin
      pinCLT = A1; //CLS sensor pin
      pinO2 = A8; //O2 Sensor pin
      pinBat = A4; //O2 Sensor pin
      pinDisplayReset = 48; // OLED reset pin
      break;
      
    default:
      //Pin mappings as per the v0.2 shield
      pinInjector1 = 8; //Output pin injector 1 is on
      pinInjector2 = 9; //Output pin injector 2 is on
      pinInjector3 = 10; //Output pin injector 3 is on
      pinInjector4 = 11; //Output pin injector 4 is on
      pinCoil1 = 28; //Pin for coil 1
      pinCoil2 = 24; //Pin for coil 2
      pinCoil3 = 40; //Pin for coil 3
      pinCoil4 = 36; //Pin for coil 4
      pinTrigger = 20; //The CAS pin
      pinTrigger2 = 21; //The Cam Sensor pin
      pinTPS = A2; //TPS input pin
      pinMAP = A3; //MAP sensor pin
      pinIAT = A0; //IAT sensor pin
      pinCLT = A1; //CLS sensor pin
      pinO2 = A8; //O2 Sensor pin
      pinBat = A4; //O2 Sensor pin
      pinDisplayReset = 48; // OLED reset pin
      break;
  }
}


/* The following functions help determine the required fuel constant. For more information about these calculations, please refer to http://www.megamanual.com/v22manual/mfuel.htm
  Calc below are for metric inputs of temp (degrees C) and MAP (kPa) to produce kg/m3.
*/
int AIRDEN(int MAP, int temp)
  {
	return (1.2929 * 273.13/(temp+273.13) * MAP/101.325);
  }

/*
This function retuns a pulsewidth time (in us) using a either Alpha-N or Speed Density algorithms, given the following:
REQ_FUEL
VE: Lookup from the main MAP vs RPM fuel table
MAP: In KPa, read from the sensor
GammaE: Sum of Enrichment factors (Cold start, acceleration). This is a multiplication factor (Eg to add 10%, this should be 110)
injDT: Injector dead time. The time the injector take to open minus the time it takes to close (Both in uS)
TPS: Throttle position (0% to 100%)

This function is called by PW_SD and PW_AN for speed0density and pure Alpha-N calculations respectively. 
*/
unsigned int PW(int REQ_FUEL, byte VE, byte MAP, int corrections, int injOpen, byte TPS)
  {
    //Standard float version of the calculation
    //return (REQ_FUEL * (float)(VE/100.0) * (float)(MAP/100.0) * (float)(TPS/100.0) * (float)(corrections/100.0) + injOpen);
    //Note: The MAP and TPS portions are currently disabled, we use VE and corrections only
    
    //100% float free version, does sacrifice a little bit of accuracy.
    int iVE = ((int)VE << 7) / 100;
    //int iVE = divs100( ((int)VE << 7));
    //int iMAP = ((int)MAP << 7) / 100;
    int iCorrections = (corrections << 7) / 100;
    //int iTPS = ((int)TPS << 7) / 100;

    unsigned long intermediate = ((long)REQ_FUEL * (long)iVE) >>7; //Need to use an intermediate value to avoid overflowing the long
    //intermediate = (intermediate * iMAP) >> 7;
    intermediate = (intermediate * iCorrections) >> 7;
    //intermediate = (intermediate * iTPS) >> 7;
    intermediate += injOpen; //Add the injector opening time
    if ( intermediate > 65535) { intermediate = 65535; } //Make sure this won't overflow when we convert to uInt. This means the maximum pulsewidth possible is 65.535mS
    return (unsigned int)(intermediate);

  }
 
//Convenience functions for Speed Density and Alpha-N
unsigned int PW_SD(int REQ_FUEL, byte VE, byte MAP, int corrections, int injOpen)
{
  //return PW(REQ_FUEL, VE, MAP, corrections, injOpen, 100); //Just use 1 in place of the TPS
  return PW(REQ_FUEL, VE, 100, corrections, injOpen, 100); //Just use 1 in place of the TPS
}

unsigned int PW_AN(int REQ_FUEL, byte VE, byte TPS, int corrections, int injOpen)
{
  //Sanity check
  if(TPS > 100) { TPS = 100; }
  //return PW(REQ_FUEL, VE, 100, corrections, injOpen, TPS); //Just use 1 in place of the MAP
  return PW(REQ_FUEL, VE, 100, corrections, injOpen, 100); //Just use 1 in place of the MAP
}
