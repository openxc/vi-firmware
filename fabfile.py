import os

from fabric.api import *
from fabric.colors import green, yellow, red
from fabric.contrib.console import confirm

from prettyprint import pp
import copy
import re

VERSION_PATTERN = r'^v\d+(\.\d+)+?$'
env.releases_directory = "release"
env.temporary_directory = "openxc-vi-firmware"
env.temporary_path = "/tmp/%(temporary_directory)s" % env
env.root_dir = os.path.abspath(os.path.dirname(__file__))

env.mode = 'default'
env.board = None
env.debug = False
env.transmitter = False
env.bootloader = True
env.allow_raw_uart_write = False
env.payload_format = "JSON"
env.logging_output = "OFF"
env.usb_product_id = 1
env.power_management = "SILENT_CAN"
env.boards = {
    "reference": {"name": "FORDBOARD", "extension": "bin"},
    "chipkit": {"name": "CHIPKIT", "extension": "hex"},
    "c5": {"name": "CROSSCHASM_C5_BT", "extension": "hex"}, #for backwards compatibility
    "c5bt": {"name": "CROSSCHASM_C5_BT", "extension": "hex"},
    "c5cell": {"name": "CROSSCHASM_C5_CELLULAR", "extension": "hex"}
}

def latest_git_tag():
    description = local('git describe master', capture=True).rstrip('\n')
    if '-' in description:
        latest_tag = description[:description.find('-')]
    else:
        latest_tag = description
    if not re.match(VERSION_PATTERN, latest_tag):
        latest_tag = None
    return latest_tag

def prepare_temp_path():
    local("rm -rf %(temporary_path)s" % env)
    local("mkdir -p %(temporary_path)s" % env)

def prepare_releases_path():
    local("mkdir -p %(releases_directory)s" % env)
    local("cp %(root_dir)s/release-README %(temporary_path)s/README.txt" % env)


def compare_versions(x, y):
    """
    Expects 2 strings in the format of 'X.Y.Z' where X, Y and Z are
    integers. It will compare the items which will organize things
    properly by their major, minor and bugfix version.
    ::

        >>> my_list = ['v1.13', 'v1.14.2', 'v1.14.1', 'v1.9', 'v1.1']
        >>> sorted(my_list, cmp=compare_versions)
        ['v1.1', 'v1.9', 'v1.13', 'v1.14.1', 'v1.14.2']

    """
    def version_to_tuple(version):
        # Trim off the leading v
        version_list = version[1:].split('.', 2)
        if len(version_list) <= 3:
            [version_list.append(0) for _ in range(3 - len(version_list))]
        try:
            return tuple((int(version) for version in version_list))
        except ValueError: # not an integer, so it goes to the bottom
            return (0, 0, 0)

    x_major, x_minor, x_bugfix = version_to_tuple(x)
    y_major, y_minor, y_bugfix = version_to_tuple(y)
    return (cmp(x_major, y_major) or cmp(x_minor, y_minor)
            or cmp(x_bugfix, y_bugfix))


def make_tag():
    if confirm(yellow("Tag this release?"), default=True):
        print(green("The last 5 tags were: "))
        tags = local('git tag | tail -n 20', capture=True)
        pp(sorted(tags.split('\n'), compare_versions, reverse=True))
        prompt("New release tag in the format vX.Y[.Z]?", 'tag',
                validate=VERSION_PATTERN)
        local('git tag -as %(tag)s' % env)
        local('git push origin', capture=True)
        local('git push --tags origin', capture=True)
        local('git fetch --tags origin', capture=True)
    else:
        env.tag = latest_git_tag()
        print(green("Using latest tag %(tag)s" % env))
    return env.tag


def release_descriptor(path):
    with lcd(path):
        return local('git describe HEAD', capture=True).rstrip("\n")

def build_option(key, value):
    if isinstance(value, bool):
        if value:
            value = "1"
        else:
            value = "0"
    return "%s=%s" % (key, value)

def build_options():
    if env.board is None:
        abort("You must specify the target board - your choices are %s" % env.boards.keys())
    board_options = env.boards[env.board]

    DEFAULT_COMPILER_OPTIONS = {
        'DEBUG': env.debug,
        'BOOTLOADER': env.bootloader,
        'TRANSMITTER': False,
        'DEFAULT_LOGGING_OUTPUT': env.logging_output,
        'DEFAULT_METRICS_STATUS': False,
        'DEFAULT_CAN_ACK_STATUS': False,
        'DEFAULT_ALLOW_RAW_WRITE_NETWORK': False,
        'DEFAULT_ALLOW_RAW_WRITE_UART': env.allow_raw_uart_write,
        'DEFAULT_ALLOW_RAW_WRITE_USB': True,
        'DEFAULT_OUTPUT_FORMAT': env.payload_format,
        'DEFAULT_RECURRING_OBD2_REQUESTS_STATUS': False,
        'DEFAULT_POWER_MANAGEMENT': env.power_management,
        'DEFAULT_USB_PRODUCT_ID': env.usb_product_id,
        'DEFAULT_EMULATED_DATA_STATUS': False,
        'DEFAULT_OBD2_BUS': 1,
        'NETWORK': False,
    }

    options = copy.copy(DEFAULT_COMPILER_OPTIONS)
    options['DEBUG'] = env.debug
    options['BOOTLOADER'] = env.bootloader
    options['TRANSMITTER'] = env.transmitter
    options['PLATFORM'] = board_options['name']
    if env.mode == 'emulator':
        options['DEFAULT_EMULATED_DATA_STATUS'] = True
        options['DEFAULT_POWER_MANAGEMENT'] = "ALWAYS_ON"
    elif env.mode == 'translated_obd2':
        options['DEFAULT_POWER_MANAGEMENT'] = "OBD2_IGNITION_CHECK"
        options['DEFAULT_RECURRING_OBD2_REQUESTS_STATUS'] = True
    elif env.mode == 'obd2':
        options['DEFAULT_POWER_MANAGEMENT'] = "OBD2_IGNITION_CHECK"
    return " ".join((build_option(key, value)
        for key, value in options.iteritems()))

