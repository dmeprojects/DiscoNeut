#ifndef LED_DEFINES
#define HEADER_DEFINES

#include "stdint.h"

static uint8_t CENTER_LEDS = 20;

/*Circle array
LED: 7 - 8 - 18 - 34 - 33 - 32 - 21 - 6
*/
static uint8_t CIRCLE_LEDS[] = {6, 7, 17, 33, 32, 31, 21, 5};

/*Upper line
LED: 1 - 13
*/
static uint8_t UPPER_LINE_LEDS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}; 

/*
Center line
LED: 26 - 14
*/

static uint8_t CENTER_LINE_LEDS[] = {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};


/*Bottom line
LED: 27 - 39*/
static uint8_t LOWER_LINE_LEDS[] = {27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39};

#endif