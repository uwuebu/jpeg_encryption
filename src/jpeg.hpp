#pragma once
#include <stdio.h> // Ensure FILE is defined
#include <jpeglib.h>
#include <string>
#include <vector>
#include "master_key.hpp"

class Jpeg {
public:
  // load from disk; returns false on error
  bool load(const std::wstring &path);

  // process AC coefficients
  void processAC(bool isLuminance);

  // save back to disk with given quality
  bool save(const std::wstring &path, int quality = 90);

  // accessors
  int getWidth() const;
  int getHeight() const;
  int getComponents() const;

  // Extract DC coefficients into a 1D array
  std::vector<int> extractDC(bool isLuminance);

  // Apply modified DC coefficients from a 1D array
  void applyDC(const std::vector<int>& dcCoefficients, bool isLuminance);

  // Extract AC coefficients into a 2D array
  std::vector<std::vector<int>> extractAC(bool isLuminance);

  // Apply modified AC coefficients from a 2D array
  void applyAC(const std::vector<std::vector<int>>& acCoefficients, bool isLuminance);

  // Generate DC permutation keystream
  std::vector<int> generateDCPermutationKeystream(
    int lenDC, const ChaoticSystems::MasterKey &key);

  // Generate DC permutation key
  std::vector<int> generateDCPermutationKey(int lenDC, int alpha = 15);

  // Generate AC permutation keystream
  std::vector<std::vector<int>> generateACPermutationKeys(
    bool isLuminance, const ChaoticSystems::MasterKey &key);

  // Generate AC permutation key using Jia chaotic map
  std::vector<int> generateACPermutationKey(int groupCount);

  // Generate AC inter-block permutation key
  std::vector<int> generateACInterBlockPermutationKey(
    int numBlocks, int alpha, const std::vector<double>& jiaKS);

  // Helper function to extract significant digits
  uint64_t extractSignificantDigits(double value, int digits);

  // Process DC coefficients with a provided key
  void processDCWithKey(bool isLuminance, const std::vector<int>& key);

  // Process DC coefficients with a provided key in reverse order
  void processDCReverse(bool isLuminance, const std::vector<int>& key);

  // Substitute DC coefficients using logistic map keystream
  std::vector<int> substituteDC(const std::vector<int>& DC, const std::vector<double>& logisticKS, int alpha = 15);

  // Decrypt DC coefficients using logistic map keystream
  std::vector<int> decryptDC(const std::vector<int>& DC_encrypted, const std::vector<double>& logisticKS, int alpha = 15);

  // Permute AC blocks using a key
  void permuteACBlocks(bool isLuminance, const std::vector<int>& keys);

  // Reverse permute AC blocks using a key
  void reversePermuteACBlocks(bool isLuminance, const std::vector<int>& keys);

  // Extract AC groups from a block
  std::vector<std::vector<int>> extractACGroups(const std::vector<int>& AC_block, std::vector<int>& zeroGroupIndices);

  // Remove zero groups from AC groups
  std::vector<std::vector<int>> removeZeroGroups(const std::vector<std::vector<int>>& groups, const std::vector<int>& zeroGroupIndices);

  // Shuffle AC groups using a key
  void shuffleGroups(std::vector<std::vector<int>>& groups, const std::vector<int>& keys);

  // Unshuffle AC groups using a key
  void unshuffleGroups(std::vector<std::vector<int>>& groups, const std::vector<int>& keys);

  // Reinsert zero groups into AC groups
  void reinsertZeroGroups(std::vector<std::vector<int>>& groups, const std::vector<int>& zeroGroupIndices, const std::vector<std::vector<int>>& originalZeroGroups);

  // Flatten AC groups back into a single block
  std::vector<int> flattenGroups(const std::vector<std::vector<int>>& groups);

  // Calculate the number of non-zero groups for AC intrablock permutations
  int calculateLeadingZeroGroups(const std::vector<std::vector<int>>& AC_blocks);

  // Generate AC permutation keys for all AC blocks
  std::vector<std::vector<int>> generateACPermutationKeys(bool isLuminance);

  // Process AC intra-block shuffling (forward or reverse) with 2D keys
  void processACIntraBlock(bool isLuminance, const std::vector<std::vector<int>>& intraKeys, bool reverse = false);

  void applyNonZeroAC(const std::vector<int>& encryptedAC, bool isLuminance);

  // Substitute AC coefficients inter-block with a provided logistic keystream
  void substituteACInterBlock(bool isLuminance, const std::vector<double>& logisticKeyStream);

  // Reverse substitute AC coefficients inter-block with a provided logistic keystream
  void reverseSubstituteACInterBlock(bool isLuminance, const std::vector<double>& logisticKeyStream);

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