# MIPStudio startup splash

## Architecture reviewed

The application uses C++17 and Qt 6 Widgets, with manually constructed widgets
rather than Qt Quick or Designer forms. The inspected local kit is Qt 6.11.2,
MinGW 13.1, 64-bit. No Qt or compiler upgrade is part of this patch.

- [CMakeLists.txt](../CMakeLists.txt) builds the shared emulator core, the CLI
  debugger, and the desktop GUI.
- [CPU.cpp](../src/CPU.cpp), [ALU.cpp](../src/ALU.cpp),
  [ControlUnit.cpp](../src/ControlUnit.cpp), and [Memory.cpp](../src/Memory.cpp)
  implement MIPS32 execution and 64 KiB of big-endian byte-addressable memory.
- [Assembler.cpp](../src/Assembler.cpp) is the two-pass assembler, including
  labels, pseudo-instruction expansion, data directives, and diagnostics.
- [EmulatorSession.cpp](../src/EmulatorSession.cpp) provides GUI execution,
  breakpoints, stepping, reset, and the instruction limit. The independent CLI
  entry point is [main.cpp](../src/main.cpp), with [Debugger.cpp](../src/Debugger.cpp).
- [MainWindow.cpp](../src/gui/MainWindow.cpp) owns editing, assembly, execution,
  dock panels, disassembly, files, and saved workspace state.
  [CodeEditor.cpp](../src/gui/CodeEditor.cpp),
  [MipsSyntaxHighlighter.cpp](../src/gui/MipsSyntaxHighlighter.cpp), and
  [SourceMap.cpp](../src/gui/SourceMap.cpp) supply editor/debugger integration.
- [AppSettings.cpp](../src/gui/AppSettings.cpp) and
  [SettingsDialog.cpp](../src/gui/SettingsDialog.cpp) manage preferences and
  themes; [InstructionReferenceDialog.cpp](../src/gui/InstructionReferenceDialog.cpp)
  supplies the searchable instruction reference.
- Existing [tests](../tests) and assembly fixtures in
  [Assembly_Codes](../Assembly_Codes) exercise the assembler and execution core.

The emulator, assembler, instruction semantics, CLI debugger, and settings
storage keys are unchanged. The GUI entry point is
[gui_main.cpp](../src/gui_main.cpp), not the CLI entry point.
Normal application settings still use native Windows storage. Tests explicitly
use an isolated INI directory for preferences and saved layouts.

## Patch behavior and files

[SplashScreen.h](../include/gui/SplashScreen.h) and
[SplashScreen.cpp](../src/gui/SplashScreen.cpp) implement an original painted
640 × 420 splash in logical pixels. Qt handles display scaling; unusually small
screens proportionally reduce the window to fit. It centers on the primary
screen's usable area. The navy/graphite background, electric-blue circuit traces,
neutral logo inset, and spinner are drawn with QPainter. No MATLAB assets or
new image-generation dependencies are used.

The supplied [logo](../assets/mips_studio_logo.png) retains its aspect ratio and
uses its existing `:/branding/mips_studio_logo.png` resource alias from
[resources.qrc](../resources.qrc). The resource file also embeds the Windows ICO
for the application/taskbar icon.
CMake now explicitly enables AUTORCC so the logo is compiled into the executable.
The executable icon reference in [windows.rc](../resources/windows.rc) is
preserved. Its [ICO asset](../assets/mips_studio.ico) is now a valid Windows icon
containing seven sizes (16, 24, 32, 48, 64, 128, and 256 pixels), generated from
the supplied artwork by [Generate-Icon.ps1](../tools/Generate-Icon.ps1).
The previous file had a PNG signature despite its `.ico` extension.

The splash starts its monotonic timer at its first paint. Only then does it
request initialization. A precise QTimer and a readiness flag jointly gate one
transition after at least 4,500 ms. The spinner indicates activity, without a
percentage. Clicking the splash does not dismiss it. Closing it with Alt+F4
cancels startup and exits.

