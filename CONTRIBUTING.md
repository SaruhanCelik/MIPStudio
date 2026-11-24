# Contributing

Thanks for your interest in MIPStudio.

- Please open an issue describing a bug or proposed change before making large modifications.
- Include a minimal `.asm` reproduction for assembler or CPU behavior changes.
- Build using Qt 6, CMake, and a C++17 toolchain; see `README.md`.
- If your changes affect the assembler/CPU, add or adjust a focused test in `tests/` and run the regression suite with `MIPSTUDIO_BUILD_STARTUP_TESTS=ON`.
- Keep `build*/`, `.exe`, `.dll`, `release/`, generated logs, and local IDE configuration out of pull requests.
- For UI changes, add a screenshot to the pull request when practical.

**Before accepting external contributions, the maintainer must choose a project license** and decide how contributions will be licensed.
