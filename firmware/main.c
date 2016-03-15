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


#define VENDOR_RQ_WRITE_BUFFER  1
#define VENDOR_RQ_READ_BUFFER   2


uint8_t lastTimer0Value; // See osctune.h.

/* ---- USB related functions --------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  //usbRequest_t *rq = (void *)setupData;   // cast to structured data for parsing

  //switch(rq->bRequest) {
  //  case VENDOR_RQ_WRITE_BUFFER:
  //    return USB_NO_MSG;        // tell driver to use usbFunctionWrite()

  //  case VENDOR_RQ_READ_BUFFER:
  //    return 7;
  //}

  return 0;                               // ignore all unknown requests
}

uchar usbFunctionRead(uchar *data, uchar len) {
  return 7;
}

uchar usbFunctionWrite (uchar *data, uchar len) {
  return 1;
}


/* ---- Application ------------------------------------------------------- */

static void hardwareInit(void) {

  /**
    Even if you don't use the watchdog, turn it off here. On newer devices,
    the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
  */
  wdt_disable();

  ACSR |= 0x80;   // disable analog comparator and save 70uA
  TCCR0B = 0x03;  // prescaler 64 (see osctune.h)

  /* activate pull-ups except on USB lines */
  USB_CFG_IOPORT = (uchar)~((1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT));

  usbDeviceDisconnect();
  _delay_ms(300);
  usbDeviceConnect();
}

int main(void) {

  hardwareInit();
  usbInit();
  sei();

  for (;;) {    /* main event loop */
    usbPoll();
  }
}

