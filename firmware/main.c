/*
 * Name: main.c
 * Project: AVR USB driver for CDC-SPI on Low-Speed USB
 *              for ATtiny45/85
 * Author: Osamu Tamura
 * Adjusted for Generation 7 Electronics
 * Creation Date: 2010-01-10
 * Copyright: (c) 2010 by Recursion Co., Ltd.
 * Copyright: (c) 2012 by Markus Hitter <mah@jump-ing.de>
 * License: Proprietary, free under certain conditions. See Documentation.
 */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "pinio.h"


/**
  Using continuous calibration is much smaller (36 bytes, in osctune.h, vs.
  194 bytes for reset-time calibration, osccal.c) and ensures working USB for
  elongated periods, but also occupies 8-bit Timer 0.
*/
uint8_t lastTimer0Value; // See osctune.h.

/**
  We don't need to store much status because we don't implement multiple chunks
  in read/write transfers.
*/
static uint8_t reply_buffer[8];

/**
  Our last temperature measurements.
*/
static uint16_t temp_c = 0;
static uint16_t temp_v = 0;
static uint16_t temp_r = 0;
static uint8_t conversion_done = 0;


/* ---- USB related functions --------------------------------------------- */

/**
  We use control transfers to exchange data, up to 7 bytes at a time. As we
  don't have to comply with any standards, we can use all fields freely,
  except bmRequestType. This is probably the smallest possible implementation,
  as we don't need to implement regular read or write requests.

  These fields match the ones on terminal.py, for limitations see there.

    typedef struct usbRequest {
      uchar       bmRequestType;
      uchar       bRequest;
      usbWord_t   wValue;
      usbWord_t   wIndex;
      usbWord_t   wLength;
    } usbRequest_t;
*/
usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  uint8_t len = 0;
  // Cast to structured data for parsing.
  usbRequest_t *rq = (void *)data;

  if (rq->bRequest == 'c') {
    // TODO: copying 16 bits into 2x 8 bits this way is inefficient.
    reply_buffer[0] = temp_c & 0x00FF;
    reply_buffer[1] = (temp_c & 0xFF00) >> 8;
    len = 2;
  }

  usbMsgPtr = reply_buffer;
  return len;
}

/**
  Poll USB while doing nothing for sufficient time to allow the ADC capacitor
  to discharge. If there's something to do on the USB bus, the delay can be
  considerably longer.
*/
static void poll_a_second(void) {
  uint8_t i;

  // Count to at least 5, else binary size grows significantly (50 bytes).
  for (i = 0; i < 5; i++) {
    usbPoll();
    _delay_ms(40);
  }
}

/* ---- Temperature measurements ------------------------------------------ */

/**
  Initialise temperature measurements by the Analog Comparator.
*/
static void temp_init(void) {

  /**
    The Analog Comparator can compare to an external voltage reference
    connected to AIN0 (pin 12, PB0) or to an internal voltage reference.
    For now we use the external one, as our board provides such a thing.

    Analog Comparator and its interrupt is enabled all the time, we protect
    against taking unwanted triggers into account in the interrupt routine.
  */
  ACSR = (1 << ACIE) | (1 << ACIS0) | (1 << ACIS1);

  // Start Timer 1 with prescaling f/8.
  TCCR1B = (1 << CS11);

  SET_OUTPUT(TEMP_C);
  WRITE(TEMP_C, 0);
}

/**
  Measure temperature sensor C.

  Measuring temperature works by loading a capacitor with the thermistor in
  series while running a timer at the same time. The higher the resistance of
  thermistor, the slower the capacitor loads, the higher the counter counts.
  If the cap is sufficiently full, Analog Comparator triggers an interrupt to
  catch the counter value, measurement done.

  Currently we have a voltage divider on board, delivering 1.08 volts to AIN0.
  Capacitor is 1 uF. With the thermistor at 30 kOhms, we get values of
  around 13500, so 14 significant bits. Such resolution is plenty, even with
  an ordinary resistor replacing the thermistor we still measure jitter of
  about 100 digits. Higher temperatures give lower numbers.

  A measurement with these 30 kOhms (about the highest value we expect) takes
  about 10 ms. After that the capacitor should discharge for at least 50 ms,
  better 100 ms, so we can do some 6 measurements per second.
*/
static void temp_measure(void) {

  // Clear Timer 1. Write the high byte first to make it an atomic write.
  TCNT1H = 0;
  TCNT1L = 0;

  // Start loading the capacitor and as such, ADC.
  conversion_done = 0;
  WRITE(TEMP_C, 1);

  // While ADC does its work, wait a second while polling USB.
  poll_a_second();

  if ( ! conversion_done) {
    temp_c = 0;
  }
}

/**
  Read out the temperature measurement result. Timer 1 is started at zero in
  temp_measure() and counts up until this interrupt is triggered. By doing
  so we get a measurement by reading Timer 1 here.
*/
ISR(ANA_COMP_vect) {

  /**
    As the ACD runs all the time, we usually receive multiple triggers per
    measurement. Tests indicated about 3 trigger on each. Avoid this by
    ignoring additional triggers.
  */
  if ( ! conversion_done) {
    // Read result. 16-bit values have to be read atomically. As this is
    // interrupt time, interrupts are already locked, so no special care
    // required.
    temp_c = TCNT1;
    conversion_done = 1;

    // Start discharging.
    WRITE(TEMP_C, 0);
  }
}

/* ---- Application ------------------------------------------------------- */

static void hardware_init(void) {

  /**
    Even if you don't use the watchdog, turn it off here. On newer devices,
    the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
  */
  wdt_disable();

  // Set time 0 prescaler to 64 (see osctune.h).
  TCCR0B = 0x03;

  SET_OUTPUT(LED_Y);
  WRITE(LED_Y, 0);

  temp_init();

  usbDeviceDisconnect();
  _delay_ms(300);
  usbDeviceConnect();
}

int main(void) {

  hardware_init();
  usbInit();
  sei();

  for (;;) {    /* main event loop */
    temp_measure(); // Also polls USB.
  }
}

