#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "commitdialog.h"

#include <QStringListModel>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ci_dialog(new CommitDialog(this))
    , m_root_repo(true)
    , m_diff_file(nullptr)
{
    ui->setupUi(this);
    ui->label->setText("Build." + QDate(2021,07,15).toString(Qt::ISODate));
    ui->pushButton_2->setDisabled(true);
    ui->checkBox->setDisabled(true);
    ui->pushButton->setDisabled(true);
    ui->comboBox->addItem("[All]", 0);
    connect(ci_dialog, &CommitDialog::finished, this, &MainWindow::onCommitDialogFinished);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ci_dialog;
    if (m_diff_file != nullptr)
    {
        m_diff_file->remove();
        delete m_diff_file;
    }

    QListWidgetItem* var = nullptr;
    foreach (var, m_ListItemVec)
    {
        delete var;
    }
}

void MainWindow::setaddr(char *addr)
{
    m_addr = addr;
    m_dir.setPath(addr);
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

    // Find Repos
    m_root_repo = m_dir.exists(".svn");
    if (!m_root_repo) findrepo(0);
    else ui->LogText->appendPlainText("Root Repo");
    ui->comboBox->addItems(m_dirlist);

    // get diff status
    QFile file("Diff_utility.bat");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        if (m_root_repo)
        {
            out << "svn st > filelist0";
            file.close();
            system("Diff_utility.bat");
            loadfilelist();
            file.remove();
        }
        else
        {
            int i = 0;
            for (auto & element : m_dirlist)
            {
                out << "cd " << element << "\n";
                out << "svn st > " << m_addr << "\\filelist" << QString::fromStdString(std::to_string(i++)) << '\n';
            }

            file.close();
            system("Diff_utility.bat");
            loadfilelist();
            file.remove();
        }
    }

    // show file list
    showfilelist("[All]");

    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("File List Empty"));
        return;
    }

    ui->pushButton->setEnabled(true);
    ui->pushButton_2->setEnabled(true);
    ui->checkBox->setEnabled(true);
    ui->checkBox->setChecked(true);
}

void MainWindow::loadfilelist()
{
    const int num = m_root_repo ? 0 : m_dirlist.count() - 1;
    //read list
    for (int i = 0; i <= num; i++)
    {
        QFile file(QString::fromStdString("filelist" + std::to_string(i)));
        //ui->LogText->appendPlainText("Load File" + file.fileName());
        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                std::string stline = line.toStdString();
                std::string filename;
                QString prefix = m_root_repo ? QDir::currentPath() : m_dirlist.at(i);
                prefix += '\\';
                // locate modified file
                std::size_t pos = stline.find_first_of("M");
                if (pos != std::string::npos && pos == 0)
                {
                    filename = prefix.toStdString() + stline.substr(8);
                    m_filelist << filename.c_str();
                    m_Real_Display.insert({filename, stline});
                    m_Display_Real.insert({stline, filename});
                    m_Real_Dir.insert({filename, i});
                }

                pos = stline.find_first_of("D");
                if(pos != std::string::npos && pos == 0)
                {
                    filename = prefix.toStdString() + stline.substr(8);
                    m_filelist << filename.c_str();
                    m_Real_Display.insert({filename, stline});
                    m_Display_Real.insert({stline, filename});
                    m_Real_Dir.insert({filename, i});
                }

                pos = stline.find_first_of("A");
                if(pos != std::string::npos && pos == 0)
                {
                    filename = prefix.toStdString() + stline.substr(8);
                    m_filelist << filename.c_str();
                    m_Real_Display.insert({filename, stline});
                    m_Display_Real.insert({stline, filename});
                    m_Real_Dir.insert({filename, i});
                }
            }
        }

        file.remove();
    }
}

