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
    void on_actionSettings_triggered();
    void on_actionRemove_triggered();
    void on_actionDiff_triggered();
    void on_actionEdit_triggered();
    void on_actionShow_duplicates_triggered();

private:
    void closeEvent(QCloseEvent* e) override;

    void loadSettings();
    void storeSettings();

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
