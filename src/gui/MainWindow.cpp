#include "gui/MainWindow.h"

#include "gui/CodeEditor.h"
#include "gui/MipsSyntaxHighlighter.h"
#include "gui/SettingsDialog.h"
#include "gui/InstructionReferenceDialog.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QDialog>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QTimer>
#include <QtConcurrentRun>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include <iostream>
#include <exception>
#include <sstream>


using namespace std;


// ============================================================
// SHARED DISPLAY HELPERS
// ============================================================

namespace {

const char* REGISTER_NAMES[32] = {
    "$zero", "$at",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3",
    "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3",
    "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9",
    "$k0", "$k1",
    "$gp", "$sp", "$fp", "$ra"
};


QString toHex32(uint32_t value)
{
    return QString("0x%1")
        .arg(
            value,
            8,
            16,
            QChar('0')
        )
        .toUpper();
}


QString toHexByte(uint8_t value)
{
    return QString("%1")
        .arg(
            value,
            2,
            16,
            QChar('0')
        )
        .toUpper();
}


// ------------------------------------------------------------
// Resolve "System" into the actual profile that should be used
// by editor/debugger colors.
// ------------------------------------------------------------

QString resolvedThemeProfile(
    const AppSettings& settings
)
{
    const QString theme =
        settings.theme().toLower();


    if (theme == "dark") {
        return "dark";
    }


    if (theme == "light") {
        return "light";
    }


    const QColor windowColor =
        QApplication::palette().color(
            QPalette::Window
        );


    return windowColor.lightness() < 128
        ? "dark"
        : "light";
}

}


// ============================================================
// CONSTRUCTOR
// ============================================================

MainWindow::MainWindow(
    const QString& initialProgram,
    QWidget* parent,
    InitializationMode mode
)
    : QMainWindow(parent), startupProgram(initialProgram)
{
    if (mode == InitializationMode::Deferred)
        return;

    buildUI();

    applySettings();

    loadWindowSettings();

    updateWindowTitle();

    updateActionStates();


    if (!initialProgram.isEmpty()) {
        loadFile(initialProgram);
    }
    startupReady = true;
}

void MainWindow::initializeAsync()
{
    if (startupStep != -1 || startupReady)
        return;
    startupStep = 0;
    QTimer::singleShot(0, this, &MainWindow::initializeNextStep);
}

void MainWindow::initializeNextStep()
{
    // Widgets must be built on the GUI thread. Each bounded stage yields to Qt
    // before the next one, allowing the splash to keep painting and animating.
    using Step = void (MainWindow::*)();
    static const Step steps[] = {
        &MainWindow::prepareUI,
        &MainWindow::buildMenus,
        &MainWindow::buildToolbar,
        &MainWindow::buildEditor,
        &MainWindow::buildRegisterPanel,
        &MainWindow::buildCPUStatePanel,
        &MainWindow::buildDisassemblyPanel,
        &MainWindow::buildMemoryPanel,
        &MainWindow::buildConsolePanel,
        &MainWindow::buildSymbolsPanel,
        &MainWindow::finishUI,
        &MainWindow::applySettings,
        &MainWindow::restoreStartupLayout,
        &MainWindow::finishStartup
    };

    try {
        emit initializationStatus(startupStep < 11
            ? tr("Preparing editor and debugger") : tr("Restoring workspace preferences"));
        (this->*steps[startupStep])();
        ++startupStep;
        if (startupStep < static_cast<int>(sizeof(steps) / sizeof(steps[0])))
            QTimer::singleShot(0, this, &MainWindow::initializeNextStep);
    } catch (const std::exception& error) {
        emit initializationFailed(tr("Could not initialize the workspace: %1")
                                      .arg(QString::fromLocal8Bit(error.what())));
    } catch (...) {
        emit initializationFailed(tr("Could not initialize the workspace."));
    }
}

void MainWindow::restoreStartupLayout()
{
    // Restored floating docks are top-level windows. Suppress native mapping
    // until the same transition that presents the main window.
    for (QDockWidget* dock : findChildren<QDockWidget*>())
        dock->setAttribute(Qt::WA_DontShowOnScreen);
    loadWindowSettings();
}

void MainWindow::finishStartup()
{
    updateWindowTitle();
    updateActionStates();
    if (startupProgram.isEmpty()) {
        startupReady = true;
        emit initialized();
        return;
    }

    emit initializationStatus(tr("Loading assembly source"));
    struct StartupFile {
        QString contents;
        QString error;
        SourceMap sourceMap;
    };

    // Only file I/O and source parsing run in the worker. It never accesses a
    // widget or this MainWindow, so cancellation/destruction cannot dangle it.
    auto* watcher = new QFutureWatcher<StartupFile>(this);
    connect(watcher, &QFutureWatcher<StartupFile>::finished, this, [this, watcher] {
        watcher->deleteLater();
        try {
            const StartupFile loaded = watcher->result();
            if (!loaded.error.isEmpty()) {
                emit initializationFailed(loaded.error);
                return;
            }
            applyFileContents(startupProgram, loaded.contents, &loaded.sourceMap);
            startupReady = true;
            emit initialized();
        } catch (const std::exception& error) {
            emit initializationFailed(tr("Could not load assembly source: %1")
                                          .arg(QString::fromLocal8Bit(error.what())));
        } catch (...) {
            emit initializationFailed(tr("Could not load assembly source."));
        }
    });
    watcher->setFuture(QtConcurrent::run([filename = startupProgram] {
        StartupFile loaded;
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            loaded.error = tr("Could not open:\n%1\n\n%2").arg(filename, file.errorString());
            return loaded;
        }
        QTextStream stream(&file);
        loaded.contents = stream.readAll();
        if (stream.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
            loaded.error = tr("Could not read assembly source:\n%1").arg(filename);
            return loaded;
        }
        loaded.sourceMap.rebuild(loaded.contents);
        return loaded;
    }));
}

void MainWindow::showAfterInitialization()
{
    if (!startupReady || startupShown)
        return;
    startupShown = true;
    QList<QDockWidget*> visibleFloatingDocks;
    for (QDockWidget* dock : findChildren<QDockWidget*>()) {
        if (dock->isFloating() && !dock->isHidden()) {
            visibleFloatingDocks.append(dock);
            dock->hide();
        }
        dock->setAttribute(Qt::WA_DontShowOnScreen, false);
    }
    show();
    for (QDockWidget* dock : visibleFloatingDocks)
        dock->show();
}


// ============================================================
// BUILD UI
// ============================================================

void MainWindow::buildUI()
{
    prepareUI();
    buildMenus();
    buildToolbar();
    buildEditor();
    buildRegisterPanel();
    buildCPUStatePanel();
    buildDisassemblyPanel();
    buildMemoryPanel();
    buildConsolePanel();
    buildSymbolsPanel();
    finishUI();
}

void MainWindow::prepareUI()
{
    memoryViewAddress = settings.defaultMemoryAddress();
    setObjectName(
        "MainWindow"
    );


    setMinimumSize(
        1050,
        700
    );


    resize(
        1500,
        920
    );


    setDockOptions(
        QMainWindow::AllowNestedDocks |
        QMainWindow::AllowTabbedDocks |
        QMainWindow::AnimatedDocks
    );


}

void MainWindow::finishUI()
{
    // ========================================================
    // VIEW MENU DOCK TOGGLES
    // ========================================================

    QMenu* viewMenu = nullptr;


    for (
        QAction* action :
        menuBar()->actions()
    ) {
        if (
            action->text() == "&View"
        ) {
            viewMenu =
                action->menu();

            break;
        }
    }


    if (viewMenu) {
        viewMenu->addSeparator();

        viewMenu->addAction(
            registersDock->toggleViewAction()
        );

        viewMenu->addAction(
            cpuStateDock->toggleViewAction()
        );

        viewMenu->addAction(
            symbolsDock->toggleViewAction()
        );

        viewMenu->addAction(
            disassemblyDock->toggleViewAction()
        );

        viewMenu->addAction(
            memoryDock->toggleViewAction()
        );

        viewMenu->addAction(
            consoleDock->toggleViewAction()
        );
    }


    resetLayout();


    statusBar()->showMessage(
        "Ready"
    );
}


// ============================================================
// MENUS
// ============================================================

void MainWindow::buildMenus()
{
    // ========================================================
    // FILE
    // ========================================================

    QMenu* fileMenu =
        menuBar()->addMenu(
            "&File"
        );


    actionNew =
        fileMenu->addAction(
            "&New"
        );

    actionNew->setShortcut(
        QKeySequence::New
    );


    actionOpen =
        fileMenu->addAction(
            "&Open..."
        );

    actionOpen->setShortcut(
        QKeySequence::Open
    );


    actionSave =
        fileMenu->addAction(
            "&Save"
        );

    actionSave->setShortcut(
        QKeySequence::Save
    );


    actionSaveAs =
        fileMenu->addAction(
            "Save &As..."
        );

    actionSaveAs->setShortcut(
        QKeySequence::SaveAs
    );


    fileMenu->addSeparator();


    actionExit =
        fileMenu->addAction(
            "E&xit"
        );


    // ========================================================
    // RUN
    // ========================================================

    QMenu* runMenu =
        menuBar()->addMenu(
            "&Run"
        );


    actionAssemble =
        runMenu->addAction(
            "&Assemble"
        );

    actionAssemble->setShortcut(
        QKeySequence("F5")
    );


    actionRun =
        runMenu->addAction(
            "&Run / Continue"
        );

    actionRun->setShortcut(
        QKeySequence("F6")
    );


    actionStep =
        runMenu->addAction(
            "&Step"
        );

    actionStep->setShortcut(
        QKeySequence("F7")
    );


    actionReset =
        runMenu->addAction(
            "&Reset Program"
        );

    actionReset->setShortcut(
        QKeySequence("Ctrl+R")
    );


    // --------------------------------------------------------
    // Breakpoints
    // --------------------------------------------------------

    runMenu->addSeparator();


    actionShowBreakpoints =
        runMenu->addAction(
            "&Breakpoints..."
        );


    actionShowBreakpoints->setShortcut(
        QKeySequence("Ctrl+B")
    );


    actionResetBreakpoints =
        runMenu->addAction(
            "Reset Breakpoints"
        );


    actionResetBreakpoints->setShortcut(
        QKeySequence("Ctrl+Shift+B")
    );


    // ========================================================
    // VIEW
    // ========================================================

    QMenu* viewMenu =
        menuBar()->addMenu(
            "&View"
        );


    actionResetLayout =
        viewMenu->addAction(
            "Reset Layout"
        );


    viewMenu->addSeparator();


    // ========================================================
    // SETTINGS
    // ========================================================

    QMenu* settingsMenu =
        menuBar()->addMenu(
            "&Settings"
        );


    actionPreferences =
        settingsMenu->addAction(
            "&Preferences..."
        );


    actionPreferences->setShortcut(
        QKeySequence("Ctrl+,")
    );


    // ========================================================
    // HELP
    // ========================================================

    QMenu* helpMenu =
        menuBar()->addMenu(
            "&Help"
        );


    actionHelp =
        helpMenu->addAction(
            "MIPS Studio Help"
        );


    actionHelp->setShortcut(
        QKeySequence("F1")
    );


    actionKeyboardShortcuts =
        helpMenu->addAction(
            "Keyboard Shortcuts"
        );


    QAction* instructionReferenceAction =
        helpMenu->addAction(
            "Supported Instructions..."
        );


    helpMenu->addSeparator();


    actionAbout =
        helpMenu->addAction(
            "About MIPS Studio"
        );


    // ========================================================
    // CONNECTIONS
    // ========================================================

    connect(
        actionNew,
        &QAction::triggered,
        this,
        &MainWindow::newFile
    );


    connect(
        actionOpen,
        &QAction::triggered,
        this,
        &MainWindow::openFile
    );


    connect(
        actionSave,
        &QAction::triggered,
        this,
        &MainWindow::saveFile
    );


    connect(
        actionSaveAs,
        &QAction::triggered,
        this,
        &MainWindow::saveFileAs
    );


    connect(
        actionExit,
        &QAction::triggered,
        this,
        &QWidget::close
    );


    connect(
        actionAssemble,
        &QAction::triggered,
        this,
        &MainWindow::assembleCurrentDocument
    );


    connect(
        actionRun,
        &QAction::triggered,
        this,
        &MainWindow::onRun
    );


    connect(
        actionStep,
        &QAction::triggered,
        this,
        &MainWindow::onStep
    );


    connect(
        actionReset,
        &QAction::triggered,
        this,
        &MainWindow::onReset
    );


    connect(
        actionShowBreakpoints,
        &QAction::triggered,
        this,
        &MainWindow::showBreakpoints
    );


    connect(
        actionResetBreakpoints,
        &QAction::triggered,
        this,
        &MainWindow::resetBreakpoints
    );


    connect(
        actionResetLayout,
        &QAction::triggered,
        this,
        &MainWindow::resetLayout
    );


    connect(
        actionPreferences,
        &QAction::triggered,
        this,
        &MainWindow::openPreferences
    );


    connect(
        actionHelp,
        &QAction::triggered,
        this,
        &MainWindow::showHelp
    );

    connect(
        actionKeyboardShortcuts,
        &QAction::triggered,
        this,
        &MainWindow::showKeyboardShortcuts
    );

    connect(
        instructionReferenceAction,
        &QAction::triggered,
        this,
        [this]() {
            InstructionReferenceDialog dialog(
                this
            );

            dialog.exec();}
    );


    connect(
        actionAbout,
        &QAction::triggered,
        this,
        &MainWindow::showAbout
    );
}


