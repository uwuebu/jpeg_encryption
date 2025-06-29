#include "jpeg.hpp"
#include "chaotic_keystream_generator.hpp" // Replace .cpp with .hpp
#include <algorithm>                       // Include this for std::remove
#include <cstdint>                         // for uint64_t
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>

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

std::vector<int>
Jpeg::generateDCPermutationKeystream(int lenDC,
                                     const ChaoticSystems::MasterKey &key) {

  if (lenDC <= 1) {
    std::cerr << "Error: Invalid length for DC coefficients.\n";
    return {};
  }

  //auto jiaKeystream = key.generateJiaKeystream(lenDC - 1);
  auto jiaKeystream = key.generateArnoldKeystream(lenDC - 1);
  std::vector<int> permutationKeystream;

  for (int m = 0; m < lenDC - 2; ++m) {
    double sm = std::fabs(jiaKeystream[m]);
    uint64_t sigDigits = extractSignificantDigits(sm, key.alpha);
    int offset = sigDigits % (lenDC - m);
    permutationKeystream.push_back(m + offset);
  }

  return permutationKeystream;
}

uint64_t Jpeg::extractSignificantDigits(double value, int digits) {
  if (value <= 0.0 || digits <= 0 || digits > 17)
    return 0;

  int exponent = static_cast<int>(std::floor(std::log10(value)));
  double scaled = value / std::pow(10, exponent - digits + 1);
  uint64_t result = static_cast<uint64_t>(scaled);

  return result;
}

void Jpeg::processDCWithKey(bool isLuminance, const std::vector<int> &key) {
  std::vector<int> dcCoefficients = extractDC(isLuminance);

  // Apply the permutation using the provided key
  int lenDC = dcCoefficients.size();
  for (int m = 0; m < lenDC - 2; ++m) {
    int km = key[m];

    // Ensure km is within bounds
    if (km >= 0 && km < lenDC) {
      std::swap(dcCoefficients[m], dcCoefficients[km]);
    } else {
      std::cerr << "Error: Permutation index out of bounds. m=" << m
                << ", km=" << km << ", lenDC=" << lenDC << "\n";
    }
  }

  applyDC(dcCoefficients, isLuminance);
}

