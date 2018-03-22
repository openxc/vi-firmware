#ifdef CROSSCHASM_C5_BLE

#include "interface/ble.h"
#include "config.h"
#include "platform_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <plib.h>
#include <stdbool.h>
#include <ctype.h>


#include "util/log.h"
#include "util/timer.h"

#include "libs/STBTLE/ble_status.h"
#include "libs/STBTLE/bluenrg_aci.h"
#include "libs/STBTLE/hal_types.h"
#include "libs/STBTLE/bluenrg_gatt_server.h"
#include "libs/STBTLE/hci_const.h"
#include "libs/STBTLE/bluenrg_gap.h"
#include "libs/STBTLE/ble_status.h"
#include "libs/STBTLE/bluenrg_hal_aci.h"
#include "ringbuffer/ringbuffer.h"

#include "spi.h"
#include "hci.h"
#include "blueNRG.h"

#include "lights.h"


using openxc::util::bytebuffer::processQueue;
using openxc::util::log::debug;
using openxc::util::time::uptimeMs;

using openxc::config::getConfiguration;

using openxc::interface::ble::BleStatus;
using openxc::interface::ble::BleError;




#define OPTIMIZE_NOTIFICATION             1

/* Private MACROS*/
//Hardware related delays
#define DELAY_US_POWER_DOWN_BTLE_PIN    5000        //Delay interval after a power down is activated 
#define DELAY_US_POWER_UP_BTLE_PIN      5000        //Delay interval after a power up is activated
#define STBLE_NRG_RST_DLY_US            100000      //Delay interval after a hardware reset is activated   


//BLE Send timeout interval and retry
#define BLE_APP_SEND_TIMEOUT_MS          1000        //Delay between re-attempting to send notification 
#define BLE_APP_SEND_RETRIES             10          //Maximum number of attempts before timing out on notification
#define NOTIFICATION_SPEED_MS            1000        //speed at which notifications are sent to the device

//BLE flags
#define BLE_CONNECTED                    1<<1        //Device is connected over BLE to a phone or PC
#define BLE_CONNECTABLE                  1<<2        //Enable connection to allow PC or phone to connect over BLE
#define DEFAULT_RADIO_POWER_LEVEL        7           //Set Radio power level (7-Maximum)

#define BLE_MAX_DEV_NAME_LEN             13          //Maximum length of characters permitted for BLE name adversiment


#define MAX_BLE_NOTIFY_RETRIES            100

//UUID Generator MACROS
#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
do {\
    uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
        uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
            uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
                uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
}while(0)


#define COPY_VT_SERVICE_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x68,0x00,0xd3,0x8b, 0x42,0x3d, 0x4b,0xdb, 0xba,0x05, 0xc9,0x27,0x6d,0x84,0x53,0xe1) 
#define COPY_APP_COM_UUID(uuid_struct)     COPY_UUID_128(uuid_struct,0x68,0x00,0xd3,0x8b, 0x52,0x62, 0x11,0xe5, 0x88,0x5d, 0xfe,0xff,0x81,0x9c,0xdc,0xe2)
#define COPY_APP_RSP_UUID(uuid_struct)     COPY_UUID_128(uuid_struct,0x68,0x00,0xd3,0x8b, 0x52,0x62, 0x11,0xe5, 0x88,0x5d, 0xfe,0xff,0x81,0x9c,0xdc,0xe3)

extern void Test_Deserial(void);

static const uint8_t device_gap_name[]  = "CrossChasm";

 /*
   const uint8_t manuf_data[] = {26, AD_TYPE_MANUFACTURER_SPECIFIC_DATA, 
      0xF0, 0x00, //Company identifier code (Default is 0x0030 - STMicroelectronics: To be customized for specific identifier)
      0x02,       // ID
      0x15,       //Length of the remaining payload
      0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4, //Location UUID
      0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61,
      0x00, 0x00, // Major number 
      0x00, 0x00, // Minor number 
      0xC8        //2's complement of the Tx power (-56dB)};      
   };
 */  
