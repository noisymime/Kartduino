//#include "alphaMods.h"

//pin setup
void alphaPinSetup(){
  switch (alphaVars.carSelect){
    case 0:
      pinAC; // pin for AC clutch
      pinAcReq;
      pinFan2;
      pinCEL;
      pinVVL;
      pinACpress;
      pinACtemp;
  
    case 1:
      pinAC = 45; // pin for AC clutch
      pinAcReq = 26;
      //pinFan2;
      pinCEL = 53;
      pinVVL = 6;
      pinACpress = 28;
      pinACtemp = A5;
      pinOilPress = 30;
      pinCLTgauge = 2;
      pinMode(pinAC, OUTPUT);
      pinMode(pinAcReq, INPUT);
      pinMode(pinCEL, OUTPUT);
      pinMode(pinVVL, OUTPUT);
      pinMode(pinACpress, INPUT_PULLUP);
      pinMode(pinACtemp, INPUT);
      pinMode(pinOilPress, INPUT_PULLUP);
      pinMode(pinCLTgauge, OUTPUT);
      break;
    case 2:
      break;
    case 4:
      pinAC = 45; // pin for AC clutch
      pinAcReq = 26;
      pinFan2 = 35;
      pinCEL = 53;
      //pinVVL = 6;
      pinACpress = 28;
      pinACtemp = A5;
      pinOilPress = 30;
      //pinCLTgauge = 2;
      pinMode(pinAC, OUTPUT);
      pinMode(pinAcReq, INPUT);
      pinMode(pinCEL, OUTPUT);
      //pinMode(pinVVL, OUTPUT);
      pinMode(pinACpress, INPUT_PULLUP);
      pinMode(pinACtemp, INPUT);
      pinMode(pinOilPress, INPUT_PULLUP);
      //pinMode(pinCLTgauge, OUTPUT);
    default:
      break;
  }
}

void initialiseAC()
{
  digitalWrite(pinAC, LOW); // initialize AC low
  alphaVars.ACOn = false;
}

void fanControl2()
{
  //fan2
    int onTemp = (int)configPage6.fanSP - CALIBRATION_TEMPERATURE_OFFSET;
    int offTemp = onTemp - configPage6.fanHyster;
    if ( ((currentStatus.fanOn) && (currentStatus.coolant >= onTemp+7) && (currentStatus.RPM > 500)) || (alphaVars.AcReq== true) ) { digitalWrite(pinFan2,fanHIGH); currentStatus.fanOn = true; }
    if ( ((!currentStatus.fanOn) && (currentStatus.coolant <= offTemp+7) && (alphaVars.AcReq== false)) || (currentStatus.RPM == 0) ) { digitalWrite(pinFan2, fanLOW); currentStatus.fanOn = false; }
}

void audiFanControl()
{
  if (currentStatus.coolant < 80){
    if (loopCLT < 10){
      digitalWrite(pinFan2,LOW);
    }
    else{
      digitalWrite(pinFan2,HIGH);
    }
  }
  else if ((currentStatus.coolant >= 80) && (currentStatus.coolant < 90)){
    if (loopCLT < 100){
      digitalWrite(pinFan2,LOW);
    }
    else{
      digitalWrite(pinFan2,HIGH);
    }
  }
  else if (currentStatus.coolant >= 90){
    if (loopCLT < 150){
      digitalWrite(pinFan2,LOW);
    }
    else{
      digitalWrite(pinFan2,HIGH);
    }
  }
}

void ACControl()
{
  if ((alphaVars.AcReq) && (currentStatus.TPS < 60) && (currentStatus.RPM > 600) && (currentStatus.RPM < 3600)){digitalWrite(pinAC, HIGH); alphaVars.ACOn = true;}// turn on AC compressor
  else{ digitalWrite(pinAC, LOW); alphaVars.ACOn = false;} // shut down AC compressor
}

