#include "mainwindow.h"
#include <QDateTime>
/*
    QString debut = QDateTime::currentDateTime().toString(Qt::ISODate);
    qint64 entier_deb = QDateTime::currentMSecsSinceEpoch();
    QString historic = "historique.txt";
    // Je crée un fichier avec l'historique dans le dossier courrant.
    QFile stats(historic);
    stats.open(QFile::Append|QFile::Text);
    stats.write(requete.toUtf8());
    debut.prepend("\nDébut : ");
    debut.append("\n");
    stats.write(debut.toUtf8());
    stats.close();

*/

/**
 * \file mainwindow.cpp
 *
 * \brief Fichier mainwindow.cpp
 * associé au tagueur LASLA-Collatinus avec une interface graphique.
 *
 * Je reprends le travail fait avec le tagueur en daemon
 * qui doit toujours être installé sur HyperBase-Web.
 * J'ai aussi changé beaucoup de choses pour utiliser l'IA.
 * Voir article dans ALMA 2024.
 *
 * Idéalement, je voudrais regrouper ici quatre éléments :
 * - le tagueur à proprement parler qui prend un fichier txt
 *      et qui produit les APN (et ZPN) comme précédemment
 *      ainsi qu'un CSV qui sera utilisé par l'IA
 * - l'appel à l'IA qui se fait, sur PC, par le lancement
 *      d'un exécutable tiré des scripts Python par PyInstaller
 * - la réconciliation entre les prédictions de l'IA et
 *      les possibles proposés par la lemmatisation
 *      (à la fois par liste de formes et par Collatinus)
 * - l'édition et la corrections du fichier APN obtenu,
 *      avec toutes les possibilités qui étaient proposées
 *      dans les versions précédentes du tagueur.
 *
 * Pour le daemon, l'interface avec le monde extérieur se faisait
 * via le module serveur qui recevait les requêtes
 * et les faisait traiter par lasla::traite.
 * En bref, tout se passait dans le module lasla.
 * Les données ont donc été transférées dans le module lasla
 * alors qu'elles étaient dans mainwindow pour les tagueurs précédents.
 *
 * Une solution bête serait de reconstruire la requête dans
 * mainwindow (ou de la saisir dans un fenestron ad-hoc)
 * et de la passer comme si elle venait du serveur.
 * Cela satisferait la première étape.
 * Les deuxième et troisième étapes pourraient être traitées de même,
 * tout se faisant dans des fichiers.
 * La quatrième étape reste orpheline, ainsi qu'une interaction
 * avec un philologue pour les formes inconnues.
 * D'autre part, si je veux pouvoir passer par mainwindow
 * pour demander de l'aide au philologue, je dois mettre
 * la classe MainWindow dans la classe Lasla...
 *
 */


/******************
 * Classe EditLatin
 * héritée de Collatinus
 ******************/

/**
 * \fn EditLatin::EditLatin (QWidget *parent): QTextEdit (parent)
 * \brief Créateur de la classe EditLatin, dérivée de
 * QTextEdit afin de pouvoir redéfinir l'action
 * connectée au clic de souris sur une ligne.
 *
 * Copié de Collatinus et adapté aux besoins.
 *
 */
EditLatin::EditLatin(QWidget *parent, int id) : QTextEdit(parent)
{
    mainwindow = qobject_cast<MainWindow *>(parent);
    _ident = id;
    QFont font("Courier");
    setFont(font);
}

/**
 * \fn bool EditLatin::event(QEvent *event)
 * \brief Captation du survol de la souris pour
 *        afficher une bulle d'aide.
 *
 * La routine repère la position du curseur et identifie
 * la ligne sur laquelle on traine. Il demande alors
 * à son père (mainwindow) le texte à afficher
 * dans la bulle d'aide.
 *
 */
bool EditLatin::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::ToolTip:
        {
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            QPoint P = mapFromGlobal(helpEvent->globalPos());
            QTextCursor tc = cursorForPosition(P);
            tc.select(QTextCursor::LineUnderCursor);
            QString mot = tc.selectedText();
            if (mot.startsWith("***")) mot.remove(0,3);
            if (mot.isEmpty () || !mot.contains("\t"))
                return QWidget::event (event);
            bool OK = false;
            int num = mot.section("\t",0,0).toInt(&OK);
            if (!OK)
            {
               // qDebug() << "transfert";
                return QTextEdit::event (event);
            }
            QString txtBulle = mainwindow->bulle(num,_ident);
            // Il faudrait voir quelle info donner ! Traduire le code en 9 ?
            if (!txtBulle.isEmpty())
            {
                txtBulle.prepend("<p style='white-space:pre'>");
                txtBulle.append("</p>");
                QRect rect(P.x()-20,P.y()-10,40,40); // Je définis un rectangle autour de la position actuelle.
                QToolTip::setFont(font());
                QToolTip::showText(helpEvent->globalPos(), txtBulle.trimmed(),
                                   this, rect, 10000);
                // La bulle disparaît si le curseur sort du rectangle.
            }
           // qDebug() << "bulle";
            return true;
        }
        default:
       // qDebug() << "default";
            return QTextEdit::event(event);
    }
}

/**
 * \fn void EditLatin::mouseReleaseEvent (QMouseEvent *e)
 * \brief Captation de la fin du clic de souris.
 *
 * La routine repère la ligne sur laquelle on clique.
 * Elle envoie à son père (mainwindow) le numéro de
 * cette ligne et un identifiant.
 * Elle sert à choisir le mot à considérer
 * ou à choisir la bonne lemmatisation.
 *
 */
void EditLatin::mouseReleaseEvent(QMouseEvent *e)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) cursor.select(QTextCursor::LineUnderCursor);
    QString st = cursor.selectedText();
//    qDebug() << st;
    if (st.startsWith("***")) st.remove(0,3);
    if (!st.isEmpty() && st.contains("\t"))
    {
        bool OK = false;
        int num = st.section("\t",0,0).toInt(&OK);
        if (OK)
            mainwindow->choisir(num,_ident);
    }
    QTextEdit::mouseReleaseEvent(e);
}

/******************
 * Fin d' EditLatin
 *
 * Classe MainWindow
 * héritée de Collatinus
 ******************/

/**
 * @brief MainWindow::MainWindow
 * @param parent
 *
 * Création de l'objet autour duquel tout est construit.
 *
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    createW();
    _barre = statusBar();

    _barre->showMessage("Chargement en cours...");
//    lireDonnees();
    QString prefixe = qApp->applicationDirPath() + "/data/";
    dicoPerso = prefixe + "perso.csv";
    lemPersLa = prefixe + "lem_pers.la";
    lemPersFr = prefixe + "lem_pers.fr";
    collat = prefixe + "Collatinus.csv";
    primoC = true;

//qDebug() << "debut";
    _lasla = new Lasla(this);
//qDebug() << "lasla";
    _barre->clearMessage();
    _lasla->_nomFichier = "";
    _repertoire = "";
    _numPhrase = 1;
    _numMot = -1;
    _preProc = false;

    // Pour les fenêtres de dialogue.
    setDialOuvr();
//    qDebug() << "dialogues";
    setDialFiche();
//    qDebug() << "fiches";

/*    _couleur0 = "gainsboro";
    // Aussi "lightGray", plus foncé, ou "whiteSmoke", plus clair.
    _couleur1 = "white";
    _couleur2 = "lightCyan";
    // Aussi "aliceBlue" ou "azure"
    _couleur3 = "yellow";*/
    defCouleurs();

    readSettings();
/* Je remplace l'appel via le serveur par l'insertion des modules de Collatinus
    // Test du serveur de Collatinus
    QTcpSocket * tcpSocket = new QTcpSocket();
    tcpSocket->abort();
    tcpSocket->connectToHost(QHostAddress::LocalHost, 5555);
    if (!tcpSocket->waitForConnected(1000))
    {
        statusBar()->showMessage("Le serveur de Collatinus ne répond pas...");
    }
    tcpSocket->disconnectFromHost();
    tcpSocket->close();
    */

    _changements = false;
//    _lemCore = new LemCore(this);
//    _lasla = new Lasla(this);
//    _lemCore->setExtension(true);
//    _lemCore->setCible("k9,fr");
//    qDebug() << _lemCore->cibles().keys() << _lemCore->cible();

}

