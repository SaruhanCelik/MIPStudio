#ifndef MIPSTUDIO_SPLASH_SCREEN_H
#define MIPSTUDIO_SPLASH_SCREEN_H

#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

// All geometry is in device-independent pixels; Qt paints at the screen's DPR.
class SplashScreen : public QWidget {
    Q_OBJECT

public:
    static constexpr int MinimumDisplayMs = 4500;

    explicit SplashScreen(QWidget* parent = nullptr);
    void setStatus(const QString& status);
    void completeInitialization();
    void dismiss();

signals:
    void initializationRequested();
    void readyToShow();
    void cancelled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void checkReady();

    QPixmap logo;
    QElapsedTimer visibleTime;
    QTimer minimumTimer;
    QTimer animationTimer;
    QString statusText;
    bool initialized = false;
    bool terminal = false;
    bool firstFrameScheduled = false;
};

#endif
