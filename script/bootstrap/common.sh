#!/usr/bin/env bash

set -e
set -x
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z $COMMON_SOURCED ]; then

    # TODO this is kind of a hacky way of determining if root is required -
    # ideally we wouuld set up a little virtualenv in the dependencies folder
    SUDO_CMD=
    if [ -z $CI ]; then
        if command -v sudo >/dev/null 2>&1; then
            SUDO_CMD="sudo -E"

            if [ -z $CI ] && [ -z $VAGRANT ]; then
                echo "The bootstrap script needs to install a few packages to your system as an admin, and we will use the 'sudo' command - enter your password to continue"
                $SUDO_CMD ls > /dev/null
            fi
        fi
    fi

    KERNEL=`uname`
    ARCH=`uname -m`
    if [ ${KERNEL:0:7} == "MINGW32" ]; then
        OS="windows"
    elif [ ${KERNEL:0:6} == "CYGWIN" ]; then
        OS="cygwin"
    elif [ $KERNEL == "Darwin" ]; then
        OS="mac"
    else
        OS="linux"
        if ! command -v lsb_release >/dev/null 2>&1; then
            # Arch Linux
            if command -v pacman>/dev/null 2>&1; then
                $SUDO_CMD pacman -S lsb-release
            fi
        fi

        DISTRO=`lsb_release -si`
    fi


    die() {
        echo >&2 "${bldred}$@${txtrst}"
        exit 1
    }

    _cygwin_error() {
        echo
        echo "${bldred}Missing \"$1\"${txtrst} - run the Cygwin installer again and select the base package set:"
        echo "    $CYGWIN_PACKAGES"
        echo "After installing the packages, re-run this bootstrap script."
        die
    }

    if ! command -v tput >/dev/null 2>&1; then
        if [ $OS == "cygwin" ]; then
            echo "OPTIONAL: Install the \"ncurses\" package in Cygwin to get colored shell output"
        fi
    else
        set +e
        # These exit with 1 when provisioning in a Vagrant box...?
        txtrst=$(tput sgr0) # reset
        bldred=${txtbld}$(tput setaf 1)
        bldgreen=${txtbld}$(tput setaf 2)
        set -e
    fi


    _pushd() {
        pushd $1 > /dev/null
    }

    _popd() {
        popd > /dev/null
    }

    _wait() {
        if [ -z $CI ] && [ -z $VAGRANT ]; then
            echo "Press Enter when done"
            read
        fi
    }

    _install() {
        if [ $OS == "cygwin" ]; then
            _cygwin_error $1
        elif [ $OS == "mac" ]; then
            # brew exists with 1 if it's already installed
            set +e
            brew install $1
            set -e
        elif [ -z $CI ]; then
            if [ -z $DISTRO ]; then
                echo
                echo "Missing $1 - install it using your distro's package manager or build from source"
                _wait
            else
                if [ $DISTRO == "arch" ]; then
                    $SUDO_CMD pacman -S $1
                elif [ $DISTRO == "Ubuntu" ]; then
                    $SUDO_CMD apt-get update -qq
                    $SUDO_CMD apt-get install $1 -y
                else
                    echo
                    echo "Missing $1 - install it using your distro's package manager or build from source"
                    _wait
                fi
            fi
        fi
    }

    CYGWIN_PACKAGES="make, gcc-core, patch, unzip, python, check, curl, libsasl2, python-setuptools, ncurses"

    download() {
        url=$1
        filename=$2
        curl $url -L -o $filename
    }

    if [ `id -u` == 0 ]; then
        die "Error: running as root - don't use 'sudo' with this script"
    fi

    if ! command -v curl >/dev/null 2>&1; then
        if [ $OS == "cygwin" ]; then
            _cygwin_error "curl"
        else
            _install curl
        fi
    fi

    echo "Storing all downloaded dependencies in the \"dependencies\" folder"

    DEPENDENCIES_FOLDER="dependencies"
    mkdir -p $DEPENDENCIES_FOLDER


    if [ $OS == "windows" ]; then
        die "Compiling the VI firmware from a Windows prompt is not \
supported - see the docs for the recommended approach"
    fi

    if [ $OS == "cygwin" ]; then
    # TODO need a warning colored promp
        echo "It looks like you're developing in Cygwin. Cygwin is no \
longer the recommended build environment for \
Windows developers. It is unofficially supported by the bootstrap \
scripts, but the recommended method is to use Vagrant - see the docs for \
more information."
        echo -n "Press Enter to continue anyway, or Control-C to cancel"
        read
        echo "Continuing with bootstrap..."
    fi

    if [ $OS == "mac" ]; then
        echo "It looks like you're developing in Mac OS X. We recommend \
using Vagrant to compile the VI firmware. OS X is unofficially supported by \
the bootstrap scripts and you should be able to compile just fine, but you \
can save yourself some trouble by using the pre-configured Vagrant \
environment. See the docs for more information."
        echo -n "Press Enter to continue anyway, or Control-C to cancel"
        read
        echo "Continuing with bootstrap..."
    fi

    if [ $OS == "linux" ] && [ -z $VAGRANT ] && [ -z $CI ]; then
        echo "It looks like you're developing in Linux. We recommend \
using Vagrant to compile the VI firmware. Ubuntu and Arch Linux are
supported by the bootstrap scripts and you should be able to compile
just fine, but you can save yourself some trouble by using the \
pre-configured Vagrant environment. See the docs for more information."
        echo -n "Press Enter to continue anyway, or Control-C to cancel"
        read
        echo "Continuing with bootstrap..."
    fi

    if [ $OS == "mac" ] && ! command -v brew >/dev/null 2>&1; then
        echo "Installing Homebrew..."
        ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"
    fi

    if ! command -v make >/dev/null 2>&1; then
        if [ $OS == "cygwin" ]; then
            _cygwin_error "make"
        elif [ $OS == "mac" ]; then
                die "Missing 'make' - install the Xcode CLI tools"
        else
            if [ $DISTRO == "arch" ]; then
                _install "base-devel"
            elif [ $DISTRO == "Ubuntu" ]; then
                _install "build-essential"
            fi
        fi
    fi

    if [ -z $CI ] && [ $DISTRO == "Ubuntu" ] && [ $ARCH == "x86_64" ]; then
        _install "libc6-i386"
    fi

    if ! command -v g++ >/dev/null 2>&1; then
        if [ $DISTRO == "Ubuntu" ]; then
            _install "g++"
        fi
    fi


    if ! command -v git >/dev/null 2>&1; then
        if [ $OS == "cygwin" ]; then
            die "You need to install Git for Windows from http://git-scm.com/ and select the option to \"Run Git from the Windows Command Prompt\" during the installation."
        elif [ $OS == "mac" ] || [ $OS == "linux" ]; then
            _install git
        fi
    fi

    echo "Updating Git submodules..."

    # git submodule update is a shell script and expects some lines to fail
    set +e
    git submodule sync
    if ! git submodule update --init --recursive --quiet; then
        echo "Unable to update git submodules - try running \"git submodule update --init --recursive\" to see the full error"
        echo "If git complains that it \"Needed a single revision\", run \"rm -rf src/libs\" and then try the bootstrap script again"
        if [ $OS == "cygwin" ]; then
            echo "In Cygwin this may be true (ignore if you know ca-certifications is installed:"
            _cygwin_error "ca-certificates"
        fi
        die
    fi
    set -e

    if ! command -v python >/dev/null 2>&1; then
        echo "Installing Python..."
        _install "python"
    fi

    if [ $OS != "cygwin" ]; then
        echo "Installing Python development headers..."
        _install "python-dev"
    fi

    if ! command -v pip >/dev/null 2>&1; then
        echo "Installing Pip..."
        if ! command -v easy_install >/dev/null 2>&1; then
            _install "python-setuptools"
        fi

        if ! command -v easy_install >/dev/null 2>&1; then
            die "easy_install not available, can't install pip"
        fi

        $SUDO_CMD easy_install pip
    fi

    pip install --user --upgrade setuptools
    pip install --user --src dependencies --pre -Ur $BOOTSTRAP_DIR/ci-requirements.txt
    if [ -z $CI ]; then
        pip install --user --src dependencies --pre -Ur $BOOTSTRAP_DIR/pip-requirements.txt
    fi

    COMMON_SOURCED=1
fi