MainWindow::~MainWindow()
{
//    delete ui;
}

/**
 * \fn void MainWindow::closeEvent(QCloseEvent *event)
 * \brief Vérifie que le travail est sauvé
 *        avant la fermeture de l'application.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    bool lire = true;
    // S'il y a des changements, je demande confirmation.
    if (_changements) lire = alerte();
    if (lire)
    {
        // Je quitte si on a confirmé la sortie.
        // Mais je veux sauver la config avant
        QSettings settings("LASLA_tagger", "LASLA_tagger");
        settings.beginGroup("fenetre");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        settings.endGroup();
        settings.beginGroup("fenetre2");
        settings.setValue("geometry2", _second->saveGeometry());
        settings.endGroup();
        settings.beginGroup("couleurs");
        settings.setValue("C0",_couleur0);
        settings.setValue("C1",_couleur1);
        settings.setValue("C2",_couleur2);
        settings.setValue("C3",_couleur3);
        settings.setValue("C4",_couleur4);
        settings.endGroup();
        settings.beginGroup("options");
        // settings.setValue("police", font.family());
        int pt = _txtEdit->font().pointSize();
        settings.setValue("zoom", pt);
        settings.setValue("repertoire",_repertoire);
        settings.endGroup();

        _second->hide();
        event->accept();
    }
    else
    {
        // Si on annule la sortie.
        event->ignore();
    }
}

void MainWindow::readSettings()
{
    QSettings settings("LASLA_tagger", "LASLA_tagger");
    // état de la fenêtre
    settings.beginGroup("fenetre");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    settings.endGroup();
    // état de la fenêtre
    settings.beginGroup("fenetre2");
    _second->restoreGeometry(settings.value("geometry2").toByteArray());
    settings.endGroup();
    settings.beginGroup("couleurs");
    _couleur0 = settings.value("C0").toString();
    _couleur1 = settings.value("C1").toString();
    _couleur2 = settings.value("C2").toString();
    _couleur3 = settings.value("C3").toString();
    _couleur4 = settings.value("C4").toString();
    if (_couleur0 == "") _couleur0 = "gainsboro";
    if (_couleur1 == "") _couleur1 = "white";
    if (_couleur2 == "") _couleur2 = "lightCyan";
    if (_couleur3 == "") _couleur3 = "yellow";
    if (_couleur4 == "") _couleur4 = "aquamarine";

    settings.endGroup();
    settings.beginGroup("options");
    int pt = settings.value("zoom",12).toInt();
//    pt = 12;
    _txtEdit->setFontPointSize(pt);
    _chxEdit->setFontPointSize(pt);
    _repertoire = settings.value("repertoire",QDir::homePath()).toString();
    settings.endGroup();

}

/**
 * @brief MainWindow::createW
 *
 * Prépare la fenêtre principale.
 *
 */
void MainWindow::createW()
{
    resize(768, 532);
    _txtEdit = new EditLatin(this,0);
//    _txtEdit = new QTextEdit(this);
    setCentralWidget(_txtEdit);
    setObjectName("LASLA_tagger");
    setWindowTitle(tr("LASLA_tagger"));
    setWindowIcon(QIcon(":/res/laslalogo.jpg"));
//    QFont font("Courier");
//    _txtEdit->setFont(font);
    createSecond();

    actionPreProc = new QAction(QIcon(":/res/gear.svg"),"PreProc",this);
    actionPreProc->setObjectName(QStringLiteral("actionPreProc"));
    actionPreProc->setCheckable(true);
    actionPreProc->setChecked(false);
    actionParLot = new QAction(QIcon(":/res/copie.svg"),"Par Lot",this);
    actionParLot->setObjectName(QStringLiteral("actionParLot"));
    actionNouveau = new QAction(QIcon(":/res/document-new.svg"),"Nouveau",this);
    actionNouveau->setObjectName(QStringLiteral("actionNouveau"));
    actionOuvrir = new QAction(QIcon(":/res/document-open.svg"),"Ouvrir",this);
    actionOuvrir->setObjectName(QStringLiteral("actionOuvrir"));
    actionSauver = new QAction(QIcon(":/res/document-save.svg"),"Sauver",this);
    actionSauver->setObjectName(QStringLiteral("actionSauver"));
    actionA_propos = new QAction("À propos",this);
    actionA_propos->setObjectName(QStringLiteral("actionA_propos"));
    quitAct = new QAction(QIcon(":/res/power.svg"), tr("&Quitter"), this);
    quitAct->setStatusTip(tr("Quitter l'application"));

    actionSuivante = new QAction(QIcon(":/res/Droite.svg"),"Phrase suivante",this);
    actionSuivante->setObjectName(QStringLiteral("actionSuivante"));
    actionPrecedente = new QAction(QIcon(":/res/Gauche.svg"),"Phrase précédente",this);
    actionPrecedente->setObjectName(QStringLiteral("actionPrecedente"));
    actionDebut = new QAction(QIcon(":/res/Haut.svg"),"Début du texte",this);
    actionDebut->setObjectName(QStringLiteral("actionDebut"));
    actionFin = new QAction(QIcon(":/res/Bas.svg"),"Fin du texte",this);
    actionFin->setObjectName(QStringLiteral("actionFin"));
    actionFusion = new QAction(QIcon(":/res/noeud.svg"),"Fusion avec phrase suivante",this);
    actionFusion->setObjectName(QStringLiteral("actionFusion"));
    actionCouleurs = new QAction(QIcon(":/res/couleurs.svg"),"Changer les couleurs",this);
    actionCouleurs->setObjectName(QStringLiteral("actionCouleurs"));
    zoomAct = new QAction(QIcon(":res/zoom.svg"), tr("Plus grand"), this);
    deZoomAct = new QAction(QIcon(":res/dezoom.svg"), tr("Plus petit"), this);
    findAct = new QAction(QIcon(":res/edit-find.svg"), tr("Chercher"), this);
    reFindAct = new QAction(QIcon(":res/edit-find.svg"), tr("Chercher encore"), this);

    // raccourcis
    zoomAct->setShortcut(QKeySequence::ZoomIn);
    deZoomAct->setShortcut(QKeySequence::ZoomOut);
    findAct->setShortcut(QKeySequence::Find);
    reFindAct->setShortcut(QKeySequence::FindNext);
    actionNouveau->setShortcuts(QKeySequence::New);
    actionOuvrir->setShortcuts(QKeySequence::Open);
    actionSauver->setShortcuts(QKeySequence::Save);
    quitAct->setShortcut(
        QKeySequence(tr("Ctrl+Q")));  // QKeySequence::Quit inopérant

    editNumPhr = new QLineEdit("1",this);
    editNumPhr->setMinimumWidth(60);
    editNumPhr->setMaximumWidth(60);


    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QStringLiteral("menuBar"));
    menuBar->setGeometry(QRect(0, 0, 768, 22));
    menuFichier = new QMenu("Fichier",menuBar);
    menuFichier->setObjectName(QStringLiteral("menuFichier"));
    menuPhrase = new QMenu("Phrase",menuBar);
    menuPhrase->setObjectName(QStringLiteral("menuPhrase"));
    menuAide = new QMenu("Options",menuBar);
    menuAide->setObjectName(QStringLiteral("menuAide"));
    setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    addToolBar(Qt::TopToolBarArea, mainToolBar);
    mainToolBar->addAction(actionPreProc);
    mainToolBar->addAction(actionNouveau);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionOuvrir);
    mainToolBar->addAction(actionSauver);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionDebut);
    mainToolBar->addAction(actionPrecedente);
    mainToolBar->addWidget(editNumPhr);
    mainToolBar->addAction(actionSuivante);
    mainToolBar->addAction(actionFin);
    mainToolBar->addAction(findAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionFusion);
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionCouleurs);
    mainToolBar->addAction(zoomAct);
    mainToolBar->addAction(deZoomAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(quitAct);

    menuBar->addAction(menuFichier->menuAction());
    menuBar->addAction(menuPhrase->menuAction());
    menuBar->addAction(menuAide->menuAction());
    menuFichier->addAction(actionParLot);
    menuFichier->addAction(actionPreProc);
    menuFichier->addSeparator();
    menuFichier->addAction(actionNouveau);
    menuFichier->addAction(actionOuvrir);
    menuFichier->addAction(actionSauver);
    menuFichier->addSeparator();
    menuFichier->addAction(quitAct);
    menuPhrase->addAction(actionDebut);
    menuPhrase->addAction(actionPrecedente);
    menuPhrase->addAction(actionSuivante);
    menuPhrase->addAction(actionFin);
    menuPhrase->addSeparator();
    menuPhrase->addAction(actionFusion);
    menuPhrase->addAction(findAct);
    menuPhrase->addAction(reFindAct);
    menuAide->addAction(zoomAct);
    menuAide->addAction(deZoomAct);
    menuAide->addAction(actionCouleurs);
    menuAide->addSeparator();
    menuAide->addAction(actionA_propos);

    connect(actionPreProc, SIGNAL(changed()), this, SLOT(preProc()));
    connect(actionParLot, SIGNAL(triggered()), this, SLOT(parLot()));

    connect(actionNouveau, SIGNAL(triggered()), this, SLOT(nouveau()));
    connect(actionOuvrir, SIGNAL(triggered()), this, SLOT(ouvrir()));
    connect(actionSauver, SIGNAL(triggered()), this, SLOT(sauver()));
    connect(actionA_propos, SIGNAL(triggered()), this, SLOT(aPropos()));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

    connect(actionDebut, SIGNAL(triggered()), this, SLOT(debut()));
    connect(actionPrecedente, SIGNAL(triggered()), this, SLOT(precedente()));
    connect(actionSuivante, SIGNAL(triggered()), this, SLOT(suivante()));
    connect(actionFin, SIGNAL(triggered()), this, SLOT(fin()));
    connect(actionFusion, SIGNAL(triggered()), this, SLOT(fusionPhrase()));
    connect(editNumPhr, SIGNAL(returnPressed()), this, SLOT(allerA()));
    connect(actionCouleurs, SIGNAL(triggered()), this, SLOT(changeCouleurs()));

    connect(zoomAct, SIGNAL(triggered()), _txtEdit, SLOT(zoomIn()));
    connect(zoomAct, SIGNAL(triggered()), _chxEdit, SLOT(zoomIn()));
    connect(deZoomAct, SIGNAL(triggered()), _txtEdit, SLOT(zoomOut()));
    connect(deZoomAct, SIGNAL(triggered()), _chxEdit, SLOT(zoomOut()));
    connect(findAct, SIGNAL(triggered()), this, SLOT(chercher()));
    connect(reFindAct, SIGNAL(triggered()), this, SLOT(rechercher()));

    setWindowTitle(tr("LASLA_tagger"));
    setWindowIcon(QIcon(":/res/laslalogo.jpg"));

}