//Memory for circular buffer operation 

uint16_t vtServHandle, appComCharHandle, appRSPCharHandle;
static uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
static uint16_t conn_handle=0;
static char device_adv_name[16];


#ifdef BLE_NO_ACT_TIMEOUT_ENABLE
    uint32_t ble_no_act_timeout_max = BLE_NO_ACT_TIMEOUT_DELAY_MS; 
    uint32_t ble_no_act_timer_starttime = 0;
    bool ble_no_act_active = 0;
#endif

//Private Functions
static void ST_BLE_Failed_CB(uint8_t reason);
static tBleStatus ST_BLE_Set_Connectable(BleDevice *device);

#define NOTIFY_BUFFER_SZ 32 
uint8_t notify_buffer[NOTIFY_BUFFER_SZ]; //must be divisble by power of 2
RingBuffer_t notify_buffer_ring;

#define L2CAP_ATTEMPTS_MAX 5
bool send_l2cap_request = false;
uint32_t l2cap_request_attempts=0;
uint32_t l2captimer=0;

#ifdef OPTIMIZE_NOTIFICATION
#define SMALL_NOTIFY_PACKET_TIMEOUT 1000
    bool small_packet_notify_present = false;
    uint32_t small_packet_notify_time_ms;
#endif

uint32_t notification_fail_retries =0;    


static void flush_ble_buffers(void) //flushing out old unsent data sitting in memory
{
    debug("Flushing ble buffers");
    while(QUEUE_EMPTY(uint8_t, &getConfiguration()->ble->sendQueue)==false)
    {
        QUEUE_POP(uint8_t, &getConfiguration()->ble->sendQueue);
    }
    RingBuffer_Clear(&notify_buffer_ring);
    
    while(QUEUE_EMPTY(uint8_t, &getConfiguration()->ble->receiveQueue)==false)
    {
        QUEUE_POP(uint8_t, &getConfiguration()->ble->receiveQueue);
    }
    
}


static void app_notification_enable(bool state){

    if(state)
    {
        debug("BLE Notification Enabled");
        getConfiguration()->ble->status = BleStatus::NOTIFICATION_ENABLED;
    }
    else
    {
        debug("BLE Notification Disabled");
        getConfiguration()->ble->status = BleStatus::CONNECTED;
    }
}

static void app_disconnected(void)
{
    debug("BLE App Disconnected");
    getConfiguration()->ble->status = BleStatus::RADIO_ON_NOT_ADVERTISING;
    send_l2cap_request = false;
    flush_ble_buffers();
}

static void app_connected(void)
{
    if(getConfiguration()->ble->status  == BleStatus::CONNECTED)
    {
        debug("Multiple Devices Connected, Dropping Connection");
        ST_BLE_Failed_CB(BleError::MULTIPLE_DEVICES_CONNECTED);
    }
    else
    {
        debug("BLE App connected");
        getConfiguration()->ble->status = BleStatus::CONNECTED;
        send_l2cap_request = true;
        l2captimer = uptimeMs();
        l2cap_request_attempts = 0;        
    }
}

tBleStatus GATT_add_services(void)
{
    tBleStatus ret;
    uint8_t uuid[16];
    
    COPY_VT_SERVICE_UUID(uuid);
    ret = aci_gatt_add_serv(UUID_TYPE_128,  uuid, PRIMARY_SERVICE, 7, &vtServHandle);
    if (ret != BLE_STATUS_SUCCESS) goto fail;    
    
    COPY_APP_COM_UUID(uuid); //setup incoming command pipe
    ret =  aci_gatt_add_char(vtServHandle, UUID_TYPE_128, uuid, 20, CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                             16, 1, &appComCharHandle);
    
    if (ret != BLE_STATUS_SUCCESS) goto fail;

        
    COPY_APP_RSP_UUID(uuid);  //setup outgoing response pipe
    ret =  aci_gatt_add_char(vtServHandle, UUID_TYPE_128, uuid, 20, CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE, 0,
                             16, 1, &appRSPCharHandle);
    
    if (ret != BLE_STATUS_SUCCESS) goto fail;


    
            
    return BLE_STATUS_SUCCESS; 
    
fail: //failed to install service
    return BLE_STATUS_ERROR ;
    
}

