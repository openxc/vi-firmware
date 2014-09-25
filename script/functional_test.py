from __future__ import absolute_import

import unittest
import time
from nose.tools import eq_

from openxc.formats.json import JsonFormatter
from openxc.tools.common import configure_logging
from openxc.interface import UsbVehicleInterface

class ControlCommandTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super(ControlCommandTests, cls).setUpClass()
        configure_logging()

        cls.source = UsbVehicleInterface(cls.receive, format="json")
        cls.source.start()

    @classmethod
    def receive(cls, message, **kwargs):
        message['timestamp'] = time.time()
        print(JsonFormatter.serialize(message))

    def test_version(self):
        # TODO it'd be nice to read this from src/version.c
        eq_(self.source.version(), "6.0.4-dev (functional_tests)")

    def test_device_id(self):
        device_id = self.source.device_id()
        eq_(len(device_id), 12)