void CELcontrol()
{
  if ((mapErrorCount > 4) || (cltErrorCount > 4) || (iatErrorCount > 4)  || (errorCount > 1) || (currentStatus.RPM == 0)){
    alphaVars.CELon = true;
  }
  else{
    alphaVars.CELon = false;
  }
  if (alphaVars.CELon)  {
    digitalWrite(pinCEL, HIGH);
  }
  else {digitalWrite(pinCEL, LOW);}
}


void vvlControl()
{
  if ((currentStatus.RPM >= 5800) && (currentStatus.TPS > 80) && (currentStatus.coolant > 50))
  {
    if (!alphaVars.vvlOn)
    {
      alphaVars.vvlOn = true;
      digitalWrite(pinVVL, HIGH);
    }
  }
  else if ((currentStatus.RPM <= 5600) && (currentStatus.TPS < 80)) { digitalWrite(pinVVL, LOW);  alphaVars.vvlOn = false;}
}

 void readACReq()
 {
  if (alphaVars.carSelect == 2){
    if ((digitalRead(pinAcReq) == HIGH) && (digitalRead(pinACpress) == LOW)) {alphaVars.AcReq= true;} //pin 26 is AC Request, pin 28 is a combined pressure/temp signal that is high when the A/C compressor can be activated
      else {alphaVars.AcReq= false;}
  }
  else if (alphaVars.carSelect == 1){
    if ((digitalRead(pinAcReq) == HIGH) && (digitalRead(pinACpress) == LOW) && (analogRead(pinACtemp) < 820)) {alphaVars.AcReq = true;}
    else {alphaVars.AcReq= false;}
  }
 }

 //Simple correction if VVL is active
 static inline byte correctionVVL()
{
  byte VVLValue = 100;
  if (alphaVars.vvlOn) { VVLValue = 107; } //Adds 7% fuel when VVL is active
  return VVLValue;
}


/*
 * Returns true if decelleration fuel cutoff should be on, false if its off
 */
static inline bool correctionDFCO2()
{
  bool DFCOValue = false;
  if ( configPage2.dfcoEnabled == 1 )
  {
    if ( bitRead(currentStatus.status1, BIT_STATUS1_DFCO) == 1 ) { DFCOValue = ( currentStatus.RPM > ( configPage4.dfcoRPM * 10) ) && ( currentStatus.TPS < configPage4.dfcoTPSThresh )&& (alphaVars.DFCOwait)  && (currentStatus.coolant > 60);  }
    else { DFCOValue = ( currentStatus.RPM > (unsigned int)( (configPage4.dfcoRPM * 10) + configPage4.dfcoHyster) ) && ( currentStatus.TPS < configPage4.dfcoTPSThresh )&& (alphaVars.DFCOwait)&& (currentStatus.coolant > 60); }  }
  return DFCOValue;
}

static inline int8_t correctionAtUpshift(int8_t advance)
{
  int8_t upshiftAdvance = advance;
  if ((currentStatus.rpmDOT < -500) && (currentStatus.TPS > 90)){
   upshiftAdvance = advance - 10;
  }
  return upshiftAdvance;
}

static inline int8_t correctionZeroThrottleTiming(int8_t advance)
{
  int8_t ignZeroThrottleValue = advance;
  if ((currentStatus.TPS < 2) && !(BIT_CHECK(currentStatus.engine, BIT_ENGINE_ASE))) //Check whether TPS coorelates to zero value
  {
     if ((currentStatus.RPM > 500) && (currentStatus.RPM <= 800)) { 
      ignZeroThrottleValue = map(currentStatus.RPM, 500, 800, 25, 9);
     }
     else if ((currentStatus.RPM > 800) && (currentStatus.RPM < 1600)) { 
      ignZeroThrottleValue = map(currentStatus.RPM, 800, 1200, 9, 0);
     }
     else{ignZeroThrottleValue = advance;}
    ignZeroThrottleValue = constrain(ignZeroThrottleValue , 0, 25);
     if ((currentStatus.RPM > 3000) && (currentStatus.RPM < 5500)){ ignZeroThrottleValue = -5;}
     
  }
  else if ((currentStatus.TPS < 2) && (BIT_CHECK(currentStatus.engine, BIT_ENGINE_ASE))){
    ignZeroThrottleValue = 10;
  }
  if ((alphaVars.ACOn == true) && (currentStatus.RPM < 3000) && (currentStatus.TPS < 30)) {ignZeroThrottleValue = ignZeroThrottleValue + 2;}
  return ignZeroThrottleValue;
}

