#ifndef PE_CONTENT_MANAGER_H
#define PE_CONTENT_MANAGER_H

#include "../engine/numbers.h"

typedef enum ContentType{
    CONTENT_TYPE_STATIC_MESH = 0,
    CONTENT_TYPE_SKINNED_MESH,
    CONTENT_TYPE_TEXTURE,
    CONTENT_TYPE_SOUND,
    CONTENT_TYPE_SHADER,
    CONTENT_TYPE_MATERIAL,
	CONTENT_TYPE_LEVEL,
	CONTENT_TYPE_PROJECT,
	CONTENT_TYPE_NULL
}ContentType;

typedef struct Content{
    char name[40];
    char path[100];
    ContentType type;
    u32 id;
}Content;

static const u32 PVN_BINARY_FILE_MAGIC = 0x426E7670;


int content_manager_load_content(const char* path);

int content_manager_get_content_type_from_binary(const char* path);
#endif
