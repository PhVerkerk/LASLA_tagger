#-------------------------------------------------
#
# Project created by QtCreator 2016-12-19T11:52:22
#
# Modifié le 19 septembre 2021 pour en faire une version console.
# Il n'y aura pas d'intervention de la part d'un philologue.
# J'envisage maintenant de mener de front une version avec GUI
# et cette version daemon en mettant à jour le moteur Collatinus.
# Jusqu'à présent, c'était la version de juin 2018 qui tournait ici.
# Donc quand même une version 11.
#
#-------------------------------------------------

QT      += network
QT      += core
QT	-= gui
#QT      += svg

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Tagueur_daemon_3
TEMPLATE = app
CONFIG   += console
CONFIG   -= app_bundle

OBJECTS_DIR= obj/
MOC_DIR = moc/
DESTDIR = ../Tagueur_IA

# DESTDIR      = ../bin
# OBJECTS_DIR= ../obj/
# MOC_DIR = ../moc/

SOURCES += ../src/main_daemon.cpp\
        ../src/serveur.cpp\
        ../src/fiche.cpp\
        ../src/mot.cpp\
#        src/traitement.cpp\
        ../src/ch.cpp \
        ../src/irregs.cpp \
        ../src/lemme.cpp \
        ../src/modele.cpp \
        ../src/lasla.cpp \
        ../src/lemCore.cpp

HEADERS  += ../src/serveur.h\
        ../src/fiche.h\
        ../src/mot.h\
        ../src/ch.h \
        ../src/irregs.h \
        ../src/lemme.h \
        ../src/modele.h \
        ../src/lasla.h \
        ../src/lemCore.h

#RESOURCES += LASLA_tagger.qrc

#FORMS    += src/mainwindow.ui

#    ICON = res/laslalogo.icns

deploy.commands = windeployqt ../Tagueur_IA/LASLA_daemon_3.exe
QMAKE_EXTRA_TARGETS += deploy

#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
#    data.path = LASLA_tagger_2.app/Contents/MacOS/data
#    data.files =  data/*
#    deploy.depends += install
#    INSTALLS += data
#    deploy.commands = macdeployqt LASLA_tagger_2.app
#    QMAKE_EXTRA_TARGETS += deploy