void highIdleFunc(){
  //high idle function
    if ( (( currentStatus.RPM > 950 ) && ( currentStatus.TPS > 7 )) || ((currentStatus.RPM > 1150) && (currentStatus.rpmDOT < -100)) ) 
    {
      alphaVars.highIdleCount++;
      if (alphaVars.highIdleCount >= 2 ) {alphaVars.highIdleReq = true; }
    }
    else { 
      if (alphaVars.highIdleCount > 0){
        alphaVars.highIdleCount--;
      }
      else if(alphaVars.highIdleCount == 0)
      {
       alphaVars.highIdleReq = false; 
      }
     }
     alphaVars.highIdleCount = constrain(alphaVars.highIdleCount, 0, 12);
}

void DFCOwaitFunc(){
      //DFCO wait time
    if ( ( currentStatus.RPM > ( configPage4.dfcoRPM * 10) ) && ( currentStatus.TPS < configPage4.dfcoTPSThresh ) ) 
    {
      alphaVars.DFCOcounter++;
      if (alphaVars.DFCOcounter > 2 ) {alphaVars.DFCOwait = true;}
    }
    else {alphaVars.DFCOwait = false; alphaVars.DFCOcounter = 0;}
}


void XRSgaugeCLT(){
//Coolant gauge control
  if (currentStatus.coolant < 50){
    if (loopCLT < 320){
      digitalWrite(pinCLTgauge, LOW);
    }
    else {digitalWrite(pinCLTgauge, HIGH);}
   }
  else if ((currentStatus.coolant >= 50) && (currentStatus.coolant < 70)){
      if (loopCLT < 225){
        digitalWrite(pinCLTgauge, LOW);
      }
      else {digitalWrite(pinCLTgauge, HIGH);}
  }
  else if ((currentStatus.coolant >= 70) && (currentStatus.coolant < 90)){ 
    if (loopCLT < 100){
        digitalWrite(pinCLTgauge, LOW);
      }
      else {digitalWrite(pinCLTgauge, HIGH);}
    }
  else if (currentStatus.coolant >= 90){
      if (loopCLT <= 35){
        digitalWrite(pinCLTgauge, LOW);
      }
      else {digitalWrite(pinCLTgauge, HIGH);}
  }
}

void alphaIdleMods(){
  if ((BIT_CHECK(currentStatus.engine, BIT_ENGINE_ASE))){ currentStatus.idleDuty = currentStatus.idleDuty + 10;}
  if ((alphaVars.highIdleReq) && (currentStatus.idleDuty < 60)){ currentStatus.idleDuty = currentStatus.idleDuty + alphaVars.highIdleCount;}
  if (alphaVars.AcReq == true){ currentStatus.idleDuty = currentStatus.idleDuty + 10;}
}

void RPMdance(){
  //sweep tacho gauge for cool points
      if ((mainLoopCount > 30) && (mainLoopCount < 300))
      {
        tone(pinTachOut, mainLoopCount);
        analogWrite(pinTachOut,  138);
      }
      else if (mainLoopCount == 4999)
      {
        noTone(pinTachOut);
        digitalWrite(pinTachOut, LOW);
        alphaVars.gaugeSweep = false;
      }
}

uint16_t WOTdwellCorrection(uint16_t tempDwell){
  if ((currentStatus.TPS > 80) && (currentStatus.RPM > 3500)){
    uint16_t dwellCorr = map(currentStatus.RPM, 3500, 5000, 200, 1000);
    tempDwell = tempDwell - dwellCorr;
  }
  return tempDwell;
}

uint16_t boostAssist(uint16_t tempDuty){
  if ((currentStatus.TPS > 90) && (currentStatus.MAP < 120)){
    tempDuty = 9000;
  }
  return tempDuty;
}
