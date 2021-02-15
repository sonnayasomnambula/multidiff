#ifndef ABSTRACTSETTINGS_H
#define ABSTRACTSETTINGS_H

#include <QMutex>
#include <QSettings>
#include <QSharedPointer>
#include <QVariant>

class AbstractSettings
{
public:
    class Tag
    {
    public:
        Tag(const char* key) : mKey(key) {}

        QVariant value() const {
            return Storage::instance()->value(mKey);
        }

        template<typename T>
        T operator ()(const T& defaultValue) const {
            return Storage::instance()->value(mKey, defaultValue).template value<T>();
        }

        void save(const QVariant& value) {
            Storage::instance()->setValue(mKey, value);
        }

        bool exists() const {
            return Storage::instance()->contains(mKey);
        }

    private:
        const QString mKey;
    };

private:
    class Storage
    {
        QSharedPointer<QSettings> mLocalPointer;
        // unlike java we cannot access AbstractSettings::this from a nested class
        // (and i don't want to pass it through Tag ctor),
        // so we'll create a static variable in ctor and destroy it in dtor
        static QWeakPointer<QSettings> mCommonRef;
        static QMutex mCommonMutex;
    public:
        Storage();
       ~Storage();
        static QSharedPointer<QSettings> instance();
    } mStorage;
};

#endif // ABSTRACTSETTINGS_H
