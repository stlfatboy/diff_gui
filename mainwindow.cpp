#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // load file list
    loadfilelist();

    auto model = new QStringListModel(m_filelist);
    ui->Filelistview->setModel(model);

    for (auto file : m_filelist)
    {
        //TODO: store these item objects
        //dont forget delete
        QListWidgetItem* item = new QListWidgetItem(file, ui->Filelistwidget);
        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadfilelist()
{
    m_filelist << "file1";
    m_filelist << "file2";
    m_filelist << "file3";
}

void MainWindow::createconnections()
{

}

void MainWindow::on_Filelistview_doubleClicked(const QModelIndex &index)
{
    const QString filename = index.data().toString();
    ui->LogText->appendPlainText(QStringLiteral("Add '%1'").arg(filename));
}

void MainWindow::on_Filelistwidget_itemChanged(QListWidgetItem *item)
{
    if (item->checkState() == Qt::Checked)
    {
        if (m_targetfilelist.empty())
        {
            m_targetfilelist.push_back(item->text());
            qInfo() << "Add " << item->text() << ", Remain:" << m_targetfilelist.size();
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
        qInfo() << "Add " << item->text() << ", Remain:" << m_targetfilelist.size();
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
                qInfo() << "Remove " << item->text() << ", Remain:" << m_targetfilelist.size();
                return;
            }
            ++itr;
        }

    }
}
