# ARM / LPC17XX Dependencies

set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then

    echo "Installing GCC for ARM Embedded..."

    GCC_ARM_BASENAME="gcc-arm-none-eabi-4_8-2013q4-20131204"
    if [ $OS == "linux" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-linux.tar.bz2"
    elif [ $OS == "mac" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-mac.tar.bz2"
    elif [ $OS == "cygwin" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-win32.exe"
    fi

    GCC_ARM_URL="https://launchpad.net/gcc-arm-embedded/4.8/4.8-2013-q4-major/+download/$GCC_ARM_FILE"
    GCC_ARM_DIR="gcc-arm-embedded"

    _pushd $DEPENDENCIES_FOLDER
    if ! test -e $GCC_ARM_FILE
    then
        download $GCC_ARM_URL $GCC_ARM_FILE
    fi

    mkdir -p $GCC_ARM_DIR
    _pushd $GCC_ARM_DIR
    if [ $OS == "cygwin" ]; then
        chmod a+x ../$GCC_ARM_FILE
        INSTALL_COMMAND="cygstart.exe ../$GCC_ARM_FILE"
        PROGRAM_FILES_BASE="/cygdrive/c/"
        PROGRAM_FILES="Program Files"
        PROGRAM_FILES_64="Program Files (x86)"
        TRAILING_DIRNAME="GNU Tools ARM Embedded/4.8 2013q4/"
        GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES_64/$TRAILING_DIRNAME"
        if ! test -d "$GCC_INNER_DIR"; then
            GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES/$TRAILING_DIRNAME"
        fi
    else
        GCC_INNER_DIR="gcc-arm-none-eabi-4_8-2013q4"
        INSTALL_COMMAND="tar -xjf ../$GCC_ARM_FILE"
    fi

    if ! test -d "$GCC_INNER_DIR"
    then
        $INSTALL_COMMAND
        if [ $OS == "cygwin" ]; then
            echo -n "Press Enter when the GCC for ARM Embedded installer is finished"
            read
        fi
    fi

    if [ $OS == "cygwin" ]; then
        GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES_64/$TRAILING_DIRNAME"
        if ! test -d "$GCC_INNER_DIR"; then
            GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES/$TRAILING_DIRNAME"
            if ! test -d "$GCC_INNER_DIR"; then
                die "GCC for ARM isn't installed in the expected location."
            fi
        fi
    fi

    if ! test -d arm-none-eabi; then
        echo "Copying GCC binaries to local dependencies folder..."
        cp -R "$GCC_INNER_DIR"/* .
    fi

    _popd
    _popd

fi

if [ -z $CI ]; then
    while true; do
        read -p "Will you be using a JTAG programmer (y/n) ? " yn
        case $yn in
            [Yy]* ) source $BOOTSTRAP_DIR/flashing_arm.sh; break;;
            [Nn]* ) break;;
            * ) echo "Please answer yes or no.";;
        esac
    done
fi
