#-------------------------------------------------
#
# Project created by QtCreator 2016-12-19T11:52:22
#
# Mis à jour le 11 octobre 2022, pour avoir un développement
# unifié d'une version avec GUI et une autre en daemon.
# Le moteur passe de Collatinus 11 à 11.3.
#
#-------------------------------------------------

QT      += network widgets
QT      += core gui
QT      += svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LASLA_tagger_3
TEMPLATE = app

OBJECTS_DIR= obj/
MOC_DIR = moc/
DESTDIR = ../Tagueur_IA

SOURCES += ../src/main.cpp\
        ../src/mainwindow.cpp\
        ../src/serveur.cpp\
        ../src/fiche.cpp\
        ../src/mot.cpp\
        ../src/traitement.cpp\
        ../src/ch.cpp \
        ../src/irregs.cpp \
        ../src/lemme.cpp \
        ../src/modele.cpp \
        ../src/lasla.cpp \
        ../src/lemCore.cpp
# Pour l'instant, je garde le module "serveur"
# même si je ne vais pas l'utiliser. 28/08/2024

HEADERS  += ../src/mainwindow.h\
        ../src/serveur.h\
        ../src/fiche.h\
        ../src/mot.h\
        ../src/ch.h \
        ../src/irregs.h \
        ../src/lemme.h \
        ../src/modele.h \
        ../src/lasla.h \
        ../src/lemCore.h

RESOURCES += LASLA_tagger.qrc

#FORMS    += src/mainwindow.ui

    RC_ICONS = res/laslalogo.ico

deploy.commands = windeployqt ../Tagueur_IA/LASLA_tagger_3.exe
QMAKE_EXTRA_TARGETS += deploy

#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
#    data.path = LASLA_tagger_3.app/Contents/MacOS/data
#    data.files =  ../bin/data/*
#    deploy.depends += install
#    INSTALLS += data
#    deploy.commands = macdeployqt LASLA_tagger_3.app
#    QMAKE_EXTRA_TARGETS += deploy
