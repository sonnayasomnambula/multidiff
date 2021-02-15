#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionAdd_files_triggered();
    void on_actionAdd_directory_triggered();
    bool on_actionSettings_triggered();
    void on_actionRemove_triggered();
    void on_actionDelete_file_triggered();
    void on_actionDiff_triggered();
    void on_actionEdit_triggered();
    void on_actionShow_duplicates_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionAbout_triggered();

private:
    void closeEvent(QCloseEvent* e) override;

    void loadSettings();
    void storeSettings();

    void setupActions();
    void setActionsEnabled(bool enabled);

    static QString humanReadableSize(quint64 bytes);

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
