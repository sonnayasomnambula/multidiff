#include "filelist.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QDropEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>

#include <algo.h>
#include <eventwatcher.h>

#include "statusmessage.h"
#include "widgetlocker.h"

FileList::FileList(QWidget* parent) :
    QTreeWidget(parent)
{
    // Disable the sort after 3rd click on the same column header
    connect(header(), &QHeaderView::sortIndicatorChanged, [this, mClicked = std::deque<int>{ -1, -1, -1 }](int column) mutable {
        mClicked.push_front(column);
        mClicked.pop_back();
        Q_ASSERT(mClicked.size() == 3);

        if (column >= 0 && mClicked[0] == mClicked[1] && mClicked[1] == mClicked[2])
            sortByColumn(-1);
    });
}

void FileList::append(const QString& path)
{
    const auto file = QFileInfo(path);

    if (file.isFile())
    {
        insertTopLevelItem(topLevelItemCount(), new Item(path));
        return;
    }

    if (file.isDir())
    {
        const auto response = QMessageBox::question(this, "",
                                                    tr("'%1' is a directory.\nDo you want to add all files from this directory?").arg(path));
        if (response != QMessageBox::Yes)
            return;

        QDirIterator filesInDirectory(path, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while (filesInDirectory.hasNext())
            insertTopLevelItem(topLevelItemCount(), new Item(filesInDirectory.next()));
    }
}

QString FileList::path(int row) const
{
    return static_cast<Item*>(topLevelItem(row))->absoluteFilePath();
}

void FileList::dragEnterEvent(QDragEnterEvent* e)
{
    if (!acceptable(e->mimeData()))
        return;

    highlight();
    e->acceptProposedAction();
}

void FileList::dragMoveEvent(QDragMoveEvent* e)
{
    e->acceptProposedAction();
}

void FileList::dragLeaveEvent(QDragLeaveEvent* e)
{
    highlight(false);
    e->accept();
}

void FileList::dropEvent(QDropEvent* e)
{
    highlight(false);

    if (!acceptable(e->mimeData()))
        return;

    e->acceptProposedAction();
    for (const auto& url: e->mimeData()->urls())
        append(url.toLocalFile());

    calculateHashes();
}

void FileList::highlight(bool on)
{
    static const auto backup = palette();
    static const auto highlighted = [this]() {
        QPalette p = palette();
        p.setColor(QPalette::Window, p.color(QPalette::Highlight));
        p.setColor(QPalette::Base, p.color(QPalette::Highlight).lighter(220));
        return p;
    }();

    setPalette(on ? highlighted : backup);
}

bool FileList::acceptable(const QMimeData* mime)
{
    return mime->hasUrls() && !mime->urls().isEmpty() &&
            Algo::all_of(mime->urls(), [](const QUrl& url){ return url.isLocalFile(); });
}

QIcon FileList::square(QColor color, int size)
{
    QPixmap pix(size, size);
    QPainter painer(&pix);
    painer.setBrush(color);
    painer.setPen(Qt::black);
    painer.drawRect(pix.rect().adjusted(1, 1, -2, -2));
    return pix;
}
class ColorGenerator
{
public:
    QColor next() {
        return color(mColor++);
    }

private:
    QColor color(int id) {
        if (id < Qt::transparent)
            return static_cast<Qt::GlobalColor>(id);

        const int r = qrand() % 255;
        const int g = qrand() % 255;
        const int b = qrand() % 255;
        return { r, g, b };
    }

    int mColor = Qt::white;
};

void FileList::calculateHashes()
{
    WidgetLocker wl(this);
    AppCursorLocker acl;

    QMap<QByteArray, QColor> colors; // hash --> color
    ColorGenerator colorGenerator;

    QStringList warnings;

    for (int row=0; row<topLevelItemCount(); ++row)
    {
        try
        {
            auto item = static_cast<Item*>(topLevelItem(row));

            StatusMessage::show(tr("Calculate hash for '%1'...").arg(path(row)), StatusMessage::mcInfinite);
            const auto hash = item->calculateHash();

            // it is already known hash or a brand new one?

            auto icolor = colors.find(hash);
            if (icolor == colors.cend())
                icolor = colors.insert(hash, colorGenerator.next());
            item->setIcon(0, square(icolor.value()));
        }
        catch (const QString& warningMessage)
        {
            warnings.append(warningMessage);
        }
    }

    if (!warnings.isEmpty())
        emit warn(warnings.join("\n"));

    StatusMessage::show(tr("Done!"));
}

void FileList::removeSelectedItems()
{
    for (auto item: selectedItems())
        delete takeTopLevelItem(indexOfTopLevelItem(item));
    StatusMessage::clear();
}

QStringList FileList::selectedFiles() const
{
    QStringList list;
    for (auto i: selectionModel()->selectedRows())
        list.append(path(i.row()));
    return list;
}

FileList::Item::Item(const QFileInfo& fileInfo) :
    QTreeWidgetItem({fileInfo.fileName(),
                    fileInfo.dir().path(),
                    QString::number(fileInfo.size()),
                    fileInfo.lastModified().toString(Qt::SystemLocaleShortDate)}),
    mFileInfo(fileInfo)
{

}

QByteArray FileList::Item::calculateHash()
{
    QFile file(absoluteFilePath());
    QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Sha1);

    // calculate file sha-1 hash

    if (!file.open(QIODevice::ReadOnly))
        throw tr("Unable to open '%1'").arg(file.fileName());

    if (!hashCalculator.addData(&file))
        throw tr("Cannot read '%1'").arg(file.fileName());

    const auto hash = hashCalculator.result();
    setText(Item::eHash, hash.toBase64());
    return hash;
}

bool FileList::Item::operator <(const QTreeWidgetItem& other) const
{
    const auto& otherItem = static_cast<const Item&>(other);

    switch (treeWidget()->sortColumn())
    {
    case eSize:
        return mFileInfo.size() < otherItem.mFileInfo.size();

    case eLastModified:
        return mFileInfo.lastModified() < otherItem.mFileInfo.lastModified();

    default:
        return QTreeWidgetItem::operator <(other);
    }
}
