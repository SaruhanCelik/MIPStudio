#include "gui/AppSettings.h"

#include <QSettings>


namespace {

QSettings createSettings() {
    return QSettings(
        QSettings::defaultFormat(),
        QSettings::UserScope,
        "MIPSStudio",
        "MIPSStudio"
    );
}


QString normalizedProfile(
    const QString& profile
) {
    return profile.toLower() == "light"
        ? "light"
        : "dark";
}

}


// ============================================================
// CONSTRUCTOR
// ============================================================

AppSettings::AppSettings()
{
}


// ============================================================
// APPEARANCE
// ============================================================

QString AppSettings::theme() const {
    QSettings s = createSettings();

    return s.value(
        "appearance/theme",
        "dark"
    ).toString();
}


void AppSettings::setTheme(
    const QString& value
) {
    QSettings s = createSettings();

    s.setValue(
        "appearance/theme",
        value
    );
}


QString AppSettings::accentColor() const {
    QSettings s = createSettings();

    return s.value(
        "appearance/accentColor",
        "#2F81F7"
    ).toString();
}


void AppSettings::setAccentColor(
    const QString& value
) {
    QSettings s = createSettings();

    s.setValue(
        "appearance/accentColor",
        value
    );
}


// ============================================================
// EDITOR
// ============================================================

int AppSettings::editorFontSize() const {
    QSettings s = createSettings();

    return s.value(
        "editor/fontSize",
        11
    ).toInt();
}


void AppSettings::setEditorFontSize(
    int value
) {
    QSettings s = createSettings();

    s.setValue(
        "editor/fontSize",
        value
    );
}


int AppSettings::tabWidth() const {
    QSettings s = createSettings();

    return s.value(
        "editor/tabWidth",
        4
    ).toInt();
}


void AppSettings::setTabWidth(
    int value
) {
    QSettings s = createSettings();

    s.setValue(
        "editor/tabWidth",
        value
    );
}


bool AppSettings::showLineNumbers() const {
    QSettings s = createSettings();

    return s.value(
        "editor/showLineNumbers",
        true
    ).toBool();
}


void AppSettings::setShowLineNumbers(
    bool value
) {
    QSettings s = createSettings();

    s.setValue(
        "editor/showLineNumbers",
        value
    );
}


bool AppSettings::highlightCurrentLine() const {
    QSettings s = createSettings();

    return s.value(
        "editor/highlightCurrentLine",
        true
    ).toBool();
}


void AppSettings::setHighlightCurrentLine(
    bool value
) {
    QSettings s = createSettings();

    s.setValue(
        "editor/highlightCurrentLine",
        value
    );
}


// ============================================================
// DEBUGGER
// ============================================================

bool AppSettings::highlightChangedRegisters() const {
    QSettings s = createSettings();

    return s.value(
        "debugger/highlightChangedRegisters",
        true
    ).toBool();
}


void AppSettings::setHighlightChangedRegisters(
    bool value
) {
    QSettings s = createSettings();

    s.setValue(
        "debugger/highlightChangedRegisters",
        value
    );
}


bool AppSettings::autoScrollToPC() const {
    QSettings s = createSettings();

    return s.value(
        "debugger/autoScrollToPC",
        true
    ).toBool();
}


void AppSettings::setAutoScrollToPC(
    bool value
) {
    QSettings s = createSettings();

    s.setValue(
        "debugger/autoScrollToPC",
        value
    );
}


// ============================================================
// MEMORY
// ============================================================

int AppSettings::memoryBytesPerRow() const {
    QSettings s = createSettings();

    return s.value(
        "memory/bytesPerRow",
        4
    ).toInt();
}


void AppSettings::setMemoryBytesPerRow(
    int value
) {
    QSettings s = createSettings();

    s.setValue(
        "memory/bytesPerRow",
        value
    );
}


uint32_t AppSettings::defaultMemoryAddress() const {
    QSettings s = createSettings();

    return s.value(
        "memory/defaultAddress",
        0x1000
    ).toUInt();
}


void AppSettings::setDefaultMemoryAddress(
    uint32_t value
) {
    QSettings s = createSettings();

    s.setValue(
        "memory/defaultAddress",
        value
    );
}


// ============================================================
// EXECUTION
// ============================================================

int AppSettings::maxInstructionsPerRun() const {
    QSettings s = createSettings();

    return s.value(
        "execution/maxInstructions",
        100000
    ).toInt();
}


void AppSettings::setMaxInstructionsPerRun(
    int value
) {
    QSettings s = createSettings();

    s.setValue(
        "execution/maxInstructions",
        value
    );
}


// ============================================================
// DEFAULT SYNTAX PALETTES
// ============================================================

QString AppSettings::defaultSyntaxColor(
    const QString& profile,
    const QString& role
) const {

    const bool light =
        normalizedProfile(profile) == "light";


    if (light) {
        if (role == "normal")      return "#24292F";
        if (role == "instruction") return "#0969DA";
        if (role == "register")    return "#8250DF";
        if (role == "directive")   return "#BC4C00";
        if (role == "label")       return "#1A7F37";
        if (role == "comment")     return "#6E7781";
        if (role == "string")      return "#0A7A33";
        if (role == "number")      return "#6639BA";
    }


    // Existing dark palette stays our default identity.
    if (role == "normal")      return "#E6EDF3";
    if (role == "instruction") return "#79C0FF";
    if (role == "register")    return "#D2A8FF";
    if (role == "directive")   return "#FFA657";
    if (role == "label")       return "#7EE787";
    if (role == "comment")     return "#8B949E";
    if (role == "string")      return "#A5D6FF";
    if (role == "number")      return "#FF7B72";

    return "#E6EDF3";
}


