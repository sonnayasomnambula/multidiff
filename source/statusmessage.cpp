#include "statusmessage.h"

#include <QCoreApplication>
#include <QStatusBar>
#include <QString>

QStatusBar* StatusMessage::mBar = nullptr;

void StatusMessage::show(const QString& text, int timeout)
{
    mBar->showMessage(text, timeout);
    QCoreApplication::processEvents();
}

void StatusMessage::clear()
{
    mBar->clearMessage();
}
