#ifndef MOT_H
#define MOT_H

// #include <QtWidgets>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QList>
#include "fiche.h"

class Mot
{
public:
    Mot(QString ft, QString fl, QList<Fiche*> lf, int rg = -1, QString refs = "");
    ~Mot(); // Destructeur ?
    void ajoute (QString t);
    void setChoix (QString t, double sc); // Pour choisir un tag.
    void setChoix(int n); // Pour changer le choix à la main.
    int getChoix ();
    int getScore ();
    QString getRef ();
    void setRef(QString ref);
    QString getFTexte ();
    QString getFLem ();
    int cnt();
    QStringList tags();
    void videTags();
    QList<int> nbOcc();
    QString getTag (int n);
    int getNbr (int n);
    int getRang();
    int getOcc(int n);
    void designe(QString f, QString l, QString ind, QString c);
    QList<Fiche*> listeFiches ();
    void setFTexte(QString t);
    void setFLem(QString t);
    QString getInfo(int nFiche, bool complet = true);
    QString Lasla(QString ref);
    QString CSV(QString ref);
    void ajouteFiche(Fiche *f);
    void ajouteFiches(QList<Fiche*> lf);
    QString bulle(int n);
    QString formeFiche();
    QString static courte(QString f);

//    void verif(QStringList lMots);
    QStringList motsRequis();
    void suppr(QStringList motsAbsents);
    bool contient(QString m);

    // Pour accéder aux probas associées aux tags
    void setBest(QString t, double pr);
    double getBest(int n);
    double getBest(QString t);

    // Pour essayer de grouper auxiliaire et participe
    bool estAux();
    bool estPart();
    int existe(QString ligne);

    void setNotNormal();
    bool getNormal();
    void setApres(QString separateur);
    QString getApres();
    void setAItag(QString tag);
    void setAIind(QString ind);
    void setAIsub(QString sub);
    QString getAIresults();
    QString getPossible();

    void setFcsv(QString forme);
    QString getFcsv();
    void setAlerte(QString w);
    QString getAlerte();

private:
    QString _fTexte;
    QString _fLem;
    QString _fCSV; // La forme que j'écris dans le CSV pour l'IA
    QString _refs;
    int _rang;
    QList<Fiche*> _lFiches;
    QStringList _lTags;
    QList<int> _lNbr;
    QList<double> _lBestProbas;
    QString _tagChoisi;
    double _score;
    int _choix;

    QString _apres;
    bool _normal;

    // Pour ajouter les prédictions de l'IA.
    QString _tagAI;
    QString _indAI;
    QString _subAI;
    QString _alerte;

    static const QString nombre;
};

#endif // MOT_H
