#include "payload.h"
#include <cmp.h>
#include <stdlib.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "messagepack.h"
#include "util/strutil.h"
#include "util/log.h"
#include "config.h"
#include "util/log.h"

#define MESSAGE_PACK_SERIAL_BUF_SZ      128
#define MESSAGE_PACK_FIXMAP_MARKER     0x80    
#define MESSAGE_PACK_FIXMAP_SIZE     0x0F
#define MESSAGE_PACK_FALSE_MARKER    0xC2
#define MESSAGE_PACK_TRUE_MARKER     0xC3
#define MESSAGE_PACK_DOUBLE_MARKER   0xCB
#define MESSAGE_PACK_FIXSTR_MARKER   0xA0
#define MESSAGE_PACK_FIXMAP_MARKER   0x80
#define MESSAGE_PACK_MAX_STRLEN      0x1F

#define MAX_STRLEN 25
#define MAX_BINLEN 25
    
namespace payload = openxc::payload;
using openxc::util::log::debug;

const char openxc::payload::messagepack::VERSION_COMMAND_NAME[] = "version";
const char openxc::payload::messagepack::DEVICE_ID_COMMAND_NAME[] = "device_id";
const char openxc::payload::messagepack::DEVICE_PLATFORM_COMMAND_NAME[] = "platform";
const char openxc::payload::messagepack::DIAGNOSTIC_COMMAND_NAME[] = "diagnostic_request";
const char openxc::payload::messagepack::PASSTHROUGH_COMMAND_NAME[] = "passthrough";
const char openxc::payload::messagepack::ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME[] = "af_bypass";
const char openxc::payload::messagepack::PAYLOAD_FORMAT_COMMAND_NAME[] = "payload_format";
const char openxc::payload::messagepack::PREDEFINED_OBD2_REQUESTS_COMMAND_NAME[] = "predefined_obd2";
const char openxc::payload::messagepack::MODEM_CONFIGURATION_COMMAND_NAME[] = "modem_configuration";
const char openxc::payload::messagepack::RTC_CONFIGURATION_COMMAND_NAME[] = "rtc_configuration";

const char openxc::payload::messagepack::PAYLOAD_FORMAT_MESSAGEPACK_NAME[] = "messagepack";
const char openxc::payload::messagepack::PAYLOAD_FORMAT_PROTOBUF_NAME[] = "protobuf";
const char openxc::payload::messagepack::PAYLOAD_FORMAT_JSON_NAME[] = "json";

const char openxc::payload::messagepack::COMMAND_RESPONSE_FIELD_NAME[] = "command_response";
const char openxc::payload::messagepack::COMMAND_RESPONSE_MESSAGE_FIELD_NAME[] = "message";
const char openxc::payload::messagepack::COMMAND_RESPONSE_STATUS_FIELD_NAME[] = "status";

const char openxc::payload::messagepack::BUS_FIELD_NAME[] = "bus";
const char openxc::payload::messagepack::ID_FIELD_NAME[] = "id";
const char openxc::payload::messagepack::DATA_FIELD_NAME[] = "data";
const char openxc::payload::messagepack::NAME_FIELD_NAME[] = "name";
const char openxc::payload::messagepack::VALUE_FIELD_NAME[] = "value";
const char openxc::payload::messagepack::EVENT_FIELD_NAME[] = "event";
const char openxc::payload::messagepack::FRAME_FORMAT_FIELD_NAME[] = "frame_format";

const char openxc::payload::messagepack::FRAME_FORMAT_STANDARD_NAME[] = "standard";
const char openxc::payload::messagepack::FRAME_FORMAT_EXTENDED_NAME[] = "extended";

const char openxc::payload::messagepack::DIAGNOSTIC_MODE_FIELD_NAME[] = "mode";
const char openxc::payload::messagepack::DIAGNOSTIC_PID_FIELD_NAME[] = "pid";
const char openxc::payload::messagepack::DIAGNOSTIC_SUCCESS_FIELD_NAME[] = "success";
const char openxc::payload::messagepack::DIAGNOSTIC_NRC_FIELD_NAME[] = "negative_response_code";
const char openxc::payload::messagepack::DIAGNOSTIC_PAYLOAD_FIELD_NAME[] = "payload";
const char openxc::payload::messagepack::DIAGNOSTIC_VALUE_FIELD_NAME[] = "value";
const char openxc::payload::messagepack::SD_MOUNT_STATUS_COMMAND_NAME[] = "sd_mount_status";


enum msgpack_var_type{TYPE_STRING,TYPE_NUMBER,TYPE_TRUE,TYPE_FALSE,TYPE_BINARY,TYPE_MAP};

int32_t dynamic_allocation_bytes = 0;

typedef struct sMsgPackNode{
    char * string; //points to string header and not the string inorder to derive string 
    enum   msgpack_var_type type;
    double valuedouble;
    int    valueint;
    char *valuestring;
    uint8_t *bin;
    uint8_t binsz;
    sMsgPackNode *next,*child;
}sMsgPackNode; 

typedef struct{
    uint8_t MsgPackMapPairCount;
}meta;

typedef struct{
    uint8_t * end;
    uint8_t * start;
    uint8_t * rp;
    uint8_t * wp;
    meta     mobj;
}sFile;    