// ============================================================
// TOOLBAR
// ============================================================

void MainWindow::buildToolbar()
{
    QToolBar* toolbar =
        addToolBar(
            "Main Toolbar"
        );


    toolbar->setObjectName(
        "MainToolbar"
    );


    toolbar->setMovable(
        false
    );


    // --------------------------------------------------------
    // Files
    // --------------------------------------------------------

    toolbar->addAction(
        actionOpen
    );


    toolbar->addAction(
        actionSave
    );


    toolbar->addSeparator();


    // --------------------------------------------------------
    // Assembly
    // --------------------------------------------------------

    toolbar->addAction(
        actionAssemble
    );


    toolbar->addSeparator();


    // --------------------------------------------------------
    // Execution
    // --------------------------------------------------------

    toolbar->addAction(
        actionRun
    );


    toolbar->addAction(
        actionStep
    );


    toolbar->addAction(
        actionReset
    );


    toolbar->addSeparator();


    // --------------------------------------------------------
    // Breakpoints
    // --------------------------------------------------------

    toolbar->addAction(
        actionResetBreakpoints
    );
}


// ============================================================
// EDITOR
// ============================================================

void MainWindow::buildEditor()
{
    editor =
        new CodeEditor(this);


    editor->setObjectName(
        "AssemblyEditor"
    );


    editor->setPlaceholderText(
        "# Write MIPS assembly here\n\n"
        ".data\n"
        "message:\n"
        ".asciiz \"Hello MIPS!\"\n\n"
        ".text\n\n"
        "main:\n"
        "    la $a0, message\n"
        "    li $v0, 4\n"
        "    syscall\n"
        "    li $v0, 10\n"
        "    syscall\n"
    );


    setCentralWidget(
        editor
    );


    syntaxHighlighter =
        new MipsSyntaxHighlighter(
            editor->document()
        );


    connect(
        editor,
        &QPlainTextEdit::textChanged,
        this,
        &MainWindow::onDocumentModified
    );

    // The empty editor at startup is a clean document.
    editor->document()->setModified(
        false
    );


documentModified =
    false;

    connect(
        editor,
        &CodeEditor::breakpointToggled,
        this,
        &MainWindow::onBreakpointToggled
    );
}


// ============================================================
// REGISTER PANEL
// ============================================================

void MainWindow::buildRegisterPanel()
{
    registersDock =
        new QDockWidget(
            "Registers",
            this
        );


    registersDock->setObjectName(
        "RegistersDock"
    );


    registerTable =
        new QTableWidget(
            32,
            5,
            registersDock
        );


    registerTable->setHorizontalHeaderLabels({
        "Register",
        "#",
        "Hex",
        "Signed",
        "Unsigned"
    });


    registerTable
        ->verticalHeader()
        ->setVisible(false);


    registerTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );


    registerTable
        ->horizontalHeader()
        ->setStretchLastSection(
            true
        );


    registerTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    registerTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );


    registerTable->setAlternatingRowColors(
        true
    );


    for (
        int i = 0;
        i < 32;
        i++
    ) {
        registerTable->setItem(
            i,
            0,
            new QTableWidgetItem(
                REGISTER_NAMES[i]
            )
        );


        registerTable->setItem(
            i,
            1,
            new QTableWidgetItem(
                QString::number(i)
            )
        );


        registerTable->setItem(
            i,
            2,
            new QTableWidgetItem(
                "0x00000000"
            )
        );


        registerTable->setItem(
            i,
            3,
            new QTableWidgetItem(
                "0"
            )
        );


        registerTable->setItem(
            i,
            4,
            new QTableWidgetItem(
                "0"
            )
        );
    }


    registersDock->setWidget(
        registerTable
    );


    addDockWidget(
        Qt::RightDockWidgetArea,
        registersDock
    );
}


// ============================================================
// CPU STATE PANEL
// ============================================================

void MainWindow::buildCPUStatePanel()
{
    cpuStateDock =
        new QDockWidget(
            "CPU State",
            this
        );


    cpuStateDock->setObjectName(
        "CPUStateDock"
    );


    QWidget* panel =
        new QWidget(
            cpuStateDock
        );


    QVBoxLayout* layout =
        new QVBoxLayout(
            panel
        );


    QLabel* title =
        new QLabel(
            "Processor",
            panel
        );


    title->setObjectName(
        "SectionTitle"
    );


    pcValue =
        new QLabel(
            "PC       0x00000000",
            panel
        );


    hiValue =
        new QLabel(
            "HI       0x00000000",
            panel
        );


    loValue =
        new QLabel(
            "LO       0x00000000",
            panel
        );


    stateValue =
        new QLabel(
            "State    No program",
            panel
        );


    layout->addWidget(title);
    layout->addSpacing(8);

    layout->addWidget(pcValue);
    layout->addWidget(hiValue);
    layout->addWidget(loValue);

    layout->addSpacing(12);

    layout->addWidget(stateValue);

    layout->addStretch();


    cpuStateDock->setWidget(
        panel
    );


    addDockWidget(
        Qt::RightDockWidgetArea,
        cpuStateDock
    );
}


// ============================================================
// DISASSEMBLY PANEL
// ============================================================

void MainWindow::buildDisassemblyPanel()
{
    disassemblyDock =
        new QDockWidget(
            "Disassembly",
            this
        );


    disassemblyDock->setObjectName(
        "DisassemblyDock"
    );


    disassemblyTable =
        new QTableWidget(
            0,
            3,
            disassemblyDock
        );


    disassemblyTable->setHorizontalHeaderLabels({
        "Address",
        "Machine Code",
        "Instruction"
    });


    disassemblyTable
        ->verticalHeader()
        ->setVisible(false);


    disassemblyTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::ResizeToContents
        );


    disassemblyTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
        );


    disassemblyTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::Stretch
        );


    disassemblyTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    disassemblyTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );


    connect(
        disassemblyTable,
        &QTableWidget::cellDoubleClicked,
        this,
        &MainWindow::onDisassemblyActivated
    );


    disassemblyDock->setWidget(
        disassemblyTable
    );


    addDockWidget(
        Qt::BottomDockWidgetArea,
        disassemblyDock
    );
}


// ============================================================
// MEMORY PANEL
// ============================================================

void MainWindow::buildMemoryPanel()
{
    memoryDock =
        new QDockWidget(
            "Memory",
            this
        );


    memoryDock->setObjectName(
        "MemoryDock"
    );


    QWidget* container =
        new QWidget(
            memoryDock
        );


    QVBoxLayout* layout =
        new QVBoxLayout(
            container
        );


    layout->setContentsMargins(
        6,
        6,
        6,
        6
    );


    // --------------------------------------------------------
    // Navigation
    // --------------------------------------------------------

    QHBoxLayout* navigation =
        new QHBoxLayout();


    QLabel* addressLabel =
        new QLabel(
            "Address:",
            container
        );


    memoryAddressEdit =
        new QLineEdit(
            container
        );


    memoryAddressEdit->setPlaceholderText(
        "0x1000"
    );


    QPushButton* goButton =
        new QPushButton(
            "Go",
            container
        );


    QPushButton* stackButton =
        new QPushButton(
            "Follow $sp",
            container
        );


    navigation->addWidget(
        addressLabel
    );


    navigation->addWidget(
        memoryAddressEdit,
        1
    );


    navigation->addWidget(
        goButton
    );


    navigation->addWidget(
        stackButton
    );


    memoryTable =
        new QTableWidget(
            container
        );


    memoryTable
        ->verticalHeader()
        ->setVisible(false);


    memoryTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    layout->addLayout(
        navigation
    );


    layout->addWidget(
        memoryTable
    );


    connect(
        goButton,
        &QPushButton::clicked,
        this,
        &MainWindow::jumpMemoryFromText
    );


    connect(
        memoryAddressEdit,
        &QLineEdit::returnPressed,
        this,
        &MainWindow::jumpMemoryFromText
    );


    connect(
        stackButton,
        &QPushButton::clicked,
        this,
        &MainWindow::jumpMemoryToStackPointer
    );


    memoryDock->setWidget(
        container
    );


    addDockWidget(
        Qt::BottomDockWidgetArea,
        memoryDock
    );
}


// ============================================================
// CONSOLE PANEL
// ============================================================

