#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "commitdialog.h"
#include "progressdialog.h"

#include <QMessageBox>
#include <QStringListModel>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QInputDialog>
#include <QDebug>

#include <QtAutoUpdaterCore/Updater>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ci_dialog(new CommitDialog(this))
    , progress_dialog(new ProgressDialog(this))
    , m_updater(nullptr)
    , m_root_repo(true)
    , m_target_revision(0)
    , m_diff_file(nullptr)
{
    ui->setupUi(this);
    ui->label_version->setText("2021.8.2");
    ui->label_help->setText("<a href = \"http://10.22.34.133/index.php/2021/07/30/374/\"> Open Help");
    ui->label_help->setOpenExternalLinks(true);
    ui->pushButton_gen_diff->setDisabled(true);
    ui->pushButton_svn_ci->setDisabled(true);
    ui->pushButton_svn_up->setDisabled(true);
    ui->pushButton_svn_re->setDisabled(true);
    ui->checkBox->setDisabled(true);
    ui->comboBox->addItem("[All]", 0);

    connect(ci_dialog, &CommitDialog::finished, this, &MainWindow::onCommitDialogFinished);
    connect(this, &MainWindow::inner_startupjobs, this, &MainWindow::startupjobs, Qt::QueuedConnection);

    m_updater = QtAutoUpdater::Updater::create("qtifw", {{"path", qApp->applicationDirPath() + "/maintenancetool"}}, this);
    connect(m_updater, &QtAutoUpdater::Updater::checkUpdatesDone, this, &MainWindow::hasUpdate);
    //m_updater->checkForUpdates();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ci_dialog;
    delete progress_dialog;
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

void MainWindow::hasUpdate(QtAutoUpdater::Updater::State state)
{
    qDebug() << "Update State " << state;
    if (state == QtAutoUpdater::Updater::State::NewUpdates)
    {
        QMessageBox mb;
        mb.setText("New Version Available! Will Update on Exit");
        mb.exec();
        m_updater->runUpdater(QtAutoUpdater::Updater::InstallModeFlag::OnExit);
        return;
    }
}

void MainWindow::setaddr(char *addr)
{
    m_addr = addr;
    m_dir.setPath(addr);

    if (m_addr.length() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Path Empty!"));
    }
    else
    {
        ui->LogText->appendPlainText(QStringLiteral("Current Path: %1").arg(m_addr));
    }
}

void MainWindow::startupjobs(char *addr)
{
    if (addr)
    {
        setaddr(addr);
    }
    else
    {
        m_filelist.clear();
        m_Real_Dir.clear();
        m_Real_Display.clear();
        m_Display_Real.clear();
        ui->LogText->appendPlainText("Reload File List");
    }

    if(m_addr.length() == 0)
    {
        ui->LogText->appendPlainText("Current Working Directory Incorrect");
        return;
    }
    progress_dialog->setText("Loading Changes");
    progress_dialog->show();

    // Find Repos, Skip when refresh
    if (addr)
    {
        m_root_repo = m_dir.exists(".svn");
        if (!m_root_repo) findrepo(0);
        else ui->LogText->appendPlainText("Root Repo");
        ui->comboBox->addItems(m_dirlist);
    }


    // get diff status
    bool ret = true;
    QStringList args;
    args << "st";
    QByteArray result;
    if (m_root_repo)
    {
        ret = svn_cli_execute(m_addr, args, &result);
        if (ret) loadfilelist(result);
        else
        {
            ui->LogText->appendPlainText("Get Change State Failed");
            progress_dialog->reset();
            return;
        }
    }
    else
    {
        int i = 0;
        for (auto & dir : m_dirlist)
        {
            progress_dialog->setValue(i * 100 / m_dirlist.size());
            QCoreApplication::processEvents();
            ret = svn_cli_execute(dir, args, &result);
            if (ret) loadfilelist(result, i++);
            else
            {
                ui->LogText->appendPlainText("Get Change State Failed at" + dir);
                progress_dialog->reset();
                return;
            }
        }
    }

    // show file list
    showfilelist("[All]", addr ? false : true);

    progress_dialog->reset();

    if (!m_dirlist.empty() || m_root_repo) ui->pushButton_svn_up->setEnabled(true);

    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText("File List Empty");
        return;
    }

    ui->pushButton_gen_diff->setEnabled(true);
    ui->pushButton_svn_ci->setEnabled(true);
    ui->pushButton_svn_re->setEnabled(true);
    ui->checkBox->setEnabled(true);
    ui->checkBox->setChecked(true);
}


