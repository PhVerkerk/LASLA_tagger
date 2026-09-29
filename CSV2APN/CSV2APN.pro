#-------------------------------------------------
#
# Project created by QtCreator 2015-11-30T07:59:02
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = CSV2APN
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DESTDIR= ../Tagueur_IA
OBJECTS_DIR= ../obj/
MOC_DIR = ../moc/


SOURCES += ../src/main_CSV2APN.cpp \
    ../src/token.cpp

macx{
    TARGET = CSV2APN
    #note mac os x, fair un $ qmake -spec macx-g++
    #CONFIG += x86 ppc
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
#    deploy.commands = macdeployqt LASLA_txt.app
    QMAKE_EXTRA_TARGETS += deploy
}

HEADERS += \
    ../src/token.h
