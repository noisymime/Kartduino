/*
  Speeduino - Simple engine management for the Arduino Mega 2560 platform
  Copyright (C) Josh Stewart
  A full copy of the license may be found in the projects root directory
*/

#include <avr/pgmspace.h>
#include "globals.h"
#include "utilities.h"
#include "decoders.h"
#include "comms.h"

uint16_t ioDelay[sizeof(configPage13.outputPin)];
uint8_t pinIsValid = 0;


//This function performs a translation between the pin list that appears in TS and the actual pin numbers
//For the digital IO, this will simply return the same number as the rawPin value as those are mapped directly.
//For analog pins, it will translate them into the currect internal pin number
byte pinTranslate(byte rawPin)
{
  byte outputPin = rawPin;
  if(rawPin > BOARD_MAX_DIGITAL_PINS) { outputPin = A8 + (outputPin - BOARD_MAX_DIGITAL_PINS - 1); }

  return outputPin;
}


void setResetControlPinState()
{
  BIT_CLEAR(currentStatus.status3, BIT_STATUS3_RESET_PREVENT);

  /* Setup reset control initial state */
  switch (resetControl)
  {
    case RESET_CONTROL_PREVENT_WHEN_RUNNING:
      /* Set the reset control pin LOW and change it to HIGH later when we get sync. */
      digitalWrite(pinResetControl, LOW);
      BIT_CLEAR(currentStatus.status3, BIT_STATUS3_RESET_PREVENT);
      break;
    case RESET_CONTROL_PREVENT_ALWAYS:
      /* Set the reset control pin HIGH and never touch it again. */
      digitalWrite(pinResetControl, HIGH);
      BIT_SET(currentStatus.status3, BIT_STATUS3_RESET_PREVENT);
      break;
    case RESET_CONTROL_SERIAL_COMMAND:
      /* Set the reset control pin HIGH. There currently isn't any practical difference
         between this and PREVENT_ALWAYS but it doesn't hurt anything to have them separate. */
      digitalWrite(pinResetControl, HIGH);
      BIT_CLEAR(currentStatus.status3, BIT_STATUS3_RESET_PREVENT);
      break;
  }
}

//*********************************************************************************************************************************************************************************
void initialiseProgrammableIO()
{
  for (uint8_t y = 0; y < sizeof(configPage13.outputPin); y++)
  {
    if ( (configPage13.outputPin[y] > 0) && (configPage13.outputPin[y] < BOARD_MAX_DIGITAL_PINS) )
    {
      if ( !pinIsUsed(configPage13.outputPin[y]) )
      {
        pinMode(configPage13.outputPin[y], OUTPUT);
        digitalWrite(configPage13.outputPin[y], (configPage13.outputInverted & (1U << y)));
        BIT_SET(pinIsValid, y);
      }
    }
  }
}

void checkProgrammableIO()
{
  int16_t data, data2;
  bool firstCheck, secondCheck;

  for (uint8_t y = 0; y < sizeof(configPage13.outputPin); y++)
  {
    firstCheck = false;
    secondCheck = false;
    if ( BIT_CHECK(pinIsValid, y) ) //if outputPin == 0 it is disabled
    { 
      //byte theIndex = configPage13.firstDataIn[y];
      data = ProgrammableIOGetData(configPage13.firstDataIn[y]);
      data2 = configPage13.firstTarget[y];

      if ( (configPage13.operation[y].firstCompType == COMPARATOR_EQUAL) && (data == data2) ) { firstCheck = true; }
      else if ( (configPage13.operation[y].firstCompType == COMPARATOR_NOT_EQUAL) && (data != data2) ) { firstCheck = true; }
      else if ( (configPage13.operation[y].firstCompType == COMPARATOR_GREATER) && (data > data2) ) { firstCheck = true; }
      else if ( (configPage13.operation[y].firstCompType == COMPARATOR_GREATER_EQUAL) && (data >= data2) ) { firstCheck = true; }
      else if ( (configPage13.operation[y].firstCompType == COMPARATOR_LESS) && (data < data2) ) { firstCheck = true; }
      else if ( (configPage13.operation[y].firstCompType == COMPARATOR_LESS_EQUAL) && (data <= data2) ) { firstCheck = true; }

      if (configPage13.operation[y].bitwise != BITWISE_DISABLED)
      {
        if ( configPage13.secondDataIn[y] < LOG_ENTRY_SIZE ) //Failsafe check
        {
          data = ProgrammableIOGetData(configPage13.secondDataIn[y]);
          data2 = configPage13.secondTarget[y];
          
          if ( (configPage13.operation[y].secondCompType == COMPARATOR_EQUAL) && (data == data2) ) { secondCheck = true; }
          else if ( (configPage13.operation[y].secondCompType == COMPARATOR_NOT_EQUAL) && (data != data2) ) { secondCheck = true; }
          else if ( (configPage13.operation[y].secondCompType == COMPARATOR_GREATER) && (data > data2) ) { secondCheck = true; }
          else if ( (configPage13.operation[y].secondCompType == COMPARATOR_GREATER_EQUAL) && (data >= data2) ) { secondCheck = true; }
          else if ( (configPage13.operation[y].secondCompType == COMPARATOR_LESS) && (data < data2) ) { secondCheck = true; }
          else if ( (configPage13.operation[y].secondCompType == COMPARATOR_LESS_EQUAL) && (data <= data2) ) { secondCheck = true; }

          if (configPage13.operation[y].bitwise == BITWISE_AND) { firstCheck &= secondCheck; }
          if (configPage13.operation[y].bitwise == BITWISE_OR) { firstCheck |= secondCheck; }
          if (configPage13.operation[y].bitwise == BITWISE_XOR) { firstCheck ^= secondCheck; }
        }
      }
      

      if ( (firstCheck == true) && (configPage13.outputDelay[y] != 0) && (configPage13.outputDelay[y] < 255) )
      {
        if ( (ioDelay[y] >= configPage13.outputDelay[y]) )
        {
          if (configPage13.outputPin[y] <= 128) { digitalWrite(configPage13.outputPin[y], (configPage13.outputInverted & (1U << y)) ^ firstCheck); }
        }
        else { ioDelay[y]++; }
      }
      else
      {
        if ( configPage13.outputPin[y] <= 128 ) { digitalWrite(configPage13.outputPin[y], (configPage13.outputInverted & (1U << y)) ^ firstCheck); }
        if ( firstCheck == false ) { ioDelay[y] = 0; }
      }
      if ( firstCheck == true ) { BIT_SET(currentStatus.outputsStatus, y); }
      else { BIT_CLEAR(currentStatus.outputsStatus, y); }
    }
    else { BIT_CLEAR(currentStatus.outputsStatus, y); }
  }
}

int16_t ProgrammableIOGetData(uint16_t index)
{
  int16_t result;
  uint8_t x;
  if ( index < LOG_ENTRY_SIZE )
  {
    
    for(x = 0; x<sizeof(fsIntIndex); x++)
    {
      if (fsIntIndex[x] == index) { break; }
    }
    if (x >= sizeof(fsIntIndex)) { result = getStatusEntry(index); }
    else { result = word(getStatusEntry(index+1), getStatusEntry(index)); }
    

    //Special cases for temperatures
    if( (index == 6) || (index == 7) ) { result -= CALIBRATION_TEMPERATURE_OFFSET; }
  }
  else { result = -1; } //Index is bigger than fullStatus array
  return result;
}