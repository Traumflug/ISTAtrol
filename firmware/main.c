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
  elongated periods, but also occupies 8-bit Timer0.
*/
uint8_t lastTimer0Value; // See osctune.h.

/**
  We don't need to store much status because we don't implement multiple chunks
  in read/write transfers.
*/
static uint8_t reply_buffer[8];


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

  if (rq->bRequest == 'h') {
    reply_buffer[0] = 'H';
    reply_buffer[1] = 'e';
    reply_buffer[2] = 'l';
    reply_buffer[3] = 'l';
    reply_buffer[4] = 'o';
    len = 5;
  } else {
    reply_buffer[0] = 'H';
    reply_buffer[1] = 'u';
    reply_buffer[2] = 'h';
    reply_buffer[3] = '?';
    len = 4;
  }

  usbMsgPtr = reply_buffer;
  return len;
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

  usbDeviceDisconnect();
  _delay_ms(300);
  usbDeviceConnect();
}

int main(void) {

  hardware_init();
  usbInit();
  sei();

  for (;;) {    /* main event loop */
    usbPoll();
    WRITE(LED_Y, 1);
    _delay_ms(40);
    WRITE(LED_Y, 0);
    _delay_ms(40);
  }
}

