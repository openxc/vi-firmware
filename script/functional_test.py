from __future__ import absolute_import

import os
import time
import unittest
from nose.tools import eq_, ok_
import binascii

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

    # A bit of a hack to let us pass the product ID in at the command line, so
    # we can have 2 devices attached for testing at a time so it's more
    # automated. Set the VI_FUNC_TESTS_USB_PRODUCT_ID environment variable to a
    # number you want to use for the product ID.
    usb_product_id = os.getenv('VI_FUNC_TESTS_USB_PRODUCT_ID', None)
    global SOURCE
    SOURCE = UsbVehicleInterface(format="json", product_id=usb_product_id)
    SOURCE.start()

class ViFunctionalTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        super(ViFunctionalTests, cls).setUpClass()
        global SOURCE
        cls.vi = SOURCE
        cls.vi.callback = cls.receive

    def setUp(self):
        self.bus = 1
        self.message_id = 0x42
        self.data = "0x1234"
        ViFunctionalTests.can_message_queue = Queue()
        ViFunctionalTests.simple_vehicle_message_queue = Queue()
        ViFunctionalTests.diagnostic_response_queue = Queue()
        self.vi.set_acceptance_filter_bypass(1, True)
        self.vi.set_acceptance_filter_bypass(2, False)

    @classmethod
    def receive(cls, message, **kwargs):
        if ('id' in message and 'bus' in message and 'data' in message and
                getattr(cls, 'can_message_queue', None)):
            cls.can_message_queue.put(message)
        elif ('name' in message and 'value' in message):
            cls.simple_vehicle_message_queue.put(message)
        elif ('id' in message and 'bus' in message and 'mode' in message):
            cls.diagnostic_response_queue.put(message)


class ProtobufBaseTests(ViFunctionalTests):
    @classmethod
    def setUpClass(cls):
        super(ProtobufBaseTests, cls).setUpClass()
        cls.vi.set_payload_format("protobuf")

class JsonBaseTests(ViFunctionalTests):
    @classmethod
    def setUpClass(cls):
        super(JsonBaseTests, cls).setUpClass()
        cls.vi.set_payload_format("json")


class ControlCommandTests(object):
    def test_version(self):
        # TODO it'd be nice to read this from src/version.c
        eq_(self.vi.version(), "6.0.4-dev (functional_tests)")

    def test_device_id(self):
        device_id = self.vi.device_id()
        ok_(device_id is not None)
        if device_id != "Unknown":
            eq_(len(device_id), 12)

class ControlCommandTestsJson(ProtobufBaseTests, ControlCommandTests):
    pass

class ControlCommandTestsProtobuf(ProtobufBaseTests, ControlCommandTests):
    pass

class CanMessageTests(object):

    def _check_received_message(self, message):
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_send_and_receive_can_message(self):
        ok_(self.vi.set_acceptance_filter_bypass(1, False))
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data))
        self._check_received_message(self.can_message_queue.get(timeout=.5))

    def test_nonmatching_can_message_received_on_unfiltered_bus(self):
        ok_(self.vi.set_acceptance_filter_bypass(1, True))
        self.message_id += 1
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data))
        self._check_received_message(self.can_message_queue.get(timeout=.5))

    def test_matching_can_message_received_on_filtered_bus(self):
        ok_(self.vi.set_acceptance_filter_bypass(2, False))
        self.message_id = 0x43
        self.bus = 2
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data))
        self._check_received_message(self.can_message_queue.get(timeout=.5))

    def test_nonmatching_can_message_not_received_on_filtered_bus(self):
        ok_(self.vi.set_acceptance_filter_bypass(2, False))
        self.bus = 2
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data))
        try:
            message = self.can_message_queue.get(timeout=.5)
        except Empty:
            pass
        else:
            eq_(None, message)
            self.can_message_queue.task_done()

    def test_send_and_receive_extended_can_frame(self):
        self.message_id = 0x809
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data))
        self._check_received_message(self.can_message_queue.get(timeout=.5))

class CanMessageTestsJson(JsonBaseTests, CanMessageTests):
    pass

class CanMessageTestsProtobuf(ProtobufBaseTests, CanMessageTests):
    pass

class SimpleVehicleMessageTests(object):

    def setUp(self):
        self.message1_data = "0x0000000000000dac"
        self.data = "0x1234"
        self.expected_signal_value = 0xdac

    def test_receive_simple_vehicle_message_bus1(self):
        ok_(self.vi.write(bus=1, id=0x42, data=self.message1_data))
        message = self.simple_vehicle_message_queue.get(timeout=.5)
        eq_(message['name'], "signal1")
        eq_(message['value'], self.expected_signal_value)
        self.simple_vehicle_message_queue.task_done()

    def test_receive_simple_vehicle_message_bus2(self):
        message_data = "0x8000000000000000"
        ok_(self.vi.write(bus=2, id=0x43, data=message_data))
        message = self.simple_vehicle_message_queue.get(timeout=.5)
        eq_(message['name'], "signal2")
        eq_(message['value'], 1)
        self.simple_vehicle_message_queue.task_done()