tBleStatus GATT_App_Notify(uint8_t* rsp, uint32_t rsplen)
{  
/*
    debug("Notify Len %d",rsplen);
    
    for(int i=0 ; i< rsplen ; i++){
        debug("%d",rsp[i]);
    }
*/    
    return aci_gatt_update_char_value(vtServHandle, appRSPCharHandle, 0, rsplen, rsp);    
}


static void ST_BLE_Failed_CB(uint8_t reason)
{
    debug("BLE Failed Reason:%d", reason);
    getConfiguration()->ble->configured = false;
    //device response failed try to reset and reconfigure device
}


extern "C" {
/**
  * @brief  This function is called whenever there is an ACI event to be processed.
  * @note   Inside this function each event must be identified and correctly
  *         parsed.
  * @param  pckt  Pointer to the ACI packet
  * @retval None
*/
void HCI_Event_CB(void *pckt)
{
    
    hci_uart_pckt *hci_pckt = (hci_uart_pckt*)pckt;
    hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;
    
    //debug("HCI Event Callback Type %d", hci_pckt->type);
    
    if(hci_pckt->type != HCI_EVENT_PKT)
    {
        debug("Unhandled Packet Type");
        return;
    }
    
    //debug("Event %d", event_pckt->evt);
    
    
    switch(event_pckt->evt){          //HCI event
    
        case EVT_DISCONN_COMPLETE:        //Device was disconnected
        {
           evt_disconn_complete *cc = (evt_disconn_complete *)event_pckt->data;
           
           debug("Disconnected status:%d handle:%d reason%d",cc->status,
                                                        cc->handle,cc->reason);
                                
           app_disconnected();
           
           if(ST_BLE_Set_Connectable(getConfiguration()->ble) != BLE_STATUS_SUCCESS)
           {
              ST_BLE_Failed_CB(BleError::SET_CONNECTABLE_FAILED); //default disconnect behavior
           }           
        }
        break;
    
        case EVT_LE_META_EVENT:
        {
          evt_le_meta_event *evt = (evt_le_meta_event *)event_pckt->data;
          //debug("Meta event se: %d", evt->subevent);
          
          switch(evt->subevent){
              
            //debug("LE META SUB EVENT %d",evt->subevent);
            
            case EVT_LE_CONN_COMPLETE:
            {
                evt_le_connection_complete *cc = (evt_le_connection_complete *)evt->data;
                
                conn_handle = cc->handle;
                debug("Device connected over BLE to %x:%x:%x:%x:%x:%x at %d",cc->peer_bdaddr[0],
                                cc->peer_bdaddr[1],cc->peer_bdaddr[2],cc->peer_bdaddr[3],cc->peer_bdaddr[4],
                                cc->peer_bdaddr[5],uptimeMs());
                                
                app_connected();
                #ifdef BLE_NO_ACT_TIMEOUT_ENABLE
                    ble_no_act_active = true;            
                    ble_no_act_timer_starttime = uptimeMs();
                #endif
                
            }
            break;
            case EVT_LE_CONN_UPDATE_COMPLETE:
                //debug("Connection Updated");
            break;
            
          }
        }
        break;
    
        case EVT_VENDOR:
        {
            evt_blue_aci *blue_evt = (evt_blue_aci*)event_pckt->data;
            switch(blue_evt->ecode)
            {
                    
                case EVT_BLUE_GATT_READ_PERMIT_REQ: //application tried to read a characteristic
                {
                   // evt_gatt_read_permit_req *pr = (evt_gatt_read_permit_req*)blue_evt->data;                    
                    //did not implement any readable characteristics at the moment   
                                    
                }
                break;

                case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED: //application wrote to a characteristic attribute either data or operation on flag such as enable notification
                {  
                    evt_gatt_attr_modified *evt = (evt_gatt_attr_modified*)blue_evt->data;
                    
                    if( evt->attr_handle == appComCharHandle + 1) //we recieved some data in consequence of write to this characteristic
                    {    
                        //debug("Command %d bytes :",evt->data_length);
                        
                        for(int i = 0; i < evt->data_length ; i++) { //todo better way to point to device
                        
                            if(QUEUE_FULL(uint8_t,(QUEUE_TYPE(uint8_t)*)&getConfiguration()->ble->receiveQueue))
                            {
                                debug("Command Queue Busy");
                                break;
                            }
                            QUEUE_PUSH(uint8_t,(QUEUE_TYPE(uint8_t)*)&getConfiguration()->ble->receiveQueue, (uint8_t) evt->att_data[i]);
                        }
                        processQueue((QUEUE_TYPE(uint8_t)*) &getConfiguration()->ble->receiveQueue, openxc::interface::ble::handleIncomingMessage);//processQueue will dump queue automatically if full    
                    }
                    else if(evt->attr_handle == appRSPCharHandle + 2)  //Notifications were enabled or disabled
                    {
                        if(evt->att_data[0] == 0x01)
                        {
                            app_notification_enable(TRUE);
                            #ifdef BLE_NO_ACT_TIMEOUT_ENABLE
                                debug("BLE_NO_ACT_TIMEOUT_ENABLE is disabled");
                                ble_no_act_active = false;            //disable dropping link once notification enabled
                            #endif
                        }
                        if(evt->att_data[0] == 0x00)
                        {
                            app_notification_enable(FALSE);
                        }    
                    }
                }
                break;
                case EVT_BLUE_INITIALIZED:
                {
                    evt_blue_initialized *v= (evt_blue_initialized*)event_pckt->data;
                    if(v->reason_code == 1)
                    {
                        debug("BLUE Radio Application Initialized");
                    }
                }
                break;
                case EVT_BLUE_L2CAP_CONN_UPD_RESP:
                {
                        evt_l2cap_conn_upd_resp *resp = (evt_l2cap_conn_upd_resp*)blue_evt->data;
                        //debug("L2CAP Updated Response:%d",resp->result);
                        send_l2cap_request = false;
                }
                break;
                
                
                default:
                {
                    debug("Unhandled Event Vendor Code Type %d",event_pckt->evt);
                }
                break;
            }//////VENDOR CODE TYPE
        }////EVT VENDOR
        break;
        
        default:
        {
            debug("Unhandled Event %d",event_pckt->evt);
        }    
        break;    
      }////HCI event
}
}

