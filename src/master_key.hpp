#pragma once
#include "chaotic_keystream_generator.hpp" // Replace .cpp with .hpp
#include <fstream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace ChaoticSystems {

struct MasterKey {
  // Seeds and parameters
  double logistic_x0 = 0.678;
  double logistic_r = 4.0;
  double jia_x0 = 0.1, jia_y0 = 0.2, jia_z0 = 0.3, jia_w0 = 0.4;
  int alpha = 15;
  int burn_in = 200;
  int ac_block_seed = 54321;
  int intra_block_seed = 12345;

  // Save as simple text
  void saveToFile(const std::string &filename) const {
    std::ofstream out(filename);
    out << logistic_x0 << " " << logistic_r << "\n";
    out << jia_x0 << " " << jia_y0 << " " << jia_z0 << " " << jia_w0 << "\n";
    out << alpha << " " << burn_in << "\n";
    out << ac_block_seed << " " << intra_block_seed << "\n";
  }

  // Load from simple text
  void loadFromFile(const std::string &filename) {
    std::ifstream in(filename);
    in >> logistic_x0 >> logistic_r;
    in >> jia_x0 >> jia_y0 >> jia_z0 >> jia_w0;
    in >> alpha >> burn_in;
    in >> ac_block_seed >> intra_block_seed;
  }

  // Generate logistic keystream
  std::vector<double> generateLogisticKeystream(int length) const {
    // Assuming the missing argument is alpha, add it as the last parameter
    return LogisticKeystreamGenerator::generateKeystream(length, logistic_x0,
                                                         logistic_r, burn_in, alpha);
  }

  // Generate Jia keystream
  std::vector<double> generateJiaKeystream(int length) const {
    return JiaKeystreamGenerator::generateKeystream(
        length, burn_in, 0.001, jia_x0, jia_y0, jia_z0, jia_w0);
  }

  // Generate permutation using std::mt19937 (e.g., AC block order)
  std::vector<int> generatePermutation(int length, int seed) const {
    std::vector<int> perm(length);
    std::iota(perm.begin(), perm.end(), 0);
    std::mt19937 rng(seed);
    for (int i = 0; i < length - 1; ++i) {
      std::uniform_int_distribution<int> dist(i, length - 1);
      std::swap(perm[i], perm[dist(rng)]);
    }
    return perm;
  }
};

} // namespace ChaoticSystems
