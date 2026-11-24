
cmake_minimum_required(VERSION 3.16)

# ============================================================
# PROJECT
# ============================================================

project(
    MIPSStudio
    VERSION 0.4.0
    LANGUAGES CXX
)

# Release metadata
string(TIMESTAMP MIPSTUDIO_CURRENT_YEAR "%Y" UTC)
set(MIPSTUDIO_RELEASE_YEAR "${MIPSTUDIO_CURRENT_YEAR}" CACHE STRING "MIPStudio release/copyright year")

if(NOT MIPSTUDIO_RELEASE_YEAR MATCHES "^[0-9][0-9][0-9][0-9]$")
    message(FATAL_ERROR "MIPSTUDIO_RELEASE_YEAR must contain a four-digit year")
endif()

configure_file(
    include/gui/BuildInfo.h.in
    generated/gui/BuildInfo.h
    @ONLY
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# QT
# ============================================================

find_package(
    Qt6
    REQUIRED
    COMPONENTS Widgets Concurrent
)

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

# ============================================================
# EMULATOR CORE
# ============================================================

add_library(
    mips_core
    src/CPU.cpp
    src/ALU.cpp
    src/ControlUnit.cpp
    src/Memory.cpp
    src/Assembler.cpp
    src/EmulatorSession.cpp
)

target_include_directories(
    mips_core
    PUBLIC
    include
)

# ============================================================
# CLI DEBUGGER
# ============================================================

add_executable(
    mips_cli
    src/main.cpp
    src/Debugger.cpp
)

target_link_libraries(
    mips_cli
    PRIVATE
    mips_core
)

# ============================================================
# QT GUI
# ============================================================

set(MIPSTUDIO_WIDGET_SOURCES
    src/gui/MainWindow.cpp
    src/gui/CodeEditor.cpp
    src/gui/MipsSyntaxHighlighter.cpp
    src/gui/AppSettings.cpp
    src/gui/SettingsDialog.cpp
    src/gui/SourceMap.cpp
    src/gui/InstructionReferenceDialog.cpp
    src/gui/SplashScreen.cpp

    include/gui/MainWindow.h
    include/gui/CodeEditor.h
    include/gui/MipsSyntaxHighlighter.h
    include/gui/AppSettings.h
    include/gui/SettingsDialog.h
    include/gui/SourceMap.h
    include/gui/InstructionReferenceDialog.h
    include/gui/SplashScreen.h
    include/gui/StartupDiagnostics.h
)

# Windows-specific resources must not be compiled on macOS/Linux.
set(MIPSTUDIO_PLATFORM_RESOURCES)

if(WIN32)
    list(APPEND MIPSTUDIO_PLATFORM_RESOURCES
        resources/windows.rc
    )
endif()

qt_add_executable(
    mips_gui
    src/gui_main.cpp
    ${MIPSTUDIO_WIDGET_SOURCES}
    resources.qrc
    ${MIPSTUDIO_PLATFORM_RESOURCES}
)

target_include_directories(
    mips_gui
    PRIVATE
    include
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(
    mips_gui
    PRIVATE
    mips_core
    Qt6::Widgets
    Qt6::Concurrent
)

# ============================================================
# DESKTOP APPLICATION
# ============================================================

set_target_properties(
    mips_gui
    PROPERTIES
    OUTPUT_NAME "MIPStudio"
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# ============================================================
# TESTING
# ============================================================

option(
    MIPSTUDIO_BUILD_STARTUP_TESTS
    "Build startup and existing emulator regression tests"
    OFF
)

if(MIPSTUDIO_BUILD_STARTUP_TESTS)

    enable_testing()

    find_package(
        Qt6
        REQUIRED
        COMPONENTS Test
    )

    qt_add_executable(
        mipstudio_startup_tests
        tests/StartupTest.cpp
        ${MIPSTUDIO_WIDGET_SOURCES}
        resources.qrc
    )

    target_include_directories(
        mipstudio_startup_tests
        PRIVATE
        include
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )

    target_link_libraries(
        mipstudio_startup_tests
        PRIVATE
        mips_core
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Test
    )

    add_test(
        NAME startup
        COMMAND mipstudio_startup_tests
            -o "${CMAKE_CURRENT_BINARY_DIR}/startup-results.txt,txt"
            -o "${CMAKE_CURRENT_BINARY_DIR}/startup-results.xml,junitxml"
    )

    set(
        startup_test_environment
        "QT_QPA_PLATFORM=offscreen"
    )

    if(WIN32)
        file(
            TO_CMAKE_PATH
            "$ENV{SystemRoot}/Fonts"
            startup_test_font_directory
        )

        if(IS_DIRECTORY "${startup_test_font_directory}")
            list(
                APPEND startup_test_environment
                "QT_QPA_FONTDIR=${startup_test_font_directory}"
            )
        endif()
    endif()

    set_tests_properties(
        startup
        PROPERTIES
        ENVIRONMENT "${startup_test_environment}"
        TIMEOUT 45
    )

    foreach(
        test_name
        AsciizTest
        AssemblerErrorTest
        AssemblerTest
        DataSectionTest
        IntegrationTest
        LabelAfterLiTest
        LiExpansionTest
        PrintStringTest
        PseudoInstructionTest
        UltimateIntegrationTest
        ultimateTest
    )

        add_executable(
            ${test_name}
            tests/${test_name}.cpp
        )

        target_link_libraries(
            ${test_name}
            PRIVATE
            mips_core
        )

        add_test(
            NAME ${test_name}
            COMMAND ${test_name}
        )

        set_tests_properties(
            ${test_name}
            PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            TIMEOUT 30
        )

    endforeach()

endif()
