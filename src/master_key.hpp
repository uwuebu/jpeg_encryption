#pragma once
#include "chaotic_keystream_generator.hpp" // Replace .cpp with .hpp
#include <fstream>
#include <iomanip> // For setting precision
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

  // Save as simple text with full precision
  void saveToFile(const std::string &filename) const {
    std::ofstream out(filename);
    if (!out) {
      throw std::runtime_error("Failed to open file for saving key.");
    }
    out << std::setprecision(17) << std::fixed; // Full precision for doubles
    out << logistic_x0 << " " << logistic_r << "\n";
    out << jia_x0 << " " << jia_y0 << " " << jia_z0 << " " << jia_w0 << "\n";
    out << alpha << " " << burn_in << "\n";
  }

  // Load from simple text with full precision
  void loadFromFile(const std::string &filename) {
    std::ifstream in(filename);
    if (!in) {
      throw std::runtime_error("Failed to open file for loading key.");
    }
    in >> logistic_x0 >> logistic_r;
    in >> jia_x0 >> jia_y0 >> jia_z0 >> jia_w0;
    in >> alpha >> burn_in;
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

  // Generate Arnold 3D keystream with parameters from paper
  std::vector<double> generateArnoldKeystream(int length) const {
    // Use first three Jia values as seeds, scale to integer in [0, 255]
    int x0 = static_cast<int>(jia_x0 * 1000) % 256;
    int y0 = static_cast<int>(jia_y0 * 1000) % 256;
    int z0 = static_cast<int>(jia_z0 * 1000) % 256;

    // Use optimized values from the paper (Table 2)
    const int a = 2, b = 1, c = 1, d = 1;
    const int modN = 256;

    return Arnold3DKeystreamGenerator::generateKeystream(
        length, burn_in,
        a, b, c, d,
        modN,
        x0, y0, z0
    );
  }
};

} // namespace ChaoticSystems
