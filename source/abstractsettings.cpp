#include "abstractsettings.h"

QMutex AbstractSettings::Storage::mCommonMutex;
QWeakPointer<QSettings> AbstractSettings::Storage::mCommonRef;

AbstractSettings::Storage::Storage()
{
    QMutexLocker lock(&mCommonMutex);
    mLocalPointer = mCommonRef ? mCommonRef.toStrongRef() : QSharedPointer<QSettings>::create();
    mCommonRef = mLocalPointer.toWeakRef();
}

AbstractSettings::Storage::~Storage()
{
    QMutexLocker lock(&mCommonMutex);
    mLocalPointer.clear();
}

QSharedPointer<QSettings> AbstractSettings::Storage::instance()
{
    QMutexLocker lock(&mCommonMutex);

    if (!mCommonRef)
    {
        Q_ASSERT_X(false, Q_FUNC_INFO, "You must create an AbstractSettings object first");
        return QSharedPointer<QSettings>::create();
    }

    return mCommonRef.toStrongRef();
}
