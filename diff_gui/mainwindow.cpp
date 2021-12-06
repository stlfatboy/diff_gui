#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "commitdialog.h"
#include "progressdialog.h"
#include "logging.h"

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
{
    ui->setupUi(this);
    ui->label_version->setText("2021.12.1");
    ui->label_help->setText("<a href = \"http://10.22.34.135/index.php/2021/07/30/374/\"> Open Help");
    ui->label_help->setOpenExternalLinks(true);
    ui->pushButton_gen_diff->setDisabled(true);
    ui->pushButton_svn_ci->setDisabled(true);
    ui->pushButton_svn_up->setDisabled(true);
    ui->pushButton_svn_re->setDisabled(true);
    ui->checkBox->setDisabled(true);
    ui->comboBox->addItem(DEFAULT_FOLDER_FILTER, 0);

    connect(ci_dialog, &CommitDialog::finished, this, &MainWindow::onCommitDialogFinished);
    connect(ci_dialog, &CommitDialog::log, [&](QString data){ui->LogText->appendPlainText(data);});

    m_updater = QtAutoUpdater::Updater::create("qtifw", {{"path", qApp->applicationDirPath() + "/maintenancetool"}}, this);
    connect(m_updater, &QtAutoUpdater::Updater::checkUpdatesDone, this, &MainWindow::hasUpdate);
#ifndef QMAKE_DEBUG
    m_updater->checkForUpdates();
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ci_dialog;
    delete progress_dialog;
}

void MainWindow::hasUpdate(QtAutoUpdater::Updater::State state)
{
    qDebug() << "Auto Update State: " << state;
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
    m_addr.replace('\\', '/');
    m_dir.setPath(m_addr);

    if (m_addr.length() == 0)
    {
        ui->LogText->appendPlainText(QStringLiteral("Path Empty!"));
    }
    else
    {
        ui->LogText->appendPlainText(QStringLiteral("Current Path: %1").arg(m_addr));
        qDebug() << QStringLiteral("Current Path: %1").arg(m_addr);
    }
}

