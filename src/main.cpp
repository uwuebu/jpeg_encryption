#include <stdio.h> // Ensure FILE is defined
#include "jpeg.hpp"
#include <filesystem>
#include <iostream>

int main() {
  namespace fs = std::filesystem;
  // locate folders relative to executable
  fs::path exeDir = fs::current_path();
  fs::path rawDir = exeDir / ".." / ".." / "images" / "raw";
  fs::path editedDir = exeDir / ".." / ".." / "images" / "edited";
  fs::path restoreDir = exeDir / ".." / ".." / "images" / "restored";

  // ensure output folders exist
  fs::create_directories(editedDir);
  fs::create_directories(restoreDir);

  // process every file in rawDir
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

    // 1) apply DCT manipulation and save edited
    img.processCoeffs();
    if (!img.save(editFile.wstring(), 100)) {
      std::wcerr << L"Failed to save edited " << editFile.wstring() << L"\n";
      continue;
    }

    // 2) reload edited, reverse manipulation, save restored
    Jpeg img2;
    if (!img2.load(editFile.wstring())) {
      std::wcerr << L"Failed to reload edited " << editFile.wstring() << L"\n";
      continue;
    }
    img2.processCoeffs(); // reversal of prior change
    if (!img2.save(restoreFile.wstring(), 100)) {
      std::wcerr << L"Failed to save restored " << restoreFile.wstring()
                 << L"\n";
      continue;
    }
  }

  return 0;
}