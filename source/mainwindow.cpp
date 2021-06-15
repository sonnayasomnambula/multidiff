#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>

#include "abstractsettings.h"
#include "fileinfomodel.h"
#include "settingsdialog.h"
#include "statusmessage.h"
#include "widgetlocker.h"

struct Settings : AbstractSettings
{
    struct
    {
        Tag state = "window/state";
        Tag geometry = "window/geometry";
        Tag headerState = "window/headerState";
    } window;

    struct
    {
        Tag command = "diff/command";
    } diff;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->fileList, &FileList::doubleClicked, this, &MainWindow::on_actionEdit_triggered);

    loadSettings();
    setupActions();

    StatusMessage::setStatusBar(ui->statusBar);
    StatusMessage::show(tr("Drag'n'drop files or directories here"), StatusMessage::mcInfinite);
}

void MainWindow::setupActions()
{
    ui->fileList->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->fileList->addAction(ui->actionEdit);
    ui->fileList->addAction(ui->actionRemove);
    ui->fileList->addAction(ui->actionDiff);

    connect(ui->fileList->selectionModel(), &QItemSelectionModel::selectionChanged, [this]{
        setActionsEnabled(!ui->fileList->selectionModel()->selectedRows().isEmpty());
    });
    setActionsEnabled(false);
}

void MainWindow::setActionsEnabled(bool enabled)
{
    ui->actionDelete_file->setEnabled(enabled);
    ui->actionDiff->setEnabled(enabled);
    ui->actionEdit->setEnabled(enabled);
    ui->actionRemove->setEnabled(enabled);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    storeSettings();
    QMainWindow::closeEvent(e);
}

void MainWindow::loadSettings()
{
    Settings settings;

    restoreGeometry(settings.window.geometry(saveGeometry()));
    restoreState(settings.window.state(saveState()));
    auto header = ui->fileList->header();
    header->restoreState(settings.window.headerState(header->saveState()));
}

void MainWindow::storeSettings()
{
    Settings settings;

    settings.window.geometry.save(saveGeometry());
    settings.window.state.save(saveState());
    settings.window.headerState.save(ui->fileList->header()->saveState());
}

void MainWindow::on_actionAdd_files_triggered()
{
    ui->fileList->add(QFileDialog::getOpenFileUrls(this));
}

void MainWindow::on_actionAdd_directory_triggered()
{
    ui->fileList->add({QFileDialog::getExistingDirectoryUrl(this)});
}

bool MainWindow::on_actionSettings_triggered()
{
    Settings settings;
    SettingsDialog dialog;

    dialog.setDiffCommand(settings.diff.command.value().toString());

    if (dialog.exec() != QDialog::Accepted)
        return false;

    settings.diff.command.save(dialog.diffCommand());
    return true;
}

void MainWindow::on_actionRemove_triggered()
{
    ui->fileList->removeSelected();
    StatusMessage::clear();
}

void MainWindow::on_actionDelete_file_triggered()
{
    const auto selection = ui->fileList->selectionModel()->selectedRows();
    if (QMessageBox::warning(
                this,
                tr("Remove files"),
                tr("Remove %n file(s) from disk?", nullptr, selection.size()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QMultiMap<QString, QModelIndex> removed; // path --> index
    for (auto& row: selection) {
        auto path = ui->fileList->fileInfo(row).absoluteFilePath();
        if (removed.contains(path) || QFile(path).remove()) { // the list may contain duplicates
            removed.insert(path, row);
        } else {
            QMessageBox::critical(
                        this,
                        tr("Remove files"),
                        tr("Cannot remove '%1'").arg(path));
            break;
        }
    }

    ui->fileList->remove(removed.values());
    StatusMessage::clear();
}

void MainWindow::on_actionDiff_triggered()
{
    const auto selection = ui->fileList->selectionModel()->selectedRows();
    if (selection.size() < 2)
    {
        StatusMessage::show(tr("Nothing selected"));
        return;
    }

    QString command = Settings().diff.command.value().toString();
    if (command.isEmpty())
    {
        StatusMessage::show(tr("Please set side-by-side diff command"), 15000);
        if (!on_actionSettings_triggered())
            return;
    }

    const auto f1(ui->fileList->fileInfo(selection[0]));
    const auto f2(ui->fileList->fileInfo(selection[1]));
    constexpr qint64 halfMB= 512 * 1024;
    if (f1.size() + f2.size() > halfMB)
    {
        const auto response = QMessageBox::warning(this, "",
            tr("Files are too big (%1)! Show anyway?").arg(humanReadableSize(f1.size() + f2.size())),
            QMessageBox::Yes | QMessageBox::No);
        if (response != QMessageBox::Yes)
            return;
    }

    QStringList args = { f1.absoluteFilePath(), f2.absoluteFilePath() };
    QString errorString;
    int status = QProcess::execute(command, args);
    if (status == -2)
    {
        errorString = tr("Unable to execute '%1': the process cannot be started").arg(command);
    }
    else if (status != 0)
    {
        errorString = tr("Command '%1' finished with status %2").arg(command).arg(status);
    }

    if (!errorString.isEmpty())
        QMessageBox::warning(this, "", errorString);
}

QString MainWindow::humanReadableSize(quint64 bytes)
{
    const QList<QString> suffixes = { tr("B"), tr("KB"), tr("MB"), tr("GB"), tr("TB") };
    double size = bytes;
    for (int i = 0; i < suffixes.size(); ++i)
    {
        if (size < 1024 || i == suffixes.size() - 1)
            return QString("%1 %2").arg(size, 0, 'f', 1).arg(suffixes[i]);
        size /= 1024.;
    }

    return "";
}

void MainWindow::on_actionEdit_triggered()
{
    const auto selection = ui->fileList->selectionModel()->selectedRows();
    if (selection.isEmpty())
    {
        StatusMessage::show(tr("Nothing selected"));
        return;
    }

    const auto path = ui->fileList->fileInfo(selection.first()).absoluteFilePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::on_actionShow_duplicates_triggered()
{
    ui->fileList->selectNextDuplicates();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QApplication::aboutQt();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "", tr("Diff over multiple files. Drag'n'drop files or directories here."));
}
