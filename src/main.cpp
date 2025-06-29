#include "chaotic_keystream_generator.hpp"
#include "jpeg.hpp"
#include <chrono> // Include for timing
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdio.h>
#include <thread> // For parallel processing

namespace fs = std::filesystem;

// ==============================
// === MASTER KEY GENERATION ===
// ==============================

// Generates a new chaotic key with randomized seeds for all parameters
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

  return key;
}

int main() {
  // ======================
  // === SETUP PATHS ======
  // ======================
  fs::path exeDir = fs::current_path();
  fs::path rawDir = exeDir / ".." / ".." / "images" / "raw";
  fs::path editedDir = exeDir / ".." / ".." / "images" / "edited";
  fs::path restoreDir = exeDir / ".." / ".." / "images" / "restored";
  fs::path keyFile = exeDir / "master_key.txt";

  fs::create_directories(editedDir);
  fs::create_directories(restoreDir);

  // ===========================
  // === LOAD OR GENERATE KEY ==
  // ===========================
  ChaoticSystems::MasterKey key;
  if (fs::exists(keyFile)) {
    std::cout << "[INFO] Loaded master key from: " << keyFile << "\n";
    key.loadFromFile(keyFile.string());
  } else {
    std::cout << "[INFO] Generating new master key: " << keyFile << "\n";
    key = generateRandomMasterKey();
    key.saveToFile(keyFile.string());
  }

  // ===========================
  // === PROCESS IMAGE BATCH ===
  // ===========================
  for (auto &entry : fs::directory_iterator(rawDir)) {
    if (!entry.is_regular_file())
      continue;

    fs::path inFile = entry.path();
    fs::path editFile = editedDir / inFile.filename();
    fs::path restoreFile = restoreDir / inFile.filename();

    Jpeg img;
    if (!img.load(inFile.wstring())) {
      std::wcerr << L"Failed to load " << inFile.wstring() << L"\n";
      continue;
    }

    // ===========================================
    // === ENCRYPTION LOOP (3 rounds of chaos) ===
    // ===========================================
    for (int round = 0; round < 1; ++round) {
      std::cout << "[INFO] Encryption Round " << round + 1 << "\n";

      // === Run encryption operations in parallel
      std::thread lumaDCThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img.processDCWithKey(true, img.generateDCPermutationKeystream(img.extractDC(true).size(), key));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Luminance Permutation Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img.applyDC(img.substituteDC(img.extractDC(true), key.generateLogisticKeystream(img.extractDC(true).size()), key.alpha), true);
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Luminance Substitution Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      std::thread chromaDCThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img.processDCWithKey(false, img.generateDCPermutationKeystream(img.extractDC(false).size(), key));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Chrominance Permutation Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img.applyDC(img.substituteDC(img.extractDC(false), key.generateLogisticKeystream(img.extractDC(false).size()), key.alpha), false);
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Chrominance Substitution Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      std::thread lumaACThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img.permuteACBlocks(true, img.generateACInterBlockPermutationKey(img.extractAC(true).size(), key.alpha, key.generateLogisticKeystream(img.extractAC(true).size() - 1)));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Luminance Inter-block Permutation Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img.processACIntraBlock(true, img.generateACPermutationKeys(true, key));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Luminance Intra-block Permutation Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img.substituteACInterBlock(true, key.generateLogisticKeystream(img.extractAC(true).size()));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Luminance Substitution Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      std::thread chromaACThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img.permuteACBlocks(false, img.generateACInterBlockPermutationKey(img.extractAC(false).size(), key.alpha, key.generateLogisticKeystream(img.extractAC(false).size() - 1)));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Chrominance Inter-block Permutation Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img.processACIntraBlock(false, img.generateACPermutationKeys(false, key));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Chrominance Intra-block Permutation Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img.substituteACInterBlock(false, key.generateLogisticKeystream(img.extractAC(false).size()));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Chrominance Substitution Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      lumaDCThread.join();
      chromaDCThread.join();
      lumaACThread.join();
      chromaACThread.join();
    }

    // === Save the encrypted JPEG image
    if (!img.save(editFile.wstring(), 100)) {
      std::wcerr << L"Failed to save edited " << editFile.wstring() << L"\n";
      continue;
    }

    // ==================================
    // === BEGIN DECRYPTION PHASE =======
    // ==================================
    Jpeg img2;
    if (!img2.load(editFile.wstring())) {
      std::wcerr << L"Failed to reload edited " << editFile.wstring() << L"\n";
      continue;
    }

    // === Run decryption in 3 reverse rounds
    for (int round = 0; round < 1; ++round) {
      std::cout << "[INFO] Decryption Round " << round + 1 << "\n";

      std::thread lumaDCDecryptThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img2.applyDC(img2.decryptDC(img2.extractDC(true), key.generateLogisticKeystream(img2.extractDC(true).size()), key.alpha), true);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Luminance Substitution Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img2.processDCReverse(true, img2.generateDCPermutationKeystream(img2.extractDC(true).size(), key));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Luminance Permutation Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      std::thread chromaDCDecryptThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img2.applyDC(img2.decryptDC(img2.extractDC(false), key.generateLogisticKeystream(img2.extractDC(false).size()), key.alpha), false);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Chrominance Substitution Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img2.processDCReverse(false, img2.generateDCPermutationKeystream(img2.extractDC(false).size(), key));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] DC Chrominance Permutation Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      std::thread lumaACDecryptThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img2.reverseSubstituteACInterBlock(true, key.generateLogisticKeystream(img2.extractAC(true).size()));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Luminance Substitution Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img2.processACIntraBlock(true, img2.generateACPermutationKeys(true, key), true);
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Luminance Intra-block Permutation Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img2.reversePermuteACBlocks(true, img2.generateACInterBlockPermutationKey(img2.extractAC(true).size(), key.alpha, key.generateLogisticKeystream(img2.extractAC(true).size() - 1)));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Luminance Inter-block Permutation Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      std::thread chromaACDecryptThread([&]() {
        auto start = std::chrono::high_resolution_clock::now();
        img2.reverseSubstituteACInterBlock(false, key.generateLogisticKeystream(img2.extractAC(false).size()));
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Chrominance Substitution Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img2.processACIntraBlock(false, img2.generateACPermutationKeys(false, key), true);
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Chrominance Intra-block Permutation Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";

        start = std::chrono::high_resolution_clock::now();
        img2.reversePermuteACBlocks(false, img2.generateACInterBlockPermutationKey(img2.extractAC(false).size(), key.alpha, key.generateLogisticKeystream(img2.extractAC(false).size() - 1)));
        end = std::chrono::high_resolution_clock::now();
        std::cout << "[INFO] AC Chrominance Inter-block Permutation Reverse Time: "
                  << std::chrono::duration<double>(end - start).count() << " seconds\n";
      });

      lumaDCDecryptThread.join();
      chromaDCDecryptThread.join();
      lumaACDecryptThread.join();
      chromaACDecryptThread.join();
    }

    // === Save restored image (after full decryption)
    if (!img2.save(restoreFile.wstring(), 100)) {
      std::wcerr << L"Failed to save restored " << restoreFile.wstring()
                 << L"\n";
      continue;
    }
  }

  return 0;
}
