#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QtWidgets>
#include <QtNetwork>
#include <QRegExp>
#include <QProgressDialog>

#include "fiche.h"
#include "mot.h"
#include "lasla.h"
#include "lemCore.h"

namespace Ui {
class MainWindow;
}

class MainWindow;

class EditLatin : public QTextEdit
{
    Q_OBJECT

   private:
    MainWindow *mainwindow;
    int _ident;

   protected:
    void mouseReleaseEvent(QMouseEvent *e);

   public:
    EditLatin(QWidget *parent, int id);
    bool event(QEvent *event);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void choisir(int n, int id); // Pour choisir la bonne lemmatisation pour le mot n
    QString bulle(int n, int id);

private:
    QAction *quitAct;
    QAction *actionNouveau;
    QAction *actionOuvrir;
    QAction *actionSauver;
    QAction *actionA_propos;
    QAction *actionSuivante;
    QAction *actionPrecedente;
    QAction *actionDebut;
    QAction *actionFin;
    QAction *actionFusion;
    QAction *actionCouleurs;
    QAction *deZoomAct;
    QAction *zoomAct;
    QAction *findAct;
    QAction *reFindAct;
    QWidget *centralWidget;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
//    QTextBrowser *textBrowser;
    QMenuBar *menuBar;
    QMenu *menuFichier;
    QMenu *menuPhrase;
    QMenu *menuAide;
    QToolBar *mainToolBar;
    EditLatin *_txtEdit;
    EditLatin *_chxEdit;
    QWidget *_second;
//    QTextEdit *_txtEdit;
    QLineEdit *editNumPhr;
//    QStatusBar *statusBar;

    // Pour compléter le format APN.
    QLineEdit *refOeuvre;
    QCheckBox *CR;
    QCheckBox *CR2;
    QCheckBox *CR3;
    QCheckBox *CR4;
    QString _refOeuvre;
    QString _nomFichier;
    QString _repertoire;
    void dialogueOuvr(QString nomF);
    QDialog *dOuvr;
    void setDialOuvr();
    void afficher();
    QString saisie(QString m);
    int _decalPhr; // Pour donner un numéro autre que 1 à la 1ère phrase.
    int numLg;
    int numPar;
    int numCh;
    int numLvr;
    QLineEdit *nLg;
    QLineEdit *nPar;
    QLineEdit *nCh;
    QLineEdit *nLvr;

    QString _couleur0;
    QString _couleur1;
    QString _couleur2;
    QString _couleur3;
    QString _couleur4;
    QLineEdit *c4;
    QLineEdit *c0;
    QLineEdit *c1;
    QLineEdit *c2;
    QLineEdit *c3;

    // Pour créer une nouvelle fiche.
    QLineEdit *ficForme;
    QLineEdit *ficLemme;
    QLineEdit *ficIndice;
    QLineEdit *ficCode9;
    void setDialFiche();
    QDialog *dFiche;
    QDialog *dMot;
    QLineEdit *ficForme2;
    QDialog *dRef;
    QLineEdit *ficForme3;
    QDialog *dCoul;
    QDialog *dFind;
    QLineEdit *findForme;

    void createW();
    void createSecond();
    void readSettings();
    bool estRomain(QString r);
    QRegExp const chiffres = QRegExp("[0-9]");
    void taggage();
    double retag(QString *bitag, int iFixe);

//    LemCore *_lemCore; // Je dois créer un LemCore pour charger l'extension.
    Lasla *_lasla;
    QList<Fiche*> appelCollatinus(QString m, bool *enclit);
    QList<Fiche*> _analyses; // Je crée une variable globale avec les analyses.

    QString _texte; // Le texte chargé
    int _numPhrase; // Le numéro de la phrase à afficher.
    int _numMot; // Le numéro du mot pour lequel on veut choisir la bonne lemmatisation.
    QStringList _elements; // Le texte découpé : les éléments pairs sont les séparateurs
    QList<Mot*> _mots; // Les mots du texte analysés.
//    int _rang;
    QList<int> _finsPhrase; // Le rang du dernier mot de la phrase dans _mots.
    QList<int> _fPhrEl; // Le rang du dernier mot de la phrase dans _elements.
    QList<int> _pointsFixes; // Les indices des mots qui sont des points fixes (un seul bitag possible).
    bool _changements;
    bool static plusFreq(Fiche *f1, Fiche *f2);
    bool alerte();
    QString tag(QString code9);
    QString dicoPerso;

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

    void lireDonnees();
    QString defFormat();
    void grec();

//    QMap<QChar,int> _cntCar;
    void cntChar(QString f);

    void creerNvlFiche();
    void couperMot();
    void decimer(int i, int debPhr, int finPhr);
    void ajouterMot(QString m, int nm, QString r);
    void ajouterMotAvant();
    void ajouterMotApres();
    void supprMot();
    void separeEncli();
    void changerRef();
    QList<Fiche*> getAnalyses(QString forme);

private slots:
    void closeEvent(QCloseEvent *event);
    void nouveau();
    void ouvrir();
    void sauver(QString nomFichier = "");
    void aPropos();
    void suivante();
    void precedente();
    void debut();
    void fin();
    void fusionPhrase();
    void allerA();
    void paramOuvr();
    void paramFiche();
    void changeCouleurs();
    void setCouleurs();
    void defCouleurs(); // Pour donner les valeurs par défaut aux couleurs
//    void plusGrand();
//    void plusPetit();
    void chercher();
    void rechercher(int i = -1);
};

#endif // MAINWINDOW_H
