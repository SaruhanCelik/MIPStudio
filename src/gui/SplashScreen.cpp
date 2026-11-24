#include "gui/SplashScreen.h"
#include "gui/BuildInfo.h"
#include "gui/StartupDiagnostics.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QSysInfo>

Q_LOGGING_CATEGORY(mipstudioStartup, "mipstudio.startup", QtWarningMsg)

namespace {
QString platformDescription()
{
#if defined(Q_OS_WIN)
    const QString platform = QStringLiteral("Windows");
#elif defined(Q_OS_MACOS)
    const QString platform = QStringLiteral("macOS");
#elif defined(Q_OS_LINUX)
    const QString platform = QStringLiteral("Linux");
#else
    const QString platform = QSysInfo::productType();
#endif
    return QStringLiteral("%1 %2-bit").arg(platform).arg(QSysInfo::WordSize);
}
}

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent, Qt::SplashScreen | Qt::FramelessWindowHint),
      logo(QStringLiteral(":/branding/mips_studio_logo.png")),
      statusText(tr("Initializing workspace"))
{
    setObjectName(QStringLiteral("MIPStudioSplash"));
    setWindowTitle(QStringLiteral("MIPStudio — Starting"));
    setAccessibleName(tr("MIPStudio startup"));
    setAttribute(Qt::WA_OpaquePaintEvent);

    QSize splashSize(640, 420);
    if (QScreen* primary = QGuiApplication::primaryScreen()) {
        setScreen(primary);
        const QRect available = primary->availableGeometry();
        splashSize.scale(available.size().boundedTo(splashSize), Qt::KeepAspectRatio);
        setFixedSize(splashSize);
        move(available.center() - rect().center());
    } else {
        setFixedSize(splashSize);
    }

    minimumTimer.setSingleShot(true);
    minimumTimer.setTimerType(Qt::PreciseTimer);
    connect(&minimumTimer, &QTimer::timeout, this, &SplashScreen::checkReady);

    animationTimer.setInterval(33);
    connect(&animationTimer, &QTimer::timeout, this, QOverload<>::of(&SplashScreen::update));
}

void SplashScreen::setStatus(const QString& status)
{
    statusText = status;
    setAccessibleDescription(status);
    update();
}

void SplashScreen::completeInitialization()
{
    if (terminal)
        return;
    initialized = true;
    setStatus(tr("Workspace ready"));
    checkReady();
}

void SplashScreen::checkReady()
{
    if (terminal || !visibleTime.isValid())
        return;

    const qint64 remaining = MinimumDisplayMs - visibleTime.elapsed();
    if (remaining > 0) {
        minimumTimer.start(static_cast<int>(remaining));
        return;
    }
    if (initialized) {
        // Set the guard before emitting: readiness and timers may race in the event queue.
        terminal = true;
        animationTimer.stop();
        minimumTimer.stop();
        qCInfo(mipstudioStartup).noquote()
            << QStringLiteral("event=splash-ready visible_ms=%1").arg(visibleTime.elapsed());
        emit readyToShow();
    }
}

void SplashScreen::dismiss()
{
    terminal = true;
    animationTimer.stop();
    minimumTimer.stop();
    hide();
}

void SplashScreen::closeEvent(QCloseEvent* event)
{
    const bool wasStarting = !terminal;
    dismiss();
    event->accept();
    if (wasStarting)
        emit cancelled();
}

