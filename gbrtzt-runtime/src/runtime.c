/* SPDX-License-Identifier: MIT */
#include <gbrtzt/runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <dt-bindings/zmk/rgb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

K_MUTEX_DEFINE(runtime_lock);
static uint8_t macros[GBR_SLOTS][GBR_MACRO_SIZE], dances[GBR_SLOTS][GBR_DANCE_SIZE];
static uint8_t rgb[GBR_RGB_SIZE]={0,0,0,100,25};
static uint32_t revisions[3][GBR_SLOTS];
/* Only RPC/settings thread uses these buffers, under runtime_lock. */
static uint8_t blob[16+GBR_MAX_SIZE];
static uint8_t *record(uint8_t kind,uint8_t slot) {
    if(slot>=GBR_SLOTS)return NULL;
    return kind==GBR_MACRO?macros[slot]:kind==GBR_DANCE?dances[slot]:kind==GBR_RGB&&slot==0?rgb:NULL;
}
bool gbr_rgb_supported(void) { return IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW); }
static int key_event(uint32_t code,struct zmk_behavior_binding_event event,bool down) {
    struct zmk_behavior_binding b={.behavior_dev=DEVICE_DT_NAME(DT_NODELABEL(kp)),.param1=code};
    event.timestamp=k_uptime_get();return zmk_behavior_invoke_binding(&b,event,down);
}
static int apply_rgb(const uint8_t *data) {
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
    struct zmk_behavior_binding_event e={.position=0,.timestamp=k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source=ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };
    struct zmk_behavior_binding b={.behavior_dev=DEVICE_DT_NAME(DT_NODELABEL(rgb_ug)),.param1=RGB_COLOR_HSB_CMD,.param2=RGB_COLOR_HSB_VAL(gbr_u16(data+1),data[3],data[4])};
    int rc=zmk_behavior_invoke_binding(&b,e,true);if(rc<0)return rc;
    b.param1=RGB_EFS_CMD;b.param2=0;rc=zmk_behavior_invoke_binding(&b,e,true);if(rc<0)return rc;
    b.param1=data[0]?RGB_ON_CMD:RGB_OFF_CMD;return zmk_behavior_invoke_binding(&b,e,true);
#else
    return -ENOTSUP;
#endif
}
static void rgb_sync(struct k_work *work) { k_mutex_lock(&runtime_lock,K_FOREVER);if(revisions[GBR_RGB-1][0])apply_rgb(rgb);k_mutex_unlock(&runtime_lock); }
K_WORK_DELAYABLE_DEFINE(rgb_work,rgb_sync);
int gbr_get(uint8_t kind,uint8_t slot,uint8_t *out,uint32_t *rev) {
    uint8_t *src=record(kind,slot);if(!src)return -EINVAL;
    k_mutex_lock(&runtime_lock,K_FOREVER);memcpy(out,src,gbr_size(kind));*rev=revisions[kind-1][slot];k_mutex_unlock(&runtime_lock);return 0;
}
int gbr_put(uint8_t kind,uint8_t slot,uint32_t expected,const uint8_t *data,size_t len,uint32_t *rev) {
    uint8_t *dst=record(kind,slot);if(!dst)return -EINVAL;
    int rc=gbr_validate(kind,data,len);if(rc)return rc;
    if(kind==GBR_RGB&&!gbr_rgb_supported())return -ENOTSUP;
    k_mutex_lock(&runtime_lock,K_FOREVER);
    if(expected!=revisions[kind-1][slot]){rc=-ESTALE;goto done;}
    if(expected==UINT32_MAX){rc=-EOVERFLOW;goto done;}
    memcpy(blob,"GRT1",4);blob[4]=GBR_VERSION;blob[5]=kind;blob[6]=slot;blob[7]=0;
    gbr_w32(blob+8,expected+1);gbr_w32(blob+12,crc32_ieee(data,len));memcpy(blob+16,data,len);
    char key[24];snprintk(key,sizeof(key),"gbrtzt/%u/%u",kind,slot);
    rc=settings_save_one(key,blob,len+16);if(rc<0)goto done;
    memcpy(dst,data,len);revisions[kind-1][slot]=expected+1;
    if(kind==GBR_RGB)rc=apply_rgb(data);
done: *rev=revisions[kind-1][slot];k_mutex_unlock(&runtime_lock);return rc;
}
static int settings_set(const char *name,size_t len,settings_read_cb read_cb,void *arg) {
    if(!name||strlen(name)!=3||name[1]!='/'||name[0]<'1'||name[0]>'3'||name[2]<'0'||name[2]>'7')return -ENOENT;
    uint8_t kind=name[0]-'0',slot=name[2]-'0',*dst=record(kind,slot);size_t size=gbr_size(kind);
    if(!dst||len!=size+16)return -EINVAL;
    k_mutex_lock(&runtime_lock,K_FOREVER);int rc=read_cb(arg,blob,len);
    if(rc!=(int)len||memcmp(blob,"GRT1",4)||blob[4]!=GBR_VERSION||blob[5]!=kind||blob[6]!=slot||blob[7]||gbr_u32(blob+12)!=crc32_ieee(blob+16,size)||gbr_validate(kind,blob+16,size)){rc=-EINVAL;goto done;}
    memcpy(dst,blob+16,size);revisions[kind-1][slot]=gbr_u32(blob+8);rc=0;
done:k_mutex_unlock(&runtime_lock);return rc;
}
static int gbr_settings_commit(void) { if(gbr_rgb_supported())k_work_reschedule(&rgb_work,K_MSEC(300));return 0; }
SETTINGS_STATIC_HANDLER_DEFINE(gbrtzt,"gbrtzt",NULL,settings_set,gbr_settings_commit,NULL);

