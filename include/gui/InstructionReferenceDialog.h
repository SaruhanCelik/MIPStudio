#ifndef MIPSSTUDIO_INSTRUCTIONREFERENCEDIALOG_H
#define MIPSSTUDIO_INSTRUCTIONREFERENCEDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>


class QComboBox;
class QLineEdit;
class QListWidget;
class QTextBrowser;


// ============================================================
// INSTRUCTION REFERENCE
// ------------------------------------------------------------
// Searchable reference containing every real and pseudo-
// instruction accepted by MIPS Studio.
// ============================================================

class InstructionReferenceDialog :
    public QDialog
{
    Q_OBJECT


public:
    explicit InstructionReferenceDialog(
        QWidget* parent = nullptr
    );


private slots:
    void refreshList();

    void showSelectedEntry();


private:
    struct Entry {
        QString name;

        QString syntax;

        QString category;

        QString description;

        QString operation;

        QString example;

        bool pseudo =
            false;
    };


    QLineEdit* searchEdit =
        nullptr;


    QComboBox* kindCombo =
        nullptr;


    QComboBox* categoryCombo =
        nullptr;


    QListWidget* instructionList =
        nullptr;


    QTextBrowser* detailsBrowser =
        nullptr;


    QVector<Entry>
        entries;


    static QVector<Entry>
        buildEntries();


    void buildUI();
};


#endif