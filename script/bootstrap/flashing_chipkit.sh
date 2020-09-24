set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

## FTDI library for programming chipKIT

if ! command -v unzip >/dev/null 2>&1; then
    _install "unzip"
fi

if [ -z "$MPIDE_DIR" ] || ! test -e $MPIDE_DIR || [ $OS == "cygwin" ]; then

    echo "Installing MPIDE to get avrdude for flashing chipKIT Max32 platform"

    MPIDE_BUILD=20140821
    MPIDE_BASENAME=mpide-0023-linux64-20140821
    MPIDE_FILE="$MPIDE_BASENAME".tgz
    EXTRACT_COMMAND="tar -xzf"

    MPIDE_URL=http://chipkit.s3.amazonaws.com/builds/$MPIDE_FILE

    _pushd $DEPENDENCIES_FOLDER
    if ! test -e $MPIDE_FILE
    then
        echo "Downloading MPIDE..."
        download $MPIDE_URL $MPIDE_FILE
    fi

    if ! test -d $MPIDE_BASENAME
    then
        echo "Installing MPIDE to system..."
        $EXTRACT_COMMAND $MPIDE_FILE
        sudo cp -R $MPIDE_BASENAME /usr/lib/mpide
	rm -rf $MPIDE_BASENAME
	rm -rf $MPIDE_FILE
        echo "MPIDE installed"
    fi
    _popd

fi

echo
echo "${bldgreen}chipKIT flashing dependencies installed.$txtrst"
