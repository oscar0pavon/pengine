#ifndef PE_ANDROID_H
#define PE_ANDROID_H

#include <android_native_app_glue.h>
void pe_android_handle_cmd(struct android_app *app, int32_t cmd);
void pe_android_poll_envents();

#endif // !PE_ANDROID_H
