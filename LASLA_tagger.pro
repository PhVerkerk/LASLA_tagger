#-------------------------------------------------
#
# Project created by QtCreator 2016-12-19T11:52:22
#
#-------------------------------------------------

QT      += network widgets
QT      += core gui
QT      += svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LASLA_tagger_2
TEMPLATE = app

OBJECTS_DIR= obj/
MOC_DIR = moc/

SOURCES += src/main.cpp\
        src/mainwindow.cpp\
        src/fiche.cpp\
        src/mot.cpp\
        src/traitement.cpp\
        src/ch.cpp \
        src/irregs.cpp \
        src/lemme.cpp \
        src/modele.cpp \
        src/lasla.cpp \
        src/lemCore.cpp

HEADERS  += src/mainwindow.h\
        src/fiche.h\
        src/mot.h\
        src/ch.h \
        src/irregs.h \
        src/lemme.h \
        src/modele.h \
        src/lasla.h \
        src/lemCore.h

RESOURCES += LASLA_tagger.qrc

#FORMS    += src/mainwindow.ui

    ICON = res/laslalogo.icns

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
    data.path = LASLA_tagger_2.app/Contents/MacOS/data
    data.files =  data/*
    deploy.depends += install
    INSTALLS += data
    deploy.commands = macdeployqt LASLA_tagger_2.app
    QMAKE_EXTRA_TARGETS += deploy
