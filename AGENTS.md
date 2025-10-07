# Agent Instructions for Luanti (C++17 Voxel Game Engine)

## Build & Test Commands
- **Build**: `cmake -B build -DCMAKE_BUILD_TYPE=Debug -DRUN_IN_PLACE=1 -DENABLE_LTO=0 && cmake --build build -j4` (configure first time, then just `cmake --build build -j4`)
- **Run tests**: `./bin/luanti --run-unittests` (after building with `BUILD_UNITTESTS=TRUE`, which is default)
- **Run single test**: `./bin/luanti --run-unittests --test-module TestModuleName` (e.g. `--test-module TestMap`)
- **Run benchmarks**: `./bin/luanti --run-benchmarks` (requires `BUILD_BENCHMARKS=TRUE`)
- **Build options**: See `doc/compiling/README.md` for CMake options like `BUILD_CLIENT`, `BUILD_SERVER`, `ENABLE_CURL`, etc.

## Code Style
- **Language**: C++17, GCC ≥7.5 or Clang ≥7.0.1
- **Indentation**: Tabs (size 4) for C++/Lua/headers. Spaces (size 4) for markdown. See `.editorconfig`
- **Headers**: Use `#pragma once` (not include guards)
- **License header**: All files start with `// Luanti` or `// Minetest` + `// SPDX-License-Identifier: LGPL-2.1-or-later` + copyright
- **Line endings**: LF (Unix style), UTF-8 encoding, insert final newline, trim trailing whitespace
- **Naming**: Follow existing patterns in similar files (check neighboring code)
- **Error handling**: Match existing patterns in the codebase
- **Comments**: Minimal; code should be self-documenting. Use comments for complex logic only
- **Testing**: Tests use Catch2 framework. Test files: `src/unittest/test_*.cpp`. Register with `TestManager::registerTestModule(this)`

## Important Rules
- **NO manual updates**: Don't run `updatepo.sh` or update translation files manually (done at release time)
- **NO settings updates**: Don't update `minetest.conf.example` or `settings_translation_file.cpp` manually
- **Follow existing code**: Check imports, libraries, and patterns in neighboring files before adding dependencies
- **Clang-tidy**: Enabled checks include `modernize-use-emplace`, `performance-*`. See `.clang-tidy`
- **Code style**: Follow http://dev.minetest.net/Code_style_guidelines for C++
