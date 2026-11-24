#ifndef MIPSTUDIO_STARTUP_DIAGNOSTICS_H
#define MIPSTUDIO_STARTUP_DIAGNOSTICS_H

#include <QLoggingCategory>

// Opt in with QT_LOGGING_RULES=mipstudio.startup.info=true. Normal launches are quiet.
Q_DECLARE_LOGGING_CATEGORY(mipstudioStartup)

#endif