void MainWindow::buildConsolePanel()
{
    consoleDock =
        new QDockWidget(
            "Console",
            this
        );


    consoleDock->setObjectName(
        "ConsoleDock"
    );


    // Container allows us to place controls above the tabs.
    QWidget* container =
        new QWidget(
            consoleDock
        );


    QVBoxLayout* layout =
        new QVBoxLayout(
            container
        );


    layout->setContentsMargins(
        0,
        0,
        0,
        0
    );


    layout->setSpacing(
        0
    );


    // ========================================================
    // CONSOLE TOOLBAR
    // ========================================================

    QWidget* consoleToolbar =
        new QWidget(
            container
        );


    consoleToolbar->setObjectName(
        "ConsoleToolbar"
    );


    QHBoxLayout* toolbarLayout =
        new QHBoxLayout(
            consoleToolbar
        );


    toolbarLayout->setContentsMargins(
        8,
        5,
        8,
        5
    );


    toolbarLayout->addStretch();


    QPushButton* clearButton =
        new QPushButton(
            "Clear Console",
            consoleToolbar
        );


    clearButton->setToolTip(
        "Clear program output"
    );


    toolbarLayout->addWidget(
        clearButton
    );


    // ========================================================
    // CONSOLE TABS
    // ========================================================

    QTabWidget* tabs =
        new QTabWidget(
            container
        );


    outputView =
        new QPlainTextEdit(
            tabs
        );


    outputView->setReadOnly(
        true
    );


    outputView->setPlaceholderText(
        "Program output will appear here."
    );


    problemsView =
        new QPlainTextEdit(
            tabs
        );


    problemsView->setReadOnly(
        true
    );


    problemsView->setPlaceholderText(
        "Assembler errors and warnings will appear here."
    );


    tabs->addTab(
        outputView,
        "Output"
    );


    tabs->addTab(
        problemsView,
        "Problems"
    );


    layout->addWidget(
        consoleToolbar
    );


    layout->addWidget(
        tabs,
        1
    );


    connect(
        clearButton,
        &QPushButton::clicked,
        this,
        &MainWindow::clearConsole
    );


    consoleDock->setWidget(
        container
    );


    addDockWidget(
        Qt::BottomDockWidgetArea,
        consoleDock
    );
}


// ============================================================
// CLEAR CONSOLE
// ============================================================

void MainWindow::clearConsole()
{
    outputView->clear();


    statusBar()->showMessage(
        "Console cleared.",
        2000
    );
}


// ============================================================
// SYMBOLS PANEL
// ============================================================

void MainWindow::buildSymbolsPanel()
{
    symbolsDock =
        new QDockWidget(
            "Symbols",
            this
        );


    symbolsDock->setObjectName(
        "SymbolsDock"
    );


    symbolTable =
        new QTableWidget(
            0,
            4,
            symbolsDock
        );


    symbolTable->setHorizontalHeaderLabels({
        "Label",
        "Address",
        "Section",
        "Line"
    });


    symbolTable
        ->verticalHeader()
        ->setVisible(false);


    // Label consumes remaining width.
    symbolTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::Stretch
        );


    symbolTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
        );


    symbolTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents
        );


    symbolTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents
        );


    symbolTable->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
    );


    symbolTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    symbolTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );


    connect(
        symbolTable,
        &QTableWidget::cellDoubleClicked,
        this,
        &MainWindow::onSymbolActivated
    );


    symbolsDock->setWidget(
        symbolTable
    );


    addDockWidget(
        Qt::LeftDockWidgetArea,
        symbolsDock
    );
}


// ============================================================
// PREFERENCES
// ============================================================

void MainWindow::openPreferences()
{
    SettingsDialog dialog(
        settings,
        this
    );


    // Apply updates the IDE while keeping Preferences open.
    connect(
        &dialog,
        &SettingsDialog::settingsApplied,
        this,
        [this]() {
            applySettings();

            refreshUI();


            statusBar()->showMessage(
                "Preferences applied.",
                2000
            );
        }
    );


    dialog.exec();


    // Re-read/apply stored values when the dialog closes.
    applySettings();

    refreshUI();
}


// ============================================================
// APPLY SETTINGS
// ============================================================

void MainWindow::applySettings()
{
    const QString profile =
        resolvedThemeProfile(
            settings
        );


    const bool light =
        profile == "light";


    // Global application theme.
    applyTheme();


    // ========================================================
    // EDITOR MECHANICS
    // ========================================================

    editor->setEditorFontSize(
        settings.editorFontSize()
    );


    editor->setLineNumbersVisible(
        settings.showLineNumbers()
    );


    editor->setCurrentLineHighlightEnabled(
        settings.highlightCurrentLine()
    );


    // ========================================================
    // GUTTER / DEBUGGER VISUALS
    // ========================================================

    editor->setVisualColors(

        // Gutter background
        light
            ? QColor("#DDD9D1")
            : QColor("#0B0F14"),

        // Gutter hover
        light
            ? QColor("#C9C2B7")
            : QColor("#1C232D"),

        // Gutter text
        light
            ? QColor("#696158")
            : QColor("#6E7681"),

        // Current editor line
        QColor(
            settings.editorCurrentLineColor(
                profile
            )
        ),

        // Breakpoint
        QColor(
            settings.breakpointColor(
                profile
            )
        ),

        // PC
        QColor(
            settings.executionColor(
                profile
            )
        )
    );


    // ========================================================
    // SYNTAX COLORS
    // ========================================================

    SyntaxPalette syntaxPalette;


    syntaxPalette.normalText =
        QColor(
            settings.syntaxNormalColor(
                profile
            )
        );


    syntaxPalette.instruction =
        QColor(
            settings.syntaxInstructionColor(
                profile
            )
        );


    syntaxPalette.reg =
        QColor(
            settings.syntaxRegisterColor(
                profile
            )
        );


    syntaxPalette.directive =
        QColor(
            settings.syntaxDirectiveColor(
                profile
            )
        );


    syntaxPalette.label =
        QColor(
            settings.syntaxLabelColor(
                profile
            )
        );


    syntaxPalette.comment =
        QColor(
            settings.syntaxCommentColor(
                profile
            )
        );


    syntaxPalette.string =
        QColor(
            settings.syntaxStringColor(
                profile
            )
        );


    syntaxPalette.number =
        QColor(
            settings.syntaxNumberColor(
                profile
            )
        );


    syntaxHighlighter->setPalette(
        syntaxPalette
    );


    // ========================================================
    // EDITOR BASE COLORS
    // ========================================================

    QPalette editorPalette =
        editor->palette();


    editorPalette.setColor(
        QPalette::Text,
        syntaxPalette.normalText
    );


    editorPalette.setColor(
        QPalette::Base,
        light
            ? QColor("#F0ECE4")
            : QColor("#0D1117")
    );


    editor->setPalette(
        editorPalette
    );


    // IMPORTANT:
    // Tab width uses font metrics, so calculate it AFTER the
    // editor font has been finalized.
    editor->setEditorTabWidth(
        settings.tabWidth()
    );


    refreshRegisters();
    refreshMemory();
    refreshDisassembly();

    synchronizePCToSource();
}


// ============================================================
// APPLY THEME
// ============================================================

