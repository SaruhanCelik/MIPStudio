#include "gui/CodeEditor.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>


// ============================================================
// LINE NUMBER AREA
// ============================================================

LineNumberArea::LineNumberArea(
    CodeEditor* editor
)
    : QWidget(editor),
      codeEditor(editor)
{
    setCursor(
        Qt::PointingHandCursor
    );


    setToolTip(
        "Click beside an executable line to toggle a breakpoint."
    );
}


QSize LineNumberArea::sizeHint() const
{
    return QSize(
        codeEditor->lineNumberAreaWidth(),
        0
    );
}


void LineNumberArea::paintEvent(
    QPaintEvent* event
)
{
    codeEditor->lineNumberAreaPaintEvent(
        event
    );
}


void LineNumberArea::mousePressEvent(
    QMouseEvent* event
)
{
    codeEditor->gutterMousePress(
        event
    );
}


void LineNumberArea::enterEvent(
    QEnterEvent* event
)
{
    codeEditor->setGutterHovered(
        true
    );


    QWidget::enterEvent(
        event
    );
}


void LineNumberArea::leaveEvent(
    QEvent* event
)
{
    codeEditor->setGutterHovered(
        false
    );


    QWidget::leaveEvent(
        event
    );
}


// ============================================================
// CONSTRUCTOR
// ============================================================

CodeEditor::CodeEditor(
    QWidget* parent
)
    : QPlainTextEdit(parent),
      lineNumberArea(
          new LineNumberArea(this)
      ),
      currentExecutionLine(-1),
      lineNumbersVisible(true),
      currentLineHighlightEnabled(true),
      gutterHovered(false),
      gutterBackgroundColor("#0B0F14"),
      gutterHoverBackgroundColor("#161B22"),
      gutterTextColor("#6E7681"),
      currentLineColor("#182331"),
      breakpointColor("#F85149"),
      executionColor("#58A6FF")
{
    // Never wrap assembly source.
    setLineWrapMode(
        QPlainTextEdit::NoWrap
    );


    // Explicit horizontal + vertical scrolling.
    setHorizontalScrollBarPolicy(
        Qt::ScrollBarAsNeeded
    );


    setVerticalScrollBarPolicy(
        Qt::ScrollBarAsNeeded
    );


    setEditorFontSize(
        11
    );


    connect(
        this,
        &CodeEditor::blockCountChanged,
        this,
        &CodeEditor::updateLineNumberAreaWidth
    );


    connect(
        this,
        &CodeEditor::updateRequest,
        this,
        &CodeEditor::updateLineNumberArea
    );


    connect(
        this,
        &CodeEditor::cursorPositionChanged,
        this,
        &CodeEditor::highlightCurrentLine
    );


    updateLineNumberAreaWidth(
        0
    );


    highlightCurrentLine();
}


// ============================================================
// GUTTER WIDTH
// ============================================================

int CodeEditor::lineNumberAreaWidth() const
{
    if (!lineNumbersVisible) {
        return 0;
    }


    int digits =
        1;


    int maximum =
        qMax(
            1,
            blockCount()
        );


    while (maximum >= 10) {
        maximum /= 10;

        digits++;
    }


    // Wider than before so the interactive breakpoint area is
    // visually obvious.
    return
        36 +
        fontMetrics()
            .horizontalAdvance(
                QLatin1Char('9')
            )
        * digits;
}


// ============================================================
// GUTTER HOVER
// ============================================================

void CodeEditor::setGutterHovered(
    bool hovered
)
{
    gutterHovered =
        hovered;


    lineNumberArea->update();
}


// ============================================================
// UPDATE GUTTER SIZE
// ============================================================

void CodeEditor::updateLineNumberAreaWidth(
    int
)
{
    int width =
        lineNumberAreaWidth();


    setViewportMargins(
        width,
        0,
        0,
        0
    );


    lineNumberArea->setVisible(
        lineNumbersVisible
    );
}


// ============================================================
// CURRENT EDITOR LINE
// ============================================================

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection>
        selections;


    if (
        currentLineHighlightEnabled &&
        !isReadOnly()
    ) {
        QTextEdit::ExtraSelection
            selection;


        selection.format.setBackground(
            currentLineColor
        );


        selection.format.setProperty(
            QTextFormat::FullWidthSelection,
            true
        );


        selection.cursor =
            textCursor();


        selection.cursor.clearSelection();


        selections.append(
            selection
        );
    }


    setExtraSelections(
        selections
    );
}


