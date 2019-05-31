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
} static const tags;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->fileList, &FileList::doubleClicked, this, &MainWindow::on_actionEdit_triggered);

    loadSettings();
    StatusMessage::setStatusBar(ui->statusBar);
    StatusMessage::show(tr("Drag'n'drop files here"), StatusMessage::mcInfinite);
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
    restoreGeometry(settings.value(tags.main.geometry).toByteArray());
    restoreState(settings.value(tags.main.state).toByteArray());
    ui->fileList->header()->restoreState(settings.value(tags.main.header).toByteArray());
}

void MainWindow::storeSettings()
{
    QSettings settings;
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

    QProcess::execute(command.arg(f1.absoluteFilePath(), f2.absoluteFilePath()));
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
