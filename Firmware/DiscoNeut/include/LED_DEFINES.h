#ifndef LED_DEFINES
#define HEADER_DEFINES

#include "stdint.h"

//static uint8_t TOTALLEDS    39

static uint8_t UPPER_CENTER_LED = 6;
static uint8_t CENTER_LED = 19;
static uint8_t LOWER_CENTER_LED = 32;

/*Circle array
LED: 7 - 8 - 18 - 34 - 33 - 32 - 21 - 6
*/
static uint8_t CIRCLE_LEDS[] = {6, 7, 17, 33, 32, 31, 21, 5};

static uint8_t QUARTER_CIRCLE_LEDS[] = {5, 7, 31, 33};

/*Upper line
LED: 1 - 13
*/
static uint8_t UPPER_LINE_LEDS[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 

//Leds from center
static uint8_t UPPER_LINE_LEDS_LEFT[] = {5, 4, 3, 2, 1, 0};
static uint8_t UPPER_LINE_LEDS_RIGHT[] = {6, 7, 8, 9, 10, 11, 12};

/*
Center line
LED: 26 - 14
*/

static uint8_t CENTER_LINE_LEDS[] = {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
//Leds from center
static uint8_t CENTER_LINE_LEDS_LEFT[] = {20, 21, 22, 23, 24, 25};
static uint8_t CENTER_LINE_LEDS_RIGHT[] = {18, 17, 16, 15, 14, 13};


/*Bottom line
LED: 27 - 39*/
static uint8_t LOWER_LINE_LEDS[] = {27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39};
static uint8_t LOWER_LINE_LEDS_LEFT[] = {31, 30, 29, 28, 27, 26};
static uint8_t LOWER_LINE_LEDS_RIGHT[] = {33, 34, 35, 36, 37, 38};

/*Standard color defines*/
#define     ORANGE              0xFF9900
#define     GREEN               0x00FF00
#define     LIGHTGREEN          0x00FF99
#define     RED                 0xFF0000
#define     BLUE                0x0000FF
#define     LIGHTBLUE           0xFF66FF
#define     PURPLE              0xCC00FF
#define     WHITE               0xFFFFFF
#define     PINK                0xFF66FF
#define     DARKPINK            0xFF0066
#define     APPLEBLUESEAGREEN   0xFF66FF
#define     YELLOW              0xFFFF00
#define     BLACK               0x000000

#endif