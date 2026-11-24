#include "gui/BuildInfo.h"
#include "gui/CodeEditor.h"
#include "gui/InstructionReferenceDialog.h"
#include "gui/MainWindow.h"
#include "gui/SplashScreen.h"

#include <QApplication>
#include <QAction>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QLockFile>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QTextBrowser>
#include <QtTest>
#include <cmath>

class StartupTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(settingsDirectory.isValid());
        // Never read or change the developer's actual preferences/layout.
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsDirectory.path());
        QSettings isolated(QSettings::defaultFormat(), QSettings::UserScope, "MIPSStudio", "MIPSStudio");
        QVERIFY(isolated.fileName().startsWith(settingsDirectory.path()));
        qApp->setQuitOnLastWindowClosed(false);
        QCoreApplication::setApplicationVersion(QStringLiteral(MIPSTUDIO_VERSION));
    }

    void init()
    {
        QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "MIPSStudio", "MIPSStudio");
        settings.clear();
    }

    void earlyReadinessWaitsForMinimumAndAnimates()
    {
        SplashScreen splash;
        QSignalSpy requested(&splash, &SplashScreen::initializationRequested);
        QSignalSpy ready(&splash, &SplashScreen::readyToShow);
        QSignalSpy cancelled(&splash, &SplashScreen::cancelled);
        QVERIFY(!QPixmap(":/branding/mips_studio_logo.png").isNull());
        QVERIFY(splash.windowFlags().testFlag(Qt::FramelessWindowHint));
        QElapsedTimer elapsed;
        connect(&splash, &SplashScreen::initializationRequested, this, [&] { elapsed.start(); });
        splash.show();
        QTRY_COMPARE(requested.count(), 1);
        QCOMPARE(splash.screen(), QGuiApplication::primaryScreen());
        QVERIFY((splash.geometry().center() - splash.screen()->availableGeometry().center()).manhattanLength() <= 2);

        splash.completeInitialization();
        splash.completeInitialization();
        const QImage firstFrame = splash.grab().toImage();
        const QString preview = qEnvironmentVariable("MIPSTUDIO_SPLASH_PREVIEW");
        if (!preview.isEmpty())
            QVERIFY(firstFrame.save(preview));

        // Event-loop heartbeats and changing pixels prove the wait is responsive.
        int heartbeats = 0;
        QTimer heartbeat;
        connect(&heartbeat, &QTimer::timeout, this, [&] { ++heartbeats; });
        heartbeat.start(20);
        QTest::qWait(180);
        QVERIFY(firstFrame != splash.grab().toImage());
        QVERIFY(heartbeats >= 3);
        QTest::mouseClick(&splash, Qt::LeftButton);
        QVERIFY(splash.isVisible());
        QCOMPARE(cancelled.count(), 0);
        QCOMPARE(ready.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 6000);
        // Both clocks start in the same signal delivery; allow only millisecond rounding.
        QVERIFY(elapsed.elapsed() >= SplashScreen::MinimumDisplayMs - 1);
        splash.completeInitialization();
        QTest::qWait(60);
        QCOMPARE(ready.count(), 1);
        QCOMPARE(requested.count(), 1);
        splash.dismiss();
        QVERIFY(!splash.isVisible());
    }

    void lateReadinessKeepsSplashVisible()
    {
        SplashScreen splash;
        QSignalSpy requested(&splash, &SplashScreen::initializationRequested);
        QSignalSpy ready(&splash, &SplashScreen::readyToShow);
        splash.show();
        QTRY_COMPARE(requested.count(), 1);
        QTest::qWait(SplashScreen::MinimumDisplayMs + 150);
        QCOMPARE(ready.count(), 0);
        QVERIFY(splash.isVisible());
        splash.completeInitialization();
        QCOMPARE(ready.count(), 1);
        splash.dismiss();
    }

    void cancellationStopsTransition()
    {
        SplashScreen splash;
        QSignalSpy requested(&splash, &SplashScreen::initializationRequested);
        QSignalSpy ready(&splash, &SplashScreen::readyToShow);
        QSignalSpy cancelled(&splash, &SplashScreen::cancelled);
        splash.show();
        QTRY_COMPARE(requested.count(), 1);
        QVERIFY(splash.close());
        QCOMPARE(cancelled.count(), 1);
        splash.completeInitialization();
        QTest::qWait(60);
        QCOMPARE(ready.count(), 0);
    }

    void deferredWindowLoadsSourceExactlyOnce()
    {
        QTemporaryDir sourceDirectory;
        QVERIFY(sourceDirectory.isValid());
        const QString filename = sourceDirectory.filePath(QStringLiteral("program-\u00E7.asm"));
        const QByteArray source(".text\nmain:\n    li $v0, 10\n    syscall\n");
        QFile file(filename);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(source), source.size());
        file.close();

        MainWindow window(filename, nullptr, MainWindow::InitializationMode::Deferred);
        QSignalSpy ready(&window, &MainWindow::initialized);
        QSignalSpy failed(&window, &MainWindow::initializationFailed);
        QVERIFY(!window.findChild<CodeEditor*>());
        window.showAfterInitialization();
        QVERIFY(!window.isVisible());
        window.initializeAsync();
        window.initializeAsync();
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
        QCOMPARE(failed.count(), 0);
        QVERIFY(!window.isVisible());
        auto* editor = window.findChild<CodeEditor*>();
        QVERIFY(editor);
        QCOMPARE(editor->toPlainText(), QString::fromUtf8(source));
        QVERIFY(!editor->document()->isModified());
        QCOMPARE(window.findChildren<CodeEditor*>().size(), 1);
        window.initializeAsync();
        QTest::qWait(60);
        QCOMPARE(ready.count(), 1);
        window.showAfterInitialization();
        window.showAfterInitialization();
        QVERIFY(window.isVisible());
        window.hide();
    }

    void unreadableSourceFailsWithoutPresentingWindow()
    {
        MainWindow window(settingsDirectory.filePath("missing.asm"), nullptr,
                          MainWindow::InitializationMode::Deferred);
        QSignalSpy ready(&window, &MainWindow::initialized);
        QSignalSpy failed(&window, &MainWindow::initializationFailed);
        window.initializeAsync();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QCOMPARE(ready.count(), 0);
        QVERIFY(failed.at(0).at(0).toString().contains("missing.asm"));
        window.showAfterInitialization();
        QVERIFY(!window.isVisible());
    }

    void restoredFloatingDocksStaySuppressedUntilTransition()
    {
        {
            MainWindow previous;
            previous.setAttribute(Qt::WA_DontShowOnScreen);
            previous.show();
            QTest::qWait(30); // Let QMainWindow establish the dock layout before saving it.
            auto* dock = previous.findChild<QDockWidget*>("RegistersDock");
            QVERIFY(dock);
            dock->setAttribute(Qt::WA_DontShowOnScreen);
            dock->setFloating(true);
            dock->show();
            QVERIFY(dock->isFloating());
            QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "MIPSStudio", "MIPSStudio");
            settings.setValue("windowState", previous.saveState());
        }
        MainWindow window({}, nullptr, MainWindow::InitializationMode::Deferred);
        QSignalSpy ready(&window, &MainWindow::initialized);
        window.initializeAsync();
        QTRY_COMPARE(ready.count(), 1);
        QVERIFY(!window.isVisible());
        auto* dock = window.findChild<QDockWidget*>("RegistersDock");
        QVERIFY(dock);
        QVERIFY(dock->isFloating());
        QVERIFY(dock->testAttribute(Qt::WA_DontShowOnScreen));
        window.showAfterInitialization();
        QVERIFY(window.isVisible());
        QVERIFY(dock->isVisible());
        QVERIFY(!dock->testAttribute(Qt::WA_DontShowOnScreen));
        dock->hide();
        window.hide();
    }

    void simultaneousStartupLockIsExclusive()
    {
        const QString filename = settingsDirectory.filePath("startup.lock");
        QLockFile first(filename);
        QLockFile second(filename);
        first.setStaleLockTime(0);
        second.setStaleLockTime(0);
        QVERIFY(first.tryLock(0));
        QVERIFY(!second.tryLock(0));
        first.unlock();
        QVERIFY(second.tryLock(0));
    }

    void instructionReferenceIsReadable_data()
    {
        QTest::addColumn<QString>("theme");
        QTest::newRow("dark") << QString("dark");
        QTest::newRow("light") << QString("light");
    }

    void instructionReferenceIsReadable()
    {
        QFETCH(QString, theme);
        AppSettings().setTheme(theme);
        MainWindow window;
        InstructionReferenceDialog dialog(&window);
        dialog.show();
        QTest::qWait(50);
        auto* list = dialog.findChild<QListWidget*>("InstructionList");
        auto* details = dialog.findChild<QTextBrowser*>("InstructionDetails");
        QVERIFY(list);
        QVERIFY(details);
        QVERIFY(list->count() > 50);
        QVERIFY(details->toPlainText().contains("Signed 32-bit addition"));
        for (QWidget* view : {static_cast<QWidget*>(list), static_cast<QWidget*>(details)}) {
            const QPalette palette = view->palette();
            QVERIFY(contrast(palette.color(QPalette::Text), palette.color(QPalette::Base)) >= 4.5);
            QVERIFY(contrast(palette.color(QPalette::HighlightedText), palette.color(QPalette::Highlight)) >= 4.5);
            QCOMPARE(palette.color(QPalette::Base), QColor(theme == "dark" ? "#0D1117" : "#F0ECE4"));
        }
        saveHelpPreview(dialog, "instructions-" + theme);

        // Searching and the empty-results page must remain functional after styling.
        auto* search = dialog.findChild<QLineEdit*>();
        QVERIFY(search);
        search->setText("no-such-instruction");
        QCOMPARE(list->count(), 0);
        QVERIFY(details->toPlainText().contains("No matching instructions"));
        search->clear();
        QVERIFY(list->count() > 50);
    }

    void keyboardShortcutsMenuOpensReference_data()
    {
        instructionReferenceIsReadable_data();
    }

    void keyboardShortcutsMenuOpensReference()
    {
        QFETCH(QString, theme);
        AppSettings().setTheme(theme);
        MainWindow window;
        QAction* openShortcuts = nullptr;
        QList<QStringList> expected;
        for (QAction* menuAction : window.menuBar()->actions()) {
            QMenu* menu = menuAction->menu();
            if (!menu)
                continue;
            for (QAction* action : menu->actions()) {
                if (action->text() == "Keyboard Shortcuts")
                    openShortcuts = action;
                // Prove the table uses live actions rather than a hardcoded list.
                if (action->text() == "&Run / Continue")
                    action->setShortcut(QKeySequence("Ctrl+Alt+F6"));
                QStringList bindings;
                for (const QKeySequence& shortcut : action->shortcuts()) {
                    if (!shortcut.isEmpty())
                        bindings << shortcut.toString(QKeySequence::NativeText);
                }
                if (!bindings.isEmpty())
                    expected.append({QString(menu->title()).remove('&'),
                                     QString(action->text()).remove('&'), bindings.join(", ")});
            }
        }
        QVERIFY(openShortcuts);
        QVERIFY(expected.size() >= 12);

        bool inspected = false;
        bool readable = false;
        QList<QStringList> actual;
        QTimer::singleShot(0, &window, [&] {
            auto* dialog = window.findChild<QDialog*>("KeyboardShortcutsDialog");
            if (!dialog)
                return;
            auto* table = dialog->findChild<QTableWidget*>("ShortcutsTable");
            if (table) {
                QTest::qWait(30);
                inspected = dialog->isVisible();
                readable = contrast(table->palette().color(QPalette::Text),
                                    table->palette().color(QPalette::Base)) >= 4.5;
                for (int row = 0; row < table->rowCount(); ++row) {
                    QStringList cells;
                    for (int column = 0; column < table->columnCount(); ++column)
                        cells << table->item(row, column)->text();
                    actual.append(cells);
                }
                saveHelpPreview(*dialog, "shortcuts-" + theme);
            }
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            if (buttons)
                buttons->button(QDialogButtonBox::Close)->click();
            else
                dialog->reject();
        });
        openShortcuts->trigger();
        QVERIFY(inspected);
        QVERIFY(readable);
        QCOMPARE(actual, expected);
    }

private:
    static double contrast(const QColor& foreground, const QColor& background)
    {
        auto luminance = [](const QColor& color) {
            auto linear = [](double value) {
                return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
            };
            return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
        };
        const double first = luminance(foreground);
        const double second = luminance(background);
        return (qMax(first, second) + 0.05) / (qMin(first, second) + 0.05);
    }

    static void saveHelpPreview(QWidget& dialog, const QString& name)
    {
        const QString directory = qEnvironmentVariable("MIPSTUDIO_HELP_PREVIEW_DIR");
        if (!directory.isEmpty()) {
            QDir().mkpath(directory);
            dialog.grab().save(QDir(directory).filePath(name + ".png"));
        }
    }

    QTemporaryDir settingsDirectory;
};

QTEST_MAIN(StartupTest)
#include "StartupTest.moc"
