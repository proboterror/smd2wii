#include "defines.h"
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
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
}

void gamepads_query()
{
	uint8_t b;

#ifdef SMD_SELECT_PORT
#ifdef SMD1_DATA_PORT
	uint8_t sega_d1[10] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
#endif

	PORT(SMD_SELECT_PORT) |= (1 << SMD_SELECT_PIN);
	for (b = 0; b < 10; b++)
	{
		_delay_us(10);
#ifdef SMD1_DATA_PORT
		sega_d1[b] = ((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN0)&1) 
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN1)&1) << 1)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN2)&1) << 2)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN3)&1) << 3)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN4)&1) << 4)
			| (((PIN(SMD1_DATA_PORT)>>SMD1_DATA_PIN5)&1) << 5)
			| (((PIN(SMD_SELECT_PORT)>>SMD_SELECT_PIN)&1) << 7);
#endif
		PORT(SMD_SELECT_PORT) ^= (1 << SMD_SELECT_PIN);
	}
#ifdef SMD1_DATA_PORT
	smd1.connected = !(sega_d1[1] & 0b001100);
	if (smd1.connected)
	{
		smd1.six_buttons = !(sega_d1[5] & 0b001111) && !(~sega_d1[7] & 0b001111);
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
#endif
#endif
}
