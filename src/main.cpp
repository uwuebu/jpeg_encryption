#include "chaotic_keystream_generator.hpp"
#include "jpeg.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <random>
#include <stdio.h>
#include <thread> // Include for threading

namespace fs = std::filesystem;

// Generates a new MasterKey with random seeds
ChaoticSystems::MasterKey generateRandomMasterKey() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(0.01, 0.99);
  std::uniform_int_distribution<int> intDist(1000, 999999);

  ChaoticSystems::MasterKey key;
  key.logistic_x0 = dist(gen);
  key.logistic_r = 3.9 + dist(gen) * 0.1;
  key.jia_x0 = dist(gen);
  key.jia_y0 = dist(gen);
  key.jia_z0 = dist(gen);
  key.jia_w0 = dist(gen);
  key.alpha = 15;
  key.burn_in = 200;
  key.ac_block_seed = intDist(gen);
  key.intra_block_seed = intDist(gen);

  return key;
}

int main() {
  fs::path exeDir = fs::current_path();
  fs::path rawDir = exeDir / ".." / ".." / "images" / "raw";
  fs::path editedDir = exeDir / ".." / ".." / "images" / "edited";
  fs::path restoreDir = exeDir / ".." / ".." / "images" / "restored";
  fs::path keyFile = exeDir / "master_key.txt";

  fs::create_directories(editedDir);
  fs::create_directories(restoreDir);

  // Load or create master key
  ChaoticSystems::MasterKey key;
  if (fs::exists(keyFile)) {
    std::cout << "[INFO] Loaded master key from: " << keyFile << "\n";
    key.loadFromFile(keyFile.string());
  } else {
    std::cout << "[INFO] Generating new master key: " << keyFile << "\n";
    key = generateRandomMasterKey();
    key.saveToFile(keyFile.string());
  }

  for (auto &entry : fs::directory_iterator(rawDir)) {
    if (!entry.is_regular_file()) continue;

    fs::path inFile = entry.path();
    fs::path editFile = editedDir / inFile.filename();
    fs::path restoreFile = restoreDir / inFile.filename();

    Jpeg img;
    if (!img.load(inFile.wstring())) {
      std::wcerr << L"Failed to load " << inFile.wstring() << L"\n";
      continue;
    }

    // Declare variables to be used in both encryption and decryption phases
    std::vector<int> luminanceInterKey, chrominanceInterKey;
    std::vector<double> luminanceLogisticKeystream, chrominanceLogisticKeystream;
    std::vector<int> luminanceACInterKey, chrominanceACInterKey;
    std::vector<std::vector<int>> luminanceIntraKeys, chrominanceIntraKeys;
    std::vector<double> luminanceACKeystream, chrominanceACKeystream;

    // Threads for parallel encryption
    std::thread luminanceDCThread([&]() {
      int lenLuminanceDC = img.extractDC(true).size();
      luminanceInterKey = img.generateDCPermutationKeystream(lenLuminanceDC, key);
      luminanceLogisticKeystream = key.generateLogisticKeystream(lenLuminanceDC);

      img.processDCWithKey(true, luminanceInterKey);
      auto luminanceDC = img.extractDC(true);
      auto substitutedLuminanceDC = img.substituteDC(luminanceDC, luminanceLogisticKeystream, key.alpha);
      img.applyDC(substitutedLuminanceDC, true);
    });

    std::thread chrominanceDCThread([&]() {
      int lenChrominanceDC = img.extractDC(false).size();
      chrominanceInterKey = img.generateDCPermutationKeystream(lenChrominanceDC, key);
      chrominanceLogisticKeystream = key.generateLogisticKeystream(lenChrominanceDC);

      img.processDCWithKey(false, chrominanceInterKey);
      auto chrominanceDC = img.extractDC(false);
      auto substitutedChrominanceDC = img.substituteDC(chrominanceDC, chrominanceLogisticKeystream, key.alpha);
      img.applyDC(substitutedChrominanceDC, false);
    });

    std::thread luminanceACThread([&]() {
      int luminanceACBlockCount = img.extractAC(true).size();
      luminanceACInterKey = key.generatePermutation(luminanceACBlockCount, key.ac_block_seed);
      img.permuteACBlocks(true, luminanceACInterKey);

      luminanceIntraKeys = img.generateACPermutationKeys(true, key);
      img.processACIntraBlock(true, luminanceIntraKeys);

      int luminanceNonZeroACCount = 0;
      for (const auto &block : img.extractAC(true))
        for (int coeff : block)
          if (coeff != 0) ++luminanceNonZeroACCount;

      luminanceACKeystream = key.generateLogisticKeystream(luminanceNonZeroACCount);
      img.substituteACInterBlock(true, luminanceACKeystream);
    });

    std::thread chrominanceACThread([&]() {
      int chrominanceACBlockCount = img.extractAC(false).size();
      chrominanceACInterKey = key.generatePermutation(chrominanceACBlockCount, key.ac_block_seed + 1);
      img.permuteACBlocks(false, chrominanceACInterKey);

      chrominanceIntraKeys = img.generateACPermutationKeys(false, key);
      img.processACIntraBlock(false, chrominanceIntraKeys);

      int chrominanceNonZeroACCount = 0;
      for (const auto &block : img.extractAC(false))
        for (int coeff : block)
          if (coeff != 0) ++chrominanceNonZeroACCount;

      chrominanceACKeystream = key.generateLogisticKeystream(chrominanceNonZeroACCount);
      img.substituteACInterBlock(false, chrominanceACKeystream);
    });

    // Wait for all encryption threads to finish
    luminanceDCThread.join();
    chrominanceDCThread.join();
    luminanceACThread.join();
    chrominanceACThread.join();

    // Save encrypted image
    if (!img.save(editFile.wstring(), 100)) {
      std::wcerr << L"Failed to save edited " << editFile.wstring() << L"\n";
      continue;
    }

    // ===== DECRYPTION PHASE =====
    Jpeg img2;
    if (!img2.load(editFile.wstring())) {
      std::wcerr << L"Failed to reload edited " << editFile.wstring() << L"\n";
      continue;
    }

    // Threads for parallel decryption
    std::thread luminanceDCDecryptThread([&]() {
      auto decryptedLuminanceDC = img2.decryptDC(img2.extractDC(true), luminanceLogisticKeystream, key.alpha);
      img2.applyDC(decryptedLuminanceDC, true);
      img2.processDCReverse(true, luminanceInterKey);
    });

    std::thread chrominanceDCDecryptThread([&]() {
      auto decryptedChrominanceDC = img2.decryptDC(img2.extractDC(false), chrominanceLogisticKeystream, key.alpha);
      img2.applyDC(decryptedChrominanceDC, false);
      img2.processDCReverse(false, chrominanceInterKey);
    });

    std::thread luminanceACDecryptThread([&]() {
      img2.reverseSubstituteACInterBlock(true, luminanceACKeystream);
      img2.processACIntraBlock(true, luminanceIntraKeys, true);
      img2.reversePermuteACBlocks(true, luminanceACInterKey);
    });

    std::thread chrominanceACDecryptThread([&]() {
      img2.reverseSubstituteACInterBlock(false, chrominanceACKeystream);
      img2.processACIntraBlock(false, chrominanceIntraKeys, true);
      img2.reversePermuteACBlocks(false, chrominanceACInterKey);
    });

    // Wait for all decryption threads to finish
    luminanceDCDecryptThread.join();
    chrominanceDCDecryptThread.join();
    luminanceACDecryptThread.join();
    chrominanceACDecryptThread.join();

    // Save restored image
    if (!img2.save(restoreFile.wstring(), 100)) {
      std::wcerr << L"Failed to save restored " << restoreFile.wstring() << L"\n";
      continue;
    }
  }

  return 0;
}