void Jpeg::processDCReverse(bool isLuminance, const std::vector<int> &key) {
  std::vector<int> dcCoefficients = extractDC(isLuminance);

  // Apply the reverse permutation using the provided key
  int lenDC = dcCoefficients.size();
  for (int m = lenDC - 3; m >= 0; --m) { // Iterate through the key in reverse
    int km = key[m];
    std::swap(dcCoefficients[m], dcCoefficients[km]);
  }

  applyDC(dcCoefficients, isLuminance);
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

std::vector<int> Jpeg::extractDC(bool isLuminance) {
  std::vector<int> dcCoefficients;

  for (int comp = 0; comp < din.num_components; comp++) {
    if (isLuminance && comp > 0)
      continue; // Skip chrominance
    if (!isLuminance && comp == 0)
      continue; // Skip luminance

    auto *ci = din.comp_info + comp;
    int rows = ci->height_in_blocks;
    int cols = ci->width_in_blocks;

    for (int r = 0; r < rows; ++r) {
      JBLOCKARRAY row = din.mem->access_virt_barray((j_common_ptr)&din,
                                                    coeffs[comp], r, 1, TRUE);
      for (int c = 0; c < cols; ++c) {
        JCOEFPTR block = row[0][c];
        dcCoefficients.push_back(block[0]); // Extract DC coefficient
      }
    }
  }

  // std::cout << "Extracted " << dcCoefficients.size() << " DC
  // coefficients.\n";
  return dcCoefficients;
}

void Jpeg::applyDC(const std::vector<int> &dcCoefficients, bool isLuminance) {
  int index = 0;

  for (int comp = 0; comp < din.num_components; comp++) {
    if (isLuminance && comp > 0)
      continue; // Skip chrominance
    if (!isLuminance && comp == 0)
      continue; // Skip luminance

    auto *ci = din.comp_info + comp;
    int rows = ci->height_in_blocks;
    int cols = ci->width_in_blocks;

    for (int r = 0; r < rows; ++r) {
      JBLOCKARRAY row = din.mem->access_virt_barray((j_common_ptr)&din,
                                                    coeffs[comp], r, 1, TRUE);
      for (int c = 0; c < cols; ++c) {
        if (index >= dcCoefficients.size()) {
          std::cerr << "Error: DC coefficient index out of range. index="
                    << index << ", size=" << dcCoefficients.size() << "\n";
          return;
        }

        JCOEFPTR block = row[0][c];
        block[0] = dcCoefficients[index++]; // Apply modified DC coefficient
      }
    }
  }
}

std::vector<std::vector<int>> Jpeg::extractAC(bool isLuminance) {
  std::vector<std::vector<int>> acCoefficients;

  for (int comp = 0; comp < din.num_components; comp++) {
    if (isLuminance && comp > 0)
      continue; // Skip chrominance
    if (!isLuminance && comp == 0)
      continue; // Skip luminance

    auto *ci = din.comp_info + comp;
    int rows = ci->height_in_blocks;
    int cols = ci->width_in_blocks;

    for (int r = 0; r < rows; ++r) {
      JBLOCKARRAY row = din.mem->access_virt_barray((j_common_ptr)&din,
                                                    coeffs[comp], r, 1, TRUE);
      for (int c = 0; c < cols; ++c) {
        JCOEFPTR block = row[0][c];
        std::vector<int> acBlock;

        for (int i = 1; i < DCTSIZE2; ++i) {
          acBlock.push_back(block[i]); // Extract AC coefficients
        }

        acCoefficients.push_back(acBlock);
      }
    }
  }

  return acCoefficients;
}

void Jpeg::applyAC(const std::vector<std::vector<int>> &acCoefficients,
                   bool isLuminance) {
  int index = 0;

  for (int comp = 0; comp < din.num_components; comp++) {
    if (isLuminance && comp > 0)
      continue; // Skip chrominance
    if (!isLuminance && comp == 0)
      continue; // Skip luminance

    auto *ci = din.comp_info + comp;
    int rows = ci->height_in_blocks;
    int cols = ci->width_in_blocks;

    for (int r = 0; r < rows; ++r) {
      JBLOCKARRAY row = din.mem->access_virt_barray((j_common_ptr)&din,
                                                    coeffs[comp], r, 1, TRUE);
      for (int c = 0; c < cols; ++c) {
        JCOEFPTR block = row[0][c];
        const auto &acBlock = acCoefficients[index++];

        // Only iterate through the actual number of AC values in the block
        for (size_t i = 0; i < acBlock.size(); ++i) {
          block[i + 1] = acBlock[i]; // Apply modified AC coefficients
        }
      }
    }
  }
}

std::vector<int> Jpeg::substituteDC(const std::vector<int> &DC,
                                    const std::vector<double> &logisticKS,
                                    int alpha) {
  std::vector<int> DC_encrypted(DC.size(), 0);
  int prevCipherSign = 0;
  int prevCipherMag = 0;

  size_t ks_index = 0; // separate index for keystream

  for (size_t n = 0; n < DC.size(); ++n) {
    int dc = DC[n];

    // Step 1: Skip values 0 or -1024
    if (dc == 0 || dc == -1024) {
      DC_encrypted[n] = dc;
      continue;
    }

    // Step 2: Extract sig only once
    int64_t sig = extractSignificantDigits(logisticKS[ks_index], alpha);
    int ks_sign = sig % 2;

    // Step 2.2: Substitute and diffuse sign
    int sign_n = (dc < 0) ? 1 : 0;
    int sign_c = ks_sign ^ sign_n ^ prevCipherSign;
    prevCipherSign = sign_c;

    // Step 3.1: Bit length, MSB, and mask
    int mag = std::abs(dc);
    int digit = static_cast<int>(std::floor(std::log2(mag))) + 1;
    int msb = 1 << (digit - 1);
    int mask = msb - 1;

    // Step 3.2: Derive km from same sig
    int km = sig & mask;

    // Step 3.3: Substitute and diffuse magnitude (excluding MSB)
    int sum = (mag + km) & mask; // wraparound safe
    int substituted = (km ^ sum ^ prevCipherMag) & mask;
    substituted |= msb;
    prevCipherMag = substituted;

    // Step 4: Reapply encrypted sign
    DC_encrypted[n] = (sign_c == 1) ? -substituted : substituted;

    ++ks_index; // Only increment for non-skipped DCs
  }

  return DC_encrypted;
}

std::vector<int> Jpeg::decryptDC(const std::vector<int> &DC_encrypted,
                                 const std::vector<double> &logisticKS,
                                 int alpha) {
  std::vector<int> DC(DC_encrypted.size(), 0);
  int prevCipherSign = 0;
  int prevCipherMag = 0;

  size_t ks_index = 0; // separate index for keystream

  for (size_t n = 0; n < DC_encrypted.size(); ++n) {
    int dc_c = DC_encrypted[n];

    // Step 1: Skip values 0 or -1024
    if (dc_c == 0 || dc_c == -1024) {
      DC[n] = dc_c;
      continue;
    }

    // Step 2: Extract sig once
    int64_t sig = extractSignificantDigits(logisticKS[ks_index], alpha);
    int ks_sign = sig % 2;

    int sign_c = (dc_c < 0) ? 1 : 0;
    int sign = sign_c ^ ks_sign ^ prevCipherSign;
    prevCipherSign = sign_c;

    int mag_c = std::abs(dc_c);
    int digit = static_cast<int>(std::floor(std::log2(mag_c))) + 1;
    int msb = 1 << (digit - 1);
    int mask = msb - 1;

    int km = sig & mask;

    int maskedPart = mag_c & mask;
    int unmasked = (maskedPart ^ km ^ prevCipherMag);
    int mag = ((unmasked - km + mask + 1) & mask) | msb; // wraparound safe
    prevCipherMag = mag_c;

    DC[n] = (sign == 1) ? -mag : mag;

    ++ks_index; // Only increment for non-skipped DCs
  }

  return DC;
}

std::vector<std::vector<int>>
Jpeg::generateACPermutationKeys(bool isLuminance,
                                const ChaoticSystems::MasterKey &key) {

  auto acBlocks = extractAC(isLuminance);
  std::vector<std::vector<int>> keys;

  for (size_t blockIndex = 0; blockIndex < acBlocks.size(); ++blockIndex) {
    const auto &block = acBlocks[blockIndex];
    std::vector<int> zeroGroupIndices;
    auto groups = extractACGroups(block, zeroGroupIndices);
    int nonZeroGroupCount = groups.size() - zeroGroupIndices.size();

    if (nonZeroGroupCount <= 1) {
      keys.emplace_back();
      continue;
    }

    // Use master key's Jia keystream (use blockIndex as offset/seed)
    //auto jiaKS = key.generateJiaKeystream(nonZeroGroupCount - 1);
    auto jiaKS = key.generateArnoldKeystream(nonZeroGroupCount - 1);


    std::vector<int> perm(nonZeroGroupCount - 1);
    for (int i = 0; i < nonZeroGroupCount - 2; ++i) {
      double sm = std::fabs(jiaKS[i]);
      int offset = static_cast<int>(sm * (nonZeroGroupCount - i)) %
                   (nonZeroGroupCount - i);
      perm[i] = i + offset;
    }

    keys.push_back(perm);
  }

  return keys;
}

void Jpeg::permuteACBlocks(bool isLuminance, const std::vector<int> &keys) {
  auto acBlocks = extractAC(isLuminance);

  int N = acBlocks.size();
  for (int i = 0; i < N - 1; ++i) {
    int j = keys[i];
    if (j >= N) {
      std::cerr << "Warning: Key index out of bounds during AC block "
                   "permutation. Skipping swap.\n";
      continue;
    }
    std::swap(acBlocks[i], acBlocks[j]); // Swap entire blocks
  }

  applyAC(acBlocks, isLuminance);
}

void Jpeg::reversePermuteACBlocks(bool isLuminance,
                                  const std::vector<int> &keys) {
  auto acBlocks = extractAC(isLuminance);

  int N = acBlocks.size();
  for (int i = N - 2; i >= 0; --i) {
    int j = keys[i];
    if (j >= N) {
      std::cerr << "Warning: Key index out of bounds during reverse AC block "
                   "permutation. Skipping swap.\n";
      continue;
    }
    std::swap(acBlocks[i], acBlocks[j]); // Reverse swap entire blocks
  }

  applyAC(acBlocks, isLuminance);
}

std::vector<std::vector<int>>
Jpeg::extractACGroups(const std::vector<int> &AC_block,
                      std::vector<int> &zeroGroupIndices) {
  std::vector<std::vector<int>> groups;
  std::vector<int> currentGroup;

  for (int i = 0; i < AC_block.size(); ++i) {
    currentGroup.push_back(AC_block[i]);

    // End group at first non-zero
    if (AC_block[i] != 0) {
      groups.push_back(currentGroup);
      currentGroup.clear();
    }
  }

  // Identify pure zero groups (e.g., [0, 0, ..., 0] of length 16)
  for (int i = 0; i < groups.size(); ++i) {
    if (groups[i].size() == 16 &&
        std::all_of(groups[i].begin(), groups[i].end(),
                    [](int x) { return x == 0; })) {
      zeroGroupIndices.push_back(i);
    }
  }

  return groups;
}

std::vector<std::vector<int>>
Jpeg::removeZeroGroups(const std::vector<std::vector<int>> &groups,
                       const std::vector<int> &zeroGroupIndices) {
  std::vector<std::vector<int>> result;
  for (int i = 0; i < groups.size(); ++i) {
    if (std::find(zeroGroupIndices.begin(), zeroGroupIndices.end(), i) ==
        zeroGroupIndices.end()) {
      result.push_back(groups[i]);
    }
  }
  return result;
}

void Jpeg::shuffleGroups(std::vector<std::vector<int>> &groups,
                         const std::vector<int> &keys) {
  int len = groups.size();
  for (int i = 0; i < len - 1; ++i) {
    int j = keys[i];
    if (j >= len) {
      std::cerr << "Warning: Key index out of bounds during shuffle. Skipping "
                   "swap.\n";
      continue;
    }
    std::swap(groups[i], groups[j]);
  }
}

void Jpeg::unshuffleGroups(std::vector<std::vector<int>> &groups,
                           const std::vector<int> &keys) {
  int len = groups.size();
  for (int i = len - 2; i >= 0; --i) {
    int j = keys[i];
    if (j >= len) {
      std::cerr << "Warning: Key index out of bounds during unshuffle. "
                   "Skipping swap.\n";
      continue;
    }
    std::swap(groups[i], groups[j]);
  }
}

void Jpeg::reinsertZeroGroups(
    std::vector<std::vector<int>> &groups,
    const std::vector<int> &zeroGroupIndices,
    const std::vector<std::vector<int>> &originalZeroGroups) {
  for (int i = 0; i < zeroGroupIndices.size(); ++i) {
    groups.insert(groups.begin() + zeroGroupIndices[i], originalZeroGroups[i]);
  }
}

std::vector<int>
Jpeg::flattenGroups(const std::vector<std::vector<int>> &groups) {
  std::vector<int> flat;
  for (const auto &group : groups) {
    flat.insert(flat.end(), group.begin(), group.end());
  }
  return flat;
}

int Jpeg::calculateLeadingZeroGroups(
    const std::vector<std::vector<int>> &AC_blocks) {
  int totalGroups = 0;

  for (const auto &block : AC_blocks) {
    std::vector<int> zeroGroupIndices;
    auto groups = extractACGroups(block, zeroGroupIndices);
    totalGroups +=
        groups.size() - zeroGroupIndices.size(); // Count non-zero groups
  }

  return totalGroups;
}

void Jpeg::processACIntraBlock(bool isLuminance,
                               const std::vector<std::vector<int>> &intraKeys,
                               bool reverse) {
  auto acBlocks = extractAC(isLuminance);

  for (size_t blockIndex = 0; blockIndex < acBlocks.size(); ++blockIndex) {
    auto &block = acBlocks[blockIndex];
    const auto &intraKey = intraKeys[blockIndex];

    std::vector<int> zeroGroupIndices;
    auto groups = extractACGroups(block, zeroGroupIndices);
    auto nonZeroGroups = removeZeroGroups(groups, zeroGroupIndices);

    if (reverse) {
      // First round reverse shuffle
      unshuffleGroups(nonZeroGroups, intraKey);

      // Reinsert zeros (round 1 done)
      reinsertZeroGroups(nonZeroGroups, zeroGroupIndices, groups);

      // Extract groups again (for second round)
      std::vector<int> zeroGroupIndices2;
      auto groups2 =
          extractACGroups(flattenGroups(nonZeroGroups), zeroGroupIndices2);
      auto nonZeroGroups2 = removeZeroGroups(groups2, zeroGroupIndices2);

      // Second round reverse shuffle
      if (!nonZeroGroups2.empty()) {
        unshuffleGroups(nonZeroGroups2, intraKey);
      }

      // Final reinsertion
      reinsertZeroGroups(nonZeroGroups2, zeroGroupIndices2, groups2);
      block = flattenGroups(nonZeroGroups2);
    } else {
      // First round shuffle
      shuffleGroups(nonZeroGroups, intraKey);

      // Reinsert zeros (round 1 done)
      reinsertZeroGroups(nonZeroGroups, zeroGroupIndices, groups);

      // Extract groups again (for second round)
      std::vector<int> zeroGroupIndices2;
      auto groups2 =
          extractACGroups(flattenGroups(nonZeroGroups), zeroGroupIndices2);
      auto nonZeroGroups2 = removeZeroGroups(groups2, zeroGroupIndices2);

      // Second round shuffle
      if (!nonZeroGroups2.empty()) {
        shuffleGroups(nonZeroGroups2, intraKey);
      }

      // Final reinsertion
      reinsertZeroGroups(nonZeroGroups2, zeroGroupIndices2, groups2);
      block = flattenGroups(nonZeroGroups2);
    }
  }

  applyAC(acBlocks, isLuminance);
}

void Jpeg::applyNonZeroAC(const std::vector<int> &encryptedAC,
                          bool isLuminance) {
  int acIndex = 0;

  for (int comp = 0; comp < din.num_components; comp++) {
    if (isLuminance && comp > 0)
      continue;
    if (!isLuminance && comp == 0)
      continue;

    auto *ci = din.comp_info + comp;
    int rows = ci->height_in_blocks;
    int cols = ci->width_in_blocks;

    for (int r = 0; r < rows; ++r) {
      JBLOCKARRAY row = din.mem->access_virt_barray((j_common_ptr)&din,
                                                    coeffs[comp], r, 1, TRUE);
      for (int c = 0; c < cols; ++c) {
        JCOEFPTR block = row[0][c];

        for (int i = 1; i < DCTSIZE2; ++i) { // i = 1 to 63 (AC coeffs)
          if (block[i] != 0) {
            block[i] = encryptedAC[acIndex++];
          }
        }
      }
    }
  }

  if (acIndex != encryptedAC.size()) {
    std::cerr
        << "[ERROR] applyNonZeroAC: Did not consume all encrypted AC values!\n";
  }
}

void Jpeg::substituteACInterBlock(
    bool isLuminance, const std::vector<double> &logisticKeyStream) {

  auto acBlocks = extractAC(isLuminance);

  std::vector<int> allAC;
  std::vector<int> zeroGroupIndices;

  for (const auto &block : acBlocks) {
    auto groups = extractACGroups(block, zeroGroupIndices);
    for (const auto &group : groups) {
      for (int coeff : group) {
        if (coeff != 0) {
          allAC.push_back(coeff);
        }
      }
    }
  }

  int n = allAC.size();
  if (n == 0) {
    std::cerr << "Warning: No non-zero AC coefficients found.\n";
    return;
  }

  if (logisticKeyStream.size() < n) {
    std::cerr << "Error: Logistic keystream too short.\n";
    return;
  }

  std::vector<int> encryptedAC(n);
  int sign_c_prev = 0;
  int mag_c_prev = 0;

  for (int i = 0; i < n; ++i) {
    int val = allAC[i];
    int sign = (val < 0) ? 1 : 0;
    int abs_val = std::abs(val);

    if (abs_val == 1) {
      int64_t key_bit = extractSignificantDigits(logisticKeyStream[i], 1) & 1;
      int sign_c = key_bit ^ sign_c_prev ^ sign;
      sign_c_prev = sign_c;
      encryptedAC[i] = (sign_c == 1) ? -1 : 1;
      mag_c_prev = 1;
      continue;
    }

    int bitLen = static_cast<int>(std::floor(std::log2(abs_val))) + 1;
    int high_bit = 1 << (bitLen - 1);
    int low_mask = high_bit - 1;

    int64_t sig =
        extractSignificantDigits(logisticKeyStream[i], std::max(1, bitLen));
    int key_bit = sig % 2;
    int key_mask = sig & low_mask;

    int sign_c = key_bit ^ sign_c_prev ^ sign;
    sign_c_prev = sign_c;

    int sum = (abs_val + i) & low_mask;
    int temp = key_mask ^ sum ^ mag_c_prev;
    int new_mag = (temp & low_mask) | high_bit;

    if (new_mag == 0) {
      std::cerr << "[WARNING] Zero magnitude at i=" << i << ", fixed to MSB.\n";
      new_mag = high_bit;
    }

    mag_c_prev = new_mag;
    encryptedAC[i] = (sign_c == 1) ? -new_mag : new_mag;
  }

  applyNonZeroAC(encryptedAC, isLuminance);
}

void Jpeg::reverseSubstituteACInterBlock(
    bool isLuminance, const std::vector<double> &logisticKeyStream) {

  auto acBlocks = extractAC(isLuminance);

  std::vector<int> allAC;
  std::vector<int> zeroGroupIndices;

  for (const auto &block : acBlocks) {
    auto groups = extractACGroups(block, zeroGroupIndices);
    for (const auto &group : groups) {
      for (int coeff : group) {
        if (coeff != 0) {
          allAC.push_back(coeff);
        }
      }
    }
  }

  int n = allAC.size();
  if (n == 0) {
    std::cerr << "Warning: No non-zero AC coefficients found.\n";
    return;
  }

  if (logisticKeyStream.size() < n) {
    std::cerr << "Error: Logistic keystream too short.\n";
    return;
  }

  std::vector<int> decryptedAC(n);
  int sign_c_prev = 0;
  int mag_c_prev = 0;

  for (int i = 0; i < n; ++i) {
    int val_c = allAC[i];
    int sign_c = (val_c < 0) ? 1 : 0;
    int abs_c = std::abs(val_c);

    if (abs_c == 1) {
      int64_t sig = extractSignificantDigits(logisticKeyStream[i], 1);
      int key_bit = sig % 2;
      int sign_p = key_bit ^ sign_c_prev ^ sign_c;
      sign_c_prev = sign_c;
      decryptedAC[i] = (sign_p == 1) ? -1 : 1;
      mag_c_prev = 1;
      continue;
    }

    int bitLen = static_cast<int>(std::floor(std::log2(abs_c))) + 1;
    int high_bit = 1 << (bitLen - 1);
    int low_mask = high_bit - 1;

    int64_t sig =
        extractSignificantDigits(logisticKeyStream[i], std::max(1, bitLen));
    int key_bit = sig % 2;
    int key_mask = sig & low_mask;

    int sign_p = key_bit ^ sign_c_prev ^ sign_c;
    sign_c_prev = sign_c;

    int cipher_low = abs_c & low_mask;
    int unmasked = (cipher_low ^ key_mask ^ mag_c_prev) - i;
    unmasked = (unmasked & low_mask) | high_bit;

    if (unmasked == 0) {
      std::cerr << "[WARNING] Decrypted zero mag at i=" << i
                << ", corrected to MSB.\n";
      unmasked = high_bit;
    }

    decryptedAC[i] = (sign_p == 1) ? -unmasked : unmasked;
    mag_c_prev = abs_c;
  }

  applyNonZeroAC(decryptedAC, isLuminance);
}

std::vector<int> Jpeg::generateACInterBlockPermutationKey(
    int numBlocks, int alpha, const std::vector<double> &logisticKS) {
  std::vector<int> permKey;

  for (int m = 0; m < numBlocks - 1; ++m) {
    double sm = std::fabs(logisticKS[m]);
    int64_t sig = extractSignificantDigits(sm, alpha);
    int offset = sig % (numBlocks - m);
    int km = m + offset;
    permKey.push_back(km); // Swap index for m
  }

  return permKey;
}
