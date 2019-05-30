#ifndef FILELIST_H
#define FILELIST_H

#include <deque>

#include <QFileInfo>
#include <QTreeWidget>

/// QTreeWidget with top-level items only, presented file info
class FileList : public QTreeWidget
{
    Q_OBJECT

public:
    explicit FileList(QWidget* parent);

    void append(const QList<QUrl>& urls);
    int count() const { return topLevelItemCount(); }

    /// full filename with path
    QString path(int row) const;

    void removeSelectedItems();

    /// \return full filename of each selected item
    QStringList selectedFiles() const;

    /// Search for duplicates beginning {current row + 1} and select it
    void showDuplicates();

private:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    /// Highlight the drop area
    void highlightDropArea(bool on = true);

    /// Only local files can be dropped
    static bool isAcceptable(const QMimeData* mime);

    QModelIndex indexFromRow(int row) const;

    class Item : public QTreeWidgetItem
    {
    public:
        /// Columns
        /// Only for reference - all is set in the .ui file
        enum { eName, eDir, eSize, eLastModified, eHash, ColCount };

        explicit Item(const QFileInfo& fileInfo, const QByteArray& hash);

        void setColor(QColor color);

        QString absoluteFilePath() const { return mFileInfo.absoluteFilePath(); }
        QByteArray hash() const;

        /// Draw a colored square pixmap with 1px black border
        static QPixmap coloredSquarePixmap(QColor color, int size = 64);

    private:
        bool operator <(const QTreeWidgetItem& other) const;

        QFileInfo mFileInfo;
    };
};

#endif // FILELIST_H