static tBleStatus ST_BLE_Disconnect(BleDevice* device)
{
    tBleStatus ret;
    debug("Disconnecting Device");
    ret = aci_gap_terminate(conn_handle,HCI_CONNECTION_TIMEOUT);
    
    if (ret != BLE_STATUS_SUCCESS)
    {
      debug("Connection termination Failed");
    }
    return ret; 
}

/**
 * @brief  Set the device in connectable mode over BLE
 * @description Configure GAP related parameters to start advertisement and reset GATT session 
 * @param  none
 * @return tBleStatus   Return BLE STATUS code (SEE BLE STATUS CODES)
 */
static tBleStatus ST_BLE_Set_Connectable(BleDevice* device)
{  
   tBleStatus ret;

   unsigned char adv[20]; 
   uint8_t uuid[16];
   
    COPY_VT_SERVICE_UUID(uuid);
    adv[0] = 0x11;
    adv[1] = 0x06;
    memcpy(&adv[2],uuid,16);
    
   /* disable scan response */
   ret = hci_le_set_scan_resp_data(18,(const uint8_t*)adv);
   if(ret != BLE_STATUS_SUCCESS)
   {
       return ret;//failed
   }
   adv[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
   ret = strlen(device->blesettings.advname);
   
   if(ret > 15)
   {
       ret = 15;
   }
   memcpy(&adv[1],device->blesettings.advname,ret);
   adv[ret+1] = 0;

  ret = aci_gap_set_discoverable(ADV_IND, (device->blesettings.adv_min_ms*1000)/625, (device->blesettings.adv_max_ms*1000)/625, STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
                                 ret + 1 ,(const char*) adv , 0, NULL, 0, 0); //using default slave con parameters
  
  if (ret != BLE_STATUS_SUCCESS)
  {
      debug("Gap set discoverable mode failed ,%d",ret);
      return ret; //Critical BLE stack failure
  }
  
  /*disable advertising power level */
  ret = aci_gap_delete_ad_type(AD_TYPE_TX_POWER_LEVEL); 
  
  if (ret != BLE_STATUS_SUCCESS)
  {
      debug("Gap Delete of Power Level Broadcast Failed");
      return ret; //Critical BLE stack failure
  }
 
  debug("Gap set in connectable mode success");
  
  return BLE_STATUS_SUCCESS;
  
}

extern "C" {
    void __debug_hci(const unsigned char *s)
    {
        debug((const char*)s);
    }
}

uint8_t ST_BLE_Get_Version(uint8_t *hwVersion, uint16_t *fwVersion)
{
  uint8_t status;
  uint8_t hci_version, lmp_pal_version;
  uint16_t hci_revision, manufacturer_name, lmp_pal_subversion;

  status = hci_le_read_local_version(&hci_version, &hci_revision, &lmp_pal_version, 
                     &manufacturer_name, &lmp_pal_subversion);

  if (status == BLE_STATUS_SUCCESS) {
    *hwVersion = hci_revision >> 8;
    *fwVersion = (hci_revision & 0xFF) << 8;              // Major Version Number
    *fwVersion |= ((lmp_pal_subversion >> 4) & 0xF) << 4; // Minor Version Number
    *fwVersion |= lmp_pal_subversion & 0xF;               // Patch Version Number
  }

  return status;
}
 
bool openxc::interface::ble::initialize(BleDevice* device)
{
    uint8_t ret;
    uint32_t timer;
    uint8_t macadd[10];
    uint8_t hwVersion; 
    uint16_t fwVersion;
    
    RingBuffer_Initialize(&notify_buffer_ring,(char*)notify_buffer, NOTIFY_BUFFER_SZ);
    
    device->status = BleStatus::RADIO_OFF;
    
    initializeCommon(device);
    
    app_disconnected();
    
    debug("BLE radio power down radio"); 
    
    SPBTLE_RST_CONFIGURE();    

    BlueNRG_PowerOff();
    
    HCI_Init(); //initialize list structures for messaging
    
    TRISBCLR = (1 << 4);

    LATBCLR  = (1 << 4); //Disable optional GPS
    
    TRISBCLR = (1 << 9);

    LATBCLR  = (1 << 9); 
     
    BlueNRG_SpiInit(); // initialize SPI and interrupt for notifications

    BlueNRG_ISRInit(); //Configure change notification interrupt for BTLE IRQ handling
    
    debug("Reseting BLE Radio");
    
    BlueNRG_RST(); // perform a hardware reset
    
    debug("BLE radio is powered up"); 
    
    //timer = uptimeMs() + 100;
    
    //while(uptimeMs() < timer);
    
    //Write BLE MAC address and set it as public
    //ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN,
    //                                device->blesettings.bdaddr);

    
    //if(ret != BLE_STATUS_SUCCESS)
    //{
    //     debug("BLE MAC address configuration failed code: %d", ret);
    //     goto error; //TBD add more meaning full error codes
    //}
    
    //Initialize GATT server, note that it is important to do so before initializing gap
    
    if(ret = ST_BLE_Get_Version(&hwVersion,&fwVersion), ret == BLE_STATUS_SUCCESS){
        debug("Rev Hw:%x Fw%x",hwVersion,fwVersion);
        
    }else{
        debug("Firmware revision read failed");
    }
    
    ret = aci_gatt_init();    
    
    if(ret != BLE_STATUS_SUCCESS)
    {
         debug("BLE Gatt Init Failed");
         goto error;
    }
    //Initialize GAP, note that two default services are available on the device by default
    ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0, 0x0A, &service_handle, &dev_name_char_handle, &appearance_char_handle);

    if(ret != BLE_STATUS_SUCCESS)
    {
         debug("BLE GAP init Failed");
         goto error;
    }
    
    if(ret = hci_read_bd_addr(device->blesettings.bdaddr), ret != BLE_STATUS_SUCCESS){
        debug("Ble mac add read failed");
		memset(&device->blesettings.bdaddr[0],0xFF,6);		
    }

    //debug("Mac address %x:%x:%x:%x:%x:%x", device->blesettings.bdaddr[0], device->blesettings.bdaddr[1], device->blesettings.bdaddr[2],
    //                device->blesettings.bdaddr[3], device->blesettings.bdaddr[4], device->blesettings.bdaddr[5]);
    
	sprintf(device_adv_name,"OpenXC-VI-%02X%02X",device->blesettings.bdaddr[1],device->blesettings.bdaddr[0]);
								
				
	device->blesettings.advname = (const char*)device_adv_name;
	
    //Initialize device name charactertistic
    ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen((const char*)device_gap_name), (uint8_t *)device_gap_name);  

    if(ret != BLE_STATUS_SUCCESS)
    {
         debug("BLE Gap Advertisment Name Update Failed");
         goto error;
    }

    //BLE stack initialized

    //default is maximum power
    ret = aci_hal_set_tx_power_level(1,4); 

    if(ret != BLE_STATUS_SUCCESS)
    {
        debug("ACI Power Level Update Failed");
        goto error;
    }

     //ADD Service to GATT Gattdb.c
    ret = GATT_add_services();

    if(ret != BLE_STATUS_SUCCESS)
    {
        debug("Creation of Services on Gatt Failed");
        goto error;
    }

    ret = ST_BLE_Set_Connectable(device); //set device in connectable advertising mode
    
    if(ret != BLE_STATUS_SUCCESS)
    {
        debug("BLE set mode connectable failed");
        goto error;
    }
    
    
    debug("BLE Configured Sucessfully");
    
    device->configured = true;
    
    device->status  = BleStatus::ADVERTISING;
    
    return true;
    