/* One macro at a time. Playback snapshots the slot; editing it cannot change a
 * running sequence. No bulk behavior queue: long text cannot overflow it. */
static struct { bool active,tap_up; uint16_t index,count; uint8_t data[GBR_MACRO_SIZE];uint32_t held[17];unsigned held_count;struct zmk_behavior_binding_event event; } player;
static void player_release(void) { for(unsigned i=0;i<player.held_count;i++)key_event(player.held[i],player.event,false);player.held_count=0;player.active=false;player.tap_up=false; }
static void macro_tick(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(macro_work,macro_tick);
int gbr_macro_start(uint8_t slot,struct zmk_behavior_binding_event event) {
    if(slot>=GBR_SLOTS)return -EINVAL;k_mutex_lock(&runtime_lock,K_FOREVER);
    if(player.active){k_mutex_unlock(&runtime_lock);return -EBUSY;}
    memcpy(player.data,macros[slot],GBR_MACRO_SIZE);player.count=gbr_u16(player.data+288);player.index=0;player.tap_up=false;player.held_count=0;player.event=event;player.active=player.count>0;
    if(player.active)k_work_reschedule(&macro_work,K_NO_WAIT);k_mutex_unlock(&runtime_lock);return 0;
}
static void macro_tick(struct k_work *work) {
    k_mutex_lock(&runtime_lock,K_FOREVER);if(!player.active)goto done;
    if(player.index>=player.count){player_release();goto done;}
    const uint8_t *s=player.data+292+player.index*8;unsigned type=s[0],ms=gbr_u16(s+2);uint32_t code=gbr_u32(s+4);int rc=0;
    if(player.tap_up){rc=key_event(code,player.event,false);if(rc>=0&&player.held_count)player.held_count--;
        player.tap_up=false;player.index++;ms=10;
    } else if(type==0){rc=key_event(code,player.event,true);if(rc>=0){player.held[player.held_count++]=code;player.tap_up=true;}ms=MAX(ms,10);}
    else {if(type==1){rc=key_event(code,player.event,true);if(rc>=0)player.held[player.held_count++]=code;}
        else if(type==2){rc=key_event(code,player.event,false);if(rc>=0)for(unsigned i=0;i<player.held_count;i++)if(player.held[i]==code){player.held[i]=player.held[--player.held_count];break;}}
        player.index++;}
    if(rc<0){player_release();goto done;}k_work_reschedule(&macro_work,K_MSEC(MAX(ms,1)));
done:k_mutex_unlock(&runtime_lock);
}

struct dance_state { bool active,pressed,decided;uint8_t count,data[GBR_DANCE_SIZE];int64_t deadline;struct zmk_behavior_binding_event event;struct k_work_delayable timer; };
static struct dance_state active_dances[4];
static void dance_action(struct dance_state *d,bool pressed) {const uint8_t *a=d->data+36+(d->count-1)*8;if(a[0]==0)key_event(gbr_u32(a+4),d->event,pressed);else if(pressed)gbr_macro_start(gbr_u16(a+2),d->event);}
static void decide(struct dance_state *d) {if(!d->active||d->decided)return;d->decided=true;dance_action(d,true);if(!d->pressed){dance_action(d,false);d->active=false;}}
static void dance_timer(struct k_work *work) {struct dance_state *d=CONTAINER_OF(k_work_delayable_from_work(work),struct dance_state,timer);k_mutex_lock(&runtime_lock,K_FOREVER);if(d->active&&!d->decided){int64_t left=d->deadline-k_uptime_get();if(left>0)k_work_reschedule(&d->timer,K_MSEC(left));else decide(d);}k_mutex_unlock(&runtime_lock);}
int gbr_dance_event(uint8_t slot,struct zmk_behavior_binding_event event,bool pressed) {
    if(slot>=GBR_SLOTS)return -EINVAL;k_mutex_lock(&runtime_lock,K_FOREVER);struct dance_state *d=NULL;
    for(unsigned i=0;i<4;i++)if(active_dances[i].active&&active_dances[i].event.position==event.position){d=&active_dances[i];break;}
    if(!d&&pressed){for(unsigned i=0;i<4;i++)if(!active_dances[i].active){d=&active_dances[i];break;}if(d&&dances[slot][34]){memcpy(d->data,dances[slot],GBR_DANCE_SIZE);d->active=true;d->count=0;d->decided=false;d->event=event;}else d=NULL;}
    if(!d){k_mutex_unlock(&runtime_lock);return 0;}
    d->pressed=pressed;
    if(pressed&&!d->decided){d->count++;d->deadline=k_uptime_get()+gbr_u16(d->data+32);if(d->count>=d->data[34])decide(d);else k_work_reschedule(&d->timer,K_MSEC(gbr_u16(d->data+32)));}
    else if(!pressed&&d->decided){dance_action(d,false);d->active=false;k_work_cancel_delayable(&d->timer);}
    k_mutex_unlock(&runtime_lock);return 0;
}
static int position_listener(const zmk_event_t *eh) {const struct zmk_position_state_changed *e=as_zmk_position_state_changed(eh);if(e&&e->state){k_mutex_lock(&runtime_lock,K_FOREVER);for(unsigned i=0;i<4;i++)if(active_dances[i].active&&active_dances[i].event.position!=e->position)decide(&active_dances[i]);k_mutex_unlock(&runtime_lock);}return ZMK_EV_EVENT_BUBBLE;}
ZMK_LISTENER(gbrtzt_dance,position_listener);
ZMK_SUBSCRIPTION(gbrtzt_dance,zmk_position_state_changed);
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) && IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
static int split_listener(const zmk_event_t *eh) {const struct zmk_split_peripheral_status_changed *e=as_zmk_split_peripheral_status_changed(eh);if(e&&e->connected)k_work_reschedule(&rgb_work,K_MSEC(500));return ZMK_EV_EVENT_BUBBLE;}
ZMK_LISTENER(gbrtzt_rgb,split_listener);
ZMK_SUBSCRIPTION(gbrtzt_rgb,zmk_split_peripheral_status_changed);
#endif
int gbr_reset(void) {
    k_mutex_lock(&runtime_lock,K_FOREVER);player_release();k_work_cancel_delayable(&macro_work);for(unsigned i=0;i<4;i++){if(active_dances[i].active&&active_dances[i].decided&&active_dances[i].pressed)dance_action(&active_dances[i],false);active_dances[i].active=false;k_work_cancel_delayable(&active_dances[i].timer);}
    int result=0;for(uint8_t kind=1;kind<=3;kind++)for(uint8_t slot=0;slot<(kind==3?1:GBR_SLOTS);slot++){char key[24];snprintk(key,sizeof(key),"gbrtzt/%u/%u",kind,slot);int rc=settings_delete(key);if(rc<0){result=rc;continue;}memset(record(kind,slot),0,gbr_size(kind));revisions[kind-1][slot]=0;if(kind==GBR_DANCE)gbr_w16(dances[slot]+32,250);}
    k_mutex_unlock(&runtime_lock);return result;
}
static int runtime_init(void) {for(unsigned i=0;i<GBR_SLOTS;i++)gbr_w16(dances[i]+32,250);for(unsigned i=0;i<4;i++)k_work_init_delayable(&active_dances[i].timer,dance_timer);return 0;}
SYS_INIT(runtime_init,APPLICATION,80);
