#include "gui/SettingsDialog.h"

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>


// ============================================================
// CONSTRUCTOR
// ============================================================

SettingsDialog::SettingsDialog(
    AppSettings& settings,
    QWidget* parent
)
    : QDialog(parent),
      settings(settings)
{
    buildUI();

    loadValues();


    setWindowTitle(
        "Preferences — MIPS Studio"
    );


    resize(
        680,
        560
    );


    setMinimumSize(
        620,
        500
    );
}


// ============================================================
// CREATE EXPLICIT - / + STEPPER
// ------------------------------------------------------------
// Instead of relying on tiny native QSpinBox arrows, MIPS
// Studio uses obvious minus and plus buttons.
// ============================================================

QWidget* SettingsDialog::createStepper(
    QSpinBox* spinBox,
    QWidget* parent
) {
    QWidget* container =
        new QWidget(parent);


    QHBoxLayout* layout =
        new QHBoxLayout(container);


    layout->setContentsMargins(
        0,
        0,
        0,
        0
    );


    layout->setSpacing(
        6
    );


    // Hide the confusing built-in arrows.
    spinBox->setButtonSymbols(
        QAbstractSpinBox::NoButtons
    );


    QPushButton* minusButton =
        new QPushButton(
            "−",
            container
        );


    QPushButton* plusButton =
        new QPushButton(
            "+",
            container
        );


    minusButton->setObjectName(
        "StepperButton"
    );


    plusButton->setObjectName(
        "StepperButton"
    );


    minusButton->setToolTip(
        "Decrease"
    );


    plusButton->setToolTip(
        "Increase"
    );


    minusButton->setFixedSize(
        38,
        34
    );


    plusButton->setFixedSize(
        38,
        34
    );


    spinBox->setMinimumHeight(
        34
    );


    layout->addWidget(
        spinBox,
        1
    );


    layout->addWidget(
        minusButton
    );


    layout->addWidget(
        plusButton
    );


    connect(
        minusButton,
        &QPushButton::clicked,
        spinBox,
        &QSpinBox::stepDown
    );


    connect(
        plusButton,
        &QPushButton::clicked,
        spinBox,
        &QSpinBox::stepUp
    );


    return container;
}


// ============================================================
// BUILD UI
// ============================================================

