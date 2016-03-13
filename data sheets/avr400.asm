;**** A P P L I C A T I O N   N O T E   A V R 4 0 0 ************************
;*
;* Title:		Low Cost A/D Converter 
;* Version:		1.0
;* Last updated:	97.07.18
;* Target:		AT90Sxxxx (All AVR Devices)
;*
;* Support E-mail:	avr@atmel.com
;*
;* Code Size		:37 words
;* Low Register Usage	:0
;* High Register Usage	:2
;* Status Flag Usage 	:1 (t flag)
;* Interrupt usage	:Timer/Counter0 overflow interrupt,
;*			 Analog comparator interrupt
;*
;* DESCRIPTION
;*
;* This application note shows how you can make a A/D converter using a AVR 
;* device, one external resistor and one external capacitor. This solution 
;* uses the Timer/Counter0 overflow interrupt in addition to the Analog 
;* comparator interrupt. The usage of interrupts free's the MCU while
;* conversion is taking place.
;*
;* To minimize the usage of external components, this A/D converter uses
;* the charging of a capacitor (controlled by port D pin 2)through a 
;* resistor while converting.
;* The voltage across the capacitor will follow an exponential curve of
;* voltage versus time. By constricting the voltage range of 
;* the converter to 2/5Vdd, the exponential curve is a good approximation
;* of a straight line. This makes it possible to simply measure the time
;* it takes before the voltage across the capacitor equals the voltage which
;* is to be converted. To do this we use the analog comparator. The 
;* comparator will give an interrupt when the voltage across the capacitor
;* rises above the measurement voltage. The output is divided into 64
;* different levels.
;*
;* To ensure correct timing the time constant of the RC-network must 
;* satisfie   512*(1/f)=-R*C*ln(1-2/5).
;* For the A/D converter to operate properly, the capacitor must be  
;* completly discharged between each conversion. This is done by allowing
;* the discharging to take a minimum of 200us.
;*  
;* 
;* *** Initialization
;*
;*  1. Call convert_init
;*  2. Enable global interrupts (with sei)
;* 
;* *** A/D conversion
;*
;*  1. Call AD_convert
;*  2. Wait for conversion complete (t to be set) (less than 521 cycles)
;*  3. Read data from result
;**************************************************************************


.include "1200def.inc"

;***** Constants

.equ	preset=192			;T/C0 Preset constant (256-64)	

;***** A/D converter Global Registers

.def	result=r16			;Result and intermediate data
.def	temp=r17			;Scratch register




;**************************************************************************
;*
;*  	PROGRAM START - EXECUTION STARTS HERE
;*
;**************************************************************************
	.cseg


	.org $0000
	rjmp RESET      ;Reset handle
	.org OVF0addr
	rjmp ANA_COMP   ;Timer0 overflow handle
	.org ACIaddr
	rjmp ANA_COMP   ;Analog comparator handle



;**************************************************************************
;*
;* ANA_COMP - Analog comparator interrupt routine
;*
;*
;* DESCRIPTION
;* This routine is executed when one of two events occur:
;* 1. Timer/counter0 overflow interrupt
;* 2. Analog Comparator interrupt
;* Both events signals the end of a conversion. Timer overflow if the signal
;* is out of range, and analog comparator if it is in range.
;* The offset will be corrected, and the t'flag will be set.
;* Due to the cycles needed for interruption handling, it is necessary
;* to subtract 1 more than was added previously.
;* 
;*
;* Total numbers of words		: 7
;* Total number of cycles		: 10
;* Low register usage			: 0
;* High register usage			: 2 (result,temp)
;* Status flags				: 1 (t flag)
;*
;**************************************************************************
ANA_COMP:       in      result,TCNT0    ;Load timer value

	        clr     temp    	;Stop timer0
		out     TCCR0,temp         

		subi    result,preset+1 ;Rescale A/D output

		cbi     PORTD,PD2       ;Start discharge
		set			;Set conversion complete flag
		
		reti                    ;Return from interrupt


;**************************************************************************
;*
;* convert_init - Subroutine for A/D converter initialization
;*
;*
;* DESCRIPTION
;* This routine initializes the A/D converter. It sets the timer and the
;* analog comparator. The analog comparator interrupt is being initiated by  
;* a rising edge on AC0. To enable the A/D converter the global interurrupt
;* flag must be set (with SEI).
;*
;* The conversion complete flag (t) is cleared.
;*
;* Total number of words		: 6
;* Total number of cycles		: 10
;* Low register usage			: 0
;* High register usage			: 1 (result)
;* Status flag usage			: 0
;* 
;**************************************************************************
convert_init:
	        ldi     result,$0B   	;Initiate comparator
		out     ACSR,result 	;and enable comparator interrupt

		
		ldi     result,$02      ;Enable timer interrupt
		out     TIMSK,result
		
		sbi     PORTD,PD2       ;Set converter charge/discharge
					;as output

		ret			;Return from subroutine



;**************************************************************************
;*
;* AD_convert - Subroutine to start an A/D conversion
;*
;* DESCRIPTION
;* This routine starts the conversion. It loads the offset value into the
;* timer0 and starts the timer. It also starts the charging of the 
;* capacitor.
;*
;*
;* Total number of words		: 7
;* Total number of cycles		: 10
;* Low register usage			: 0
;* High register usage			: 1 (result)
;* Status flag usage			: 1 (t flag)
;*
;**************************************************************************
AD_convert:
		ldi     result,preset   ;Clear counter
		out     TCNT0,result    ;and load offset value
		
		clt			;Clear conversion complete flag (t)

		ldi	result,$02	;Start timer0 with prescaling f/8
		out     TCCR0,result    
		sbi     PORTB,PB2       ;Start charging of capacitor
		

		ret			;Return from subroutine


;**************************************************************************
;*
;*	Example program
;*
;* This program can be used as an example on how to set up the A/D 
;* converter properly. 
;* NOTE! To ensure proper operation, make sure the discharging period
;* of the capacitor  is longer than 200us in front of each conversion. 	
;* The results of the conversion is presented on port B.
;* To ensure proper discharging we have added a delay loop. This loop is 
;* 11 thousand cycles. This will give a 550us delay with a 20MHz oscillator
;* (11ms with a 1MHz oscillator).
;*
;**************************************************************************

RESET:
		rcall	convert_init	;Initialize A/D converter
		sei			;Enable global interrupt
		ldi	result,$ff	;set port B as output
		out	DDRB,result	
Delay:		clr	result		;Clear temp counter 1
		ldi	temp,$f0	;Reset temp counter 2
loop1:		inc	result		;Count up temp counter 1
		brne	loop1		;Check if inner loop is finished
		inc 	temp		;Count up temp counter 2
		brne 	loop1		;Check if delay is finished

		rcall	AD_convert	;Start conversion
Wait:		brtc	Wait		;Wait until conversion is complete
		out	PORTB,result	;Write result on port B

		rjmp	Delay		;Repeat conversion