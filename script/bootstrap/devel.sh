set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

echo "Installing dependencies for running test suite..."

if ! command -v lcov >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        echo "Missing lcov - Cygwin doesn't have a packaged version of lcov, and it's only required to calculate test suite coverage. We'll skip it."
    elif [ $OS == "mac" ]; then
        _install "lcov"
    else
        if [ $DISTRO == "Ubuntu" ]; then
            _install "lcov"
        elif [ $DISTRO == "arch" ]; then
            echo "Missing lcov - install from the AUR."
            _wait
        fi
    fi
fi

if ! command -v clang >/dev/null 2>&1; then
    if [ $OS == "linux" ]; then
        if [ $DISTRO == "arch" ]; then
            _install "clang"
        else
            _install "clang-3.5"
        fi
    fi
fi

if [ $OS == "cygwin" ] && ! command -v ld >/dev/null 2>&1; then
    _cygwin_error "gcc-core"
fi

if ! ld -lcheck -o /tmp/checkcheck 2>/dev/null; then
    echo "Installing the check unit testing library..."

    _install "check"
fi

echo
echo "${bldgreen}Core developer dependencies installed.$txtrst"
