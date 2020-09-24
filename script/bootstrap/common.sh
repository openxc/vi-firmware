#!/usr/bin/env bash

set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z $COMMON_SOURCED ]; then

    # TODO this is kind of a hacky way of determining if root is required -
    # ideally we wouuld set up a little virtualenv in the dependencies folder
    SUDO_CMD=
    if command -v sudo >/dev/null 2>&1; then
        SUDO_CMD="sudo -E"

        if [ -z $CI ] && [ -z $VAGRANT ]; then
            echo "The bootstrap script needs to install a few packages to your system as an admin, and we will use the 'sudo' command - enter your password to continue"
            $SUDO_CMD ls > /dev/null
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

    if [ $OS == "linux" ]; then
        echo "Sane build distro"
    else
        die "Compiling the VI firmware from a $OS prompt is not \
        supported - see the docs for the recommended approach"
    fi

    set +e
    # These exit with 1 when provisioning in a Vagrant box...?
    txtrst=$(tput sgr0) # reset
    bldred=${txtbld}$(tput setaf 1)
    bldgreen=${txtbld}$(tput setaf 2)
    set -e


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

        if [ -z $DISTRO ]; then
            echo
            echo "Missing $1 - install it using your distro's package manager or build from source"
            _wait
        else
            $SUDO_CMD apt-get update -qq -y
            $SUDO_CMD apt install -y $1 
        fi
    }

    CYGWIN_PACKAGES="make, gcc-core, patch, unzip, python, check, curl, libsasl2, python-setuptools, ncurses"

    download() {
        url=$1
        filename=$2

        #if 3rd arg exists, run curl and disable SSL cert checking. Added
        #for digilent - see pic32.sh
        if [ ! -z $3 ]; then 
            k_opt="-k"
        else
            k_opt=""
        fi
        echo "download() running 'curl -Ss $k_opt $url -L -o $filename'"
        curl -Ss $k_opt $url -L -o $filename
    }

    if [ `id -u` == 0 ]; then
        die "Error: running as root - don't use 'sudo' with this script"
    fi

    if ! command -v curl >/dev/null 2>&1; then
        _install curl
    fi

    echo "Storing all downloaded dependencies in the \"dependencies\" folder"

    DEPENDENCIES_FOLDER="dependencies"
    mkdir -p $DEPENDENCIES_FOLDER

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



    if ! command -v make >/dev/null 2>&1; then
        _install "build-essential"
    fi

    _install "libc6-i386"

    if ! command -v g++ >/dev/null 2>&1; then
        _install "g++"
        _install "check"
    fi


    if ! command -v git >/dev/null 2>&1; then
        _install git
    fi

    echo "Updating Git submodules..."

    # git submodule update is a shell script and expects some lines to fail
    set +e
    git submodule sync
    if ! git submodule update --init --recursive --quiet; then
        echo "Unable to update git submodules - try running \"git submodule update --init --recursive\" to see the full error"
        echo "If git complains that it \"Needed a single revision\", run \"rm -rf src/libs\" and then try the bootstrap script again"
        die
    fi
    set -e

    echo "Fixing gitdir for nested submodules"

    # https://marc.info/?l=git&m=145937129606918&w=2
    # There's a bug with the git submodule update command (still unfixed as of v2.23.0) that causes nested submodules to use an absolue
    # path for their gitdir, whereas everything else uses relative paths. Since the project root in this context
    # is /vagrant/, the gitdirs for the nested submodules will be incorrect and consequentially most git commands won't 
    # work (unless you're in a vagrant machine). For now we'll just rewrite the gitdirs for the nested submodules to use relative paths
    sed -i -e 's/\/vagrant\//..\/..\/..\/..\/..\//g' src/libs/AT-commander/lpc17xx/BSP/.git
    sed -i -e 's/\/vagrant\//..\/..\/..\/..\/..\//g' src/libs/AT-commander/lpc17xx/CDL/.git
    sed -i -e 's/\/vagrant\//..\/..\/..\/..\/..\//g' src/libs/AT-commander/lpc17xx/emqueue/.git
    sed -i -e 's/\/vagrant\//..\/..\/..\/..\/..\//g' src/libs/openxc-message-format/libs/nanopb/.git
    sed -i -e 's/\/vagrant\//..\/..\/..\/..\/..\//g' src/libs/uds-c/deps/isotp-c/.git
    sed -i -e 's/\/vagrant\//..\/..\/..\/..\/..\/..\/..\//g' src/libs/uds-c/deps/isotp-c/deps/bitfield-c/.git

    # Force git to use CR/LF line endings. The default from within a vagrant machine is different, so without this
    # it'll think all of the files have changed when inside a vagrant machine
    git config --global core.autocrlf true
    
    if ! command -v python3 >/dev/null 2>&1; then
        echo "Installing Python..."
        _install "python3"
        $SUDO_CMD ln -s /usr/bin/python3 /usr/bin/python
    fi

    if ! command -v pip3 >/dev/null 2>&1; then
        _install python3-pip
        $SUDO_CMD ln -s /usr/bin/pip3 /usr/bin/pip
    fi

    PIP_SUDO_CMD=
    if [ -z $VIRTUAL_ENV ]; then
        # Only use sudo if the user doesn't have an active virtualenv
        PIP_SUDO_CMD=$SUDO_CMD
    fi

    $PIP_SUDO_CMD pip3 install --src dependencies --pre -r $BOOTSTRAP_DIR/ci-requirements.txt
    if [ -z $CI ]; then
        $PIP_SUDO_CMD pip3 install --src dependencies --pre -r $BOOTSTRAP_DIR/pip-requirements.txt
    fi
	
    if ! command -v clang >/dev/null 2>&1; then
        $SUDO_CMD sudo apt-get install clang -y
    fi

    COMMON_SOURCED=1
fi
