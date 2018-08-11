#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStringListModel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // load file list
    loadfilelist();

    auto model = new QStringListModel(m_filelist);
    ui->Filelistview->setModel(model);

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

void MainWindow::on_Filelistview_doubleClicked(const QModelIndex &index)
{
    const QString filename = index.data().toString();
    ui->Filechoiceview->appendPlainText(QStringLiteral("Add '%1'").arg(filename));
}