void MainWindow::checkVersionConsistency()
{
    if (m_dirlist.empty() || m_root_repo) return;

    QStringList args;
    args << "info" << "--show-item" << "revision";
    QStringList revision;

    QString target_dir;
    bool consist = true;
    for (auto & element : m_dirlist)
    {
        QByteArray temp_result;
        if (svn_cli_execute(element, args, &temp_result))
        {
            const int temp_revision = temp_result.toInt();
            if (m_target_revision != 0 && temp_revision != m_target_revision) consist = false;
            if (temp_revision > m_target_revision)
            {
                target_dir = element;
                m_target_revision = temp_revision;
            }
        }
    }

    if (!consist)
    {
        QMessageBox msgBox;
        msgBox.setText("Your sub-directories' revision are not the same.");
        msgBox.setInformativeText(QString("Do you want to update them to %1(sync to %2)?").arg(m_target_revision).arg(target_dir));
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (QMessageBox::Yes == msgBox.exec())
        {
            update_repo(m_target_revision);
        }
    }
}


void MainWindow::loadfilelist(QByteArray & data, int workingdir)
{
    int j = 0;
    while(-1 != (j = data.indexOf('\n', j)))
    {
        QString line(data.left(j));
        line.remove('\n');
        line.remove('\r');
        std::string stline = line.toStdString();
        std::string filename;
        QString prefix = m_root_repo ? QDir::currentPath() : m_dirlist.at(workingdir);
        prefix += '\\';

        // Modified
        std::size_t pos = stline.find_first_of("M");
        if (pos != std::string::npos && pos == 0)
        {
            filename = prefix.toStdString() + stline.substr(8);
            m_filelist << filename.c_str();
            m_Real_Display.insert({filename, stline});
            m_Display_Real.insert({stline, filename});
            m_Real_Dir.insert({filename, workingdir});
        }

        // Delete
        pos = stline.find_first_of("D");
        if(pos != std::string::npos && pos == 0)
        {
            filename = prefix.toStdString() + stline.substr(8);
            m_filelist << filename.c_str();
            m_Real_Display.insert({filename, stline});
            m_Display_Real.insert({stline, filename});
            m_Real_Dir.insert({filename, workingdir});
        }

        // Added
        pos = stline.find_first_of("A");
        if(pos != std::string::npos && pos == 0)
        {
            filename = prefix.toStdString() + stline.substr(8);
            m_filelist << filename.c_str();
            m_Real_Display.insert({filename, stline});
            m_Display_Real.insert({stline, filename});
            m_Real_Dir.insert({filename, workingdir});
        }

        // Conflict
        pos = stline.find_first_of("C");
        if (pos != std::string::npos && pos == 0)
        {
            QMessageBox msgBox;
            msgBox.setText(QString("Conflict Detected at repo: %1").arg(m_dirlist[workingdir]));
            msgBox.setInformativeText("Do you want to go to repo and resolve it?");
            msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
            msgBox.setDefaultButton(QMessageBox::Yes);
            if (QMessageBox::Yes == msgBox.exec())
            {
                svn_tortoise_execute(TortoiseSVNCMD::resolve, m_dirlist[workingdir]);
                emit inner_startupjobs(nullptr);
                return;
            }
        }

        // Missing
        pos = stline.find_first_of("!");
        if (pos != std::string::npos && pos == 0)
        {
            QMessageBox msgBox;
            msgBox.setText(QString("Missing Detected at repo: %1").arg(m_dirlist[workingdir]));
            msgBox.setInformativeText("Do you want to go to repo and check it?");
            msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
            msgBox.setDefaultButton(QMessageBox::Yes);
            if (QMessageBox::Yes == msgBox.exec())
            {
                svn_tortoise_execute(TortoiseSVNCMD::repostatus, m_dirlist[workingdir]);
                emit inner_startupjobs(nullptr);
                return;
            }
        }

        data.remove(0, ++j);
        j = 0;
    }
}

