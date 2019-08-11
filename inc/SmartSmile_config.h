//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------
#define CHIN_BUTTON_MASK 0x01
#define TIME_BUTTON_MASK 0x02
SBIT (RED_LED,     SFR_P1, 0);          	   // '0' means ON, '1' means OFF
SBIT (YELLOW_LED,  SFR_P1, 1);        		   // '0' means ON, '1' means OFF
SBIT (GREEN_LED,   SFR_P1, 2);         		   // '0' means ON, '1' means OFF
SBIT (CHIN_BUTTON, SFR_P0, CHIN_BUTTON_MASK>>1);  // CHIN_BUTTON == 0 means switch pressed
SBIT (TIME_BUTTON, SFR_P0, TIME_BUTTON_MASK>>1);  // TIME_BUTTON == 0 means switch pressed
// !BUTTON indicates button is pressed
// BUTTON indicates button is not pressed


//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------
#define LED_ON           			1
#define LED_OFF          			0
#define SWITCH_PRESSED        		0        // Macros to determine switch state
#define SWITCH_NOT_PRESSED    		1

// With current RTC settings, RTC clock operates at ~16,384 Hz == 1<<14
#define SECOND						((U32)1 << 14)
#define MINUTE						((U32)60 << 14)
#define HOUR						((U32)3600 << 14)
#define TIME_ELAPSED_ADDRESS_START  0x1000
#define ONE_HUNDRED					1
//#define THREE_HUNDRED				1
//#define FOUR_HUNDRED				1
//#define FIVE_HUNDRED				1
