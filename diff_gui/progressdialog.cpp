#include "progressdialog.h"
#include "ui_progressdialog.h"
#include "logging.h"


ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::setText(QString text)
{
    ui->label->setText(text);
}

void ProgressDialog::setValue(int val)
{
    ui->progressBar->setValue(val);
}

void ProgressDialog::reset()
{
    ui->progressBar->reset();
    this->close();
}
