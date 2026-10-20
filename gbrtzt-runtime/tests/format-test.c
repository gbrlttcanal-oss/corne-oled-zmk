#include <gbrtzt/format.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
    uint8_t m[GBR_MACRO_SIZE]={0};assert(!gbr_validate(1,m,sizeof m));assert(gbr_validate(1,m,sizeof m-1));
    gbr_w16(m+288,1);gbr_w16(m+294,10);gbr_w32(m+296,0x83070004);assert(!gbr_validate(1,m,sizeof m));
    m[292]=1;assert(gbr_validate(1,m,sizeof m));gbr_w16(m+288,2);m[300]=2;gbr_w32(m+304,0x83070004);assert(!gbr_validate(1,m,sizeof m));
    m[300]=1;assert(gbr_validate(1,m,sizeof m));m[300]=2;gbr_w32(m+304,0x70005);assert(gbr_validate(1,m,sizeof m));
    memset(m,0,sizeof m);gbr_w16(m+288,129);assert(gbr_validate(1,m,sizeof m));gbr_w16(m+288,7);for(int i=0;i<7;i++){m[292+i*8]=3;gbr_w16(m+294+i*8,5000);}assert(gbr_validate(1,m,sizeof m));gbr_w16(m+288,6);assert(!gbr_validate(1,m,sizeof m));
    uint8_t d[GBR_DANCE_SIZE]={0};gbr_w16(d+32,250);d[34]=2;gbr_w32(d+40,0x70029);d[44]=1;gbr_w16(d+46,7);assert(!gbr_validate(2,d,sizeof d));gbr_w16(d+46,8);assert(gbr_validate(2,d,sizeof d));gbr_w16(d+46,0);d[44]=2;assert(gbr_validate(2,d,sizeof d));
    uint8_t rgb[]={1,0,0,100,25};assert(!gbr_validate(3,rgb,sizeof rgb));gbr_w16(rgb+1,360);assert(gbr_validate(3,rgb,sizeof rgb));
    puts("GbrTzT binary format validation passed");return 0;
}
