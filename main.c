#include "nes2wii.h"
#include "defines.h"
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include "wiimote.h"
#include "gamepads.h"

// classic controller id
const unsigned char classic_controller_id[6] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x01};

volatile int8_t jx = 0, jy = 0, rx = 0, ry = 0;
volatile uint8_t tl = 0, tr = 0;
volatile uint8_t left = 0, right = 0, up = 0, down = 0, a = 0, b = 0, x = 0, y = 0, 
				select = 0, start = 0, home = 0, l = 0, r = 0, zl = 0, zr = 0;
volatile uint8_t connected = 0;
volatile uint8_t dpad_mode = DPAD_MODE_BOTH;

// calibration data
const unsigned char cal_data[32] = {
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

void wiimote_query()
{
	wdt_reset();
	if (connected < 2)
	{
		connected = 2;
		RED_ON;
	}
	unsigned char but_dat[8]; // struct containing button data
	if (twi_reg[0xFE] == 1) // data format
	{
		but_dat[0] = 0b00000000; // RX<4:3>	LX<5:0>
		but_dat[1] = 0b00000000; // RX<2:1>	LY<5:0>
		but_dat[2] = 0b00000000; // RX<0>	LT<4:3>	RY<4:0>
		but_dat[3] = 0b00000000; // LT<2:0>	RT<4:0>
		but_dat[4] = 0b11111111; // BDR	BDD	BLT	B-	BH	B+	BRT	 1
		but_dat[5] = 0b11111111; // BZL	BB	BY	BA	BX	BZR	BDL	BDU
		but_dat[6] = 0;
		but_dat[7] = 0;
		
		but_dat[0] |= (jx / 4 + 0x20) & 0x3F;
		but_dat[1] |= (jy / -4 + 0x20) & 0x3F;
		but_dat[0] |= ((rx / 8 + 0x10) & 0x18) << 3;
		but_dat[1] |= ((rx / 8 + 0x10) & 0x06) << 5;
		but_dat[2] |= ((rx / 8 + 0x10) & 0x01) << 7;
		but_dat[2] |= (ry / -8 + 0x10) & 0x1F;
		but_dat[2] |= ((tl / 8) & 0x18) << 2;
		but_dat[3] |= ((tl / 8) & 0x07) << 5;
		but_dat[3] |= (tr / 8) & 0x1F;

		if (right) but_dat[4] &= ~(1<<7);
		if (down) but_dat[4] &= ~(1<<6);
		if (l) but_dat[4] &= ~(1<<5);
		if (select) but_dat[4] &= ~(1<<4);
		if (home) but_dat[4] &= ~(1<<3);
		if (start) but_dat[4] &= ~(1<<2);
		if (r) but_dat[4] &= ~(1<<1);
		
		if (zl) but_dat[5] &= ~(1<<7);
		if (b) but_dat[5] &= ~(1<<6);
		if (y) but_dat[5] &= ~(1<<5);
		if (a) but_dat[5] &= ~(1<<4);
		if (x) but_dat[5] &= ~(1<<3);
		if (zr) but_dat[5] &= ~(1<<2);
		if (left) but_dat[5] &= ~(1<<1);
		if (up) but_dat[5] &= ~(1<<0);
		
		but_dat[6] = 0;
		but_dat[7] = 0;
		
		if (((but_dat[4] & 0xFE) == 0xFE) && (but_dat[5] == 0xFF) && 
			(ABS(jx) < DEAD_ZONE) && (ABS(jy) < DEAD_ZONE) &&
			(ABS(rx) < DEAD_ZONE) && (ABS(ry) < DEAD_ZONE))
			GREEN_OFF;
		else
			GREEN_ON;
		
		wm_newaction(but_dat);
	} 
	else if (twi_reg[0xFE] == 3) // data format
	{
		but_dat[0] = jx + 0x80;
		but_dat[1] = rx + 0x80;
		but_dat[2] = 0x7fl - jy;
		but_dat[3] = 0x7fl - ry;
		but_dat[4] = tl;
		but_dat[5] = tr;
			
		but_dat[6] = 0b11111111; // BDR	BDD	BLT	B-	BH	B+	BRT	 1
		but_dat[7] = 0b11111111; // BZL	BB	BY	BA	BX	BZR	BDL	BDU
			
		if (right) but_dat[6] &= ~(1<<7);
		if (down) but_dat[6] &= ~(1<<6);
		if (l) but_dat[6] &= ~(1<<5);
		if (select) but_dat[6] &= ~(1<<4);
		if (home) but_dat[6] &= ~(1<<3);
		if (start) but_dat[6] &= ~(1<<2);
		if (r) but_dat[6] &= ~(1<<1);

		if (zl) but_dat[7] &= ~(1<<7);
		if (b) but_dat[7] &= ~(1<<6);
		if (y) but_dat[7] &= ~(1<<5);
		if (a) but_dat[7] &= ~(1<<4);
		if (x) but_dat[7] &= ~(1<<3);
		if (zr) but_dat[7] &= ~(1<<2);
		if (left) but_dat[7] &= ~(1<<1);
		if (up) but_dat[7] &= ~(1<<0);

		if (((but_dat[6] & 0xFE) == 0xFE) && (but_dat[7] == 0xFF) &&
			(ABS(jx) < DEAD_ZONE) && (ABS(jy) < DEAD_ZONE) &&
			(ABS(rx) < DEAD_ZONE) && (ABS(ry) < DEAD_ZONE))
			GREEN_OFF;
		else
			GREEN_ON;

		wm_newaction(but_dat);
	} 
}

int main()
{
	RED_OFF;
	GREEN_OFF;
	RED_LED_PORT_DDR |= (1<<RED_LED_PIN); // Red led, output
	GREEN_LED_PORT_DDR |= (1<<GREEN_LED_PIN); // Red led, output
	gamepads_init();
	dpad_mode = eeprom_read_byte((void*)0); // current mode
	if (dpad_mode > 2) dpad_mode = DPAD_MODE_BOTH;

	wm_init((void*)classic_controller_id, (void*)cal_data, wiimote_query);
	connected = 1;
	wdt_enable(WDTO_2S);

	while(1)
	{
		if (connected == 2)
		{
			connected = 3;
		}

		gamepads_query();

		a = smd1.c;
		b = smd1.b;
		x = smd1.y;
		y = smd1.a;
		l = smd1.x;
		r = smd1.z;
		select = smd1.mode;
		start = smd1.start;
		zl = 0;
		zr = 0;

		uint8_t tmp_up = 0;
		uint8_t tmp_down = 0;
		uint8_t tmp_left = 0;
		uint8_t tmp_right = 0;
		if (dpad_mode == DPAD_MODE_BOTH || dpad_mode == DPAD_MODE_DPAD)
		{
			tmp_up |= smd1.up;
			tmp_down |= smd1.down;
			tmp_left |= smd1.left;
			tmp_right |= smd1.right;
		}
		up = tmp_up;
		down = tmp_down;
		left = tmp_left;
		right = tmp_right;

		int temp_jx = 0;
		int temp_jy = 0;
		int temp_rx = 0;
		int temp_ry = 0;
		if (dpad_mode == DPAD_MODE_BOTH || dpad_mode == DPAD_MODE_STICK)
		{
			if (smd1.up)
				temp_jy = -127;
			if (smd1.down)
				temp_jy = 127;
			if (smd1.left)
				temp_jx = -127;
			if (smd1.right)
				temp_jx = 127;
		}

		jx = temp_jx;
		jy = temp_jy;
		rx = temp_rx;
		ry = temp_ry;

		// Need to switch mode? Hold Start+A+B for a second
		static uint16_t time = 0;
		if (start && a && b)
		{
			time++;
			if (time >= 500)
			{
				dpad_mode = (dpad_mode+1) % 3;
				eeprom_write_byte((void*)0, dpad_mode);
				RED_OFF;
				_delay_ms(1000);
				uint8_t i;
				for (i = 0; i <= dpad_mode; i++)
				{
					RED_ON;
					_delay_ms(200);
					RED_OFF;
					_delay_ms(200);
				}
				wdt_enable(WDTO_500MS);
				cli();
				while(1); // restart
			}
		} else time = 0;
	}
	return 0;
}
