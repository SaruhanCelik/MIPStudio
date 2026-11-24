#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include <array>
#include <cstdint>
#include <functional>

#include "EmulatorSession.h"
#include "gui/AppSettings.h"
#include "gui/SourceMap.h"


class QAction;
class QCloseEvent;
class QDockWidget;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;

class CodeEditor;
class MipsSyntaxHighlighter;


// ============================================================
// MainWindow
// ============================================================

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class InitializationMode { Immediate, Deferred };

    explicit MainWindow(
        const QString& initialProgram = QString(),
        QWidget* parent = nullptr,
        InitializationMode mode = InitializationMode::Immediate
    );

    void initializeAsync();
    void showAfterInitialization();

signals:
    void initializationStatus(const QString& status);
    void initialized();
    void initializationFailed(const QString& message);


protected:
    void closeEvent(
        QCloseEvent* event
    ) override;


private:
    QString startupProgram;
    int startupStep = -1;
    bool startupReady = false;
    bool startupShown = false;

    void initializeNextStep();
    void prepareUI();
    void finishUI();
    void restoreStartupLayout();
    void finishStartup();
    void applyFileContents(const QString& filename, const QString& contents,
                           const SourceMap* preparedSourceMap = nullptr);

    // ========================================================
    // BACKEND
    // ========================================================

    EmulatorSession session;


    // ========================================================
    // SETTINGS
    // ========================================================

    AppSettings settings;


    // ========================================================
    // SOURCE MAP
    // ========================================================

    SourceMap sourceMap;


    // ========================================================
    // DOCUMENT
    // ========================================================

    QString currentFile;

    bool documentModified = false;


    // ========================================================
    // EDITOR
    // ========================================================

    CodeEditor* editor = nullptr;

    MipsSyntaxHighlighter*
        syntaxHighlighter = nullptr;


    // ========================================================
    // TABLES
    // ========================================================

    QTableWidget*
        registerTable = nullptr;

    QTableWidget*
        disassemblyTable = nullptr;

    QTableWidget*
        memoryTable = nullptr;

    QTableWidget*
        symbolTable = nullptr;


    // ========================================================
    // CONSOLE
    // ========================================================

    QPlainTextEdit*
        outputView = nullptr;

    QPlainTextEdit*
        problemsView = nullptr;


    // ========================================================
    // CPU STATE
    // ========================================================

    QLabel* pcValue = nullptr;
    QLabel* hiValue = nullptr;
    QLabel* loValue = nullptr;
    QLabel* stateValue = nullptr;


    // ========================================================
    // MEMORY NAVIGATOR
    // ========================================================

    QLineEdit*
        memoryAddressEdit = nullptr;

    uint32_t memoryViewAddress =
        0x1000;


    // ========================================================
    // DOCKS
    // ========================================================

    QDockWidget* registersDock = nullptr;
    QDockWidget* cpuStateDock = nullptr;
    QDockWidget* disassemblyDock = nullptr;
    QDockWidget* memoryDock = nullptr;
    QDockWidget* consoleDock = nullptr;
    QDockWidget* symbolsDock = nullptr;


    // ========================================================
    // ACTIONS
    // ========================================================

    QAction* actionNew = nullptr;
    QAction* actionOpen = nullptr;
    QAction* actionSave = nullptr;
    QAction* actionSaveAs = nullptr;
    QAction* actionExit = nullptr;

    QAction* actionAssemble = nullptr;
    QAction* actionRun = nullptr;
    QAction* actionStep = nullptr;
    QAction* actionReset = nullptr;

    QAction* actionShowBreakpoints = nullptr;
    QAction* actionResetBreakpoints = nullptr;

    QAction* actionPreferences = nullptr;
    QAction* actionResetLayout = nullptr;

    QAction* actionHelp = nullptr;
    QAction* actionKeyboardShortcuts = nullptr;
    QAction* actionAbout = nullptr;


    // ========================================================
    // REGISTER CHANGE TRACKING
    // ========================================================

    std::array<uint32_t, 32>
        previousRegisters{};


    // ========================================================
    // BUILD UI
    // ========================================================

    void buildUI();

    void buildMenus();
    void buildToolbar();
    void buildEditor();

    void buildRegisterPanel();
    void buildCPUStatePanel();
    void buildDisassemblyPanel();
    void buildMemoryPanel();
    void buildConsolePanel();
    void buildSymbolsPanel();


    // ========================================================
    // SETTINGS
    // ========================================================

    void applySettings();

    void applyTheme();

    void openPreferences();

    void resetLayout();


    // ========================================================
    // WINDOW PERSISTENCE
    // ========================================================

    void loadWindowSettings();

    void saveWindowSettings();


    // ========================================================
    // FILE MANAGEMENT
    // ========================================================

    bool maybeSave();

    bool saveDocument(
        const QString& filename
    );

    void loadFile(
        const QString& filename
    );

    void updateWindowTitle();


    bool sameFile(
        const QString& first,
        const QString& second
    ) const;


    // ========================================================
    // SOURCE MAPPING
    // ========================================================

    void rebuildSourceMap();

    void synchronizeBreakpoints();

    void synchronizePCToSource();


    // ========================================================
    // DEBUGGER
    // ========================================================

    void showBreakpoints();

    void resetBreakpoints();


    // ========================================================
    // REFRESH
    // ========================================================

    void capturePreviousState();

    void refreshUI();

    void refreshRegisters();
    void refreshCPUState();
    void refreshDisassembly();
    void refreshMemory();
    void refreshSymbols();

    void updateActionStates();


    // ========================================================
    // MEMORY
    // ========================================================

    void jumpMemoryToAddress(
        uint32_t address
    );

    void jumpMemoryFromText();

    void jumpMemoryToStackPointer();


    // ========================================================
    // CONSOLE
    // ========================================================

    void clearConsole();


    // ========================================================
    // HELP
    // ========================================================

    void showHelp();

    void showKeyboardShortcuts();

    void showAbout();


    // ========================================================
    // DISASSEMBLER
    // ========================================================

    QString disassemble(
        uint32_t instruction,
        uint32_t address
    ) const;


    // ========================================================
    // OUTPUT
    // ========================================================

    void appendOutput(
        const QString& text
    );

    void appendProblem(
        const QString& text
    );


    QString captureConsoleDuring(
        const std::function<void()>& function
    );


private slots:
    // Files
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();


    // Execution
    void assembleCurrentDocument();
    void onStep();
    void onRun();
    void onReset();


    // Source
    void onDocumentModified();

    void onBreakpointToggled(
        int line,
        bool enabled
    );


    // Navigation
    void onSymbolActivated(
        int row,
        int column
    );

    void onDisassemblyActivated(
        int row,
        int column
    );
};

#endif
