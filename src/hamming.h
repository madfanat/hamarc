#pragma once

#include <stdexcept>

class Hamming {
 public:
  Hamming(const int parity_bits)
      : block_size_(1 << (parity_bits - 1)),
        data_bits_(block_size_ - parity_bits) {}

  int Encode(const int& data) const {
    int result = 0;
    int data_index = 0;
    int check = 0;

    for (int i = 1; i < block_size_; ++i) {
      if (i & i - 1) {
        result |= Bit(data, data_index) << i;
        ++data_index;
        check ^= Bit(result, i);
      }
    }

    for (int p = 1; p < block_size_; p <<= 1) {
      int parity = 0;
      for (int i = 1; i < block_size_; ++i) {
        if (i & p) {
          parity ^= Bit(result, i);
        }
      }
      result |= parity << p;
      check ^= parity;
    }

    result |= check;

    return result;
  }

  int Decode(const int& data) const {
    const int corrected = Correct(data);
    int result = 0;
    int data_index = 0;
    for (int i = 1; i < block_size_; ++i) {
      if (i & i - 1) {
        result |= Bit(corrected, i) << data_index;
        ++data_index;
      }
    }
    return result;
  }

 private:
  int block_size_;
  int data_bits_;
  static int Bit(const int& number, const int& index) {
    return number >> index & 1;
  }

  int Correct(const int& data) const {
    int syndrome = 0;
    int parity = 0;

    for (int i = 1; i < block_size_; ++i) {
      if (Bit(data, i)) {
        syndrome ^= i;
        parity ^= 1;
      }
    }

    if (!syndrome) {
      if (parity != Bit(data, 0)) {
        return data ^ 1;
      }
      return data;
    }

    if (parity == Bit(data, 0)) {
      throw std::runtime_error("The error can't be fixed.");
    }

    return data ^ 1 << syndrome;
  }
};
