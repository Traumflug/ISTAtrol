#!/usr/bin/env python3

# Name: terminal.py
# Project: ISTAtrol
# Author: Markus "Traumflug" Hitter, <mah@jump-ing.de>
# Creation Date: 2016-03-16
# License: GPLv3
#
# This project uses libusb and pyusb bindings for interfacing the usb bus and
# communicating with the ISTAtrol controller.
#
# Prerequisites:
#
#   sudo apt-get install python3-usb
#

import sys
import usb.core
import time

class ISTAtrolPort:
  def __init__(self, idVendor = 0x16c0, idProduct = 0x05e1):
    self.idVendor = idVendor;
    self.idProduct = idProduct;
    self.dev = None
    self.count = 0

  def open(self):
    self.dev = usb.core.find(idVendor = self.idVendor, idProduct = self.idProduct)
    if self.dev is None:
      sys.stderr.write("Device not found.\n")
      return

    self.dev.set_configuration()
    print (self.dev.configurations())

  def do(self):
    if self.dev is None:
      sys.stderr.write("No device open.\n")
      return

    # See https://github.com/walac/pyusb/blob/master/usb/core.py#L997
    # ctrl_transfer(self, bmRequestType, bRequest, wValue=0, wIndex=0,
    #               data_or_wLength = None, timeout = None)
    #
    # bmRequestType behaves a bit odd, not all numbers get through to the
    # peripheral. Numbers of 160 ( = 0xA0) and above usually work. Other
    # numbers may raise anexception.
    #                128 - 159 works and returns 'B'
    #                160 - 255 works and returns 'B' and 4 zeros
    #
    # data_or_wLength has to be at least as big as the number of bytes returned.
    result = self.dev.ctrl_transfer(0xC0, ord('c'), 0, 0, 10)
    print("%5d\t%5d" % (self.count, result[1] * 256 + result[0]))
    self.count += 1
    #print("Counter %5d -- Valve %5d" %
    #      (result[1] * 256 + result[0], result[3] * 256 + result[2]))


dev = ISTAtrolPort()
dev.open()

while 1:
  dev.do()
  time.sleep(1)

# Done.
