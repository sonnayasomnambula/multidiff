#ifndef ABSTRACTSETTINGS_H
#define ABSTRACTSETTINGS_H

#include <QSettings>
#include <QVariant>

class AbstractSettings
{
public:
    template <typename T>
    class Tag
    {
        const QString mKey;

    public:
        Tag(const char* key) : mKey(key) {}

        T operator ()(const T& defaultValue = T()) const {
            return QSettings().value(mKey, defaultValue).template value<T>();
        }

        void save(const QVariant& value) {
            QSettings().setValue(mKey, value);
        }

        bool exists() const {
            return QSettings().contains(mKey);
        }
    };

    class State
    {
        Tag<QByteArray> mState;

    public:
        State(const char* key) : mState(key) {}
        template <class Widget> void save(Widget* w) { mState.save(w->saveState()); }
        template <class Widget> void restore(Widget* w) { w->restoreState(mState(w->saveState())); }
    };

    class Geometry
    {
        Tag<QByteArray> mGeo;

    public:
        Geometry(const char* key) : mGeo(key) {}
        template <class Widget> void save(Widget* w) { mGeo.save(w->saveGeometry()); }
        template <class Widget> void restore(Widget* w) { w->restoreGeometry(mGeo(w->saveGeometry())); }
    };
};

#endif // ABSTRACTSETTINGS_H
