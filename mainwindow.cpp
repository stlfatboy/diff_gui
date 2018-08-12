#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
#include <QDebug>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

      // Not using model
//    auto model = new QStringListModel(m_filelist);
//    ui->Filelistview->setModel(model);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadfilelist()
{
    //read list
    m_filelist << "file1";
    m_filelist << "file2";
    m_filelist << "file3";


    // show file list
    for (auto file : m_filelist)
    {
        //TODO: store these item objects
        //dont forget delete
        QListWidgetItem* item = new QListWidgetItem(file, ui->Filelistwidget);
        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
}

void MainWindow::createconnections()
{

}

void MainWindow::on_Filelistwidget_itemChanged(QListWidgetItem *item)
{
    if (item->checkState() == Qt::Checked)
    {
        if (m_targetfilelist.empty())
        {
            m_targetfilelist.push_back(item->text());
            ui->LogText->appendPlainText(QStringLiteral("Add %1, Remain: %2").arg(item->text()).arg(m_targetfilelist.size()));
            return;
        }

        auto itr = m_targetfilelist.begin();
        while (itr != m_targetfilelist.end())
        {
            QString s = *itr;
            if(s == item->text())
            {
                return;
            }
            ++itr;
        }

        m_targetfilelist.push_back(item->text());
        ui->LogText->appendPlainText(QStringLiteral("Add %1, Remain: %2").arg(item->text()).arg(m_targetfilelist.size()));
    }
    else if (item->checkState() == Qt::Unchecked)
    {
        auto itr = m_targetfilelist.begin();
        while (itr != m_targetfilelist.end())
        {
            QString s = *itr;
            if(s == item->text())
            {
                m_targetfilelist.removeOne(*itr);
                ui->LogText->appendPlainText(QStringLiteral("Remove %1, Remain: %2").arg(item->text()).arg(m_targetfilelist.size()));
                //qInfo() << "Remove " << item->text() << ", Remain:" << m_targetfilelist.size();
                return;
            }
            ++itr;
        }

    }
}

void MainWindow::on_pushButton_clicked()
{
    //QProcess::execute("mousepad /home/ducksoul/Documents/a.txt");
    // load file list
    loadfilelist();
}
