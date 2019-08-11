//-----------------------------------------------------------------------------
// SmartSmile project for Stanford Engineering for Extreme Affordability
// Hardware and software designed by Quint Underwood
//
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <compiler_defs.h>
#include <SI_C8051F930_Register_Enums.h>       // SFR declarations
#include "InitDevice.h"
#include "C8051F930_lib.h"                     // library declarations
#include "SmartSmile_config.h"                 // Pin declarations, global constants
#include "SmaRTClock.h"
//#include "F93x_FlashPrimitives.h"
#include "F93x_FlashUtils.h"
#include "power.h"

 //-----------------------------------------------------------------------------
 // Function PROTOTYPES
 //-----------------------------------------------------------------------------
 void enable_port_match_on_button_press(U8 mask);
 void enable_port_match_on_button_release(U8 mask);
 void start_timer();
 void stop_timer();
 void display_hours_on_LEDs(U32 hours);
 U32 get_hours_elapsed();
 void increment_hours_elapsed(U32 hours_elapsed);


//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void SiLabs_Startup (void)
{
  // Disable the watchdog here
	PCA0MD &= ~PCA0MD_WDTE__BMASK; //DISABLE WATCHDOG TIMER
}


//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
int main (void) {
	bool erase_flash;
	 bool chin_button_state;
	 bool time_button_state;
	 U32 hours_elapsed;
	 //U8 wakeup_sources;
	 erase_flash = 0;
	 // Debug Trap -- Prevents the system from entering sleep mode after a reset if Switch 2 is pressed.
	while(!TIME_BUTTON) {
		erase_flash = 1;
	}


	//Enter default mode
	enter_DefaultMode_from_RESET();

	RED_LED    = LED_OFF;                  // Initialize the state of the signals
	YELLOW_LED = LED_OFF;
	GREEN_LED  = LED_OFF;
	/*
	RED_LED = LED_ON;
		YELLOW_LED = LED_ON;
		GREEN_LED = LED_ON;
		*/

	LPM_Init();                         // Initialize Power Management
	LPM_Enable_Wakeup(PORT_MATCH);      // Enable Port Match wake-up source
	LPM_Enable_Wakeup(RTC);      		// Enable RTC wake-up source
	enable_port_match_on_button_press(CHIN_BUTTON_MASK);
	enable_port_match_on_button_press(TIME_BUTTON_MASK);


	chin_button_state = CHIN_BUTTON;
	time_button_state = TIME_BUTTON;
	RTC_WriteAlarm(SECOND);

	   if (erase_flash) {
		   FLASH_PageErase(TIME_ELAPSED_ADDRESS_START, 0);
		   hours_elapsed = 0;
		   //goal_time = THREE_HUNDRED;
	   } else {
		   hours_elapsed = get_hours_elapsed();
		   //goal_time = get_goal_time();
	   }


	//----------------------------------
	// Main Application Loop
	//----------------------------------
	while (1) {

		//Immediately go to sleep after proper initialization
	    LPM (SLEEP);

	  //-----------------------------------------------------------------------
	  // Task #1 - Handle Port Match Event
	  //-----------------------------------------------------------------------
	  if(Port_Match_Wakeup) {
		  if (chin_button_state != CHIN_BUTTON) { //CHIN_BUTTON was either pressed or released
			  chin_button_state = CHIN_BUTTON;
			  if (!chin_button_state) { //CHIN_BUTTON was pressed, run smaRTClock
				  start_timer();
				  //RED_LED = LED_ON;
				  enable_port_match_on_button_release(CHIN_BUTTON_MASK);
			  } else { //CHIN_BUTTON was released, stop smaRTClock
				  stop_timer();
				  //RED_LED = LED_OFF;
				  enable_port_match_on_button_press(CHIN_BUTTON_MASK);
			  }
		  }

		  if (time_button_state != TIME_BUTTON) { //TIME_BUTTON was either pressed or released
			  time_button_state = TIME_BUTTON;
			  //TIME_BUTTON was pressed, copy the RTC capture timer to the RTC capture variable and display time via LEDs
			  if (!time_button_state) {
				  display_hours_on_LEDs(hours_elapsed);
				  enable_port_match_on_button_release(TIME_BUTTON_MASK);
			  } else { //TIME_BUTTON was released, turn off LEDs
				  RED_LED    = LED_OFF;
				  YELLOW_LED = LED_OFF;
				  GREEN_LED  = LED_OFF;
				  enable_port_match_on_button_press(TIME_BUTTON_MASK);
			  }
		  }
		  Port_Match_Wakeup = 0;        // Reset Port Match Flag to indicate
	  }
	  if (RTC_Alarm) {
		  increment_hours_elapsed(++hours_elapsed);
		  RTC_Alarm = 0;
		  //display_hours_on_LEDs(hours_elapsed);
	  }
	}

	// NOTREACHED
	return 0;
}