void SplashScreen::paintEvent(QPaintEvent*)
{
    const bool firstFrame = !firstFrameScheduled && !terminal;
    firstFrameScheduled = true;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.scale(width() / 640.0, height() / 420.0);

    QLinearGradient background(0, 420, 640, 0);
    background.setColorAt(0, QColor("#111820"));
    background.setColorAt(0.55, QColor("#0C182B"));
    background.setColorAt(1, QColor("#153650"));
    painter.fillRect(QRect(0, 0, 640, 420), background);

    // Original processor-inspired traces, kept behind the readable content.
    painter.setPen(QPen(QColor(69, 151, 230, 28), 1));
    for (int i = 0; i < 9; ++i) {
        const qreal x = 370 + i * 28;
        QPainterPath trace;
        trace.moveTo(x, 0);
        trace.lineTo(x, 82 + i * 12);
        trace.lineTo(x - 36, 118 + i * 12);
        trace.lineTo(x - 36, 270 + i * 10);
        trace.lineTo(x + 16, 322 + i * 10);
        trace.lineTo(640, 322 + i * 10);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(trace);
        painter.setBrush(QColor("#214865"));
        painter.drawEllipse(QPointF(x - 36, 270 + i * 10), 2, 2);
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#29415A"), 1));
    painter.drawRect(QRectF(0.5, 0.5, 639, 419));
    painter.fillRect(QRect(32, 21, 28, 3), QColor("#48ABFF"));

    auto text = [&](const QRectF& bounds, int size, QFont::Weight weight,
                    const QColor& color, const QString& value) {
        QFont font(QStringLiteral("Segoe UI"));
        font.setPixelSize(size);
        font.setWeight(weight);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(bounds, Qt::AlignLeft | Qt::AlignVCenter, value);
    };

    text(QRectF(32, 33, 330, 20), 14, QFont::DemiBold, QColor("#E6F1FC"), QStringLiteral("MIPStudio"));
    text(QRectF(32, 56, 330, 17), 11, QFont::Normal, QColor("#B8CADC"),
         tr("Version %1").arg(QCoreApplication::applicationVersion()));
    text(QRectF(32, 73, 330, 17), 11, QFont::Normal, QColor("#B8CADC"), platformDescription());
    text(QRectF(32, 90, 330, 17), 11, QFont::Normal, QColor("#B8CADC"),
         tr("Release %1").arg(QStringLiteral(MIPSTUDIO_RELEASE_YEAR)));

    // A neutral light inset preserves the supplied logo's black lettering and gold detail.
    const QRectF logoCard(410, 35, 198, 204);
    painter.setPen(QPen(QColor("#6486A4"), 1));
    painter.setBrush(QColor("#FFFFFF"));
    painter.drawRoundedRect(logoCard, 10, 10);
    if (!logo.isNull()) {
        const QSizeF fitted = QSizeF(logo.size()).scaled(QSizeF(184, 184), Qt::KeepAspectRatio);
        const QRectF target(logoCard.center() - QPointF(fitted.width() / 2, fitted.height() / 2), fitted);
        painter.drawPixmap(target, logo, QRectF(logo.rect()));
    } else {
        // Keep the asset reference visible without inventing another brand mark.
        text(logoCard.adjusted(14, 14, -14, -14), 12, QFont::Normal, QColor("#263B50"),
             tr("Logo unavailable"));
    }

    text(QRectF(30, 122, 370, 55), 44, QFont::DemiBold, QColor("#F2F7FD"), QStringLiteral("MIPStudio"));
    text(QRectF(33, 186, 370, 42), 15, QFont::Normal, QColor("#B9D5EE"),
         tr("MIPS Assembly Development &\nSimulation Environment"));

    text(QRectF(33, 249, 360, 18), 11, QFont::Normal, QColor("#9CB3C9"), tr("Designed & Developed by"));
    text(QRectF(32, 270, 380, 27), 21, QFont::DemiBold, QColor("#E6F1FC"), QStringLiteral("Saruhan \u00C7elik"));
    text(QRectF(33, 300, 400, 18), 12, QFont::Normal, QColor("#BECDDC"), tr("Marmara University (affiliation)"));
    text(QRectF(33, 319, 360, 18), 12, QFont::Normal, QColor("#BECDDC"), QStringLiteral("Istanbul, T\u00FCrkiye"));

    const qreal phase = visibleTime.isValid() ? (visibleTime.elapsed() % 1600) / 1600.0 : 0;
    painter.setPen(QPen(QColor("#32516F"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(34, 345, 10, 10));
    painter.setPen(QPen(QColor("#54B8FF"), 2, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(34, 345, 10, 10), -static_cast<int>(phase * 360 * 16), 100 * 16);
    text(QRectF(53, 340, 530, 20), 11, QFont::Normal, QColor("#9DBDD8"), statusText);

    painter.setPen(QColor("#29415A"));
    painter.drawLine(QPointF(32, 370), QPointF(608, 370));
    text(QRectF(33, 377, 570, 16), 11, QFont::Normal, QColor("#B4C5D5"),
         QStringLiteral("\u00A9 %1 Saruhan \u00C7elik").arg(QStringLiteral(MIPSTUDIO_RELEASE_YEAR)));
    text(QRectF(33, 396, 570, 16), 11, QFont::Normal, QColor("#9CB3C9"),
         tr("MIPStudio \u2014 Built for Learning, Designed for Engineering."));

    painter.end();
    if (firstFrame) {
        // Font discovery and first-frame rendering can take time. Start counting
        // after that work has completed and Qt has returned to its event loop.
        QTimer::singleShot(0, this, [this] {
            if (terminal)
                return;
            visibleTime.start();
            minimumTimer.start(MinimumDisplayMs);
            animationTimer.start();
            qCInfo(mipstudioStartup) << "event=splash-painted";
            emit initializationRequested();
        });
    }
}
