#include "defines.h"
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include "gamepads.h"

struct smd_state smd1;

void gamepads_init()
{
	PORT(SMD_SELECT_PORT) |= (1 << SMD_SELECT_PIN);
	DDR(SMD_SELECT_PORT) |= (1 << SMD_SELECT_PIN);

	DDR(SMD1_DATA_PORT) &= ~(1 << SMD1_DATA_PIN0);
	DDR(SMD1_DATA_PORT) &= ~(1 << SMD1_DATA_PIN1);
	DDR(SMD1_DATA_PORT) &= ~(1 << SMD1_DATA_PIN2);
	DDR(SMD1_DATA_PORT) &= ~(1 << SMD1_DATA_PIN3);
	DDR(SMD1_DATA_PORT) &= ~(1 << SMD1_DATA_PIN4);
	DDR(SMD1_DATA_PORT) &= ~(1 << SMD1_DATA_PIN5);
	PORT(SMD1_DATA_PORT) |= (1 << SMD1_DATA_PIN0);
	PORT(SMD1_DATA_PORT) |= (1 << SMD1_DATA_PIN1);
	PORT(SMD1_DATA_PORT) |= (1 << SMD1_DATA_PIN2);
	PORT(SMD1_DATA_PORT) |= (1 << SMD1_DATA_PIN3);
	PORT(SMD1_DATA_PORT) |= (1 << SMD1_DATA_PIN4);
	PORT(SMD1_DATA_PORT) |= (1 << SMD1_DATA_PIN5);

	memset((void*)&smd1, 0, sizeof(smd1));
}

void gamepads_query()
{
	uint8_t b;

	uint8_t sega_d1[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

	// Fixed: long delay between controller state query.
	_delay_ms(5); // Must be >= 3 to give 6-button controller time to reset

	PORT(SMD_SELECT_PORT) |= (1 << SMD_SELECT_PIN);

	// Fixed: 8 iterations for read controller state.
	for (b = 0; b < 8; b++)
	{
		//_delay_us(10); // short delay to stabilise outputs in controller
		// Note: do not sleep between sequential reads (breaks 6-button pad detection).
		sega_d1[b] = ((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN0)&1)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN1)&1) << 1)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN2)&1) << 2)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN3)&1) << 3)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN4)&1) << 4)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN5)&1) << 5)
			| (((PIN(SMD_SELECT_PORT)>>SMD_SELECT_PIN)&1) << 7);

		PORT(SMD_SELECT_PORT) ^= (1 << SMD_SELECT_PIN);
	}

	smd1.connected = !(sega_d1[1] & 0b001100);

	if (smd1.connected)
	{
		// Fixed: 6-buttons controller detection:
		smd1.six_buttons =  ((sega_d1[5] & 0b011) == 0);

		smd1.a = !!(~sega_d1[1] & 0b010000);
		smd1.b = !!(~sega_d1[0] & 0b010000);
		smd1.c = !!(~sega_d1[0] & 0b100000);
		smd1.x = !!(smd1.six_buttons && (~sega_d1[6] & 0b000100));
		smd1.y = !!(smd1.six_buttons && (~sega_d1[6] & 0b000010));
		smd1.z = !!(smd1.six_buttons && (~sega_d1[6] & 0b000001));
		smd1.up = !!(~sega_d1[0] & 0b000001);
		smd1.down = !!(~sega_d1[0] & 0b000010);
		smd1.left = !!(~sega_d1[0] & 0b000100);
		smd1.right = !!(~sega_d1[0] & 0b001000);
		smd1.start = !!(~sega_d1[1] & 0b100000);
		smd1.mode = !!(smd1.six_buttons && (~sega_d1[6] & 0b001000));
	} else {
		smd1.six_buttons = 0;
		smd1.a = 0;
		smd1.b = 0;
		smd1.c = 0;
		smd1.x = 0;
		smd1.y = 0;
		smd1.z = 0;
		smd1.up = 0;
		smd1.down = 0;
		smd1.left = 0;
		smd1.right = 0;
		smd1.start = 0;
		smd1.mode = 0;
	}
}

