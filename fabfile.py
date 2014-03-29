import os

from fabric.api import *
from fabric.colors import green, yellow
from fabric.contrib.console import confirm

from prettyprint import pp
import re

VERSION_PATTERN = r'^v\d+(\.\d+)+?$'
env.releases_directory = "release"
env.temporary_directory = "openxc-translator-firmware"
env.temporary_path = "/tmp/%(temporary_directory)s" % env
env.root_dir = os.path.abspath(os.path.dirname(__file__))

env.boards = [{"name": "FORDBOARD", "extension": "bin"},
    {"name": "CHIPKIT", "extension": "hex"},
    {"name": "CROSSCHASM_C5", "extension": "hex"}]

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


def compile_emulator(target_path):
    with lcd("src"):
        local("make clean", capture=True)
        for board in env.boards:
            output = local("PLATFORM=%s DEBUG=0 BOOTLOADER=1 DEFAULT_EMULATED_DATA_STATUS=1 DEFAULT_POWER_MANAGEMENT=ALWAYS_ON make -j4" %
                    (board['name']), capture=True)
            if output.failed:
                puts(output)
                abort("Building emulator for %s failed" % board['name'])
            local("cp build/%s/vi-firmware-%s.%s %s/canemulator-%s-ct%s.%s"
                    % (board['name'], board['name'], board['extension'],
                        target_path, board['name'], env.firmware_release,
                        board['extension']))

def compress_release(source, archive_path):
    with lcd(os.path.dirname(source)):
        local("zip -r %s %s" % (archive_path, os.path.basename(source)))

@task
def test():
    with(lcd("src")):
        local("make -j4 test")

@task
def release():
    local("script/bootstrap.sh")
    test()
    make_tag()

    prepare_temp_path()
    prepare_releases_path()

    env.firmware_release = release_descriptor(".")

    compile_emulator(env.temporary_path)
    filename = "openxc-emulator-firmware-%s.zip" % (env.firmware_release)
    emulator_archive = "%s/%s/%s" % (env.root_dir, env.releases_directory,
            filename)
    compress_release("%s/canemulator-*" % env.temporary_path, emulator_archive)

