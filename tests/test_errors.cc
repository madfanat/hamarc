#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "archiver.h"

namespace {
namespace fs = std::filesystem;

class ArchiveErrors : public testing::Test {
 protected:
  fs::path work;
  HammingArchiver archiver;

  void SetUp() override {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    work = fs::temp_directory_path() / ("hamarc_errors_" + std::to_string(now));
    ASSERT_TRUE(fs::create_directory(work));
  }

  void TearDown() override {
    std::error_code error;
    fs::remove_all(work, error);
  }

  static std::string Read(const fs::path& file) {
    std::ifstream input(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
  }

  fs::path CreateArchive() {
    const auto source = work / "source";
    std::ofstream(source, std::ios::binary) << "payload";
    const auto archive = work / "test.haf";
    archiver.Create(archive.string(), {source.string()});
    return archive;
  }

  void ExpectNoTemporaryFiles() {
    for (const auto& entry : fs::directory_iterator(work)) {
      EXPECT_FALSE(entry.path().filename().string().starts_with("test.haf.tmp."));
    }
  }

  int Run(const std::string& arguments) {
    // Single quotes prevent shell expansion in POSIX paths.
    const auto quote = [](const std::string& value) {
#ifdef _WIN32
      return "\"" + value + "\"";
#else
      std::string result = "'";
      for (char c : value) result += c == '\'' ? "'\\''" : std::string(1, c);
      return result + "'";
#endif
    };
    const auto command = quote(HAMARC_EXE_PATH) + " -f " +
        quote((work / "cli.haf").string()) + " " + arguments;
    const int status = std::system(command.c_str());
#ifdef _WIN32
    return status;
#else
    if (status == -1 || !WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
#endif
  }
};

TEST_F(ArchiveErrors, DeleteIgnoresExistingLegacyTemporaryDirectory) {
  const auto archive = CreateArchive();
  const auto original = Read(archive);
  ASSERT_TRUE(fs::create_directory(archive.string() + ".tmp"));
  EXPECT_NO_THROW(archiver.Delete(archive.string(), {"absent"}));
  EXPECT_EQ(Read(archive), original);
  EXPECT_TRUE(fs::is_directory(archive.string() + ".tmp"));
  ExpectNoTemporaryFiles();
}

TEST_F(ArchiveErrors, DeleteRemovesSelectedEntry) {
  const auto archive = CreateArchive();
  EXPECT_NO_THROW(archiver.Delete(archive.string(), {"source"}));
  EXPECT_EQ(fs::file_size(archive), 0);
  ExpectNoTemporaryFiles();
}

TEST_F(ArchiveErrors, DeletePreservesTruncatedArchive) {
  const auto archive = CreateArchive();
  fs::resize_file(archive, fs::file_size(archive) - 1);
  const auto original = Read(archive);
  EXPECT_THROW(archiver.Delete(archive.string(), {}), std::exception);
  EXPECT_EQ(Read(archive), original);
  EXPECT_THROW(archiver.Delete(archive.string(), {"source"}), std::exception);
  EXPECT_EQ(Read(archive), original);
  ExpectNoTemporaryFiles();
}

TEST_F(ArchiveErrors, DeleteCleansUpAfterDecodingFailure) {
  const auto archive = CreateArchive();
  auto damaged = Read(archive);
  damaged[0] ^= 3;
  std::ofstream(archive, std::ios::binary) << damaged;
  EXPECT_THROW(archiver.Delete(archive.string(), {}), std::exception);
  EXPECT_EQ(Read(archive), damaged);
  ExpectNoTemporaryFiles();
}

TEST_F(ArchiveErrors, MissingInputIsAnErrorForEveryOperation) {
  const auto missing = (work / "missing").string();
  const auto output = (work / "output").string();
  EXPECT_THROW(archiver.Create(output, {missing}), std::exception);
  EXPECT_THROW(archiver.Add(output, {missing}), std::exception);
  EXPECT_THROW(archiver.List(missing), std::exception);
  EXPECT_THROW(archiver.Extract(missing, {}), std::exception);
  EXPECT_THROW(archiver.Delete(missing, {}), std::exception);
  EXPECT_THROW(archiver.Concatenate(output, {missing}), std::exception);
  EXPECT_FALSE(fs::exists(missing));
}

TEST_F(ArchiveErrors, UnwritableOutputIsAnError) {
  const auto output = (work / "missing" / "output").string();
  EXPECT_THROW(archiver.Create(output, {}), std::exception);
  EXPECT_THROW(archiver.Add(output, {}), std::exception);
  EXPECT_THROW(archiver.Concatenate(output, {}), std::exception);
}

TEST_F(ArchiveErrors, EmptyConcatenationInputIsValid) {
  const auto empty = work / "empty.haf";
  std::ofstream{empty};
  const auto output = work / "output.haf";
  EXPECT_NO_THROW(archiver.Concatenate(output.string(), {empty.string()}));
  EXPECT_EQ(fs::file_size(output), 0);
}

TEST_F(ArchiveErrors, ParseFailureDoesNotCreateOutput) {
  EXPECT_EQ(Run("-c -f"), EXIT_FAILURE);
  EXPECT_FALSE(fs::exists(work / "cli.haf"));
}

TEST_F(ArchiveErrors, OperationFailureReturnsFailureExitCode) {
  EXPECT_EQ(Run("-l"), EXIT_FAILURE);
}
}  // namespace
