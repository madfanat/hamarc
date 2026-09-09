#include <cstdlib>
#include <exception>
#include <iostream>

#include "archiver.h"
#include "parser.h"

int main(const int argc, char* argv[]) {
  try {
    ArgumentParser parser;
    if (!parser.Parse(argc, argv)) return EXIT_FAILURE;
    HammingArchiver archiver;

    switch (parser.mode) {
      case ArgumentParser::kCreate:
        archiver.Create(parser.archive, parser.files);
        break;
      case ArgumentParser::kList:
        archiver.List(parser.archive);
        break;
      case ArgumentParser::kExtract:
        archiver.Extract(parser.archive, parser.files);
        break;
      case ArgumentParser::kAppend:
        archiver.Add(parser.archive, parser.files);
        break;
      case ArgumentParser::kDelete:
        archiver.Delete(parser.archive, parser.files);
        break;
      case ArgumentParser::kConcatenate:
        archiver.Concatenate(parser.archive, parser.files);
        break;
      default:
        break;
    }
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
