C5 CELLULAR Config
------------------

When building for the C5 Cellular, there are some modem-specific build
options that you can choose. These options are set directly in the
firmware, in the configuration struct from config.cpp. Some common
scenarios are described here.

Building for a particular mobile network (SIM card):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following five options govern how the modem finds and connects to
mobile network(s). These options should be considered whenever you
compile the firmware for use with a particular SIM.

- **networkOperatorSettings.allowDataRoaming**
   -  set to "true" to let the SIM use any network it can find, whether
      or not the SIM sees it as a "home" network. This is appropriate
      for many "global" SIM cards, which see all mobile networks as
      roaming. If you are using a SIM that is tied to a specific
      carrier, you can set this to "false" to stop the modem from
      connecting to a foreign network on start-up.


-  **networkOperatorSettings.operatorSelectMode**

   -  **AUTOMATIC**: Find and use any available mobile network which accepts
      the SIM. The options "PLMN" and "networkType" are ignored in this
      mode.

   -  **MANUAL**: Find and connect to only the mobile network with a PLMN
      matching the "PLMN" option, and of the type specified by
      networkType.

   -  **DEREGISTER**: This mode forces the modem to deregister from the
      current network. The modem will stay disconnected from any mobile
      network until the mode changes. Not particularly useful as a
      connection strategy, but can be used by custom code looking for
      implement a custom connection strategy.

   -  **SET\_ONLY**: Only useful for custom code implementations. Forces the
      modem to ignore the specified "PLMN" and "networkType", and to
      only interpret the format specifier for the network name (which is
      fixed in this firmware).

   -  **MANUAL\_AUTOMATIC**: Modem follows the MANUAL connection strategy.
      If a suitable network cannot be found, modem falls back to the
      "AUTOMATIC" connection strategy.


-  **networkOperatorSettings.networkDescriptor.PLMN**

   -  a 5-6 digit number that combines the "mobile country code" (MCC)
      and "mobile network code" (MNC) to uniquely identify a particular
      mobile network. Using the PLMN avoids any issues with variations
      in network text names. For example, 302220 is the PLMN for TELUS.
      A list of PLMNs is easily found online, e.g.
      "http://en.wikipedia.org/wiki/Mobile\_country\_code".


-  **networkOperatorSettings.networkDescriptor.networkType**

   -  an enumeration that specifies the access technology of the desired
      mobile network. Generally, "GSM" specifies a 2G network and
      "UTRAN" specifies a 3G network. A particular mobile network (PLMN)
      can have both types of networks available for the modem to use.


-  **networkDataSettings.APN**

   -  is the familiar "access point name" the a mobile operator will
      generally require in order for the device to access data services
      on the network. It is typically a string like "internet.com" or
      "apn".

Setting up GPS signals:
~~~~~~~~~~~~~~~~~~~~~~~

The GPS receiver in the C5 Cellular publishes a GPS message that
contains a variety of signals. You can select which signals in this
message will be published through the VI, and how often. Note that all
of the selected GPS signals share a single publish interval.

The following are available in **telit.config.globalPositioningSettings**:

1. To enable/disable GPS functionality, set the "gpsEnable" flag to
   "true" or "false".
2. Set the interval (in milliseconds) to publish the GPS message via
   "gpsInterval".
3. Select the GPS signals you want to include in the published GPS
   message by setting the appropriate "gpsEnableSignal\_gps\_" flag. The
   following signal "enable" flags are available:

   -  gpsEnableSignal\_gps\_time
   -  gpsEnableSignal\_gps\_latitude
   -  gpsEnableSignal\_gps\_longitude
   -  gpsEnableSignal\_gps\_hdop
   -  gpsEnableSignal\_gps\_altitude
   -  gpsEnableSignal\_gps\_fix
   -  gpsEnableSignal\_gps\_course
   -  gpsEnableSignal\_gps\_speed
   -  gpsEnableSignal\_gps\_speed\_knots
   -  gpsEnableSignal\_gps\_date
   -  gpsEnableSignal\_gps\_nsat

Server connection settings:
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following configuration options tell the server how to connect to
your OpenXC web server:

-  **serverConnectSettings.host**

   -  the web server address, for example
      "openxcserverdemo.azurewebsites.net". Only supply the base address
      of the web server here, do not include any particular resources
      (e.g. /api/{IMEI}/data). Specific, standard OpenXC resources are
      requested by the firmware's server API calls.


-  **serverConnectSettings.port**

   -  simply the TCP/IP port on which the web service operates.
      Typically this is port 80 (HTTP).

Socket connection settings:
~~~~~~~~~~~~~~~~~~~~~~~~~~~

More advanced settings that control the specific behaviour of the TCP/IP
sockets used to communicate with thr OpenXC web server. You typically do
not have to modify these unless you have some specific network issues or
advanced requirements (or you find the default values to be less than
optimal). TCP/IP socket settings are:

-  **socketConnectSettings.packetSize**

   -  the TCP/IP packet size. Set to 0 for default (300 bytes).


-  **socketConnectSettings.idleTimeout**

   -  the number of seconds that the modem will tolerate an idle socket
      before closing it. An idle socket is a socket without any data
      exchange. Set to 0 for no timeout, and the modem will keep an idle
      socket open indefinitely. Note that the server on the other end
      can still close the socket due to it's own idle timeout (or other)
      criteria.


-  **socketConnectSettings.connectTimeout**

   -  the duration of time that the modem will attempt to open a socket
      before timing out. The duration is specified in "hundreds of
      milliseconds", so a value of 600 would be 60000 milliseconds, or
      60 seconds. A longer timeout period allow the modem a higher
      chance of gaining access to the server in a single connect
      request, at the expense of hanging the command interface for the
      duration of the timer (other parallel operations cannot be
      performed). Shortening the timer will increase the odds of a
      failed connection attempt, but the command interface will become
      more responsive.


-  **socketConnectSettings.txFlushTimer**

   -  the duration of time that the modem will hold a partial TCP/IP
      packet before flushing it to the TCP/IP socket. Specified in
      "hundreds of milliseconds".