class SimpleVehicleMessageTestsJson(JsonBaseTests, SimpleVehicleMessageTests):
    def setUp(self):
        super(SimpleVehicleMessageTestsJson, self).setUp()
        SimpleVehicleMessageTests.setUp(self)

class SimpleVehicleMessageTestsProtobuf(ProtobufBaseTests,
        SimpleVehicleMessageTests):
    def setUp(self):
        super(SimpleVehicleMessageTestsProtobuf, self).setUp()
        SimpleVehicleMessageTests.setUp(self)

class DiagnosticRequestTests(object):

    def setUp(self):
        self.message_id = 0x121
        self.mode = 3
        self.bus = 1
        self.pid = 1
        self.payload = bytearray([0x12, 0x34])

    def test_diagnostic_request(self):
        # This test is done with bus 1, since that has the CAN AF off, so we
        # can receive the sent message (via loopback) to validate it matches the
        # request.
        ok_(self.vi.create_diagnostic_request(self.message_id, self.mode,
                bus=self.bus, pid=self.pid, payload=self.payload))
        message = self.can_message_queue.get(timeout=.5)
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_("0x04%02x%02x%s000000" % (self.mode, self.pid,
                binascii.hexlify(self.payload)), message['data'])
        self.can_message_queue.task_done()

    def test_diagnostic_request_changes_acceptance_filters(self):
        # This test is done with bus 2, since that has the CAN AF ON, so we
        # make sure the VI can change the AF to accept responses.
        # We use bus 2 since that should still have the AF on.

        self.bus = 2
        ok_(self.vi.create_diagnostic_request(self.message_id, self.mode,
                bus=self.bus, pid=self.pid, payload=self.payload))
        # send the response, which should be accepted by the AF
        response_id = self.message_id + 0x8
        # we don't care about the payload at this point, just want to make sure
        # we receive the raw CAN message
        ok_(self.vi.write(bus=self.bus, id=response_id, data="0xabcd"))
        message = None
        while message is None:
            message = self.can_message_queue.get(timeout=.5)
            if message['id'] == self.message_id:
                # skip the request
                continue
            elif message['id'] == response_id:
                break
        ok_(message is not None)
        self.can_message_queue.task_done()

    def test_receive_diagnostic_response(self):
        ok_(self.vi.create_diagnostic_request(self.message_id, self.mode, bus=self.bus,
                pid=self.pid, payload=self.payload))

        response_id = self.message_id + 0x8
        # we don't care about the payload at this point, just want to make sure
        # we receive the raw CAN message
        ok_(self.vi.write(bus=self.bus, id=response_id, data="0x03430142"))
        response = self.diagnostic_response_queue.get(timeout=.5)
        eq_(self.message_id, response['id'])
        eq_(self.bus, response['bus'])
        eq_(self.mode, response['mode'])
        eq_(self.pid, response['pid'])
        ok_(response['success'])
        eq_("0x42", response['payload'])
        self.diagnostic_response_queue.task_done()


    def test_receive_obd_formatted_diagnostic_response(self):
        self.pid = 0xa
        self.mode = 1
        ok_(self.vi.create_diagnostic_request(self.message_id, self.mode, bus=self.bus,
                pid=self.pid, payload=self.payload, decoded_type="obd2"))
        response_id = self.message_id + 0x8
        response_value = 0x42
        ok_(self.vi.write(bus=self.bus, id=response_id, data="0x034%01x%02x%02x" % (
                self.mode, self.pid, response_value)))

        response = self.diagnostic_response_queue.get(timeout=.5)
        eq_(self.message_id, response['id'])
        eq_(self.bus, response['bus'])
        eq_(self.mode, response['mode'])
        eq_(self.pid, response['pid'])
        ok_(response['success'])
        eq_(response_value * 3, response['value'])
        self.diagnostic_response_queue.task_done()

    def test_create_recurring_request(self):
        try:
            ok_(self.vi.create_diagnostic_request(self.message_id, self.mode,
                    bus=self.bus, pid=self.pid, payload=self.payload,
                    frequency=10))
            for _ in range(5):
                message = self.can_message_queue.get(timeout=.5)
                eq_(self.message_id, message['id'])
                eq_(self.bus, message['bus'])
                eq_("0x04%02x%02x%s000000" % (self.mode, self.pid,
                        binascii.hexlify(self.payload)), message['data'])
                self.can_message_queue.task_done()
        finally:
            ok_(self.vi.delete_diagnostic_request(self.message_id, self.mode,
                    bus=self.bus, pid=self.pid))

    def test_cancel_diagnostic_request(self):
        ok_(self.vi.create_diagnostic_request(self.message_id, self.mode,
                bus=self.bus, pid=self.pid, payload=self.payload,
                frequency=5))
        time.sleep(1)
        ok_(self.vi.delete_diagnostic_request(self.message_id, self.mode,
                bus=self.bus, pid=self.pid))
        ViFunctionalTests.can_message_queue = Queue()
        try:
            self.can_message_queue.get(timeout=.5)
        except Empty:
            pass
        else:
            ok_(False)

