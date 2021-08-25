#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QFile>
#include <QDir>
#include <unordered_map>
#include <QMap>
#include <QtAutoUpdaterCore>


enum TortoiseSVNCMD
{
    resolve = 0,
    repostatus
};

namespace Ui {
class MainWindow;
}

class CommitDialog;
class ProgressDialog;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void setaddr(char *addr);
    void startupjobs(char* addr);
    void checkVersionConsistency();
    void loadfilelist(QByteArray & data, int workingdir = -1);
    void showfilelist(const QString & filter, bool isRefresh = false);
    void findrepo(int deepth);

private slots:
    void hasUpdate(QtAutoUpdater::Updater::State state);

    void on_Filelistwidget_itemChanged(QListWidgetItem *item);

    void on_checkBox_stateChanged(int arg1);

    void onCommitDialogFinished(int result);

    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_pushButton_gen_diff_clicked();

    void on_pushButton_svn_ci_clicked();

    void on_Filelistwidget_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_svn_up_clicked();

    void on_pushButton_svn_re_clicked();

    void on_pushButton_refresh_clicked();

signals:

    void inner_startupjobs(char* addr);

private:

    bool svn_cli_execute(const QString &addr, const QStringList & args,  QByteArray *result = nullptr);

    bool svn_tortoise_execute(TortoiseSVNCMD cmd, const QString &addr, int closeonend = 0);

    void update_repo(int revision);

    Ui::MainWindow *ui;
    CommitDialog* ci_dialog;
    std::vector<QListWidgetItem*> m_ListItemVec;
    ProgressDialog* progress_dialog;

    QtAutoUpdater::Updater* m_updater;

    std::unordered_map<std::string, std::string>    m_Real_Display;
    std::unordered_map<std::string, std::string>    m_Display_Real;
    std::unordered_map<std::string, int>            m_Real_Dir;

    QDir m_dir;
    bool m_root_repo;
    QStringList m_dirlist;
    QStringList m_filelist;
    QStringList m_targetfilelist;
    int m_target_revision;

    QString m_addr;
    QString m_diff;

    QFile* m_diff_file;
};

#endif // MAINWINDOW_H