void SettingsDialog::buildUI()
{
    QVBoxLayout* mainLayout =
        new QVBoxLayout(this);


    QTabWidget* tabs =
        new QTabWidget(this);


    // ========================================================
    // APPEARANCE
    // ========================================================

    QWidget* appearanceTab =
        new QWidget(tabs);


    QFormLayout* appearanceLayout =
        new QFormLayout(
            appearanceTab
        );


    themeCombo =
        new QComboBox(
            appearanceTab
        );


    themeCombo->addItem(
        "Dark",
        "dark"
    );


    themeCombo->addItem(
        "Light",
        "light"
    );


    themeCombo->addItem(
        "System",
        "system"
    );


    accentButton =
        new QPushButton(
            appearanceTab
        );


    appearanceLayout->addRow(
        "Theme:",
        themeCombo
    );


    appearanceLayout->addRow(
        "UI accent:",
        accentButton
    );


    connect(
        accentButton,
        &QPushButton::clicked,
        this,
        &SettingsDialog::chooseAccentColor
    );


    // When the user explicitly chooses Dark/Light,
    // synchronize the color editors to that profile.
    connect(
        themeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            synchronizeProfilesFromTheme();
        }
    );


    // ========================================================
    // EDITOR
    // ========================================================

    QWidget* editorTab =
        new QWidget(tabs);


    QFormLayout* editorLayout =
        new QFormLayout(
            editorTab
        );


    fontSizeSpin =
        new QSpinBox(
            editorTab
        );


    fontSizeSpin->setRange(
        8,
        32
    );


    fontSizeSpin->setSingleStep(
        1
    );


    tabWidthSpin =
        new QSpinBox(
            editorTab
        );


    tabWidthSpin->setRange(
        1,
        12
    );


    tabWidthSpin->setSingleStep(
        1
    );


    editorLayout->addRow(
        "Font size:",
        createStepper(
            fontSizeSpin,
            editorTab
        )
    );


    editorLayout->addRow(
        "Tab width:",
        createStepper(
            tabWidthSpin,
            editorTab
        )
    );


    lineNumbersCheck =
        new QCheckBox(
            "Show line numbers",
            editorTab
        );


    currentLineCheck =
        new QCheckBox(
            "Highlight current editor line",
            editorTab
        );


    editorLayout->addRow(
        "",
        lineNumbersCheck
    );


    editorLayout->addRow(
        "",
        currentLineCheck
    );


    // ========================================================
    // SYNTAX COLORS
    // ========================================================

    QWidget* syntaxTab =
        new QWidget(tabs);


    QVBoxLayout* syntaxMain =
        new QVBoxLayout(
            syntaxTab
        );


    QLabel* syntaxHelp =
        new QLabel(
            "Choose the theme color set you want to edit. "
            "Selecting Dark or Light here also selects that "
            "theme in Appearance, so Apply gives an immediate preview.",
            syntaxTab
        );


    syntaxHelp->setWordWrap(
        true
    );


    syntaxHelp->setObjectName(
        "SettingsHint"
    );


    syntaxMain->addWidget(
        syntaxHelp
    );


    QFormLayout* syntaxLayout =
        new QFormLayout();


    syntaxProfileCombo =
        new QComboBox(
            syntaxTab
        );


    syntaxProfileCombo->addItem(
        "Dark theme colors",
        "dark"
    );


    syntaxProfileCombo->addItem(
        "Light theme colors",
        "light"
    );


    normalColorButton =
        new QPushButton(syntaxTab);

    instructionColorButton =
        new QPushButton(syntaxTab);

    registerColorButton =
        new QPushButton(syntaxTab);

    directiveColorButton =
        new QPushButton(syntaxTab);

    labelColorButton =
        new QPushButton(syntaxTab);

    commentColorButton =
        new QPushButton(syntaxTab);

    stringColorButton =
        new QPushButton(syntaxTab);

    numberColorButton =
        new QPushButton(syntaxTab);


    syntaxLayout->addRow(
        "Editing / preview profile:",
        syntaxProfileCombo
    );


    syntaxLayout->addRow(
        "Normal text:",
        normalColorButton
    );


    syntaxLayout->addRow(
        "Instructions:",
        instructionColorButton
    );


    syntaxLayout->addRow(
        "Registers:",
        registerColorButton
    );


    syntaxLayout->addRow(
        "Directives:",
        directiveColorButton
    );


    syntaxLayout->addRow(
        "Labels:",
        labelColorButton
    );


    syntaxLayout->addRow(
        "Comments:",
        commentColorButton
    );


    syntaxLayout->addRow(
        "Strings:",
        stringColorButton
    );


    syntaxLayout->addRow(
        "Numbers:",
        numberColorButton
    );


    syntaxMain->addLayout(
        syntaxLayout
    );


    QPushButton* resetSyntaxButton =
        new QPushButton(
            "Restore Default Syntax Colors",
            syntaxTab
        );


    syntaxMain->addWidget(
        resetSyntaxButton
    );


    syntaxMain->addStretch();


    connect(
        syntaxProfileCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            loadSyntaxColors();

            synchronizeThemeFromSyntaxProfile();
        }
    );


    QPushButton* syntaxButtons[] = {
        normalColorButton,
        instructionColorButton,
        registerColorButton,
        directiveColorButton,
        labelColorButton,
        commentColorButton,
        stringColorButton,
        numberColorButton
    };


    for (
        QPushButton* button :
        syntaxButtons
    ) {
        connect(
            button,
            &QPushButton::clicked,
            this,
            [this, button]() {
                chooseColor(button);
            }
        );
    }


    connect(
        resetSyntaxButton,
        &QPushButton::clicked,
        this,
        [this]() {
            settings.resetSyntaxColors(
                syntaxProfile()
            );

            loadSyntaxColors();
        }
    );


    // ========================================================
    // DEBUGGER
    // ========================================================

    QWidget* debuggerTab =
        new QWidget(tabs);


    QVBoxLayout* debuggerMain =
        new QVBoxLayout(
            debuggerTab
        );


    changedRegistersCheck =
        new QCheckBox(
            "Highlight registers changed by the last step/run",
            debuggerTab
        );


    autoScrollCheck =
        new QCheckBox(
            "Automatically scroll source/disassembly to PC",
            debuggerTab
        );


    debuggerMain->addWidget(
        changedRegistersCheck
    );


    debuggerMain->addWidget(
        autoScrollCheck
    );


    QGroupBox* debuggerColorsGroup =
        new QGroupBox(
            "Debugger Colors",
            debuggerTab
        );


    QFormLayout* debuggerColorsLayout =
        new QFormLayout(
            debuggerColorsGroup
        );


    debuggerProfileCombo =
        new QComboBox(
            debuggerColorsGroup
        );


    debuggerProfileCombo->addItem(
        "Dark theme colors",
        "dark"
    );


    debuggerProfileCombo->addItem(
        "Light theme colors",
        "light"
    );


    breakpointColorButton =
        new QPushButton(
            debuggerColorsGroup
        );


    executionColorButton =
        new QPushButton(
            debuggerColorsGroup
        );


    changedRegisterColorButton =
        new QPushButton(
            debuggerColorsGroup
        );


    currentLineColorButton =
        new QPushButton(
            debuggerColorsGroup
        );


    debuggerColorsLayout->addRow(
        "Editing / preview profile:",
        debuggerProfileCombo
    );


    debuggerColorsLayout->addRow(
        "Breakpoint:",
        breakpointColorButton
    );


    debuggerColorsLayout->addRow(
        "Current PC:",
        executionColorButton
    );


    debuggerColorsLayout->addRow(
        "Changed register:",
        changedRegisterColorButton
    );


    debuggerColorsLayout->addRow(
        "Editor current line:",
        currentLineColorButton
    );


    QPushButton* resetDebuggerButton =
        new QPushButton(
            "Restore Default Debugger Colors",
            debuggerColorsGroup
        );


    debuggerColorsLayout->addRow(
        "",
        resetDebuggerButton
    );


    debuggerMain->addWidget(
        debuggerColorsGroup
    );


    debuggerMain->addStretch();


    connect(
        debuggerProfileCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]() {
            loadDebuggerColors();

            synchronizeThemeFromDebuggerProfile();
        }
    );


    QPushButton* debuggerButtons[] = {
        breakpointColorButton,
        executionColorButton,
        changedRegisterColorButton,
        currentLineColorButton
    };


    for (
        QPushButton* button :
        debuggerButtons
    ) {
        connect(
            button,
            &QPushButton::clicked,
            this,
            [this, button]() {
                chooseColor(button);
            }
        );
    }


    connect(
        resetDebuggerButton,
        &QPushButton::clicked,
        this,
        [this]() {
            settings.resetDebuggerColors(
                debuggerProfile()
            );

            loadDebuggerColors();
        }
    );


    // ========================================================
    // MEMORY
    // ========================================================

    QWidget* memoryTab =
        new QWidget(tabs);


    QFormLayout* memoryLayout =
        new QFormLayout(
            memoryTab
        );


    bytesPerRowCombo =
        new QComboBox(
            memoryTab
        );


    bytesPerRowCombo->addItem(
        "4 bytes",
        4
    );


    bytesPerRowCombo->addItem(
        "8 bytes",
        8
    );


    bytesPerRowCombo->addItem(
        "16 bytes",
        16
    );


    memoryLayout->addRow(
        "Bytes per row:",
        bytesPerRowCombo
    );


    // ========================================================
    // EXECUTION
    // ========================================================

    QWidget* executionTab =
        new QWidget(tabs);


    QFormLayout* executionLayout =
        new QFormLayout(
            executionTab
        );


    maxInstructionsSpin =
        new QSpinBox(
            executionTab
        );


    maxInstructionsSpin->setRange(
        100,
        10000000
    );


    maxInstructionsSpin->setSingleStep(
        1000
    );


    executionLayout->addRow(
        "Maximum instructions per Run:",
        createStepper(
            maxInstructionsSpin,
            executionTab
        )
    );


    // ========================================================
    // ADD TABS
    // ========================================================

    tabs->addTab(
        appearanceTab,
        "Appearance"
    );


    tabs->addTab(
        editorTab,
        "Editor"
    );


    tabs->addTab(
        syntaxTab,
        "Syntax Colors"
    );


    tabs->addTab(
        debuggerTab,
        "Debugger"
    );


    tabs->addTab(
        memoryTab,
        "Memory"
    );


    tabs->addTab(
        executionTab,
        "Execution"
    );


    mainLayout->addWidget(
        tabs
    );


    // ========================================================
    // BOTTOM BUTTONS
    // ========================================================

    QHBoxLayout* bottomRow =
        new QHBoxLayout();


    QPushButton* resetButton =
        new QPushButton(
            "Reset All Settings",
            this
        );


    bottomRow->addWidget(
        resetButton
    );


    bottomRow->addStretch();


    QDialogButtonBox* buttons =
        new QDialogButtonBox(
            QDialogButtonBox::Ok |
            QDialogButtonBox::Apply |
            QDialogButtonBox::Cancel,
            this
        );


    bottomRow->addWidget(
        buttons
    );


    mainLayout->addLayout(
        bottomRow
    );


    connect(
        buttons,
        &QDialogButtonBox::accepted,
        this,
        &SettingsDialog::acceptSettings
    );


    connect(
        buttons,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
    );


    QPushButton* applyButton =
        buttons->button(
            QDialogButtonBox::Apply
        );


    applyButton->setObjectName(
        "PrimarySettingsButton"
    );


    connect(
        applyButton,
        &QPushButton::clicked,
        this,
        &SettingsDialog::applySettingsWithoutClosing
    );


    connect(
        resetButton,
        &QPushButton::clicked,
        this,
        &SettingsDialog::resetDefaults
    );
}


