#ifndef MIPSSTUDIO_SETTINGSDIALOG_H
#define MIPSSTUDIO_SETTINGSDIALOG_H

#include <QDialog>

#include "gui/AppSettings.h"


class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;


// ============================================================
// SettingsDialog
// ------------------------------------------------------------
// Preferences window.
//
// OK:
//     Save settings + apply them + close.
//
// Apply:
//     Save settings + apply them WITHOUT closing.
//
// Cancel:
//     Close without saving changes made since the last Apply.
// ============================================================

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(
        AppSettings& settings,
        QWidget* parent = nullptr
    );


signals:
    // MainWindow listens to this so settings can be previewed
    // immediately without closing the dialog.
    void settingsApplied();


private slots:
    void acceptSettings();
    void applySettingsWithoutClosing();

    void chooseAccentColor();

    void resetDefaults();


private:
    AppSettings& settings;


    // ========================================================
    // APPEARANCE
    // ========================================================

    QComboBox* themeCombo = nullptr;

    QPushButton* accentButton = nullptr;


    // ========================================================
    // EDITOR
    // ========================================================

    QSpinBox* fontSizeSpin = nullptr;
    QSpinBox* tabWidthSpin = nullptr;

    QCheckBox* lineNumbersCheck = nullptr;
    QCheckBox* currentLineCheck = nullptr;


    // ========================================================
    // SYNTAX COLORS
    // ========================================================

    QComboBox* syntaxProfileCombo = nullptr;

    QPushButton* normalColorButton = nullptr;
    QPushButton* instructionColorButton = nullptr;
    QPushButton* registerColorButton = nullptr;
    QPushButton* directiveColorButton = nullptr;
    QPushButton* labelColorButton = nullptr;
    QPushButton* commentColorButton = nullptr;
    QPushButton* stringColorButton = nullptr;
    QPushButton* numberColorButton = nullptr;


    // ========================================================
    // DEBUGGER
    // ========================================================

    QCheckBox* changedRegistersCheck = nullptr;
    QCheckBox* autoScrollCheck = nullptr;

    QComboBox* debuggerProfileCombo = nullptr;

    QPushButton* breakpointColorButton = nullptr;
    QPushButton* executionColorButton = nullptr;
    QPushButton* changedRegisterColorButton = nullptr;
    QPushButton* currentLineColorButton = nullptr;


    // ========================================================
    // MEMORY
    // ========================================================

    QComboBox* bytesPerRowCombo = nullptr;


    // ========================================================
    // EXECUTION
    // ========================================================

    QSpinBox* maxInstructionsSpin = nullptr;


    // ========================================================
    // UI
    // ========================================================

    void buildUI();

    void loadValues();

    void loadSyntaxColors();

    void loadDebuggerColors();


    // Save current widgets into AppSettings.
    void saveValues();


    // Synchronize "theme profile" selectors.
    void synchronizeProfilesFromTheme();

    void synchronizeThemeFromSyntaxProfile();

    void synchronizeThemeFromDebuggerProfile();


    // ========================================================
    // STEPPER CONTROL
    // ========================================================

    QWidget* createStepper(
        QSpinBox* spinBox,
        QWidget* parent
    );


    // ========================================================
    // COLORS
    // ========================================================

    QString syntaxProfile() const;

    QString debuggerProfile() const;


    void chooseColor(
        QPushButton* button
    );


    QString buttonColor(
        QPushButton* button
    ) const;


    void setButtonColor(
        QPushButton* button,
        const QString& color
    );
};

#endif