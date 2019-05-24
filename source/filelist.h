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

    /// full filename with path
    QString path(int row) const;

    void removeSelectedItems();

    /// \return full filename of each selected item
    QStringList selectedFiles() const;

signals:
    void warn(const QString& message);

private:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    void highlight(bool on = true);

    /// Calculate file hashes and update icons
    void calculateHashes();

    static QIcon square(QColor color, int size = 64);
    static bool acceptable(const QMimeData* mime);

    class Item : public QTreeWidgetItem
    {
    public:
        Item(const QFileInfo& fileInfo);

        QString absoluteFilePath() const { return mFileInfo.absoluteFilePath(); }

        /// throws QString
        QByteArray calculateHash();

    private:
        /// only for reference - all is set in the .ui file
        enum { eName, eDir, eSize, eLastModified, eHash, ColCount };

        bool operator <(const QTreeWidgetItem& other) const;

        QFileInfo mFileInfo;
    };
};

#endif // FILELIST_H
