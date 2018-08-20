#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
//#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDate>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_diff_file(nullptr)
{
    ui->setupUi(this);
    ui->label->setText("Build." + QDate(2018,8,16).toString(Qt::ISODate));
    ui->pushButton_2->setDisabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_diff_file != nullptr)
    {
        m_diff_file->remove();
        delete m_diff_file;
    }
}

void MainWindow::setaddr(char *addr)
{
    m_addr = addr;
    ui->LogText->appendPlainText(QStringLiteral("Current Path: %1").arg(m_addr));

    if (m_addr.length() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Path Empty!"));
        return;
    }
}

void MainWindow::startupjobs(char *addr)
{
    setaddr(addr);

    if(m_addr.length() == 0)
    {
        return;
    }

    // get diff status
    QFile file("Diff_utility.bat");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        out << "cd " << m_addr << "\n";
        out << "svn st > filelist";
    }
    system("Diff_utility.bat");

    file.close();
    file.remove();

    // load file list
    loadfilelist();

    // show file list
    showfilelist();

    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("File List Empty"));
        return;
    }

    ui->pushButton_2->setEnabled(true);
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
            std::string filename;

            // locate modified file
            std::size_t pos = stline.find_first_of("M");
            if (pos != std::string::npos && pos == 0)
            {
                filename = stline.substr(8);
                m_filelist << filename.c_str();
                m_Real_Display.insert({filename, stline});
                m_Display_Real.insert({stline, filename});
            }

            pos = stline.find_first_of("D");
            if(pos != std::string::npos && pos == 0)
            {
                filename = stline.substr(8);
                m_filelist << filename.c_str();
                m_Real_Display.insert({filename, stline});
                m_Display_Real.insert({stline, filename});
            }

            pos = stline.find_first_of("A");
            if(pos != std::string::npos && pos == 0)
            {
                filename = stline.substr(8);
                m_filelist << filename.c_str();
                m_Real_Display.insert({filename, stline});
                m_Display_Real.insert({stline, filename});
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
        QListWidgetItem* item = new QListWidgetItem( m_Real_Display.at(file.toStdString()).c_str(), ui->Filelistwidget);
        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
}

void MainWindow::run_diffcmd()
{

    // create diff cmd
    m_diff_file = new QFile("Diff_utility.bat");
    if (m_diff_file->open(QIODevice::WriteOnly))
    {
        QTextStream out(m_diff_file);
        out << "cd " << m_addr << "\n";
        out << "svn diff ";
        for(auto target : m_targetfilelist)
        {
            out << target << " ";
        }
        out << "> diff.patch";
    }
    m_diff_file->close();

    if (system("Diff_utility.bat"))
    {
        ui->LogText->appendPlainText(QStringLiteral("Create diff.patch Failed"));
    }
    else
    {
        ui->LogText->appendPlainText(QStringLiteral("Create diff.patch Successfully"));
        m_diff_file->remove();
    }
}

void MainWindow::on_Filelistwidget_itemChanged(QListWidgetItem *item)
{
    if (item->checkState() == Qt::Checked)
    {
        if (m_targetfilelist.empty())
        {
            m_targetfilelist.push_back(m_Display_Real.at(item->text().toStdString()).c_str());
            ui->LogText->appendPlainText(QStringLiteral("Add %1, Remain: %2").arg(item->text()).arg(m_targetfilelist.size()));
            return;
        }

        auto itr = m_targetfilelist.begin();
        while (itr != m_targetfilelist.end())
        {
            QString s = *itr;
            if(s == m_Display_Real.at(item->text().toStdString()).c_str())
            {
                return;
            }
            ++itr;
        }

        m_targetfilelist.push_back(m_Display_Real.at(item->text().toStdString()).c_str());
        ui->LogText->appendPlainText(QStringLiteral("Add %1, Remain: %2").arg(item->text()).arg(m_targetfilelist.size()));
    }
    else if (item->checkState() == Qt::Unchecked)
    {
        auto itr = m_targetfilelist.begin();
        while (itr != m_targetfilelist.end())
        {
            QString s = *itr;
            if(s == m_Display_Real.at(item->text().toStdString()).c_str())
            {
                m_targetfilelist.removeOne(*itr);
                ui->LogText->appendPlainText(QStringLiteral("Remove %1, Remain: %2").arg(item->text()).arg(m_targetfilelist.size()));
                return;
            }
            ++itr;
        }

    }
}

void MainWindow::on_pushButton_2_clicked()
{
    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Target File List Empty"));
        return;
    }

    run_diffcmd();
}


