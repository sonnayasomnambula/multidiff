#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::diffCommand() const
{
    return ui->diffCommand->text();
}

void SettingsDialog::setDiffCommand(const QString& diffCommand)
{
    ui->diffCommand->setText(diffCommand);
}
