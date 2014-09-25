from __future__ import absolute_import

import unittest
from nose.tools import eq_

try:
    from Queue import Queue
except ImportError:
    # Python 3
    from queue import Queue

from openxc.tools.common import configure_logging
from openxc.interface import UsbVehicleInterface

class ControlCommandTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        super(ControlCommandTests, cls).setUpClass()
        configure_logging()

        cls.source = UsbVehicleInterface(cls.receive, format="json")
        cls.source.start()

    def setUp(self):
        self.bus = 1
        self.message_id = 0x42
        self.data = "0x1234"
        ControlCommandTests.can_message_queue = Queue()

    @classmethod
    def receive(cls, message, **kwargs):
        if ('id' in message and 'bus' in message and 'data' in message and
                getattr(cls, 'can_message_queue', None)):
            cls.can_message_queue.put(message)

    def test_version(self):
        # TODO it'd be nice to read this from src/version.c
        eq_(self.source.version(), "6.0.4-dev (functional_tests)")

    def test_device_id(self):
        device_id = self.source.device_id()
        eq_(len(device_id), 12)

    def test_send_and_receive_can_message(self):
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        message = self.can_message_queue.get(timeout=1)
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_nonmatching_can_message_received_on_unfiltered_bus(self):
        self.message_id += 1
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        message = self.can_message_queue.get(timeout=1)
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_matching_can_message_received_on_filtered_bus(self):
        self.message_id = 0x43
        self.bus = 2
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        message = self.can_message_queue.get(timeout=1)
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_nonmatching_can_message_not_received_on_filtered_bus(self):
        self.bus = 2
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        message = self.can_message_queue.get(timeout=1)
        eq_(None, message)
        self.can_message_queue.task_done()
