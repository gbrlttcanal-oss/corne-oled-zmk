/* SPDX-License-Identifier: MIT */
#include <gbrtzt/format.h>
#include <errno.h>
#include <string.h>
uint16_t gbr_u16(const uint8_t *p) { return p[0] | (uint16_t)p[1] << 8; }
uint32_t gbr_u32(const uint8_t *p) { return gbr_u16(p) | (uint32_t)gbr_u16(p+2) << 16; }
void gbr_w16(uint8_t *p, uint16_t v) { p[0]=v; p[1]=v>>8; }
void gbr_w32(uint8_t *p, uint32_t v) { gbr_w16(p,v); gbr_w16(p+2,v>>16); }
size_t gbr_size(uint8_t kind) { return kind==GBR_MACRO?GBR_MACRO_SIZE:kind==GBR_DANCE?GBR_DANCE_SIZE:kind==GBR_RGB?GBR_RGB_SIZE:0; }
int gbr_hid_valid(uint32_t code) { unsigned page=(code>>16)&255, id=code&65535; return id>0 && ((page==7 && id<=231) || (page==12 && id<=4095)); }
int gbr_validate(uint8_t kind, const uint8_t *d, size_t len) {
    if (!d || !gbr_size(kind) || len!=gbr_size(kind)) return -EINVAL;
    if (kind==GBR_RGB) return (d[0]<=1 && gbr_u16(d+1)<360 && d[3]<=100 && d[4]<=100)?0:-EINVAL;
    if (!memchr(d,0,32)) return -EINVAL;
    if (kind==GBR_DANCE) {
        if (gbr_u16(d+32)<100 || gbr_u16(d+32)>1000 || d[34]>4 || d[35]) return -EINVAL;
        for (unsigned i=0;i<d[34];i++) { const uint8_t *a=d+36+i*8;
            if (a[1] || (a[0]!=0 && a[0]!=1)) return -EINVAL;
            if (a[0]==0 ? (!gbr_hid_valid(gbr_u32(a+4)) || gbr_u16(a+2)) : (gbr_u16(a+2)>=GBR_SLOTS || gbr_u32(a+4))) return -EINVAL;
        }
        return 0;
    }
    unsigned count=gbr_u16(d+288), held_count=0, duration=0; uint32_t held[16];
    if (count>GBR_STEPS || d[290]>1 || d[291]>1 || !memchr(d+32,0,256)) return -EINVAL;
    for(unsigned i=0;i<count;i++) { const uint8_t *s=d+292+i*8; unsigned type=s[0], ms=gbr_u16(s+2); uint32_t code=gbr_u32(s+4);
        if (s[1] || type>3 || ms>5000 || (type==3 ? code!=0 : !gbr_hid_valid(code))) return -EINVAL;
        duration+=ms+(type==0?10:0); if(duration>30000)return -EINVAL;
        unsigned j=0;while(j<held_count && held[j]!=code)j++;
        if(type==0 && j<held_count)return -EINVAL;
        if(type==1) { if(j<held_count || held_count==16)return -EINVAL; held[held_count++]=code; }
        if(type==2) { if(j==held_count)return -EINVAL;held[j]=held[--held_count]; }
    }
    return held_count?-EINVAL:0;
}
