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

QString FileList::path(int row) const
{
    return static_cast<Item*>(topLevelItem(row))->absoluteFilePath();
}

void FileList::dragEnterEvent(QDragEnterEvent* e)
{
    if (!isAcceptable(e->mimeData()))
        return;

    highlightDropArea();
    e->acceptProposedAction();
}

void FileList::dragMoveEvent(QDragMoveEvent* e)
{
    e->acceptProposedAction();
}

void FileList::dragLeaveEvent(QDragLeaveEvent* e)
{
    highlightDropArea(false);
    e->accept();
}

void FileList::dropEvent(QDropEvent* e)
{
    highlightDropArea(false);

    if (!isAcceptable(e->mimeData()))
        return;

    e->acceptProposedAction();
    append(e->mimeData()->urls());
}

void FileList::append(const QList<QUrl>& urls)
{
    class ColorGenerator
    {
    public:
        QColor next() {
            if (mColor < Qt::transparent)
                return static_cast<Qt::GlobalColor>(mColor++);

            return { qrand() % 255, qrand() % 255, qrand() % 255 };
        }

    private:
        int mColor = Qt::white;
    };

    /// The class accumulates warnings while generating a list of FileList::Item
    class Collector
    {
    public:
        Collector(FileList* treeWidget) : mTreeWidget(treeWidget) {}

        void collect(const QList<QUrl>& urls)
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
                    mWarnings.append(tr("Cannot add '%1'").arg(path));
            }

            StatusMessage::show(tr("There are %1/%2 unique files")
                                    .arg(mColors.size())
                                    .arg(mItems.size()),
                                StatusMessage::mcInfinite);
        }

        const auto& warnings() const { return mWarnings; }
        const auto& collected() const { return mItems; }

    private:
        void appendFile(const QString& path)
        {
            QFile file(path);
            QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Sha1);

            // calculate file sha-1 hash

            if (!file.open(QIODevice::ReadOnly))
            {
                mWarnings.append(tr("Unable to open '%1'").arg(path));
                return;
            }

            if (!hashCalculator.addData(&file))
            {
                mWarnings.append(tr("Cannot read '%1'").arg(path));
                return;
            }

            StatusMessage::show(tr("Calculate hash for '%1'...").arg(path), StatusMessage::mcInfinite);
            const auto hash = hashCalculator.result();

            // it is already known hash or a brand new one?

            auto icolor = mColors.find(hash);
            if (icolor == mColors.cend())
                icolor = mColors.insert(hash, mColorGenerator.next());

            mItems.append(new Item(path, hash, icolor.value()));
        }

        void appendDir(const QString &path)
        {
            const auto response = QMessageBox::question(mTreeWidget, "",
                tr("'%1' is a directory.\nDo you want to add all files from this directory?").arg(path));

            if (response != QMessageBox::Yes)
                return;

            QDirIterator files(path, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
            while (files.hasNext())
            {
                appendFile(files.next());
            }
        }

        QList<Item*> mItems; ///< Generated items
        FileList* mTreeWidget = nullptr;
        QStringList mWarnings; ///< Localized non-fatal error messages
        QMap<QByteArray, QColor> mColors; ///< Each hash have unique color
        ColorGenerator mColorGenerator;
    };

    Collector collector(this);

    {
        WidgetLocker widgetLocker(this);
        AppCursorLocker cursorLocker;

        collector.collect(urls);

        for (const auto item: collector.collected())
            insertTopLevelItem(count(), item);
    }

    if (!collector.warnings().isEmpty())
        QMessageBox::warning(this, "", collector.warnings().join("\n"));
}

void FileList::highlightDropArea(bool on)
{
    static const auto normal = palette();
    static const auto highlighted = [this]() {
        QPalette p = palette();
        p.setColor(QPalette::Window, p.color(QPalette::Highlight));
        p.setColor(QPalette::Base, p.color(QPalette::Highlight).lighter(220));
        return p;
    }();

    setPalette(on ? highlighted : normal);
}

bool FileList::isAcceptable(const QMimeData* mime)
{
    const auto urls = mime->urls();
    return mime->hasUrls() && !urls.isEmpty() &&
            std::all_of(urls.cbegin(), urls.cend(), [](const QUrl& url){ return url.isLocalFile(); });
}

QModelIndex FileList::indexFromRow(int row) const
{
    return indexFromItem(topLevelItem(row));
}

int FileList::count() const
{
    return topLevelItemCount();
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

void FileList::showDuplicates()
{
    if (count() == 0)
    {
        StatusMessage::show(tr("No files"));
        return;
    }

    int topRow = selectedIndexes().isEmpty() ? 0 : selectedIndexes().first().row() + 1;
    if (topRow >= count())
        topRow = 0;

    for (int top = topRow; top < count(); ++top)
    {
        const auto hash = topLevelItem(top)->text(Item::eHash);
        setCurrentIndex(indexFromRow(top));

        for (int row = top + 1; row < count(); ++row)
        {
            if (topLevelItem(row)->text(Item::eHash) == hash)
                selectionModel()->select(indexFromRow(row), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        if (selectionModel()->selectedRows().size() > 1)
            return;
    }

    selectionModel()->clear();
    StatusMessage::show(tr("No duplicates after row %1").arg(topRow));
}

FileList::Item::Item(const QFileInfo& fileInfo, const QByteArray& hash, QColor color) :
    QTreeWidgetItem({fileInfo.fileName(),
                    fileInfo.dir().path(),
                    QString::number(fileInfo.size()),
                    fileInfo.lastModified().toString(Qt::SystemLocaleShortDate),
                    QString(hash.toBase64())}),
    mFileInfo(fileInfo)
{
    setIcon(eName, coloredSquarePixmap(color));
}

QPixmap FileList::Item::coloredSquarePixmap(QColor color, int size)
{
    QPixmap pix(size, size);
    QPainter painer(&pix);
    painer.setBrush(color);
    painer.setPen(Qt::black);
    painer.drawRect(pix.rect().adjusted(1, 1, -2, -2));
    return pix;
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
