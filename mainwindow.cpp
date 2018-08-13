#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
//#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setaddr(char *addr)
{
    m_addr = addr;
    ui->LogText->appendPlainText(QStringLiteral("Current Path: %1").arg(m_addr));
}

void MainWindow::loadfilelist()
{
    //read list
    QFile file("filelist");
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        while(!in.atEnd())
        {
            QString line = in.readLine();
            std::string stline = line.toStdString();

            // locate modified file
            std::size_t pos = stline.find_first_of("M");
            if (pos != std::string::npos && pos == 0)
            {
                //ui->LogText->appendPlainText(QStringLiteral("Readline %1").arg(line));
                //qDebug() << line;
                m_filelist << stline.substr(8).c_str();
            }

        }
    }

    file.remove();
}

void MainWindow::showfilelist()
{
    for (auto file : m_filelist)
    {
        //TODO: store these item objects
        //dont forget delete
        QListWidgetItem* item = new QListWidgetItem(file, ui->Filelistwidget);
        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
}

void MainWindow::gen_targetfilelist()
{
    // create diff cmd
    m_diff = "svn diff ";
    for(auto target : m_targetfilelist)
    {
        m_diff += target;
        m_diff += " ";
    }
    m_diff += "> diff.patch";
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
    // get diff status
    QFile file("Diff_utility.bat");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        out << "cd " << m_addr << "\n";
        out << "svn st > filelist";
    }
    system("Diff_utility.bat");
    ui->LogText->appendPlainText(QStringLiteral("Acquire Modified File List"));

    // load file list
    loadfilelist();

    // show file list
    showfilelist();
    ui->pushButton->setDisabled(true);
}

void MainWindow::on_pushButton_2_clicked()
{
    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Target File List Empty"));
        return;
    }

    gen_targetfilelist();

    //ui->LogText->appendPlainText(diff);
    if (system(m_diff.toStdString().c_str()))
    {
        ui->LogText->appendPlainText(QStringLiteral("Create diff.patch Failed"));
    }
    else
    {
        ui->LogText->appendPlainText(QStringLiteral("Create diff.patch Successfully"));
    }
}


