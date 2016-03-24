#!/usr/bin/env python3
#
# Communications terminal for the ISTAtrol heating valve controller.
#
# Copyright (C) 2016 Markus "Traumflug" Hitter <mah@jump-ing.de>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <http://www.gnu.org/licenses/>.
#
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
    self.lastC = 0

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
    readingC = result[1] * 256 + result[0]

    # Putting the value pairs recorded in Calibration measurements.gnumeric
    # into a linear regression tool like
    #   http://www.arndt-bruenner.de/mathe/scripts/regr.htm
    # we get this linear fomula:
    #
    #  f(x) = -0,00791 * x + 71,445927
    #
    # Let's take this formula to get an idea about the temperature
    # in deg Celsius:
    tempC = -0.00791 * readingC + 71.445927

    valveText = ""
    if readingC != self.lastC: # Ignore duplicates.
      if chr(result[2]) == '+':
        valveText = "  (Valve opened)"
      elif chr(result[2]) == '-':
        valveText = "  (Valve closed)"

    print("%5d\t%5d\t%2.1f°C\t%s%s" % (self.count, result[1] * 256 + result[0],
                                       tempC, time.strftime("%X"), valveText))
    self.count += 1
    self.lastC = readingC


print("ISTAtrol communications terminal.")
print("Copyright (C) 2016 Markus \"Traumflug\" Hitter <mah@jump-ing.de>.")
print("This program is free software and comes with ABSOLUTELY NO WARRANTY;")
print("for details see license.txt (GPLv3).")

dev = ISTAtrolPort()
dev.open()

while 1:
  try:
    dev.do()
  except:
    print(sys.exc_info())
    print("... trying again ...")
    time.sleep(10)
    dev.open()
    continue
  time.sleep(60)

# Done.
