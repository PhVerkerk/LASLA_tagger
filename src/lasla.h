/*               lasla.h
 *
 *  This file is part of COLLATINUS.
 *
 *  COLLATINUS is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  COLLATINVS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with COLLATINUS; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * © Yves Ouvrard, 2009 - 2016
 */

#ifndef LASLA_H
#define LASLA_H

#include <QDebug>
#include <QRegExp>

#include "fiche.h"
#include "mot.h"
#include "lemCore.h"
#include "lemme.h"
#include "ch.h"

/**
 * @brief La classe Lasla introduisait des fonctions
 * pour la première version du tagueur LASLA-Tagger.
 *
 * Devenue inutile dans Collatinus depuis que
 * la deuxième version de ce tagueur l'intègre.
 * Je la recycle donc complètement pour le tagueur en daemon.
 * Par suite, aussi pour la version avec GUI.
 */
class Lasla : public QObject
{
//    Q_OBJECT

public:
    Lasla(QObject *parent = 0, LemCore *l=0, QString resDir="");
    // Créateur de la classe
    QString k9(QString m);
    // Code en 9 pour le LASLA
    void graphMed(bool activees);
    bool optionMed(); // Retourne la valeur de _medieval
    QString transfMed(QString f, bool rad=false);
    // Fonctions relais.
    void setDate(QString date);

    QString traite(QString requete); // Traite la requête et appelle nouveau()
    // Cette routine est appelée par le Serveur.

    void ajoute(QString lg, int numMot = -1);
    /*
     * Les formes inconnues sont traitées dans MainWindow
     * avec comme résultat une nouvelle fiche LASLA ou
     * un nouveau lemme pour Collatinus.
     * Les lignes correspondantes sont sauvées dans le dico perso
     * ou à la fin de lem_ext.la et lem_ext.fr.
     * Mais pour que ce soit immédiatement utilisable,
     * il faut les charger (sinon, il faudra attendre le prochain démarrage).
     * */

    QString CSV2APN(QString nom);
    QString _warnings;
    // Pour intégrer ce que faisait le programme de réconciliation.
    // Il faut une variable supplémentaire pour les warnings
    // que j'envoyais en qDebug().

    // Pour le fichier APN
    QString _refOeuvre;
    QString _nomFichier;
//    QString _repertoire;
    bool _CR1;
    bool _CR2;
    bool _CR3;
    bool _CR4;
    int _decalPhr; // Pour donner un numéro autre que 1 à la 1ère phrase.
    int numLg;
    int numPar;
    int numCh;
    int numLvr;
    QString formatRef;

    // Le texte
    QString _texte; // Le texte chargé.
    QStringList _elements; // Le texte découpé : les éléments pairs sont les séparateurs
    QList<Mot*> _mots; // Les mots du texte analysés.
    QList<int> _finsPhrase; // Le rang du dernier mot de la phrase dans _mots.
    QList<int> _fPhrEl; // Le rang du dernier mot de la phrase dans _elements.
    QList<int> _pointsFixes; // Les indices des mots qui sont des points fixes (un seul bitag possible).
    bool _prose;
// Je rends ces variables publiques pour que Mainwindow
// puisse y avoir accès.
    QString nouveau();
    // Pour la phase de préprocessing.
    void setPreProc(bool val);
    QStringList inconnus();
    QStringList listeModeles(QString forme = "");
    bool sauver(QString nomFichier);
    bool ouvrir(QString nomFichier);
    void separEncl(int numMot, int numPhrase);
    void decimer(int i, int debPhr, int finPhr);
    void fusionPhrase(int numPhrase);
    void changerRef(int numMot, int numPhrase, QString m);
    bool ajouterMot(QString m, int nm, int numPhrase, QString r, QString ligne = "");
    QString tag(QString code9);

private:
    /*! Un pointeur vers le noyau de lemmatisation qui peut être partagé. */
    LemCore * _lemCore;
    /*! Le nom du répertoire contenant les données. */
    QString _resDir;
    QString dicoPerso;
    QString dicoAuto;
    QMap<QString,QString> _catLasla;
    /*!< QMap avec les correspondances entre les modèles de Collatinus et les catégories du LASLA */
    void lisCat();
    // Pour la lecture du fichier.
    QString _date;
    bool _primoL;
    bool _primoA;
//    bool estAux();
//    bool estPart();
    // Fonctions déplacées dans Mot.
    void vontEnsemble(int numAux, int numPart);
    bool estSemidep(QString lem);
    void ficheAmbigue(QString str, Fiche *fiche);
    void choisir(int iMot, QString rep);