void MainWindow::applyTheme()
{
    const QString profile =
        resolvedThemeProfile(
            settings
        );


    QString accent =
        settings.accentColor();


    if (!QColor(accent).isValid()) {
        accent =
            "#2F81F7";
    }


    const QString editorText =
        settings.syntaxNormalColor(
            profile
        );


    // ========================================================
    // LIGHT / PAPER THEME
    // ========================================================

    if (profile == "light") {
        setStyleSheet(
            QString(R"(

                /* ============================================
                   GLOBAL
                   ============================================ */

                QMainWindow,
                QDialog {
                    background-color: #E5E0D8;
                }

                QWidget {
                    color: #2D2925;
                    font-family: "Segoe UI";
                }


                /* ============================================
                   MENUS
                   ============================================ */

                QMenuBar {
                    background: #ECE7DF;
                    color: #2D2925;
                    border-bottom: 1px solid #C6BFB5;
                    padding: 4px;
                }

                QMenuBar::item {
                    padding: 6px 10px;
                    border-radius: 4px;
                }

                QMenuBar::item:selected {
                    background: #D6D0C7;
                }

                QMenu {
                    background: #F0ECE5;
                    color: #2D2925;
                    border: 1px solid #C3BCB1;
                    padding: 6px;
                }

                QMenu::item {
                    padding: 7px 28px 7px 12px;
                    border-radius: 4px;
                }

                QMenu::item:selected {
                    background: %1;
                    color: #FFFFFF;
                }


                /* ============================================
                   TOOLBAR
                   ============================================ */

                QToolBar {
                    background: #ECE7DF;
                    border: none;
                    border-bottom: 1px solid #C6BFB5;
                    spacing: 6px;
                    padding: 7px;
                }

                QToolButton {
                    color: #2D2925;
                    background: transparent;
                    border: 1px solid transparent;
                    border-radius: 6px;
                    padding: 7px 11px;
                }

                QToolButton:hover {
                    background: #D7D1C7;
                    border-color: #BFB7AC;
                }

                QToolButton:pressed {
                    background: #C8C0B5;
                }

                QToolButton:disabled {
                    color: #91897F;
                }


                /* ============================================
                   EDITOR
                   ============================================ */

                QPlainTextEdit#AssemblyEditor {
                    background: #F0ECE4;
                    color: %2;
                    border: none;
                    selection-background-color: %1;
                    selection-color: #FFFFFF;
                    padding: 8px;
                }


                /* ============================================
                   DOCKS
                   ============================================ */

                QDockWidget {
                    color: #2D2925;
                    border: 1px solid #C6BFB5;
                }

                QDockWidget::title {
                    background: #DAD4CB;
                    color: #2D2925;
                    padding: 8px 10px;
                    border-bottom: 1px solid #C2BAAF;
                }


                /* ============================================
                   TABLES
                   ============================================ */

                QTableWidget {
                    background: #ECE8E1;
                    color: #2D2925;
                    alternate-background-color: #E3DED6;
                    gridline-color: #C7C0B5;
                    border: none;
                    selection-background-color: #C8DDF4;
                    selection-color: #25211E;
                }

                QListWidget#InstructionList,
                QTextBrowser#InstructionDetails {
                    background: #F0ECE4;
                    color: #2D2925;
                    border: 1px solid #C6BFB5;
                    selection-background-color: #C8DDF4;
                    selection-color: #25211E;
                }

                QListWidget#InstructionList::item {
                    padding: 7px;
                }

                QListWidget#InstructionList::item:selected {
                    background: #C8DDF4;
                    color: #25211E;
                }

                QHeaderView::section {
                    background: #DAD4CB;
                    color: #5E574F;
                    border: none;
                    border-right: 1px solid #C2BAAF;
                    border-bottom: 1px solid #C2BAAF;
                    padding: 7px;
                    font-weight: 600;
                }


                /* ============================================
                   TABS
                   ============================================ */

                QTabWidget::pane {
                    border: none;
                    border-top: 1px solid #C2BAAF;
                }

                QTabBar::tab {
                    background: #DAD4CB;
                    color: #625B53;
                    padding: 8px 15px;
                    border-right: 1px solid #C2BAAF;
                }

                QTabBar::tab:hover {
                    background: #CEC7BD;
                }

                QTabBar::tab:selected {
                    background: #ECE8E1;
                    color: #24211E;
                    border-bottom: 2px solid %1;
                }


                /* ============================================
                   CONSOLE
                   ============================================ */

                QDockWidget QPlainTextEdit {
                    background: #ECE8E1;
                    color: #2D2925;
                    border: none;
                    font-family: "Consolas";
                    padding: 6px;
                }


                /* ============================================
                   INPUT CONTROLS
                   ============================================ */

                QLineEdit,
                QComboBox,
                QSpinBox {
                    color: #2D2925;
                    background: #F2EEE7;
                    border: 1px solid #B9B0A4;
                    border-radius: 5px;
                    padding: 6px 8px;
                }

                QLineEdit:focus,
                QComboBox:focus,
                QSpinBox:focus {
                    border: 1px solid %1;
                }


                /* ============================================
                   STANDARD BUTTONS
                   ============================================ */

                QPushButton {
                    color: #2D2925;
                    background: #D9D2C8;
                    border: 1px solid #B7AEA2;
                    border-radius: 5px;
                    padding: 6px 10px;
                }

                QPushButton:hover {
                    background: #C8C0B5;
                    border-color: #A69D91;
                }

                QPushButton:pressed {
                    background: #B8AFA3;
                }


                /* ============================================
                   +/- BUTTONS
                   ============================================ */

                QPushButton#StepperButton {
                    color: #24211E;
                    background: #C4BBAF;
                    border: 1px solid #9F9589;
                    border-radius: 6px;
                    font-size: 18px;
                    font-weight: 700;
                    padding: 0px;
                }

                QPushButton#StepperButton:hover {
                    background: #AEA598;
                    border-color: %1;
                }

                QPushButton#StepperButton:pressed {
                    background: #9E9589;
                }


                /* ============================================
                   APPLY BUTTON
                   ============================================ */

                QPushButton#PrimarySettingsButton {
                    background: %1;
                    color: #FFFFFF;
                    border-color: %1;
                    font-weight: 600;
                }


                /* ============================================
                   SETTINGS HINT
                   ============================================ */

                QLabel#SettingsHint {
                    color: #655E56;
                    background: #DAD4CB;
                    border: 1px solid #C6BFB5;
                    border-radius: 6px;
                    padding: 9px;
                }


                /* ============================================
                   SCROLLBARS
                   ============================================ */

                QScrollBar:vertical {
                    background: #D8D2C9;
                    width: 12px;
                    margin: 0;
                }

                QScrollBar::handle:vertical {
                    background: #9E968B;
                    min-height: 30px;
                    border-radius: 6px;
                }

                QScrollBar::handle:vertical:hover {
                    background: #7E776E;
                }

                QScrollBar:horizontal {
                    background: #D8D2C9;
                    height: 12px;
                    margin: 0;
                }

                QScrollBar::handle:horizontal {
                    background: #9E968B;
                    min-width: 30px;
                    border-radius: 6px;
                }

                QScrollBar::handle:horizontal:hover {
                    background: #7E776E;
                }

                QScrollBar::add-line,
                QScrollBar::sub-line {
                    width: 0px;
                    height: 0px;
                }

                QScrollBar::add-page,
                QScrollBar::sub-page {
                    background: transparent;
                }


                /* ============================================
                   MISC
                   ============================================ */

                QLabel#SectionTitle {
                    font-size: 15px;
                    font-weight: 600;
                    color: #2D2925;
                }

                QStatusBar {
                    background: #DAD4CB;
                    color: #625B53;
                    border-top: 1px solid #C2BAAF;
                }

            )")
            .arg(
                accent,
                editorText
            )
        );


        return;
    }


    // ========================================================
    // DARK THEME
    // ========================================================

    setStyleSheet(
        QString(R"(

            QMainWindow,
            QDialog {
                background-color: #101317;
            }

            QWidget {
                color: #E6EDF3;
                font-family: "Segoe UI";
            }


            /* ================================================
               MENUS
               ================================================ */

            QMenuBar {
                background: #161B22;
                color: #E6EDF3;
                padding: 4px;
                border-bottom: 1px solid #30363D;
            }

            QMenuBar::item {
                padding: 6px 10px;
                border-radius: 4px;
            }

            QMenuBar::item:selected {
                background: #21262D;
            }

            QMenu {
                background: #161B22;
                border: 1px solid #30363D;
                padding: 6px;
            }

            QMenu::item {
                padding: 7px 28px 7px 12px;
                border-radius: 4px;
            }

            QMenu::item:selected {
                background: %1;
            }


            /* ================================================
               TOOLBAR
               ================================================ */

            QToolBar {
                background: #161B22;
                border: none;
                border-bottom: 1px solid #30363D;
                spacing: 6px;
                padding: 7px;
            }

            QToolButton {
                background: transparent;
                border: 1px solid transparent;
                border-radius: 6px;
                padding: 7px 11px;
            }

            QToolButton:hover {
                background: #21262D;
                border-color: #30363D;
            }

            QToolButton:pressed {
                background: #30363D;
            }

            QToolButton:disabled {
                color: #6E7681;
            }


            /* ================================================
               EDITOR
               ================================================ */

            QPlainTextEdit#AssemblyEditor {
                background: #0D1117;
                color: %2;
                border: none;
                selection-background-color: %1;
                selection-color: #FFFFFF;
                padding: 8px;
            }


            /* ================================================
               DOCKS
               ================================================ */

            QDockWidget {
                color: #E6EDF3;
                border: 1px solid #30363D;
            }

            QDockWidget::title {
                background: #161B22;
                padding: 8px 10px;
                border-bottom: 1px solid #30363D;
            }


            /* ================================================
               TABLES
               ================================================ */

            QTableWidget {
                background: #0D1117;
                alternate-background-color: #11161D;
                gridline-color: #21262D;
                border: none;
                selection-background-color: %1;
                selection-color: #FFFFFF;
            }

            QListWidget#InstructionList,
            QTextBrowser#InstructionDetails {
                background: #0D1117;
                color: #E6EDF3;
                border: 1px solid #30363D;
                selection-background-color: #23466B;
                selection-color: #FFFFFF;
            }

            QListWidget#InstructionList::item {
                padding: 7px;
            }

            QListWidget#InstructionList::item:selected {
                background: #23466B;
                color: #FFFFFF;
            }

            QHeaderView::section {
                background: #161B22;
                color: #8B949E;
                border: none;
                border-right: 1px solid #30363D;
                border-bottom: 1px solid #30363D;
                padding: 7px;
                font-weight: 600;
            }


            /* ================================================
               TABS
               ================================================ */

            QTabWidget::pane {
                border: none;
                border-top: 1px solid #30363D;
            }

            QTabBar::tab {
                background: #161B22;
                color: #8B949E;
                padding: 8px 15px;
                border-right: 1px solid #30363D;
            }

            QTabBar::tab:hover {
                background: #1F252D;
            }

            QTabBar::tab:selected {
                color: #FFFFFF;
                background: #0D1117;
                border-bottom: 2px solid %1;
            }


            /* ================================================
               CONSOLE
               ================================================ */

            QDockWidget QPlainTextEdit {
                background: #0D1117;
                color: #C9D1D9;
                border: none;
                font-family: "Consolas";
                padding: 6px;
            }


            /* ================================================
               INPUT CONTROLS
               ================================================ */

            QLineEdit,
            QComboBox,
            QSpinBox {
                color: #E6EDF3;
                background: #161B22;
                border: 1px solid #30363D;
                border-radius: 5px;
                padding: 6px 8px;
            }

            QLineEdit:focus,
            QComboBox:focus,
            QSpinBox:focus {
                border: 1px solid %1;
            }


            /* ================================================
               STANDARD BUTTONS
               ================================================ */

            QPushButton {
                color: #E6EDF3;
                background: #161B22;
                border: 1px solid #30363D;
                border-radius: 5px;
                padding: 6px 10px;
            }

            QPushButton:hover {
                background: #21262D;
            }

            QPushButton:pressed {
                background: #30363D;
            }


            /* ================================================
               +/- BUTTONS
               ================================================ */

            QPushButton#StepperButton {
                color: #FFFFFF;
                background: #2D333B;
                border: 1px solid #484F58;
                border-radius: 6px;
                font-size: 18px;
                font-weight: 700;
                padding: 0px;
            }

            QPushButton#StepperButton:hover {
                background: #3B424C;
                border-color: %1;
            }

            QPushButton#StepperButton:pressed {
                background: #484F58;
            }


            /* ================================================
               APPLY
               ================================================ */

            QPushButton#PrimarySettingsButton {
                background: %1;
                color: #FFFFFF;
                border-color: %1;
                font-weight: 600;
            }


            QLabel#SettingsHint {
                color: #9DA7B3;
                background: #161B22;
                border: 1px solid #30363D;
                border-radius: 6px;
                padding: 9px;
            }


            /* ================================================
               SCROLLBARS
               ================================================ */

            QScrollBar:vertical {
                background: #0D1117;
                width: 12px;
                margin: 0;
            }

            QScrollBar::handle:vertical {
                background: #30363D;
                min-height: 30px;
                border-radius: 6px;
            }

            QScrollBar::handle:vertical:hover {
                background: #484F58;
            }

            QScrollBar:horizontal {
                background: #0D1117;
                height: 12px;
                margin: 0;
            }

            QScrollBar::handle:horizontal {
                background: #30363D;
                min-width: 30px;
                border-radius: 6px;
            }

            QScrollBar::handle:horizontal:hover {
                background: #484F58;
            }

            QScrollBar::add-line,
            QScrollBar::sub-line {
                width: 0px;
                height: 0px;
            }

            QScrollBar::add-page,
            QScrollBar::sub-page {
                background: transparent;
            }


            QLabel#SectionTitle {
                font-size: 15px;
                font-weight: 600;
            }

            QStatusBar {
                background: #161B22;
                color: #8B949E;
                border-top: 1px solid #30363D;
            }

        )")
        .arg(
            accent,
            editorText
        )
    );
}


// ============================================================
// NEW FILE
// ============================================================

void MainWindow::newFile()
{
    if (!maybeSave()) {
        return;
    }


    editor->blockSignals(
        true
    );


    editor->clear();

    editor->document()->setModified(
    false
);


    editor->blockSignals(
        false
    );


    editor->clearBreakpoints();

    editor->setExecutionLine(
        -1
    );


    session.clearBreakpoints();

    sourceMap.clear();


    currentFile.clear();

    documentModified =
        false;


    outputView->clear();

    problemsView->clear();


    refreshSymbols();

    refreshDisassembly();

    refreshMemory();

    refreshCPUState();


    updateWindowTitle();

    updateActionStates();


    statusBar()->showMessage(
        "New assembly file"
    );
}


