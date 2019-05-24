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

    void append(const QString& path);

    /// full filename with path
    QString path(int row) const;

    void removeSelectedItems();

    /// \return full filename of each selected item
    QStringList selectedFiles() const;

    /// Search for duplicates beginning {current row + 1} and select it
    void showDuplicates();

signals:
    /// Warn user about something
    /// \param message localized message
    void warn(const QString& message);

private:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    /// Highlight the drop area
    void highlight(bool on = true);

    /// Calculate file hashes and update icons
    void calculateHashes();

    /// Draw a colored square icon
    static QIcon square(QColor color, int size = 64);

    /// Only local files can be dropped
    static bool acceptable(const QMimeData* mime);

    QModelIndex indexFromRow(int row) const;
    int count() const;

    class Item : public QTreeWidgetItem
    {
    public:
        /// Columns
        /// Only for reference - all is set in the .ui file
        enum { eName, eDir, eSize, eLastModified, eHash, ColCount };

        Item(const QFileInfo& fileInfo);

        QString absoluteFilePath() const { return mFileInfo.absoluteFilePath(); }

        /// Calculate file SHA-1 and update column `eHash` text
        /// \throws localized message as QString
        QByteArray calculateHash();

    private:
        bool operator <(const QTreeWidgetItem& other) const;

        QFileInfo mFileInfo;
    };
};

#endif // FILELIST_H
