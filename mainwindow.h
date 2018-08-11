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
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadfilelist();

private slots:
    void on_Filelistview_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QStringList m_filelist;
};

#endif // MAINWINDOW_H
