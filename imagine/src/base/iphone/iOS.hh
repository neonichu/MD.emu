#pragma once

#include <util/ansiTypes.h>

namespace Base
{
void ios_setVideoInterval(uint interval);

void ios_setICadeActive(uint active);
uint ios_iCadeActive();
}

#ifdef __cplusplus
extern "C" {
#endif

enum { IPHONE_ISRC_LIBRARY = 0, IPHONE_ISRC_CAMERA };
uchar base_iphone_canUseImageSrc(uchar src);
typedef void (*IPhoneImgPickerCallback)(void *user, const void *imgData, unsigned long imgDataSize, const void *thumbData, unsigned long thumbDataSize);
void base_iphone_pickImage(uchar src, void * user, IPhoneImgPickerCallback callback);
void base_iphone_releasePickedImage();

void base_iphone_sendMail(const char *subject, const char *body, const char *attachementPath);

typedef void (*IPhoneGKDataCallback)(void *user, const void *data, unsigned long size);
void iphone_pickPeer(IPhoneGKDataCallback callback);

#if defined(GREYSTRIPE)
    void GSDisplayAd();
#endif
    
#ifdef __cplusplus
}
#endif