[MainWindow.h](../include/gui/MainWindow.h) and
[MainWindow.cpp](../src/gui/MainWindow.cpp) add deferred initialization without
changing the existing synchronous constructor default. Small GUI construction
stages yield to the event loop. Optional command-line file reading and source
mapping run through Qt Concurrent; widget operations stay on the GUI thread.
The main window and restored floating docks stay suppressed until transition.
Failures show an error dialog and exit with a nonzero code when it is closed.

A per-user QLockFile suppresses repeated launches while startup is underway.
It is released at transition, so later deliberate launches still work. This is
a startup guard, not a persistent single-instance application protocol; an
assembly-file argument on a suppressed duplicate launch is not forwarded.

The output executable is now named `MIPStudio.exe` rather than `MIPS-Studio.exe`.
Existing shortcuts or packaging scripts referring to the old name must be updated.
Old executables already present in the project are not rebuilt automatically.

## Version and release metadata

- Set the actual application version in `project(... VERSION ...)` in
  [CMakeLists.txt](../CMakeLists.txt). It is currently **0.4.0**; this patch does
  not announce an unapproved 1.0.0 release. The splash, QApplication metadata,
  and About version display use that value.
- Set the `MIPSTUDIO_RELEASE_YEAR` CMake cache value to the intended release year.
  A fresh configuration defaults to the current UTC year, currently **2026**.
  An existing configured build retains its cache value. Pin it explicitly for
  reproducible release builds.
- [BuildInfo.h.in](../include/gui/BuildInfo.h.in) generates the shared metadata
  header inside each build directory. Reconfigure after changing these values.
- The platform/bitness label comes from the compiled platform and Qt's word size.
  Marmara University is explicitly labeled as an affiliation.

## Build in Qt Creator

1. Open [CMakeLists.txt](../CMakeLists.txt). Choose the existing **Desktop Qt
   6.11.2 MinGW 64-bit** kit, or your already supported Qt 6 Widgets kit.
2. Choose a **fresh build directory** for this checkout. The preexisting build
   caches refer to `C:/Users/htr/Saru/MIPS_Emulator`, which is a different source
   location. Do not reuse those caches at the current path.
3. Select Debug, configure, and build the `mips_gui` target. Run the resulting
   application. Repeat with a separate Release configuration/build directory.
4. In Projects → Build → CMake, check the project version and set
   `MIPSTUDIO_RELEASE_YEAR=2026` (or the actual release year).

Qt **Widgets** and **Concurrent** are required. Both modules are available in the
inspected local Qt installation. There are no missing logo assets. Qt **Test**
is required only when the optional tests are enabled.

In VS Code, select the same Qt/MinGW kit, configure a fresh build directory,
then use **CMake: Build**. Use the CMake Tools build/test integration for
automated builds and CTest execution.

[CMakePresets.json](../CMakePresets.json) provides reproducible `debug` and
`release` configurations for the installed kit. Both the C++ compiler and the
Windows resource compiler are pinned to `C:/Qt/Tools/mingw1310_64/bin`.
For another installation, override `MIPSTUDIO_QT_ROOT` and
`MIPSTUDIO_QT_TOOLS` in a local inherited preset, and adjust the compiler
subdirectory if its version differs. Do not combine this Qt build with the
unrelated `C:/MinGW` compiler/resource tools.

## Regression tests

Set `MIPSTUDIO_BUILD_STARTUP_TESTS=ON` in the IDE's CMake configuration and
reconfigure. Build all targets and run the discovered CTest tests through the
IDE/CMake Tools test integration. The `startup` test uses Qt's offscreen platform;
the existing core tests use the source directory for their assembly fixtures.
On Windows, the offscreen test environment loads fonts from the system Fonts
directory. Text and JUnit XML startup reports are written into the Debug build
directory. CTest still reports the actual process exit status.

[StartupTest.cpp](../tests/StartupTest.cpp) checks:

