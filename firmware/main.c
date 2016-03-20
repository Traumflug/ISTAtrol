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


/* ---- Start calibration values ------------------------------------------ */

/**
  About calibration values in general.

  We're tight on flash memory, so we can't afford to allow setting changes at
  runtime, as long as we also feature an USB connection. Without USB we'd
  need a display, which we barely have the room for, too.

  Probably there's no way around upgrading to an ATtiny4313 with more Flash to
  improve on this. Or to fit an oscillator crystal onto the board, because
  V-USB implementation for 20 MHz is a whopping 384 bytes smaller than the
  crystal-free 12.8 MHz version.
*/

/** \def TARGET_TEMPERATURE

  This is our main goal!

  Unit is thermistor readout, which reacty the opposite way than a thermometer
  display. Lower values mean higher temperature, higher values mean colder.
  Best value is found during calibration.

  Unit:  1
  Range: 500..65000
*/
#define TARGET_TEMPERATURE 5700

/** \def THERMISTOR_HYSTERESIS

  This is how much the thermistor readout is allowed to deviate from
  TARGET_TEMPERATURE before the valve is moved. Thermistor readouts jitter
  quite a bit, so set this not too small.

  Smaller values give more precision. Too small values make the valve motor
  move back and forth all the time. Bigger values are harmless but may result
  in considerable deviations from the target temperature.

  Unit:  1
  Range: 0..499
*/
#define THERMISTOR_HYSTERESIS 30

/** \def RADIATOR_RESPONSE_TIME

  If the valve is opened, it takes considerable time until the temperature
  sensor on the ISTA counter sees a temperature raise. It makes no sense, to
  actuate the valve a second time within this delay. Actually it's harmful to
  do so, because this can cause overreactions.

  The initial value is found during calibration. Too large values lead to a
  slow regulation response. Too small values may lead to overreactions, up
  to unstable behaviour (valve moving full open and full close all the time).

  Unit:  seconds (approximately)
  Range: 0..65535
*/
#define RADIATOR_RESPONSE_TIME 100

/** \def MOT_OPEN_TIME

  Time to run the valve motor on a valve open operation. As we're extremely
  tight on Flash space, this is a constant value. A better implementation
  would allow to set this time by the caller, but then we'd have to pass a
  parameter, which costs a few bytes per call.

  Unit:  milliseconds
  Range: 1..6500
*/
#define MOT_OPEN_TIME 200

/** \def MOT_CLOSE_TIME

  Same as MOT_OPEN_TIME, but for the opposite valve movement. This is a
  distinct value to allow closing the valve faster than opening it. Closing
  faster may help to not overshoot the target temperature.

  Unit:  milliseconds
  Range: 1..6500
*/
#define MOT_CLOSE_TIME 1000

/* ---- End calibration values -------------------------------------------- */


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
#ifdef CAN_AFFORD_USB_COMMANDS
static union {
  uint8_t byte[8];
  uint16_t value[4];
} reply;
#endif

/**
  Track wether a valve motor movement happened.

  Values: ' '  no motor movement
          '+'  valve opened
          '-'  valve closed
*/
//uint8_t motor_moved = ' '; // See struct answer below.

/**
  Our last temperature measurements.
*/
static uint16_t temp_c = 0;
static uint16_t temp_v = 0;
static uint16_t temp_r = 0;
static uint16_t temp_temp = 0;
static uint8_t conversion_done = 0;

#ifndef CAN_AFFORD_USB_COMMANDS
/**
  The only answer to USB commands. As we can't afford to copy values into a
  response (costs 8 bytes Flash per byte copied), use a static struct for
  this answer.

  Regular variables are kept in comments and moved in and out here as needed.
*/
static struct {
  uint16_t temp_last;
  uint8_t motor_moved;
} answer = { 0, ' '};
#endif

/* ---- Valve motor movements --------------------------------------------- */

/**
  Intitialise for motor movements. Nothing special.

  The valve motor takes just about 15 mA (40 mA when blocked), so it's
  connected directly to two I/O pins. This should work as long an these two
  pins are never configured as input.

  To move the motor in one direction, one pin is set to High, to move the
  motor the other direction, the other pin is set to High. Each time the
  second pin is kept Low.
*/
static void motor_init(void) {

  SET_OUTPUT(MOT_OPEN);
  WRITE(MOT_OPEN, 0);
  SET_OUTPUT(MOT_CLOSE);
  WRITE(MOT_CLOSE, 0);
}

/**
  Run the motor to open the valve a bit.

  Yes, we should call usbPoll every 40 ms, but for now, let's try without.
*/
static void motor_open(void) {

  WRITE(MOT_OPEN, 1);
  _delay_ms(MOT_OPEN_TIME);
  WRITE(MOT_OPEN, 0);
}

/**
  Run the motor to close the valve a bit.

  Yes, we should call usbPoll every 40 ms, but for now, let's try without.
*/
static void motor_close(void) {

  WRITE(MOT_CLOSE, 1);
  _delay_ms(MOT_CLOSE_TIME);
  WRITE(MOT_CLOSE, 0);
}

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
#ifdef CAN_AFFORD_USB_COMMANDS
  uint8_t len = 0;
  // Cast to structured data for parsing.
  usbRequest_t *rq = (void *)data;

  if (rq->bRequest == 'c') {
    reply.value[0] = temp_c;
    reply.byte[2] = motor_moved;
    len = 3;
    motor_moved = ' ';
#ifdef MULTISENSOR_BROKEN
    reply.value[1] = temp_v;
    len = 4;
    reply.value[2] = temp_r;
    len = 6;
#endif
  }

  usbMsgPtr = reply.byte;
  return len;
