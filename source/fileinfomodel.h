#ifndef FILEINFOMODEL_H
#define FILEINFOMODEL_H

#include <set>

#include <QAbstractTableModel>
#include <QFileInfo>
#include <QByteArray>
#include <QPixmap>

struct FileItem
{
    QFileInfo fileInfo;
    QByteArray hash;
    QPixmap pixmap;
};

class FileInfoModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    using QAbstractTableModel::QAbstractTableModel;

    enum { eName, eDir, eSize, eLastModified, eHash, ColCount };

    /// The class accumulates warnings while generating a list of items
    class Collector
    {
    public:
        Collector(QWidget* parent) : mParent(parent) {}
        void collect(const QList<QUrl>& urls);

        const auto& collected() const { return mItems; }
        const auto& warnings() const { return mWarnings; }

    private:
        void appendFile(const QString& path);
        void appendDir(const QString &path);

        QList<FileItem> mItems; ///< Collected data
        QWidget* mParent = nullptr; ///< Used for QMessageBox
        QStringList mWarnings; ///< Localized non-fatal error messages
        /// The last button clicked by the user; values YesToAll or Cancel require some special processing
        int mLastClickedButton = 0;
    };

    void add(const QList<FileItem>& items);
    void remove(std::set<int, std::greater<int>>& rows);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    FileItem item(unsigned row) const; // TODO: incapsulate

private:
    QVariant displayData(const QModelIndex &index) const;
    void updatePixmaps();

    /// Draw a colored square pixmap with 1px black border
    static QPixmap coloredSquarePixmap(QColor color, int size = 16);

    QList<FileItem> mData;
};

#endif // FILEINFOMODEL_H
