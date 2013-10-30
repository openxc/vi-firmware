@ECHO OFF

set AVRDUDE=
for %%e in (%PATHEXT%) do (
  for %%X in (avrdude%%e) do (
    if not defined AVRDUDE (
      set AVRDUDE=%%~$PATH:X
    )
  )
)

if not defined AVRDUDE (
  IF EXIST .\\avrdude.exe ( set AVRDUDE=.\\avrdude.exe )
)

if not defined AVRDUDE (
	echo Cannot find avrdude.exe.  Make sure it is in your path or the same directory as avrwrite.bat
	exit /B
)

set AVRDUDE_CONF=../conf/avrdude.conf

if "%1"=="" (
	echo Firmware file must be passed as first argument, e.g.:
	echo upload_hex.bat firmware.hex com1
	exit /B
)

if "%2"=="" (
	echo COM port must be passed as second argument, e.g.:
	echo upload_hex.bat firmware.hex com1
	exit /B
)

IF EXIST %1 (
	set HEX_FILE=%1
	set MCU=32MX795F512L
	set AVRDUDE_ARD_PROGRAMMER=stk500v2
	set AVRDUDE_ARD_BAUDRATE=115200
	set PORT=%2
	set AVRDUDE_COM_OPTS=-q -V -p %MCU%
	set AVRDUDE_ARD_OPTS=-c %AVRDUDE_ARD_PROGRAMMER% -b %AVRDUDE_ARD_BAUDRATE% -P %PORT%
	echo %AVRDUDE_COM_OPTS%
	echo %AVRDUDE_ARD_OPTS%
	%AVRDUDE% -C %AVRDUDE_CONF% %AVRDUDE_COM_OPTS% %AVRDUDE_ARD_OPTS% -U flash:w:%HEX_FILE%:i
)

IF NOT EXIST %1 (
	echo "%1" doesn't exist
	exit /B
)
