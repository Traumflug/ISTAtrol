/** \file pinio.h

  Magic I/O routines, also known as "FastIO".

  Now you can simply SET_OUTPUT(STEP); WRITE(STEP, 1); WRITE(STEP, 0);.

  The point here is to move any pin/port mapping calculations into the
  preprocessor. This way there is no longer math at runtime neccessary, all
  instructions melt into a single one with fixed numbers.

  This makes code for setting a pin small, smaller than calling a subroutine.
  It also make code fast, on AVR a pin can be turned on and off in just two
  clock cycles.
*/
/*
  Copyright (C) 2016 Markus "Traumflug" Hitter <mah@jump-ing.de>

  This program is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _PINIO_H
#define _PINIO_H

#include <avr/io.h>


#ifndef MASK
  /// MASKING- returns \f$2^PIN\f$
  #define MASK(PIN) (1 << PIN)
#endif


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

// Temperature sensor on the ISTA counter.
// Currently PD3, which likely changes, as this pin is also INT1.
#define TEMP_C_PIN      PIND3
#define TEMP_C_RPORT    PIND
#define TEMP_C_WPORT    PORTD
#define TEMP_C_DDR      DDRD
#define TEMP_C_PWM      NULL

// Temperature sensor on the radiator valve.
#define TEMP_V_PIN      PIND4
#define TEMP_V_RPORT    PIND
#define TEMP_V_WPORT    PORTD
#define TEMP_V_DDR      DDRD
#define TEMP_V_PWM      NULL

// Temperature sensor room.
#define TEMP_R_PIN      PIND5
#define TEMP_R_RPORT    PIND
#define TEMP_R_WPORT    PORTD
#define TEMP_R_DDR      DDRD
#define TEMP_R_PWM      &OC0B

// Valve motor, open direction.
#define MOT_OPEN_PIN    PINB3
#define MOT_OPEN_RPORT  PINB
#define MOT_OPEN_WPORT  PORTB
#define MOT_OPEN_DDR    DDRB
#define MOT_OPEN_PWM    &OC1A

// Valve motor, close direction.
#define MOT_CLOSE_PIN   PINB4
#define MOT_CLOSE_RPORT PINB
#define MOT_CLOSE_WPORT PORTB
#define MOT_CLOSE_DDR   DDRB
#define MOT_CLOSE_PWM   &OC1B

#endif /* _PINIO_H */
