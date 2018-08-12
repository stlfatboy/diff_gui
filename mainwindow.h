#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadfilelist();
    void createconnections();

private slots:
    void on_Filelistwidget_itemChanged(QListWidgetItem *item);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QStringList m_filelist;
    QStringList m_targetfilelist;
};

#endif // MAINWINDOW_H
