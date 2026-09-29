QT += network widgets
QT += core
QT -= gui

TARGET = Client_C11
VERSION = "1.0"
#CONFIG += console
#CONFIG -= app_bundle
CONFIG += release_binary

TEMPLATE = app

SOURCES += ../src/client_main.cpp
OBJECTS_DIR= ../obj/
MOC_DIR = ../moc/
DESTDIR = ../bin

