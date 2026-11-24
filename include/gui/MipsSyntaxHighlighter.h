#ifndef MIPSSTUDIO_MIPSSYNTAXHIGHLIGHTER_H
#define MIPSSTUDIO_MIPSSYNTAXHIGHLIGHTER_H

#include <QColor>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>


// ============================================================
// SyntaxPalette
// ============================================================

struct SyntaxPalette {
    QColor normalText;
    QColor instruction;
    QColor reg;
    QColor directive;
    QColor label;
    QColor comment;
    QColor string;
    QColor number;
};


// ============================================================
// MipsSyntaxHighlighter
// ============================================================

class MipsSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit MipsSyntaxHighlighter(
        QTextDocument* parent = nullptr
    );

    void setPalette(
        const SyntaxPalette& palette
    );


protected:
    void highlightBlock(
        const QString& text
    ) override;


private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };


    QVector<Rule> rules;

    SyntaxPalette currentPalette;


    void rebuildRules();
};

#endif