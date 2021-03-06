#ifndef STATUSMESSAGE_H
#define STATUSMESSAGE_H

class QStatusBar;
class QString;

/// Shows simple text in the status bar
class StatusMessage
{
public:
    /// Call this function first
    static void setStatusBar(QStatusBar* statusBar) { mBar = statusBar; }

    /// \brief Displays the given text for the specified number of milli-seconds (timeout).
    /// If timeout is 0, the message remains displayed until clear() is called or until show()
    /// is called again to change the message.
    /// WARNING: Please set the status bar first.
    static void show(const QString& text, int timeout = mcShort);

    /// Removes any message being shown.
    /// WARNING: Please set the status bar first.
    static void clear();

    static const int mcShort = 2000;
    static const int mcInfinite = 0;

private:
    static QStatusBar* mBar;
};

#endif // STATUSMESSAGE_H
