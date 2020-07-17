#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDebug>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>

#include "fileinfomodel.h"
#include "settingsdialog.h"
#include "statusmessage.h"
#include "widgetlocker.h"

/// tags for QSettings class
struct Tags
{
    struct Main {
        const QString geometry = QStringLiteral("main/geometry");
        const QString state = QStringLiteral("main/state");
        const QString header = QStringLiteral("main/header");
    } const main;

    struct Diff {
        const QString command = QStringLiteral("diff/command");
    } const diff;

    const QString version = QStringLiteral("version");
} static const tags;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->fileList, &FileList::doubleClicked, this, &MainWindow::on_actionEdit_triggered);

    loadSettings();
    setupActions();

    StatusMessage::setStatusBar(ui->statusBar);
    StatusMessage::show(tr("Drag'n'drop files here"), StatusMessage::mcInfinite);
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
    QSettings settings;
    if (settings.value(tags.version).toString() != qApp->applicationVersion())
        return;

    restoreGeometry(settings.value(tags.main.geometry).toByteArray());
    restoreState(settings.value(tags.main.state).toByteArray());
    ui->fileList->header()->restoreState(settings.value(tags.main.header).toByteArray());
}

void MainWindow::storeSettings()
{
    QSettings settings;
    settings.setValue(tags.version, qApp->applicationVersion());
    settings.setValue(tags.main.geometry, saveGeometry());
    settings.setValue(tags.main.state, saveState());
    settings.setValue(tags.main.header, ui->fileList->header()->saveState());
}

bool MainWindow::on_actionSettings_triggered()
{
    QSettings settings;
    SettingsDialog dialog;

    dialog.setDiffCommand(settings.value(tags.diff.command).toString());

    if (dialog.exec() != QDialog::Accepted)
        return false;

    settings.setValue(tags.diff.command, dialog.diffCommand());
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

    QSettings settings;
    QString command = settings.value(tags.diff.command).toString();
    if (command.isEmpty())
    {
        StatusMessage::show(tr("Please set side-by-side diff command"), 15000);
        if (!on_actionSettings_triggered())
            return;
    }

    const auto f1(ui->fileList->fileInfo(selection[0]));
    const auto f2(ui->fileList->fileInfo(selection[1]));
    constexpr qint64 halfMB= 512 * 1024;
    if (f1.size() > halfMB || f2.size() > halfMB)
    {
        const auto response = QMessageBox::question(this, "", tr("Files are too big! Show anyway?"));
        if (response != QMessageBox::Yes)
            return;
    }

    QProcess::execute(command.arg(f1.absoluteFilePath(), f2.absoluteFilePath()), {});
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