// ============================================================
// PROFILE SYNCHRONIZATION
// ============================================================

void SettingsDialog::synchronizeProfilesFromTheme()
{
    QString theme =
        themeCombo
            ->currentData()
            .toString();


    if (
        theme != "dark" &&
        theme != "light"
    ) {
        return;
    }


    {
        QSignalBlocker blocker(
            syntaxProfileCombo
        );


        int index =
            syntaxProfileCombo->findData(
                theme
            );


        if (index >= 0) {
            syntaxProfileCombo->setCurrentIndex(
                index
            );
        }
    }


    {
        QSignalBlocker blocker(
            debuggerProfileCombo
        );


        int index =
            debuggerProfileCombo->findData(
                theme
            );


        if (index >= 0) {
            debuggerProfileCombo->setCurrentIndex(
                index
            );
        }
    }


    loadSyntaxColors();

    loadDebuggerColors();
}


void SettingsDialog::synchronizeThemeFromSyntaxProfile()
{
    QString profile =
        syntaxProfile();


    int index =
        themeCombo->findData(
            profile
        );


    if (index >= 0) {
        QSignalBlocker blocker(
            themeCombo
        );


        themeCombo->setCurrentIndex(
            index
        );
    }


    // Keep debugger profile in sync too.
    int debuggerIndex =
        debuggerProfileCombo->findData(
            profile
        );


    if (debuggerIndex >= 0) {
        QSignalBlocker blocker(
            debuggerProfileCombo
        );


        debuggerProfileCombo->setCurrentIndex(
            debuggerIndex
        );


        loadDebuggerColors();
    }
}