// ============================================================
// OPEN FILE
// ============================================================

void MainWindow::openFile()
{
    if (!maybeSave()) {
        return;
    }


    const QString filename =
        QFileDialog::getOpenFileName(
            this,
            "Open MIPS Assembly File",
            QString(),
            "MIPS Assembly (*.asm *.s);;"
            "All Files (*.*)"
        );


    if (filename.isEmpty()) {
        return;
    }


    loadFile(
        filename
    );
}

// ============================================================
// SAME FILE
// ============================================================

bool MainWindow::sameFile(
    const QString& first,
    const QString& second
) const
{
    if (
        first.isEmpty() ||
        second.isEmpty()
    ) {
        return false;
    }


    const QString firstPath =
        QFileInfo(first)
            .absoluteFilePath();


    const QString secondPath =
        QFileInfo(second)
            .absoluteFilePath();


    return QString::compare(
        firstPath,
        secondPath,
        Qt::CaseInsensitive
    ) == 0;
}

// ============================================================
// LOAD FILE
// ============================================================

void MainWindow::loadFile(
    const QString& filename
)
{
    QFile file(
        filename
    );


    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text
        )) {
        QMessageBox::critical(
            this,
            "Open File",
            "Could not open:\n" +
            filename
        );

        return;
    }


    QTextStream stream(
        &file
    );


    const QString contents =
        stream.readAll();

    applyFileContents(filename, contents);
}

void MainWindow::applyFileContents(const QString& filename, const QString& contents,
                                   const SourceMap* preparedSourceMap)
{
    const bool fileChanged = !sameFile(currentFile, filename);

    editor->blockSignals(
        true
    );


    editor->setPlainText(
        contents
    );


    // Loading from disk is not a user modification.
    editor->document()->setModified(
        false
    );


    editor->blockSignals(
        false
    );


    editor->clearBreakpoints();


    editor->setExecutionLine(
        -1
    );


    session.clearBreakpoints();


    // --------------------------------------------------------
    // OUTPUT POLICY
    // --------------------------------------------------------
    //
    // Different source file:
    //     clear old output.
    //
    // Same source file:
    //     preserve output.
    //
    // Reassemble/reset:
    //     loadFile() isn't involved, so output also survives.
    // --------------------------------------------------------

    if (fileChanged) {
        outputView->clear();
    }


    // Old diagnostics should never follow a freshly loaded
    // source document.
    problemsView->clear();


    currentFile =
        filename;


    documentModified =
        false;


    if (preparedSourceMap)
        sourceMap = *preparedSourceMap;
    else
        rebuildSourceMap();

    refreshSymbols();

    refreshDisassembly();

    refreshCPUState();


    updateWindowTitle();

    updateActionStates();


    statusBar()->showMessage(
        "Opened " +
        QFileInfo(filename).fileName(),
        3000
    );
}


// ============================================================
// SAVE FILE
// ============================================================

void MainWindow::saveFile()
{
    if (currentFile.isEmpty()) {
        saveFileAs();

        return;
    }


    saveDocument(
        currentFile
    );
}


// ============================================================
// SAVE AS
// ============================================================

void MainWindow::saveFileAs()
{
    QString filename =
        QFileDialog::getSaveFileName(
            this,
            "Save MIPS Assembly File",
            currentFile,
            "MIPS Assembly (*.asm);;"
            "Assembly Source (*.s);;"
            "All Files (*.*)"
        );


    if (filename.isEmpty()) {
        return;
    }


    if (
        QFileInfo(filename)
            .suffix()
            .isEmpty()
    ) {
        filename +=
            ".asm";
    }


    saveDocument(
        filename
    );
}


// ============================================================
// SAVE DOCUMENT
// ============================================================

bool MainWindow::saveDocument(
    const QString& filename
)
{
    QSaveFile file(
        filename
    );


    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Text
        )) {
        QMessageBox::critical(
            this,
            "Save File",
            "Could not save:\n" +
            filename
        );

        return false;
    }


    QTextStream stream(
        &file
    );


    stream <<
        editor->toPlainText();


    if (!file.commit()) {
        QMessageBox::critical(
            this,
            "Save File",
            "The file could not be written completely."
        );

        return false;
    }


    currentFile =
        filename;


    documentModified =
        false;

    editor->document()->setModified(
    false
);

    rebuildSourceMap();

    synchronizeBreakpoints();

    refreshSymbols();


    updateWindowTitle();

    updateActionStates();


    statusBar()->showMessage(
        "Saved " +
        QFileInfo(filename).fileName(),
        3000
    );


    return true;
}

// ============================================================
// BREAKPOINT MANAGER
// ============================================================

void MainWindow::showBreakpoints()
{
    QDialog dialog(
        this
    );


    dialog.setWindowTitle(
        "Breakpoints — MIPS Studio"
    );


    dialog.resize(
        520,
        360
    );


    QVBoxLayout* layout =
        new QVBoxLayout(
            &dialog
        );


    QLabel* description =
        new QLabel(
            "Active breakpoints in the current assembly source.",
            &dialog
        );


    layout->addWidget(
        description
    );


    QTableWidget* table =
        new QTableWidget(
            &dialog
        );


    table->setColumnCount(
        3
    );


    table->setHorizontalHeaderLabels({
        "Address",
        "Source Line",
        "Instruction"
    });


    table
        ->verticalHeader()
        ->setVisible(
            false
        );


    table->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );


    table->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );


    table
        ->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::ResizeToContents
        );


    table
        ->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
        );


    table
        ->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::Stretch
        );


    const auto& breakpoints =
        session.getBreakpoints();


    for (
        uint32_t address :
        breakpoints
    ) {
        const int row =
            table->rowCount();


        table->insertRow(
            row
        );


        const int sourceLine =
            sourceMap.lineForAddress(
                address
            );


        QString sourceText;


        if (sourceLine > 0) {
            sourceText =
                editor
                    ->document()
                    ->findBlockByNumber(
                        sourceLine - 1
                    )
                    .text()
                    .trimmed();
        }


        table->setItem(
            row,
            0,
            new QTableWidgetItem(
                toHex32(address)
            )
        );


        table->setItem(
            row,
            1,
            new QTableWidgetItem(
                sourceLine > 0
                    ? QString::number(
                          sourceLine
                      )
                    : "-"
            )
        );


        table->setItem(
            row,
            2,
            new QTableWidgetItem(
                sourceText
            )
        );
    }


    if (breakpoints.empty()) {
        table->setRowCount(
            1
        );


        QTableWidgetItem* empty =
            new QTableWidgetItem(
                "No active breakpoints"
            );


        empty->setFlags(
            empty->flags() &
            ~Qt::ItemIsSelectable
        );


        table->setItem(
            0,
            0,
            empty
        );


        table->setSpan(
            0,
            0,
            1,
            3
        );
    }


    layout->addWidget(
        table,
        1
    );


    QHBoxLayout* buttons =
        new QHBoxLayout();


    QPushButton* clearAll =
        new QPushButton(
            "Reset Breakpoints",
            &dialog
        );


    QPushButton* closeButton =
        new QPushButton(
            "Close",
            &dialog
        );


    buttons->addWidget(
        clearAll
    );


    buttons->addStretch();


    buttons->addWidget(
        closeButton
    );


    layout->addLayout(
        buttons
    );


    connect(
        clearAll,
        &QPushButton::clicked,
        &dialog,
        [this, &dialog]() {
            resetBreakpoints();

            dialog.accept();
        }
    );


    connect(
        closeButton,
        &QPushButton::clicked,
        &dialog,
        &QDialog::accept
    );


    dialog.exec();
}

// ============================================================
// RESET BREAKPOINTS
// ============================================================

void MainWindow::resetBreakpoints()
{
    const bool hasAny =
        !editor
            ->breakpointLines()
            .isEmpty();


    if (!hasAny) {
        statusBar()->showMessage(
            "No breakpoints to reset.",
            2000
        );

        return;
    }


    editor->clearBreakpoints();


    session.clearBreakpoints();


    refreshDisassembly();


    statusBar()->showMessage(
        "All breakpoints cleared.",
        2500
    );
}

// ============================================================
// UNSAVED CHANGES
// ============================================================

bool MainWindow::maybeSave()
{
    // --------------------------------------------------------
    // IMPORTANT
    // --------------------------------------------------------
    //
    // Do not trust a historical textChanged event.
    //
    // Ask QTextDocument whether the source is ACTUALLY dirty.
    // This prevents Open on first launch from producing a fake
    // "Do you want to save?" prompt.
    // --------------------------------------------------------

    const bool reallyModified =
        editor &&
        editor->document()->isModified();


    documentModified =
        reallyModified;


    if (!reallyModified) {
        return true;
    }


    const QMessageBox::StandardButton result =
        QMessageBox::warning(
            this,
            "Unsaved Changes",
            "The current assembly file has unsaved changes.",
            QMessageBox::Save |
            QMessageBox::Discard |
            QMessageBox::Cancel
        );


    if (
        result ==
        QMessageBox::Cancel
    ) {
        return false;
    }


    if (
        result ==
        QMessageBox::Discard
    ) {
        return true;
    }


    if (currentFile.isEmpty()) {
        QString filename =
            QFileDialog::getSaveFileName(
                this,
                "Save MIPS Assembly File",
                QString(),
                "MIPS Assembly (*.asm)"
            );


        if (filename.isEmpty()) {
            return false;
        }


        return saveDocument(
            filename
        );
    }


    return saveDocument(
        currentFile
    );
}


// ============================================================
// DOCUMENT MODIFIED
// ============================================================

void MainWindow::onDocumentModified()
{
    // Qt knows whether the document is genuinely modified.
    documentModified =
        editor
            ->document()
            ->isModified();


    rebuildSourceMap();

    refreshSymbols();


    if (documentModified) {
        // Source now differs from assembled machine code.
        editor->setExecutionLine(
            -1
        );
    }


    updateWindowTitle();

    updateActionStates();
}


// ============================================================
// WINDOW TITLE
// ============================================================

void MainWindow::updateWindowTitle()
{
    QString filename =
        currentFile.isEmpty()
            ? "Untitled.asm"
            : QFileInfo(
                  currentFile
              ).fileName();


    if (documentModified) {
        filename +=
            " *";
    }


    setWindowTitle(
        filename +
        " — MIPS Studio"
    );
}


// ============================================================
// SOURCE MAP
// ============================================================

void MainWindow::rebuildSourceMap()
{
    sourceMap.rebuild(
        editor->toPlainText()
    );
}


// ============================================================
// BREAKPOINT SYNCHRONIZATION
// ============================================================

void MainWindow::synchronizeBreakpoints()
{
    session.clearBreakpoints();


    for (
        int line :
        editor->breakpointLines()
    ) {
        if (
            sourceMap.hasAddressForLine(
                line
            )
        ) {
            session.addBreakpoint(
                sourceMap.addressForLine(
                    line
                )
            );
        }
    }


    refreshDisassembly();
}


