#ifndef ABSTRACTSETTINGS_H
#define ABSTRACTSETTINGS_H

#include <QSettings>
#include <QVariant>

class AbstractSettings
{
public:
    class Tag
    {
    public:
        Tag(const char* key) : mKey(key) {}

        QVariant value() const {
            return QSettings().value(mKey);
        }

        template<typename T>
        T operator ()(const T& defaultValue) const {
            return QSettings().value(mKey, defaultValue).template value<T>();
        }

        void save(const QVariant& value) {
            QSettings().setValue(mKey, value);
        }

        bool exists() const {
            return QSettings().contains(mKey);
        }

    private:
        const QString mKey;
    };
};

#endif // ABSTRACTSETTINGS_H
