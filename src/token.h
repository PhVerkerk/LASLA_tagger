#ifndef TOKEN_H
#define TOKEN_H

#include <QObject>
#include <QDebug>
#include <stdio.h>

class Token : public QObject
{
    Q_OBJECT
public:
    explicit Token(QString ligne, QObject *parent = nullptr);
//    ~Token();
    void ajoute(QString ligne);
    void setCodeSub(QString code);
    QString getRef();
    QString getForme();
    QString getLgTagueur();
    QString getChTagueur();
    QString getChIA();
    QString getCodeSub();
    QString getPossible(QString tag, QString indice);

private:
    QString _reference;
    QString _forme;
    QString _lgTagueur;
    QString _numero;
    QString _choixTagueur;
    QString _choixIA;
    QString _codeSub;
    QStringList _possibles;

signals:

public slots:
};

#endif // TOKEN_H