// ============================================================
// BREAKPOINT TOGGLE
// ============================================================

void MainWindow::onBreakpointToggled(
    int line,
    bool enabled
)
{
    rebuildSourceMap();


    if (
        !sourceMap.hasAddressForLine(
            line
        )
    ) {
        statusBar()->showMessage(
            QString(
                "Line %1 does not contain an executable instruction."
            ).arg(line),
            3500
        );


        synchronizeBreakpoints();

        return;
    }


    const uint32_t address =
        sourceMap.addressForLine(
            line
        );


    if (enabled) {
        session.addBreakpoint(
            address
        );


        statusBar()->showMessage(
            "Breakpoint added at " +
            toHex32(address),
            2500
        );
    }
    else {
        session.removeBreakpoint(
            address
        );


        statusBar()->showMessage(
            "Breakpoint removed from " +
            toHex32(address),
            2500
        );
    }


    refreshDisassembly();
}


// ============================================================
// CAPTURE CONSOLE
// ============================================================

QString MainWindow::captureConsoleDuring(
    const std::function<void()>& function
)
{
    std::ostringstream captured;


    std::streambuf* oldBuffer =
        std::cout.rdbuf(
            captured.rdbuf()
        );


    try {
        function();
    }
    catch (...) {
        std::cout.rdbuf(
            oldBuffer
        );

        throw;
    }


    std::cout.rdbuf(
        oldBuffer
    );


    return QString::fromStdString(
        captured.str()
    );
}


// ============================================================
// ASSEMBLE
// ============================================================

void MainWindow::assembleCurrentDocument()
{
    if (currentFile.isEmpty()) {
        saveFileAs();


        if (currentFile.isEmpty()) {
            return;
        }
    }


    if (documentModified) {
        if (!saveDocument(
                currentFile
            )) {
            return;
        }
    }


    problemsView->clear();


    bool assembled =
        false;


    const QString assemblerOutput =
        captureConsoleDuring(
            [&]() {
                assembled =
                    session.assembleProgram(
                        currentFile
                            .toStdString()
                    );
            }
        );


    if (!assembled) {
        appendProblem(
            assemblerOutput.isEmpty()
                ? "Assembly failed."
                : assemblerOutput
        );


        statusBar()->showMessage(
            "Assembly failed."
        );


        updateActionStates();

        return;
    }


    rebuildSourceMap();

    synchronizeBreakpoints();


    capturePreviousState();

    refreshUI();


    appendOutput(
        "Assembly completed successfully."
    );


    statusBar()->showMessage(
        "Assembly completed successfully.",
        4000
    );


    updateActionStates();
}


// ============================================================
// STEP
// ============================================================

void MainWindow::onStep()
{
    if (
        !session.hasProgram() ||
        documentModified
    ) {
        statusBar()->showMessage(
            "Assemble the current source before stepping."
        );

        return;
    }


    if (session.getCPU().halted) {
        statusBar()->showMessage(
            "Program is halted. Reset before stepping."
        );

        return;
    }


    capturePreviousState();


    const QString output =
        captureConsoleDuring(
            [&]() {
                session.step();
            }
        );


    if (!output.isEmpty()) {
        appendOutput(
            output
        );
    }


    refreshUI();

    updateActionStates();


    statusBar()->showMessage(
        "Executed one instruction."
    );
}


// ============================================================
// RUN / CONTINUE
// ============================================================

void MainWindow::onRun()
{
    if (
        !session.hasProgram() ||
        documentModified
    ) {
        statusBar()->showMessage(
            "Assemble the current source before running."
        );

        return;
    }


    if (session.getCPU().halted) {
        statusBar()->showMessage(
            "Program is halted. Reset before running."
        );

        return;
    }


    capturePreviousState();


    EmulatorSession::RunResult result;


    QApplication::setOverrideCursor(
        Qt::WaitCursor
    );


    const QString output =
        captureConsoleDuring(
            [&]() {
                result =
                    session.run(
                        settings
                            .maxInstructionsPerRun()
                    );
            }
        );


    QApplication::restoreOverrideCursor();


    if (!output.isEmpty()) {
        appendOutput(
            output
        );
    }


    refreshUI();

    updateActionStates();


    if (result.breakpointHit) {
        statusBar()->showMessage(
            "Paused at breakpoint " +
            toHex32(
                result.stopAddress
            )
        );

        return;
    }


    if (
        result.instructionLimitReached
    ) {
        statusBar()->showMessage(
            QString(
                "Execution stopped after %1 instructions: "
                "safety limit reached."
            )
            .arg(
                result.instructionsExecuted
            )
        );

        return;
    }


    if (result.halted) {
        statusBar()->showMessage(
            QString(
                "Program halted — %1 instructions executed."
            )
            .arg(
                result.instructionsExecuted
            )
        );

        return;
    }


    statusBar()->showMessage(
        QString(
            "%1 instructions executed."
        )
        .arg(
            result.instructionsExecuted
        )
    );
}


// ============================================================
// RESET
// ============================================================

void MainWindow::onReset()
{
    if (!session.hasProgram()) {
        statusBar()->showMessage(
            "No assembled program to reset."
        );

        return;
    }


    capturePreviousState();


    bool successful =
        false;


    const QString output =
        captureConsoleDuring(
            [&]() {
                successful =
                    session.reset();
            }
        );


    if (!successful) {
        appendProblem(
            output.isEmpty()
                ? "Reset failed."
                : output
        );

        return;
    }


    synchronizeBreakpoints();

    refreshUI();

    updateActionStates();


    statusBar()->showMessage(
        "Program reset."
    );
}


// ============================================================
// CAPTURE PREVIOUS REGISTER STATE
// ============================================================

void MainWindow::capturePreviousState()
{
    const CPU& cpu =
        session.getCPU();


    for (
        int i = 0;
        i < 32;
        i++
    ) {
        previousRegisters[i] =
            cpu.registers[i];
    }
}


// ============================================================
// REFRESH ALL
// ============================================================

void MainWindow::refreshUI()
{
    refreshRegisters();

    refreshCPUState();

    refreshDisassembly();

    refreshMemory();

    refreshSymbols();

    synchronizePCToSource();
}


// ============================================================
// REFRESH REGISTERS
// ============================================================

void MainWindow::refreshRegisters()
{
    const CPU& cpu =
        session.getCPU();


    const bool highlightChanges =
        settings.highlightChangedRegisters();


    const QString profile =
        resolvedThemeProfile(
            settings
        );


    const QColor changedColor(
        settings.changedRegisterColor(
            profile
        )
    );


    for (
        int i = 0;
        i < 32;
        i++
    ) {
        const uint32_t value =
            cpu.registers[i];


        registerTable
            ->item(i, 2)
            ->setText(
                toHex32(value)
            );


        registerTable
            ->item(i, 3)
            ->setText(
                QString::number(
                    static_cast<int32_t>(
                        value
                    )
                )
            );


        registerTable
            ->item(i, 4)
            ->setText(
                QString::number(
                    value
                )
            );


        const bool changed =
            highlightChanges &&
            value !=
            previousRegisters[i];


        for (
            int column = 0;
            column < 5;
            column++
        ) {
            registerTable
                ->item(i, column)
                ->setBackground(
                    changed
                        ? changedColor
                        : QColor(
                              Qt::transparent
                          )
                );
        }
    }
}


// ============================================================
// REFRESH CPU STATE
// ============================================================

void MainWindow::refreshCPUState()
{
    if (!session.hasProgram()) {
        pcValue->setText(
            "PC       0x00000000"
        );

        hiValue->setText(
            "HI       0x00000000"
        );

        loValue->setText(
            "LO       0x00000000"
        );

        stateValue->setText(
            "State    No program"
        );

        return;
    }


    const CPU& cpu =
        session.getCPU();


    pcValue->setText(
        "PC       " +
        toHex32(cpu.pc)
    );


    hiValue->setText(
        "HI       " +
        toHex32(cpu.hi)
    );


    loValue->setText(
        "LO       " +
        toHex32(cpu.lo)
    );


    stateValue->setText(
        cpu.halted
            ? "State    Halted"
            : "State    Ready"
    );
}


// ============================================================
// SOURCE / PC SYNCHRONIZATION
// ============================================================

void MainWindow::synchronizePCToSource()
{
    if (
        !session.hasProgram() ||
        documentModified
    ) {
        editor->setExecutionLine(
            -1
        );

        return;
    }


    const uint32_t pc =
        session.getCPU().pc;


    const int line =
        sourceMap.lineForAddress(
            pc
        );


    editor->setExecutionLine(
        line
    );


    if (
        settings.autoScrollToPC() &&
        line > 0
    ) {
        editor->goToLine(
            line
        );
    }
}


// ============================================================
// REFRESH DISASSEMBLY
// ============================================================

void MainWindow::refreshDisassembly()
{
    disassemblyTable->setRowCount(
        0
    );


    if (!session.hasProgram()) {
        return;
    }


    Memory& memory =
        session.getMemory();


    const CPU& cpu =
        session.getCPU();


    const QString profile =
        resolvedThemeProfile(
            settings
        );


    QColor pcHighlight(
        settings.executionColor(
            profile
        )
    );


    pcHighlight.setAlpha(
        profile == "light"
            ? 45
            : 65
    );


    constexpr int MAXIMUM_INSTRUCTIONS =
        256;


    for (
        int row = 0;
        row < MAXIMUM_INSTRUCTIONS;
        row++
    ) {
        const uint32_t address =
            static_cast<uint32_t>(
                row * 4
            );


        const uint32_t instruction =
            memory.memRead32(
                address
            );


        // Temporary display-end heuristic until the assembler
        // exposes exact text-segment bounds.
        if (
            row > 20 &&
            instruction == 0 &&
            address > cpu.pc + 64
        ) {
            break;
        }


        const int tableRow =
            disassemblyTable
                ->rowCount();


        disassemblyTable->insertRow(
            tableRow
        );


        QString addressText =
            toHex32(address);


        if (
            session.hasBreakpoint(
                address
            )
        ) {
            addressText =
                "● " +
                addressText;
        }


        if (address == cpu.pc) {
            addressText =
                "▶ " +
                addressText;
        }


        QTableWidgetItem* addressItem =
            new QTableWidgetItem(
                addressText
            );


        QTableWidgetItem* codeItem =
            new QTableWidgetItem(
                toHex32(
                    instruction
                )
            );


        QTableWidgetItem* instructionItem =
            new QTableWidgetItem(
                disassemble(
                    instruction,
                    address
                )
            );


        addressItem->setData(
            Qt::UserRole,
            address
        );


        if (address == cpu.pc) {
            addressItem->setBackground(
                pcHighlight
            );

            codeItem->setBackground(
                pcHighlight
            );

            instructionItem->setBackground(
                pcHighlight
            );
        }


        disassemblyTable->setItem(
            tableRow,
            0,
            addressItem
        );


        disassemblyTable->setItem(
            tableRow,
            1,
            codeItem
        );


        disassemblyTable->setItem(
            tableRow,
            2,
            instructionItem
        );


        if (
            address == cpu.pc &&
            settings.autoScrollToPC()
        ) {
            disassemblyTable
                ->scrollToItem(
                    addressItem,
                    QAbstractItemView::PositionAtCenter
                );
        }
    }
}


