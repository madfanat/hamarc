#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "hamming.h"

class HammingArchiver {
 public:
  HammingArchiver() : hamming_(4) {}

  void Create(const std::string& archive,
              const std::vector<std::string>& files) {
    auto output = OpenOutput(archive);

    for (const auto& file : files) {
      AddToStream(output, file);
    }
    output.close();
    std::cout << "The archive " << archive << " was created.\n";
  }

  void Add(const std::string& archive, const std::vector<std::string>& files) {
    auto out = OpenOutput(archive, std::ios::binary | std::ios::app);

    for (const auto& file : files) {
      AddToStream(out, file);
    }
    out.close();
    std::cout << files.size()
              << " files were successfully added to the archive " << archive
              << ".\n";
  }

  void List(const std::string& archive) {
    auto input = OpenInput(archive);

    std::cout << "Files in the archive " << archive << ":\n";
    while (input.peek() != EOF) {
      int length = Read<int>(input);
      if (length < 0) throw std::runtime_error("Invalid filename length.");
      std::string filename;
      filename.resize(length);
      for (int i = 0; i < length; ++i) filename[i] = ReadByte(input);
      auto size = Read<long long>(input);
      ValidatePayload(input, size);
      std::cout << " - " << filename << "\n";
      input.seekg(size * 2, std::ios::cur);
      if (!input) throw std::runtime_error("Failed to seek in the archive.");
    }
  }

  void Extract(const std::string& archive,
               const std::vector<std::string>& files) {
    auto input = OpenInput(archive);

    bool do_extract_all = files.empty();
    while (input.peek() != EOF) {
      int length = Read<int>(input);
      if (length < 0) throw std::runtime_error("Invalid filename length.");
      std::string name;
      name.resize(length);
      for (int i = 0; i < length; ++i) name[i] = ReadByte(input);
      auto size = Read<long long>(input);
      ValidatePayload(input, size);
      bool is_found = do_extract_all;
      if (!is_found) {
        for (const auto& file : files)
          if (file == name) is_found = true;
      }

      if (is_found) {
        auto output = OpenOutput(name);
        for (size_t i = 0; i < size; ++i) {
          output.put(ReadByte(input));
        }
        output.close();
        std::cout << " - " << name << " was successfully extracted\n";
      } else {
        input.seekg(size * 2, std::ios::cur);
        if (!input) throw std::runtime_error("Failed to seek in the archive.");
      }
    }
  }

  void Delete(const std::string& archive,
              const std::vector<std::string>& files) {
    auto input = OpenInput(archive);
    Temporary temporary(archive);
    const auto temporary_archive = temporary.path / "archive";
    auto output = OpenOutput(temporary_archive.string());

    while (input.peek() != EOF) {
      int length = Read<int>(input);
      if (length < 0) throw std::runtime_error("Invalid filename length.");
      std::string name;
      name.resize(length);
      for (int i = 0; i < length; ++i) name[i] = ReadByte(input);
      auto size = Read<long long>(input);
      ValidatePayload(input, size);
      bool do_delete = false;
      for (const auto& file : files)
        if (file == name) do_delete = true;

      if (do_delete) {
        input.seekg(size * 2, std::ios::cur);
        if (!input) throw std::runtime_error("Failed to seek in the archive.");
      } else {
        Write(output, length);
        for (char c : name) WriteByte(output, c);
        Write(output, size);

        std::vector<char> buffer(size * 2);
        if (!input.read(buffer.data(), buffer.size())) {
          throw std::runtime_error("Failed to read the archive payload.");
        }
        output.write(buffer.data(), buffer.size());
      }
    }
    input.clear();
    input.close();
    if (!input) throw std::runtime_error("Failed to close the archive input.");
    output.close();
    std::filesystem::rename(temporary_archive, archive);
    std::cout << "The archive " << archive << " was successfully updated.\n";
  }

