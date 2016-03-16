/** \file
  \brief I/O primitives.
*/

#ifndef _PINIO_H
#define _PINIO_H

#include <avr/io.h>

#ifndef MASK
  /// MASKING- returns \f$2^PIN\f$
  #define MASK(PIN) (1 << PIN)
#endif


/** Magic I/O routines, also known as "FastIO".

  Now you can simply SET_OUTPUT(STEP); WRITE(STEP, 1); WRITE(STEP, 0);.

  The point here is to move any pin/port mapping calculations into the
  preprocessor. This way there is no longer math at runtime neccessary, all
  instructions melt into a single one with fixed numbers.

  This makes code for setting a pin small, smaller than calling a subroutine.
  It also make code fast, on AVR a pin can be turned on and off in just two
  clock cycles.
*/
/// Read a pin.
#define _READ(IO)        (IO ## _RPORT & MASK(IO ## _PIN))
/// Write to a pin.
#define _WRITE(IO, v) \
  do { \
    if (v) { IO ## _WPORT |= MASK(IO ## _PIN); } \
    else { IO ## _WPORT &= ~MASK(IO ## _PIN); } \
  } while (0)

/**
  Setting pins as input/output: other than with ARMs, function of a pin
  on AVR isn't given by a dedicated function register, but solely by the
  on-chip peripheral connected to it. With the peripheral (e.g. UART, SPI,
  ...) connected, a pin automatically serves with this function. With the
  peripheral disconnected, it automatically returns to general I/O function.
*/
/// Set pin as input.
#define _SET_INPUT(IO)   do { IO ## _DDR &= ~MASK(IO ## _PIN); } while (0)
/// Set pin as output.
#define _SET_OUTPUT(IO)  do { IO ## _DDR |=  MASK(IO ## _PIN); } while (0)

/// Enable pullup resistor.
#define _PULLUP_ON(IO)   _WRITE(IO, 1)
/// Disable pullup resistor.
#define _PULLUP_OFF(IO)  _WRITE(IO, 0)

/**
  Why double up on these macros?
  See http://gcc.gnu.org/onlinedocs/cpp/Stringification.html
*/
/// Read a pin wrapper.
#define READ(IO)        _READ(IO)
/// Write to a pin wrapper.
#define WRITE(IO, v)    _WRITE(IO, v)

/// Set pin as input wrapper.
#define SET_INPUT(IO)   _SET_INPUT(IO)
/// Set pin as output wrapper.
#define SET_OUTPUT(IO)  _SET_OUTPUT(IO)

/// Enable pullup resistor.
#define PULLUP_ON(IO)   _PULLUP_ON(IO)
/// Disable pullup resistor.
#define PULLUP_OFF(IO)  _PULLUP_OFF(IO)


/**
  Here we map used pins to I/O ports and their pin number inside this port.
*/
// Yellow LED on PD6.
#define LED_Y_PIN       PIND6
#define LED_Y_RPORT     PIND
#define LED_Y_WPORT     PORTD
#define LED_Y_DDR       DDRD
#define LED_Y_PWM       NULL

// Green LED on PB2.
#define LED_G_PIN       PINB6
#define LED_G_RPORT     PINB
#define LED_G_WPORT     PORTB
#define LED_G_DDR       DDRB
#define LED_G_PWM       NULL

//EEEEEKKKK
//EEEEEKKKK
//EEEEEKKKK
//EEEEEKKKK
#undef PD6
#define PD6_PIN			PIND6
#define PD6_RPORT		PIND
#define PD6_WPORT		PORTD
#define PD6_DDR			DDRD
#define PD6_PWM			&OCR2B

#undef PD7
#define PD7_PIN			PIND7
#define PD7_RPORT		PIND
#define PD7_WPORT		PORTD
#define PD7_DDR			DDRD
#define PD7_PWM			&OCR2A

#endif /* _PINIO_H */
