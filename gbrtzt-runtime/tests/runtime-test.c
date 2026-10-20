/* SPDX-License-Identifier: MIT */
#include "../src/runtime.c"
static void advance(int64_t target){
    for(unsigned guard=0;guard<2048;guard++){
        struct k_work_delayable *next=NULL,*all[]={&macro_work,&rgb_work,&active_dances[0].timer,&active_dances[1].timer,&active_dances[2].timer,&active_dances[3].timer};
        for(unsigned i=0;i<6;i++)if(all[i]->pending&&all[i]->due<=target&&(!next||all[i]->due<next->due))next=all[i];
        if(!next){now=target;return;}now=next->due;next->pending=false;next->work.handler(&next->work);
    }assert(!"timer loop");
}
static int read_flash(void *arg,void *out,size_t len){memcpy(out,arg,len);return len;}
static void step(uint8_t *m,unsigned i,unsigned type,uint32_t code,unsigned ms){uint8_t *s=m+292+i*8;s[0]=type;gbr_w16(s+2,ms);gbr_w32(s+4,code);}
static void expect(unsigned i,uint32_t code,bool down){assert(i<event_count);assert(events[i].code==code&&events[i].down==down);}
int main(void){
    runtime_init();uint8_t m[GBR_MACRO_SIZE]={0},out[GBR_MACRO_SIZE],d[GBR_DANCE_SIZE]={0};uint32_t rev;
    struct zmk_behavior_binding_event e={.position=2};
    /* Save, optimistic concurrency, failed flash, and boot-time restoration. */
    strcpy((char *)m,"Ctrl+C");gbr_w16(m+288,3);step(m,0,1,0x700e0,5);step(m,1,0,0x70006,20);step(m,2,2,0x700e0,0);
    assert(gbr_put(GBR_MACRO,0,0,m,sizeof(m),&rev)==0&&rev==1);
    assert(gbr_put(GBR_MACRO,0,0,m,sizeof(m),&rev)==-ESTALE);
    fail_flash=true;assert(gbr_put(GBR_MACRO,0,1,m,sizeof(m),&rev)==-5&&rev==1);fail_flash=false;
    memset(macros,0,sizeof(macros));memset(revisions,0,sizeof(revisions));
    assert(settings_set("1/0",flash_len[0][0],read_flash,flash[0][0])==0);
    assert(gbr_get(GBR_MACRO,0,out,&rev)==0&&rev==1&&!memcmp(m,out,sizeof(m)));
    flash[0][0][20]^=1;assert(settings_set("1/0",flash_len[0][0],read_flash,flash[0][0])==-EINVAL);flash[0][0][20]^=1;
    /* Playback keeps its snapshot if the slot changes, and releases every key. */
    assert(gbr_macro_start(0,e)==0);assert(gbr_macro_start(0,e)==-EBUSY);
    step(m,1,0,0x70019,20);assert(gbr_put(GBR_MACRO,0,1,m,sizeof(m),&rev)==0);
    advance(100);assert(event_count==4&&!player.active);expect(0,0x700e0,true);expect(1,0x70006,true);expect(2,0x70006,false);expect(3,0x700e0,false);assert(events[2].at-events[1].at==20);
    /* A failed release is retried during cleanup, avoiding a held modifier. */
    event_count=0;assert(gbr_macro_start(0,e)==0);advance(125);fail_release=true;advance(200);assert(!player.active&&player.held_count==0);expect(event_count-1,0x700e0,false);
    /* Single tap waits; a second tap chooses the second action. */
    gbr_w16(d+32,250);d[34]=2;gbr_w32(d+40,0x70029);gbr_w32(d+48,0x7002b);
    assert(gbr_put(GBR_DANCE,0,0,d,sizeof(d),&rev)==0);event_count=0;
    assert(gbr_dance_event(0,e,true)==0);advance(220);gbr_dance_event(0,e,false);advance(449);assert(event_count==0);advance(450);assert(event_count==2);expect(0,0x70029,true);expect(1,0x70029,false);
    event_count=0;gbr_dance_event(0,e,true);gbr_dance_event(0,e,false);advance(500);gbr_dance_event(0,e,true);assert(event_count==1);expect(0,0x7002b,true);gbr_dance_event(0,e,false);assert(event_count==2);expect(1,0x7002b,false);
    /* Held dance stays held until physical release; interrupted dance resolves. */
    event_count=0;gbr_dance_event(0,e,true);advance(750);assert(event_count==1);gbr_dance_event(0,e,false);assert(event_count==2);
    event_count=0;gbr_dance_event(0,e,true);gbr_dance_event(0,e,false);struct zmk_position_state_changed other={.position=9,.state=true};position_listener(&other);assert(event_count==2);
    /* Editing an active dance does not alter its pending action. */
    event_count=0;gbr_dance_event(0,e,true);gbr_w32(d+40,0x70004);assert(gbr_put(GBR_DANCE,0,1,d,sizeof(d),&rev)==0);gbr_dance_event(0,e,false);advance(1000);expect(0,0x70029,true);
    /* Macro action references the saved slot and does not nest dances. */
    d[34]=1;d[36]=1;gbr_w32(d+40,0);assert(gbr_put(GBR_DANCE,0,2,d,sizeof(d),&rev)==0);event_count=0;gbr_dance_event(0,e,true);gbr_dance_event(0,e,false);advance(1100);assert(event_count==4);expect(1,0x70019,true);
    assert(gbr_reset()==0);assert(gbr_get(GBR_MACRO,0,out,&rev)==0&&rev==0&&gbr_u16(out+288)==0);assert(flash_len[0][0]==0);
    puts("runtime: persistence, snapshots, key releases, tap timing and macro actions OK");
}
