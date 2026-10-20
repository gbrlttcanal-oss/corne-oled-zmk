/* Minimal deterministic host adapters. Tests execute the production runtime.c. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define CONFIG_ZMK_RGB_UNDERGLOW 0
#define CONFIG_ZMK_SPLIT 0
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 0
#define IS_ENABLED(x) (x)
#define K_MUTEX_DEFINE(x) int x
#define K_FOREVER -1
#define K_NO_WAIT 0
#define K_MSEC(x) (x)
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CONTAINER_OF(p,t,m) ((t *)((char *)(p)-offsetof(t,m)))
#define DEVICE_DT_NAME(x) "kp"
#define DT_NODELABEL(x) x
#define SYS_INIT(...)
#define SETTINGS_STATIC_HANDLER_DEFINE(...)
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)
#define ZMK_EV_EVENT_BUBBLE 0
#define snprintk snprintf
static int64_t now;
struct k_work { void (*handler)(struct k_work *); };
struct k_work_delayable { struct k_work work; bool pending; int64_t due; };
#define K_WORK_DELAYABLE_DEFINE(n,f) struct k_work_delayable n={.work={.handler=f}}
static int k_mutex_lock(int *m,int timeout){(void)m;(void)timeout;return 0;}
static int k_mutex_unlock(int *m){(void)m;return 0;}
static int64_t k_uptime_get(void){return now;}
static void k_work_init_delayable(struct k_work_delayable *w,void (*f)(struct k_work *)){*w=(struct k_work_delayable){.work={.handler=f}};}
static int k_work_reschedule(struct k_work_delayable *w,int64_t delay){w->pending=true;w->due=now+delay;return 0;}
static int k_work_cancel_delayable(struct k_work_delayable *w){w->pending=false;return 0;}
static struct k_work_delayable *k_work_delayable_from_work(struct k_work *w){return CONTAINER_OF(w,struct k_work_delayable,work);}
struct zmk_behavior_binding {const char *behavior_dev;uint32_t param1,param2;};
struct zmk_behavior_binding_event {uint32_t position;int64_t timestamp;};
struct zmk_position_state_changed {uint32_t position;bool state;};
typedef struct zmk_position_state_changed zmk_event_t;
static const struct zmk_position_state_changed *as_zmk_position_state_changed(const zmk_event_t *e){return e;}
static struct {uint32_t code;bool down;int64_t at;} events[1024];
static unsigned event_count;
static bool fail_release;
static int zmk_behavior_invoke_binding(struct zmk_behavior_binding *b,struct zmk_behavior_binding_event e,bool down){
    if(!down&&fail_release){fail_release=false;return -5;}
    assert(event_count<1024);events[event_count].code=b->param1;events[event_count].down=down;events[event_count++].at=e.timestamp;return 0;
}
typedef int (*settings_read_cb)(void *,void *,size_t);
static uint8_t flash[3][8][1400];static size_t flash_len[3][8];static bool fail_flash;
static int settings_save_one(const char *key,const void *data,size_t len){unsigned k,s;assert(sscanf(key,"gbrtzt/%u/%u",&k,&s)==2);if(fail_flash)return -5;memcpy(flash[k-1][s],data,len);flash_len[k-1][s]=len;return 0;}
static int settings_delete(const char *key){unsigned k,s;assert(sscanf(key,"gbrtzt/%u/%u",&k,&s)==2);flash_len[k-1][s]=0;return 0;}
static uint32_t crc32_ieee(const uint8_t *data,size_t len){uint32_t crc=~0u;for(size_t i=0;i<len;i++){crc^=data[i];for(int b=0;b<8;b++)crc=(crc>>1)^(0xedb88320u&-(crc&1));}return ~crc;}
