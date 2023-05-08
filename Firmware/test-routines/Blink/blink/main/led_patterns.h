#ifndef LED_PATTERNS
#define LED_PATTERNS



/*Defines*/

/*Prototypes defines*/
/*
Function draws a circle on the screen

circle contains of 3 leds
*/
//void leds_draw_circle(void);

int drawCircle ( void );

int scrollCircle ( void);

void readPattern (uint8_t lPattern, uint8_t lRuns, uint32_t lDelayMs);


#endif