void MainWindow::showfilelist(const QString & filter, bool isRefresh)
{
    ui->Filelistwidget->clear();
    m_targetfilelist.clear();
    const bool show_all = filter == "[All]";

    for (const auto & file : m_filelist)
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

        item->setCheckState(isRefresh ? Qt::Unchecked : Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        std::size_t pos = str.find_first_of("M");
        if (pos != std::string::npos && pos == 0)
        {
            continue;
        }
        item->setForeground(QBrush(Qt::GlobalColor::red));
    }
}

void MainWindow::on_pushButton_gen_diff_clicked()
{
    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Target File List Empty"));
        return;
    }

    int ret = 0;
    m_diff_file = new QFile("Diff_utility.bat");
    if (!m_diff_file->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }

    // 区分运行时是否在svn仓库
    // 如果不是svn仓库则需要先cd进svn目录后再执行Diff
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
        diff.open(QIODevice::WriteOnly | QIODevice::Text);
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
    // 递归深度优先查找svn仓库
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


void MainWindow::onCommitDialogFinished(int result)
{
    if (QDialog::Rejected == result)
    {
        ui->LogText->appendPlainText("User Canceled Commit");
        return;
    }

    if (m_targetfilelist.empty())
    {
        ui->LogText->appendPlainText("No File Selected, Commit Canceled");
        return;
    }

    auto msg = ci_dialog->getMessage();
    if (msg.isEmpty())
    {
        ui->LogText->appendPlainText("Empty Commit Message, Commit Canceled");
        return;
    }
    msg.replace('\n', '\r');

    progress_dialog->setText("Committing");
    progress_dialog->show();

    bool ret = true;
    QStringList args;

    // 区分运行时是否在svn仓库
    // 如果不是svn仓库则需要先cd进svn目录后再执行ci
    if (m_root_repo)
    {
        args << "ci" << "-m" << msg;
        for(auto & target : m_targetfilelist)
        {
            args << target;
        }

        ret = svn_cli_execute(m_addr, args);
    }
    else
    {
        int count = 0;
        int last_dir = -1;
        for(auto & target : m_targetfilelist)
        {
            int current_dir = m_Real_Dir.find(target.toStdString())->second;
            if (current_dir != last_dir)
            {
                // Run if dir changed
                if (last_dir != -1)
                {
                    progress_dialog->setValue(count * 100 / m_targetfilelist.size());
                    QCoreApplication::processEvents();
                    ret = svn_cli_execute(m_dirlist.at(last_dir), args);
                    if (!ret)
                    {
                        ui->LogText->appendPlainText("Separate Commit Abort");
                        return;
                    }
                    args.clear();
                }

                last_dir = current_dir;
                args << "ci" << "-m" << msg;
            }

            args << target;
            count++;
        }

        progress_dialog->setValue(count * 100 / m_dirlist.size());
        QCoreApplication::processEvents();
        ret = svn_cli_execute(m_dirlist.at(last_dir), args);
        if (!ret)
        {
            ui->LogText->appendPlainText("Separate Commit Abort");
            return;
        }
    }

    if (ret) ui->LogText->appendPlainText(QStringLiteral("Commit Finished"));
    startupjobs(nullptr);

    progress_dialog->reset();
}


void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    // 找到前两级目录并应用过滤给文件
    QString fliter = arg1;
    auto pos1 = arg1.toStdString().rfind('\\');
    if (pos1 != std::string::npos)
    {
        auto pos0 = arg1.toStdString().rfind('\\', pos1);
        fliter = QString::fromStdString(arg1.toStdString().substr(pos0 + 1, arg1.length() - pos0));
    }

    ui->LogText->appendPlainText("Select Folder " + fliter);
    showfilelist(fliter);
}


void MainWindow::on_pushButton_svn_ci_clicked()
{
    ci_dialog->show();
}


void MainWindow::on_Filelistwidget_itemDoubleClicked(QListWidgetItem *item)
{
    const auto & filename = m_Display_Real.at(item->text().toStdString());

    QProcess p;
    const QString program = "TortoiseProc.exe";
    QStringList args;
    args << "/command:diff" << "/path:" + QString::fromStdString(filename);
    p.startDetached(program, args);
}


void MainWindow::on_pushButton_svn_up_clicked()
{
    bool ok;
    const int ver = QInputDialog::getInt(this, "SVN Update to", "Version:", m_target_revision, 0 , 2147483647, 1, &ok);
    if (!ok)
    {
        return;
    }

    update_repo(ver);
}