sMsgPackNode* msgPackParse(uint8_t* buf,uint32_t* len);

static size_t msgPackWriteBuffer(cmp_ctx_t *ctx, const void *data, size_t count) {
    
    sFile * smsgb = (sFile *)ctx->buf;
    
    if(ctx->error > 0)return 0;
    
    if(smsgb->wp + count > (smsgb->end+1)){
        return 0;
    }
    memcpy(smsgb->wp, data, count);
    smsgb->wp  += count;
    return count;
}

static bool msgPackReadBuffer(cmp_ctx_t *ctx, void *data, size_t count) {
    sFile * smsgb = (sFile *)ctx->buf;
    if(smsgb->rp + count > (smsgb->end+1)){
        return 0;
    }
    memcpy((void*)data, (const void*)smsgb->rp, count);
    smsgb->rp  += count;
    
    return count;
}

static void msgPackInitBuffer(sFile* smsgpackb, uint8_t* buf, uint8_t len){

    smsgpackb->end   = buf + len - 1;
    smsgpackb->start = buf;
    smsgpackb->rp    = buf;
    smsgpackb->wp    = buf;
    
}
 
static void msgPackAddObjectString(cmp_ctx_t *ctx, const char* fname ,const char* obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_str(ctx, (const char *)obj, strlen(obj));
    s->mobj.MsgPackMapPairCount++;
}
static void msgPackAddObjectDouble(cmp_ctx_t *ctx, const char* fname ,double obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_double(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
static void msgPackAddObject8bNumeric(cmp_ctx_t *ctx, const char* fname ,uint8_t obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_u8(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
static void msgPackAddObject16bNumeric(cmp_ctx_t *ctx, const char* fname ,uint16_t obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_u16(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
/*
static void msgPackAddObject32bNumeric(cmp_ctx_t *ctx, const char* fname ,uint32_t obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_u32(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
*/
static void msgPackAddObject64bNumeric(cmp_ctx_t *ctx, const char* fname ,uint32_t obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_u64(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
/*
static void msgPackAddObjectFloat(cmp_ctx_t *ctx, const char* fname ,float obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_float(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
*/
static void msgPackAddObjectBoolean(cmp_ctx_t *ctx, const char* fname ,bool obj){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    cmp_write_bool(ctx, obj);
    s->mobj.MsgPackMapPairCount++;
}
static void msgPackAddObjectBinary(cmp_ctx_t *ctx, const char* fname ,uint8_t* obj, uint8_t len){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    //cmp_write_bin_marker(ctx, len);
    cmp_write_bin(ctx,(const void *)obj, len); //writes marker as well
    s->mobj.MsgPackMapPairCount++;
}
/*
static void msgPackAddObjectMap(cmp_ctx_t *ctx, const char* fname ,uint8_t* obj,uint8_t len){
    sFile *s = (sFile *)ctx->buf;
    cmp_write_str(ctx, (const char *)fname, strlen((const char *)fname));
    msgPackWriteBuffer(ctx, obj, len); 
    s->mobj.MsgPackMapPairCount++;
}
*/
    

static void msgPackAddDynamicField(cmp_ctx_t *ctx, openxc_DynamicField* field) {

    if(field->type == openxc_DynamicField_Type_NUM) {
        cmp_write_double(ctx, field->numeric_value);
    } else if(field->type == openxc_DynamicField_Type_BOOL) {
        cmp_write_bool(ctx, field->boolean_value);
    } else if(field->type == openxc_DynamicField_Type_STRING) {
        cmp_write_str(ctx, field->string_value, strlen(field->string_value));
    }

} 

static void serializeSimple(openxc_VehicleMessage* message,  cmp_ctx_t *ctx) {
        
    const char* name = message->simple_message.name;    
    sFile *s = (sFile *)ctx->buf;
    msgPackAddObjectString(ctx, payload::messagepack::NAME_FIELD_NAME, name);

    if(message->simple_message.value.type != openxc_DynamicField_Type_UNUSED) {
        cmp_write_str(ctx, payload::messagepack::VALUE_FIELD_NAME, strlen(payload::messagepack::VALUE_FIELD_NAME));
        msgPackAddDynamicField(ctx, &message->simple_message.value);
        s->mobj.MsgPackMapPairCount++;
    }
    
    if(message->simple_message.event.type != openxc_DynamicField_Type_UNUSED) {
        cmp_write_str(ctx, payload::messagepack::EVENT_FIELD_NAME, strlen(payload::messagepack::EVENT_FIELD_NAME));        
        msgPackAddDynamicField(ctx, &message->simple_message.event);
        s->mobj.MsgPackMapPairCount++;
    }
}

void serializeCan(openxc_VehicleMessage* message, cmp_ctx_t *ctx) {
    
    msgPackAddObject8bNumeric(ctx, payload::messagepack::BUS_FIELD_NAME, message->can_message.bus);
    
    msgPackAddObject8bNumeric(ctx, payload::messagepack::ID_FIELD_NAME, message->can_message.bus);
    
    msgPackAddObjectBinary(ctx, payload::messagepack::DATA_FIELD_NAME,
            message->can_message.data.bytes,message->can_message.data.size);
            
    if(message->can_message.frame_format != openxc_CanMessage_FrameFormat_UNUSED) {
        msgPackAddObjectString(ctx, payload::messagepack::FRAME_FORMAT_FIELD_NAME,
                message->can_message.frame_format == openxc_CanMessage_FrameFormat_STANDARD ?
                    payload::messagepack::FRAME_FORMAT_STANDARD_NAME :
                        payload::messagepack::FRAME_FORMAT_EXTENDED_NAME);
    }
}

static void serializeDiagnostic(openxc_VehicleMessage* message, cmp_ctx_t *ctx) {
        
    msgPackAddObject8bNumeric(ctx, payload::messagepack::BUS_FIELD_NAME,
            message->diagnostic_response.bus);
    msgPackAddObject8bNumeric(ctx, payload::messagepack::ID_FIELD_NAME,
            message->diagnostic_response.message_id);
    msgPackAddObject8bNumeric(ctx, payload::messagepack::DIAGNOSTIC_MODE_FIELD_NAME,
            message->diagnostic_response.mode);
    msgPackAddObjectBoolean(ctx, payload::messagepack::DIAGNOSTIC_SUCCESS_FIELD_NAME,
            message->diagnostic_response.success);

            
    if(message->diagnostic_response.pid != 0) {
        msgPackAddObject16bNumeric(ctx, payload::messagepack::DIAGNOSTIC_PID_FIELD_NAME,
                message->diagnostic_response.pid);
    }

    if(message->diagnostic_response.negative_response_code != 0) {
        msgPackAddObjectDouble(ctx, payload::messagepack::DIAGNOSTIC_NRC_FIELD_NAME,
                message->diagnostic_response.negative_response_code);
    }

    if(message->diagnostic_response.value.type != openxc_DynamicField_Type_UNUSED) {
        cmp_write_str(ctx, payload::messagepack::VALUE_FIELD_NAME, strlen(payload::messagepack::VALUE_FIELD_NAME));
        msgPackAddDynamicField(ctx, &message->diagnostic_response.value);
                
    } else if(message->diagnostic_response.payload.size > 0) {
        msgPackAddObjectBinary(ctx, payload::messagepack::DIAGNOSTIC_PAYLOAD_FIELD_NAME, 
                message->diagnostic_response.payload.bytes, message->diagnostic_response.payload.size);
        
    }
 
}

static bool serializeCommandResponse(openxc_VehicleMessage* message, cmp_ctx_t *ctx) {
    
    const char* typeString = NULL;
    
    if(message->command_response.type == openxc_ControlCommand_Type_VERSION) {
        typeString = payload::messagepack::VERSION_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_DEVICE_ID) {
        typeString = payload::messagepack::DEVICE_ID_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PLATFORM) {
        typeString = payload::messagepack::DEVICE_PLATFORM_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_DIAGNOSTIC) {
        typeString = payload::messagepack::DIAGNOSTIC_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PASSTHROUGH) {
        typeString = payload::messagepack::PASSTHROUGH_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS) {
        typeString = payload::messagepack::ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PAYLOAD_FORMAT) {
        typeString = payload::messagepack::PAYLOAD_FORMAT_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS) {
        typeString = payload::messagepack::PREDEFINED_OBD2_REQUESTS_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_MODEM_CONFIGURATION) {
        typeString = payload::messagepack::MODEM_CONFIGURATION_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_RTC_CONFIGURATION) {
        typeString = payload::messagepack::RTC_CONFIGURATION_COMMAND_NAME;
    } else if(message->command_response.type == openxc_ControlCommand_Type_SD_MOUNT_STATUS) {
        typeString = payload::messagepack::SD_MOUNT_STATUS_COMMAND_NAME;
    } else {
        return false;
    }

    msgPackAddObjectString(ctx, payload::messagepack::COMMAND_RESPONSE_FIELD_NAME,
            typeString);
            
            
    if(message->command_response.type != openxc_ControlCommand_Type_UNUSED) {
        msgPackAddObjectString(ctx,
                payload::messagepack::COMMAND_RESPONSE_MESSAGE_FIELD_NAME,
                message->command_response.message);
    }

    if(message->command_response.type != openxc_ControlCommand_Type_UNUSED) {
        msgPackAddObjectBoolean(ctx,
                payload::messagepack::COMMAND_RESPONSE_STATUS_FIELD_NAME,
                message->command_response.status);
    }
    return true;
}


int openxc::payload::messagepack::serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length)
{

    sFile smsgpackb;
    
    uint8_t MessagePackBuffer[MESSAGE_PACK_SERIAL_BUF_SZ]; 
    
    size_t finalLength = 0;

    cmp_ctx_t cmp;
    
    memset((void*)&smsgpackb,0,sizeof(smsgpackb));
    
    msgPackInitBuffer(&smsgpackb, MessagePackBuffer, MESSAGE_PACK_SERIAL_BUF_SZ);
    
    cmp_init(&cmp,(void*)&smsgpackb, msgPackReadBuffer, msgPackWriteBuffer);
    
    cmp_write_map(&cmp,2);
    
    //"{\"command\": \"diagnostic_request\", \"actio"
    //"n\": \"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1}}\0";
    
    //"{\"command\": \"version\"}\0";
    
    //msgPackAddObjectString(&cmp, "command", "device_id");
    //cmp_write_map(&cmp,3);
    //msgPackAddObjectString(&cmp, "command", "passthrough");
    //msgPackAddObject8bNumeric(&cmp, "bus", 1);
    //msgPackAddObjectBoolean(&cmp, "enabled", 1);
    //goto x;
    /*
    cmp_write_map(&cmp,3);
    msgPackAddObjectString(&cmp, "command", "diagnostic_request");
    msgPackAddObjectString(&cmp, "action", "add");
    cmp_write_str(&cmp, "request", 7);
    cmp_write_map(&cmp,3);
    msgPackAddObject8bNumeric(&cmp, "bus",  1);
    msgPackAddObject8bNumeric(&cmp, "id",   2);
    msgPackAddObject8bNumeric(&cmp, "mode", 1);
    smsgpackb.mobj.MsgPackMapPairCount -=3;
    goto x;
    */
    if(message->type != openxc_VehicleMessage_Type_UNUSED) {
        
        msgPackAddObject64bNumeric(&cmp, "timestamp", message->timestamp);
                
    }
    if(message->type == openxc_VehicleMessage_Type_SIMPLE) {
        serializeSimple(message, &cmp);
    } else if(message->type == openxc_VehicleMessage_Type_CAN) {
        serializeCan(message, &cmp);
    } else if(message->type == openxc_VehicleMessage_Type_DIAGNOSTIC) {
        serializeDiagnostic(message, &cmp);
    } else if(message->type == openxc_VehicleMessage_Type_COMMAND_RESPONSE) {
        serializeCommandResponse(message, &cmp);
    } else {
        debug("Unrecognized message type -- not sending");
    }
    
    if(cmp.error > 0){
        debug("%s\n\n", cmp_strerror(&cmp));
        return 0;
    }
    if( smsgpackb.mobj.MsgPackMapPairCount > MESSAGE_PACK_FIXMAP_SIZE)
    {
        debug("Unhandled, excedded fix map limit %d",smsgpackb.mobj.MsgPackMapPairCount);
        return 0;
    }
    
    smsgpackb.start[0] = MESSAGE_PACK_FIXMAP_MARKER | smsgpackb.mobj.MsgPackMapPairCount; //todo create a function to add this more gracefully
    

    finalLength = smsgpackb.wp - smsgpackb.start;
    
    memcpy(payload, MessagePackBuffer, finalLength);
    
    //debug ("parsing %d bytes",finalLength);
    //openxc_VehicleMessage m;
    //int l = openxc::payload::messagepack::deserialize(payload,finalLength,&m);
    //debug ("deserialized %d bytes",l);
    //while(1);
/*
    for(int i = 0;i < finalLength; i ++)
    {
        debug("%x", payload[i]);
    }
    
    debug("\n\n");

    
*/
    return finalLength;
}
sMsgPackNode * msgPackSeekNode(sMsgPackNode* root,const char * name ){
    sMsgPackNode * node = root;
    while(node){
        if(strcmp(node->string, name) == 0){
            if(node->child)return node->child;
            
            return node;
        }
        node = node->next;
    }
    //debug("Missing %s",name);
    return node;
}

sMsgPackNode* getnode(cmp_ctx_t * ctx){

    
    char vstr[MAX_STRLEN+1];
    char nstr[MAX_STRLEN+1];
    char binarr[MAX_BINLEN+1];
    uint32_t str_size;
    char binsz = 0;
    cmp_object_t obj;
    enum msgpack_var_type type;
    double vd=0.0;
    int    vi=0;
    sMsgPackNode* ch;
    
    str_size = MAX_STRLEN;
    
    if (!cmp_read_str(ctx, nstr, &str_size)){
        //debug("Node incomplete %s",cmp_strerror(ctx));
        return NULL;
    }
    
    if (cmp_read_object(ctx, &obj) == true) { //read value
        switch (obj.type) {
            case CMP_TYPE_FIXARRAY:
            case CMP_TYPE_ARRAY16:
            case CMP_TYPE_ARRAY32:
            case CMP_TYPE_EXT8:
            case CMP_TYPE_EXT16:
            case CMP_TYPE_EXT32:
            case CMP_TYPE_FIXEXT1:
            case CMP_TYPE_FIXEXT2:
            case CMP_TYPE_FIXEXT4:
            case CMP_TYPE_FIXEXT8:
            case CMP_TYPE_FIXEXT16:
            case CMP_TYPE_NIL:
            case CMP_TYPE_UINT64:
            case CMP_TYPE_NEGATIVE_FIXNUM:
            case CMP_TYPE_SINT8:
            case CMP_TYPE_SINT16:
            case CMP_TYPE_SINT32:
            case CMP_TYPE_SINT64:
            {
                debug("Unhandled object %d",obj.type);
                return NULL;
                break;
            }
            case CMP_TYPE_FIXMAP:
            case CMP_TYPE_MAP16:
            case CMP_TYPE_MAP32:
            {
                sFile * s = (sFile *)ctx->buf;
                s->rp--;
                uint32_t plen = (s->end - s->rp) + 1;
                ch = msgPackParse(s->rp,&plen);
                if(ch == NULL){
                    //debug("Child incomplete %s",cmp_strerror(ctx));
                    return NULL;
                }
                    
                s->rp += plen + 1;
                type = msgpack_var_type::TYPE_MAP;
                 break;
            }
            case CMP_TYPE_BIN8:
            case CMP_TYPE_BIN16:
            case CMP_TYPE_BIN32:
            {
                if(obj.as.bin_size > MAX_BINLEN)
                {
                    debug("Payload exceeded limit %d bytes",obj.as.bin_size);
                    return NULL;
                }
                if (!msgPackReadBuffer(ctx, binarr, obj.as.bin_size))
                {
                    //debug("Data missing total %d %d %d //%dbytes",obj.as.bin_size,s->rp-s->start,s->end-s->rp,s->end-s->start);
                    return NULL;
                }
                binsz = obj.as.bin_size;
                type = msgpack_var_type::TYPE_BINARY;
                break;
            }
            
            case CMP_TYPE_FIXSTR:
            case CMP_TYPE_STR8:
            case CMP_TYPE_STR16:
            case CMP_TYPE_STR32:
            {
                if(obj.as.str_size > MAX_STRLEN)
                    return NULL;
            
                if (!msgPackReadBuffer(ctx, vstr, obj.as.str_size))
                    return NULL;
                
                vstr[obj.as.str_size] = 0;
                type = msgpack_var_type::TYPE_STRING;
                break;
            }    
                
            case CMP_TYPE_BOOLEAN:
            {
                if (obj.as.boolean)
                    type = msgpack_var_type::TYPE_TRUE;
                else
                    type = msgpack_var_type::TYPE_FALSE;
                
                break;
            }
            case CMP_TYPE_FLOAT:
            {
                type = msgpack_var_type::TYPE_NUMBER;
                vd = obj.as.flt;
                break;
            }
              
            case CMP_TYPE_DOUBLE:
            {
                type = msgpack_var_type::TYPE_NUMBER;
                vd = obj.as.dbl;
                break;                
            }
                
            case CMP_TYPE_UINT16:
            {
                type = msgpack_var_type::TYPE_NUMBER;
                vi = obj.as.u16;
                break;
            }
                
            case CMP_TYPE_UINT32:
            {
                type = msgpack_var_type::TYPE_NUMBER;
                vi = obj.as.u32;
                break;
            }
                
            case CMP_TYPE_POSITIVE_FIXNUM:
            case CMP_TYPE_UINT8:
            {
                type = msgpack_var_type::TYPE_NUMBER;
                vi = obj.as.u8;
                break;
            }
            
            default:
            {
                debug("Unrecognized object type %u\n", obj.type);
                break;
            }
        
        }
    } else
    {
        //debug("Node incomplete %s",cmp_strerror(ctx));
        return NULL;
    }
    sMsgPackNode* node = (sMsgPackNode*)malloc(sizeof(sMsgPackNode));
    dynamic_allocation_bytes += sizeof(sMsgPackNode);
    if(node == NULL){
        debug("Unable to create node in memory");
        return NULL;
    }
    
    str_size = strlen(nstr);
    node->string = (char*)malloc(str_size + 1);
    dynamic_allocation_bytes += str_size + 1;
    memcpy(node->string,nstr,str_size);    //copy null terminator as well
    node->string[str_size] = '\0';    
    node->type = type;
    
    if(node->type == msgpack_var_type::TYPE_BINARY){
        node->bin = (uint8_t *)malloc(binsz);
        dynamic_allocation_bytes += binsz;
        if(node->bin ==NULL){
            debug("Unable to allocate mem bin");
        }
        memcpy(node->bin,binarr,binsz);
        node->binsz = binsz;
        //debug("allocated node");
    }
    if(node->type == msgpack_var_type::TYPE_STRING){
        str_size = strlen(vstr);
        node->valuestring = (char *)malloc(str_size+1);
        dynamic_allocation_bytes += str_size + 1;
        if(node->valuestring == NULL){
            debug("Unable to allocate mem str");
        }
        memcpy(node->valuestring,vstr,str_size);
        node->valuestring[str_size] = '\0';
    }
    node->child = NULL;
    if(type == msgpack_var_type::TYPE_MAP){
        node->child = ch;
    }
    node->valueint = vi;
    node->valuedouble = vd;
    node->next = NULL;
    return node;
}

void msgPackListNodes(sMsgPackNode* root)
{
    sMsgPackNode* n = root;
    debug("list:");
    while(n)
    {
        if(n->child){
            debug("child:");
            msgPackListNodes(n->child);
        }
            
        debug("%s",n->string);
        n = n->next;
    }
}

void MsgPackDelete(sMsgPackNode* root)//rentrant
{
    sMsgPackNode *pv, *rp;
    
    rp = root;
    while(root->next){
        pv = rp->next;
        while(pv->next){
                rp = pv;
                pv = pv->next;
                if(pv->child){
                    MsgPackDelete(pv->child);
                    pv->child = NULL;
                    goto x;
                }
        }
        //debug("deleting:%s",pv->string);
        dynamic_allocation_bytes -= (strlen(pv->string) + 1);
        free(pv->string);
        
        if(pv->type == msgpack_var_type::TYPE_STRING){
            dynamic_allocation_bytes -= (strlen(pv->valuestring) + 1);
            free(pv->valuestring);
            
        }
        if(pv->type == msgpack_var_type::TYPE_BINARY){
            dynamic_allocation_bytes -= (pv->binsz);
            free(pv->bin);
        }
        if(pv->child){
            MsgPackDelete(pv->child);
        }
        free(pv);
        dynamic_allocation_bytes -= sizeof(sMsgPackNode);
        rp->next = NULL;
x:
        rp = root;
    }
    //debug("deleting:%s",root->string);
    dynamic_allocation_bytes -= (strlen(root->string) + 1);
    free(root->string);
    if(root->type == msgpack_var_type::TYPE_STRING){
        dynamic_allocation_bytes -= (strlen(root->valuestring) + 1);
        free(root->valuestring);
    }
    if(pv->type == msgpack_var_type::TYPE_BINARY){
        dynamic_allocation_bytes -= (root->binsz);
        free(root->bin);
    }
    free(root);
    dynamic_allocation_bytes -= sizeof(sMsgPackNode);
}

sMsgPackNode* msgPackParse(uint8_t* buf,uint32_t* len){ //reentrant
    
    sMsgPackNode *node = NULL;
    sMsgPackNode *root = NULL;
    
    sFile smsgpackb;
    
    cmp_ctx_t cmp;
    
    if (!(buf[0] > 0x80 && buf[0] < 0x8f)){
        return NULL;
    }
    
    uint32_t map_len;

    msgPackInitBuffer(&smsgpackb, buf, *len);
    
    cmp_init(&cmp,(void*)&smsgpackb, msgPackReadBuffer, msgPackWriteBuffer);
    
    
    if(cmp_read_map(&cmp, &map_len) == false)
    {
        return NULL;
    }
    
    //debug("Maplen %d", map_len);
    root = getnode(&cmp); //get node creates a node on the heap using malloc
    
    if( root == NULL)
    {
        //debug("Root Node missing");
        return NULL;
    }
    node = root;
    map_len--;
    while( map_len ){
        
        node->next = getnode(&cmp);
        
        if(node->next == NULL){
            //debug("Message pack contains partial information");
            MsgPackDelete(root);
            return NULL;            
        }        
        node = node->next;
        map_len--;
    }
    *len = (uint32_t)(smsgpackb.rp - smsgpackb.start); //update bytes read from buffer
    return root;
}


static void deserializePassthrough(sMsgPackNode* root, openxc_ControlCommand* command) {
    
    command->type = openxc_ControlCommand_Type_PASSTHROUGH;

    sMsgPackNode* element = msgPackSeekNode(root, "bus");
    
    if(element != NULL) {
        command->passthrough_mode_request.bus = element->valueint;
    }

    element = msgPackSeekNode(root, "enabled");
    if(element != NULL) {
        command->passthrough_mode_request.enabled = bool(element->valueint);
    }
}

static void deserializePayloadFormat(sMsgPackNode* root,
        openxc_ControlCommand* command) {
            
    command->type = openxc_ControlCommand_Type_PAYLOAD_FORMAT;

    sMsgPackNode* element = msgPackSeekNode(root, "format");
    if(element != NULL) {
        if(!strcmp(element->valuestring,
                    openxc::payload::messagepack::PAYLOAD_FORMAT_JSON_NAME)) {
            command->payload_format_command.format =
                    openxc_PayloadFormatCommand_PayloadFormat_JSON;
        } else if(!strcmp(element->valuestring,
                    openxc::payload::messagepack::PAYLOAD_FORMAT_PROTOBUF_NAME)) {
            command->payload_format_command.format =
                    openxc_PayloadFormatCommand_PayloadFormat_PROTOBUF;
        } else if(!strcmp(element->valuestring,
                    openxc::payload::messagepack::PAYLOAD_FORMAT_MESSAGEPACK_NAME)) {
            command->payload_format_command.format = openxc_PayloadFormatCommand_PayloadFormat_MESSAGEPACK;
        }
    }
}

static void deserializePredefinedObd2RequestsCommand(sMsgPackNode* root,
        openxc_ControlCommand* command) {
            
    command->type = openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS;

    sMsgPackNode* element = msgPackSeekNode(root, "enabled");
    if(element != NULL) {
        command->predefined_obd2_requests_command.enabled = bool(element->valueint);
    }
}

static void deserializeAfBypass(sMsgPackNode* root, openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS;

    sMsgPackNode* element = msgPackSeekNode(root, "bus");
    if(element != NULL) {
        command->acceptance_filter_bypass_command.bus = element->valueint;
    }

    element = msgPackSeekNode(root, "bypass");
    if(element != NULL) {
        command->acceptance_filter_bypass_command.bypass =
            bool(element->valueint);
    }
}

static void deserializeDiagnostic(sMsgPackNode* root, openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_DIAGNOSTIC;

    sMsgPackNode* action = msgPackSeekNode(root, "action");
    if(action != NULL && action->type == msgpack_var_type::TYPE_STRING) {
        if(!strcmp(action->valuestring, "add")) {
            command->diagnostic_request.action =
                    openxc_DiagnosticControlCommand_Action_ADD;
        } else if(!strcmp(action->valuestring, "cancel")) {
            command->diagnostic_request.action =
                    openxc_DiagnosticControlCommand_Action_CANCEL;
        }
    }

    sMsgPackNode* request = msgPackSeekNode(root, "request");
    if(request != NULL) {
     sMsgPackNode * element = msgPackSeekNode(request, "bus");
        if(element != NULL) {
            command->diagnostic_request.request.bus = element->valueint;
        }

        element = msgPackSeekNode(request, "mode");
        if(element != NULL) {
            command->diagnostic_request.request.mode = element->valueint;
        }

        element = msgPackSeekNode(request, "id");
        if(element != NULL) {
            command->diagnostic_request.request.message_id = element->valueint;
        }

        element = msgPackSeekNode(request, "pid");
        if(element != NULL) {
            command->diagnostic_request.request.pid = element->valueint;
        }

        element = msgPackSeekNode(request, "payload");
        if(element != NULL) {
            if(request->type == msgpack_var_type::TYPE_BINARY){
                command->diagnostic_request.request.payload.size = request->binsz;
                memcpy(command->diagnostic_request.request.payload.bytes, 
                        request->bin,request->binsz);
            }
        }

        element = msgPackSeekNode(request, "multiple_responses");
        if(element != NULL) {
            command->diagnostic_request.request.multiple_responses =
                bool(element->valueint);
        }

        element = msgPackSeekNode(request, "frequency");
        if(element != NULL) {
            command->diagnostic_request.request.frequency =
                element->valuedouble;
        }

        element = msgPackSeekNode(request, "decoded_type");
        if(element != NULL) {
            if(!strcmp(element->valuestring, "obd2")) {
                command->diagnostic_request.request.decoded_type =
                        openxc_DiagnosticRequest_DecodedType_OBD2;
            } else if(!strcmp(element->valuestring, "none")) {
                command->diagnostic_request.request.decoded_type =
                        openxc_DiagnosticRequest_DecodedType_NONE;
            }
        }

        element = msgPackSeekNode(request, "name");
        if(element != NULL && element->type == msgpack_var_type::TYPE_STRING) {
            strcpy(command->diagnostic_request.request.name,
                    element->valuestring);    
        }
    }
}

static bool deserializeDynamicField(sMsgPackNode* element,
        openxc_DynamicField* field) {
    bool status = true;
    switch(element->type) {
        case msgpack_var_type::TYPE_STRING:
            field->type = openxc_DynamicField_Type_STRING;
            strcpy(field->string_value, element->valuestring);
            break;
        case msgpack_var_type::TYPE_FALSE:
        case msgpack_var_type::TYPE_TRUE:
            field->type = openxc_DynamicField_Type_BOOL;
            field->boolean_value = bool(element->valueint);
            break;
        case msgpack_var_type::TYPE_NUMBER:
            field->type = openxc_DynamicField_Type_NUM;
            field->numeric_value = element->valuedouble;
            break;
        default:
            debug("Unsupported type in value field: %d", element->type);
            status = false;
            break;
    }
    return status;
}

static void deserializeSimple(sMsgPackNode* root, openxc_VehicleMessage* message) {
    message->type = openxc_VehicleMessage_Type_SIMPLE;
    openxc_SimpleMessage* simpleMessage = &message->simple_message;

    sMsgPackNode* element = msgPackSeekNode(root, "name");
    if(element != NULL && element->type == msgpack_var_type::TYPE_STRING) {
        strcpy(simpleMessage->name, element->valuestring);
    }

    element = msgPackSeekNode(root, "value");
    if(element != NULL) {
        if(deserializeDynamicField(element, &simpleMessage->value)) {
        }
    }

    element = msgPackSeekNode(root, "event");
    if(element != NULL) {
        if(deserializeDynamicField(element, &simpleMessage->event)) {
        }
    }
}

static void deserializeCan(sMsgPackNode* root, openxc_VehicleMessage* message) {
    message->type = openxc_VehicleMessage_Type_CAN;
    openxc_CanMessage* canMessage = &message->can_message;

    sMsgPackNode* element = msgPackSeekNode(root, "id");
    if(element != NULL) {
        canMessage->id = element->valueint;
        element = msgPackSeekNode(root, "data");
        if(element != NULL) { 
            if(element->type == msgpack_var_type::TYPE_BINARY){
                 canMessage->data.size = element->binsz;
                 memcpy(canMessage->data.bytes,
                        element->bin,element->binsz);
            }
        }

        element = msgPackSeekNode(root, "bus");
        if(element != NULL) {
            canMessage->bus = element->valueint;
        }

        element = msgPackSeekNode(root, payload::messagepack::FRAME_FORMAT_FIELD_NAME);
        if(element != NULL) {
            if(!strcmp(element->valuestring,
                        payload::messagepack::FRAME_FORMAT_STANDARD_NAME)) {
                canMessage->frame_format = openxc_CanMessage_FrameFormat_STANDARD;
            } else if(!strcmp(element->valuestring,
                        payload::messagepack::FRAME_FORMAT_EXTENDED_NAME)) {
                canMessage->frame_format = openxc_CanMessage_FrameFormat_EXTENDED;
            }
        }
    }
}

static void deserializeModemConfiguration(sMsgPackNode* root, openxc_ControlCommand* command) {
    // set up the struct for a modem configuration message
    command->type = openxc_ControlCommand_Type_MODEM_CONFIGURATION;
    openxc_ModemConfigurationCommand* modemConfigurationCommand = &command->modem_configuration_command;
    
    // parse server command
    sMsgPackNode* server = msgPackSeekNode(root, "server");
    if(server != NULL) {
        sMsgPackNode* host = msgPackSeekNode(server, "host");
        if(host != NULL) {
            strcpy(modemConfigurationCommand->serverConnectSettings.host, host->valuestring);
        }
        sMsgPackNode* port = msgPackSeekNode(server, "port");
        if(port != NULL) {
            modemConfigurationCommand->serverConnectSettings.port = port->valueint;
        }
    }
}

static void deserializeRTCConfiguration(sMsgPackNode* root, openxc_ControlCommand* command) {

    command->type = openxc_ControlCommand_Type_RTC_CONFIGURATION;
    openxc_RTCConfigurationCommand* rtcConfigurationCommand = &command->rtc_configuration_command;
    
    sMsgPackNode* time = msgPackSeekNode(root, "time");
    
    if(time != NULL) {
        rtcConfigurationCommand->unix_time = time->valueint;
    }
}




//Entire data is chunked into a single packet by higher level protocol
//unable to decode partial messages at this moment correctly
size_t openxc::payload::messagepack::deserialize(uint8_t payload[], size_t length,
        openxc_VehicleMessage* message){
    
    uint32_t Messagelen=0;
    uint32_t i = 0;
    sMsgPackNode *root;
    //debug("Deserialize %d bytes",length);
    root = NULL;
    //find the start of message by searching for FIXMAPMARKER
    while(i < length){
        if(payload[i] > 0x80 && payload[i] < 0x8f){//attempt to parse message if found
            uint32_t len = length-i;
            root = msgPackParse(&payload[i],&len);
            if( root != NULL)//we found a message
            {
                //debug("Message Pack Data Complete %d bytes", len);
                //msgPackListNodes(root);
                Messagelen = i + len; //message len to discard on success, discarding previous data
                break;
            }
        }
        i++;
    }
    if(root == NULL)
    {
        //debug("MessagePackMemUsed:%d bytes", dynamic_allocation_bytes);
        return 0;
    }
    sMsgPackNode* commandNameObject = msgPackSeekNode(root, "command");

    if(commandNameObject != NULL) {
        
        message->type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
        openxc_ControlCommand* command = &message->control_command;
        
        if(!strncmp(commandNameObject->valuestring, VERSION_COMMAND_NAME,
                    strlen(VERSION_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_VERSION;
        } else if(!strncmp(commandNameObject->valuestring,
                    DEVICE_ID_COMMAND_NAME, strlen(DEVICE_ID_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_DEVICE_ID;
        } else if(!strncmp(commandNameObject->valuestring,
                    DEVICE_PLATFORM_COMMAND_NAME, strlen(DEVICE_PLATFORM_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_PLATFORM;
        } else if(!strncmp(commandNameObject->valuestring,
                    DIAGNOSTIC_COMMAND_NAME, strlen(DIAGNOSTIC_COMMAND_NAME))) {
            deserializeDiagnostic(root, command);
        } else if(!strncmp(commandNameObject->valuestring,
                    PASSTHROUGH_COMMAND_NAME, strlen(PASSTHROUGH_COMMAND_NAME))) {
            deserializePassthrough(root, command);
        } else if(!strncmp(commandNameObject->valuestring,
                    PREDEFINED_OBD2_REQUESTS_COMMAND_NAME,
                        strlen(PREDEFINED_OBD2_REQUESTS_COMMAND_NAME))) {
            deserializePredefinedObd2RequestsCommand(root, command);
        } else if(!strncmp(commandNameObject->valuestring,
                    ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME,
                    strlen(ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME))) {
            deserializeAfBypass(root, command);
        } else if(!strncmp(commandNameObject->valuestring,
                    PAYLOAD_FORMAT_COMMAND_NAME,
                    strlen(PAYLOAD_FORMAT_COMMAND_NAME))) {
            deserializePayloadFormat(root, command);
        } 
        else if(!strncmp(commandNameObject->valuestring,
                    MODEM_CONFIGURATION_COMMAND_NAME,
                    strlen(MODEM_CONFIGURATION_COMMAND_NAME))) {
            deserializeModemConfiguration(root, command);
        }
        else if(!strncmp(commandNameObject->valuestring,
                    RTC_CONFIGURATION_COMMAND_NAME,
                    strlen(RTC_CONFIGURATION_COMMAND_NAME))) {
            deserializeRTCConfiguration(root, command);
        }
        else if(!strncmp(commandNameObject->valuestring,
                    SD_MOUNT_STATUS_COMMAND_NAME,
                    strlen(SD_MOUNT_STATUS_COMMAND_NAME))) {
                command->type = openxc_ControlCommand_Type_SD_MOUNT_STATUS;
        }
        else {
            debug("Unrecognized command: %s", commandNameObject->valuestring);
        }    
    } else{
        sMsgPackNode* nameObject = msgPackSeekNode(root, "name");
        if(nameObject == NULL) {
            deserializeCan(root, message);
        } else {
            deserializeSimple(root, message);
        }
    }
    
    MsgPackDelete(root);
    Messagelen = MIN(Messagelen,length);
    //debug("Parsed: %d bytes", Messagelen);
    //debug("MessagePackMemUsed:%d bytes", dynamic_allocation_bytes);    
    return Messagelen;        
}
        