- resource loading, frameless flags, and primary-screen placement;
- the minimum timer, changing animation frames, and event-loop heartbeats;
- readiness arriving both before and after the minimum time;
- click resistance, cancellation, and duplicate readiness/initialization calls;
- deferred source-file loading (including a Unicode filename) and load failure;
- restored floating docks staying suppressed until the transition;
- the startup lock excluding a second concurrent owner.

For visual review, optionally set `MIPSTUDIO_SPLASH_PREVIEW` to an absolute PNG
output filename in the test environment; the startup test saves a rendered frame.
Repeat the startup test with `QT_SCALE_FACTOR=2` to inspect high-DPI rendering.
These are test-runner environment settings, not production startup switches.

## Package and manually verify Windows Release

Copy the newly built Release `MIPStudio.exe` into a clean distribution folder.
Run the **matching Qt kit's** `windeployqt.exe` with `--release --compiler-runtime`
and the copied executable as its target. Preserve all generated plugins and
runtime DLLs. Qt Concurrent is linked, but the deployed DLL list is determined
from the executable's actual imports. Do not copy a Debug Qt DLL
into a Release package. The logo requires no loose PNG at runtime.

Before signing or distributing, perform these checks on the actual package:

1. Launch from Explorer and from an unrelated working directory. Confirm the
   original logo, all credits, version/year, and original executable icon.
2. Time launch: the first painted splash must remain for at least about 4.5 s;
   the main window must not appear behind it. The spinner must keep moving.
3. Double-click the executable repeatedly during startup: one startup proceeds.
4. Launch with an existing assembly-file argument, including spaces and Unicode
   in its path. Confirm the source loads and remains unmodified.
5. Launch with an unreadable/nonexistent assembly path. Confirm an error appears,
   the main window stays hidden, and closing the error exits cleanly. Relaunch
   successfully afterward to verify lock cleanup.
6. Close the splash with Alt+F4, then relaunch. No orphan main window should remain.
7. Save a layout with floating docks, restart, and verify they appear only at the
   main-window transition. Verify the saved layout and preferences survive.
8. Repeat at 100%, 150%, and 200% scaling and with a secondary display attached.
9. Assemble/run/step/reset a supplied sample; check registers, memory, output,
   breakpoints, preferences, file saving, and ordinary application shutdown.
10. Run on a clean Windows 64-bit machine without Qt installed and confirm all
    runtime dependencies are included.

## Verified results — 29 October 2026 (Istanbul)

| Check | Actual result |
| --- | --- |
| Debug configuration and build | Passed; `CMAKE_BUILD_TYPE=Debug`, startup tests enabled |
| CTest, full Debug suite | **12/12 passed**, 0 failures, 14.45 seconds |
| Startup suite at 200% scaling | **Passed**, 0 failures |
| Release configuration and build | Passed; `CMAKE_BUILD_TYPE=Release`, 64-bit PE executable |
| Windows resource compiler output | Confirmed `pe-x86-64`; correct executable icon linked |
| Runtime deployment | Passed with the matching `windeployqt` |
| Packaged Release launch | Passed with Qt and MinGW removed from child `PATH` |
| Splash minimum lifetime | **4,501 ms**, measured by the running Release application after first-frame rendering |
| Native splash-to-main observation | 4,568 ms; splash observed before the main window |
| Shutdown | Main window closed normally; process exit code **0** |
| Visual review | Native Release splash/main-window captures and offscreen 100%/200% previews inspected |

The executable is [release/MIPStudio/MIPStudio.exe](../release/MIPStudio/MIPStudio.exe).
Keep that entire directory together; it contains the required Qt plugins and
compiler runtimes. The application version remains **0.4.0**.
The distributable [MIPStudio-0.4.0-win64.zip](../release/MIPStudio-0.4.0-win64.zip)
contains the executable and its dependencies. Extract it into its own folder
before launching the application.

### Diagnosed failures and fixes

