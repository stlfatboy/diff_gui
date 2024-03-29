#-------------------------------------------------
#
# Project created by QtCreator 2018-08-10T23:47:06
#
#-------------------------------------------------

QT       += core gui autoupdatercore

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets sql

TARGET = Diff_GUI
TEMPLATE = app

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS \
        QT_MESSAGELOGCONTEXT

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        commitdialog.cpp \
        logging.cpp \
        main.cpp \
        mainwindow.cpp \
        progressdialog.cpp

HEADERS += \
        commitdialog.h \
        logging.h \
        mainwindow.h \
        progressdialog.h

FORMS += \
        ci_dialog.ui \
        mainwindow.ui \
        progressdialog.ui

RESOURCES += \
    res.qrc

CONFIG(debug, debug|release) {
    DEFINES += QMAKE_DEBUG
}

RC_ICONS = options.ico