error:
    device->configured = false;
    BlueNRG_PowerOff();
    BlueNRG_ISRDeInit();
    return false;
}


void openxc::interface::ble::deinitialize(BleDevice* device)
{
    debug("BLE Deinitialize");
    if(device->status  == BleStatus::CONNECTED)
    {
        ST_BLE_Disconnect(device);
    }
    BlueNRG_PowerOff();
    BlueNRG_ISRDeInit();
    device->status  = BleStatus::RADIO_OFF;
    device->configured = false;
    deinitializeCommon(device);
}
 

 
/* handles configuration and connection state over BLE*/

void openxc::interface::ble::read(BleDevice* device) {
    
    int16_t err;
    uint8_t ret;
    
    
    if(device->configured == false)
    {
        debug("BLE device failure attempt to reconfigure");
        if(!initialize(device))
            debug("Init Failed");
    }
    err = HCI_Process();

    //send_l2cap_request = false;
    if(send_l2cap_request == true &&  uptimeMs() > l2captimer + 1000){
        
        l2captimer = uptimeMs(); 
        //debug("Updating L2CAP connection parameters");
        ret = aci_l2cap_connection_parameter_update_request(conn_handle, device->blesettings.slave_min_ms, device->blesettings.slave_max_ms, 0, 600); 
        
        if(ret != BLE_STATUS_SUCCESS)
            debug("Failed L2CAP Connection Update");
        
        l2cap_request_attempts++;
        if(l2cap_request_attempts >= L2CAP_ATTEMPTS_MAX)
        {
            debug("Unable to Update connection over L2CAP, possibly a slow device");
            send_l2cap_request = false;
        }
        
        return;
    }
    
    err = HCI_Process();
     
    if( err < 0) 
    {
        //ST_BLE_Failed_CB(BleError::ISR_SPI_READ_TIMEDOUT);
        //debug("ISR Timed-out");
    }
    
#ifdef BLE_NO_ACT_TIMEOUT_ENABLE
    if(ble_no_act_active == true && device->status == BleStatus::CONNECTED)
    {
        if(uptimeMs() > ble_no_act_timer_starttime + ble_no_act_timeout_max)
        {
            debug("Unknown host behaviour attempting to drop connection %d",uptimeMs());
            if(ST_BLE_Disconnect(device) == BLE_STATUS_SUCCESS)//todo timeout if this always fails
            {
                debug("Ble host disconnected");
                
            }
            else{
                debug("Unknown error disconnection failed");
                ST_BLE_Failed_CB(BleError::DISCONNECT_DEVICE_FAILED);
            }
            ble_no_act_active = false;
        }
    }
#endif        
}

