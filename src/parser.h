#pragma once
#include <iostream>
#include <string>
#include <vector>

class ArgumentParser {
 public:
  bool Parse(const int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
      std::string argument = argv[i];
      if (argument == "-c" || argument == "--create") {
        mode = kCreate;
      } else if (argument == "-l" || argument == "--list") {
        mode = kList;
      } else if (argument == "-x" || argument == "--extract") {
        mode = kExtract;
      } else if (argument == "-a" || argument == "--append") {
        mode = kAppend;
      } else if (argument == "-d" || argument == "--delete") {
        mode = kDelete;
      } else if (argument == "-A" || argument == "--concatenate") {
        mode = kConcatenate;
      } else if (argument.rfind("--file=", 0) == 0) {
        archive = argument.substr(7);
      } else if (argument == "-f" || argument == "--file") {
        if (i + 1 < argc) {
          archive = argv[++i];
        } else {
          std::cerr << "The archive name isn't provided.\n";
          return false;
        }
      } else {
        files.push_back(argument);
      }
    }
    if (archive.empty()) {
      std::cerr << "The archive name isn't provided.\n";
      return false;
    }
    if (mode == kDefault) {
      std::cerr << "The mode isn't provided.\n";
      return false;
    }
    return true;
  }

  enum Mode {
    kDefault,
    kCreate,
    kList,
    kExtract,
    kAppend,
    kDelete,
    kConcatenate
  };

  Mode mode = kDefault;
  std::string archive;
  std::vector<std::string> files;
};