1. **Environment/IDE integration:** stale build caches referenced another source
   checkout. CMake Tools also raised a File API directory `EEXIST` error; after
   the IDE reload its agent tools reported `No client available`. The installed
   tools were used directly after that loss of the IDE client. The project does
   not require CMake Tools to build. Its current agent integration is still
   unverified; this does not block the tested standalone package.
2. **Toolchain selection:** the original PATH contained an unrelated 32-bit
   MinGW installation. Its `windres` generated an `i386` resource object that
   failed to link into the 64-bit application. Presets and workspace settings
   now explicitly select Qt's matching GCC **and windres**. The original failed
   link is retained in [build-debug-initial.log](../verification/build-debug-initial.log).
3. **Project asset:** the old `.ico` was a PNG with the wrong extension. A real
   seven-resolution ICO was generated from the supplied logo. The original RC
   reference is preserved, and the ICO is also embedded for taskbar/window use.
4. **Startup/tests:** first-frame font discovery previously consumed part of the
   splash timer. The timer now starts after painting completes. The offscreen
   test loads Windows fonts; saved-layout fixtures are laid out before capture,
   and settings are isolated from normal user preferences. Circuit paths now
   use no fill brush. Equivalent hexadecimal test constants remove GCC's
   misleading exponentiation warnings without changing emulator expectations.

Qt **6.11.2**, GNU MinGW **13.1.0**, and the target
`x86_64-w64-mingw32` were confirmed. Widgets, Concurrent, and Test are installed.
No missing compiler or Qt component needs to be installed on this machine.

### Commands actually executed

From the project root, using the installed kit:

```powershell
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --preset debug
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --build --preset debug
& 'C:/Qt/Tools/CMake_64/bin/ctest.exe' --preset debug --output-on-failure --output-log verification/ctest-debug.log
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --preset release
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --build --preset release
```

The 200% check used `QT_SCALE_FACTOR=2` and `MIPSTUDIO_SPLASH_PREVIEW` pointing
to [splash-200.png](../verification/splash-200.png), then:

```powershell
& 'C:/Qt/Tools/CMake_64/bin/ctest.exe' --preset debug -R '^startup$' --output-on-failure --output-log verification/ctest-startup-200.log
```

Packaging and the real Windows startup check:

```powershell
New-Item -ItemType Directory -Force -Path release/MIPStudio
Copy-Item build-verified-release/MIPStudio.exe release/MIPStudio/MIPStudio.exe
$env:PATH = 'C:/Qt/Tools/mingw1310_64/bin;C:/Qt/6.11.2/mingw_64/bin;' + $env:PATH
& 'C:/Qt/6.11.2/mingw_64/bin/windeployqt.exe' --release --compiler-runtime --dir release/MIPStudio release/MIPStudio/MIPStudio.exe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./tools/Verify-Startup.ps1 -Executable ./release/MIPStudio/MIPStudio.exe -EvidenceDirectory ./verification
```

[Verify-Startup.ps1](../tools/Verify-Startup.ps1) launches only the specified test
instance, observes its own native windows, captures them, and closes it normally.
It uses a working directory outside the package and a child PATH containing only
Windows system directories. Opt-in `mipstudio.startup` logging supplies the
application's monotonic lifetime measurement; normal launches remain quiet.

### Evidence and remaining limits

- [Debug configure output](../verification/configure-debug.log),
  [Debug build output](../verification/build-debug.log), and
  [full CTest results](../verification/ctest-debug.log).
- [Startup text results](../verification/startup-results-100.txt),
  [startup JUnit XML](../verification/startup-results-100.xml), and
  [200% CTest results](../verification/ctest-startup-200.log).
- [Release configure output](../verification/configure-release.log),
  [Release build output](../verification/build-release.log), and
  [deployment output](../verification/deploy-release.log).
- [Native startup result](../verification/release-startup-result.json),
  [application startup log](../verification/release-startup.log),
  [Release splash capture](../verification/release-splash.png), and
  [Release main-window capture](../verification/release-main-window.png).

