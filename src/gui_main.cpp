#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>

#include <cstdlib>
#include <exception>
#include <memory>

#include "gui/BuildInfo.h"
#include "gui/MainWindow.h"
#include "gui/SplashScreen.h"
#include "gui/StartupDiagnostics.h"

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    // Preserve the existing settings identity. Display branding is independent.
    QCoreApplication::setOrganizationName(QStringLiteral("Saruhan Celik"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("mipsstudio"));
    QCoreApplication::setApplicationName(QStringLiteral("MIPS Studio"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("MIPStudio"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MIPSTUDIO_VERSION));
    application.setWindowIcon(QIcon(QStringLiteral(":/branding/mips_studio.ico")));

    // Ignore a second launch during startup, including repeated Explorer clicks.
    // Release the lock at the transition so existing multi-window use is preserved.
    const QString lockDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (lockDirectory.isEmpty() || !QDir().mkpath(lockDirectory)) {
        QMessageBox::critical(nullptr, QStringLiteral("MIPStudio startup"),
                              QObject::tr("Could not create the startup lock directory."));
        return EXIT_FAILURE;
    }
    QLockFile startupLock(QDir(lockDirectory).filePath(QStringLiteral("startup.lock")));
    startupLock.setStaleLockTime(0); // A live initialization may legitimately take longer than 30 seconds.
    if (!startupLock.tryLock(0)) {
        if (startupLock.error() == QLockFile::LockFailedError)
            return EXIT_SUCCESS;
        QMessageBox::critical(nullptr, QStringLiteral("MIPStudio startup"),
                              QObject::tr("Could not acquire the startup lock."));
        return EXIT_FAILURE;
    }

    // The splash may close before a main window exists (cancel/error/transition).
    application.setQuitOnLastWindowClosed(false);
    SplashScreen splash;
    std::unique_ptr<MainWindow> window;
    bool startupEnded = false;

    auto failStartup = [&](const QString& message) {
        if (startupEnded)
            return;
        startupEnded = true;
        splash.dismiss();
        startupLock.unlock();
        auto* error = new QMessageBox(QMessageBox::Critical, QStringLiteral("MIPStudio startup"),
                                     message, QMessageBox::Close);
        error->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(error, &QMessageBox::finished, &application, [&](int) {
            application.exit(EXIT_FAILURE);
        });
        error->open();
    };

    QObject::connect(&splash, &SplashScreen::cancelled, &application, [&] {
        startupEnded = true;
        application.quit();
    });
    QObject::connect(&splash, &SplashScreen::initializationRequested, &application, [&] {
        if (startupEnded || window)
            return;
        try {
            // QApplication has entered its event loop and painted the splash.
            const QString initialProgram = application.arguments().value(1);
            window = std::make_unique<MainWindow>(initialProgram, nullptr,
                                                 MainWindow::InitializationMode::Deferred);
            QObject::connect(window.get(), &MainWindow::initializationStatus,
                             &splash, &SplashScreen::setStatus);
            QObject::connect(window.get(), &MainWindow::initialized,
                             &splash, &SplashScreen::completeInitialization);
            QObject::connect(window.get(), &MainWindow::initializationFailed,
                             &application, failStartup);
            window->initializeAsync();
        } catch (const std::exception& error) {
            failStartup(QObject::tr("Could not start MIPStudio: %1")
                            .arg(QString::fromLocal8Bit(error.what())));
        } catch (...) {
            failStartup(QObject::tr("Could not start MIPStudio."));
        }
    });
    QObject::connect(&splash, &SplashScreen::readyToShow, &application, [&] {
        if (startupEnded || !window)
            return;
        startupEnded = true;
        window->showAfterInitialization();
        qCInfo(mipstudioStartup) << "event=main-window-shown";
        splash.dismiss();
        startupLock.unlock();
        application.setQuitOnLastWindowClosed(true);
        window->raise();
        window->activateWindow();
    });

    splash.show();
    return application.exec();
}