  void Concatenate(const std::string& archive,
                   const std::vector<std::string>& files) {
    auto output = OpenOutput(archive);

    for (const auto& file : files) {
      auto in = OpenInput(file);
      char buffer[4096];
      while (in.read(buffer, sizeof(buffer)) || in.gcount()) {
        output.write(buffer, in.gcount());
      }
      if (!in.eof()) throw std::runtime_error("Failed to read " + file);
    }
    output.close();
    std::cout << "Successfully merged the archives into the archive " << archive
              << ".\n";
  }

 private:
  struct Temporary {
    std::filesystem::path path;

    explicit Temporary(const std::string& archive) {
      std::random_device random;
      for (int attempt = 0; attempt < 32; ++attempt) {
        auto candidate = std::filesystem::path(archive);
        candidate += ".tmp." + std::to_string(random());
        if (std::filesystem::create_directory(candidate)) {
          path = std::move(candidate);
          return;
        }
      }
      throw std::runtime_error("Cannot create a temporary archive directory.");
    }

    ~Temporary() {
      std::error_code error;
      std::filesystem::remove_all(path, error);
    }
  };

  static std::ifstream OpenInput(const std::string& file) {
    std::ifstream input(file, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open input: " + file);
    input.exceptions(std::ios::badbit);
    return input;
  }

  static std::ofstream OpenOutput(const std::string& file,
                                  std::ios::openmode mode = std::ios::binary) {
    std::ofstream output;
    output.open(file, mode);
    if (!output) throw std::runtime_error("Cannot open output: " + file);
    output.exceptions(std::ios::failbit | std::ios::badbit);
    return output;
  }

  static void ValidatePayload(std::ifstream& input, long long size) {
    if (size < 0 || size > std::numeric_limits<std::streamoff>::max() / 2) {
      throw std::runtime_error("Invalid archive payload size.");
    }
    const auto position = input.tellg();
    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    if (!input || position == std::streampos(-1) || end < position ||
        size * 2 > end - position) {
      throw std::runtime_error("The archive payload is truncated.");
    }
    input.seekg(position);
    if (!input) throw std::runtime_error("Failed to seek in the archive.");
  }

  void WriteByte(std::ofstream& output, const char data) {
    output.put(static_cast<char>(hamming_.Encode(data & 0x0F)));
    output.put(static_cast<char>(hamming_.Encode(data >> 4 & 0x0F)));
  }

  char ReadByte(std::ifstream& input) {
    char left, right;
    if (!input.get(left) || !input.get(right)) {
      throw std::runtime_error("The archive is truncated or unreadable.");
    }
    return static_cast<char>(
        hamming_.Decode(static_cast<unsigned char>(left)) & 0x0F |
        (hamming_.Decode(static_cast<unsigned char>(right)) & 0x0F) << 4);
  }

  template <typename T>
  void Write(std::ofstream& output, T value) {
    const auto pointer = reinterpret_cast<const char*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) {
      WriteByte(output, pointer[i]);
    }
  }

  template <typename T>
  T Read(std::ifstream& input) {
    T value;
    const auto pointer = reinterpret_cast<char*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) {
      pointer[i] = ReadByte(input);
    }
    return value;
  }

  void AddToStream(std::ofstream& output, const std::string& file) {
    auto input = OpenInput(file);
    const std::string name = std::filesystem::path(file).filename().string();
    int length = static_cast<int>(name.size());
    Write(output, length);

    for (char c : name) {
      WriteByte(output, c);
    }

    const size_t size = std::filesystem::file_size(file);
    Write(output, size);

    char buffer[4096];
    while (input.read(buffer, sizeof(buffer)) || input.gcount()) {
      const std::streamsize count = input.gcount();
      for (std::streamsize i = 0; i < count; ++i) {
        WriteByte(output, buffer[i]);
      }
      if (input.eof()) break;
    }
    if (!input.eof()) throw std::runtime_error("Failed to read " + file);
  }

  Hamming hamming_;
};
