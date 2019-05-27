#include "statusmessage.h"

#include <QCoreApplication>
#include <QStatusBar>
#include <QString>

QStatusBar* StatusMessage::mBar = nullptr;

void StatusMessage::show(const QString& text, int timeout)
{
    Q_ASSERT(mBar); if (!mBar) return;
    mBar->showMessage(text, timeout);
    QCoreApplication::processEvents();
}

void StatusMessage::clear()
{
    Q_ASSERT(mBar); if (!mBar) return;
    mBar->clearMessage();
}
