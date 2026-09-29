#ifndef PE_WOWDBC_H
#define PE_WOWDBC_H

#include <engine/file_loader.h>
#include <engine/numbers.h>
#include <stdbool.h>

//a WDBC file: a 20-byte header, then record_count * record_size bytes of
//fixed-width uint32 fields, then a string block a string field's value is a
//byte offset into. https://wowdev.wiki/DBC
typedef struct PWowDBC {
  File file;
  const u8 *records;
  const char *strings;
  u32 record_count;
  u32 field_count;
  u32 record_size;
  u32 string_block_size;
} PWowDBC;

//reads path whole and parses its WDBC header. false if it does not exist, is
//not a WDBC file, or is truncated against its own header's record/string
//counts. keeps the file's buffer alive until pe_wowdbc_free
bool pe_wowdbc_load(const char *path, PWowDBC *out);

void pe_wowdbc_free(PWowDBC *dbc);

u32 pe_wowdbc_get_u32(const PWowDBC *dbc, u32 record, u32 field);

//a string field's value is an offset into the string block; a record, field
//or offset out of range answers "", never NULL
const char *pe_wowdbc_get_string(const PWowDBC *dbc, u32 record, u32 field);

#endif
