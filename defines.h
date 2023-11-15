#ifndef _DEFINES_H_
#define _DEFINES_H_

#define F_CPU 16000000L
#define UART_BAUD 9600UL

#define TWI_PORT C
#define TWI_SCL_PIN 0
#define TWI_SDA_PIN 1

#define DETECT_PORT C
#define DETECT_PIN 2

#define SMD_SELECT_PORT A
#define SMD_SELECT_PIN 6
#define SMD1_DATA_PORT A
#define SMD1_DATA_PIN0 0
#define SMD1_DATA_PIN1 1
#define SMD1_DATA_PIN2 2
#define SMD1_DATA_PIN3 3
#define SMD1_DATA_PIN4 4
#define SMD1_DATA_PIN5 5

#define RED_LED_PORT D
#define RED_LED_PIN 6
#define GREEN_LED_PORT D
#define GREEN_LED_PIN 7

#define DEAD_ZONE 32

#endif
