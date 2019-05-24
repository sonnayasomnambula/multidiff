#include "filelist.h"

#include <QAction>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDropEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QMimeData>
#include <QPainter>

#include <algo.h>
#include <eventwatcher.h>

#include "statusmessage.h"
#include "widgetlocker.h"

FileList::FileList(QWidget* parent) :
    QTreeWidget(parent)
{
    connect(header(), &QHeaderView::sortIndicatorChanged, [this](int logicalIndex){
        if (mHeaderWatcher.thirdClickInARow(logicalIndex))
            sortByColumn(-1);
    });
}

bool FileList::HeaderWatcher::thirdClickInARow(int column)
{
    mClicked.push_front(column);
    mClicked.pop_back();
    Q_ASSERT(mClicked.size() == 3);

    if (column < 0)
        return false;

    return mClicked[0] == mClicked[1] && mClicked[1] == mClicked[2];
}

void FileList::add(const QUrl& url)
{
    const QFileInfo fileInfo(url.toLocalFile());

    insertTopLevelItem(topLevelItemCount(), new QTreeWidgetItem({url.fileName(),
                                                                 fileInfo.dir().path(),
                                                                 QString::number(fileInfo.size()),
                                                                 fileInfo.lastModified().toString(Qt::SystemLocaleShortDate)}));
}

QString FileList::path(int row) const
{
    QDir dir(topLevelItem(row)->text(EColumn::eDir));
    QString name(topLevelItem(row)->text(EColumn::eName));
    return dir.absoluteFilePath(name);
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
        add(url);

    rediff();
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

bool FileList::compare(const QString& path1, const QString& path2)
{
    return path1 == path2; // TODO
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
            return QColor(static_cast<Qt::GlobalColor>(id));

        const int r = qrand() % 255;
        const int g = qrand() % 255;
        const int b = qrand() % 255;
        return QColor(r, g, b);
    }

    int mColor = Qt::white;
};

void FileList::rediff()
{
    WidgetLocker wl(this);
    AppCursorLocker acl;

    QMap<QByteArray, QColor> colors; // hash --> color
    ColorGenerator colorGenerator;

    QStringList warnings;

    for (int row=0; row<topLevelItemCount(); ++row)
    {
        QFile file(path(row));
        QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Sha1);

        StatusMessage::show(tr("Calculate hash for %1...").arg(file.fileName()), 0);

        // calculate file sha-1 hash

        if (!file.open(QIODevice::ReadOnly))
        {
            warnings.append(tr("Unable to open '%1'").arg(file.fileName()));
            continue;
        }

        if (!hashCalculator.addData(&file))
        {
            warnings.append(tr("Cannot read '%1'").arg(file.fileName()));
            continue;
        }

        // it is already known hash or a brand new one?

        const auto hash = hashCalculator.result();
        auto icolor = colors.find(hash);
        if (icolor == colors.cend())
            icolor = colors.insert(hash, colorGenerator.next());

        topLevelItem(row)->setIcon(EColumn::eName, square(icolor.value()));
        topLevelItem(row)->setText(EColumn::eHash, hash.toBase64());
    }

    if (!warnings.isEmpty())
        emit warn(warnings.join("\n"));

    StatusMessage::show(tr("Done!"));
}

void FileList::removeSelected()
{
    for (auto item: selectedItems())
        delete takeTopLevelItem(indexOfTopLevelItem(item));
}

QStringList FileList::selectedFiles() const
{
    QStringList list;
    for (auto i: selectionModel()->selectedRows())
        list.append(path(i.row()));
    return list;
}
