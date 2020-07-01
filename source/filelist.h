#ifndef FILELIST_H
#define FILELIST_H

#include <deque>

#include <QFileInfo>
#include <QTreeView>
#include <QUrl>

class FileInfoModel;
class QSortFilterProxyModel;

/// QTreeWidget with top-level items only, presented file info
class FileList : public QTreeView
{
    Q_OBJECT

public:
    explicit FileList(QWidget* parent);

    void add(const QList<QUrl>& urls);
    void remove(QModelIndexList what);
    void removeSelected();

    /// Select the rows with the same 'Hash' column value
    /// Search from {current row + 1} or from begin, if nothing selected
    void selectNextDuplicates();

    QFileInfo fileInfo(const QModelIndex& index) const;

private:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    /// Highlight the drop area
    void highlightDropArea(bool on = true);

    /// Only local files can be dropped
    static bool isAcceptable(const QMimeData* mime);

    FileInfoModel* mModel = nullptr;
    QSortFilterProxyModel* mProxy = nullptr;
};

#endif // FILELIST_H
