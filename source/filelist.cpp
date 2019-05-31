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
#include <QRandomGenerator>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include "fileinfomodel.h"
#include "statusmessage.h"
#include "widgetlocker.h"

class Base64Delegate : public QStyledItemDelegate
{
public:
    explicit Base64Delegate(QObject *parent = nullptr) :
        QStyledItemDelegate(parent)
    {}

    QString displayText(const QVariant& value, const QLocale&) const override
    {
        return value.toByteArray().toBase64();
    }
};

FileList::FileList(QWidget* parent) :
    QTreeView(parent),
    mModel(new FileInfoModel(this)),
    mProxy(new QSortFilterProxyModel(this))
{
    mProxy->setSourceModel(mModel);
    setModel(mProxy);
    setItemDelegateForColumn(FileInfoModel::eHash, new Base64Delegate(this));

    // Disable the sort after 3rd click on the same column header
    connect(header(), &QHeaderView::sortIndicatorChanged, [this, mClicked = std::deque<int>{ -1, -1, -1 }](int column) mutable {
        mClicked.push_front(column);
        mClicked.pop_back();
        Q_ASSERT(mClicked.size() == 3);

        if (column >= 0 && mClicked[0] == mClicked[1] && mClicked[1] == mClicked[2])
            sortByColumn(-1);
    });
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
    add(e->mimeData()->urls());
}

void FileList::add(const QList<QUrl>& urls)
{
    FileInfoModel::Collector collector(this);

    {
        AppCursorLocker acl;
        WidgetLocker wl(this);

        collector.collect(urls);
        mModel->add(collector.collected());
    }

    if (!collector.warnings().isEmpty())
        QMessageBox::warning(this, "", collector.warnings().join("\n"));
}

void FileList::removeSelected()
{
    const auto selection = selectionModel()->selectedRows();

    // extract rows, map proxy to data, sort, remove duplicates if any
    std::set<int, std::greater<int>> rows;
    std::transform(selection.cbegin(), selection.cend(), std::inserter(rows, rows.begin()), [this](const QModelIndex& index) {
        return mProxy->mapToSource(index).row();
    });

    mModel->remove(rows);
}

void FileList::selectNextDuplicates()
{
    const auto rowCount = mProxy->rowCount();

    if (rowCount == 0)
    {
        StatusMessage::show(tr("No files"));
        return;
    }

    int topRow = currentIndex().row() + 1;
    if (selectionModel()->selectedRows().isEmpty() || topRow >= rowCount)
        topRow = 0;

    auto top = mProxy->index(topRow, FileInfoModel::eHash);
    StatusMessage::show(tr("Search..."), StatusMessage::mcInfinite);

    while (top.isValid())
    {
        const auto hash = mProxy->data(top);
        setCurrentIndex(top);

        auto index = top;
        while (index = indexBelow(index), index.isValid())
        {
            if (mProxy->data(index) == hash)
                selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        const auto size = selectionModel()->selectedRows().size();
        if (size > 1)
        {
            const QString hashString(hash.toByteArray().toBase64());
            StatusMessage::show(tr("Found %n file(s) with hash %1", "", size).arg(hashString),
                                StatusMessage::mcInfinite);
            return;
        }

        top = indexBelow(top);
    }

    selectionModel()->clear();
    StatusMessage::show(tr("No duplicates after row %1").arg(topRow));
}

QFileInfo FileList::fileInfo(const QModelIndex& index) const
{
    if (index.row() >= mModel->rowCount())
        return {};

    return mModel->item(mProxy->mapToSource(index).row()).fileInfo;
}

void FileList::highlightDropArea(bool on)
{
    static const auto normal = palette();
    static const auto highlighted = [this] {
        auto p = palette();
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
