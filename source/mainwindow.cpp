#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>

#include "settingsdialog.h"
#include "statusmessage.h"

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
    loadSettings();
    StatusMessage::setStatusBar(ui->statusBar);
    StatusMessage::show(tr("Drag'n'drop files here"), StatusMessage::mcInfinite);

    connect(ui->fileList, &FileList::warn, [this](const QString& message){
        QMessageBox::warning(this, "", message);
    });
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

void MainWindow::on_actionSettings_triggered()
{
    QSettings settings;
    SettingsDialog dialog;

    dialog.setDiffCommand(settings.value(tags.diff.command).toString());

    if (dialog.exec() != QDialog::Accepted)
        return;

    settings.setValue(tags.diff.command, dialog.diffCommand());
}

void MainWindow::on_actionRemove_triggered()
{
    ui->fileList->removeSelectedItems();
}

void MainWindow::on_actionDiff_triggered()
{
    const auto selection = ui->fileList->selectedFiles();
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
        on_actionSettings_triggered();
        return;
    }

    QProcess::execute(command.arg(selection[0], selection[1]));
}

void MainWindow::on_actionEdit_triggered()
{
    const auto selection = ui->fileList->selectedFiles();
    if (selection.isEmpty())
    {
        StatusMessage::show(tr("Nothing selected"));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(selection.first()));
}