void SettingsDialog::synchronizeThemeFromDebuggerProfile()
{
    QString profile =
        debuggerProfile();


    int index =
        themeCombo->findData(
            profile
        );


    if (index >= 0) {
        QSignalBlocker blocker(
            themeCombo
        );


        themeCombo->setCurrentIndex(
            index
        );
    }


    int syntaxIndex =
        syntaxProfileCombo->findData(
            profile
        );


    if (syntaxIndex >= 0) {
        QSignalBlocker blocker(
            syntaxProfileCombo
        );


        syntaxProfileCombo->setCurrentIndex(
            syntaxIndex
        );


        loadSyntaxColors();
    }
}


// ============================================================
// COLOR BUTTON
// ============================================================

void SettingsDialog::setButtonColor(
    QPushButton* button,
    const QString& colorText
)
{
    QColor color(
        colorText
    );


    if (!color.isValid()) {
        color =
            QColor("#808080");
    }


    QString foreground =
        color.lightness() > 150
            ? "#25221E"
            : "#FFFFFF";


    button->setProperty(
        "selectedColor",
        color.name(
            QColor::HexRgb
        )
    );


    button->setText(
        color
            .name(
                QColor::HexRgb
            )
            .toUpper()
    );


    // Dynamic property gives the global stylesheet a chance
    // to style the rest while this button shows its color.
    button->setStyleSheet(
        QString(
            "QPushButton {"
            "background-color: %1;"
            "color: %2;"
            "font-weight: 600;"
            "padding: 7px 12px;"
            "border: 1px solid rgba(128,128,128,0.55);"
            "border-radius: 5px;"
            "}"
        )
        .arg(
            color.name(
                QColor::HexRgb
            ),
            foreground
        )
    );
}


