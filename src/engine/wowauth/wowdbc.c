#include "wowdbc.h"

#include <engine/utils.h>

#include <string.h>

//real DBCs cap in the low millions of records and low hundreds of fields;
//rejecting anything wilder up front keeps a corrupt header from turning the
//size math below into a bogus, huge "expected" that happens to pass
#define WOWDBC_RECORD_COUNT_MAX 10000000
#define WOWDBC_FIELD_COUNT_MAX 1024

bool pe_wowdbc_load(const char *path, PWowDBC *out) {
  ZERO(*out);

  if (pe_file_openb(path, &out->file) != 0)
    return false;

  const u8 *data = out->file.data;
  u32 size = out->file.size_in_bytes;

  if (size < 20 || memcmp(data, "WDBC", 4) != 0) {
    close_file(&out->file);
    return false;
  }

  u32 header[4];
  memcpy(header, data + 4, sizeof(header));
  out->record_count = header[0];
  out->field_count = header[1];
  out->record_size = header[2];
  out->string_block_size = header[3];

  if (out->record_count > WOWDBC_RECORD_COUNT_MAX ||
      out->field_count > WOWDBC_FIELD_COUNT_MAX) {
    close_file(&out->file);
    return false;
  }

  u64 expected = 20 + (u64)out->record_count * out->record_size +
                 out->string_block_size;
  if (size < expected) {
    close_file(&out->file);
    return false;
  }

  out->records = data + 20;
  out->strings = (const char *)out->records +
                 (u64)out->record_count * out->record_size;
  return true;
}

void pe_wowdbc_free(PWowDBC *dbc) {
  close_file(&dbc->file);
}

u32 pe_wowdbc_get_u32(const PWowDBC *dbc, u32 record, u32 field) {
  if (record >= dbc->record_count || field >= dbc->field_count)
    return 0;

  u32 value;
  memcpy(&value,
         dbc->records + (u64)record * dbc->record_size + (u64)field * 4, 4);
  return value;
}

const char *pe_wowdbc_get_string(const PWowDBC *dbc, u32 record, u32 field) {
  u32 offset = pe_wowdbc_get_u32(dbc, record, field);
  if (offset >= dbc->string_block_size)
    return "";
  return dbc->strings + offset;
}