/*
	Ported from:
	 https://github.com/MickGyver/DaemonBite-Retro-Controllers-USB/blob/master/SegaControllerUSB/SegaController32U4.cpp
	Original code confirmed working with ATMEGA32U4 based board and third party 6-button SMD controller.
	However, code below does not detect 6-button controller with 8MHz ATMEGA8A.
	Note: original code uses only 10 microseconds (µs) delay between setting the select pin and reading the button pins,
	 which seems to be incorrect (no 6-button pad controller reset delay), but somehow working well in original environment.
	See also:
	 https://github.com/jonthysell/SegaController/wiki/How-To-Read-Sega-Controllers
	 https://github.com/jonthysell/SegaController/blob/master/src/SegaController.cpp

	The 6-Button Controller
	The logic works as follows:
	The controller's state is maintained by a counter with 8 values (0-7).
	-When the select pin changes (HIGH to LOW or LOW to HIGH) the counter increments.
	-If more than 1.5 ms elapses with no change in the select pin, then the counter resets to 0.
	-Most games run at 60 frames per second and read the controller only once per frame. Reading just once every 16.6 ms gives enough time for the counter to reset to 0 before the next frame.
	-The first 4 states (0-3) report buttons like a 3-button controller. Games programmed to read a 3-button controller once per frame still work as expected.
	-The 5th state (4) reports that the controller is in fact a 6-button controller.
	-The 6th state (5) reports the buttons X, Y, Z and MODE.
	-The last two states (6-7) can be ignored for official controllers.

	Reading the 6-Button Controller
	-Loop through each state (0-7).
	-If the state is even, output LOW to the select pin. If it's odd, output HIGH.
	-Read the six input pins according to the table (LOW = the button is being pressed).
	-After the loop, wait at least 1.5ms (16.6 ms is better) so that the controller can reset.
*/
void gamepads_query2()
{
  // "Normal" Six button controller reading routine, done a bit differently in this project
  // Cycle  TH out  TR in  TL in  D3 in  D2 in  D1 in  D0 in
  // 0      LO      Start  A      0      0      Down   Up
  // 1      HI      C      B      Right  Left   Down   Up
  // 2      LO      Start  A      0      0      Down   Up      (Check connected and read Start and A in this cycle)
  // 3      HI      C      B      Right  Left   Down   Up      (Read B, C and directions in this cycle)
  // 4      LO      Start  A      0      0      0      0       (Check for six button controller in this cycle)
  // 5      HI      C      B      Mode   X      Y      Z       (Read X,Y,Z and Mode in this cycle)
  // 6      LO      ---    ---    ---    ---    ---    Home    (Home only for 8bitdo wireless gamepads)
  // 7      HI      ---    ---    ---    ---    ---    ---

	struct State
	{
		bool pinSelect;
		uint8_t ignoreCycles;
		bool connected;
		bool sixButtonMode;
	};

	static struct State state = {false, 0, false, false};

	static const uint8_t SC_CYCLE_DELAY = 10;	// Delay (µs) between setting the select pin and reading the button pins

	// Set the select pin low/high
	state.pinSelect = !state.pinSelect;

	// Following commented out code have been unwrapped:
	//(!_pinSelect) ? PORT_SELECT &= ~MASK_SELECT : PORT_SELECT |= MASK_SELECT; // Set LOW on even cycle, HIGH on uneven cycle
	// =>
	//(state.pinSelect) ? PORT(SMD_SELECT_PORT) |= (1 << SMD_SELECT_PIN) : PORT(SMD_SELECT_PORT) &= ~(1 << SMD_SELECT_PIN) ; // Set HIGH on uneven cycle, LOW on even cycle
	// =>
	if (state.pinSelect)
		PORT(SMD_SELECT_PORT) |= (1 << SMD_SELECT_PIN);
	else
		PORT(SMD_SELECT_PORT) &= ~(1 << SMD_SELECT_PIN) ; // Set HIGH on uneven cycle, LOW on even cycle

	// Short delay to stabilise outputs in controller
	_delay_us(SC_CYCLE_DELAY);

	if(state.ignoreCycles <= 0)
	{
		if(state.pinSelect) // Select pin is HIGH
		{
			if(state.connected)
			{
				// Check if six button mode is active
				if(state.sixButtonMode)
				{
				  // Read input pins for X, Y, Z, Mode
					smd1.x = 	!((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN2) & 1);
					smd1.y = 	!((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN1) & 1);
					smd1.z = 	!((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN0) & 1);
					smd1.mode = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN3) & 1);

					state.sixButtonMode = false;
					state.ignoreCycles = 2; // Ignore the two next cycles (cycles 6 and 7 in table above)
				}
				else
				{
					// Read input pins for Up, Down, Left, Right, B, C
					smd1.b = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN4) & 1);
					smd1.c = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN5) & 1);

					smd1.up = 	!((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN0) & 1);
					smd1.down = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN1) & 1);
					smd1.left = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN2) & 1);
					smd1.right = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN3) & 1);
				}
			}
			else // No Mega Drive controller is connected
			{
				// Clear current state
				smd1.six_buttons = 0;
				smd1.a = 0;
				smd1.b = 0;
				smd1.c = 0;
				smd1.x = 0;
				smd1.y = 0;
				smd1.z = 0;
				smd1.up = 0;
				smd1.down = 0;
				smd1.left = 0;
				smd1.right = 0;
				smd1.start = 0;
				smd1.mode = 0;
			}
		}
		else // Select pin is LOW
		{
			// Check if a controller is connected
			state.connected = (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN2) & 1) == 0) &&
							  (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN3) & 1) == 0);

			smd1.connected = state.connected; // ToDo: delete

			// Check for six button mode
			state.sixButtonMode = (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN0) & 1) == 0) &&
								  (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN1) & 1) == 0);

			// Read input pins for A and Start
			if(state.connected)
			{
				if(!state.sixButtonMode)
				{
					smd1.a = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN4) & 1);
					smd1.start = !((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN5) & 1);
				}
			}
		}
	}
	else
	{
		state.ignoreCycles--; // Decrease the ignore cycles counter, this cycle is unused on normal 6-button controllers
	}
}