// ============================================================
// DISASSEMBLY NAVIGATION
// ============================================================

void MainWindow::onDisassemblyActivated(
    int row,
    int
)
{
    QTableWidgetItem* item =
        disassemblyTable->item(
            row,
            0
        );


    if (!item) {
        return;
    }


    const uint32_t address =
        item
            ->data(
                Qt::UserRole
            )
            .toUInt();


    const int line =
        sourceMap.lineForAddress(
            address
        );


    if (line > 0) {
        editor->goToLine(
            line
        );
    }
}


// ============================================================
// HELP
// ============================================================

void MainWindow::showHelp()
{
    QMessageBox::information(
        this,
        "MIPS Studio Help",

        "MIPS Studio\n\n"

        "Basic workflow:\n"
        "1. Open or create an assembly source file.\n"
        "2. Assemble the program with F5.\n"
        "3. Step with F7 or Run/Continue with F6.\n"
        "4. Inspect registers, memory and disassembly.\n"
        "5. Click the editor gutter to toggle breakpoints.\n\n"

        "Debugger:\n"
        "• Blue arrow = current execution line\n"
        "• Red dot = breakpoint\n"
        "• Changed registers are highlighted\n"
        "• Double-click disassembly to jump to source\n"
        "• Double-click symbols to navigate\n\n"

        "Memory:\n"
        "Enter hexadecimal such as 0x1000 or decimal addresses "
        "in the Memory navigator. Follow $sp jumps to the "
        "current stack-pointer address.\n\n"

        "Output:\n"
        "Program syscall output appears in Console → Output. "
        "Assembler diagnostics appear in Problems."
    );
}


// ============================================================
// KEYBOARD SHORTCUTS
// ============================================================

void MainWindow::showKeyboardShortcuts()
{
    QDialog dialog(this);
    dialog.setObjectName("KeyboardShortcutsDialog");
    dialog.setWindowTitle("Keyboard Shortcuts");
    dialog.resize(620, 460);

    auto* layout = new QVBoxLayout(&dialog);
    auto* hint = new QLabel("Keyboard shortcuts available in MIPStudio:", &dialog);
    hint->setObjectName("SettingsHint");
    layout->addWidget(hint);

    auto* table = new QTableWidget(0, 3, &dialog);
    table->setObjectName("ShortcutsTable");
    table->setHorizontalHeaderLabels({"Menu", "Action", "Shortcut"});
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    // Use the live bindings so the reference stays in sync with the application.
    for (QAction* menuAction : menuBar()->actions()) {
        QMenu* menu = menuAction->menu();
        if (!menu)
            continue;
        for (QAction* action : menu->actions()) {
            QStringList bindings;
            for (const QKeySequence& shortcut : action->shortcuts()) {
                if (!shortcut.isEmpty())
                    bindings.append(shortcut.toString(QKeySequence::NativeText));
            }
            if (bindings.isEmpty())
                continue;

            const int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString(menu->title()).remove('&')));
            table->setItem(row, 1, new QTableWidgetItem(QString(action->text()).remove('&')));
            table->setItem(row, 2, new QTableWidgetItem(bindings.join(", ")));
        }
    }
    layout->addWidget(table);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}


// ============================================================
// ABOUT
// ============================================================

void MainWindow::showAbout()
{
    QMessageBox::about(
        this,
        "About MIPS Studio",

        "<h2>MIPS Studio</h2>"
        "<p>A modern MIPS32 assembler, emulator and debugger.</p>"

        "<p>Features include source editing, assembly, execution, "
        "register inspection, memory inspection, disassembly, "
        "breakpoints, source mapping, symbols, syscall output and "
        "customizable IDE themes.</p>"

        "<p><b>MIPStudio " + QCoreApplication::applicationVersion().toHtmlEscaped() + "</b></p>"
    );
}

// ============================================================
// REFRESH MEMORY
// ============================================================

void MainWindow::refreshMemory()
{
    int bytesPerRow =
        settings.memoryBytesPerRow();


    if (
        bytesPerRow != 4 &&
        bytesPerRow != 8 &&
        bytesPerRow != 16
    ) {
        bytesPerRow =
            4;
    }


    QStringList headers;

    headers <<
        "Address";


    for (
        int i = 0;
        i < bytesPerRow;
        i++
    ) {
        headers <<
            QString("+%1")
                .arg(
                    i,
                    0,
                    16
                )
                .toUpper();
    }


    headers <<
        "ASCII";


    memoryTable->setColumnCount(
        bytesPerRow + 2
    );


    memoryTable->setHorizontalHeaderLabels(
        headers
    );


    memoryTable->setRowCount(
        0
    );


    memoryAddressEdit->setText(
        toHex32(
            memoryViewAddress
        )
    );


    if (!session.hasProgram()) {
        return;
    }


    Memory& memory =
        session.getMemory();


    constexpr int ROWS =
        32;


    for (
        int row = 0;
        row < ROWS;
        row++
    ) {
        const uint32_t address =
            memoryViewAddress +
            static_cast<uint32_t>(
                row *
                bytesPerRow
            );


        memoryTable->insertRow(
            row
        );


        memoryTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                toHex32(address)
            )
        );


        QString ascii;


        for (
            int offset = 0;
            offset < bytesPerRow;
            offset++
        ) {
            const uint8_t value =
                memory.memReadByte(
                    address +
                    static_cast<uint32_t>(
                        offset
                    )
                );


            memoryTable->setItem(
                row,
                offset + 1,
                new QTableWidgetItem(
                    toHexByte(value)
                )
            );


            if (
                value >= 32 &&
                value <= 126
            ) {
                ascii +=
                    QChar(value);
            }
            else {
                ascii +=
                    '.';
            }
        }


        memoryTable->setItem(
            row,
            bytesPerRow + 1,
            new QTableWidgetItem(
                ascii
            )
        );
    }


    memoryTable
        ->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );


    memoryTable
        ->horizontalHeader()
        ->setStretchLastSection(
            true
        );
}


// ============================================================
// MEMORY NAVIGATION
// ============================================================

void MainWindow::jumpMemoryToAddress(
    uint32_t address
)
{
    int bytesPerRow =
        settings.memoryBytesPerRow();


    if (bytesPerRow <= 0) {
        bytesPerRow =
            4;
    }


    memoryViewAddress =
        address -
        (
            address %
            static_cast<uint32_t>(
                bytesPerRow
            )
        );


    refreshMemory();


    memoryDock->show();

    memoryDock->raise();
}


void MainWindow::jumpMemoryFromText()
{
    const QString text =
        memoryAddressEdit
            ->text()
            .trimmed();


    bool ok =
        false;


    const uint32_t address =
        text.toUInt(
            &ok,
            0
        );


    if (!ok) {
        statusBar()->showMessage(
            "Invalid memory address.",
            3000
        );

        return;
    }


    jumpMemoryToAddress(
        address
    );
}


void MainWindow::jumpMemoryToStackPointer()
{
    if (!session.hasProgram()) {
        statusBar()->showMessage(
            "No running program."
        );

        return;
    }


    jumpMemoryToAddress(
        session.getCPU()
            .registers[29]
    );
}


// ============================================================
// REFRESH SYMBOLS
// ============================================================

void MainWindow::refreshSymbols()
{
    symbolTable->setRowCount(
        0
    );


    const QVector<SourceSymbol>& symbols =
        sourceMap.symbols();


    for (
        int row = 0;
        row < symbols.size();
        row++
    ) {
        const SourceSymbol& symbol =
            symbols[row];


        symbolTable->insertRow(
            row
        );


        symbolTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                symbol.name
            )
        );


        symbolTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                toHex32(
                    symbol.address
                )
            )
        );


        symbolTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                symbol.section
            )
        );


        symbolTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                QString::number(
                    symbol.line
                )
            )
        );
    }
}


// ============================================================
// SYMBOL NAVIGATION
// ============================================================

void MainWindow::onSymbolActivated(
    int row,
    int
)
{
    const QVector<SourceSymbol>& symbols =
        sourceMap.symbols();


    if (
        row < 0 ||
        row >= symbols.size()
    ) {
        return;
    }


    const SourceSymbol& symbol =
        symbols[row];


    editor->goToLine(
        symbol.line
    );


    if (
        symbol.section ==
        ".data"
    ) {
        jumpMemoryToAddress(
            symbol.address
        );
    }
}


// ============================================================
// ACTION STATES
// ============================================================

void MainWindow::updateActionStates()
{
    const bool hasSource =
        !editor
            ->toPlainText()
            .trimmed()
            .isEmpty();


    const bool hasProgram =
        session.hasProgram();


    const bool halted =
        hasProgram &&
        session.getCPU().halted;


    actionAssemble->setEnabled(
        hasSource
    );


    // Source modifications invalidate the currently assembled
    // machine-code image from the GUI's point of view.
    actionRun->setEnabled(
        hasProgram &&
        !halted &&
        !documentModified
    );


    actionStep->setEnabled(
        hasProgram &&
        !halted &&
        !documentModified
    );


    actionReset->setEnabled(
        hasProgram
    );
}


// ============================================================
// OUTPUT
// ============================================================

void MainWindow::appendOutput(
    const QString& text
)
{
    if (text.isEmpty()) {
        return;
    }


    outputView->moveCursor(
        QTextCursor::End
    );


    outputView->insertPlainText(
        text
    );


    if (!text.endsWith('\n')) {
        outputView->insertPlainText(
            "\n"
        );
    }


    outputView->moveCursor(
        QTextCursor::End
    );
}


// ============================================================
// PROBLEMS
// ============================================================

void MainWindow::appendProblem(
    const QString& text
)
{
    if (text.isEmpty()) {
        return;
    }


    problemsView->appendPlainText(
        text.trimmed()
    );
}


// ============================================================
// DISASSEMBLER
// ============================================================

