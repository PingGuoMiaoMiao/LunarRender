#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#include "moonbit.h"

static bool lunarrender_valid_path(moonbit_string_t path) {
  if (path == NULL) {
    return false;
  }
  const int32_t length = Moonbit_array_length(path);
  return length > 0 && length <= 32768;
}

bool lunarrender_save_write_temp(moonbit_string_t path, moonbit_bytes_t bytes) {
  if (!lunarrender_valid_path(path) || bytes == NULL) {
    fprintf(stderr, "LunarRender save write rejected invalid input\n");
    return false;
  }
  FILE *file = _wfopen((const wchar_t *)path, L"wb");
  if (file == NULL) {
    fprintf(stderr, "LunarRender save write failed at fopen\n");
    return false;
  }
  const int32_t length = Moonbit_array_length(bytes);
  const size_t written = fwrite(bytes, 1, (size_t)length, file);
  const bool flushed = fflush(file) == 0;
  const bool closed = fclose(file) == 0;
  if (written != (size_t)length || !flushed || !closed) {
    fprintf(stderr, "LunarRender save write failed at flush\n");
    return false;
  }
  return true;
}

bool lunarrender_save_replace_temp(moonbit_string_t temp_path, moonbit_string_t target_path) {
  if (!lunarrender_valid_path(temp_path) || !lunarrender_valid_path(target_path)) {
    fprintf(stderr, "LunarRender save replace rejected invalid path\n");
    return false;
  }
  const BOOL replaced = MoveFileExW(
      (const wchar_t *)temp_path,
      (const wchar_t *)target_path,
      MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
  if (!replaced) {
    fprintf(stderr, "LunarRender save replace failed at MoveFileExW\n");
    return false;
  }
  return true;
}

bool lunarrender_save_ensure_directory(moonbit_string_t path) {
  if (!lunarrender_valid_path(path)) {
    fprintf(stderr, "LunarRender save directory rejected invalid path\n");
    return false;
  }
  const DWORD attributes = GetFileAttributesW((const wchar_t *)path);
  if (attributes != INVALID_FILE_ATTRIBUTES) {
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  if (CreateDirectoryW((const wchar_t *)path, NULL)) {
    return true;
  }
  const DWORD error = GetLastError();
  const DWORD existing_attributes = GetFileAttributesW((const wchar_t *)path);
  return error == ERROR_ALREADY_EXISTS && existing_attributes != INVALID_FILE_ATTRIBUTES &&
      (existing_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
