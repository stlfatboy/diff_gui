#include "commitdialog.h"
#include "ui_ci_dialog.h"
#include "logging.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

static const QString user_data_dir = QDir::homePath() + "/Diff_Utils/";
static const QString db_file_name = user_data_dir + FILENAME;
QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

CommitDialog::CommitDialog(QWidget * parent)
    : QDialog(parent)
    , ui(new Ui::CIDialog)

{
    ui->setupUi(this);
    QDir dir(user_data_dir);
    if (!dir.exists())
    {
        qInfo() << "Create User Data Folder";
        if (!dir.mkpath(user_data_dir))
        {
            qWarning() << "Create User Data Folder Failed";
        }
    }

    db.setDatabaseName(db_file_name);
    if (!db.open())
    {
        QMessageBox msgBox;
        msgBox.setText("Open Commit History DB Failed: " + db.lastError().text());
        msgBox.exec();
    }
    else
    {
        ui->comboBox_ci_history->setInsertPolicy(QComboBox::InsertBeforeCurrent);
        QSqlQuery query;
        query.exec("CREATE TABLE history (id INTEGER PRIMARY KEY, data TEXT NOT NULL)");
        qDebug() << "Create Table " + query.lastError().text();

        int count = 0;
        query.exec("SELECT * FROM history ORDER BY id DESC;");
        while(query.next())
        {
            QString data = query.value(1).toString();
            if (!data.isEmpty())
            {
                ui->comboBox_ci_history->addItem(data);
                qDebug() << QStringLiteral("Readed CIHistory(%1): %2").arg(query.value(0).toString()).arg(data);
                if (count++ > 20) break;
            }
        }

        qDebug() << "SELECT all " + query.lastError().text();
    }

    db.close();

//    QFile file(file_name);
//    if (file.open(QIODevice::ReadOnly))
//    {
//        QDataStream history(&file);
//        while(history.status() != QDataStream::ReadPastEnd)
//        {
//            QString data;
//            history >> data;
//            if (history.status() == QDataStream::ReadCorruptData)
//            {
//                qDebug() << "CI History ReadCorruptData: Delete it later";
//                m_refresh_history = true;
//                break;
//            }

//            if (!data.isEmpty())
//            {
//                ui->comboBox_ci_history->addItem(data);
//                qDebug() << "Readed CIHistory" << history.status() << ": " << data;
//            }
//        }

//        file.close();
//    }
}


CommitDialog::~CommitDialog()
{
    db.close();
    delete ui;
}

QString CommitDialog::getMessage()
{
    return ui->plainTextEdit->toPlainText();
}

void CommitDialog::addHistory(QString &data)
{
    if (-1 == ui->comboBox_ci_history->findText(data))
    {
        QString in = QStringLiteral("INSERT INTO history VALUES (%1,\"%2\");")
                .arg(QDateTime::currentDateTimeUtc().currentSecsSinceEpoch())
                .arg(data);
        db.open();
        QSqlQuery query;
        if (!query.exec(in))
        {
            QMessageBox msgBox;
            msgBox.setText("Commit history insert error" + query.lastError().text());
            qWarning() << "Commit history insert error" + query.lastError().text();
            msgBox.exec();
        }
        db.close();

        ui->comboBox_ci_history->addItem(data);
    }

//    if (m_refresh_history)
//    {
//        QFile::remove(file_name);
//        m_refresh_history = false;
//    }

//    QFile file(file_name);
//    if(!file.open(QIODevice::Append))
//    {
//        QDir dir;
//        dir.mkpath(QDir::homePath() + "/Diff_Utils/");
//        if(!file.open(QIODevice::ReadWrite)) return;
//    }

//    if (-1 == ui->comboBox_ci_history->findText(data))
//    {
//        qDebug() << "Added CIHistory: " << data;
//        ui->comboBox_ci_history->addItem(data);
//        QDataStream history(&file);
//        if (ui->comboBox_ci_history->count() > 30)
//        {
//            ui->comboBox_ci_history->removeItem(0);
//            QString ignore;
//            history >> ignore;
//        }
//        history << data;
//    }

//    file.close();
}


void CommitDialog::on_comboBox_ci_history_currentTextChanged(const QString &arg1)
{
    ui->plainTextEdit->clear();
    ui->plainTextEdit->insertPlainText(arg1);
}

