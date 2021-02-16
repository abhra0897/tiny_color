/*
MIT License
Copyright (c) 2020 Avra Mitra
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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

static volatile uint16_t g_tick_counter = 0;				//Counts ticks
static volatile uint8_t g_adc_reading = 0;					// read the adc result in ISR


/* Setting clock to 9.6MHz using CLKPR reg. */
void set_clk_9_6MHz();
/* 8bit timer0 is set to generate interrupt every 1.04us */
void set_timer_25us();
/* Read 16-bit g_tick_counter atomically */
uint16_t get_tick_25us();
/* Set up GPIOs */
void setup_gpio();
/* Set gpio values based on color value and current bit pos */
void out_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bit_pos_val);
/* configure ADC */
void setup_adc();
/* get the adc result which is stored during the interrupt (in a ISR) */
uint8_t get_adc_reading();


int main(void)
{
	set_clk_9_6MHz();
	setup_gpio();
	setup_adc();
	set_timer_25us();

	// Wait before changing the color
	uint16_t color_change_wait_ticks = 10;
	// Used for scanning bitsvoid
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
			// note: bit_pos_val also acts as pulse width
			// now wait until the required time is passed. 
			while (get_tick_25us() - bcm_encode_start_tick < bit_pos_val)
			{
				// wait here and do calculation for getting the next color in the color cycle
				
				// change color after certain time (to slow down the color ramp)
				// also, satisfy only if color_change_wait_ticks is more than 10. otherwise the colour change will be too fast
				if (get_tick_25us() - color_change_start_tick >= color_change_wait_ticks &&
					color_change_wait_ticks >= 10)
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
		color_change_wait_ticks = get_adc_reading() * 8;	//modify the wait time based on potentiometer
	}
	
	return 0;
}

ISR(ADC_vect)
{
	cli();
	g_adc_reading = ADCH; // not needed ADCL
	sei();
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

/* atomically get current tick value. updated every 25us*/
uint16_t get_tick_25us()
{
	uint16_t ret_us;
	cli();                              //disable interrupt for atomic reading
	ret_us = g_tick_counter;                   //read g_tick_counter and store to ret
	sei();                              //enable interrupt after reading is done
	return ret_us;
}

/* atomically get the adc reading */
uint8_t get_adc_reading()
{
	uint8_t adc_res;
	cli();
	adc_res = g_adc_reading;
	sei();
	return adc_res;
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
void setup_gpio()
{
	PORTB &= ~(1 << LED_RED) & ~(1 << LED_GREEN) & ~(1 << LED_BLUE);
	DDRB |= (1 << LED_RED) | (1 << LED_GREEN) | (1 << LED_BLUE);
}

/* configure ADC  */
void setup_adc()
{
	ADMUX |= 0x02; //mux1 = 1, mux0 = 0. ADC2 is selected
	ADMUX |= (1 << ADLAR); //left adjust the data cause we're happy with the 8bit in ADCH only
	
	
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //prescaler = 128 
	ADCSRA |= (1 << ADIE); // enable interrupt
	ADCSRA |= (1 << ADATE); //ADC auto trigger enable, free running mode
	ADCSRA |= (1 << ADEN); //enable ADC
	DIDR0 |= (1 << ADC2D); //disable digital input on that pins
	
	ADCSRA |= (1 << ADSC); //start the conversion in free running mode
	sei();
}
