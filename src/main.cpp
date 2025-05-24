#include "stb_image.h"
#include <filesystem>
#include <fstream> // added for file I/O
#include <iostream>
#include <jpeglib.h>
#include <stdio.h>
#include <vector>

void encrypt_data() {}

// stub: hook to modify JPEG bitstream or DCT blocks after coeffs are read
void process_coeffs(jpeg_decompress_struct &din, jvirt_barray_ptr *coeffs) {
    for (int comp = 0; comp < din.num_components; comp++) {
        jpeg_component_info *ci = din.comp_info + comp;
        for (JDIMENSION row = 0; row < ci->height_in_blocks; row++) {
            JBLOCKARRAY buffer = din.mem->access_virt_barray(
                (j_common_ptr)&din, coeffs[comp], row, 1, TRUE);
            for (JDIMENSION col = 0; col < ci->width_in_blocks; col++) {
                JCOEFPTR block = buffer[0][col];
                // reverse the 64 coefficients in this block
                for (int i = 0; i < DCTSIZE2/2; ++i) {
                    JCOEF tmp = block[i];
                    block[i] = block[DCTSIZE2 - 1 - i];
                    block[DCTSIZE2 - 1 - i] = tmp;
                }
            }
        }
    }
}

// convert_to_jpeg:
//   Takes raw RGB pixels, runs through libjpeg‐turbo stages,
//   and returns a compressed JPEG byte buffer.
void convert_to_jpeg(const unsigned char *raw, int w, int h,
                     std::vector<unsigned char> &jpeg_buf, int quality = 90) {
  jpeg_compress_struct cinfo;
  jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  // write to memory
  unsigned char *outptr = nullptr;
  unsigned long outsize = 0;
  jpeg_mem_dest(&cinfo, &outptr, &outsize);

  // set image params
  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);

  // 1) start compressor → color conversion, downsampling, DCT, quant, Huffman
  jpeg_start_compress(&cinfo, TRUE);

  // default pipeline: write scanlines
  JSAMPROW row[1];
  while (cinfo.next_scanline < cinfo.image_height) {
    row[0] = const_cast<JSAMPROW>(&raw[cinfo.next_scanline * w * 3]);
    jpeg_write_scanlines(&cinfo, row, 1);
  }

  // 2) finish and collect buffer
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  // copy memory into vector and free
  jpeg_buf.assign(outptr, outptr + outsize);
  free(outptr);
}

// edit_in_memory_jpeg:
//   Decompress header+coeffs, call process_coeffs hook, then re‐write JPEG.
void edit_in_memory_jpeg(std::vector<unsigned char> &buf) {
  jpeg_decompress_struct din;
  jpeg_error_mgr jerr;
  din.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&din);

  jpeg_mem_src(&din, buf.data(), buf.size());
  jpeg_read_header(&din, TRUE);

  // get DCT coefficient arrays
  jvirt_barray_ptr *coeffs = jpeg_read_coefficients(&din);
  process_coeffs(din, coeffs);

  // setup compressor to write modified coeffs back in‐memory
  jpeg_compress_struct dout;
  dout.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&dout);

  unsigned char *outptr = nullptr;
  unsigned long outsize = 0;
  jpeg_mem_dest(&dout, &outptr, &outsize);

  // preserve original tables and params
  jpeg_copy_critical_parameters(&din, &dout);
  jpeg_write_coefficients(&dout, coeffs);

  // finalize
  jpeg_finish_compress(&dout);
  jpeg_destroy_compress(&dout);
  jpeg_finish_decompress(&din);
  jpeg_destroy_decompress(&din);

  // update buffer
  buf.assign(outptr, outptr + outsize);
  free(outptr);
}

// load_image:
//   Reads the whole file into a memory buffer, then decodes via stb_image.
//   Supports JPEG, PNG, BMP, etc.
bool load_image(const std::wstring &path, int &w, int &h,
                std::vector<unsigned char> &out) {
  // open file as binary stream (Windows: use _wfopen for wide paths)
  FILE *file = _wfopen(path.c_str(), L"rb");
  if (!file)
    return false;

  // get file size
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  // read all bytes into vector<char>
  std::vector<char> fileData(fileSize);
  if (fread(fileData.data(), 1, fileSize, file) !=
      static_cast<size_t>(fileSize)) {
    fclose(file);
    return false;
  }
  fclose(file);

  // decode from memory, force 3 channels (RGB)
  int channels;
  unsigned char *img = stbi_load_from_memory(
      reinterpret_cast<unsigned char *>(fileData.data()),
      static_cast<int>(fileData.size()), &w, &h, &channels, 3);
  if (!img)
    return false;

  // move decoded pixels into output vector
  size_t total = static_cast<size_t>(w) * h * 3;
  out.assign(img, img + total);
  stbi_image_free(img);
  return true;
}

int main() {
  namespace fs = std::filesystem;

  // Determine the executable’s current working directory
  fs::path exeDir = fs::current_path();

  // Construct input/output file paths relative to working dir
  fs::path infile = exeDir / "images" / "raw" / "example1.bmp";
  fs::path outfile = exeDir / "images" / "edited" / "example1.jpeg";

  // Variables to receive image metadata and pixel buffer
  int w, h;
  std::vector<unsigned char> data;

  // 1) Load image into memory
  if (!load_image(infile.wstring(), w, h, data)) {
    std::wcerr << L"Cannot load “" << infile.wstring() << L"”\n";
    return 1;
  }

  // convert raw RGB → JPEG in memory
  std::vector<unsigned char> jpegData;
  convert_to_jpeg(data.data(), w, h, jpegData, /*quality=*/100);

  // edit DCT‐level coefficients in memory
  edit_in_memory_jpeg(jpegData);

  // write final JPEG blob to disk
  FILE *fout = _wfopen(outfile.c_str(), L"wb");
  fwrite(jpegData.data(), 1, jpegData.size(), fout);
  fclose(fout);

  return 0;
}