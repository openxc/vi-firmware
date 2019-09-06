echo off
set base_dir=C:\Users\cbaldw19\OpenXC\OpenXC\vi-firmware
set vi_dir=E:
if exist E: (
	cd %base_dir%\src\build\FORDBOARD
	if exist %vi_dir%\firmware.bin (
		del /f %vi_dir%\firmware.bin
	)
	cd
	copy vi-firmware-FORDBOARD.bin %vi_dir%
)
pause