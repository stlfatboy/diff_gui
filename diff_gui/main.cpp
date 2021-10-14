#include "mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QLoggingCategory>

#include "logging.h"

QScopedPointer<QFile> m_logFile;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Open stream file writes
    QTextStream out(m_logFile.data());
    // Write the date of recording
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
    // By type determine to what level belongs message
    switch (type)
    {
    case QtInfoMsg:     out << "INF "; break;
    case QtDebugMsg:    out << "DBG "; break;
    case QtWarningMsg:  out << "WRN "; break;
    case QtCriticalMsg: out << "CRT "; break;
    case QtFatalMsg:    out << "FTL "; break;
    }
    // Write to the output category of the message and the message itself
    out << QStringLiteral("%1[%2:%3 in %4]")
               .arg(msg)
               .arg(context.function)
               .arg(context.line)
               .arg(context.file) << endl;
    out.flush();    // Clear the buffered data
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    // Set the logging file
    // check which a path to file you use
    m_logFile.reset(new QFile(QDir::homePath() + "/Diff_Utils/log.txt"));
    // Open the file logging
    m_logFile.data()->open(QFile::Append | QFile::Text);
    // Set handler
    qInstallMessageHandler(messageHandler);

    qDebug() << "Init App...";

    MainWindow w;
    a.setWindowIcon(QIcon(":/icon/options.ico"));
    w.show();
    w.startupjobs(argv[1]);
    w.checkVersionConsistency();

    return a.exec();
}
