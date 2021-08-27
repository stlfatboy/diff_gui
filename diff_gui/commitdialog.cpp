#include "commitdialog.h"
#include "ui_ci_dialog.h"

#include <QFile>

CommitDialog::CommitDialog(QWidget * parent)
    : QDialog(parent)
    , ui(new Ui::CIDialog)
{
    ui->setupUi(this);

    QFile file(FILENAME);
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream history(&file);
        while(history.status() != QDataStream::ReadPastEnd)
        {
            QString data;
            history >> data;
            ui->comboBox->addItem(data);
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
    QFile file(FILENAME);
    if(!file.open(QIODevice::ReadWrite))
    {
        return;
    }

    ui->comboBox->addItem(data);
    if (ui->comboBox->count() > 30)
    {
        ui->comboBox->removeItem(0);

        file.readLine();
    }
    QDataStream history(&file);
    history << file.readAll();
    history << data;

    file.close();
}
