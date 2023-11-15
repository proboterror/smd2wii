#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#include <inttypes.h>
#include "defines.h"

#define GLUE(a,b) a##b
#define DDR(p) GLUE(DDR,p)
#define PORT(p) GLUE(PORT,p)
#define PIN(p) GLUE(PIN,p)

#define WAIT(t) {TCNT0=0; while(TCNT0 < (F_CPU / 1000000UL) * t);}

void gamepads_init();
void gamepads_query();

struct smd_state {
	uint16_t connected : 1, six_buttons : 1, a : 1, b : 1, c : 1, x : 1, y : 1, z : 1, start : 1, mode :1, up : 1, down : 1, left : 1, right : 1;
};

extern struct smd_state smd1;

#endif
