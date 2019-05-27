#ifndef WIDGETLOCKER_H
#define WIDGETLOCKER_H

#include <QApplication>
#include <QWidget>

class AppCursorLocker
{
public:
    AppCursorLocker(Qt::CursorShape cursor = Qt::BusyCursor)
    {
        QApplication::setOverrideCursor(cursor);
        QCoreApplication::processEvents();
    }

   ~AppCursorLocker()
    {
        QApplication::restoreOverrideCursor();
        QCoreApplication::processEvents();
    }
};

/// Disable widget at ctor, restore state at dtor
class WidgetLocker
{
public:
    WidgetLocker(QWidget* widget) :
        mWidget(widget),
        mEnabledBackup(mWidget->isEnabled())
    {
        mWidget->setEnabled(false);
        QCoreApplication::processEvents();
    }

    ~WidgetLocker()
    {
        mWidget->setEnabled(mEnabledBackup);
        QCoreApplication::processEvents();
    }

private:
    QWidget* mWidget = nullptr;
    bool mEnabledBackup = true;
};

#endif // WIDGETLOCKER_H
