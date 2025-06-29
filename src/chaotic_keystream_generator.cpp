#include "chaotic_keystream_generator.hpp"
#include <cmath>
#include <random>
#include <vector>

namespace ChaoticSystems {

std::vector<double> Arnold3DKeystreamGenerator::generateKeystream(
    int steps, int burn_in,
    int a, int b, int c, int d,
    int modN, int x0, int y0, int z0) {

  std::vector<double> keystream;
  int x = x0, y = y0, z = z0;

  // Burn-in phase
  for (int i = 0; i < burn_in; ++i) {
    int x_new = (x + a * z) % modN;
    int y_new = (b * c * x + y + a * b * c * z + c * z) % modN;
    int z_new = (b * c * d * x + b * d + d * y + a * b * c * d * z + a * b * z + c * d * z + z) % modN;

    x = x_new;
    y = y_new;
    z = z_new;
  }

  for (int i = 0; i < steps; ++i) {
    int x_new = (x + a * z) % modN;
    int y_new = (b * c * x + y + a * b * c * z + c * z) % modN;
    int z_new = (b * c * d * x + b * d + d * y + a * b * c * d * z + a * b * z + c * d * z + z) % modN;

    x = x_new;
    y = y_new;
    z = z_new;

    // Normalize to [0,1) and convert to double
    keystream.push_back(static_cast<double>(x) / modN);
    keystream.push_back(static_cast<double>(y) / modN);
    keystream.push_back(static_cast<double>(z) / modN);
  }

  return keystream;
}

// Implementation of JiaKeystreamGenerator
std::vector<double>
JiaKeystreamGenerator::generateKeystream(int steps, int burn_in, double dt,
                                         double x0, double y0, double z0,
                                         double w0) {
  const double a = 10.0, b = 8.0 / 3.0, c = 28.0, d = 1.3, e = 1.0;
  State current = {x0, y0, z0, w0};
  std::vector<double> keystream;

  // Burn-in phase
  for (int i = 0; i < burn_in; ++i)
    current = rk4Step(current, dt, a, b, c, d, e);

  // Generate keystream
  int generatedValues = 0;
  while (generatedValues < steps) {
    current = rk4Step(current, dt, a, b, c, d, e);
    keystream.push_back(std::fabs(current.x));
    ++generatedValues;

    if (generatedValues < steps) {
      keystream.push_back(std::fabs(current.y));
      ++generatedValues;
    }

    if (generatedValues < steps) {
      keystream.push_back(std::fabs(current.z));
      ++generatedValues;
    }

    if (generatedValues < steps) {
      keystream.push_back(std::fabs(current.w));
      ++generatedValues;
    }
  }

  return keystream;
}

State JiaKeystreamGenerator::rk4Step(const State &s, double h, double a,
                                     double b, double c, double d, double e) {
  State k1 = jiaDeriv(s, a, b, c, d, e);

  State s2 = {s.x + 0.5 * h * k1.x, s.y + 0.5 * h * k1.y, s.z + 0.5 * h * k1.z,
              s.w + 0.5 * h * k1.w};
  State k2 = jiaDeriv(s2, a, b, c, d, e);

  State s3 = {s.x + 0.5 * h * k2.x, s.y + 0.5 * h * k2.y, s.z + 0.5 * h * k2.z,
              s.w + 0.5 * h * k2.w};
  State k3 = jiaDeriv(s3, a, b, c, d, e);

  State s4 = {s.x + h * k3.x, s.y + h * k3.y, s.z + h * k3.z, s.w + h * k3.w};
  State k4 = jiaDeriv(s4, a, b, c, d, e);

  return {s.x + (h / 6) * (k1.x + 2 * k2.x + 2 * k3.x + k4.x),
          s.y + (h / 6) * (k1.y + 2 * k2.y + 2 * k3.y + k4.y),
          s.z + (h / 6) * (k1.z + 2 * k2.z + 2 * k3.z + k4.z),
          s.w + (h / 6) * (k1.w + 2 * k2.w + 2 * k3.w + k4.w)};
}

State JiaKeystreamGenerator::jiaDeriv(const State &s, double a, double b,
                                      double c, double d, double e) {
  return {-a * (s.x - s.y) + s.w, -s.x * s.z + c * s.y - s.x,
          s.x * s.y - b * s.z, -d * s.x + e * s.y};
}

// Implementation of LogisticKeystreamGenerator
std::vector<double>
LogisticKeystreamGenerator::generateKeystream(int length, double x0, double r,
                                              int burn_in, double epsilon) {
  std::vector<double> keystream;
  double x = x0;

  // Burn-in phase
  for (int i = 0; i < burn_in; ++i) {
    x = r * x * (1 - x);
    if (x == 0.5 || x == 0.75)
      x += epsilon;
  }

  // Generate logistic sequence
  for (int i = 0; i < length; ++i) {
    x = r * x * (1 - x);
    if (x == 0.5 || x == 0.75)
      x += epsilon;
    keystream.push_back(x);
  }

  return keystream;
}

} // namespace ChaoticSystems