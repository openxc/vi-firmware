#ifndef _TELIT_HE910_H_
#define _TELIT_HE910_H_

#include "interface/uart.h"
#include "interface/interface.h"

#define MAX_DEVICE_ID_LENGTH    17
#define MAX_ICCID_LENGTH        32

namespace openxc {
namespace telitHE910 {

typedef enum {
    GSM = 0,
    UTRAN = 2,
} NetworkType;

typedef enum {
    AUTOMATIC = 0,
    MANUAL = 1,
    DEREGISTER = 2,
    SET_ONLY = 3,
    MANUAL_AUTOMATIC = 4,
} OperatorSelectMode;

typedef enum {
    NOT_REGISTERED_NOT_SEARCHING = 0,
    REGISTERED_HOME = 1,
    NOT_REGISTRED_SEARCHING = 2,
    REGISTRATION_DENIED = 3,
    UNKNOWN = 4,
    REGISTERED_ROAMING = 5
} NetworkConnectionStatus;

typedef enum {
    SOCKET_CLOSED = 0,
    SOCKET_OPEN = 1,
    SOCKET_SUSPENDED = 2,
    SOCKET_SUSPENDED_DATA_PENDING = 3,
    SOCKET_LISTENING = 4,
    SOCKET_INCOMING = 5
} SocketStatus;

typedef struct {
    unsigned int PLMN;
    NetworkType networkType;
} NetworkDescriptor;

typedef struct {
    bool gpsEnable;
    unsigned int gpsInterval;
    bool gpsEnableSignal_gps_time;
    bool gpsEnableSignal_gps_latitude;
    bool gpsEnableSignal_gps_longitude;
    bool gpsEnableSignal_gps_hdop;
    bool gpsEnableSignal_gps_altitude;
    bool gpsEnableSignal_gps_fix;
    bool gpsEnableSignal_gps_course;
    bool gpsEnableSignal_gps_speed;
    bool gpsEnableSignal_gps_speed_knots;
    bool gpsEnableSignal_gps_date;
    bool gpsEnableSignal_gps_nsat;
} GlobalPositioningSettings;

typedef struct {
    bool allowDataRoaming;
    OperatorSelectMode operatorSelectMode;
    NetworkDescriptor networkDescriptor;
} NetworkOperatorSettings;

typedef struct {
    char APN[64];
} NetworkDataSettings;

typedef struct {
    char host[128];
    unsigned int port;
} ServerConnectSettings;

typedef struct {
    unsigned int packetSize;
    unsigned int idleTimeout;
    unsigned int connectTimeout;
    unsigned int txFlushTimer;
} SocketConnectSettings;

typedef struct {
    GlobalPositioningSettings globalPositioningSettings;
    NetworkOperatorSettings networkOperatorSettings;
    NetworkDataSettings networkDataSettings;
    SocketConnectSettings socketConnectSettings;
    ServerConnectSettings serverConnectSettings;
} ModemConfigurationDescriptor;

typedef struct {
    unsigned int connectedPLMN;
} ModemConnectionInfo;

typedef enum {
    INVALID0 = 0,
    INVALID1 = 1,
    FIX_2D = 2,
    FIX_3D = 3,
    FIX_MAX_ENUM
} GPSFixType;

typedef struct {
    openxc::interface::InterfaceDescriptor descriptor;
    ModemConfigurationDescriptor config;
    openxc::interface::uart::UartDevice* uart;
    QUEUE_TYPE(uint8_t) sendQueue;
    QUEUE_TYPE(uint8_t) receiveQueue;
    char deviceId[MAX_DEVICE_ID_LENGTH];
    char ICCID[MAX_ICCID_LENGTH];
} TelitDevice;

typedef enum {
    POWER_OFF,
    POWER_ON_DELAY,
    POWER_ON,
    POWER_UP_DELAY,
    INITIALIZE,
    WAIT_FOR_NETWORK,
    CLOSE_PDP,
    OPEN_PDP_DELAY,
    OPEN_PDP,
    READY
} TELIT_CONNECTION_STATE;

#define SEND_BUFFER_SIZE 4096

/*
 * INITIALIZATION FUNCTIONS
 *
 * These functions should be all that is required under normal circumstances to operate the modem.
 * The initialization function will bring up the modem, connect to a network, and open a data session.
 * The deinitialization function will suspend all network activity and power down the modem.
 */
 
/*Public: State machine to manage the telit modem. Call repeatedly.*/
TELIT_CONNECTION_STATE connectionManager(TelitDevice* device);

/*Public: Gets the device manager state.*/
TELIT_CONNECTION_STATE getDeviceState();

/* Public: Shut down the modem peripheral (power off).*/
void deinitialize();

/*
 * SETTINGS FUNCTIONS
 * 
 * The settings functions control NVM settings in the modem.
 */

/*Public: Commits all unsaved settings to modem's non-volatile memory (NVM). 
  Note that some (many) settings are automatically committed to NVM when they are changed, 
  and in those cases this function does not need to be called.*/
bool saveSettings(void);

/*
 * BAUD RATE FUNCTIONS
 *
 * The baud rate functions set the UART baud rate of the modem.
 */
 
/*Public: Sets the modem's UART baud rate. If successful the modem replies with "OK" at the old baud rate, 
 then the new baud rate takes effect immediately. Must call "saveSettings()" for the new baud to be saved to NVM.*/
bool setBaud(unsigned int baudRate);

/* 
 * SIM CARD FUNCTIONS
 *
 * The SIM card functions retrieve information on SIM slot status and SIM number.
 */

/*Public: Returns SIM card status (inserted or not inserted).*/
bool getSIMStatus(unsigned int* status);

/*Public: Returns the SIM number (ICCID).*/
bool getICCID(char* ICCID);

/* 
 * DEVICE INFO FUNCTIONS
 *
 * Device functions get information about the modem, such as firmware version or IMEI.
 */

/*Public: Get the modem IMEI number.*/
bool getDeviceIMEI(char* IMEI);

/*Public: Get the device firmware version.*/
bool getDeviceFirmwareVersion(char* firmwareVersion);

/* 
 * NETWORK FUNCTIONS
 * 
 * Network functions get/set mobile network operator information. They can be used to change network
 * connection modes, attached to a specific operator, etc.
 */

/*Public: Sets the network connection mode, which can be used to force a mobile operator change*/
bool setNetworkConnectionMode(OperatorSelectMode mode, NetworkDescriptor network);

/*Public: Returns the modem network connection status info struct.*/
bool getNetworkConnectionStatus(NetworkConnectionStatus* status);

/*Public: Returns the current network description (PLMN and access technology).*/
bool getCurrentNetwork(NetworkDescriptor* network);

/*
 * PDP FUNCTIONS
 *
 * PDP functions get/set packet data protocol settings required for a cellular data session.
 */
 
/*Public: Provides settings for a PDP context.*/
bool configurePDPContext(NetworkDataSettings dataSettings);

/*Public: Opens a PDP context.*/
bool openPDPContext(void);

/*Public: Closes a PDP context.*/
bool closePDPContext(void);

/*Public: Gets the connected status of a PDP context.*/
bool getPDPContext(bool* connected);

/* 
 * RADIO FUNCTIONS
 *
 * Radio function retrieve information about the connected radio towers and radio signal quality.
 */

/*Public: Returns the signal strength and bit error rate indicators.*/
void getSignalQuality();

/*Public: Returns connected cell tower information.*/
void getConnectedRadioTowerInformation();

/*
 * SOCKET FUNCTIONS
 *
 * Socket functions are used to configure, open, close and monitor the status of up to six
 * TCP/IP sockets available in the modem.
 */

/*Public: Sets the configuration settings for a TCP/IP socket.*/
bool configureSocket(unsigned int socketNumber, SocketConnectSettings socketSettings);

/*Public: Returns the status of the specified socket.*/
bool getSocketStatus(unsigned int socketNumber, SocketStatus* status);

/*Public: Returns true if data received data is pending on socket_number.*/
bool isSocketDataAvailable(unsigned int socketNumber);

/*Public: Opens a TCP/IP socket to the specified address:port, using the specified socket number.*/
bool openSocket(unsigned int socketNumber, ServerConnectSettings serverSettings);

/*Public: Returns true if the specified socket is open and can accept data.*/
bool isSocketOpen(unsigned int socketNumber);

/*Public: Closes the specified TCP/IP socket number.*/
bool closeSocket(unsigned int socketNumber);

/*Public: Sends data on the specified TCP/IP socket number.*/
bool writeSocket(unsigned int socketNumber, char* data, unsigned int *len);

/*Public: Reads data from the specified TCP/IP socket number.
 * 
 * socketNumber: the TCP/IP socket to read from
 * data: pointer to a char array to store read data
 * len: pointer to maximum number of bytes to copy to 'data', replaced with actual bytes read on return
 */
bool readSocket(unsigned int socketNumber, char* data, unsigned int* len);

/*Public: Reads a single byte from the specified TCP/IP socket number.
 * 
 * socketNumber: the TCP/IP socket to read from
 * data: pointer to a char array to store read data
 * len: pointer to maximum number of bytes to copy to 'data', replaced with actual bytes read on return (max 1)
 */
bool readSocketOne(unsigned int socketNumber, char* data, unsigned int* len);

/*
 * GPS FUNCTIONS
 * 
 * GPS functions set/get the power state of the modem's GPS receiver, and query GPS location.
 */

/*Public: Sets the power state of the GPS chip.*/
bool setGPSPowerState(bool enable);

/*Public: Returns power state of the modem's GPS chip.*/
bool getGPSPowerState(bool* enable);

/*Public: Returns the GPS location string from the modem.*/
bool getGPSLocation();

/*
 * PIPELINE FUNCTIONS
 *
 * Pipeline functions are device callbacks used by the common pipeline to send/receive device data 
 * (after device initialization).
 */

/*Public: Callback for device to process pipeline data sitting in the TelitDevice send queue.*/
void processSendQueue(TelitDevice* device);
 
/*Public: Returns network connection status of the modem (i.e. is the modem in a state where it can process pipeline data).*/
bool connected(TelitDevice* device);

/*Public: Returns number of bytes allocated for the device data send buffer.*/
unsigned int sizeSendBuffer(TelitDevice* device);

/*Public: Resets the tracking pointer for the device data send buffer, which resets buffer to empty state.*/
void resetSendBuffer(TelitDevice* device);

/*Public: Returns number of bytes stored in the device data send buffer.*/
unsigned int bytesSendBuffer(TelitDevice* device);

/*Public: Copies all bytes from the device data send buffer to the destination pointer, within the limits of the specified length.*/
unsigned int readAllSendBuffer(TelitDevice* device, char* destination, unsigned int read_len);
 
void flushDataBuffer(TelitDevice* device);
void firmwareCheck(TelitDevice* device);
void commandCheck(TelitDevice* device);
 
} // namespace telitHE910
} // namespace openxc

#endif // _TELIT_HE910_H_
