#pragma once
#include <vector>

namespace ChaoticSystems {

// State structure for Jia chaotic map
struct State {
  double x, y, z, w;
};

// Jia Keystream Generator
class JiaKeystreamGenerator {
public:
  // Generates a keystream using the Jia chaotic map
  static std::vector<double> generateKeystream(int length, int burn_in,
                                               double step, double x0,
                                               double y0, double z0, double w0);

private:
  // Performs a single Runge-Kutta 4th order step
  static State rk4Step(const State &s, double h, double a, double b, double c, double d, double e);

  // Computes the derivatives for the Jia chaotic map
  static State jiaDeriv(const State &s, double a, double b, double c, double d, double e);
};

// Logistic Keystream Generator
class LogisticKeystreamGenerator {
public:
  // Generates a keystream using the logistic map
  static std::vector<double>
  generateKeystream(int length,
                    double x0, // Initial value ∈ (0, 1)
                    double r,  // Control parameter (chaotic when close to 4)
                    int burnIn, double epsilon = 1e-14);
};

class Arnold3DKeystreamGenerator {
public:
  static std::vector<double> generateKeystream(
      int steps, int burn_in,
      int a, int b, int c, int d,
      int modN, int x0, int y0, int z0);
};


} // namespace ChaoticSystems
