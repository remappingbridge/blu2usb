#ifndef BLU2USB_MICE_H
#define BLU2USB_MICE_H
#include "blu2usb/profiles/profiles.h"
#define BLU2USB_MAX_MICE 16
#define BLU2USB_MICE_BYTES (16 + BLU2USB_MAX_MICE * 34)
typedef struct { bool used; char name[32]; blu2usb_mouse_profile_kind_t profile; } blu2usb_saved_mouse_t;
typedef struct { blu2usb_profiles_t profiles; blu2usb_saved_mouse_t mice[BLU2USB_MAX_MICE]; } blu2usb_mice_t;
void blu2usb_mouse_title(const char *name,char out[22]);
unsigned blu2usb_mice_count(const blu2usb_mice_t *m);
int blu2usb_mice_page(const blu2usb_mice_t *m,int live,unsigned page);
bool blu2usb_mice_encode(const blu2usb_mice_t *m,uint8_t out[BLU2USB_MICE_BYTES]);
bool blu2usb_mice_decode(blu2usb_mice_t *m,const uint8_t *in,size_t size);
#endif
