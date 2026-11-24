#ifndef MIPSSTUDIO_GUI_CODEEDITOR_H
#define MIPSSTUDIO_GUI_CODEEDITOR_H

#include <QColor>
#include <QPlainTextEdit>
#include <QSet>
#include <QWidget>


class QEnterEvent;
class QEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

class CodeEditor;


// ============================================================
// LineNumberArea
// ------------------------------------------------------------
// Interactive gutter.
//
// Hovering it visibly highlights the gutter so users discover
// that it is clickable.
// ============================================================

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(
        CodeEditor* editor
    );


    QSize sizeHint() const override;


protected:
    void paintEvent(
        QPaintEvent* event
    ) override;


    void mousePressEvent(
        QMouseEvent* event
    ) override;


    void enterEvent(
        QEnterEvent* event
    ) override;


    void leaveEvent(
        QEvent* event
    ) override;


private:
    CodeEditor* codeEditor;
};


// ============================================================
// CodeEditor
// ============================================================

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(
        QWidget* parent = nullptr
    );


    int lineNumberAreaWidth() const;


    void lineNumberAreaPaintEvent(
        QPaintEvent* event
    );


    void gutterMousePress(
        QMouseEvent* event
    );


    void setGutterHovered(
        bool hovered
    );


    // ========================================================
    // BREAKPOINTS
    // ========================================================

    const QSet<int>&
    breakpointLines() const;


    void clearBreakpoints();


    // ========================================================
    // EXECUTION
    // ========================================================

    void setExecutionLine(
        int line
    );


    int executionLine() const;


    // ========================================================
    // EDITOR SETTINGS
    // ========================================================

    void setLineNumbersVisible(
        bool visible
    );


    void setCurrentLineHighlightEnabled(
        bool enabled
    );


    void setEditorFontSize(
        int pointSize
    );


    void setEditorTabWidth(
        int spaces
    );


    // ========================================================
    // THEME COLORS
    // ========================================================

    void setVisualColors(
        const QColor& gutterBackground,
        const QColor& gutterHoverBackground,
        const QColor& gutterText,
        const QColor& currentLine,
        const QColor& breakpoint,
        const QColor& execution
    );


    void goToLine(
        int line
    );


signals:
    void breakpointToggled(
        int line,
        bool enabled
    );


protected:
    void resizeEvent(
        QResizeEvent* event
    ) override;


private slots:
    void updateLineNumberAreaWidth(
        int newBlockCount
    );


    void highlightCurrentLine();


    void updateLineNumberArea(
        const QRect& rect,
        int dy
    );


private:
    LineNumberArea* lineNumberArea;


    QSet<int> breakpoints;


    int currentExecutionLine;


    bool lineNumbersVisible;

    bool currentLineHighlightEnabled;

    bool gutterHovered;


    QColor gutterBackgroundColor;

    QColor gutterHoverBackgroundColor;

    QColor gutterTextColor;

    QColor currentLineColor;

    QColor breakpointColor;

    QColor executionColor;


    int lineAtY(
        int y
    ) const;
};

#endif