#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

#include "moonbit.h"

moonbit_bytes_t lunarrender_file_read(moonbit_string_t path) {
  if (path == NULL) {
    fprintf(stderr, "LunarRender file read rejected a null path\n");
    return moonbit_make_bytes(0, 0);
  }
  const int32_t path_length = Moonbit_array_length(path);
  if (path_length <= 0 || path_length > 32768) {
    fprintf(stderr, "LunarRender file read rejected an empty or oversized path\n");
    return moonbit_make_bytes(0, 0);
  }
  FILE *file = _wfopen((const wchar_t *)path, L"rb");
  if (file == NULL) {
    fprintf(stderr, "LunarRender file read failed at fopen\n");
    return moonbit_make_bytes(0, 0);
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    fprintf(stderr, "LunarRender file read failed at fseek\n");
    return moonbit_make_bytes(0, 0);
  }
  long length = ftell(file);
  if (length <= 0 || length > 64L * 1024L * 1024L) {
    fclose(file);
    fprintf(stderr, "LunarRender file read failed at size\n");
    return moonbit_make_bytes(0, 0);
  }
  rewind(file);
  moonbit_bytes_t bytes = moonbit_make_bytes_raw((int32_t)length);
  const size_t read = fread(bytes, 1, (size_t)length, file);
  fclose(file);
  if (read != (size_t)length) {
    fprintf(stderr, "LunarRender file read failed at fread\n");
    return moonbit_make_bytes(0, 0);
  }
  return bytes;
}

bool lunarrender_file_exists(moonbit_string_t path) {
  if (path == NULL) {
    return false;
  }
  const int32_t path_length = Moonbit_array_length(path);
  if (path_length <= 0 || path_length > 32768) {
    return false;
  }
  const DWORD attributes = GetFileAttributesW((const wchar_t *)path);
  return attributes != INVALID_FILE_ATTRIBUTES &&
      (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
