#include "commitdialog.h"
#include "ui_ci_dialog.h"

CommitDialog::CommitDialog(QWidget * parent)
    : QDialog(parent)
    , ui(new Ui::CIDialog)
{
    ui->setupUi(this);
}


CommitDialog::~CommitDialog()
{
    delete ui;
}

QString CommitDialog::getMessage()
{
    return ui->plainTextEdit->toPlainText();
}