There is no remaining build/test blocker on the tested machine. The optional
Vulkan-header lookup and `windeployqt`'s missing `dxcompiler.dll`/`dxil.dll`
warning do not affect this application's tested Widgets startup; Direct3D 12
features are not used here. Offscreen-only raise/size-hint warnings are recorded
in the test report, with all assertions passing.

A separate clean Windows machine/VM, a secondary-monitor layout, and an explicit
150% run have not been tested. The isolated-PATH native run verifies deployment
on this host, not every Windows installation. Code signing and installer creation
are outside this splash/build patch.

## Help dialog fixes — 29 October 2026

The instruction reference now explicitly styles its list, rich-text details,
and selections for both Dark and Light profiles. This fixes pale text being
drawn over a native white background in Dark mode. Instruction content and
filtering are unchanged.

Help → Keyboard Shortcuts is now connected to its handler. Its resizable table
reads shortcuts from the live menu actions, including Save As, rather than
maintaining a second list of bindings. Existing keyboard bindings are unchanged.

Changes are in [MainWindow.cpp](../src/gui/MainWindow.cpp),
[InstructionReferenceDialog.cpp](../src/gui/InstructionReferenceDialog.cpp),
and [StartupTest.cpp](../tests/StartupTest.cpp). No new dependency or CMake source
registration was needed. The application remains version **0.4.0**.

Verification actually executed with the existing Qt 6.11.2 / MinGW 13.1 kit:

- Debug build passed: [build output](../verification/build-help-debug.log).
- Full CTest run passed, **12/12 suites**:
  [CTest output](../verification/ctest-help-debug.log). The QtTest suite now has
  13 passing cases including setup/cleanup, with four new Dark/Light Help cases.
  These verify text/selection contrast of at least 4.5:1, instruction searching
  and empty results, the real menu action opening the shortcut dialog, and
  every displayed binding matching its action. One binding is deliberately
  changed only within the isolated test to prove that the table uses live data.
- Captures were reviewed in both themes:
  [Dark instructions](../verification/help/instructions-dark.png),
  [Light instructions](../verification/help/instructions-light.png),
  [Dark shortcuts](../verification/help/shortcuts-dark.png), and
  [Light shortcuts](../verification/help/shortcuts-light.png).
- Release configuration and build passed:
  [configure output](../verification/configure-help-release.log),
  [build output](../verification/build-help-release.log).
- The packaged executable and ZIP were refreshed. Its native startup check
  passed with **4501 ms** splash visibility, automatic main-window transition,
  an embedded icon resource, and exit code **0**, with Qt/MinGW removed from the
  child PATH: [result](../verification/help-release/release-startup-result.json).

The VS Code CMake Tools agent commands returned an empty target list and failed
to start builds/tests, while reporting no CMake diagnostics. The installed
CMake and CTest executables were used to complete verification after those
integration failures. This is an IDE integration issue, not a failed source
build or test.

Commands executed from the project root:

```powershell
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --preset debug
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --build --preset debug
$env:MIPSTUDIO_HELP_PREVIEW_DIR = "$PWD/verification/help"
& 'C:/Qt/Tools/CMake_64/bin/ctest.exe' --preset debug --output-on-failure --output-log verification/ctest-help-debug.log
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --preset release
& 'C:/Qt/Tools/CMake_64/bin/cmake.exe' --build --preset release
Copy-Item build-verified-release/MIPStudio.exe release/MIPStudio/MIPStudio.exe -Force
Compress-Archive -Path release/MIPStudio/* -DestinationPath release/MIPStudio-0.4.0-win64.zip -Force
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./tools/Verify-Startup.ps1 -Executable ./release/MIPStudio/MIPStudio.exe -EvidenceDirectory ./verification/help-release
```

Help interactions were tested through Qt's offscreen backend; the native
packaged check covers startup and shutdown. Existing clean-machine and
multi-monitor verification limits above still apply.
