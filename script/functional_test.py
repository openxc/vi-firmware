from __future__ import absolute_import

import unittest
from nose.tools import eq_

try:
    from Queue import Queue, Empty
except ImportError:
    # Python 3
    from queue import Queue, Empty

from openxc.tools.common import configure_logging
from openxc.interface import UsbVehicleInterface

SOURCE = None

def setUpModule():
    configure_logging()

    global SOURCE
    SOURCE = UsbVehicleInterface(format="json")
    SOURCE.start()

class ViFunctionalTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        super(ViFunctionalTests, cls).setUpClass()
        global SOURCE
        cls.source = SOURCE
        cls.source.callback = cls.receive

    def setUp(self):
        self.bus = 1
        self.message_id = 0x42
        self.data = "0x1234"
        ViFunctionalTests.can_message_queue = Queue()

    @classmethod
    def receive(cls, message, **kwargs):
        if ('id' in message and 'bus' in message and 'data' in message and
                getattr(cls, 'can_message_queue', None)):
            cls.can_message_queue.put(message)


class ControlCommandTests(ViFunctionalTests):

    def test_version(self):
        # TODO it'd be nice to read this from src/version.c
        eq_(self.source.version(), "6.0.4-dev (functional_tests)")

    def test_device_id(self):
        device_id = self.source.device_id()
        eq_(len(device_id), 12)


class CanMessageTests(ViFunctionalTests):

    def _check_receive_message(self, message):
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_send_and_receive_can_message(self):
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        self._check_receive_message(self.can_message_queue.get(timeout=1))

    def test_nonmatching_can_message_received_on_unfiltered_bus(self):
        self.message_id += 1
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        self._check_receive_message(self.can_message_queue.get(timeout=1))

    def test_matching_can_message_received_on_filtered_bus(self):
        self.message_id = 0x43
        self.bus = 2
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        self._check_receive_message(self.can_message_queue.get(timeout=1))

    def test_nonmatching_can_message_not_received_on_filtered_bus(self):
        self.bus = 2
        self.source.write(bus=self.bus, id=self.message_id, data=self.data)
        try:
            message = self.can_message_queue.get(timeout=1)
        except Empty:
            pass
        else:
            eq_(None, message)
            self.can_message_queue.task_done()
