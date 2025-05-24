#include "jpeg.hpp"
#include <filesystem>
#include <iostream>
#include <stdio.h>

bool Jpeg::load(const std::wstring &path) {
  jpeg_create_decompress(&din);
  din.err = jpeg_std_error(&jerr);
  FILE *f = _wfopen(path.c_str(), L"rb");
  if (!f)
    return false;
  jpeg_stdio_src(&din, f);
  if (jpeg_read_header(&din, TRUE) != JPEG_HEADER_OK) {
    fclose(f);
    return false;
  }
  coeffs = jpeg_read_coefficients(&din);
  width = din.image_width;
  height = din.image_height;
  comps = din.num_components;
  fclose(f);
  return true;
}

void Jpeg::processCoeffs() {
  for (int comp = 0; comp < din.num_components; comp++) {
    // Only process the luminance component (typically index 0)
    if (comp > 0) {
      continue;
    }
    auto *ci = din.comp_info + comp;
    int rows = ci->height_in_blocks;
    int cols = ci->width_in_blocks;
    // swap entire block rows
    for (int r = 0; r < rows / 2; ++r) {
      // get pointers to block-row r and its mirror row
      JBLOCKARRAY rowA = din.mem->access_virt_barray((j_common_ptr)&din,
                                                     coeffs[comp], r, 1, TRUE);
      JBLOCKARRAY rowB = din.mem->access_virt_barray(
          (j_common_ptr)&din, coeffs[comp], rows - 1 - r, 1, TRUE);
      for (int c = 0; c < cols; ++c) {
        JCOEFPTR blockA = rowA[0][c];
        JCOEFPTR blockB = rowB[0][c];
        // swap whole 8x8 blocks (64 coefficients)
        for (int i = 0; i < DCTSIZE2; ++i) {
          std::swap(blockA[i], blockB[i]);
        }
      }
    }
  }
}

bool Jpeg::save(const std::wstring &path, int quality) {
  jpeg_create_compress(&dout);
  dout.err = jpeg_std_error(&jerr);
  FILE *f = _wfopen(path.c_str(), L"wb");
  if (!f)
    return false;
  jpeg_stdio_dest(&dout, f);
  jpeg_copy_critical_parameters(&din, &dout);
  jpeg_write_coefficients(&dout, coeffs);
  jpeg_finish_compress(&dout);
  jpeg_destroy_compress(&dout);
  jpeg_finish_decompress(&din);
  jpeg_destroy_decompress(&din);
  fclose(f);
  return true;
}

int Jpeg::getWidth() const { return width; }
int Jpeg::getHeight() const { return height; }
int Jpeg::getComponents() const { return comps; }
