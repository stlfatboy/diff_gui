#include "commitdialog.h"
#include "ui_ci_dialog.h"

#include <QFile>
#include <QDir>
#include <QDebug>

static const QString file_name = QDir::homePath() + "/Diff_Utils/" + FILENAME;

CommitDialog::CommitDialog(QWidget * parent)
    : QDialog(parent)
    , ui(new Ui::CIDialog)
{
    ui->setupUi(this);
    QFile file(file_name);
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream history(&file);
        while(history.status() != QDataStream::ReadPastEnd)
        {
            QString data;
            history >> data;
            ui->comboBox_ci_history->addItem(data);
            qDebug() << data;
        }
    }
}


CommitDialog::~CommitDialog()
{
    delete ui;
}

QString CommitDialog::getMessage()
{
    return ui->plainTextEdit->toPlainText();
}

void CommitDialog::addHistory(QString &data)
{
    QFile file(file_name);
    if(!file.open(QIODevice::ReadWrite))
    {
        QDir dir;
        dir.mkpath(QDir::homePath() + "/Diff_Utils/");
        if(!file.open(QIODevice::ReadWrite)) return;
    }

    ui->comboBox_ci_history->addItem(data);
    QDataStream history(&file);
    if (ui->comboBox_ci_history->count() > 30)
    {
        ui->comboBox_ci_history->removeItem(0);
        QString ignore;
        history >> ignore;
    }
    history << data;

    file.close();
}


void CommitDialog::on_comboBox_ci_history_currentTextChanged(const QString &arg1)
{
    ui->plainTextEdit->clear();
    ui->plainTextEdit->insertPlainText(arg1);
}