#endif

  usbMsgPtr = (void *)&answer;
  return sizeof(answer);
}

/**
  Poll USB while doing nothing for sufficient time to allow the ADC capacitor
  to discharge. If there's something to do on the USB bus, the delay can be
  considerably longer.

  Note that this is also the basis for caclulating RADIATOR_RESPONSE_TIME.
*/
static void poll_a_second(void) {
  uint8_t i;

  // Count to at least 5, else binary size grows significantly (50 bytes).
  for (i = 0; i < 25; i++) {
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
#ifdef MULTISENSOR_BROKEN
  SET_OUTPUT(TEMP_V);
  SET_OUTPUT(TEMP_R);
#endif
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

  This procedure measures all three sensors and takes about 0.6 seconds. USB
  is taken care of.
*/
static void temp_measure(void) {

  /**
    First step is to measure the sensor connected to the ISTA counter.
  */
  // Clear Timer 1. Write the high byte first to make it an atomic write.
  TCNT1H = 0;
  TCNT1L = 0;

  // Start loading the capacitor and as such, ADC.
  conversion_done = 0;
  temp_temp = 0;
  WRITE(TEMP_C, 1);

  // While ADC does its work, wait a second while polling USB.
  poll_a_second();

  // Store measurement.
  temp_c = temp_temp;

#ifdef MULTISENSOR_BROKEN
  /**
    Do the same for the sensor connected to the radiator valve.
  */
  TCNT1H = 0;
  TCNT1L = 0;
  conversion_done = 0;
  temp_temp = 0;
  WRITE(TEMP_V, 1);
  poll_a_second();
  temp_v = temp_temp;

  /**
    Third and last, measure the room temperature sensor.
  */
  TCNT1H = 0;
  TCNT1L = 0;
  conversion_done = 0;
  temp_temp = 0;
  WRITE(TEMP_R, 1);
  poll_a_second();
  temp_r = temp_temp;
#endif

  // Done.
}

/**
  Read out the temperature measurement result. Timer 1 is started at zero in
  temp_measure() and counts up until this interrupt is triggered. By reading
  Timer 1 here we get a measurement.
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
    temp_temp = TCNT1;
    conversion_done = 1;

    // Start discharging.
    WRITE(TEMP_C, 0);
#ifdef MULTISENSOR_BROKEN
    WRITE(TEMP_V, 0);
    WRITE(TEMP_R, 0);
#endif
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

  temp_init();

  motor_init();

  usbDeviceDisconnect();
  _delay_ms(300);
  usbDeviceConnect();
}

int main(void) {
  uint16_t time = 0;
  //uint16_t temp_last = 0; // See struct answer above.

  hardware_init();
  usbInit();
  sei();

  for (;;) {    /* main event loop */

    temp_measure(); // Also polls USB.

    time++;
    // Loop count here also depends on how much poll_a_second() actually
    // delays and how often temp_measure() calls poll_a_second().
    if (time > RADIATOR_RESPONSE_TIME) {
      /**
        This is the regulation algorithm. A tricky thing, because temperature
        response to valve movements are extremely slow, some 10 minutes on
        the Traumflug's radiator.

        As we move the valve in increments only, not to absolute positions,
        this is a pure integral ('I') regulator, no proportional of
        differential part of PID. The big advantage of this is that we don't
        have to know our absolute position; an information difficult to
        get without endstops.

        Simple Bang-Bang (on this I term) led to instability with
        RADIATOR_RESPONSE_TIME = 200, MOT_OPEN_TIME = 200 and
        MOT_CLOSE_TIME = 2000.

        So we add a predictive part. If temperature moves into the right
        direction already, we can expect it to reach target without doing
        anything, so we don't move the valve. This made temperature changes
        a lot less steep, overshoots were reduced from 5 degC to 1.5 degC
        with the same settings.

        With RADIATOR_RESPONSE_TIME = 100, MOT_OPEN_TIME = 200 and
        MOT_CLOSE_TIME = 1000 (more frequent updates, open time more agressive)
        we reached overshoot of 0.5 degC and undershoot of about 1.8 degC,
        which is quite usable already.
      */
      answer.motor_moved = ' ';

      // Bang-Bang part. Thermistor reading too small -> temperature too hot.
      if (temp_c < (TARGET_TEMPERATURE - THERMISTOR_HYSTERESIS)) {
        // Predictive part. Move valve only if thermistor reading didn't raise.
        // We ignore jitter here because a jitter to our disadvantage this
        // time is likely a jitter the other way next time.
        if (temp_c < answer.temp_last) {
          motor_close();
          answer.motor_moved = '-';
        }
      }
      // Bang-Bang part. Thermistor reading too large -> temperature too cold.
      if (temp_c > (TARGET_TEMPERATURE + THERMISTOR_HYSTERESIS)) {
        // Predictive part. Move valve only if thermistor reading didn't fall.
        if (temp_c > answer.temp_last) {
          motor_open();
          answer.motor_moved = '+';
        }
      }
      time = 0;
      answer.temp_last = temp_c;
    }
  }
}