void MainWindow::showfilelist(const QString & filter)
{
    ui->Filelistwidget->clear();
    m_targetfilelist.clear();
    const bool show_all = filter == "[All]";

    for (auto file : m_filelist)
    {
        if (!show_all)
        {
            if (file.toStdString().find(filter.toStdString()) == std::string::npos)
            {
                continue;
            }
        }
        std::string str = m_Real_Display.at(file.toStdString());
        QListWidgetItem* item = new QListWidgetItem( str.c_str(), ui->Filelistwidget);
        // Store for Future Release
        m_ListItemVec.push_back(item);

        item->setCheckState(Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        std::size_t pos = str.find_first_of("M");
        if (pos != std::string::npos && pos == 0)
        {
            continue;
        }
        item->setForeground(QBrush(Qt::GlobalColor::red));
    }
}

void MainWindow::run_diffcmd()
{
    int ret = 0;
    m_diff_file = new QFile("Diff_utility.bat");
    if (!m_diff_file->open(QIODevice::WriteOnly))
    {
        return;
    }

    if (m_root_repo)
    {
        QTextStream out(m_diff_file);
        out << "cd " << m_addr << "\n";
        out << "svn diff ";
        for(auto & target : m_targetfilelist)
        {
            out << target << " ";
        }
        out << "> diff.patch";

        m_diff_file->close();
        ret = system("Diff_utility.bat");
    }
    else
    {
        QTextStream out(m_diff_file);
        int last_dir = -1;
        QStringList diff_files;
        for(auto & target : m_targetfilelist)
        {
            int current_dir = m_Real_Dir.find(target.toStdString())->second;
            if (current_dir != last_dir)
            {
                if (last_dir != -1)
                {
                    QString file = "diff" + QString::fromStdString(std::to_string(last_dir)) + ".patch";
                    diff_files << file;
                    out << "> " << m_addr << '\\' << file << '\n';
                }
                last_dir = current_dir;
                out << "cd " << m_dirlist.at(current_dir) << "\n";
                out << "svn diff ";
            }

            out << target << " ";
        }
        QString file = "diff" + QString::fromStdString(std::to_string(last_dir)) + ".patch";
        diff_files << file;
        out << "> " << m_addr << '\\' << file;

        m_diff_file->close();
        ret = system("Diff_utility.bat");
        if (ret != 0)
        {
            ui->LogText->appendPlainText(QStringLiteral("Do Diff_utility.bat Failed"));\
            delete m_diff_file;
            return;
        }

        // Combine All Diffs
        QFile diff("diff.patch");
        diff.open(QIODevice::WriteOnly);
        QTextStream diff_combine(&diff);
        for(auto & element : diff_files)
        {
            QFile temp(element);
            temp.open(QIODevice::ReadOnly);
            QTextStream tempstream(&temp);
            diff_combine << tempstream.readAll();
            temp.close();
            temp.remove();
        }
        diff.close();
    }

    if (ret == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Create diff.patch Successfully"));
        m_diff_file->remove();
    }
    else
    {
        ui->LogText->appendPlainText(QStringLiteral("Create diff.patch Failed"));
    }
    delete m_diff_file;
}

void MainWindow::findrepo(int deepth)
{
    if (deepth > 2)
    {
        return;
    }

    auto entry_list = m_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (auto & element : entry_list)
    {
        const bool ret = m_dir.cd(element);
        //qDebug() << "deepth" << deepth << " " << m_dir.path();
        if (m_dir.exists(".svn"))
        {
            m_dirlist.append(m_dir.path());
        }
        else
        {
            findrepo(deepth + 1);
        }

        if (ret) m_dir.cdUp();
    }
}

void MainWindow::on_Filelistwidget_itemChanged(QListWidgetItem *item)
{
    if (item->checkState() == Qt::Checked)
    {
        if (m_targetfilelist.empty())
        {
            m_targetfilelist.push_back(m_Display_Real.at(item->text().toStdString()).c_str());
            ui->LogText->appendPlainText(QStringLiteral("Add    %1, Total: %2")
                                         .arg(item->text())
                                         .arg(m_targetfilelist.size()));
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
        ui->LogText->appendPlainText(QStringLiteral("Add    %1, Total: %2")
                                     .arg(item->text())
                                     .arg(m_targetfilelist.size()));
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
                ui->LogText->appendPlainText(QStringLiteral("Remove    %1, Remain: %2")
                                             .arg(item->text())
                                             .arg(m_targetfilelist.size()));
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

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    if (m_filelist.empty())
    {
        return;
    }

    int i = 0;
    int total = m_filelist.size();
    if (arg1 == 2)
    {
        for (i = 0; i < total; i++)
        {
            ui->Filelistwidget->item(i)->setCheckState(Qt::Checked);
        }
    }

    if (arg1 == 0)
    {
        for (i = 0; i < total; i++)
        {
            ui->Filelistwidget->item(i)->setCheckState(Qt::Unchecked);
        }
    }
}

void MainWindow::on_pushButton_clicked()
{
    ci_dialog->show();
}

void MainWindow::onCommitDialogFinished(int result)
{
    if (QDialog::Rejected == result)
    {
        ui->LogText->appendPlainText("User Canceled Commit");
        return;
    }

    auto msg = ci_dialog->getMessage();
    if (msg.isEmpty())
    {
        ui->LogText->appendPlainText("Empty Commit Message, Commit Canceled");
        return;
    }

    int ret = 0;
    auto commit_file = new QFile("Diff_Commit.bat");
    if (!commit_file->open(QIODevice::WriteOnly))
    {
        return;
    }

    if (m_root_repo)
    {
        QTextStream out(commit_file);
        out << "cd " << m_addr << "\n";
        out << "svn ci -m \"" << msg << "\" ";
        for(auto & target : m_targetfilelist)
        {
            out << target << " ";
        }

        commit_file->close();
    }
    else
    {
        QTextStream out(commit_file);
        int last_dir = -1;
        for(auto & target : m_targetfilelist)
        {
            int current_dir = m_Real_Dir.find(target.toStdString())->second;
            if (current_dir != last_dir)
            {
                last_dir = current_dir;
                out << '\n';
                out << "cd " << m_dirlist.at(current_dir) << "\n";
                out << "svn ci -m \"" << msg << "\" ";
            }

            out << target << " ";
        }
    }

    //ret = system("Diff_Commit.bat");
    if (ret == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Commit Successfully"));
        //m_diff_file->remove();
    }
    else
    {
        ui->LogText->appendPlainText(QStringLiteral("Commit Failed"));
    }

    delete commit_file;
}


void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    QString fliter = arg1;
    auto pos1 = arg1.toStdString().rfind('\\');
    if (pos1 != std::string::npos)
    {
        auto pos0 = arg1.toStdString().rfind('\\', pos1);
        fliter = QString::fromStdString(arg1.toStdString().substr(pos0 + 1, arg1.length() - pos0));
    }

    ui->LogText->appendPlainText("Combo Select " + fliter);
    for (auto & e : m_targetfilelist) ui->LogText->appendPlainText(e);
    showfilelist(fliter);
}
