/* SPDX-License-Identifier: MIT */
#include <gbrtzt/runtime.h>
#include <zmk/studio/custom.h>
#include <pb_encode.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <string.h>
LOG_MODULE_DECLARE(zmk,CONFIG_ZMK_LOG_LEVEL);
static uint8_t response[10+GBR_MAX_SIZE];static size_t response_size;
static bool encode(pb_ostream_t *stream,const pb_field_t *field,void *const *arg){return pb_encode_tag_for_field(stream,field)&&pb_encode_string(stream,response,response_size);}
static struct zmk_rpc_custom_subsystem_meta meta={.security=ZMK_STUDIO_RPC_HANDLER_SECURED};
static bool handle(const zmk_custom_CallRequest *req,pb_callback_t *out){
    const uint8_t *p=req->payload.bytes;size_t len=req->payload.size;memset(response,0,sizeof(response));response_size=10;response[0]=GBR_VERSION;
    int rc=-EINVAL;uint32_t rev=0;
    if(len<8||p[0]!=GBR_VERSION)goto done;
    uint8_t op=p[1],kind=p[2],slot=p[3];response[1]=op;response[2]=kind;response[3]=slot;
    if(op==0&&len==8){uint8_t *c=response+10;c[0]=GBR_SLOTS;c[1]=GBR_SLOTS;c[2]=4;c[3]=gbr_rgb_supported();gbr_w16(c+4,GBR_STEPS);gbr_w16(c+6,GBR_MACRO_SIZE);gbr_w16(c+8,GBR_DANCE_SIZE);gbr_w16(c+10,zmk_behavior_get_local_id(DEVICE_DT_NAME(DT_NODELABEL(gbr_macro))));gbr_w16(c+12,zmk_behavior_get_local_id(DEVICE_DT_NAME(DT_NODELABEL(gbr_dance))));response_size+=14;rc=0;}
    else if(op==1&&len==8){rc=gbr_get(kind,slot,response+10,&rev);if(!rc)response_size+=gbr_size(kind);}
    else if(op==2){rc=gbr_put(kind,slot,gbr_u32(p+4),p+8,len-8,&rev);}
done:gbr_w16(response+4,rc<0?-rc:0);gbr_w32(response+6,rev);out->funcs.encode=encode;out->arg=NULL;return true;
}
ZMK_RPC_CUSTOM_SUBSYSTEM(gbrtzt__runtime_v1,&meta,handle);
ZMK_RPC_SUBSYSTEM_SETTINGS_RESET(gbrtzt_runtime,gbr_reset);
