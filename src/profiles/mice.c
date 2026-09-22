#include "blu2usb/profiles/mice.h"
#include <string.h>
#include <ctype.h>
void blu2usb_mouse_title(const char *name,char out[22]) {
    char upper[32]={0}; unsigned n=0;
    for(unsigned i=0;i<31 && name[i];i++) { unsigned char c=(unsigned char)name[i]; upper[i]=(char)toupper(c); }
    bool word=false;
    for(unsigned i=0;upper[i];i++) if(!strncmp(upper+i,"MOUSE",5) &&
        (i==0 || (!isalnum((unsigned char)upper[i-1]) && upper[i-1]!='_')) &&
        (!isalnum((unsigned char)upper[i+5]) && upper[i+5]!='_')) word=true;
    for(unsigned i=0;upper[i] && n<15;i++) {
        char c=upper[i]; if(isalnum((unsigned char)c) || strchr(" -:./\\><?!",c)) out[n++]=c;
    }
    while(n && out[n-1]==' ') n--;
    out[n]=0;
    if(!n) strcpy(out,"UNKNOWN MOUSE"); else if(!word) strcat(out," MOUSE");
}
unsigned blu2usb_mice_count(const blu2usb_mice_t *m) { unsigned n=0;for(unsigned i=0;i<16;i++) n+=m->mice[i].used;return n; }
int blu2usb_mice_page(const blu2usb_mice_t *m,int live,unsigned page) {
    if(live>=0 && live<16 && m->mice[live].used) { if(!page)return live;page--; }
    for(int i=0;i<16;i++) if(i!=live && m->mice[i].used) { if(!page)return i;page--; }
    return -1;
}
bool blu2usb_mice_encode(const blu2usb_mice_t *m,uint8_t out[BLU2USB_MICE_BYTES]) {
    memset(out,0,BLU2USB_MICE_BYTES); out[0]=3;
    if(!blu2usb_profiles_serialize(&m->profiles,out+2))return false;
    for(unsigned i=0;i<16;i++) { unsigned p=16+i*34; out[p]=m->mice[i].used;out[p+1]=(uint8_t)m->mice[i].profile;memcpy(out+p+2,m->mice[i].name,31); }
    return true;
}
bool blu2usb_mice_decode(blu2usb_mice_t *m,const uint8_t *in,size_t size) {
    blu2usb_mice_t result={0};
    if(size!=BLU2USB_MICE_BYTES || in[0]!=3 || !blu2usb_profiles_restore(&result.profiles,in+2)) return false;
    for(unsigned i=0;i<16;i++) { unsigned p=16+i*34;if(in[p]>1 || in[p+1]>3 || in[p+33])return false;
        result.mice[i].used=in[p];result.mice[i].profile=(blu2usb_mouse_profile_kind_t)in[p+1];memcpy(result.mice[i].name,in+p+2,32); }
    *m=result;return true;
}
