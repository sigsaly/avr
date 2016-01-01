/*
 * rpm.c
 *
 * Created: 2015-12-19 오후 9:30:54
 * Author : sela
 */ 


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
//#define F_CPU     8000000L
//#include <util/delay.h>

#define DECIMAL_MODE
//#define HEXADECIMAL_MODE

//#define CC_MODE
#define CA_MODE

#define PULSES_PER_REV 12

volatile uint32_t g_tcount = 0;
volatile uint16_t disp_value = 0;
volatile uint8_t pulses = 0;
volatile uint32_t stop_cnt = 0;

uint32_t get_millies(void);

//              0  1  2  3  4  5  6  7  8  9  A  b  C  d  E  F
//PB7 -10 (F)   1  0  0  0  1  1  1  1  1  1  1  1  1  0  1  1
//PB6 -11 (A)   1  0  1  1  0  1  1  1  1  1  1  0  1  0  1  1
//PB5 - 1 (E)   1  0  1  0  0  0  1  0  1  0  1  1  1  1  1  1
//PB4 - 2 (D)   1  0  1  1  0  1  1  0  1  0  0  1  1  1  1  0
//PB3 - 3 (DP)  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
//PB2 - 4 (C)   1  1  0  1  1  1  1  1  1  1  1  1  0  1  0  0
//PB1 - 5 (G)   0  0  1  1  1  1  1  0  1  1  1  1  0  1  1  1
//PB0 - 7 (B)   1  1  1  1  1  0  0  1  1  1  1  0  0  1  0  0
//             F5 05 73 57 87 D6 F6 C5 F7 C7 E7 B6 F0 37 F2 E2

uint8_t num[] =
{
	0xF5, 0x05, 0x73, 0x57, 0x87, 0xD6, 0xF6, 0xC5,
	0xF7, 0xC7, 0xE7, 0xB6, 0xF0, 0x37, 0xF2, 0xE2
};

#ifdef CC_MODE
uint8_t seg[] =
{
	(1<<PD3), (1<<PD4), (1<<PD5), (1<<PD7)
};
#else
uint8_t seg[] =
{
	~(1<<PD3)&((1<<PD3)|(1<<PD4)|(1<<PD5)|(1<<PD7)),
	~(1<<PD4)&((1<<PD3)|(1<<PD4)|(1<<PD5)|(1<<PD7)),
	~(1<<PD5)&((1<<PD3)|(1<<PD4)|(1<<PD5)|(1<<PD7)),
	~(1<<PD7)&((1<<PD3)|(1<<PD4)|(1<<PD5)|(1<<PD7)),
};
#endif

int main(void)
{
	uint32_t millies;
	uint32_t old_millies = 0;
	
	DDRB = 0xff;
	DDRD = (1<<PD3)|(1<<PD4)|(1<<PD7)|(1<<PD5);
	
	// CTC mode
	TCCR0A = (1<<WGM01);
	// clk source, 8MHz/8 = 500kHz
	TCCR0B = (1<<CS01);
	// output compare value for 100us
	OCR0A = 100;  // 10kHz
	TIMSK0 |= (1<<OCIE0A);
	
	EICRA = (1<<ISC01); // int0, falling-edge interrupt
	EIMSK = (1<<INT0); // int0 enabled
	
	sei();
	
	while (1)
	{
		if(pulses >= PULSES_PER_REV)
		{
			millies = get_millies();
	
			disp_value = 60000/(millies - old_millies);
			old_millies = millies;			
			pulses = 0;
		}
		stop_cnt++;
		if(stop_cnt > 50000)
			disp_value = 0;
	}
}

ISR (TIMER0_COMPA_vect)
{
	static uint8_t count = 0;
	static uint8_t index = 0;
	uint16_t dat=0;
	
#ifdef DECIMAL_MODE
	uint16_t  x1,x2,x3;
	uint16_t  r1,r2,r3;
#endif
	
	count++;
	if(count == 50){ //200Hz - 5us pulse x 4 = 20ms - 50Hz
		count = 0;
		
#ifdef HEXADECIMAL_MODE
		switch(index){
			case 0:
				dat = disp_value&0xf000;
				dat >>= 12;
				break;
			case 1:
				dat = disp_value&0xf00;
				dat >>= 8;
				break;
			case 2:
				dat = disp_value&0xf0;
				dat >>= 4;
				break;
			case 3:
				dat = disp_value&0xf;
				break;
		}
#else
		x1 = disp_value/1000;
		r1 = disp_value - x1*1000;
		x2 = r1/100;
		r2 = r1 - x2*100;
		x3 = r2/10;
		r3 = r2 - x3*10;

		switch(index){
			case 0:
				dat = x1;
				break;
			case 1:
				dat = x2;
				break;
			case 2:
				dat = x3;
				break;
			case 3:
				dat = r3;
				break;
		}

#endif
		
#ifdef CC_MODE
		PORTB = num[dat];
#else
		PORTB = ~num[dat];
#endif
		PORTD = seg[index];
		index++;
		if(index==4)
			index = 0;
	}
	g_tcount++;
}

ISR (INT0_vect)
{
	pulses++;
	stop_cnt = 0;
}

uint32_t get_millies(void)
{
	return g_tcount/10;
}