// ============================================================
// SYNTAX COLOR HELPERS
// ============================================================

#define SYNTAX_GETTER(NAME, ROLE)                                      \
QString AppSettings::NAME(const QString& profile) const {              \
    QSettings s = createSettings();                                    \
    QString p = normalizedProfile(profile);                            \
    return s.value(                                                    \
        "syntax/" + p + "/" + ROLE,                                    \
        defaultSyntaxColor(p, ROLE)                                    \
    ).toString();                                                      \
}

#define SYNTAX_SETTER(NAME, ROLE)                                      \
void AppSettings::NAME(                                                \
    const QString& profile,                                            \
    const QString& value                                               \
) {                                                                    \
    QSettings s = createSettings();                                    \
    QString p = normalizedProfile(profile);                            \
    s.setValue("syntax/" + p + "/" + ROLE, value);                     \
}


SYNTAX_GETTER(syntaxNormalColor, "normal")
SYNTAX_SETTER(setSyntaxNormalColor, "normal")

SYNTAX_GETTER(syntaxInstructionColor, "instruction")
SYNTAX_SETTER(setSyntaxInstructionColor, "instruction")

SYNTAX_GETTER(syntaxRegisterColor, "register")
SYNTAX_SETTER(setSyntaxRegisterColor, "register")

SYNTAX_GETTER(syntaxDirectiveColor, "directive")
SYNTAX_SETTER(setSyntaxDirectiveColor, "directive")

SYNTAX_GETTER(syntaxLabelColor, "label")
SYNTAX_SETTER(setSyntaxLabelColor, "label")

SYNTAX_GETTER(syntaxCommentColor, "comment")
SYNTAX_SETTER(setSyntaxCommentColor, "comment")

SYNTAX_GETTER(syntaxStringColor, "string")
SYNTAX_SETTER(setSyntaxStringColor, "string")

SYNTAX_GETTER(syntaxNumberColor, "number")
SYNTAX_SETTER(setSyntaxNumberColor, "number")


#undef SYNTAX_GETTER
#undef SYNTAX_SETTER


void AppSettings::resetSyntaxColors(
    const QString& profile
) {
    QSettings s = createSettings();

    s.remove(
        "syntax/" +
        normalizedProfile(profile)
    );
}


// ============================================================
// DEBUG VISUAL DEFAULTS
// ============================================================

QString AppSettings::defaultDebuggerColor(
    const QString& profile,
    const QString& role
) const {

    const bool light =
        normalizedProfile(profile) == "light";


    if (light) {
        if (role == "breakpoint")      return "#CF222E";
        if (role == "execution")       return "#0969DA";
        if (role == "changedRegister") return "#DDF4E4";
        if (role == "currentLine")     return "#EAF2FF";
    }


    if (role == "breakpoint")      return "#F85149";
    if (role == "execution")       return "#58A6FF";
    if (role == "changedRegister") return "#23362B";
    if (role == "currentLine")     return "#182331";

    return "#58A6FF";
}


#define DEBUG_GETTER(NAME, ROLE)                                       \
QString AppSettings::NAME(const QString& profile) const {              \
    QSettings s = createSettings();                                    \
    QString p = normalizedProfile(profile);                            \
    return s.value(                                                    \
        "debugColors/" + p + "/" + ROLE,                               \
        defaultDebuggerColor(p, ROLE)                                  \
    ).toString();                                                      \
}

#define DEBUG_SETTER(NAME, ROLE)                                       \
void AppSettings::NAME(                                                \
    const QString& profile,                                            \
    const QString& value                                               \
) {                                                                    \
    QSettings s = createSettings();                                    \
    QString p = normalizedProfile(profile);                            \
    s.setValue("debugColors/" + p + "/" + ROLE, value);                \
}


DEBUG_GETTER(breakpointColor, "breakpoint")
DEBUG_SETTER(setBreakpointColor, "breakpoint")

DEBUG_GETTER(executionColor, "execution")
DEBUG_SETTER(setExecutionColor, "execution")

DEBUG_GETTER(changedRegisterColor, "changedRegister")
DEBUG_SETTER(setChangedRegisterColor, "changedRegister")

DEBUG_GETTER(editorCurrentLineColor, "currentLine")
DEBUG_SETTER(setEditorCurrentLineColor, "currentLine")


#undef DEBUG_GETTER
#undef DEBUG_SETTER


void AppSettings::resetDebuggerColors(
    const QString& profile
) {
    QSettings s = createSettings();

    s.remove(
        "debugColors/" +
        normalizedProfile(profile)
    );
}


// ============================================================
// RESET EVERYTHING
// ============================================================

void AppSettings::resetToDefaults() {
    QSettings s = createSettings();

    s.remove("appearance");
    s.remove("editor");
    s.remove("debugger");
    s.remove("memory");
    s.remove("execution");
    s.remove("syntax");
    s.remove("debugColors");
}
