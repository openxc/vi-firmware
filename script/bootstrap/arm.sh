# ARM / LPC17XX Dependencies

set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then

    echo "Installing GCC for ARM Embedded..."

    GCC_ARM_BASENAME="gcc-arm-none-eabi-4_8-2014q2-20140609"
    if [ $OS == "linux" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-linux.tar.bz2"
    elif [ $OS == "mac" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-mac.tar.bz2"
    elif [ $OS == "cygwin" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-win32.exe"
    fi

    GCC_ARM_URL="https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q2-update/+download/$GCC_ARM_FILE"
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
        TRAILING_DIRNAME="GNU Tools ARM Embedded/4.8 2014q2/"
        GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES_64/$TRAILING_DIRNAME"
        if ! test -d "$GCC_INNER_DIR"; then
            GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES/$TRAILING_DIRNAME"
        fi
    else
        GCC_INNER_DIR="gcc-arm-none-eabi-4_8-2014q2"
        INSTALL_COMMAND="tar -xjf ../$GCC_ARM_FILE -C /tmp"
    fi

    if ! test -d "$GCC_INNER_DIR"
    then
        $INSTALL_COMMAND
        if [ $OS == "cygwin" ]; then
            echo -n "Press Enter when the GCC for ARM Embedded installer is finished"
            read
        else
            cp -LR /tmp/$GCC_INNER_DIR .
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

    echo "Copying GCC binaries to local dependencies folder..."
    # If running Vagrant in Windows, these dependency files are accessed via
    # network share through Vagrant/Virtualbox, and symlinks are not
    # supported. We use the -L flag to dereference links while copying.
    cp -LR "$GCC_INNER_DIR"/* .

    LIBLTO_SYMLINK="$GCC_INNER_DIR/lib/gcc/arm-none-eabi/4.8.4/liblto_plugin.so"
    if ! test -e $LIBLTO_SYMLINK; then
        # If running Vagrant in Windows, these dependency files are access via
        # network share through Vagrant/Virtualbox, and symlinks are not
        # supported. These files will be missing, so we have to copy where they
        # are supposed to be symlinking to the properly named files.
        cp "$LIBLTO_SYMLINK.0.0.0" $LIBLTO_SYMLINK
        cp "$LIBLTO_SYMLINK.0.0.0" $LIBLTO_SYMLINK.0
    fi

    _popd
    _popd

fi

if [ -z $CI ]; then
    if [ -z $VAGRANT ]; then
        while true; do
            read -p "Will you be using a JTAG programmer (y/n) ? " yn
            case $yn in
                [Yy]* ) source $BOOTSTRAP_DIR/flashing_arm.sh; break;;
                [Nn]* ) break;;
                * ) echo "Please answer yes or no.";;
            esac
        done
    else
        source $BOOTSTRAP_DIR/flashing_arm.sh;
    fi
fi