void MainWindow::on_pushButton_svn_re_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("SVN Revert");
    msgBox.setInformativeText("Do you want to revert these files?");
    msgBox.setStandardButtons(QMessageBox::Apply | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if (QMessageBox::Cancel == ret)
    {
        return;
    }

    progress_dialog->setText("Reverting");
    progress_dialog->show();

    bool result = true;
    QStringList args;
    // 区分运行时是否在svn仓库
    // 如果不是svn仓库则需要先cd进svn目录后再执行revert
    if (m_root_repo)
    {
        args << "revert";
        for(auto & target : m_targetfilelist)
        {
            args << target;
        }
        result = svn_cli_execute(m_addr, args);
    }
    else
    {
        int count = 0;
        int last_dir = -1;
        for(auto & target : m_targetfilelist)
        {
            int current_dir = m_Real_Dir.find(target.toStdString())->second;
            if (current_dir != last_dir)
            {
                if (last_dir != -1)
                {
                    progress_dialog->setValue(count *100 / m_targetfilelist.size());
                    QCoreApplication::processEvents();
                    result = svn_cli_execute(m_dirlist.at(last_dir), args);
                    args.clear();
                }
                last_dir = current_dir;
                args << "revert";
            }

            args << target;
            count++;
        }

        progress_dialog->setValue(count *100 / m_targetfilelist.size());
        QCoreApplication::processEvents();
        result = svn_cli_execute(m_dirlist.at(last_dir), args);
    }

    if (result)
    {
        ui->LogText->appendPlainText("Revert Successfully");
    }
    else
    {
        ui->LogText->appendPlainText("Revert Failed");
    }

    startupjobs(nullptr);

    progress_dialog->reset();
}


bool MainWindow::svn_cli_execute(const QString &addr, const QStringList &args, QByteArray *result)
{
    QProcess p;
    p.setWorkingDirectory(addr);
    QString log = QString("(%1):svn").arg(addr);
    for (auto & arg : args) log += " " + arg;
    ui->LogText->appendPlainText(log);
    p.start("svn", args);
    p.waitForFinished();

    if (result) *result = p.readAll();
    else ui->LogText->appendPlainText(p.readAll());

    return p.exitCode() == 0;
}


bool MainWindow::svn_tortoise_execute(TortoiseSVNCMD cmd, const QString &addr, int closeonend)
{
    QProcess p;
    QStringList args;
    switch (cmd)
    {
    case TortoiseSVNCMD::resolve:
    {
        args << "/command:resolve";
        break;
    }
    case TortoiseSVNCMD::repostatus:
    {
        args << "/command:repostatus";
        break;
    }
    defualt:
    {
        break;
    }
    }
    args << "/path:" + addr;
    args << "/closeonend:" + QString::number(closeonend);
    QString log = QString("(%1):tortoisesvn").arg(addr);
    for (auto & arg : args) log += " " + arg;
    ui->LogText->appendPlainText(log);
    p.start("TortoiseProc.exe", args);
    p.waitForFinished();

    return p.exitCode() == 0;
}


void MainWindow::update_repo(int revision)
{
    progress_dialog->setText("Updating");
    progress_dialog->show();

    bool ret = true;
    QStringList args;
    // 区分运行时是否在svn仓库
    // 如果不是svn仓库则需要先cd进svn目录后再执行up
    if (m_root_repo)
    {
        args << "up";
        if (revision != 0)
        {
            args << "-r" << QString::number(revision);
        }

        ret = svn_cli_execute(m_addr, args);
    }
    else
    {
        int count = 0;
        for(auto & target : m_dirlist)
        {
            args << "up";
            if (revision != 0)
            {
                args << "-r" << QString::number(revision);
            }

            progress_dialog->setValue(++count * 100 / m_dirlist.size());
            QCoreApplication::processEvents();
            ret = svn_cli_execute(target, args);
            args.clear();
        }
    }

    ui->LogText->appendPlainText(QString("Update to %1 Finished").arg(revision == 0 ? "Latest" : QString::number(revision)));
    startupjobs(nullptr);

    progress_dialog->reset();
}


void MainWindow::on_pushButton_refresh_clicked()
{
    startupjobs(nullptr);
}

