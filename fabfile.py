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
env.boards = {
    "reference": {"name": "FORDBOARD", "extension": "bin"},
    "chipkit": {"name": "CHIPKIT", "extension": "hex"},
    "c5": {"name": "CROSSCHASM_C5", "extension": "hex"}
}

DEFAULT_COMPILER_OPTIONS = {
    'DEBUG': False,
    'BOOTLOADER': True,
    'TRANSMITTER': False,
    'DEFAULT_UART_LOGGING_STATUS': False,
    'DEFAULT_METRICS_STATUS': False,
    'DEFAULT_CAN_ACK_STATUS': False,
    'DEFAULT_ALLOW_RAW_WRITE_NETWORK': False,
    'DEFAULT_ALLOW_RAW_WRITE_UART': False,
    'DEFAULT_ALLOW_RAW_WRITE_USB': True,
    'DEFAULT_OUTPUT_FORMAT': "JSON",
    'DEFAULT_RECURRING_OBD2_REQUESTS_STATUS': False,
    'DEFAULT_POWER_MANAGEMENT': "SILENT_CAN",
    'DEFAULT_USB_PRODUCT_ID': 0x1,
    'DEFAULT_EMULATED_DATA_STATUS': False,
    'DEFAULT_OBD2_BUS': 1,
    'NETWORK': False,
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

def build_options(options):
    return " ".join((build_option(key, value)
        for key, value in options.iteritems()))

def compile_firmware(build_name, target_path):
    with lcd("src"):
        for board_name, board in env.boards.iteritems():
            env.board = board_name
            build(capture=True, clean=True)
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
    with(lcd("src")):
        local("PLATFORM=TESTING make -j4 test")

@task
def chipkit():
    env.board = 'chipkit'

@task
def reference():
    env.board = 'reference'

@task
def c5():
    env.board = 'c5'

@task
def build(capture=False, clean=False):
    if env.board is None:
        abort("You must specify the target board - your choices are %s" % env.boards.keys())
    board_options = env.boards[env.board]

    options = copy.copy(DEFAULT_COMPILER_OPTIONS)
    if env.mode == 'emulator':
        options['DEFAULT_EMULATED_DATA_STATUS'] = True
        options['DEFAULT_POWER_MANAGEMENT'] = "ALWAYS_ON"
    elif env.mode == 'translated_obd2':
        options['DEFAULT_POWER_MANAGEMENT'] = "OBD2_IGNITION_CHECK"
        options['DEFAULT_RECURRING_OBD2_REQUESTS_STATUS'] = True
    elif env.mode == 'obd2':
        options['DEFAULT_POWER_MANAGEMENT'] = "OBD2_IGNITION_CHECK"

    with lcd("%s/src" % env.root_dir):
        options['PLATFORM'] = board_options['name']
        if clean:
            local("%s make clean" % build_options(options), capture=True)
        output = local("%s make -j4" % build_options(options), capture=capture)
        if output.failed:
            puts(output)
            abort(red("Building %s failed" % board_options['name']))

@task
def flash():
    with lcd("%s/src" % env.root_dir):
        local("PLATFORM=%s make flash" % env.boards[env.board]['name'])

@task
def release():
    with lcd(env.root_dir):
        signals_file = "src/signals.cpp"
        moved_signals_file = "%s.bak" % signals_file
        if os.path.isfile("%s/%s" % (env.root_dir, signals_file)):
            print("Moved %s out of the way to %s, will not be in released builds" % (
                    signals_file, moved_signals_file))
            local("mv %s %s" % (signals_file, moved_signals_file))

        test()
        make_tag()

        prepare_temp_path()
        prepare_releases_path()

        env.firmware_release = release_descriptor(".")

        emulator()
        compile_firmware("emulator", env.temporary_path)

        obd2()
        compile_firmware("obd2", env.temporary_path)

        translated_obd2()
        compile_firmware("translated_obd2", env.temporary_path)

        filename = "openxc-vi-firmware-%s.zip" % (env.firmware_release)
        archive = "%s/%s/%s" % (env.root_dir, env.releases_directory, filename)
        compress_release(env.temporary_path, archive)