def compile_firmware(build_name, target_path):
    with lcd("src"):
        for board_name, board in env.boards.iteritems():
            env.board = board_name
            build(capture=True, do_clean=True)
            local("cp build/%s/vi-firmware-%s.%s %s/vi-%s-firmware-%s-ct%s.%s"
                    % (board['name'], board['name'], board['extension'],
                        target_path, build_name, board['name'],
                        env.firmware_release, board['extension']))

def compress_release(source, archive_path):
    with lcd(os.path.dirname(source)):
        local("zip -r %s %s" % (archive_path, os.path.basename(source)))

@task
def emulator():
    env.mode = 'emulator'

@task
def translated_obd2():
    env.mode = 'translated_obd2'

@task
def obd2():
    env.mode = 'obd2'

@task
def test():
    if current_branch() == "master":
        openxc_python_is_version = None
        with settings(warn_only=True):
            openxc_python_is_version = local("grep '^openxc==' "
                    "script/bootstrap/ci-requirements.txt", capture=True)
        if not openxc_python_is_version:
            abort(red("OpenXC Python library dependency must be "
                "a released version in master branch"))

    with(lcd("src")):
        local("PLATFORM=TESTING make -j4 test")

@task
def functional_test_flash(skip_flashing=False):
    # For Bluetooth testing
    env.allow_raw_uart_write = True
    debug(logging_output="BOTH")
    with lcd("%s" % env.root_dir):
        # Pre-requisites:
        # * Reference VI attached via USB
        # * JTAG programmed attached to VI and USB of the host computer
        if skip_flashing not in (True, 'True', 'true'):
            local("openxc-generate-firmware-code -m src/tests/functional_test_config.json > src/signals.cpp")
            flash()

            import time
            time.sleep(2)

@task
def functional_test(skip_flashing=False, extra_env=""):
    # Must use debug so the VI doesn't turn off
    env.power_management = "ALWAYS_ON"
    functional_test_flash(skip_flashing=skip_flashing)
    local("VI_FUNC_TESTS_USB_PRODUCT_ID=%d " % env.usb_product_id +
            "%s nosetests -vs script/functional_test.py" % extra_env)

@task
def reference_functional_test(skip_flashing=False):
    env.usb_product_id = 1

    baremetal()
    reference()
    functional_test(skip_flashing=skip_flashing)
    functional_test(skip_flashing=True,
            extra_env="VI_FUNC_TESTS_USE_BLUETOOTH=1")

@task
def chipkit_functional_test(skip_flashing=False):
    env.usb_product_id = 2
    chipkit()
    functional_test(skip_flashing=skip_flashing)

@task
def auto_functional_test(skip_flashing=False):
    reference_functional_test(skip_flashing)
    chipkit_functional_test(skip_flashing)

@task
def transmitter():
    env.transmitter = True

@task
def debug(logging_output="USB"):
    env.debug = True
    env.logging_output = logging_output

@task
def baremetal():
    env.bootloader = False

@task
def chipkit():
    env.board = 'chipkit'

@task
def reference():
    env.board = 'reference'

#for backwards compatibility
@task
def c5():
    env.board = 'c5bt'

@task
def c5bt():
    env.board = 'c5bt'

@task
def c5cell():
    env.board = 'c5cell'

@task
def json():
    env.payload_format = "JSON"

@task
def protobuf():
    env.payload_format = "PROTOBUF"

@task
def clean():
    with lcd("%s/src" % env.root_dir):
        local("%s make clean" % build_options(), capture=True)

@task
def build(capture=False, do_clean=False):
    options = build_options()
    with lcd("%s/src" % env.root_dir):
        if do_clean:
            clean();
        output = local("%s make -j4" % options, capture=capture)
        if output.failed:
            puts(output)
            abort(red("Building %s failed" % board_options['name']))

@task
def flash():
    with lcd("%s/src" % env.root_dir):
        local("%s make flash" % build_options())

def current_branch():
    return local("git rev-parse --abbrev-ref HEAD", capture=True)

@task
def release(skip_tests=False):
    with lcd(env.root_dir):
        if not skip_tests:
            test()

        # Make sure this happens after test(), so we move aside and test
        # signals.cpp
        signals_file = "src/signals.cpp"
        moved_signals_file = "%s.bak" % signals_file
        if os.path.isfile("%s/%s" % (env.root_dir, signals_file)):
            print("Moved %s out of the way to %s, will not be in released builds" % (
                    signals_file, moved_signals_file))
            local("mv %s %s" % (signals_file, moved_signals_file))

        make_tag()

        prepare_temp_path()
        prepare_releases_path()

        env.firmware_release = release_descriptor(".")

        compile_firmware("default", env.temporary_path)

        emulator()
        compile_firmware("emulator", env.temporary_path)

        obd2()
        compile_firmware("obd2", env.temporary_path)

        translated_obd2()
        compile_firmware("translated_obd2", env.temporary_path)

        filename = "openxc-vi-firmware-%s.zip" % (env.firmware_release)
        archive = "%s/%s/%s" % (env.root_dir, env.releases_directory, filename)
        compress_release(env.temporary_path, archive)
