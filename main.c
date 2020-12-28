#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#ifndef F_CPU
	#define F_CPU 9600000UL
#endif

/* LED RBG pins */
#define    LED_GREEN   PB1
#define    LED_BLUE    PB0
#define    LED_RED     PB2
/* Software PWM Total time = on time + off time (in us) */
#define TOTAL_TIME 1024

static volatile uint16_t g_tick_counter = 0;                //Counts ticks


/* Setting clock to 9.6MHz using CLKPR reg. */
void set_clk_9_6MHz();
/* 8bit timer0 is set to generate interrupt every 1.04us */
void set_timer_25us();
/* Read 16-bit g_tick_counter atomically */
uint16_t get_tick_25us();
/* Software PWM control. Pass R,G,B on time. Max 100us */
void soft_pwm_RGB(int R_on_time, int G_on_time, int B_on_time);
/* Set up GPIOs */
void setup_GPIO();
/* Set gpio values based on color value and current bit pos */
void out_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bit_pos_val);



int main(void)
{
    set_clk_9_6MHz();
    setup_GPIO();
    set_timer_25us();

	// Wait before changing the color
	uint16_t color_change_wait_ticks = 256;
	// Used for scanning bits
	uint8_t bit_pos_val = 0x80;
	// main colour value
    uint8_t r = 0, g = 0, b = 0;
	// temporary colour value for calculation
	uint8_t red_temp = 255, green_temp = 0, blue_temp = 0;
	
	uint16_t bcm_encode_start_tick = 0;
	uint16_t color_change_start_tick = 0;
	
	// main loop
    while(1)
    {
		// loop until all 8 bits are modulated
		// each counter increment  = 25us. so, total loop time = 2^8 * 25 = 6375us
		while (bit_pos_val >= 1)
		{
			// set gpios based on current bit's weighted value and color value'
			out_rgb(r, g, b, bit_pos_val);
			bcm_encode_start_tick = get_tick_25us();
			// note: bit_pos_val also acts as wait time
			// now wait until the required time is passed. 
			while (get_tick_25us() - bcm_encode_start_tick < bit_pos_val)
			{
				// wait here and do calculation for getting the next color in the color cycle
				
				// change color after certain time (to slow down the color ramp)
				if (get_tick_25us() - color_change_start_tick >= color_change_wait_ticks)
				{
					if (red_temp == 255 && blue_temp == 0 && green_temp < 255)
						green_temp++;
					else if ( green_temp == 255 && blue_temp == 0 && red_temp > 0)
						red_temp--;
					else if (red_temp == 0 && green_temp == 255 && blue_temp < 255)
						blue_temp++;
					else if (red_temp == 0 && blue_temp == 255 && green_temp > 0)
						green_temp--;
					else if (green_temp == 0 && blue_temp == 255 && red_temp < 255)
						red_temp++;
					else if (red_temp == 255 && green_temp == 0 && blue_temp > 0)
						blue_temp--;
					
					color_change_start_tick = get_tick_25us();
				}
			}
			bit_pos_val >>= 1;	//go to next bit
		}
		bit_pos_val = 0x80;	//all bits are traversed, now go back to MSB
		
		// update colors only after all bits are displayed
		r = red_temp;
		g = green_temp;
		b = blue_temp;
		
    }
    
    return 0;
}

ISR(TIM0_COMPA_vect)
{
    cli();                              //Disable interrupt for atomic writing
    g_tick_counter++;                   //increment g_tick_counter
    TCNT0 = 0;                          //Reset timer counter to 0
    sei();
}

void set_clk_9_6MHz()
{
    /* Clear interrupt */
    cli();
    
    /* 1000000
    *Write the Clock Prescaler Change Enable (CLKPCE) bit (8th) to one and all other bits in
    *CLKPR to zero.
    */
    CLKPR = 0x80; 

    /*
    *Within four cycles, write the desired value to CLKPS while writing a zero to CLKPCE.
    *Here, CLKPS[3:0] are zero to have prescaler 1
    */
    CLKPR = 0x00;
}


/* Fire interrupt every 25us */
void set_timer_25us()
{
	/* It takes 25us to count up to 239 at 9.6MHz  (timer_count = (required delay / clk period) - 1)*/ 
    OCR0A = 239;//for 1us, use 9 (gives 1.04us)
    /* Interrupt on compare match */
    TIMSK0 |= (1 <<  OCIE0A);
    TCNT0 = 0x00;                      //Initialize counter
    /* Set prescaler to Fclk/1 and starts timer0*/
    TCCR0B |= (1 << CS00);
    sei();                             //enable interrupt
}

/* get current tick value. updated every 25us*/
uint16_t get_tick_25us()
{
    uint16_t ret_us;
    cli();                              //disable interrupt for atomic reading
    ret_us = g_tick_counter;                   //read g_tick_counter and store to ret
    sei();                              //enable interrupt after reading is done
    return ret_us;
}

/* set GPIO values */
void out_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bit_pos_val)
{
	uint8_t temp_portb_reg = 0; 
	
	if (b & bit_pos_val)
		temp_portb_reg |= (1 << LED_BLUE); 
	if (g & bit_pos_val)
		temp_portb_reg |= (1 << LED_GREEN); 
	if (r & bit_pos_val)
		temp_portb_reg |= (1 << LED_RED); 
	
	PORTB = temp_portb_reg; 
}


/* configure gpio */
void setup_GPIO()
{
    PORTB &= ~(1 << LED_RED) & ~(1 << LED_GREEN) & ~(1 << LED_BLUE);
    DDRB |= (1 << LED_RED) | (1 << LED_GREEN) | (1 << LED_BLUE);
}