U32 get_hours_elapsed() {
	U32 total_count = 0;
	U8 byte_number = 0;
	U8 byte_buffer[1] = {0};
	U8 sum_of_bits_in_byte = 0;
	U8 byte_we_read = 0;
	while (sum_of_bits_in_byte == 0) {
		U8 bit_number = 0;
		U8 temp = 0;
		FLASH_Read(byte_buffer, TIME_ELAPSED_ADDRESS_START + byte_number, 1, 0);
		byte_we_read = byte_buffer[0];
		for (bit_number=0; bit_number<8; bit_number++) {
			temp = (byte_we_read >> bit_number);
			sum_of_bits_in_byte += temp & 0x01;
		}
		byte_number++;
	}
	total_count = byte_number*8 - sum_of_bits_in_byte;
	return total_count;
}


void increment_hours_elapsed(U32 hours_elapsed) {
	if (hours_elapsed > 0) {
		U8 byte_to_modify_in_flash = (hours_elapsed - 1) >> 3; //Want to divide by 8
		U8 num_bits_to_shift = ((hours_elapsed - 1) % 8) + 1;
		U8 byte_to_write = 0xFF << num_bits_to_shift;
		FLASH_ByteWrite(TIME_ELAPSED_ADDRESS_START + byte_to_modify_in_flash, byte_to_write, 0);
	}
}


//-----------------------------------------------------------------------------
// enable_port_match_on_button_press
//-----------------------------------------------------------------------------
//
// To trigger a port match interrupt for a given button when that button is pressed
// (corresponding to logic level 0 for SmartSmile V2 hardware), we need to
// set the appropriate bit (corresponding to the button mask we pass to the function)
// in the P0MAT register to 1. When the button is pressed and reads 0, this is mismatched
// with the value of 1 stored in the register and triggers an interrupt.
// See page 227 of the C8051F93x manual for P0MAT details.
//
void enable_port_match_on_button_press(U8 mask) {
	P0MAT = P0MAT | mask;
}

//-----------------------------------------------------------------------------
// enable_port_match_on_button_release
//-----------------------------------------------------------------------------
//
// To trigger a port match interrupt for a given button when that button is released
// (corresponding to logic level 1 for SmartSmile V2 hardware), we need to
// set the appropriate bit (corresponding to the button mask we pass to the function)
// in the P0MAT register to 0. When the button is released and reads 1, this is mismatched
// with the value of 0 stored in the register and triggers an interrupt.
// See page 227 of the C8051F93x manual for P0MAT details.
//
void enable_port_match_on_button_release(U8 mask) {
	P0MAT = P0MAT & ~mask;
}

//-----------------------------------------------------------------------------
// start_timer
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void start_timer() {
	U8 RTC0CN_current = RTC_Read(RTC0CN); //get current register values
	RTC0CN_current = RTC0CN_current | RTC0CN_RTC0TR__BMASK | RTC0CN_ALRM__SET;
	RTC_Write(RTC0CN, RTC0CN_current); //mask current register values with RTC Timer Run Control to start timer
}

//-----------------------------------------------------------------------------
// stop_timer
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void stop_timer() {
	U8 RTC0CN_current = RTC_Read(RTC0CN); //get current register values
	RTC_Write(RTC0CN, RTC0CN_current & ~RTC0CN_RTC0TR__BMASK); //mask current register values with ~RTC Timer Run Control to stop timer
}


//-----------------------------------------------------------------------------
// display_hours_on_LEDs
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------


void display_hours_on_LEDs(U32 hours) {
	#ifdef ONE_HUNDRED
		if (hours < 25) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 50) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 75) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else if (hours <= 100) {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_ON;
		}
	#endif // ONE_HUNDRED

	#ifdef THREE_HUNDRED
		if (hours < 225) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 250) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 275) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else if (hours <= 300) {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_ON;
		}
	#endif //THREE_HUNDRED

	#ifdef FOUR_HUNDRED
		if (hours < 325) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 350) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 375) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else if (hours <= 400) {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_ON;
		}
	#endif //FOUR_HUNDRED

	#ifdef FIVE_HUNDRED
		if (hours < 425) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 450) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_OFF;
		}
		else if (hours < 475) {
			RED_LED = LED_ON;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else if (hours <= 500) {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_ON;
			GREEN_LED = LED_ON;
		}
		else {
			RED_LED = LED_OFF;
			YELLOW_LED = LED_OFF;
			GREEN_LED = LED_ON;
		}
	#endif //FIVE_HUNDRED
}

