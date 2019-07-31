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

namespace QtAutoUpdater {
class Updater;
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

    void hasUpdate(bool hasUpdate, bool hasError);

private slots:
    void on_Filelistwidget_itemChanged(QListWidgetItem *item);

    void on_pushButton_2_clicked();

    void on_checkBox_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
    std::vector<QListWidgetItem*> m_ListItemVec;

    std::unordered_map<std::string, std::string> m_Real_Display;
    std::unordered_map<std::string, std::string> m_Display_Real;
    QStringList m_filelist;
    QStringList m_targetfilelist;

    QString m_addr;
    QString m_diff;

    QFile* m_diff_file;

    std::shared_ptr<QtAutoUpdater::Updater> m_updater;
};

#endif // MAINWINDOW_H
