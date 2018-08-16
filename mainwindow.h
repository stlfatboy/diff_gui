#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QFile>
#include <unordered_map>
#include <QMap>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setaddr(char *addr);
    void startupjobs(char* addr);
    void loadfilelist();
    void showfilelist();
    void run_diffcmd();

private slots:
    void on_Filelistwidget_itemChanged(QListWidgetItem *item);

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    std::unordered_map<std::string, std::string> m_Real_Display;
    std::unordered_map<std::string, std::string> m_Display_Real;
    QStringList m_filelist;
    QStringList m_targetfilelist;

    QString m_addr;
    QString m_diff;
};

#endif // MAINWINDOW_H