    /*
     * Le commentaire qui suit était dans le module serveur de
     * la première version du tagueur-daemon.
     * Je l'ai recopié quand j'ai décidé de basculer cette partie
     * dans le module Lasla qui était très maigre.
     * Je n'ai donc pas créé de module supplémentaire,
     * simplement transféré des choses de serveur vers lasla.
     *
     * En réalité, tout ce qui suit n'a aucun rapport avec le serveur lui-même.
     * Il s'agit du traitement des requêtes pour l'application envisagée, ici le Tagueur.
     * Toutefois, il ne me semble pas nécessaire de faire un deuxième conteneur
     * qui serait imbriqué dans celui-ci.
     *
     * Je vais tâcher de mettre toutes les variables et fonctions dédiées
     * en dessous de ce commentaire. Dans le fichier serveur.cpp
     * je grouperai au début tout ce qui est vraiment lié au serveur,
     * laissant ensuite libre cours à la créativité.
     *
     * Je ne pense pas avoir besoin de variables ou fonctions publiques,
     * ni d'autres slot que ceux du serveur.
     */

        // initialisation
        void lireDonnees();
        QList<Fiche*> getAnalyses(QString forme);

        // Une regExp utile ?
        QRegExp const chiffres = QRegExp("[0-9]");

        // Création des mots correspondant aux enclitiques
        // Il est inutile d'aller chercher les enclitiques à chaque fois qu'ils sortent...
        Mot * motQue;
        Mot * motVe;
        Mot * motNe;
        Mot * motCum;

        // Variables contenant les données
        QMultiMap<QString,Fiche*> _fiches;
        QMultiMap<QString,Fiche*> _fComplx;
        QMap<QString,qint64> _trigrammes;
        QMap<QString,qint64> _bigrammes;
        QMap<QString,qint64> _monogrammes;
        QMap<QString,QString> _corresp;

        /*! Booléen pour traiter les graphies médiévales */
        bool _medieval;
        /*! Associe une graphie médiévale (clef) à une graphie classique (valeur) pour une fiche */
        QMultiMap<QString, QString> _ficMed;
        /*! Une fonction pour ajouter les fiches médiévales de la forme f */
        void ajoutsMed(QString f);

        QList<Fiche*> _analyses; // Variable globale avec les analyses du mot en cours.
        void unknown(QString m);
        // Une fonction pour insérer dans _analyses les analyses attribuées à une forme inconnue.
        QStringList _inconnus; // Variable globale pour stoker les formes inconnues
        bool _preProc; // Booléen pour dire que l'on fait du préprocessing.
        // Ce booléen sera faux dans la version daemon, mais activé par la GUI.

        bool fichesEgales(Fiche* f, Fiche* fiche);
        // Un booléen pour me dire si les deux fiches décrivent la même chose.
        void ajouterFiche(Fiche* fiche, bool manuel = false);
        // Ajouter une fiche à _fiches en vérifiant qu'elle n'y figure pas encore.
        Fiche *creerFiche(QString ligne);
        // Créer la fiche seulement si elle n'existe pas encore.
        void ecrireDico(QString ligne, bool manuel = false);

        // Fonctions appelées par nouveau.
        void grec();
        bool estRomain(QString r);
        QList<Fiche*> appelCollatinus(QString m, bool *enclit);
        void sauveCSV(QString nomFichier);
        void taggage(); // Je redéfinis cette fonction en partant de ce que je fais dans Collatinus.
        void taggage_old(); // Je garde l'ancienne version de cette fonction.
        bool static plusFreq(Fiche *f1, Fiche *f2);
        double proba(QString seqTags, qint64 nbOcc);

        QString codes5a9(QString code5); // Pour convertir un "code en 5" en "code en 9"
        QString nettoie(QString forme);

};

#endif // LASLA_H
