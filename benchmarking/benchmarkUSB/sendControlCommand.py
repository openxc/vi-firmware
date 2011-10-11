#!/usr/bin/env python
#
# Looks for and configures a chipKIT with the benchmarkUSB sketch running, and
# sends a control transfer message for it to say "WHAT" on the serial console.

import usb.core

SAY_WHAT_COMMAND = 0x80

dev = usb.core.find(idVendor=0x04d8)
dev.set_configuration()
dev.ctrl_transfer(0x40, SAY_WHAT_COMMAND)
