//=========================================================
// src/Interrupts.c: generated by Hardware Configurator
//
// This file will be regenerated when saving a document.
// leave the sections inside the "$[...]" comment tags alone
// or they will be overwritten!
//=========================================================


// USER INCLUDES
#include <SI_C8051F930_Register_Enums.h>
//#include "SmartSmile_config.h"
//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

SBIT (RED_LED,    SFR_P1, 0);          // '0' means ON, '1' means OFF

//-----------------------------------------------------------------------------
// TIMER2_ISR
//-----------------------------------------------------------------------------
//
// TIMER2 ISR Content goes here. Remember to clear flag bits:
// TMR2CN::TF2H (Timer # High Byte Overflow Flag)
// TMR2CN::TF2L (Timer # Low Byte Overflow Flag)
//
//-----------------------------------------------------------------------------
INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
{
	TMR2CN_TF2H = 0;                           // clear Timer2 interrupt flag
	RED_LED = !RED_LED;                 // change state of LED
}



