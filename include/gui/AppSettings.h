#ifndef MIPSSTUDIO_APPSETTINGS_H
#define MIPSSTUDIO_APPSETTINGS_H

#include <cstdint>
#include <QString>


// ============================================================
// AppSettings
// ------------------------------------------------------------
// Persistent GUI preferences.
//
// Color settings are stored separately for Dark and Light
// profiles so changing one theme does not destroy the other.
// ============================================================

class AppSettings {
public:
    AppSettings();


    // ========================================================
    // APPEARANCE
    // ========================================================

    QString theme() const;
    void setTheme(const QString& value);

    QString accentColor() const;
    void setAccentColor(const QString& value);


    // ========================================================
    // EDITOR
    // ========================================================

    int editorFontSize() const;
    void setEditorFontSize(int value);

    int tabWidth() const;
    void setTabWidth(int value);

    bool showLineNumbers() const;
    void setShowLineNumbers(bool value);

    bool highlightCurrentLine() const;
    void setHighlightCurrentLine(bool value);


    // ========================================================
    // DEBUGGER
    // ========================================================

    bool highlightChangedRegisters() const;
    void setHighlightChangedRegisters(bool value);

    bool autoScrollToPC() const;
    void setAutoScrollToPC(bool value);


    // ========================================================
    // MEMORY
    // ========================================================

    int memoryBytesPerRow() const;
    void setMemoryBytesPerRow(int value);

    uint32_t defaultMemoryAddress() const;
    void setDefaultMemoryAddress(uint32_t value);


    // ========================================================
    // EXECUTION
    // ========================================================

    int maxInstructionsPerRun() const;
    void setMaxInstructionsPerRun(int value);


    // ========================================================
    // SYNTAX COLORS
    // ========================================================

    QString syntaxNormalColor(const QString& profile) const;
    void setSyntaxNormalColor(const QString& profile, const QString& value);

    QString syntaxInstructionColor(const QString& profile) const;
    void setSyntaxInstructionColor(const QString& profile, const QString& value);

    QString syntaxRegisterColor(const QString& profile) const;
    void setSyntaxRegisterColor(const QString& profile, const QString& value);

    QString syntaxDirectiveColor(const QString& profile) const;
    void setSyntaxDirectiveColor(const QString& profile, const QString& value);

    QString syntaxLabelColor(const QString& profile) const;
    void setSyntaxLabelColor(const QString& profile, const QString& value);

    QString syntaxCommentColor(const QString& profile) const;
    void setSyntaxCommentColor(const QString& profile, const QString& value);

    QString syntaxStringColor(const QString& profile) const;
    void setSyntaxStringColor(const QString& profile, const QString& value);

    QString syntaxNumberColor(const QString& profile) const;
    void setSyntaxNumberColor(const QString& profile, const QString& value);

    void resetSyntaxColors(const QString& profile);


    // ========================================================
    // DEBUG VISUAL COLORS
    // ========================================================

    QString breakpointColor(const QString& profile) const;
    void setBreakpointColor(const QString& profile, const QString& value);

    QString executionColor(const QString& profile) const;
    void setExecutionColor(const QString& profile, const QString& value);

    QString changedRegisterColor(const QString& profile) const;
    void setChangedRegisterColor(const QString& profile, const QString& value);

    QString editorCurrentLineColor(const QString& profile) const;
    void setEditorCurrentLineColor(const QString& profile, const QString& value);

    void resetDebuggerColors(const QString& profile);


    // ========================================================
    // RESET EVERYTHING
    // ========================================================

    void resetToDefaults();


private:
    QString defaultSyntaxColor(
        const QString& profile,
        const QString& role
    ) const;

    QString defaultDebuggerColor(
        const QString& profile,
        const QString& role
    ) const;
};

#endif