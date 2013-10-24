************************************************
Open XC Vehicle Interface Windows Driver Readme
************************************************

In order to be able to work with the Vehicle Interface from your Windows workstation,
you must first install the USB Drivers for the Vehicle Interface. 


To start, download the vi-firmware repository. You can use either use git to clone the repository
(using "git clone https://github.com/openxc/vi-firmware") or use the following URL to download a zipped archive of the repo:
https://github.com/openxc/vi-firmware/archive/master.zip


To install the drivers, run the installer_x86.exe (for 32-bit machines) or installer_x64.exe
(for 64-bit machines) which are located in the vi-firmware/conf/windows-driver/ directory, and follow the instructions.



If you run into issues using the executables, or would rather install the drivers manually, here's how:

WITHOUT holding down the pin-button on the VI, plug the VI into your computer via USB->Micro USB. Windows
will attempt to, and fail to find a suitable driver. 


Windows XP
-----------
1) Click Start, then right-click "My Computer" and click "Manage"
2) On the left side of the screen, click "Device Manager"
3) Locate a device with a yellow question mark next to its icon. Assuming you have no other USB devices with no assigned drivers,
this will be your OpenXC module. Right-click this device and click "Update Driver"
4) Select "No, not this time" for using Windows Update, and click Next.
5) Select "Install from a list or specific location (Advanced). Click Next.
6) Select "Don't search. I will choose the driver to install." Click Next.
7) Click the "Have Disk..." button, then click "Browse..."
8) Locate the OpenXC_CAN_Translator.inf file under vi-firmware/conf/windows-driver/ and select it.
9) Click "OK" on the Install From Disk dialog box, and "OK" to any additional confirmations.
10) If Windows gives you a warning regarding Driver Certification, click "Continue Anyway". 
11) After the driver is installed, restart your computer.

Windows Vista/7
---------------
1) Click the Windows "start" button, then search for "Device Manager" and open it
2) Locate a device with a yellow question mark next to its icon. Assuming you have no other USB devices with no assigned drivers,
this will be your OpenXC module. Right-click this device and click "Update Driver Software..."
3) Click "Browse my computer for driver software"
4) Click "Let me pick from a list of devices drivers on my computer"
5) Click the "Have Disk..." button, then click "Browse..."
6) Locate the OpenXC_CAN_Translator.inf file under vi-firmware/conf/windows-driver/ and select it.
7) Click "OK" on the Install From Disk dialog box, then click Next on the Update Driver Software screen.
8) If Windows gives you a warning regarding Driver Certification, click "Continue Anyway". 
9) After the driver is installed, restart your computer.



You can test that the driver is properly installed by using the OpenXC Python library. Follow the instructions on the 
python OpenXC site to install the library, then run the "openxc-version" command. It should report back which car firmware
you have flashed onto the VI (Or report no firmware installed if you haven't flashed a firmware yet).

If you're still getting errors about USB not being found/installed, restart your computer, and if the issue persists, come
ask for help in the Google Group. 