QString SettingsDialog::buttonColor(
    QPushButton* button
) const
{
    return button
        ->property(
            "selectedColor"
        )
        .toString();
}


void SettingsDialog::chooseColor(
    QPushButton* button
)
{
    QColor current(
        buttonColor(button)
    );


    QColor selected =
        QColorDialog::getColor(
            current,
            this,
            "Choose Color"
        );


    if (selected.isValid()) {
        setButtonColor(
            button,
            selected.name(
                QColor::HexRgb
            )
        );
    }
}


// ============================================================
// ACCENT
// ============================================================

void SettingsDialog::chooseAccentColor()
{
    chooseColor(
        accentButton
    );
}


// ============================================================
// PROFILE HELPERS
// ============================================================

QString SettingsDialog::syntaxProfile() const
{
    return syntaxProfileCombo
        ->currentData()
        .toString();
}


QString SettingsDialog::debuggerProfile() const
{
    return debuggerProfileCombo
        ->currentData()
        .toString();
}


// ============================================================
// LOAD SETTINGS
// ============================================================

void SettingsDialog::loadValues()
{
    int themeIndex =
        themeCombo->findData(
            settings.theme()
        );


    if (themeIndex >= 0) {
        themeCombo->setCurrentIndex(
            themeIndex
        );
    }


    setButtonColor(
        accentButton,
        settings.accentColor()
    );


    fontSizeSpin->setValue(
        settings.editorFontSize()
    );


    tabWidthSpin->setValue(
        settings.tabWidth()
    );


    lineNumbersCheck->setChecked(
        settings.showLineNumbers()
    );


    currentLineCheck->setChecked(
        settings.highlightCurrentLine()
    );


    changedRegistersCheck->setChecked(
        settings.highlightChangedRegisters()
    );


    autoScrollCheck->setChecked(
        settings.autoScrollToPC()
    );


    int memoryIndex =
        bytesPerRowCombo->findData(
            settings.memoryBytesPerRow()
        );


    if (memoryIndex >= 0) {
        bytesPerRowCombo->setCurrentIndex(
            memoryIndex
        );
    }


    maxInstructionsSpin->setValue(
        settings.maxInstructionsPerRun()
    );


    synchronizeProfilesFromTheme();

    loadSyntaxColors();

    loadDebuggerColors();
}


// ============================================================
// LOAD SYNTAX COLORS
// ============================================================

void SettingsDialog::loadSyntaxColors()
{
    QString p =
        syntaxProfile();


    setButtonColor(
        normalColorButton,
        settings.syntaxNormalColor(p)
    );


    setButtonColor(
        instructionColorButton,
        settings.syntaxInstructionColor(p)
    );


    setButtonColor(
        registerColorButton,
        settings.syntaxRegisterColor(p)
    );


    setButtonColor(
        directiveColorButton,
        settings.syntaxDirectiveColor(p)
    );


    setButtonColor(
        labelColorButton,
        settings.syntaxLabelColor(p)
    );


    setButtonColor(
        commentColorButton,
        settings.syntaxCommentColor(p)
    );


    setButtonColor(
        stringColorButton,
        settings.syntaxStringColor(p)
    );


    setButtonColor(
        numberColorButton,
        settings.syntaxNumberColor(p)
    );
}