/**
 * @brief MainWindow::aPropos
 *
 * Affiche une fenêtre de dialogue avec les remerciements.
 *
 */
void MainWindow::aPropos()
{
    QMessageBox::about(
        this, tr("LASLA_tagger"),
        tr("<b>LASLA_tagger</b><br/>\n"
           "<i>Lemmatiseur et tagger<br/>\n basé sur les textes du LASLA</i><br/>\n"
           "Licence GPL, © Philippe Verkerk, 2016 <br/><br/>\n"
           "Merci à :<ul>\n"
           "<li>Yves Ouvrard</li>\n"
           "<li>Margherita Fantoli</li>\n"
           "<li>Dominique Longrée et le <a href='http://web.philo.ulg.ac.be/lasla/'>LASLA</a>\n</li></ul>"));

}

/**
 * @brief MainWindow::setDialOuvr
 *
 * Prépare une fenêtre de dialogue pour saisir
 * la référence de l'œuvre. Sans l'ouvrir.
 *
 */
void MainWindow::setDialOuvr()
{
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text = new QLabel;
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text->setWordWrap(true);
    text->setText(
        "<p>Pour obtenir le format APN, vous devez indiquer : <ul> "
        "<li>la référence de l\'œuvre en trois caractères<br/>"
        "éventuellement suivie de \'&\' et du numéro de la 1ère phrase</li> "
        "<li>les champs qui doivent repérer le mot dans le texte "
        "«Po,V» ou «L,Ch,Par». Peuvent être décalés.<br/>"
        "Par défaut, le programme compte les sauts de ligne.</li> "
        "</ul></p>.");

    dOuvr = new QDialog(this);
    refOeuvre = new QLineEdit(tr("Zxz&1"));
    Medieval = new QCheckBox(tr("Graphies médiévales"));
    Prose = new QCheckBox(tr("Texte en prose"));
    CR = new QCheckBox(tr("Saut de ligne simple"));
    CR2 = new QCheckBox(tr("Saut de ligne double"));
    CR3 = new QCheckBox(tr("Saut de ligne triple"));
    CR4 = new QCheckBox(tr("Saut de ligne quadruple"));
    nLg = new QLineEdit("1");
    nPar = new QLineEdit("1");
    nCh = new QLineEdit("1");
    nLvr = new QLineEdit("1");
    nLg->setMaximumWidth(30);
    nPar->setMaximumWidth(30);
    nCh->setMaximumWidth(30);
    nLvr->setMaximumWidth(30);

    QPushButton *appliButton = new QPushButton(tr("Appliquer"));
//    QPushButton *cloreButton = new QPushButton(tr("Fermer"));

    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->addWidget(icon);
    topLayout->addWidget(text);
    topLayout->addWidget(refOeuvre);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(appliButton);
//    bottomLayout->addWidget(cloreButton);
    bottomLayout->addStretch();

    QGridLayout *centerLayout = new QGridLayout;
    centerLayout->addWidget(CR,0,0,Qt::AlignLeft);
    centerLayout->addWidget(CR2,1,0,Qt::AlignLeft);
    centerLayout->addWidget(CR3,2,0,Qt::AlignLeft);
    centerLayout->addWidget(CR4,3,0,Qt::AlignLeft);
    centerLayout->addWidget(nLg,0,1,Qt::AlignLeft);
    centerLayout->addWidget(nPar,1,1,Qt::AlignLeft);
    centerLayout->addWidget(nCh,2,1,Qt::AlignLeft);
    centerLayout->addWidget(nLvr,3,1,Qt::AlignLeft);
    centerLayout->addWidget(Medieval,4,0,1,-1,Qt::AlignHCenter);
    centerLayout->addWidget(Prose,5,0,1,-1,Qt::AlignHCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(centerLayout);
    mainLayout->addLayout(bottomLayout);

    dOuvr->setLayout(mainLayout);
    connect(appliButton, SIGNAL(clicked()), this, SLOT(paramOuvr()));
//    connect(cloreButton, SIGNAL(clicked()), &dOuvr, SLOT(close()));
}

/**
 * @brief MainWindow::dialogueOuvr
 * @param nomF : le nom du fichier.
 *
 * Ouvre une fenêtre de dialogue pour saisir
 * la référence de l'œuvre.
 *
 */
void MainWindow::dialogueOuvr(QString nomF)
{
    Medieval->setChecked(_lasla->optionMed());
    // Nouveau bouton médiéval.
    Prose->setChecked(_lasla->_prose);
    CR->setChecked(true);
    CR2->setChecked(false);
    CR3->setChecked(false);
    CR4->setChecked(false);

    nLg->setText("1");
    nPar->setText("1");
    nCh->setText("1");
    nLvr->setText("1");

//    QDialog dOuvr(this);
    dOuvr->setWindowTitle(nomF);

    dOuvr->exec();
}

/**
 * @brief MainWindow::paramOuvr
 *
 * Valide la saisie de la référence de l'œuvre
 * en fermant la fenêtre correspondante.
 *
 */
void MainWindow::paramOuvr()
{
    QString ref = refOeuvre->text();
    int dec = 10001; // Valeur par défaut
    if (ref.contains("&"))
    {
        dec = ref.section("&",1,1).toInt() + 10000;
        ref = ref.section("&",0,0);
    }
    if (ref.size() > 3) ref = ref.left(3);
    else while (ref.size() < 3) ref += " ";
    // Pour le format APN, la référence de l'œuvre est en 3 caractères.
    _lasla->_refOeuvre = ref;
    _lasla->_decalPhr = dec;
    _lasla->numLg = nLg->text().toInt();
    _lasla->numPar = nPar->text().toInt();
    _lasla->numCh = nCh->text().toInt();
    _lasla->numLvr = nLvr->text().toInt();
    // Les numéros des premiers ligne, paragraphe, chapitre et livre.
    // Au maximum trois parmi ces quatre.

    _lasla->formatRef = defFormat();
    // Définit le format de la référence avec 1, 2 ou 3 champs
    // Corrige aussi les erreurs 0 et 4 champs
    // en validant ou dévalidant CR.
    _lasla->_CR1 = CR->isChecked();
    _lasla->_CR2 = CR2->isChecked();
    _lasla->_CR3 = CR3->isChecked();
    _lasla->_CR4 = CR4->isChecked();
    _lasla->graphMed(Medieval->isChecked());
    _lasla->_prose = Prose->isChecked();

    if (dOuvr->isVisible())
        dOuvr->close();
    // Dans le cas du traitement d'un lot de fichiers,
    // je veux intitialiser tous les paramètres avec les mêmes valeurs.
    // Je vais donc appeler paramOuvr() sans que la fenêtre de dialogue soit ouverte.
}

void MainWindow::changeCouleurs()
{
    c0->setText(_couleur0);
    c1->setText(_couleur1);
    c2->setText(_couleur2);
    c3->setText(_couleur3);
    c4->setText(_couleur4);
    dCoul->exec();
}

void MainWindow::defCouleurs()
{
    _couleur0 = "gainsboro";
    // Aussi "lightGray", plus foncé, ou "whiteSmoke", plus clair.
    _couleur1 = "white";
    _couleur2 = "lightCyan";
    // Aussi "aliceBlue" ou "azure"
    _couleur3 = "yellow";
    // les valeurs par défaut
    _couleur4 = "aquamarine";
    dCoul->close();
    if (!_lasla->_finsPhrase.isEmpty())
        afficher();
}

void MainWindow::setCouleurs()
{
    _couleur0 = c0->text();
    _couleur1 = c1->text();
    _couleur2 = c2->text();
    _couleur3 = c3->text();
    _couleur4 = c4->text();
    // les valeurs choisies
    dCoul->close();
    if (!_lasla->_finsPhrase.isEmpty())
        afficher();
}

/**
 * @brief MainWindow::setDialFiche
 *
 * Prépare une fenêtre de dialogue pour saisir
 * une nouvelle fiche. Sans l'ouvrir.
 *
 * En réalité, prépare des fenêtres de dialogue
 * pour plusieurs fonctions :
 * 1) nouvelle fiche
 * 2) nouveau mot (à ajouter dans le texte)
 * 3) changement de référence
 * 4) définition des couleurs
 * 5) fenêtre pour la recherche
 *
 * Chacune sera ensuite remplie par une routine dédiée
 * puis exécutée avec, par exemple, dFiche->exec().
 * Les champs importants de chaque dialogue sont des variables globales
 * et pourront être relus (et traités) par le programme principal.
 *
 */
void MainWindow::setDialFiche()
{
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text = new QLabel;
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text->setWordWrap(true);
    text->setText("Nouvelle fiche :");

    dFiche = new QDialog(this);
    QLabel *tForme = new QLabel("Forme : ");
    ficForme = new QLineEdit();
    QLabel *tLemme = new QLabel("Lemme : ");
    ficLemme = new QLineEdit();
    QLabel *tInd = new QLabel("Indice : ");
    ficIndice = new QLineEdit();
    QLabel *tCode = new QLabel("Code : ");
    ficCode9 = new QLineEdit();
    QPushButton *finButton = new QPushButton(tr("Appliquer"));
    connect(finButton, SIGNAL(clicked()), this, SLOT(paramFiche()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(icon,0,0,Qt::AlignCenter);
    layout->addWidget(text,0,1,Qt::AlignCenter);
    layout->addWidget(tForme,1,0,Qt::AlignRight);
    layout->addWidget(ficForme,1,1,Qt::AlignLeft);
    layout->addWidget(tLemme,2,0,Qt::AlignRight);
    layout->addWidget(ficLemme,2,1,Qt::AlignLeft);
    layout->addWidget(tInd,3,0,Qt::AlignRight);
    layout->addWidget(ficIndice,3,1,Qt::AlignLeft);
    layout->addWidget(tCode,4,0,Qt::AlignRight);
    layout->addWidget(ficCode9,4,1,Qt::AlignLeft);
    layout->addWidget(finButton,5,1,Qt::AlignRight);

    dFiche->setLayout(layout);

    QLabel *iconC = new QLabel;
    iconC->setPixmap(QPixmap(":/res/collatinus.jpg").scaled(128,128));
    QLabel *iconC1 = new QLabel;
    iconC1->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *cText = new QLabel;
    cText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cText->setWordWrap(true);
    cText->setText("Nouveau lemme pour Collatinus.");
    QLabel *cText2 = new QLabel;
    cText2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cText2->setWordWrap(true);
    QString blabla = "Pour définir une nouvelle abréviation, "
            "il convient d'ajouter un point après la forme "
                     "et on définit alors une fiche LASLA";
    cText2->setText(blabla);

    dCollatinus = new QDialog(this);
    QLabel *cForme = new QLabel("Forme : ");
    colForme = new QLineEdit();
    QLabel *cLemme = new QLabel("Lemme : ");
    colLemme = new QLineEdit();
    QLabel *cRad1 = new QLabel("Radical 1 : ");
    QLabel *cIndice = new QLabel("Indice LASLA");
    colRad1 = new QLineEdit();
    QLabel *cRad2 = new QLabel("Radical 2 : ");
    QLabel *cCode = new QLabel("Code en 9");
    colRad2 = new QLineEdit();
    QLabel *cIndM = new QLabel("Indic. morph. : ");
    colIndMorph = new QLineEdit();
    QLabel *cTrad = new QLabel("Traduction : ");
    colTrad = new QLineEdit();
    QLabel *cLien = new QLabel("Lemme LASLA : ");
    colLien = new QLineEdit();
    QLabel *cModel = new QLabel("Modèle : ");
    colModele = new QComboBox();
//    qDebug() << _lasla->listeModeles();
    colModele->addItems(_lasla->listeModeles());
    // Il y a trop de modèles !
    QPushButton *cfinButton = new QPushButton(tr("Appliquer"));
    connect(cfinButton, SIGNAL(clicked()), this, SLOT(paramFiche()));

    QGridLayout *clayout = new QGridLayout;
    clayout->addWidget(iconC,0,0,4,1,Qt::AlignCenter);
    clayout->addWidget(iconC1,0,3,3,1,Qt::AlignCenter);
    clayout->addWidget(cText,4,0,Qt::AlignCenter);
    clayout->addWidget(cText2,5,0,2,1,Qt::AlignLeft);
    clayout->addWidget(cForme,0,1,Qt::AlignRight);
    clayout->addWidget(colForme,0,2,Qt::AlignLeft);
    clayout->addWidget(cLemme,1,1,Qt::AlignRight);
    clayout->addWidget(colLemme,1,2,Qt::AlignLeft);
    clayout->addWidget(cModel,2,1,Qt::AlignRight);
    clayout->addWidget(colModele,2,2,Qt::AlignLeft);
    clayout->addWidget(cRad1,3,1,Qt::AlignRight);
    clayout->addWidget(colRad1,3,2,Qt::AlignLeft);
    clayout->addWidget(cIndice,3,3,Qt::AlignLeft);
    clayout->addWidget(cRad2,4,1,Qt::AlignRight);
    clayout->addWidget(colRad2,4,2,Qt::AlignLeft);
    clayout->addWidget(cCode,4,3,Qt::AlignLeft);
    clayout->addWidget(cIndM,5,1,Qt::AlignRight);
    clayout->addWidget(colIndMorph,5,2,Qt::AlignLeft);
    clayout->addWidget(cTrad,6,1,Qt::AlignRight);
    clayout->addWidget(colTrad,6,2,Qt::AlignLeft);
    clayout->addWidget(cLien,5,3,Qt::AlignRight);
    clayout->addWidget(colLien,6,3,Qt::AlignRight);
    clayout->addWidget(cfinButton,7,2,Qt::AlignRight);

    dCollatinus->setLayout(clayout);

    QLabel *icon2 = new QLabel;
    icon2->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text2 = new QLabel;
    text2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text2->setWordWrap(true);
    text2->setText("Nouveau mot :");

    dMot = new QDialog(this);
    QLabel *tForme2 = new QLabel("Mot : ");
    ficForme2 = new QLineEdit();
    QPushButton *finButton2 = new QPushButton(tr("Appliquer"));
    connect(finButton2, SIGNAL(clicked()), this, SLOT(paramFiche()));

    QGridLayout *layout2 = new QGridLayout;
    layout2->addWidget(icon2,0,0,Qt::AlignCenter);
    layout2->addWidget(text2,0,1,Qt::AlignCenter);
    layout2->addWidget(tForme2,1,0,Qt::AlignRight);
    layout2->addWidget(ficForme2,1,1,Qt::AlignLeft);
    layout2->addWidget(finButton2,2,1,Qt::AlignRight);

    dMot->setLayout(layout2);

    QLabel *icon3 = new QLabel;
    icon3->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text3 = new QLabel;
    text3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text3->setWordWrap(true);
    text3->setText("<p><b>Changer la référence.</b><br />"
                   "Une référence est composée de 2 ou 3 champs :"
                   "<ul><li>la référence de l'œuvre et le numéro de phrase</li>"
                   "<li>la référence du mot dans le texte</li>"
                   "<li>éventuellement le code de subordination</li></ul>");

    dRef = new QDialog(this);
    QLabel *tForme3 = new QLabel("Référence : ");
    ficForme3 = new QLineEdit();
    QPushButton *finButton3 = new QPushButton(tr("Appliquer"));
    connect(finButton3, SIGNAL(clicked()), this, SLOT(paramFiche()));

    QGridLayout *layout3 = new QGridLayout;
    layout3->addWidget(icon3,0,0,Qt::AlignCenter);
    layout3->addWidget(text3,0,1,Qt::AlignCenter);
    layout3->addWidget(tForme3,1,0,Qt::AlignRight);
    layout3->addWidget(ficForme3,1,1,Qt::AlignLeft);
    layout3->addWidget(finButton3,2,1,Qt::AlignRight);

    dRef->setLayout(layout3);

    QLabel *icon4 = new QLabel;
    icon4->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text4 = new QLabel;
    text4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text4->setWordWrap(true);
    text4->setText("Choisir les couleurs :<br />"
                   "<a href='https://www.w3.org/TR/SVG/types.html#ColorKeywords'> Noms possibles</a>");
    text4->setOpenExternalLinks(true);

    dCoul = new QDialog(this);
    QLabel *tc0 = new QLabel("Entête : ");
    c0 = new QLineEdit();
    QLabel *tc1 = new QLabel("Lignes paires : ");
    c1 = new QLineEdit();
    QLabel *tc2 = new QLabel("Lignes impaires : ");
    c2 = new QLineEdit();
    QLabel *tc3 = new QLabel("Sélection : ");
    c3 = new QLineEdit();
    QLabel *tc4 = new QLabel("Points fixes : ");
    c4 = new QLineEdit();
    QPushButton *annulerButton = new QPushButton(tr("Annuler"));
    QPushButton *retablirButton = new QPushButton(tr("Rétablir"));
    QPushButton *appliquerButton = new QPushButton(tr("Appliquer"));
    appliquerButton->setDefault(true);
    connect(appliquerButton, SIGNAL(clicked()), this, SLOT(setCouleurs()));
    connect(annulerButton, SIGNAL(clicked()), this, SLOT(paramFiche()));
    connect(retablirButton, SIGNAL(clicked()), this, SLOT(defCouleurs()));
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(annulerButton);
    hLayout->addWidget(retablirButton);
    hLayout->addWidget(appliquerButton);

    QGridLayout *layout4 = new QGridLayout;
    layout4->addWidget(icon4,0,0,Qt::AlignCenter);
    layout4->addWidget(text4,0,1,Qt::AlignCenter);
    layout4->addWidget(tc0,1,0,Qt::AlignRight);
    layout4->addWidget(c0,1,1,Qt::AlignLeft);
    layout4->addWidget(tc1,2,0,Qt::AlignRight);
    layout4->addWidget(c1,2,1,Qt::AlignLeft);
    layout4->addWidget(tc2,3,0,Qt::AlignRight);
    layout4->addWidget(c2,3,1,Qt::AlignLeft);
    layout4->addWidget(tc3,4,0,Qt::AlignRight);
    layout4->addWidget(c3,4,1,Qt::AlignLeft);
    layout4->addWidget(tc4,5,0,Qt::AlignRight);
    layout4->addWidget(c4,5,1,Qt::AlignLeft);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(layout4);
    vLayout->addLayout(hLayout);

    dCoul->setLayout(vLayout);

    QLabel *icon5 = new QLabel;
    icon5->setPixmap(QPixmap(":/res/laslalogo.jpg"));
    QLabel *text5 = new QLabel;
    text5->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text5->setWordWrap(true);
    text5->setText("Recherche du mot");

    dFind = new QDialog(this);
    QLabel *tForme5 = new QLabel("Mot : ");
    findForme = new QLineEdit();
    QPushButton *finButton5 = new QPushButton(tr("Rechercher"));
    QPushButton *annuleButton5 = new QPushButton(tr("Annuler"));
    connect(finButton5, SIGNAL(clicked()), this, SLOT(rechercher()));
    finButton5->setDefault(true);
    connect(annuleButton5, SIGNAL(clicked()), this, SLOT(paramFiche()));

    QGridLayout *layout5 = new QGridLayout;
    layout5->addWidget(icon5,0,0,Qt::AlignCenter);
    layout5->addWidget(text5,0,1,Qt::AlignCenter);
    layout5->addWidget(tForme5,1,0,Qt::AlignRight);
    layout5->addWidget(findForme,1,1,Qt::AlignLeft);
    layout5->addWidget(annuleButton5,5,0,Qt::AlignLeft);
    layout5->addWidget(finButton5,5,1,Qt::AlignRight);

    dFind->setLayout(layout5);
}

/**
 * @brief MainWindow::paramFiche
 *
 * Valide la saisie de la fiche en fermant la fenêtre correspondante.
 *
 */
void MainWindow::paramFiche()
{
    dFiche->close();
    dCollatinus->close();
    dMot->close();
    dRef->close();
    dCoul->close();
    dFind->close();
}

/**
 * @brief MainWindow::alerte
 * @return un booléen pour dire si on continue ou pas.
 *
 * Si on souhaite quitter le programme ou charger un nouveau texte
 * sans avoir sauvé le travail précédent, on affiche une boîte
 * de dialogue pour proposer de sauver le travail.
 *
 * Si on clique sur le bouton "Annuler", la fonction retourne false
 * et l'appelant doit en tenir compte (et ne rien faire).
 *
 * Si on clique sur "Sauver", la routine MainWindow::sauver() est appelée
 * avant de retourner la valeur true, sans chercher à savoir
 * si la sauvegarde a bien été faite.
 *
 * Si on clique sur "Ne pas sauver", on retourne true sans
 * autre forme de procès.
 *
 */
bool MainWindow::alerte()
{
    // Il y a des modifications non-sauvées.
    QMessageBox attention(QMessageBox::Warning,tr("Alerte !"),tr("Votre travail n'a pas été sauvé !"));
//        attention.setText(tr("Votre travail n'a pas été sauvé !"));
    QPushButton *annulerButton =
          attention.addButton(tr("Annuler"), QMessageBox::ActionRole);
    QPushButton *ecraserButton =
          attention.addButton(tr("Ne pas sauver"), QMessageBox::ActionRole);
    QPushButton *sauverButton =
          attention.addButton(tr("Sauver"), QMessageBox::ActionRole);
    attention.setDefaultButton(sauverButton);
    attention.exec();
    if (attention.clickedButton() == annulerButton) return false;
    else if (attention.clickedButton() == sauverButton) sauver();
    return true;
}

/**
 * @brief MainWindow::suivante
 *
 * Passe à la phrase suivante et l'affiche.
 *
 */
void MainWindow::suivante()
{
    _numPhrase += 1;
    if (_numPhrase > _lasla->_finsPhrase.size())
        _numPhrase = _lasla->_finsPhrase.size();
    editNumPhr->setText(QString::number(_numPhrase));
    afficher();
}

/**
 * @brief MainWindow::precedente
 *
 * Remonte d'une phrase et l'affiche.
 *
 */
void MainWindow::precedente()
{
    _numPhrase -= 1;
    if (_numPhrase < 1) _numPhrase = 1;
    editNumPhr->setText(QString::number(_numPhrase));
    afficher();
}

/**
 * @brief MainWindow::debut
 *
 * Va à la première phrase du texte et l'affiche.
 *
 */
void MainWindow::debut()
{
    _numPhrase = 1;
    editNumPhr->setText("1");
    afficher();
}

/**
 * @brief MainWindow::fin
 *
 * Va à la dernière phrase du texte et l'affiche.
 *
 */
void MainWindow::fin()
{
    _numPhrase = _lasla->_finsPhrase.size();
    editNumPhr->setText(QString::number(_numPhrase));
    afficher();
}

/**
 * @brief MainWindow::allerA
 *
 * Prend le numéro de la phrase que l'on veut afficher dans le cadre
 * prévu à cet effet dans la barre d'outils.
 * Et affiche la phrase désignée.
 *
 */
void MainWindow::allerA()
{
    _numPhrase = editNumPhr->text().toInt();
    if (_numPhrase < 1) _numPhrase = 1;
    if (_numPhrase > _lasla->_finsPhrase.size())
        _numPhrase = _lasla->_finsPhrase.size();
    editNumPhr->setText(QString::number(_numPhrase));
    afficher();
}

/**
 * @brief MainWindow::afficher
 *
 * Routine qui affiche l'information sur les mots de la phrase courante.
 * Comme pour les fichiers APN, on met une forme par ligne.
 * Chaque ligne commence par le numéro du mot suivi d'une tabulation.
 * Cela permet à l'EditLatin de dire où on a cliqué ou où le curseur se trouve.
 *
 */
void MainWindow::afficher()
{
//    QStringList aff;
    QString num = "%1\t";
    int i = 0;
    QString ph = "";
    if (_lasla->_elements.isEmpty())
    {
        // Chargé à partir d'un APN : pas de ponctuation
        if (_numPhrase > 1) i = _lasla->_finsPhrase[_numPhrase - 2];
        for (; i < _lasla->_finsPhrase[_numPhrase - 1]; i++)
        {
            QString m = Mot::courte(_lasla->_mots[i]->getFLem());
            if (m.startsWith("<") && m.contains(">"))
                ph += m.section(">",1) + " ";
            else if (m.endsWith(">") && m.contains("<"))
                ph += m.section("<",0,0);
            else ph += m + " ";
        }
    }
    else
    {
        if (_numPhrase > 1) i = _lasla->_fPhrEl[_numPhrase - 2] + 1;
        for (; i < _lasla->_fPhrEl[_numPhrase - 1]; i++)
            ph += _lasla->_elements[i];
        QString sep = _lasla->_elements[_lasla->_fPhrEl[_numPhrase - 1]];
        if (sep.contains(" ")) sep = sep.section(" ",0,0);
        if (sep.contains("\n")) sep = sep.section("\n",0,0);
        ph += sep;
    }
    // J'ai reconstitué la phrase. Je pourrais le faire dans Lasla.
//    aff << ph;
//    aff << "---";
//    _txtEdit->setText(aff.join("\n"));
    _txtEdit->clear();
//    QTextDocument *document = new QTextDocument(_txtEdit);
    QTextCursor cursor(_txtEdit->textCursor());
//    cursor.insertBlock();
    QTextBlockFormat backgroundFormat = cursor.blockFormat();
    QTextBlockFormat choixFormat = backgroundFormat;
    choixFormat.setBackground(QColor(_couleur3));
    choixFormat.setLineHeight(120,QTextBlockFormat::ProportionalHeight);
    backgroundFormat.setBackground(QColor(_couleur0));
    // Aussi "silver", plus foncé, ou "gainsboro", plus clair.
    QTextBlockFormat fixeFormat = backgroundFormat;
    fixeFormat.setBackground(QColor(_couleur4));
    fixeFormat.setLineHeight(120,QTextBlockFormat::ProportionalHeight);

    cursor.setBlockFormat(backgroundFormat);
    cursor.insertText(ph);
    // J'ai écrit la phrase. Je la voudrais sur fond gris clair.
    backgroundFormat.setBackground(QColor("white"));
    cursor.insertBlock(); // Une ligne blanche vide.
    cursor.setBlockFormat(backgroundFormat);
    backgroundFormat.setLineHeight(120,QTextBlockFormat::ProportionalHeight);
    i = 0;
    if (_numPhrase > 1) i = _lasla->_finsPhrase[_numPhrase - 2];
    for (; i < _lasla->_finsPhrase[_numPhrase - 1]; i++)
    {
        if (backgroundFormat.background() == QColor(_couleur1))
            backgroundFormat.setBackground(QColor(_couleur2));
        // Aussi "aliceBlue" ou "azure"
        else backgroundFormat.setBackground(QColor(_couleur1));
        // J'alterne les lignes blanches et bleues.
        cursor.insertBlock();
        if (i == _numMot) cursor.setBlockFormat(choixFormat);
//        else if (_pointsFixes.contains(i) && (i != 0))
  //          cursor.setBlockFormat(fixeFormat);
        else cursor.setBlockFormat(backgroundFormat);
        // Je mets en évidence le mot sur lequel on a agi.
        // C'est aussi pour le résultat d'une recherche.

        int choix = _lasla->_mots[i]->getChoix();
        cursor.insertText(num.arg(i) + _lasla->_mots[i]->getInfo(choix,true));
    }
}
/* Version qui marche, mais qui manque de couleur.
void MainWindow::afficher()
{
    QStringList aff;
    QString num = "%1\t";
    int i = 0;
    QString ph = "";
    if (_elements.isEmpty())
    {
        // Chargé à partir d'un APN : pas de ponctuation
        if (_numPhrase > 1) i = _finsPhrase[_numPhrase - 2];
        for (; i < _finsPhrase[_numPhrase - 1]; i++)
        {
            QString m = Mot::courte(_mots[i]->getFLem());
            if (m.startsWith("<") && m.contains(">"))
                ph += m.section(">",1) + " ";
            else if (m.endsWith(">") && m.contains("<"))
                ph += m.section("<",0,0);
            else ph += m + " ";
        }
    }
    else
    {
        if (_numPhrase > 1) i = _fPhrEl[_numPhrase - 2] + 1;
        for (; i < _fPhrEl[_numPhrase - 1]; i++)
            ph += _elements[i];
        QString sep = _elements[_fPhrEl[_numPhrase - 1]];
        if (sep.contains(" ")) sep = sep.section(" ",0,0);
        if (sep.contains("\n")) sep = sep.section("\n",0,0);
        ph += sep;
    }
    aff << ph;
    aff << "---";
    i = 0;
    if (_numPhrase > 1) i = _finsPhrase[_numPhrase - 2];
    for (; i < _finsPhrase[_numPhrase - 1]; i++)
    {
        int choix = _mots[i]->getChoix(); // L'élément 0 est le plus probable
        aff << num.arg(i) + _mots[i]->getInfo(choix,true);
    }
    _txtEdit->setText(aff.join("\n"));
}
*/
/**
 * @brief MainWindow::bulle
 * @param n
 * @param id
 * @return
 *
 * Cette routine est appelée par EditLatin lorsque l'on
 * survole sur une ligne valide qui est affichée.
 * Elle reçoit comme paramètres le numéro qui figure
 * avant la tabulation dans la ligne cliquée et
 * un numéro qui indique quel est l'EditLatin appelant.
 *
 * En fonction de l'appelant (0 ou 1, pour le moment),
 * elle va retourne l'information pertinente
 * qu'il faut afficher dans la bulle d'aide.
 *
 */
QString MainWindow::bulle(int n, int id)
{
    if (id == 0)
    {
        if (_preProc)
        {
            // C'est les inconnus
            if ((n>=0) && (n < _lInc.size()))
            {
                QString ligne = _lInc[n].section("\t", 1, 1);
                // Je mets la phrase en bulle d'aide.
                QString clef = _lInc[n].section("\t", 0, 0);
                ligne.replace(clef, "<b>" + clef +"</b>");
                if (ligne.size() > 80)
                {
                    // La ligne est trop longue.
                    int pos = ligne.indexOf(clef);
                    if ((pos > 50) && (ligne.size() - pos > 50))
                    {
                        // Couper équitablement au début et à la fin.
                        int espace = ligne.indexOf(" ", pos - 40);
                        if (espace > 0)
                            ligne[espace] = '\n';
                        espace = ligne.indexOf(" ", pos + 30 + clef.size());
                        if (espace > 0)
                            ligne[espace] = '\n';
                    }
                    if (pos > 70)
                    {
                        // Couper au début
                        int espace = ligne.indexOf(" ", pos - 70);
                        if (espace > 0)
                            ligne[espace] = '\n';
                    }
                    if (ligne.size() - pos > 70)
                    {
                        // Couper à la fin
                        int espace = ligne.indexOf(" ", pos + 70 +clef.size());
                        if (espace > 0)
                            ligne[espace] = '\n';
                    }
                }
                return ligne;
            }
        }
        else if ((n>=0) && (n<_lasla->_mots.size()))
        {
            QStringList res;
            for (int i=0; i<_lasla->_mots[n]->cnt();i++)
            {
                QString r = _lasla->_mots[n]->getInfo(i,false);
                r.replace("<","&LT;");
                r.replace(">","&GT;");
                res << r;
            }
            QString retour = res.join("</li>\n<li>");
            retour.append("</li></ul>");
            retour.prepend("<ul><li>");
            return retour;
        }
        return "Blabla";
    }
    else if (n >=0)
    {
        return _lasla->_mots[_numMot]->bulle(n); // Il faut trouver quoi répondre.
    }
    return "";
}

/**
 * @brief MainWindow::createSecond
 *
 * Création d'un second EditLatin pour afficher
 * et choisir les analyses d'un mot.
 *
 */
void MainWindow::createSecond()
{
    _second = new QWidget();
    _second->setObjectName("Fenêtre de sélection");
    _second->setWindowTitle("Fenêtre de sélection");
    _second->setWindowIcon(QIcon(":/res/laslalogo.jpg"));
    _second->resize(700,500);
    QVBoxLayout *vLayout = new QVBoxLayout(_second);
    _chxEdit = new EditLatin(this,1);
    vLayout->addWidget(_chxEdit);
}

/**
 * @brief MainWindow::saisie
 * @param m : la forme non-trouvée
 * @return La ligne telle qu'elle serait dans un fichier du LASLA
 *
 * Lorsque le mot donné par le texte n'a pas d'analyse connue,
 * on interroge Collatinus. Mais si lui aussi échoue,
 * on demande à l'utilisateur de donner les éléments d'analyse.
 * La routine initialise les champs de dFiche et ouvre cette
 * fenêtre de dialogue. Après la validation, la routine prend
 * les divers éléments et recompose une ligne telle qu'elle
 * doit être dans les fichiers du LASLA.
 * Cette ligne est aussi sauvée dans le dico perso.
 *
 * On peut aussi faire appel à cette routine pour ajouter
 * une nouvelle analyse à une forme déjà connue.
 */
QString MainWindow::saisie(QString m)
{
    // En dernier recours, la saisie manuelle.
    ficForme->setText(m);
    ficLemme->clear();
    ficIndice->clear();
    ficCode9->clear();
    dFiche->exec(); // J'ouvre la fenêtre de dialogue pour créer une fiche
    QString ligne = ficForme->text() + ",";
    ligne += ficLemme->text() + ",";
    if (ficIndice->text().size() == 1) ligne += ficIndice->text() + ",";
    else ligne += " ,";
    QString c9 = ficCode9->text();
    if (c9.size() < 9) c9 += QString(9 - c9.size(),' ');
    else if (c9.size() > 9) c9 = c9.mid(0,9);
    ligne += c9 + ",";
    ligne += tag(c9) + ",1";
    // J'ai reconstitué une ligne du fichier listForm9.csv
/*    QFile fDic(dicoPerso);
    if (fDic.open(QIODevice::Append|QIODevice::Text))
    {
        ligne.append("\n");
        fDic.write(ligne.toUtf8());
        fDic.close();
    } */
    return ligne;
}

/**
 * @brief MainWindow::macroniser
 * @param a la chaîne saisie
 * @return la chaîne avec des macrons et des breves
 *
 * Pour faciliter la saisie des quantités (des syllabes),
 * j'utilise les touches mortes pour l'accent circonflexe
 * et pour les trémas avec une voyelle.
 * Les diphtongues ae et oe sont automatiquement reconnues comme longues.
 * Par convention, j'associe le macron à l'accent circonflexe
 * et le breve aux trémas.
 * Ce choix est lié au clavier français qui a ces deux touches mortes.
 * Sur un autre type de clavier, il faut peut-être prévoir
 * le + pour les longues et le - pour les brèves.
 * Une voyelle sans quantité, hors diphtongue, sera considérée comme commune.
 * J'aurais aussi pu choisir l'accent grave, mais il est plus difficile d'accès
 * sur le clavier d'un PC.
 */
QString MainWindow::macroniser(QString a)
{
    // Un circonflexe pour le macron
    // et des trémas pour le breve.
    // diphtongues en deux lettres ou une seule æÆŒœ
//    qDebug() << a;
    a.replace("ae","âe");
    a.replace("Ae","Âe");
    a.replace("æ","âe");
    a.replace("Æ","Âe");
    a.replace("oe","ôe");
    a.replace("Oe","Ôe");
    a.replace("œ","ôe");
    a.replace("Œ","Ôe");
    // minuscules
    a.replace("â",QChar(0x0101));
    a.replace("ä",QChar(0x0103));  // ā ă
    a.replace("ê",QChar(0x0113));
    a.replace("ë",QChar(0x0115));  // ē ĕ
    a.replace("î",QChar(0x012b));
    a.replace("ï",QChar(0x012d));  // ī ĭ
    a.replace("ô",QChar(0x014d));
    a.replace("ö",QChar(0x014f));  // ō ŏ
    a.replace("û",QChar(0x016b));
    a.replace("ü",QChar(0x016d));  // ū ŭ
    a.replace("^y",QChar(0x0233));
    a.replace("ÿ",QChar(0x045e));  // ȳ ў
    // majuscule
    a.replace("Â",QChar(0x0100));
    a.replace("Ä",QChar(0x0102));  // Ā Ă
    a.replace("Ê",QChar(0x0112));
    a.replace("Ë",QChar(0x0114));  // Ē Ĕ
    a.replace("Î",QChar(0x012a));
    a.replace("Ï",QChar(0x012c));  // Ī Ĭ
    a.replace("Ô",QChar(0x014c));
    a.replace("Ö",QChar(0x014e));  // Ō Ŏ
    a.replace("Û",QChar(0x016a));
    a.replace("Ü",QChar(0x016c));  // Ū Ŭ
    a.replace("^Y",QChar(0x0232));
    a.replace("¨Y",QChar(0x040e));  // Ȳ Ў
    // minuscules
    a.replace("a+",QChar(0x0101));
    a.replace("a-",QChar(0x0103));  // ā ă
    a.replace("e+",QChar(0x0113));
    a.replace("e-",QChar(0x0115));  // ē ĕ
    a.replace("i+",QChar(0x012b));
    a.replace("i-",QChar(0x012d));  // ī ĭ
    a.replace("o+",QChar(0x014d));
    a.replace("o-",QChar(0x014f));  // ō ŏ
    a.replace("u+",QChar(0x016b));
    a.replace("u-",QChar(0x016d));  // ū ŭ
    a.replace("y+",QChar(0x0233));
    a.replace("y-",QChar(0x045e));  // ȳ ў
    // majuscule
    a.replace("A+",QChar(0x0100));
    a.replace("A-",QChar(0x0102));  // Ā Ă
    a.replace("E+",QChar(0x0112));
    a.replace("E-",QChar(0x0114));  // Ē Ĕ
    a.replace("I+",QChar(0x012a));
    a.replace("I-",QChar(0x012c));  // Ī Ĭ
    a.replace("O+",QChar(0x014c));
    a.replace("O-",QChar(0x014e));  // Ō Ŏ
    a.replace("U+",QChar(0x016a));
    a.replace("U-",QChar(0x016c));  // Ū Ŭ
    a.replace("Y+",QChar(0x0232));
    a.replace("Y-",QChar(0x040e));  // Ȳ Ў
//    qDebug() << a;
    return a;
}

/**
 * @brief MainWindow::saisieCol
 * @param m : la forme non-trouvée
 * @return La ligne telle qu'elle serait dans lem_ext.la
 *
 * Lorsque le mot donné par le texte n'a pas d'analyse connue,
 * on interroge Collatinus. Mais si lui aussi échoue,
 * on demande à l'utilisateur de donner les éléments d'analyse.
 * La routine initialise les champs de dCollatinus et ouvre cette
 * fenêtre de dialogue. Après la validation, la routine prend
 * les divers éléments et recompose une ligne telle qu'elle
 * doit être dans les fichiers de Collatinus.
 * Cette ligne est aussi sauvée dans le dico perso.
 *
 */
QString MainWindow::saisieCol(QString m)
{
    // En dernier recours, la saisie manuelle.
    colForme->setText(m);
    colLemme->clear();
    colModele->clear();
    colModele->addItems(_lasla->listeModeles(m));
    colRad1->clear();
    colRad2->clear();
    colIndMorph->clear();
    colTrad->clear();
    dCollatinus->exec(); // J'ouvre la fenêtre de dialogue pour créer une fiche
    QString ligne = colForme->text();
    if (ligne.contains(".") || (colModele->currentText() == "LASLA"))
    {
        // J'ai une abréviation avec un point
        // ou une forme anomale qui est trop spéciale :
        // je dois créer une fiche LASLA.
        if (colLemme->text().isEmpty() && ligne.contains("."))
            ligne += ",#,";
        else ligne += "," + colLemme->text() + ",";
        if (colRad1->text().trimmed().isEmpty())
        {
        if (ligne[0].isUpper()) ligne += "N,";
        else ligne += " ,";
        }
        else ligne += colRad1->text().mid(0,1) + ",";
        if (colRad2->text().trimmed().isEmpty())
        {
        if (ligne[0].isUpper()) ligne += "A68      ,A8 ,1";
        else ligne += "A68      ,A8 ,1";
        // Je considère que les abréviations sont des noms indéclinables.
        }
        else
        {
            QString code = colRad2->text().trimmed() + "         ";
            // J'ajoute beaucoup de blancs à la fin pour couper à 9 caractères
            ligne += code.mid(0,9) + ",";
            // Mais je ne fais aucune vérification de la pertinence du code.
            ligne += _lasla->tag(code.mid(0,9)) + ",1";
        }
        // J'ai reconstitué une ligne du fichier listForm9.csv
/* La sauvegarde dans dico perso est faite lorsque la fiche est ajoutée.
 * ça se passe dans Lasla::ajouterFiche.
 *         QFile fDic(dicoPerso);
        if (fDic.open(QIODevice::Append|QIODevice::Text))
        {
            ligne.append("\n");
            fDic.write(ligne.toUtf8());
            fDic.close();
        }
        */
    }
    else
    {
        // Entrée pour lem_ext.la jusqu'au Tab. Traduction ensuite.
        ligne = macroniser(colLemme->text());
        if (ligne.isEmpty()) return ligne;
        // Je n'ai pas le droit de laisser un lemme en blanc.
        QString lemmeCol = Ch::atone(Ch::deramise(ligne.section("=",0,0)));
        ligne += "|" + colModele->currentText() + "|";
        ligne += macroniser(colRad1->text()) + "|";
        ligne += macroniser(colRad2->text()) + "|";
        ligne += colIndMorph->text() + "|1";
        QString trad = colTrad->text();
        if (!trad.isEmpty())
        {
            QFile fDic(lemPersFr);
            if (fDic.open(QIODevice::Append|QIODevice::Text))
            {
                if (primoC) fDic.write(_date.toUtf8());
                trad.prepend(":");
                trad.prepend(lemmeCol);
                trad.append("\n");
                // C'est une ligne de traduction
                // "lemme:traduction"
                fDic.write(trad.toUtf8());
                fDic.close();
            }
        }
        trad = colLien->text();
        if (!trad.isEmpty())
        {
            QFile fDic(collat);
            if (fDic.open(QIODevice::Append|QIODevice::Text))
            {
                if (primoC) fDic.write(_date.toUtf8());
                trad.prepend(",");
                trad.prepend(lemmeCol);
                trad.append("\n");
                // C'est une ligne d'équivalence
                // "lemmeCollatinus,lemmeLASLA"
                fDic.write(trad.toUtf8());
                fDic.close();
            }
        }
        trad = ligne;
        QFile fDic(lemPersLa);
        if (fDic.open(QIODevice::Append|QIODevice::Text))
        {
            if (primoC)
            {
                fDic.write(_date.toUtf8());
                primoC = false;
            }
            trad.append("\n");
            fDic.write(trad.toUtf8());
            fDic.close();
        }
        ligne += "\t" + colTrad->text();
    }
    return ligne;
}

/**
 * @brief MainWindow::defFormat
 * @param ref
 * @return
 *
 * Cette routine déduit des champs sélectionnés dans la fenêtre
 * qui définit la référence de l'œuvre le format que doit
 * prendre la référence de ligne. Ça peut aller d'un seul champ
 * (le numéro de vers ou de paragraphe) à 3 champs, par exemple
 * livre, chapitre, paragraphe.
 * Cette routine initialise aussi la variable passée par adresse.
 */
QString MainWindow::defFormat()
{
    QString formatRef;
    int nChamps = 0;
    if (CR->isChecked()) nChamps++;
    if (CR2->isChecked()) nChamps++;
    if (CR3->isChecked()) nChamps++;
    if (CR4->isChecked()) nChamps++;
    switch (nChamps) {
    case 4:
        CR->setChecked(false); // 3 champs max.
        formatRef = "%1,%2,%3";
//        *ref = "1,1,1";
        break;
    case 3:
        formatRef = "%1,%2,%3";
//        *ref = "1,1,1";
        break;
    case 2:
        formatRef = "%1,%2";
//        *ref = "1,1";
        break;
    case 1:
        formatRef = "%1";
//        *ref = "1";
        break;
    default:
        CR->setChecked(true); // Au moins un
        formatRef = "%1";
//        *ref = "1";
        break;
    }
    return formatRef;
}

void MainWindow::chercher()
{
    dFind->exec();
}

void MainWindow::rechercher(int i)
{
    dFind->close();
    QString mRech = findForme->text();
    mRech.replace("v","u");
    mRech.replace("U","V");
    mRech.replace("j","i");
    mRech.replace("J","I");
    if (i == -1) i = _numMot + 1;
    bool tourne = true;
    while ((i < _lasla->_mots.size()) && tourne)
    {
        if (_lasla->_mots[i]->getFLem().contains(mRech, Qt::CaseSensitive)
                || _lasla->_mots[i]->getFTexte().contains(mRech, Qt::CaseSensitive))
            tourne = false;
        else i++;
    }
    if (i == _lasla->_mots.size())
    {
        // J'ai atteint la fin du texte
        QMessageBox attention(QMessageBox::Warning,tr("Fin du texte atteinte !"),tr("Voulez-vous reprendre au début ?"));
        QPushButton *annulerButton =
              attention.addButton(tr("Non"), QMessageBox::ActionRole);
        QPushButton *sauverButton =
              attention.addButton(tr("Oui"), QMessageBox::ActionRole);
        attention.setDefaultButton(sauverButton);
        attention.exec();
        if (attention.clickedButton() == sauverButton) rechercher(0);
    }
    else
    {
        _numMot = i;
        i = 0;
        while (_numMot >= _lasla->_finsPhrase[i]) i++;
        _numPhrase = i + 1;
        editNumPhr->setText(QString::number(_numPhrase));
        afficher();
    }
}


/**
 * Fonction de test pour les nombres
 * écrits en chiffres romains.
 *
 */
bool MainWindow::estRomain(QString r)
{
    return !(r.contains(QRegExp ("[^IVXLCDM]"))
             || r.contains("IL")
             || r.contains("IVI"));
}