class DiagnosticRequestTestsJson(JsonBaseTests, DiagnosticRequestTests):
    def setUp(self):
        super(DiagnosticRequestTestsJson, self).setUp()
        DiagnosticRequestTests.setUp(self)

class DiagnosticRequestTestsProtobuf(ProtobufBaseTests, DiagnosticRequestTests):
    def setUp(self):
        super(DiagnosticRequestTestsProtobuf, self).setUp()
        DiagnosticRequestTests.setUp(self)

class CanAcceptanceFilterChangeTests(object):

    def _check_received_message(self, message):
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_message_not_received_after_filters_enabled(self):
        ok_(self.vi.set_acceptance_filter_bypass(1, False))
        # Should receive only 42
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data))
        self._check_received_message(self.can_message_queue.get(timeout=.5))

        ok_(self.vi.write(bus=self.bus, id=self.message_id + 1, data=self.data))
        try:
            self.can_message_queue.get(timeout=.5)
        except Empty:
            pass
        else:
            ok_(False)

    def test_nonmatching_can_message_received_on_unfiltered_bus(self):
        ok_(self.vi.set_acceptance_filter_bypass(1, True))
        self.message_id += 1
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data) > 0)
        self._check_received_message(self.can_message_queue.get(timeout=.5))

class CanAcceptanceFilterChangeTestsJson(JsonBaseTests,
        CanAcceptanceFilterChangeTests):
    pass

class CanAcceptanceFilterChangeTestsProtobuf(ProtobufBaseTests,
        CanAcceptanceFilterChangeTests):
    pass

class PayloadFormatTests(ViFunctionalTests):

    def tearDown(self):
        ok_(self.vi.set_payload_format("json"))

    def _check_received_message(self, message):
        eq_(self.message_id, message['id'])
        eq_(self.bus, message['bus'])
        eq_(self.data, message['data'])
        self.can_message_queue.task_done()

    def test_change_to_binary(self):
        ok_(self.vi.set_payload_format("protobuf"))
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data) > 0)
        self._check_received_message(self.can_message_queue.get(timeout=.5))

    def test_change_to_json(self):
        ok_(self.vi.set_payload_format("json"))
        ok_(self.vi.write(bus=self.bus, id=self.message_id, data=self.data) > 0)
        self._check_received_message(self.can_message_queue.get(timeout=.5))

class PredefinedObd2RequestsTests(object):
    def tearDown(self):
        ok_(self.vi.set_predefined_obd2_requests(False))

    def test_enable_predefined_obd2_requests_sends_messages(self):
        ok_(self.vi.set_predefined_obd2_requests(True))
        message = self.can_message_queue.get(timeout=6)
        eq_(0x7df, message['id'])
        eq_(1, message['bus'])
        eq_(u"0x02010d0000000000", message['data'])
        self.can_message_queue.task_done()

    def test_disable_predefined_obd2_requests_stops_messages(self):
        ok_(self.vi.set_predefined_obd2_requests(True))
        message = self.can_message_queue.get(timeout=5)
        ok_(message is not None)
        ok_(self.vi.set_predefined_obd2_requests(False))
        try:
            while self.can_message_queue.get(timeout=.5) is None:
                continue
        except Empty:
            pass

        message = None
        try:
            message = self.can_message_queue.get(timeout=6)
        except Empty:
            pass
        eq_(None, message)

    def test_pid_request_set_after_support_query(self):
        # TODO test that proper PID requests are sent if we response to the
        # query properly. It's tough because we have to response in 100ms from
        # the request, which with the delays on USB is difficult.
        pass

    def test_simple_vehicle_message_response_for_pid(self):
        # TODO test that proper simple vehicle messages are sent if we reply to
        # the PID request. difficult because of the reasons mentioned above.
        pass


class PredefinedObd2RequestsTestsJson(JsonBaseTests,
        PredefinedObd2RequestsTests):
    def tearDown(self):
        super(PredefinedObd2RequestsTestsJson, self).tearDown()
        PredefinedObd2RequestsTests.tearDown(self)

class PredefinedObd2RequestsTestsProtobuf(ProtobufBaseTests,
        PredefinedObd2RequestsTests):
    def tearDown(self):
        super(PredefinedObd2RequestsTestsProtobuf, self).tearDown()
        PredefinedObd2RequestsTests.tearDown(self)
