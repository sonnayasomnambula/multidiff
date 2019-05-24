#ifndef FILELIST_H
#define FILELIST_H

#include <QTreeWidget>
#include <deque>

/// QTreeWidget with top-level items only, presented file info (see enum EColumn)
class FileList : public QTreeWidget
{
    Q_OBJECT

public:
    explicit FileList(QWidget* parent);

    /// \brief add file info
    /// \param url local file url
    void add(const QUrl& url);

    /// full filename with path
    QString path(int row) const;

    void removeSelected();
    QStringList selectedFiles() const;

signals:
    void warn(const QString& message);

private:
    /// only for reference - all is set in the .ui file
    enum EColumn { eName, eDir, eSize, eLastModified, eHash, ColCount };

    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    void highlight(bool on = true);

    /// recalculate file-to-file diff and set icons
    void rediff();

    static bool acceptable(const QMimeData* mime);
    static bool compare(const QString& path1, const QString& path2);
    static QIcon square(QColor color, int size = 64);

    /// I want to disable the sort after 3rd click on the same column header
    class HeaderWatcher
    {
    public:
        bool thirdClickInARow(int column);

    private:
        std::deque<int> mClicked{ -1, -1, -1 };
    } mHeaderWatcher;
};

#endif // FILELIST_H