bool openxc::interface::ble::connected(BleDevice* device) 
{
    if(device != NULL && device->configured && device->status == BleStatus::NOTIFICATION_ENABLED)
    {
        return true; //return app connection status
    }
    return false;
}


void openxc::interface::ble::processSendQueue(BleDevice* device) 
{    
    static uint8_t ndata[21];
    char d;uint8_t ret;
    uint32_t sz;
    
    if(connected(device))
    {

        while((RingBuffer_FreeSpace(&notify_buffer_ring) > 0) && QUEUE_EMPTY(uint8_t, &device->sendQueue)==false)
        {
            d = QUEUE_POP(uint8_t,&device->sendQueue);
            RingBuffer_Write(&notify_buffer_ring, &d, 1);
            
        }
        
        sz = RingBuffer_UsedSpace(&notify_buffer_ring);
        
        if(sz > 0)
        {
            if(sz > 20)
            {
                sz = 20;
        
                small_packet_notify_present = false;

            }
            else
            {

                if(small_packet_notify_present == false)
                {
                    small_packet_notify_time_ms = uptimeMs();
                    small_packet_notify_present = true;
                    return;
                }
                else{
                    if( uptimeMs() > small_packet_notify_time_ms + SMALL_NOTIFY_PACKET_TIMEOUT)
                    {
                        small_packet_notify_time_ms = uptimeMs();
                        small_packet_notify_present = false;
                    }
                    else
                    {
                        return;
                    }
                }

            }
            
            RingBuffer_Peek(&notify_buffer_ring,(char*) ndata, sz);
            
            //Avoiding retries to allow more bandwidth to foreground application
            
            ret = GATT_App_Notify((uint8_t*)ndata, sz);
            
            if( ret != BLE_STATUS_SUCCESS)
            {
                if(ret == BLE_STATUS_TIMEOUT)
                {
                    debug("Notification Timed Out %d",ret);
                    app_disconnected();
                    if(ST_BLE_Set_Connectable(getConfiguration()->ble) != BLE_STATUS_SUCCESS)
                    {
                        ST_BLE_Failed_CB(BleError::SET_CONNECTABLE_FAILED);
                    }    
                }
                else if(notification_fail_retries > MAX_BLE_NOTIFY_RETRIES)
                {
                    debug("Notification failed code %d",ret);
                    notification_fail_retries = 0;
                    notification_fail_retries++;
                }
                
            }
            else
            {
                notification_fail_retries = 0;
                RingBuffer_Read(&notify_buffer_ring,(char*) ndata, sz);
                
            }

        }    
    }
    return;
}

#endif

