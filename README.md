# hamarc

__hamarc__ is a C++23 command-line tool for archiving files using Hamming error-correcting codes.

## Usage

| Short     | Long            | Description                                         |
| --------- | --------------- | --------------------------------------------------- |
| `-c`      | `--create`      | Create an archive.                                  |
| `-l`      | `--list`        | List the files in an archive.                       |
| `-x`      | `--extract`     | Extract selected files (all files if not specified) |
| `-a`      | `--append`      | Append files to an archive.                         |
| `-d`      | `--delete`      | Delete selected files from an archive.              |
| `-A`      | `--concatenate` | Merge archives into one archive.                    |
| `-f PATH` | `--file=PATH`   | Path to the archive (required)                      |

Examples:

```sh
hamarc --create --file=archive.haf file1.txt file2.txt
hamarc --list --file=archive.haf
hamarc --extract --file=archive.haf file1.txt
hamarc --append --file=archive.haf file3.txt
hamarc --delete --file=archive.haf file2.txt
hamarc --concatenate --file=merged.haf archive1.haf archive2.haf
```

### Specifications

- Files are stored by basename.
- Extraction writes to the current directory.
- Extended Hamming (8, 4) encoding by default.

## Building and testing

Requirements:

- CMake 3.24 or later.
- A C++23 compiler.

Build the project:

```sh
cmake -S . -B build
cmake --build build
```

Run the tests:

```sh
ctest --test-dir build --output-on-failure
```
