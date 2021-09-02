#ifndef COMMITDIALOG_H
#define COMMITDIALOG_H

#include <QDialog>

#define FILENAME "commit_history"

namespace Ui {
class CIDialog;
}

class CommitDialog : public QDialog
{
    Q_OBJECT
public:
    CommitDialog(QWidget * parent);
    ~CommitDialog();

    QString getMessage();
    void addHistory(QString & data);

signals:
    void log(QString data);

private slots:
    void on_comboBox_ci_history_currentTextChanged(const QString &arg1);

private:
    Ui::CIDialog* ui;
};

#endif // COMMITDIALOG_H