QString MainWindow::disassemble(
    uint32_t instruction,
    uint32_t address
) const
{
    const uint8_t opcode =
        static_cast<uint8_t>(
            (
                instruction >>
                26
            )
            &
            0x3F
        );


    const uint8_t rs =
        static_cast<uint8_t>(
            (
                instruction >>
                21
            )
            &
            0x1F
        );


    const uint8_t rt =
        static_cast<uint8_t>(
            (
                instruction >>
                16
            )
            &
            0x1F
        );


    const uint8_t rd =
        static_cast<uint8_t>(
            (
                instruction >>
                11
            )
            &
            0x1F
        );


    const uint8_t shamt =
        static_cast<uint8_t>(
            (
                instruction >>
                6
            )
            &
            0x1F
        );


    const uint8_t funct =
        static_cast<uint8_t>(
            instruction &
            0x3F
        );


    const uint16_t immediate =
        static_cast<uint16_t>(
            instruction &
            0xFFFF
        );


    const int16_t signedImmediate =
        static_cast<int16_t>(
            immediate
        );


    const uint32_t target =
        instruction &
        0x03FFFFFFU;


    auto reg =
        [](
            uint8_t index
        ) {
            return QString(
                REGISTER_NAMES[index]
            );
        };


    auto branchTarget =
        [&]() {
            const int64_t offsetBytes =
                static_cast<int64_t>(
                    signedImmediate
                )
                *
                4;


            return static_cast<uint32_t>(
                static_cast<int64_t>(
                    address +
                    4
                )
                +
                offsetBytes
            );
        };


    auto jumpTarget =
        [&]() {
            return
                (
                    (
                        address +
                        4
                    )
                    &
                    0xF0000000U
                )
                |
                (
                    target <<
                    2
                );
        };


    // ========================================================
    // SPECIAL
    // ========================================================

    if (
        opcode ==
        0x00
    ) {
        if (
            instruction ==
            0
        ) {
            return "nop";
        }


        switch (
            funct
        ) {
            case 0x00:
                return QString(
                    "sll %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rt))
                .arg(shamt);


            case 0x02:
                return QString(
                    "srl %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rt))
                .arg(shamt);


            case 0x03:
                return QString(
                    "sra %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rt))
                .arg(shamt);


            case 0x04:
                return QString(
                    "sllv %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rt))
                .arg(reg(rs));


            case 0x06:
                return QString(
                    "srlv %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rt))
                .arg(reg(rs));


            case 0x07:
                return QString(
                    "srav %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rt))
                .arg(reg(rs));


            case 0x08:
                return QString(
                    "jr %1"
                )
                .arg(reg(rs));


            case 0x09:
                return QString(
                    "jalr %1, %2"
                )
                .arg(reg(rd))
                .arg(reg(rs));


            case 0x0A:
                return QString(
                    "movz %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x0B:
                return QString(
                    "movn %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x0C: {
                const uint32_t code =
                    (
                        instruction >>
                        6
                    )
                    &
                    0xFFFFFU;


                return
                    code == 0
                    ? QString(
                          "syscall"
                      )
                    : QString(
                          "syscall 0x%1"
                      )
                      .arg(
                          code,
                          0,
                          16
                      )
                      .toUpper();
            }


            case 0x0D: {
                const uint32_t code =
                    (
                        instruction >>
                        6
                    )
                    &
                    0xFFFFFU;


                return
                    code == 0
                    ? QString(
                          "break"
                      )
                    : QString(
                          "break 0x%1"
                      )
                      .arg(
                          code,
                          0,
                          16
                      )
                      .toUpper();
            }


            case 0x10:
                return QString(
                    "mfhi %1"
                )
                .arg(reg(rd));


            case 0x11:
                return QString(
                    "mthi %1"
                )
                .arg(reg(rs));


            case 0x12:
                return QString(
                    "mflo %1"
                )
                .arg(reg(rd));


            case 0x13:
                return QString(
                    "mtlo %1"
                )
                .arg(reg(rs));


            case 0x18:
                return QString(
                    "mult %1, %2"
                )
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x19:
                return QString(
                    "multu %1, %2"
                )
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x1A:
                return QString(
                    "div %1, %2"
                )
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x1B:
                return QString(
                    "divu %1, %2"
                )
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x20:
                return QString(
                    "add %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x21:
                return QString(
                    "addu %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x22:
                return QString(
                    "sub %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x23:
                return QString(
                    "subu %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x24:
                return QString(
                    "and %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x25:
                return QString(
                    "or %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x26:
                return QString(
                    "xor %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x27:
                return QString(
                    "nor %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x2A:
                return QString(
                    "slt %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x2B:
                return QString(
                    "sltu %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            default:
                return QString(
                    ".word %1"
                )
                .arg(
                    toHex32(
                        instruction
                    )
                );
        }
    }


    // ========================================================
    // REGIMM
    // ========================================================

    if (
        opcode ==
        0x01
    ) {
        const QString destination =
            toHex32(
                branchTarget()
            );


        switch (
            rt
        ) {
            case 0x00:

                return QString(
                    "bltz %1, %2"
                )
                .arg(
                    reg(rs),
                    destination
                );


            case 0x01:

                return QString(
                    "bgez %1, %2"
                )
                .arg(
                    reg(rs),
                    destination
                );


            case 0x10:

                return QString(
                    "bltzal %1, %2"
                )
                .arg(
                    reg(rs),
                    destination
                );


            case 0x11:

                return QString(
                    "bgezal %1, %2"
                )
                .arg(
                    reg(rs),
                    destination
                );


            default:

                return QString(
                    ".word %1"
                )
                .arg(
                    toHex32(
                        instruction
                    )
                );
        }
    }


    // ========================================================
    // SPECIAL2
    // ========================================================

    if (
        opcode ==
        0x1C
    ) {
        switch (
            funct
        ) {
            case 0x02:

                return QString(
                    "mul %1, %2, %3"
                )
                .arg(reg(rd))
                .arg(reg(rs))
                .arg(reg(rt));


            case 0x20:

                return QString(
                    "clz %1, %2"
                )
                .arg(reg(rd))
                .arg(reg(rs));


            case 0x21:

                return QString(
                    "clo %1, %2"
                )
                .arg(reg(rd))
                .arg(reg(rs));


            default:

                return QString(
                    ".word %1"
                )
                .arg(
                    toHex32(
                        instruction
                    )
                );
        }
    }


    // ========================================================
    // I / J
    // ========================================================

    switch (
        opcode
    ) {
        case 0x02:

            return QString(
                "j %1"
            )
            .arg(
                toHex32(
                    jumpTarget()
                )
            );


        case 0x03:

            return QString(
                "jal %1"
            )
            .arg(
                toHex32(
                    jumpTarget()
                )
            );


        case 0x04:

            return QString(
                "beq %1, %2, %3"
            )
            .arg(reg(rs))
            .arg(reg(rt))
            .arg(
                toHex32(
                    branchTarget()
                )
            );


        case 0x05:

            return QString(
                "bne %1, %2, %3"
            )
            .arg(reg(rs))
            .arg(reg(rt))
            .arg(
                toHex32(
                    branchTarget()
                )
            );


        case 0x06:

            return QString(
                "blez %1, %2"
            )
            .arg(reg(rs))
            .arg(
                toHex32(
                    branchTarget()
                )
            );


        case 0x07:

            return QString(
                "bgtz %1, %2"
            )
            .arg(reg(rs))
            .arg(
                toHex32(
                    branchTarget()
                )
            );


        case 0x08:

            return QString(
                "addi %1, %2, %3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(signedImmediate);


        case 0x09:

            return QString(
                "addiu %1, %2, %3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(signedImmediate);


        case 0x0A:

            return QString(
                "slti %1, %2, %3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(signedImmediate);


        case 0x0B:

            return QString(
                "sltiu %1, %2, %3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(signedImmediate);


        case 0x0C:

            return QString(
                "andi %1, %2, 0x%3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(
                immediate,
                4,
                16,
                QChar('0')
            )
            .toUpper();


        case 0x0D:

            return QString(
                "ori %1, %2, 0x%3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(
                immediate,
                4,
                16,
                QChar('0')
            )
            .toUpper();


        case 0x0E:

            return QString(
                "xori %1, %2, 0x%3"
            )
            .arg(reg(rt))
            .arg(reg(rs))
            .arg(
                immediate,
                4,
                16,
                QChar('0')
            )
            .toUpper();


        case 0x0F:

            return QString(
                "lui %1, 0x%2"
            )
            .arg(reg(rt))
            .arg(
                immediate,
                4,
                16,
                QChar('0')
            )
            .toUpper();


        case 0x20:

            return QString(
                "lb %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x21:

            return QString(
                "lh %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x23:

            return QString(
                "lw %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x24:

            return QString(
                "lbu %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x25:

            return QString(
                "lhu %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x28:

            return QString(
                "sb %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x29:

            return QString(
                "sh %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        case 0x2B:

            return QString(
                "sw %1, %2(%3)"
            )
            .arg(reg(rt))
            .arg(signedImmediate)
            .arg(reg(rs));


        default:

            return QString(
                ".word %1"
            )
            .arg(
                toHex32(
                    instruction
                )
            );
    }
}


// ============================================================
// RESET LAYOUT
// ============================================================

void MainWindow::resetLayout()
{
    registersDock->show();
    cpuStateDock->show();
    disassemblyDock->show();
    memoryDock->show();
    consoleDock->show();
    symbolsDock->show();


    addDockWidget(
        Qt::LeftDockWidgetArea,
        symbolsDock
    );


    addDockWidget(
        Qt::RightDockWidgetArea,
        registersDock
    );


    addDockWidget(
        Qt::RightDockWidgetArea,
        cpuStateDock
    );


    splitDockWidget(
        registersDock,
        cpuStateDock,
        Qt::Vertical
    );


    addDockWidget(
        Qt::BottomDockWidgetArea,
        disassemblyDock
    );


    addDockWidget(
        Qt::BottomDockWidgetArea,
        memoryDock
    );


    splitDockWidget(
        disassemblyDock,
        memoryDock,
        Qt::Horizontal
    );


    addDockWidget(
        Qt::BottomDockWidgetArea,
        consoleDock
    );


    tabifyDockWidget(
        disassemblyDock,
        consoleDock
    );


    disassemblyDock->raise();


    resizeDocks(
        {
            registersDock,
            cpuStateDock
        },
        {
            560,
            230
        },
        Qt::Vertical
    );


    resizeDocks(
        {
            symbolsDock
        },
        {
            230
        },
        Qt::Horizontal
    );


    statusBar()->showMessage(
        "Workspace layout reset.",
        2500
    );
}


// ============================================================
// WINDOW SETTINGS
// ============================================================

void MainWindow::loadWindowSettings()
{
    QSettings qsettings(
        QSettings::defaultFormat(),
        QSettings::UserScope,
        "MIPSStudio",
        "MIPSStudio"
    );


    if (
        qsettings.contains(
            "windowGeometry"
        )
    ) {
        restoreGeometry(
            qsettings
                .value(
                    "windowGeometry"
                )
                .toByteArray()
        );
    }


    if (
        qsettings.contains(
            "windowState"
        )
    ) {
        restoreState(
            qsettings
                .value(
                    "windowState"
                )
                .toByteArray()
        );
    }
}


void MainWindow::saveWindowSettings()
{
    QSettings qsettings(
        QSettings::defaultFormat(),
        QSettings::UserScope,
        "MIPSStudio",
        "MIPSStudio"
    );


    qsettings.setValue(
        "windowGeometry",
        saveGeometry()
    );


    qsettings.setValue(
        "windowState",
        saveState()
    );
}


// ============================================================
// CLOSE EVENT
// ============================================================

void MainWindow::closeEvent(
    QCloseEvent* event
)
{
    if (!maybeSave()) {
        event->ignore();

        return;
    }


    saveWindowSettings();


    event->accept();
}