// ============================================================
// GUTTER UPDATE
// ============================================================

void CodeEditor::updateLineNumberArea(
    const QRect& rect,
    int dy
)
{
    if (dy != 0) {
        lineNumberArea->scroll(
            0,
            dy
        );
    }
    else {
        lineNumberArea->update(
            0,
            rect.y(),
            lineNumberArea->width(),
            rect.height()
        );
    }


    if (
        rect.contains(
            viewport()->rect()
        )
    ) {
        updateLineNumberAreaWidth(
            0
        );
    }
}


// ============================================================
// RESIZE
// ============================================================

void CodeEditor::resizeEvent(
    QResizeEvent* event
)
{
    QPlainTextEdit::resizeEvent(
        event
    );


    QRect content =
        contentsRect();


    lineNumberArea->setGeometry(
        QRect(
            content.left(),
            content.top(),
            lineNumberAreaWidth(),
            content.height()
        )
    );
}


// ============================================================
// LINE AT Y
// ============================================================

int CodeEditor::lineAtY(
    int y
) const
{
    QTextBlock block =
        firstVisibleBlock();


    int blockNumber =
        block.blockNumber();


    int top =
        qRound(
            blockBoundingGeometry(
                block
            )
            .translated(
                contentOffset()
            )
            .top()
        );


    while (
        block.isValid()
    ) {
        int height =
            qRound(
                blockBoundingRect(
                    block
                )
                .height()
            );


        int bottom =
            top + height;


        if (
            y >= top &&
            y < bottom
        ) {
            return
                blockNumber + 1;
        }


        block =
            block.next();


        blockNumber++;


        top =
            bottom;
    }


    return -1;
}


// ============================================================
// GUTTER CLICK
// ============================================================

void CodeEditor::gutterMousePress(
    QMouseEvent* event
)
{
    if (
        event->button()
        != Qt::LeftButton
    ) {
        return;
    }


    int line =
        lineAtY(
            event->position().y()
        );


    if (line < 1) {
        return;
    }


    bool enabled =
        false;


    if (
        breakpoints.contains(
            line
        )
    ) {
        breakpoints.remove(
            line
        );


        enabled =
            false;
    }
    else {
        breakpoints.insert(
            line
        );


        enabled =
            true;
    }


    lineNumberArea->update();


    emit breakpointToggled(
        line,
        enabled
    );
}


// ============================================================
// PAINT GUTTER
// ============================================================

void CodeEditor::lineNumberAreaPaintEvent(
    QPaintEvent* event
)
{
    QPainter painter(
        lineNumberArea
    );


    painter.fillRect(
        event->rect(),
        gutterHovered
            ? gutterHoverBackgroundColor
            : gutterBackgroundColor
    );


    // A small separator makes it clearer that the gutter is
    // an independent clickable IDE control.
    painter.setPen(
        gutterHovered
            ? executionColor
            : gutterTextColor
    );


    painter.drawLine(
        lineNumberArea->width() - 1,
        event->rect().top(),
        lineNumberArea->width() - 1,
        event->rect().bottom()
    );


    QTextBlock block =
        firstVisibleBlock();


    int blockNumber =
        block.blockNumber();


    int top =
        qRound(
            blockBoundingGeometry(
                block
            )
            .translated(
                contentOffset()
            )
            .top()
        );


    int bottom =
        top +
        qRound(
            blockBoundingRect(
                block
            )
            .height()
        );


    while (
        block.isValid() &&
        top <= event->rect().bottom()
    ) {
        if (
            block.isVisible() &&
            bottom >= event->rect().top()
        ) {
            int line =
                blockNumber + 1;


            // ------------------------------------------------
            // BREAKPOINT
            // ------------------------------------------------

            if (
                breakpoints.contains(
                    line
                )
            ) {
                painter.setPen(
                    Qt::NoPen
                );


                painter.setBrush(
                    breakpointColor
                );


                painter.drawEllipse(
                    QPoint(
                        12,
                        top +
                        fontMetrics().height() / 2
                    ),
                    5,
                    5
                );
            }
            else if (gutterHovered) {
                // Subtle empty breakpoint target.
                painter.setPen(
                    gutterTextColor
                );


                painter.setBrush(
                    Qt::NoBrush
                );


                painter.drawEllipse(
                    QPoint(
                        12,
                        top +
                        fontMetrics().height() / 2
                    ),
                    4,
                    4
                );
            }


            // ------------------------------------------------
            // CURRENT EXECUTION
            // ------------------------------------------------

            if (
                line ==
                currentExecutionLine
            ) {
                painter.setPen(
                    executionColor
                );


                QFont arrowFont =
                    painter.font();


                arrowFont.setBold(
                    true
                );


                painter.setFont(
                    arrowFont
                );


                painter.drawText(
                    20,
                    top,
                    12,
                    fontMetrics().height(),
                    Qt::AlignCenter,
                    "▶"
                );
            }


            // ------------------------------------------------
            // LINE NUMBER
            // ------------------------------------------------

            painter.setPen(
                line ==
                currentExecutionLine
                    ? executionColor
                    : gutterTextColor
            );


            painter.drawText(
                33,
                top,
                lineNumberArea->width() - 39,
                fontMetrics().height(),
                Qt::AlignRight,
                QString::number(
                    line
                )
            );
        }


        block =
            block.next();


        top =
            bottom;


        bottom =
            top +
            qRound(
                blockBoundingRect(
                    block
                )
                .height()
            );


        blockNumber++;
    }
}


