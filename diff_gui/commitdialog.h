#ifndef COMMITDIALOG_H
#define COMMITDIALOG_H

#include <QDialog>

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

private:
    Ui::CIDialog* ui;
};

#endif // COMMITDIALOG_H
