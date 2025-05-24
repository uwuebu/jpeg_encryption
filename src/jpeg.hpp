#pragma once
#include <stdio.h> // Ensure FILE is defined
#include <jpeglib.h>
#include <string>
#include <vector>

class Jpeg {
public:
  // load from disk; returns false on error
  bool load(const std::wstring &path);

  // apply DCT-block manipulation hook
  void processCoeffs();

  // save back to disk with given quality
  bool save(const std::wstring &path, int quality = 90);

  // accessors
  int getWidth() const;
  int getHeight() const;
  int getComponents() const;

private:
  // JPEG internals
  jpeg_decompress_struct din{};
  jpeg_compress_struct dout{};
  jpeg_error_mgr jerr{};
  jvirt_barray_ptr *coeffs = nullptr;

  int width = 0;
  int height = 0;
  int comps = 0;
};