// ============================================================
// BREAKPOINT API
// ============================================================

const QSet<int>&
CodeEditor::breakpointLines() const
{
    return breakpoints;
}


void CodeEditor::clearBreakpoints()
{
    breakpoints.clear();


    lineNumberArea->update();
}


// ============================================================
// EXECUTION LINE
// ============================================================

void CodeEditor::setExecutionLine(
    int line
)
{
    currentExecutionLine =
        line;


    lineNumberArea->update();
}


int CodeEditor::executionLine() const
{
    return currentExecutionLine;
}


// ============================================================
// EDITOR SETTINGS
// ============================================================

void CodeEditor::setLineNumbersVisible(
    bool visible
)
{
    lineNumbersVisible =
        visible;


    updateLineNumberAreaWidth(
        0
    );
}


void CodeEditor::setCurrentLineHighlightEnabled(
    bool enabled
)
{
    currentLineHighlightEnabled =
        enabled;


    highlightCurrentLine();
}


void CodeEditor::setEditorFontSize(
    int pointSize
)
{
    QFont editorFont =
        font();


    editorFont.setFamily(
        "Consolas"
    );


    editorFont.setPointSize(
        pointSize
    );


    setFont(
        editorFont
    );


    // FontMetrics changed, therefore both gutter width and tab
    // calculations may depend on it.
    updateLineNumberAreaWidth(
        0
    );


    viewport()->update();
}


void CodeEditor::setEditorTabWidth(
    int spaces
)
{
    if (spaces < 1) {
        spaces = 1;
    }


    QFontMetrics metrics(
        font()
    );


    qreal singleSpace =
        metrics.horizontalAdvance(
            QLatin1Char(' ')
        );


    setTabStopDistance(
        singleSpace *
        static_cast<qreal>(
            spaces
        )
    );
}


// ============================================================
// COLORS
// ============================================================

void CodeEditor::setVisualColors(
    const QColor& gutterBackground,
    const QColor& gutterHoverBackground,
    const QColor& gutterText,
    const QColor& currentLine,
    const QColor& breakpoint,
    const QColor& execution
)
{
    gutterBackgroundColor =
        gutterBackground;


    gutterHoverBackgroundColor =
        gutterHoverBackground;


    gutterTextColor =
        gutterText;


    currentLineColor =
        currentLine;


    breakpointColor =
        breakpoint;


    executionColor =
        execution;


    highlightCurrentLine();


    lineNumberArea->update();
}


// ============================================================
// GO TO LINE
// ============================================================

void CodeEditor::goToLine(
    int line
)
{
    if (line < 1) {
        return;
    }


    QTextBlock block =
        document()
            ->findBlockByNumber(
                line - 1
            );


    if (!block.isValid()) {
        return;
    }


    QTextCursor cursor(
        block
    );


    setTextCursor(
        cursor
    );


    centerCursor();


    setFocus();
}