// ============================================================
// LOAD DEBUGGER COLORS
// ============================================================

void SettingsDialog::loadDebuggerColors()
{
    QString p =
        debuggerProfile();


    setButtonColor(
        breakpointColorButton,
        settings.breakpointColor(p)
    );


    setButtonColor(
        executionColorButton,
        settings.executionColor(p)
    );


    setButtonColor(
        changedRegisterColorButton,
        settings.changedRegisterColor(p)
    );


    setButtonColor(
        currentLineColorButton,
        settings.editorCurrentLineColor(p)
    );
}


// ============================================================
// SAVE WIDGET VALUES
// ============================================================

void SettingsDialog::saveValues()
{
    settings.setTheme(
        themeCombo
            ->currentData()
            .toString()
    );


    settings.setAccentColor(
        buttonColor(
            accentButton
        )
    );


    settings.setEditorFontSize(
        fontSizeSpin->value()
    );


    settings.setTabWidth(
        tabWidthSpin->value()
    );


    settings.setShowLineNumbers(
        lineNumbersCheck->isChecked()
    );


    settings.setHighlightCurrentLine(
        currentLineCheck->isChecked()
    );


    settings.setHighlightChangedRegisters(
        changedRegistersCheck->isChecked()
    );


    settings.setAutoScrollToPC(
        autoScrollCheck->isChecked()
    );


    settings.setMemoryBytesPerRow(
        bytesPerRowCombo
            ->currentData()
            .toInt()
    );


    settings.setMaxInstructionsPerRun(
        maxInstructionsSpin->value()
    );


    // --------------------------------------------------------
    // Syntax profile currently being edited
    // --------------------------------------------------------

    QString syntaxP =
        syntaxProfile();


    settings.setSyntaxNormalColor(
        syntaxP,
        buttonColor(
            normalColorButton
        )
    );


    settings.setSyntaxInstructionColor(
        syntaxP,
        buttonColor(
            instructionColorButton
        )
    );


    settings.setSyntaxRegisterColor(
        syntaxP,
        buttonColor(
            registerColorButton
        )
    );


    settings.setSyntaxDirectiveColor(
        syntaxP,
        buttonColor(
            directiveColorButton
        )
    );


    settings.setSyntaxLabelColor(
        syntaxP,
        buttonColor(
            labelColorButton
        )
    );


    settings.setSyntaxCommentColor(
        syntaxP,
        buttonColor(
            commentColorButton
        )
    );


    settings.setSyntaxStringColor(
        syntaxP,
        buttonColor(
            stringColorButton
        )
    );


    settings.setSyntaxNumberColor(
        syntaxP,
        buttonColor(
            numberColorButton
        )
    );


    // --------------------------------------------------------
    // Debugger profile
    // --------------------------------------------------------

    QString debugP =
        debuggerProfile();


    settings.setBreakpointColor(
        debugP,
        buttonColor(
            breakpointColorButton
        )
    );


    settings.setExecutionColor(
        debugP,
        buttonColor(
            executionColorButton
        )
    );


    settings.setChangedRegisterColor(
        debugP,
        buttonColor(
            changedRegisterColorButton
        )
    );


    settings.setEditorCurrentLineColor(
        debugP,
        buttonColor(
            currentLineColorButton
        )
    );
}


// ============================================================
// APPLY
// ============================================================

void SettingsDialog::applySettingsWithoutClosing()
{
    saveValues();


    emit settingsApplied();
}


// ============================================================
// OK
// ============================================================

void SettingsDialog::acceptSettings()
{
    saveValues();


    emit settingsApplied();


    accept();
}


// ============================================================
// RESET
// ============================================================

void SettingsDialog::resetDefaults()
{
    settings.resetToDefaults();


    loadValues();


    // Show the reset immediately.
    emit settingsApplied();
}