void MainWindow::startupjobs(char *addr)
{
    qInfo() << m_addr;

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
        qDebug() << QStringLiteral("Reload File List");
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
        qDebug() << QStringLiteral("Find Repos");
        m_root_repo = m_dir.exists(".svn");
        if (!m_root_repo)
        {
            qDebug() << QStringLiteral("Find Repos recursively");
            findrepo(0);
        }
        else
        {
            ui->LogText->appendPlainText("Root Repo");
        }
        ui->comboBox->addItems(m_dirlist);
    }


    do{
        if (m_inner_refresh)
        {
            qDebug() << QStringLiteral("a inner refresh");
            m_inner_refresh = false;
        }

        // get diff status
        bool ret = true;
        QStringList args;
        args << "st";
        QByteArray result;
        if (m_root_repo)
        {
            qInfo() << QStringLiteral("svn_cli_execute for root repo");
            ret = svn_cli_execute(m_addr, args, &result);
            if (ret) loadfilelist(result);
            else
            {
                ui->LogText->appendPlainText("Get Change State Failed");
                qWarning() << QStringLiteral("Get Change State Failed");
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
                    qWarning() << QStringLiteral("Get Change State Failed at %1").arg(dir);
                    progress_dialog->reset();
                    return;
                }
            }
        }
    }while(m_inner_refresh);

    // show file list
    m_cur_fliter = DEFAULT_FOLDER_FILTER;
    showfilelist();

    progress_dialog->reset();

    if (!m_dirlist.empty() || m_root_repo) ui->pushButton_svn_up->setEnabled(true);

    if(m_targetfilelist.size() == 0)
    {
        ui->LogText->appendPlainText("No Available File(s)");
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

    QString target_dir;
    bool consist = true;
    for (auto & element : m_dirlist)
    {
        QByteArray temp_result;
        qDebug() << QStringLiteral("check revision consistency(for %1)").arg(element);
        if (svn_cli_execute(element, args, &temp_result))
        {
            const int temp_revision = temp_result.toInt();
            qDebug() << QStringLiteral("%1 at revision %2").arg(element).arg(temp_revision);
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
    bool exception_reject = false;
    if (m_inner_refresh)
    {
        qDebug() << QStringLiteral("inner refresh clean");
        m_filelist.clear();
        m_Real_Dir.clear();
        m_Real_Display.clear();
        m_Display_Real.clear();
    }

    int m = 0;
    int d = 0;
    int a = 0;
    int c = 0;
    int mi = 0;
    int u = 0;

    while(-1 != (j = data.indexOf('\n', j)))
    {
        QString line(data.left(j));
        line.remove('\n');
        line.remove('\r');
        line.replace('\\', '/');
        std::string stline = line.toStdString();
        std::string filename;
        QString prefix = m_root_repo ? m_addr : m_dirlist.at(workingdir);
        prefix += '/';

        // Modified
        std::size_t pos = stline.find_first_of("M");
        if (pos != std::string::npos && pos == 0)
        {
            m++;
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
            d++;
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
            a++;
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
            c++;
            if (!exception_reject)
            {
                QMessageBox msgBox;
                msgBox.setText(QString("Conflict Detected at repo: %1").arg(m_root_repo ? m_addr : m_dirlist[workingdir]));
                msgBox.setInformativeText("Do you want to go to repo and resolve it?");
                msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
                msgBox.setDefaultButton(QMessageBox::Yes);
                if (QMessageBox::Yes == msgBox.exec())
                {
                    svn_tortoise_execute(TortoiseSVNCMD::resolve, m_root_repo ? m_addr : m_dirlist[workingdir]);
                    m_inner_refresh = true;
                    return;
                }
                else
                {
                    exception_reject = true;
                }
            }
        }

        // Missing
        pos = stline.find_first_of("!");
        if (pos != std::string::npos && pos == 0)
        {
            mi++;
            if (!exception_reject)
            {
                QMessageBox msgBox;
                msgBox.setText(QString("Missing Detected at repo: %1").arg(m_root_repo ? m_addr : m_dirlist[workingdir]));
                msgBox.setInformativeText("Do you want to go to repo and check it?");
                msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
                msgBox.setDefaultButton(QMessageBox::Yes);
                if (QMessageBox::Yes == msgBox.exec())
                {
                    svn_tortoise_execute(TortoiseSVNCMD::repostatus, m_root_repo ? m_addr : m_dirlist[workingdir]);
                    m_inner_refresh = true;
                    return;
                }
                else
                {
                    exception_reject = true;
                }
            }
        }

        u++;

        data.remove(0, ++j);
        j = 0;
    }

    qDebug() << QStringLiteral("loaded modified(%1) delete(%2) add(%3) conflict(%4) missing(%5) unknown(%6)")
                    .arg(m).arg(d).arg(a).arg(c).arg(mi).arg(u);
}

void MainWindow::showfilelist()
{
    ui->Filelistwidget->clear();
    m_targetfilelist.clear();
    const bool show_all = m_cur_fliter == DEFAULT_FOLDER_FILTER;
    if (!show_all) qInfo() << "Using filter " + m_cur_fliter;

    for (const auto & file : m_filelist)
    {
        if (!show_all)
        {
            if (file.toStdString().find(m_cur_fliter.toStdString()) == std::string::npos)
            {
                continue;
            }
        }
        std::string str = m_Real_Display.at(file.toStdString());
        QListWidgetItem* item = new QListWidgetItem( str.c_str(), ui->Filelistwidget);
        // Store for Future Release
        m_ListItemVec.push_back(item);
        item->setCheckState(m_unchecked_files.contains(QString::fromStdString(str)) ? Qt::Unchecked : Qt::Checked);
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

    progress_dialog->setText("Generating Diff");
    progress_dialog->show();
    QApplication::processEvents();

    QMessageBox msgBox;
    int ret = -1;
    const QString diff_file = m_addr + "/diff.patch";
    const QString err_file = m_addr + "/diff_error";
    m_dir.remove(err_file);

    // 区分运行时是否在svn仓库
    // 如果不是svn仓库则需要先cd进svn目录后再执行Diff
    if (m_root_repo)
    {
        QStringList args;
        args << "diff" << "--force";
        unsigned len = 0;
        for(auto target : m_targetfilelist)
        {
            if (QDir(target).exists()) continue;
            args << target.remove(m_addr + '/');
            len += target.size();
        }
        if (len > 8192)
        {
            msgBox.setText(QString("Diff with Too Much args(%1)").arg(len));
            msgBox.exec();
        }
        else
        {
            QProcess p;
            p.setStandardOutputFile(diff_file);
            p.setStandardErrorFile(err_file);
            p.setWorkingDirectory(m_addr);
            p.start("svn", args);
            p.waitForFinished();
            ret = p.exitCode();
            if(ret != 0)
            {
                QFile err(err_file);
                err.open(QIODevice::Append);
                QTextStream err_stream(&err);
                for (auto & arg : args) err_stream << arg + '\n';
                err.close();
            }
        }
    }
    else
    {
        QStringList diff_files;
        std::map<int, QStringList> sorted_args;
        for(auto target : m_targetfilelist)
        {
            const int current_dir = m_Real_Dir.find(target.toStdString())->second;
            // 查找或创建
            auto arg_itr = sorted_args.find(current_dir);
            if (arg_itr == sorted_args.end())
            {
                QStringList temp;
                temp << "diff" << "--force";
                sorted_args.insert({current_dir, std::move(temp)});
            }

            if (QDir(target).exists()) continue;
            sorted_args.at(current_dir) << target.remove(m_dirlist.at(current_dir) + '/');
        }

        // 执行Diff
        int count = 0;
        for (auto & arg : sorted_args)
        {
            unsigned len = 0;
            for(auto & s : arg.second) len += s.size();
            if (len > 8192)
            {
                msgBox.setText(QString("Diff with Too Much args(%1) at %2").arg(len).arg(m_dirlist.at(arg.first)));
                msgBox.exec();
                break;
            }

            const QString temp_diff = m_addr + "/diff" + QString::fromStdString(std::to_string(arg.first)) + ".patch";
            diff_files << temp_diff;

            QProcess p;
            p.setStandardOutputFile(temp_diff);
            p.setStandardErrorFile(err_file);
            p.setWorkingDirectory(m_dirlist.at(arg.first));
            p.start("svn", arg.second);
            p.waitForFinished();
            ret = p.exitCode();
            if(ret != 0)
            {
                QFile err(err_file);
                err.open(QIODevice::Append);
                QTextStream err_stream(&err);
                err_stream << m_dirlist.at(arg.first) << '\n';
                for (auto & e : arg.second) err_stream << e + '\n';
                err.close();
                break;
            }

            progress_dialog->setValue(++count * 100 / sorted_args.size());
            QApplication::processEvents();
        }

        // Combine All Diffs
        if (ret == 0)
        {
            QFile diff(diff_file);
            diff.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream diff_combine(&diff);
            for(auto & element : diff_files)
            {
                QFile temp(element);
                if (!temp.open(QIODevice::ReadOnly)) continue;
                QTextStream tempstream(&temp);
                diff_combine << tempstream.readAll();
                temp.close();
                temp.remove();
            }
            diff.close();
        }
    }

    progress_dialog->reset();

    if (ret == 0)
    {
        msgBox.setText(QStringLiteral("Create diff.patch Successfully"));
        m_dir.remove(err_file);
    }
    else
    {
        msgBox.setText(QStringLiteral("Create diff.patch Failed"));
    }

    msgBox.exec();
}

void MainWindow::findrepo(int deepth)
{
    // 递归深度优先查找svn仓库
    if (deepth > 2)
    {
        qDebug() << QStringLiteral("reaching max deepth(%1)").arg(deepth);
        qDebug() << QStringLiteral("total repo(%1)").arg(m_dirlist.size());
        return;
    }

    auto entry_list = m_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (auto & element : entry_list)
    {
        const bool ret = m_dir.cd(element);
        qDebug() << QStringLiteral("%1(deepth %2)").arg(m_dir.path()).arg(deepth);
        if (m_dir.exists(".svn"))
        {
            m_dirlist.append(m_dir.path());
            qDebug() << QStringLiteral("hit(%1)").arg(m_dir.path());
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
        if (m_unchecked_files.contains(item->text())) m_unchecked_files.removeAll(item->text());
        //qDebug() << "itemChanged: " << m_unchecked_files;

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
        if (!m_unchecked_files.contains(item->text())) m_unchecked_files.append(item->text());
        //qDebug() << "itemChanged: " << m_unchecked_files;

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
    if (arg1 == Qt::Checked)
    {
        for (i = 0; i < total; i++)
        {
            ui->Filelistwidget->item(i)->setCheckState(Qt::Checked);
            if (m_unchecked_files.contains(ui->Filelistwidget->item(i)->text()))
            {
                m_unchecked_files.removeAll(ui->Filelistwidget->item(i)->text());
                // qDebug() << "state Checked: " << m_unchecked_files;
            }
        }
    }

    if (arg1 == Qt::Unchecked)
    {
        for (i = 0; i < total; i++)
        {
            ui->Filelistwidget->item(i)->setCheckState(Qt::Unchecked);
            if (!m_unchecked_files.contains(ui->Filelistwidget->item(i)->text()))
            {
                m_unchecked_files.append(ui->Filelistwidget->item(i)->text());
                // qDebug() << "state Unchecked: " << m_unchecked_files;
            }
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
    ci_dialog->addHistory(msg);


    progress_dialog->setText("Committing");
    progress_dialog->show();

    bool ret = false;
    QStringList args;
    QByteArray res_info;

    // 区分运行时是否在svn仓库
    // 如果不是svn仓库则需要先cd进svn目录后再执行ci
    do{
        if (m_root_repo)
        {
            args << "ci" << "-m" << msg;
            for(auto & target : m_targetfilelist)
            {
                args << target;
            }

            ret = svn_cli_execute(m_addr, args, &res_info);
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
                            break;
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
            ret = svn_cli_execute(m_dirlist.at(last_dir), args, &res_info);
            if (!ret)
            {
                ui->LogText->appendPlainText("Separate Commit Abort");
                break;
            }
        }
    }
    while(false);

    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::MarkdownText);
    res_info.replace('\n', "  \n");
    msgBox.setText((ret ? QStringLiteral("## Commit Finished\n") : QStringLiteral("## Commit Failed\n")) + res_info);
    msgBox.exec();

    qInfo() << "Refresh After Commit";
    startupjobs(nullptr);

    progress_dialog->reset();
}


void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    // 找到前两级目录并应用过滤给文件
    m_cur_fliter = arg1;
    auto pos1 = arg1.toStdString().rfind('\\');
    if (pos1 != std::string::npos)
    {
        auto pos0 = arg1.toStdString().rfind('\\', pos1);
        m_cur_fliter = QString::fromStdString(arg1.toStdString().substr(pos0 + 1, arg1.length() - pos0));
    }

    ui->LogText->appendPlainText("Select Folder " + m_cur_fliter);
    showfilelist();
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

    qInfo() << "Refresh After Revert";
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
    p.waitForReadyRead();

    if (result) *result = p.readAll() + p.readAllStandardError();
    else ui->LogText->appendPlainText(p.readAll() + p.readAllStandardError());

    return p.exitCode() == 0;
}


bool MainWindow::svn_tortoise_execute(TortoiseSVNCMD cmd, const QString &addr, int closeonend, bool detach)
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
    case TortoiseSVNCMD::showlog:
    {
        args << "/command:log";
        break;
    }
    default:
        break;
    }
    args << "/path:" + addr;
    args << "/closeonend:" + QString::number(closeonend);
    QString log = QString("(%1):tortoisesvn").arg(addr);
    for (auto & arg : args) log += " " + arg;
    ui->LogText->appendPlainText(log);
    if (detach)
    {
        p.startDetached("TortoiseProc.exe", args);
    }
    else
    {
        p.start("TortoiseProc.exe", args);
        p.waitForFinished();
    }


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

    progress_dialog->reset();

    QMessageBox msgBox;
    msgBox.setText(QString("Update to %1 Finished").arg(revision == 0 ? "Latest" : QString::number(revision)));
    msgBox.exec();
    m_target_revision = revision;
    qInfo() << "Refresh After Update";
    startupjobs(nullptr);
}


void MainWindow::on_pushButton_refresh_clicked()
{
    qInfo() << "Refresh on click";
    startupjobs(nullptr);
}


void MainWindow::on_pushButton_show_log_clicked()
{
    qInfo() << "Show Log for " + ui->comboBox->currentText();

    if (m_cur_fliter == DEFAULT_FOLDER_FILTER)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(QString("You Choose to Show All Logs, Will Open %1 Window(s)").arg(m_dirlist.count()));
        msgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Abort);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Abort)
        {
            return;
        }

        for (const auto & dir : m_dirlist)
        {
            svn_tortoise_execute(TortoiseSVNCMD::showlog, dir, 0, true);
        }

    }
    else
    {
        svn_tortoise_execute(TortoiseSVNCMD::showlog, m_cur_fliter);
    }
}
