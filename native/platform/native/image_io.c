#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <windows.h>
#include <wincodec.h>

#include "moonbit.h"

static void lunarrender_write_u32_le(moonbit_bytes_t bytes, int32_t offset, uint32_t value) {
  bytes[offset] = (uint8_t)(value & 0xffu);
  bytes[offset + 1] = (uint8_t)((value >> 8) & 0xffu);
  bytes[offset + 2] = (uint8_t)((value >> 16) & 0xffu);
  bytes[offset + 3] = (uint8_t)((value >> 24) & 0xffu);
}

moonbit_bytes_t lunarrender_image_decode_rgba(moonbit_string_t path) {
  HRESULT init_result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  const bool should_uninitialize = SUCCEEDED(init_result);
  IWICImagingFactory *factory = NULL;
  IWICBitmapDecoder *decoder = NULL;
  IWICBitmapFrameDecode *frame = NULL;
  IWICFormatConverter *converter = NULL;
  moonbit_bytes_t result = moonbit_make_bytes(0, 0);
  HRESULT result_code = CoCreateInstance(
      &CLSID_WICImagingFactory,
      NULL,
      CLSCTX_INPROC_SERVER,
      &IID_IWICImagingFactory,
      (void **)&factory);
  if (FAILED(result_code)) {
    fprintf(stderr, "LunarRender image decode failed at factory\n");
    goto cleanup;
  }
  result_code = factory->lpVtbl->CreateDecoderFromFilename(
      factory,
      (LPCWSTR)path,
      NULL,
      GENERIC_READ,
      WICDecodeMetadataCacheOnLoad,
      &decoder);
  if (FAILED(result_code)) {
    fprintf(stderr, "LunarRender image decode failed at decoder\n");
    goto cleanup;
  }
  result_code = decoder->lpVtbl->GetFrame(decoder, 0, &frame);
  if (FAILED(result_code)) {
    fprintf(stderr, "LunarRender image decode failed at frame\n");
    goto cleanup;
  }
  result_code = factory->lpVtbl->CreateFormatConverter(factory, &converter);
  if (FAILED(result_code)) {
    fprintf(stderr, "LunarRender image decode failed at converter\n");
    goto cleanup;
  }
  result_code = converter->lpVtbl->Initialize(
      converter,
      (IWICBitmapSource *)frame,
      &GUID_WICPixelFormat32bppRGBA,
      WICBitmapDitherTypeNone,
      NULL,
      0.0,
      WICBitmapPaletteTypeCustom);
  if (FAILED(result_code)) {
    fprintf(stderr, "LunarRender image decode failed at converter initialization\n");
    goto cleanup;
  }
  UINT width = 0;
  UINT height = 0;
  result_code = converter->lpVtbl->GetSize(converter, &width, &height);
  if (FAILED(result_code) || width == 0 || height == 0 || width > 4096 || height > 4096) {
    fprintf(stderr, "LunarRender image decode failed at dimensions\n");
    goto cleanup;
  }
  const uint64_t pixel_size = (uint64_t)width * (uint64_t)height * 4u;
  if (pixel_size > 64u * 1024u * 1024u) {
    fprintf(stderr, "LunarRender image decode rejected oversized image\n");
    goto cleanup;
  }
  result = moonbit_make_bytes_raw((int32_t)pixel_size + 8);
  lunarrender_write_u32_le(result, 0, width);
  lunarrender_write_u32_le(result, 4, height);
  WICRect rect = {0, 0, (INT)width, (INT)height};
  result_code = converter->lpVtbl->CopyPixels(
      converter,
      &rect,
      width * 4,
      (UINT)pixel_size,
      result + 8);
  if (FAILED(result_code)) {
    fprintf(stderr, "LunarRender image decode failed at pixels\n");
    result = moonbit_make_bytes(0, 0);
  }

cleanup:
  if (converter != NULL) converter->lpVtbl->Release(converter);
  if (frame != NULL) frame->lpVtbl->Release(frame);
  if (decoder != NULL) decoder->lpVtbl->Release(decoder);
  if (factory != NULL) factory->lpVtbl->Release(factory);
  if (should_uninitialize) CoUninitialize();
  return result;
}
