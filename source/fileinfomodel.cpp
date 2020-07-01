#include "fileinfomodel.h"

#include <set>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>
#include <QUrl>

#include "statusmessage.h"

void FileInfoModel::add(const QList<FileItem>& items)
{
    beginResetModel(); // TODO: beginInsertRows();
    mData.append(items);
    updatePixmaps();
    endResetModel();
}

void FileInfoModel::remove(std::set<int, std::greater<int>>& rows)
{
    beginResetModel();

    for (int i: rows)
        mData.removeAt(i);

    endResetModel();
}

QVariant FileInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)    return {};
    if (role != Qt::DisplayRole)        return {};

    switch (section)
    {
        case eName:         return tr("Name");
        case eDir:          return tr("Directory");
        case eSize:         return tr("Size");
        case eLastModified: return tr("Last modified");
        case eHash:         return tr("Hash");
        default:            return {};
    }
}

int FileInfoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mData.size();
}

int FileInfoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return ColCount;
}

QVariant FileInfoModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return {};

    if (role == Qt::DisplayRole)
        return displayData(index);

    if (role == Qt::DecorationRole && index.column() == 0)
        return mData[index.row()].pixmap;

    return {};
}

FileItem FileInfoModel::item(int row) const
{
    if (row < mData.size())
        return mData[row];

    return {};
}

QVariant FileInfoModel::displayData(const QModelIndex& index) const
{
    const auto& file = mData[index.row()].fileInfo;
    switch (index.column())
    {
        case eName:         return file.fileName();
        case eDir:          return file.dir().absolutePath();
        case eSize:         return file.size();
        case eLastModified: return file.lastModified();
        case eHash:         return mData[index.row()].hash;
        default:            return {};
    }
}

void FileInfoModel::updatePixmaps()
{
    class ColorGenerator
    {
    public:
        QColor next() {
            if (mColor < Qt::transparent)
                return static_cast<Qt::GlobalColor>(mColor++);

            return { mRand.bounded(255), mRand.bounded(255), mRand.bounded(255) };
        }

    private:
        int mColor = Qt::white;
        QRandomGenerator mRand;
    };

    ColorGenerator uniqueColors;
    QMap<QByteArray, QColor> colors; // each hash have unique color

    for (auto& item: mData)
    {
        // it is already known hash or a brand new one?

        auto icolor = colors.find(item.hash);
        if (icolor == colors.cend())
            icolor = colors.insert(item.hash, uniqueColors.next());

        item.pixmap = coloredSquarePixmap(*icolor);
    }

    StatusMessage::show(QObject::tr("There are %n/%1 unique file(s)", "", colors.size()).arg(mData.size()), StatusMessage::mcInfinite);
}

QPixmap FileInfoModel::coloredSquarePixmap(QColor color, int size)
{
    QPixmap pix(size, size);
    QPainter painer(&pix);
    painer.setBrush(color);
    painer.setPen(Qt::black);
    painer.drawRect(pix.rect().adjusted(0, 0, -1, -1));
    return pix;
}

void FileInfoModel::Collector::collect(const QList<QUrl>& urls)
{
    for (const auto& url: urls)
    {
        const auto path = url.toLocalFile();
        QFileInfo entry(path);

        if (entry.isFile())
            appendFile(path);
        else if (entry.isDir())
            appendDir(path);
        else
            mWarnings.append(QObject::tr("Cannot add '%1'").arg(path));

        if (mLastClickedButton == QMessageBox::Cancel)
            break;
    }
}

void FileInfoModel::Collector::appendFile(const QString& path)
{
    QFile file(path);
    QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Sha1);

    // calculate file sha-1 hash

    if (!file.open(QIODevice::ReadOnly))
    {
        mWarnings.append(QObject::tr("Unable to open '%1'").arg(path));
        return;
    }

    if (!hashCalculator.addData(&file))
    {
        mWarnings.append(QObject::tr("Cannot read '%1'").arg(path));
        return;
    }

    StatusMessage::show(QObject::tr("Calculate hash for '%1'...").arg(path), StatusMessage::mcInfinite);
    const auto hash = hashCalculator.result();

    mItems.append({path, hash, {}});
}

void FileInfoModel::Collector::appendDir(const QString& path)
{
    if (mLastClickedButton != QMessageBox::YesToAll)
    {
        mLastClickedButton = QMessageBox::question(mParent, "",
            QObject::tr("'%1' is a directory.\nDo you want to add all files from this directory?").arg(path),
            QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (mLastClickedButton == QMessageBox::No || mLastClickedButton == QMessageBox::Cancel)
            return;
    }

    QDirIterator files(path, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (files.hasNext())
    {
        appendFile(files.next());
    }
}
