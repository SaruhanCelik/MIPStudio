# Publishing MIPStudio for Windows

The uploaded source was configured as **MIPStudio v0.4.0**. Its existing deployed Windows application is in `release/MIPStudio/`, and the source ZIP already contained `release/MIPStudio-0.4.0-win64.zip`.

## What is included in the binary release?

The archive must contain `MIPStudio.exe` **plus** deployed runtime DLLs and plugin directories (`platforms/`, `imageformats/`, etc.). An `.exe` on its own is not a portable Qt distribution.

## Release checklist

1. Build with the release Qt 6 / MinGW kit (`cmake --preset release` then `cmake --build --preset release`, when matching the saved paths).
2. Deploy from that *same* Qt kit with `windeployqt`:

   ```powershell
   New-Item -ItemType Directory -Force release/MIPStudio | Out-Null
   Copy-Item build-verified-release/MIPStudio.exe release/MIPStudio/MIPStudio.exe -Force
   & 'C:/Qt/6.11.2/mingw_64/bin/windeployqt.exe' --release --compiler-runtime --dir release/MIPStudio release/MIPStudio/MIPStudio.exe
   ```

3. Launch `release/MIPStudio/MIPStudio.exe` and test an assembly program. Prefer a separate clean Windows machine or VM to detect missing DLLs.
4. Optional: run the included startup verification utility:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./tools/Verify-Startup.ps1 -Executable ./release/MIPStudio/MIPStudio.exe -EvidenceDirectory ./verification
   ```

5. Package **the contents** of `release/MIPStudio` (not its parent directory):

   ```powershell
   Compress-Archive -Path release/MIPStudio/* -DestinationPath release/MIPStudio-0.4.0-win64.zip -Force
   ```

6. On GitHub, choose **Releases → Draft a new release**. Create tag `v0.4.0` at the release commit; title it `MIPStudio v0.4.0`; attach `MIPStudio-0.4.0-win64.zip` as the downloadable release asset. Publish after checking license/Qt redistribution obligations.
7. Keep `release/` **ignored** in Git. Updating a release asset does not require committing the full distribution or DLLs to the repository.

## Suggested release notes

**MIPStudio v0.4.0 — Windows x64**

Desktop MIPS assembly development and simulation environment featuring a two-pass assembler, source editor, register/memory inspection, instruction stepping, breakpoints, built-in reference, settings, and a startup splash screen.

**Installation:** Download the ZIP, extract all files, and run `MIPStudio.exe`. Keep all DLLs and subfolders next to it.

**Notes:** Windows x64 only for this packaged build. The executable is not code-signed. Not a complete compatibility implementation of every MIPS32 instruction or operating-system service.

## Qt redistribution

A binary distribution includes Qt DLLs, Qt plugins, and MinGW runtime files. Check the applicable license terms for the Qt packages and components used, offer or provide required notices, and comply with LGPL/GPL/commercial licensing conditions as applicable. Choosing MIT (or another license) for your own source does **not** replace third-party dependency obligations.
