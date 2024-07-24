#include "mainwindow.h"
#define VERIF

/**
* Ce fichier contient les routines de traitement
* des données.
* Il complète le fichier "mainwindow.cpp" qui
* contient les routines de gestion des fenêtres.
**/

void MainWindow::lireDonnees()
{
    QString prefixe = qApp->applicationDirPath() + "/data/";
    dicoPerso = prefixe + "perso.csv";
    QList<Fiche*> fCherche;
    QString ligne;
    Fiche * fiche;

    // Lecture de la liste des formes et création des fiches
    QFile fListe (prefixe + "listForm9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (ligne.startsWith("!")) continue;
        fiche = new Fiche(ligne);
//        cntChar(fiche->getForme());
        _fiches.insert(fiche->getClef(),fiche);
        if (fiche->getTag()[1] == 'e')
        {
            // C'est un enclitique qui ne peut se trouver seul.
            fCherche << fiche; // Un mot contient une liste de fiche*
            if (fiche->getClef() == "que")
                motQue = new Mot("-", "que", fCherche);
            if (fiche->getClef() == "ue")
                motVe = new Mot("-", "ue", fCherche);
            if (fiche->getClef() == "ne")
                motNe = new Mot("-", "ne", fCherche);
            if (fiche->getClef() == "cum")
                motCum = new Mot("-", "cum", fCherche);
            fCherche.clear(); // Je laisse la liste vide pour le prochain usage.
        }
    }
    fListe.close();
    // Pour l'instant, que les formes simples...
    // Les formes composées sont dans "../LASLA_data/listComp9.csv"

    fListe.setFileName(prefixe + "listComp9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        fiche = new Fiche(ligne);
        QString str = fiche->getClef();
        if (str.contains("/") && !str.startsWith("$"))
        {
            _fComplx.insert(str,fiche); // Il y a ambiguïté.
//            qDebug() << str << fiche->getForme() << fiche->getCmpl();
        }
        else _fiches.insert(str,fiche);
    }
    fListe.close();
    // Fin de la lecture des formes officielles.
//    qDebug() << _fComplx.size();

    fListe.setFileName(dicoPerso);
    if (fListe.open (QIODevice::ReadOnly|QIODevice::Text))
    {
        // Si le dico perso existe, je le lis.
        while (!fluxL.atEnd ())
        {
            ligne = fluxL.readLine ();
            fiche = new Fiche(ligne);
            _fiches.insert(fiche->getClef(),fiche);
        }
        fListe.close();
    }
    // J'ai mis de côté les formes ambiguës, pour lesquelles je ne savais pas quelle clef choisir.
    foreach (QString str, _fComplx.keys())
    {
        // Pour chaque fiche mise de côté, je dois choisir la clef.
        fiche = _fComplx.value(str);
        QString ff = fiche->getForme();
        // J'ai une forme ambiguë "<bene dictum>st" ou "<data nulla>st"
        ff.replace(">","<");
        ff.replace(" ","> ");
        ff.append(">");
        // J'ai construit le pendant virtuel "<bene> dictum<st>" ou "<data> nulla<st>"
//        qDebug() << str << fiche->getForme() << ff;
        QStringList ecl = str.split("/"); // Même s'il n'y en a que 2.
        bool egaux = false;
        // Je cherche le pendant virtuel parmi les formes associées à la 2e clef
        // "dictumst" ou "nullast".
        foreach (Fiche *f, _fiches.values(ecl[1]))
        {
            QString s = f->getForme();
            if (s == ff)
            {
//                qDebug() << str << f->getForme() << s << ecl[0] << f->getClef() << (ecl[0] == f->getClef());
                egaux = egaux || (ecl[1] == f->getClef());
            }
        }
        if (egaux)
        {
            // J'ai trouvé la forme reconstruite, donc la clef est "nullast"
            fiche->setClef(ecl[1]);
            fiche->setCmpl(Fiche::cmpl(fiche->getForme(),ecl[1]));
            _fiches.insert(ecl[1],fiche);
        }
        else
        {
            // Je n'ai pas retrouvé la forme reconstruite, donc la clef est "bene".
            fiche->setClef(ecl[0]);
            fiche->setCmpl(Fiche::cmpl(fiche->getForme(),ecl[0]));
            _fiches.insert(ecl[0],fiche);
        }
//        qDebug() << str << fiche->getForme() << fiche->getClef() << fiche->getCmpl();
    }

    // Lecture des trigrammes
    fListe.setFileName(prefixe + "list_3T9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        _trigrammes.insert(ligne.section(",",0,2),ligne.section(",",3,3).toInt());
    }
    fListe.close();
    // Les trigrammes obtenus contiennent les deux virgules comme séparateur.
    // Les bigrammes, une seule.

    // Lecture des bigrammes
    fListe.setFileName(prefixe + "list_2T9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        _bigrammes.insert(ligne.section(",",0,1),ligne.section(",",2,2).toInt());
        if (ligne.contains("snt"))
                _trigrammes.insert(ligne.section(",",0,1),ligne.section(",",2,2).toInt());
        // Je charge les bigrammes de début et fin de phrase, pour les premier et dernier mots.
    }
    fListe.close();

    // Lecture des monogrammes
    fListe.setFileName(prefixe + "list_1T9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        _monogrammes.insert(ligne.section(",",0,0),ligne.section(",",1,1).toInt());
    }
    fListe.close();
    _monogrammes.insert("   ",1);
    _bigrammes.insert("snt",_monogrammes.value("snt"));
    // Pour le premier mot...

    // Lecture des correspondances entre Collatinus et LASLA
    fListe.setFileName(prefixe + "Collatinus.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (!ligne.startsWith("!"))
        {
            ligne.remove("-");
            _corresp.insert(ligne.section(",",0,0),ligne.section(",",1,1));
        }
    }
    fListe.close();
    _changements = false;
}
/*
void MainWindow::cntChar(QString f)
{
    for (int i=0;i<f.size();i++) _cntCar[f[i]]++;
}
*/
/**
 * @brief MainWindow:nouveau
 * Ouvre un nouveau fichier texte et lemmatise l'ensemble des mots.
 * Doit aussi sauver un fichier XML pour conserver l'état intermédiaire.
 */
void MainWindow::nouveau()
{
    bool lire = true;
    if (_changements) lire = alerte();
    if (lire)
    {
        statusBar()->showMessage("Chargement du texte et lemmatisation...");
        QString nomFichier =
                QFileDialog::getOpenFileName(this, "Lire le fichier",_repertoire,"Text files (*.txt)");
        if (!nomFichier.isEmpty())
        {
            QFile f(nomFichier);
            if (f.open(QFile::ReadOnly)) _texte = f.readAll();
            f.close();
        }
        if (!_texte.isEmpty())
        {
            _changements = true; // Pour ne pas perdre mon travail !
            QFileInfo info = QFileInfo(nomFichier);
            _repertoire = info.canonicalPath();
            _nomFichier = info.baseName();
            // Nom du fichier, sans l'extension qui devrait être txt.

            dialogueOuvr(nomFichier);
            // Pour définir la référence de l'œuvre et le mode de référencement
            // Ainsi qu'un éventuel décalage des numéros de phrase, ligne, etc...
//            QList<Fiche*> analyses;
            QList<Fiche*> analyses_C;
            Mot * mot;
//            _rang = 0;
            QString ref;
            QString refO;
            QString formatRef = defFormat();
            ref = formatRef;
            if (CR4->isChecked()) ref = ref.arg(numLvr);
            if (CR3->isChecked()) ref = ref.arg(numCh);
            if (CR2->isChecked()) ref = ref.arg(numPar);
            if (CR->isChecked()) ref = ref.arg(numLg);
//            int numPhr = 0;
            _mots.clear();
            _finsPhrase.clear();
            _fPhrEl.clear();
            _pointsFixes.clear();
            _pointsFixes << 0; // Le début du texte est un point fixe. (?)
            // J'ai chargé mon texte. Je dois le lemmatiser entièrement.
            _elements = _texte.split(QRegExp("\\b"));
//            qDebug() << _elements;
            if (_texte.contains(QRegExp("[0-9$]"))) grec();
            // Les $ encadrent des mots grecs
            // S'il y a des chiffres, ils seront placés en séparateurs.
            // Le test du point (abréviation ou fin de phrase) ainsi que
            // le traitement de l'apostrophe pourraient être aussi
            // traités en amont de l'analyse, puisqu'il s'agit de modifier
            // les éléments. On gagnerait en lisibilité pour l'analyse.

//            for (int i = 1; i < _elements.length(); i += 2)
            int ratio = 1;
            int i = (_elements.size()-1) / 200;
            while (ratio < i) ratio *= 2;
            int jj = 1;
            QProgressDialog progr("Lemmatisation en cours...", "Arrêter", 0, (_elements.size() / ratio) + 1, _txtEdit);
            progr.setWindowModality(Qt::WindowModal);
            progr.setMinimumDuration(1000);
            progr.setValue(0);
            progr.show();
//            qint64 tps;

            i = 1;
            while (i < _elements.length())
            {
//                if ((i & 254) == 0)
                if ((jj * ratio) < i)
                {
                    jj = i / ratio + 1;
                    progr.setValue(jj);
                    if (progr.wasCanceled())
                        break;
                    //... Stop !
                }
                QString np = QString::number(_finsPhrase.size() + _decalPhr);
                np[0] = '&'; // Par défaut. Je ne gère pas les '#' et '+'.
                refO = _refOeuvre.mid(0,3) + np + "/" + ref;

                QString m = _elements.at(i); // les mots dans l'ordre.
                _analyses.clear();
                bool enclit = false;
                // Attention aux v, U, j et J !
                m.replace("v","u");
                m.replace("U","V");
                m.replace("j","i");
                m.replace("J","I");

                QString sep = _elements[i+1];
                // C'est le séparateur qui suit le mot examiné.
                // Attention, ajouter 1 s'il y a enclitique !
                if (sep.contains("."))
                {
                    if (sep.startsWith("."))
                    {
                        // Ce point peut être celui d'une abréviation ou celui d'une fin de phrase.
                        _analyses = _fiches.values(m + ".");
                        if (!_analyses.isEmpty())
                        {
                            m += "."; // C'est une abréviation
                            sep.remove(0,1);
                            _elements[i].append(".");
                            _elements[i+1].remove(0,1);
                        }
                        else if (estRomain(m))
                        {
                            // Un nombre en chiffres romain suivi d'un point serait un ordinal
                            m += ".";
                            sep.remove(0,1);
                            _elements[i].append(".");
                            _elements[i+1].remove(0,1);
                            // Créer la fiche.
                            Fiche * fiche = new Fiche(m + ",nbr, ,D28      ,D8,1");
                            _analyses  << fiche;
                        }
                    }
                    else if (sep.startsWith("\'.") && (m == "M"))
                    {
                        m = "M\'.";
                        sep.remove(0,2);
                        _elements[i].append("\'.");
                        _elements[i+1].remove(0,2);
                        _analyses = _fiches.values(m);
                    }
                }
                else if (sep.startsWith("'"))
                {
                // J'ai traité le cas le "M'." qui est l'abréviation de Manius.
                // Mais il y a d'autres "'" en fin de mot comme "bonu'" ou
                // au milieu de formes jointes comme "<acturu>'s".
                    if (sep == "'")
                    {
                        // Je dois joindre les 3 éléments
                        _analyses = _fiches.values(m + "'" + _elements[i+2]);
                        if (!_analyses.isEmpty())
                        {
                            m += "'" + _elements[i+2];
                            _elements[i].append("'" + _elements[i+2]);
                            _elements.removeAt(i+1);
                            _elements.removeAt(i+1);
                            sep = _elements[i+1];
                        }
                    }
                    else
                    {
                        _analyses = _fiches.values(m + "'");
                        if (!_analyses.isEmpty())
                        {
                            m += "'";
                            sep.remove(0,1);
                            _elements[i].append("'");
                            _elements[i+1].remove(0,1);
                        }
                    }
                }

                if (_analyses.isEmpty()) _analyses = _fiches.values(m);
                if ((_fPhrEl.last() == (i - 1)) && m[0].isUpper())
                {
                    // Je suis en début de phrase et j'ai une majuscule
                    QString mMin = m.toLower();
                    if (m[0] == 'V') mMin[0] = 'u';
                    _analyses.append(_fiches.values(mMin));
                }
                // Traiter les majuscules et les enclitiques
                if (_analyses.isEmpty() && m[0].isUpper())
                {
                    if (estRomain(m))
                    {
                        Fiche * fiche = new Fiche(m + ",nbr, ,D18      ,D8,1");
                        _analyses  << fiche;
                    }
                    else
                    {
                        QString mMin = m.toLower();
                        if (m[0] == 'V') mMin[0] = 'u';
                        _analyses = _fiches.values(mMin);
                    }
                }
                if (_analyses.isEmpty())
                {
                    if (m.endsWith("que") || m.endsWith("cum"))
                    {
                        _analyses = _fiches.values(m.mid(0,m.size()-3));
                        if (_analyses.isEmpty() && m[0].isUpper())
                        {
                            QString mMin = m.toLower().mid(0,m.size()-3);
                            if (m[0] == 'V') mMin[0] = 'u';
                            _analyses = _fiches.values(mMin);
                        }
                        enclit = !_analyses.isEmpty();
                    }
                    else if (m.endsWith("ue") || m.endsWith("ne"))
                    {
                        _analyses = _fiches.values(m.mid(0,m.size()-2));
                        if (_analyses.isEmpty() && m[0].isUpper())
                        {
                            QString mMin = m.toLower().mid(0,m.size()-2);
                            if (m[0] == 'V') mMin[0] = 'u';
                            _analyses = _fiches.values(mMin);
                        }
                        enclit = !_analyses.isEmpty();
                    }
                }

                // Appel systématique à Collatinus
                _analyses.append(appelCollatinus(m, &enclit));
                // Si la liste d'analyses n'est pas vide,
                // je ne dois créer que les nouvelles analyses.
                if (_analyses.isEmpty())
                {
                    // En dernier recours, la saisie manuelle.
                    progr.setWindowModality(Qt::NonModal);
                    progr.hide();
                    // Mettre une partie du texte environnant dans la fenêtre.
                    QString t;
                    int ii = 0;
                    if (!_fPhrEl.isEmpty()) ii = _fPhrEl.last() + 1;
                    for (; ii < i+20; ii++)
                        if (ii < _elements.size()) t.append(_elements[ii]);
                    _txtEdit->setText(t);
                    QString ligne = saisie(m);
                    // J'ai reconstitué une ligne du fichier listForm9.csv
                    Fiche * fiche = new Fiche(ligne);
                    _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
                    _analyses.append(fiche);
                    progr.setWindowModality(Qt::WindowModal);
                    progr.show();
                }
                // Je dois examiner si mes analyses possibles groupent plusieurs mots.
                QList<Fiche*> aMultiple;
                QList<Fiche*> aRetirer;
                foreach (Fiche * f, _analyses)
                {
                    QString c = f->getCmpl();
                    if (!c.isEmpty()  && !c.startsWith("§ <")
                            && (c.startsWith("§ ") || c.startsWith("<§ ")))
                    {
                        // L'info complémentaire demande au moins un mot de plus.
                        bool egaux = (_elements[i+1] == " ") && !enclit;
                        // Pas de séparation après le mot courant
                        // Je pense que si le premier mot porte l'enclitique, il ne faut rien faire.
                        if (c.contains(" <")) c = c.section(" <",0,0);
                        if (c.contains(" (")) c = c.section(" (",0,0);
                        c.remove("<");
                        c.remove(">");
                        QStringList ecl = c.split(QRegExp("\\b"));
                        for (int j=1; j < ecl.size()-1; j++)
                            egaux = egaux && (ecl[j] == _elements[i+j+1]);
                        if (egaux) aMultiple << f;
                        else aRetirer << f;
                        // qDebug() << m << c << f->getClef() << f->getForme() << egaux << ecl;
                    }
                }
                if (aMultiple.isEmpty())
                {
                    if (!aRetirer.isEmpty())
                    {
                        // Il y a des analyses qui n'ont pas trouvé le mot nécessaire.
                        foreach (Fiche * f, aRetirer)
                        {
                            _analyses.removeOne(f);
                        }
                    }
                    if (_analyses.size() > 1)
                    {
                        qSort (_analyses.begin(), _analyses.end(), plusFreq);
                        // Je trie les analyses pour avoir la plus fréquente d'abord.
                        if (_analyses[0]->jointe())
                        {
                            // Forme jointe --> plusieurs mots avec analyse unique (ou pas).
                            QString r = refO;
                            r[3] = '#'; // Pour les formes sur plusieurs lemmes.
                            // Il faut ordonner les mots...
                            bool motInter = false;
                            QList<Fiche*> lf1;
                            QList<Fiche*> lf2;
                            for (int na = 0; na < _analyses.size(); na++)
                            {
                                QString f = _analyses[na]->getForme();
                                if (f.startsWith("<")) lf2 << _analyses[na];
                                else lf1 << _analyses[na];
                                if ((f.count(">") == 2) && (f.count("<") == 2))
                                    motInter = true;
                            }
                            // La liste lf1 vient d'abord
                            mot = new Mot (m,lf1[0]->getForme(),lf1, i, refO);
                            _mots.append(mot);
                            if (motInter)
                            {
                                // Il y a 4 formes qui donnent 3 mots.
                                QList<Fiche*> lf3;
                                lf1.clear();
                                for (int na = 0; na < lf2.size(); na++)
                                {
                                    QString f = lf2[na]->getForme();
                                    if (f.endsWith(">")) lf1 << lf2[na];
                                    else lf3 << lf2[na];
                                }
                                mot = new Mot (m,lf1[0]->getForme(),lf1, i, r);
                                _mots.append(mot);
                                mot = new Mot (m,lf3[0]->getForme(),lf3, i, r);
                            }
                            else
                            {
                                // On est passé par Collatinus qui a trouvé un <st>.
                                mot = new Mot (m,lf2[0]->getForme(),lf2, i, r);
                            }
                        } // Forme jointe
                        else
                        {
                            mot = new Mot (m,_analyses[0]->getForme(),_analyses, i, refO);
                        }
                    }
                    else if (_analyses.isEmpty()) mot = new Mot (m,"???",_analyses, i, refO);
                    else mot = new Mot (m,_analyses[0]->getForme(),_analyses, i, refO);
                    _mots.append(mot);
                    if (enclit)
                    {
                        if (m.endsWith("que")) _mots << motQue;
                        else if (m.endsWith("cum")) _mots << motCum;
                        else if (m.endsWith("ue")) _mots << motVe;
                        else if (m.endsWith("ne")) _mots << motNe;
                    }
                } // Fin de (aMultiple.isEmpty())
                else if (aMultiple.size() > 1)
                {
                    // J'ai trouvé plusieurs analyses sur plusieurs mots.
                    // Il peut s'agir de formes jointes...
                    if ((aMultiple[0]->jointe())
                            && ((aMultiple.size() == 2) || (aMultiple.size() == 3)))
                    {
                        // Forme jointe --> plusieurs mots avec analyse unique.
                        QString r = refO;
                        r[3] = '#'; // Pour les formes sur plusieurs lemmes.
                        // Il faut ordonner les mots...
                        if (aMultiple.size() == 2)
                        {
                            QList<Fiche*> lf;
                            int prems = 0;
                            if (aMultiple[0]->getForme().startsWith("<")) prems = 1;
                            // D'abord le deuxième.
                            lf << aMultiple[prems];
                            mot = new Mot (m,lf[0]->getForme(),lf, i, refO);
                            _mots.append(mot);
                            lf.clear();
                            lf << aMultiple[1-prems];
                            mot = new Mot (m,lf[0]->getForme(),lf, i, r);
                        }
                        else
                        {
                            // Il y en a 4 qui donnent 3 mots.
                            QList<Fiche*> lf;
                            int prems = 0;
                            int dern = 0;
                            while ((prems < 3) && (aMultiple[prems]->getForme().startsWith("<")))
                                prems++;
                            while ((dern < 3) && (aMultiple[dern]->getForme().endsWith(">")))
                                dern++;
                            if ((prems < 3) && (dern < 3))
                            {
                                lf << aMultiple[prems];
                                mot = new Mot (m,lf[0]->getForme(),lf, i, refO);
                                _mots.append(mot);
                                lf.clear();
                                lf << aMultiple[3-prems-dern];
                                mot = new Mot (m,lf[0]->getForme(),lf, i, r);
                                _mots.append(mot);
                                lf.clear();
                                lf << aMultiple[dern];
                                mot = new Mot (m,lf[0]->getForme(),lf, i, r);
                            }
                            else
                            {
                                qDebug() << "Problème avec " << m << prems << dern
                                         << aMultiple[0]->getForme() << aMultiple[1]->getForme()
                                         << aMultiple[2]->getForme();
                                mot = new Mot (m,aMultiple[0]->getForme(),aMultiple, i, refO);
                            }
                        }
                    }
                    else
                    {
//                        qDebug() << "Ici ? " << aMultiple.size();
                        qSort (aMultiple.begin(), aMultiple.end(), plusFreq);
                        // Je trie les analyses pour avoir la plus fréquente d'abord.
                        mot = new Mot (m,aMultiple[0]->getForme(),aMultiple, i, refO);
                    }
                    _mots.append(mot);
                    i += aMultiple[0]->getForme().count(" ") * 2;
                    if (aMultiple[0]->getForme().contains(" <") || aMultiple[0]->getForme().contains(" ("))
                        i -= 2;
                    sep = _elements[i+1];
                } // Fin de (aMultiple.size() > 1)
                else
                {
                    // Une seule analyse sur plusieurs mots
                    mot = new Mot (aMultiple[0]->getForme(),aMultiple[0]->getForme(),aMultiple, i, refO);
                    _mots.append(mot);
                    i += aMultiple[0]->getForme().count(" ") * 2;
                    if (aMultiple[0]->getForme().contains(" <") || aMultiple[0]->getForme().contains(" ("))
                        i -= 2;
                    sep = _elements[i+1];
                }
                // J'en ai fini avec le mot.

                // Suis-je à la fin d'une phrase ?
                if (sep.contains("?") || sep.contains("!") || sep.contains(";")
                        || sep.contains(":") || sep.contains("."))
                {
                    _finsPhrase.append(_mots.size()); // C'est le rang du mot courant.
                    _fPhrEl.append(i + 1); // C'est le rang du mot dans _elements.
                }

                // Suis-je à la fin d'une ligne ?
                if (sep.contains("\n"))
                {
                    // Oui ! Je dois mettre à jour la référenciation
                    int cnt = sep.count("\n");
                    switch (cnt) {
                    case 2:
                        if (CR2->isChecked()){
                            numLg = 1;
                            numPar++;
                        }
                        else numLg++;
                        // Si je ne tiens pas compte des doubles sauts, je les considère comme des simples
                        break;
                    case 3:
                        if (CR3->isChecked()){
                            numLg = 1;
                            numPar = 1;
                            numCh++;
                        }
                        else if (CR2->isChecked()){
                            // Si je ne tiens pas compte des triples sauts, je les considère comme des doubles
                            numLg = 1;
                            numPar++;
                        }
                        else numLg++;
                        break;
                    case 4:
                        if (CR4->isChecked()){
                            numLg = 1;
                            numPar = 1;
                            numCh = 1;
                            numLvr++;
                        }
                        else if (CR3->isChecked()){
                            numLg = 1;
                            numPar = 1;
                            numCh++;
                        }
                        else if (CR2->isChecked()){
                            numLg = 1;
                            numPar++;
                        }
                        else numLg++;
                        break;
                    default:
                        numLg++; // simple saut de ligne
                        break;
                    }
                    ref = formatRef;
                    if (CR4->isChecked()) ref = ref.arg(numLvr);
                    if (CR3->isChecked()) ref = ref.arg(numCh);
                    if (CR2->isChecked()) ref = ref.arg(numPar);
                    if (CR->isChecked()) ref = ref.arg(numLg);
                }
                i += 2; // Pour pouvoir regrouper des mots, j'ai dû passer de for(;;) à while ().
            } // Le texte est lemmatisé.
            progr.reset();
            progr.setLabelText("Nettoyage...");
            progr.setCancelButtonText(QString());
            ratio = 1;
            i = (_finsPhrase.size()-1) / 100;
            while (ratio < i) ratio *= 2;
            jj = 1;
            progr.setMaximum(_finsPhrase.size() / ratio + 1);
            progr.setValue(0);
//            progr.show();

            // Je dois vérifier que les analyses conditionnelles sont OK.
            // Par exemple, l'auxiliaire entre crochets doit être présent dans la phrase.
            int deb = 0;
            for (int iPhrase = 0; iPhrase < _finsPhrase.size();iPhrase++)
            {
                if ((jj * ratio) < iPhrase)
                {
                    jj = iPhrase / ratio + 1;
                    progr.setValue(jj);
                }
                for (int i = deb; i < _finsPhrase[iPhrase];i++)
                    decimer(i,deb,_finsPhrase[iPhrase]);
                deb = _finsPhrase[iPhrase];
            }
            // J'ai fini les vérifs. Je sauve l'état brut.
            sauver(_repertoire + "/" + _nomFichier + "_00.APN");
            // Début du taggage.
            taggage();
//            qDebug() << _finsPhrase.size() << _mots.size() << _finsPhrase.last();
            sauver(_repertoire + "/" + _nomFichier + "_01.APN");

            debut();
        } // Fin du traitement d'un texte non-vide.
        else _txtEdit->setText("Texte vide !");
        statusBar()->clearMessage();
    } // Fin de la lecture
}

/**
 * @brief MainWindow::ouvrir
 *
 * Il s'agit ici d'ouvrir un fichier au format APN
 * et de le rendre éditable comme s'il avait été créé
 * à partir d'un nouveau texte.
 * C'est essentiel pour pouvoir travailler sur un texte
 * en s'arrêtant de temps en temps.
 *
 * Il semblerait que j'ai oublié un cas pathologique (janvier 2019).
 * Si on a introduit, lors de la correction,
 * une nouvelle analyse que Collatinus ne connaît pas
 * et que l'on a perdu le dico perso où elle est enregistrée,
 * on la perdait si la forme était analysée.
 * Le problème devrait être résolu (février 2019) par l'ajout d'un test.
 * Je regarde si la liste "analyses" contient bien une fiche
 * qui correspond à la ligne que je viens de lire.
 * J'ai élargi le test if (analyses.isEmpty())
 * à if (analyses.isEmpty() || !contient).
 *
 */
void MainWindow::ouvrir()
{
    bool lire = true;
    if (_changements) lire = alerte();
    if (lire)
    {
        statusBar()->showMessage("Récupération du travail en cours...");
        QString nomFichier =
                QFileDialog::getOpenFileName(this, "Lire le fichier",_repertoire,"APN files (*.APN)");
        if (!nomFichier.isEmpty())
        {
            _mots.clear();
            _elements.clear();
            _finsPhrase.clear();
            _changements = false;
            QString linea;
            QString tete; // La ref de l'œuvre et n° de phrase.
            bool debPhrase;
            QString car4;
            QChar car8 = '1';
            QString lemme;
            QString indice;
            QString forme;
            QString code9;
            QString ref; // La référence de la ligne du texte (Par,l ou autre)
            QString fin; // Le code de subordination
            QString t; // Le tag
            QList<Fiche*> analyses;

            QFileInfo info = QFileInfo(nomFichier);
            _repertoire = info.canonicalPath();
            _nomFichier = info.baseName(); // Nom du fichier, sans l'extension qui devrait être APN.
            QFile f(nomFichier);
            f.open (QIODevice::ReadOnly|QIODevice::Text);
            QTextStream fluxM (&f);

            int ratio = 1;
            int i = (f.size()-1) / 100;
            while (ratio < i) ratio *= 2;
            int j = 1;
            QProgressDialog progr("Lecture en cours...", "Arrêter", 0, f.size() / ratio + 1, _txtEdit);
            progr.setWindowModality(Qt::WindowModal);
            progr.setMinimumDuration(1000);
            progr.setValue(0);
            i = 0;
            while (!fluxM.atEnd ())
            {
//                linea = ligne;
                linea = fluxM.readLine ();
                i += linea.size() + 1;
                if (j * ratio  < i)
                {
                    j = i / ratio + 1;
                    progr.setValue(j);
                    if (progr.wasCanceled())
                        break;
                    //... Stop !
                }
                if (linea.isEmpty () || linea[0] == '!') continue;

                /* Le traitement de la ligne dépend du caractère n°4 de la ligne qui suit
                & : par défaut,
                = : enclitique,
                # : une forme pour plusieurs lemmes -> formes type 'abductast' ou nombres,
                + : je ne sais plus, il n'y en a que 6 chez Pline, probablement pas très important
                ? : il en traine un.
                Il semblerait que + et ? soient des erreurs et que l'on peut les ignorer.
                linea est la ligne que je suis en train de traiter
                ligne est la ligne qui suit.
                */
                if (car8 != linea[7])
                {
                    // Je commence une nouvelle phrase;
                    debPhrase = true;
                    car8 = linea[7];
                    _finsPhrase << _mots.size();
                }
                else debPhrase = false;
                tete = linea.mid(0,8);
                car4 = linea.mid(3,1);
                lemme = linea.mid(8,21).trimmed();
                indice = linea.mid(29,1);
                forme = linea.mid(30,25).trimmed();
                ref = linea.mid(55,12).trimmed();
                code9 = linea.mid(67,9);
                fin = linea.mid(76);
                // création du tag à partir du code 9
                t = tag(code9);
                analyses = getAnalyses(forme);
                if ((car4 == "=") || ((car4 == "#") && (lemme == "NE")) || (lemme == "QVE"))
                {
                    t[1] = 'e';
                    // Pour distinguer la conjonction de coordination "que" qui est après les coordonnés.
                    // Si j'ai un enclitique, je ne dois pas retenir les autres analyses.
                    // Le problème ne se pose que pour "NE 2" et "CVM 2".
                    QList<Fiche*> na = analyses;
                    analyses.clear();
                    foreach (Fiche *f, na)
                    {
                        if (f->getTag().endsWith("e ")) analyses << f;
                    }
                    if (analyses.isEmpty())
                    {
                        analyses = na;
                        // Si un mot a été marqué comme enclitique par erreur,
                        // la liste se retrouverait vide, ce qui cause un plantage à l'écriture.
                        QMessageBox::about(
                                    this, tr("LASLA_tagger"),
                                    tr("Ligne incorrecte : \n")+ linea);
                    }
                }
                else
                {
                    // J'ai toute l'info qui est sur la ligne.
                    // Je dois créer un mot avec toutes les analyses de la forme.
//                    if (forme == "ante") qDebug() << forme << analyses.size();
                    if (debPhrase)
                    {
                        // Je suis sur le premier mot de la phrase...
                        QString f = forme.toLower();
                        if (forme[0].isLower())
                        {
                            if (forme[0] == 'u') f[0] = 'V';
                            else f[0] = forme[0].toUpper();
                        }
                        // ... je bascule la 1ère lettre Maj/min.
                        // En réalité, j'ai perdu de l'information.
//                        int as = analyses.size();
                        analyses.append(getAnalyses(f));
//                        if (f == "ante") qDebug() << forme << f << analyses.size() << as;
                    }
                    bool contient = false;
                    foreach (Fiche *f, analyses)
                    {
                        // Mais analyses peut ne pas être vide sans pour autant
                        // contenir la fiche correspondant à la ligne.
                        // C'est le cas si on a introduit une nouvelle analyse
                        // et que l'on a changé d'ordinateur sans copier le dico-perso.
                        bool egaux = (forme == f->getForme());
                        egaux = egaux && (lemme == f->getLemme());
                        egaux = egaux && (indice == f->getIndice());
                        egaux = egaux && (code9 == f->getCode());
                        contient = contient || egaux;
                    }
                    if (analyses.isEmpty() || !contient)
                    {
                        qDebug() << linea;
                        // Si analyses est vide, c'est qu'une nouvelle fiche a été introduite !
                        // C'est aussi le cas si analyses ne contient pas la fiche
                        // qui correspond à la ligne.
                        QString ligne = forme + "," + lemme + "," + indice;
                        ligne += "," + code9 + "," + t + ",1";
                        Fiche *f = new Fiche(ligne);
                        _fiches.insert(f->getClef(),f);
                        analyses << f;
                        QFile fDic(dicoPerso);
                        if (fDic.open(QIODevice::Append|QIODevice::Text))
                        {
                            ligne.append("\n");
                            fDic.write(ligne.toUtf8());
                            fDic.close();
                        }
                        // J'ai créé la fiche correspondante et je l'ai sauvée.
                    }
                    else qSort (analyses.begin(), analyses.end(), plusFreq);
                }
                Mot * mot = new Mot (Fiche::clef(forme),forme,analyses,-1, tete + "/" + ref + "/" + fin);
                _mots.append(mot);
                // Et y choisir l'analyse désignée dans la ligne.
                mot->designe(forme,lemme,indice,code9);
                if (mot->cnt() == 0)
                    QMessageBox::about(
                        this, tr("LASLA_tagger"),
                        tr("Ligne incomprise : \n")+ linea +
                                tr("\nL'erreur peut provoquer un plantage lors de la prochaine sauvegarde !"));

//                if (debPhrase)
//                if (forme=="uide<n>") qDebug() << forme << analyses.size() << mot->listeFiches().size();
            }
            f.close();
            _finsPhrase << _mots.size();
            // La fin du texte est aussi la fin de la dernière phrase.
        }
        // Je dois supprimer les analyses impossibles.
        if (!_mots.isEmpty())
        {
            int deb = 0;
            int ratio = 1;
            int i = (_finsPhrase.size() - 1) / 100;
            while (ratio < i) ratio *= 2;
            int j = 1;
            QProgressDialog progr("Finalisation...", QString(), 0, 1 + _finsPhrase.size() / ratio, _txtEdit);
            progr.setWindowModality(Qt::WindowModal);
            progr.setMinimumDuration(1000);
            progr.setValue(0);
            for (int iPhrase = 0; iPhrase < _finsPhrase.size();iPhrase++)
            {
                if (j * ratio  < iPhrase)
                {
                    j = iPhrase / ratio + 1;
                    progr.setValue(j);
                }
                for (int i = deb; i < _finsPhrase[iPhrase];i++)
                    decimer(i,deb,_finsPhrase[iPhrase]);
                deb = _finsPhrase[iPhrase];
            }
            debut();
        }
        else _txtEdit->setText("Fichier vide !");
        statusBar()->clearMessage();
    }
}

/**
 * @brief MainWindow::sauver
 * Pour sauver le résultat au format APN.
 * Sauve aussi un fichier intermédiaire au format XML avec tous les choix possibles.
 */
void MainWindow::sauver(QString nomFichier)
{
    if (nomFichier.isEmpty())
        nomFichier =
                QFileDialog::getSaveFileName(this, "Sauvegarder le travail sous...",
                                             _repertoire + "/" + _nomFichier+".APN","APN Files (*.APN)");
    if (!nomFichier.isEmpty())
    {
#ifdef VERIF
        QString nf = nomFichier;
        if (nomFichier.endsWith(".APN"))
            nf.replace(".APN",".ZPN");
        QFile v(nf);
        v.open(QFile::WriteOnly);
#endif
        QFile f(nomFichier);
        if (f.open(QFile::WriteOnly))
        {
//            QStringList aff;
//            int numPhr = 0;
            for (int i = 0; i < _mots.size(); i++)
            {
                QString ref = _mots[i]->getRef();
                if (ref.isEmpty())
                {
                    ref = _mots[i-1]->getRef();
                    ref[3] = '='; // enclitique
                }
//                aff << _mots[i]->Lasla(ref);
                QString toto = _mots[i]->Lasla(ref) + "\n";
                f.write(toto.toUtf8());
                qDebug() << toto;
#ifdef VERIF
                v.write(toto.toUtf8());
                foreach (Fiche *fm, _mots[i]->listeFiches())
                {
                    toto = fm->Lasla("!\t/ ") + "\n";
                    v.write(toto.toUtf8());
                }
#endif
            }
//            f.write(aff.join("\n").toUtf8());
            f.close();
            _changements = false;
        }
#ifdef VERIF
            v.close();
#endif
    }
}

/**
 * @brief MainWindow::plusFreq
 * @param f1
 * @param f2
 * @return si la fiche f1 est plus fréquente que f2
 *
 * Cette fonction ordonne les fiches selon plusieurs critères.
 * Elle met les formes multiples avant les formes simples.
 * Sinon c'est l'ordre donné par le nombre d'occurrences.
 *
 */
bool MainWindow::plusFreq (Fiche * f1, Fiche * f2)
{
    if (f1->getCmpl().isEmpty() && !f2->getCmpl().isEmpty())
        return false;
    if (!f1->getCmpl().isEmpty() && f2->getCmpl().isEmpty())
        return true;
    if (f1->getCmpl().isEmpty() && f2->getCmpl().isEmpty())
    {
        if (f1->getNbr() == f2->getNbr())
        {
            // Si les fréquences sont égales, je trie en fonction du code ou de l'ensemble.
            if (f1->getCode() == f2->getCode())
                return (f1->Lasla("/") > f2->Lasla("/"));
            return (f1->getCode() > f2->getCode());
        }
        return (f1->getNbr() > f2->getNbr());
    }
    // Les deux compléments sont non-vides.
    if (f1->getCmpl().count(" ") == f2->getCmpl().count(" "))
    {
        if (f1->getNbr() == f2->getNbr())
        {
            if (f1->getCode() == f2->getCode())
                return (f1->Lasla("/") > f2->Lasla("/"));
            return (f1->getCode() > f2->getCode());
        }
        return (f1->getNbr() > f2->getNbr());
    }
    return (f1->getCmpl().count(" ") > f2->getCmpl().count(" "));
}

/**
 * @brief MainWindow::tag
 * @param code9
 * @return Calcule le tag à partir du code en 9
 *
 * Cette fonction est une copie de celle que j'ai utilisée
 * lors du dépouillement des textes du LASLA.
 * Les codes en 9 sont "simplifiés" pour donner une étiquette
 * sur trois caractères. Ces étiquettes sont utilisées
 * pour les trigrammes et sont donc essentielles pour
 * le tagueur. Toutefois, quand on introduit une nouvelle fiche,
 * il suffit de donner le code en 9, puisque le tag s'en déduit.
 *
 */
QString MainWindow::tag(QString code9)
{
    // création du tag à partir du code 9
    QString t;
    if ((code9[0] == ' ') || (code9[0] == '0')) t = "X  ";
    else if (code9[0] == 'B')
    {
        if ((code9[5] == '4') || (code9[5] == '5') || (code9[5] == '6'))
            t = "V" + code9[2] + code9[3];
        else if (code9[6] == '1') t = "B" + code9[5] + "1";
        // tentative pour distinguer legimus présent et parfait.
        else t = "B" + code9[5] + " ";
    }
    else
    {
        t = code9.mid(0,4);
        t.remove(1,1);
    }
    return t;
}

/**
 * @brief MainWindow::choisir
 * @param n : le numéro qui figure au début de la ligne cliquée
 * @param id : l'identifiant de l'EditLatin appelant
 *
 * Cette routine est appelée par EditLatin lorsque l'on
 * clique sur une ligne valide qui est affichée.
 * Elle donne comme paramètres le numéro qui figure
 * avant la tabulation dans la ligne cliquée et
 * un numéro qui indique quel est l'EditLatin appelant.
 *
 * En fonction de l'appelant (0 ou 1, pour le moment),
 * elle va choisir un mot dans la phrase pour lui choisir
 * une autre analyse, ou choisir l'analyse désignée
 * parmi toutes celles proposées pour le mot courant.
 *
 */
void MainWindow::choisir(int n, int id)
{
    _changements = true; // Dès que je passe ici, je peux supposer que quelque chose change.
    if (id == 0)
    {
        if ((n>=0) && (n<_mots.size()))
        {
            _numMot = n;
//            qDebug() << n << _mots[n]->getFTexte() << _mots[n]->getChoix();
            QStringList aff;
            QStringList aff0;
            QString num = "%1\t";
/* Manque de couleurs
 *             for (int i = 0; i < _mots[n]->cnt(); i++)
            {
                if (i == _mots[n]->getChoix())
                aff << "***" + num.arg(i) + _mots[n]->getInfo(i,false) + "***";
                else aff << num.arg(i) + _mots[n]->getInfo(i,false);
            }
            */
//            aff << "--";
//            aff.prepend("--");
            aff << "-1\tAjouter une analyse...";
            aff0.prepend("-1\tAjouter une analyse...");
            aff << "-2\tAjouter un mot...";
            aff0.prepend("-2\tAjouter un mot...");
//            aff << "-3\tAjouter un mot après...";
//            aff.prepend("-3\tAjouter un mot après...");
            aff << "-4\tSupprimer ce mot...";
            aff0.prepend("-4\tSupprimer ce mot...");
            QString m = _mots[n]->getFLem();
            if (m.contains(" "))
            {
                aff << "-5\tSéparer le mot...";
                aff0.prepend("-5\tSéparer le mot...");
            }
            bool encl = m.endsWith("que") && (m.size() > 3);
            encl = encl || (m.endsWith("ne") && (m.size() > 2));
            encl = encl || (m.endsWith("ue") && !m.endsWith("que") && (m.size() > 2));
            encl = encl || (m.endsWith("cum") && (m.size() > 3));
            if (encl)
            {
                aff << "-6\tSéparer l'enclitique...";
                aff0.prepend("-6\tSéparer l'enclitique...");
            }
//            aff << "-9\tModifier la référence " + _mots[_numMot]->getRef();
//            aff0.prepend("-9\tModifier la référence " + _mots[_numMot]->getRef());
            _chxEdit->clear();
            QTextCursor cursor(_chxEdit->textCursor());

            QTextBlockFormat backgroundFormat = cursor.blockFormat();
            QTextBlockFormat choixFormat = backgroundFormat;
            backgroundFormat.setBackground(QColor(_couleur0));
            choixFormat.setBackground(QColor(_couleur3));
            choixFormat.setLineHeight(120,QTextBlockFormat::ProportionalHeight);
            // Et un fond jaune pour mettre en évidence le choix fait.

            QTextCharFormat format(cursor.charFormat());
            format.setFontFamily("Courier");
            QTextCharFormat boldFormat = format;
            format.setFontWeight(QFont::Normal);
            boldFormat.setFontWeight(QFont::Bold);
            // Les formats normal et gras pour les caractères.

            cursor.setBlockFormat(backgroundFormat);
            cursor.insertText("-9\tModifier la référence ",format);
            cursor.insertText(_mots[_numMot]->getRef() + "\n",boldFormat);
            cursor.insertText(aff0.join("\n"),format);
            // J'ai écrit le préambule sur fond gris clair.
            backgroundFormat.setBackground(QColor("white"));
            cursor.insertBlock(); // Une ligne blanche vide.
            cursor.setBlockFormat(backgroundFormat);
            backgroundFormat.setLineHeight(120,QTextBlockFormat::ProportionalHeight);

            for (int i = 0; i < _mots[n]->cnt(); i++)
            {
                if (backgroundFormat.background() == QColor(_couleur1))
                    backgroundFormat.setBackground(QColor(_couleur2));
                else backgroundFormat.setBackground(QColor(_couleur1));
                // J'alterne les lignes blanches et bleues.
                cursor.insertBlock();
                cursor.setBlockFormat(backgroundFormat);

                if (i == _mots[n]->getChoix())
                {
                    cursor.insertText(num.arg(i) + _mots[n]->getInfo(i,false),boldFormat);
                    cursor.setBlockFormat(choixFormat);
                }
                else cursor.insertText(num.arg(i) + _mots[n]->getInfo(i,false),format);
            }
            backgroundFormat.setLineHeight(100,QTextBlockFormat::ProportionalHeight);
            backgroundFormat.setBackground(QColor("white"));
            cursor.insertBlock(); // Une ligne blanche vide.
            cursor.setBlockFormat(backgroundFormat);
            backgroundFormat.setBackground(QColor(_couleur0));
            cursor.insertBlock();
            cursor.setBlockFormat(backgroundFormat);
            cursor.insertText(aff.join("\n"),format);
            cursor.insertText("\n-9\tModifier la référence ",format);
            cursor.insertText(_mots[_numMot]->getRef(),boldFormat);
            // Et les commandes à la fin sur fond gris clair.

            _second->show();
        }
    }
    else
    {
        // J'étais dans la seconde fenêtre
        if (n < 0)
        {
            // Opérations spéciales
            switch (n)
            {
            case -1:
                // C'est pour définir une nouvelle fiche.
                creerNvlFiche();
                break;
            case -2:
                // Pour ajouter un nouveau mot avant.
                ajouterMotAvant();
                break;
            case -3:
                // Pour ajouter un nouveau mot après.
                ajouterMotApres();
                break;
            case -4:
                // Pour supprimer le mot.
                supprMot();
                break;
            case -5:
                // La forme est en plusieurs mots. Les séparer.
                couperMot();
                break;
            case -6:
                // Le mot aurait un enclitique
                separeEncli();
                break;
            case -9:
                // Pour modifier la référence.
                changerRef();
                break;
            default:
                break;
            }
        }
        else
        {
            _mots[_numMot]->setChoix(n);
        }
        _second->hide();
        afficher(); // Mettre à jour l'affichage de la phrase.
    }
}

/**
 * @brief MainWindow::grec
 *
 * Dans les textes du LASLA, les mots grecs sont transcrits en betacode
 * et délimités par des '$'. Les lettres grecques sont écrites avec
 * l'alphabet latin, mais les accents et les esprits sont codés
 * avec des /\() et quelques autres. Comme j'utilise l'expression
 * rationnelle \b (limite de mots) pour découper le texte en mots,
 * les mots grecs sont séparés en plusieurs morceaux.
 *
 * Si un texte contient un dollar, j'appelle cette routine
 * dont le but est de recoller les mots grecs.
 *
 * J'appelle aussi cette routine si le texte contient
 * un chiffre, que je mets en séparateur.
 *
 */
void MainWindow::grec()
{
    if (_texte.contains(chiffres))
    {
        // Le texte contient un chiffre : il doit passer dans les séparateurs.
        int i = _elements.length() - 2;
        while (i > 0)
        {
            if (_elements[i].contains(chiffres))
            {
                // Si m est un nombre (il suffit qu'il contienne un chiffre), je le saute.
//                qDebug() << _elements[i-1] << _elements[i];
                _elements[i-1] += _elements[i] + _elements[i+1];
                _elements.removeAt(i);
                _elements.removeAt(i);
//                qDebug() << _elements[i-1] << _elements[i];
            }
            i -= 2;
        }
    }
    if (_texte.contains("$"))
    {
        // Le texte contient au moins un mot grec.
        // Il faudrait joindre les morceaux séparés.
        int i = _elements.length() - 1;
        while (i > 0)
        {
            while ((i > 0) && !_elements.at(i).contains("$")) i -= 2; // Les $ sont dans les séparateurs.
            //                    qDebug() << i << _elements[i];
            if (_elements.at(i).contains("$"))
            {
                int j = i - 2;
                while ((j >= 0) && !_elements.at(j).contains(" ") && !_elements.at(j).contains("$"))
                    j -= 2;
                // Pour $a)nexai/$tise$ il faudrait vérifier que le séparateur contient aussi un espace
                // (pour sauter le $ interne). Remplacer && !_elements.at(j).contains("$") par
                // && !(_elements.at(j).contains("$") && _elements.at(j).contains(QRegExp("\\s")))
                if (_elements.at(j).contains("$"))
                {
                    // J'ai un mot grec entre les indices j et i (j < i)
                    QString motGrec = _elements.at(j);
                    // qDebug() << _elements[j] << _elements[j+1] << _elements[j+2];
                    int p = motGrec.indexOf('$');
                    // qDebug() << _elements[j] << _elements[j+1] << _elements[j+2] << p;
                    _elements[j] = motGrec.mid(0,p);
                    motGrec.remove(0,p);
                    for (int k = j + 1; k < i; k++) motGrec += _elements[k];
                    p = _elements[i].indexOf('$') + 1;
                    // qDebug() << _elements[i] << _elements[i+1] << _elements[i+2] << p;
                    motGrec += _elements[i].mid(0,p);
                    _elements[i].remove(0,p);
                    _elements[j+1] = motGrec;
                    for (int k = j + 2; k < i; k++) _elements.removeAt(j+2);
                    // qDebug() << _elements[j] << _elements[j+1] << _elements[j+2] << _elements[j+3];
                }
                else qDebug() << i << j << _elements[i] << _elements[j];
                i = j - 2;
            }
        }
    }
}

void MainWindow::creerNvlFiche()
{
    QString ligne = saisie(_mots[_numMot]->getFTexte());
    Fiche * fiche = new Fiche(ligne);
    _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
    // Il faut parcourir le texte pour ajouter cette analyse à tous les mots identiques.
    for (int i = 0; i < _mots.size(); i++)
        if (_mots[i]->getFTexte() == _mots[_numMot]->getFTexte())
            _mots[i]->ajouteFiche(fiche);
    _mots[_numMot]->setChoix(_mots[_numMot]->cnt() - 1);
    // Pour le mot en cours d'examen, je valide cette dernière fiche.
}

void MainWindow::ajouterMot(QString m,int nm, QString r)
{
    // J'analyse le mot m et je l'insère en position nm, avec la ref r.
    bool e = false;
//    QList<Fiche*> analyses;
    _analyses = _fiches.values(m);
    if (_analyses.isEmpty() && m[0].isUpper())
        _analyses = _fiches.values(m.toLower());
    _analyses.append(appelCollatinus(m,&e));
        // Appel à Collatinus.
    foreach (Fiche * f, _analyses)
    {
        QString c = f->getCmpl();
        if (!c.isEmpty()  && !c.startsWith("§ <")
                && (c.startsWith("§ ") || c.startsWith("<§ ")))
            _analyses.removeOne(f);
    }
    if (_analyses.isEmpty())
    {
        QString ligne = saisie(m);
        Fiche * fiche = new Fiche(ligne);
        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
        _analyses << fiche;
    }
    if (_analyses.size()>1)
        qSort (_analyses.begin(), _analyses.end(), plusFreq);
    Mot *mm = new Mot(m,m,_analyses,-1,r);
    _mots.insert(nm,mm);
//    decimer(nm,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1] + 1);
    // Quand je sépare un mot, j'en ajoute deux et je ne dois décimer qu'à la fin.
    for (int i=_numPhrase - 1; i < _finsPhrase.size(); i++)
        _finsPhrase[i]++;
}

void MainWindow::ajouterMotApres()
{
    // Pour insérer un nouveau mot après le mot examiné.
    QString r = _mots[_numMot]->getRef();
    ficForme2->clear();
    dMot->exec();
    QString m = ficForme2->text();
    if (!m.isEmpty())
    {
        ajouterMot(m,_numMot + 1,r);
        decimer(_numMot + 1,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1]);
    }
}

void MainWindow::ajouterMotAvant()
{
    // Pour insérer un nouveau mot avant ou après le mot examiné.
    QString r = _mots[_numMot]->getRef();
    if (r.isEmpty()) r = _mots[_numMot - 1]->getRef();
    // Si le mot courant est un enclitique, il n'a pas de référence propre :
    // je vais chercher la référence du mot d'appui.
    ficForme2->clear();
    dMot->exec();
    QString m = ficForme2->text();
    if (!m.isEmpty())
    {
        if (QMessageBox::Yes == QMessageBox::question(this,"Avant ou après",
                                  "Voulez-vous insérer \n" + m + " AVANT "
                                  + _mots[_numMot]->getFTexte() + " ?\n (non --> après)"))
        {
            ajouterMot(m,_numMot,r);
            decimer(_numMot,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1]);
        }
        else
        {
            ajouterMot(m,_numMot + 1,r);
            decimer(_numMot + 1,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1]);
        }
    }
}

void MainWindow::supprMot()
{
    // Supprimer le mot courant
    if (QMessageBox::Yes == QMessageBox::question(this,"Confirmer la suppression",
                              "Êtes-vous sûr de vouloir supprimer \n" + _mots[_numMot]->getFTexte() + " ?"))
    {
        _mots.removeAt(_numMot);
        for (int i=_numPhrase - 1; i < _finsPhrase.size(); i++)
            _finsPhrase[i]--;
    }
}

void MainWindow::couperMot()
{
    // Pour couper une forme en plusieurs mots
    QString m = _mots[_numMot]->formeFiche();
    QString r = _mots[_numMot]->getRef();
    supprMot();
    QStringList ecl = m.split(" ");
    int j = 0;
    for (int i = 0;i < ecl.size();i++)
        if (!ecl[i].startsWith("<") || !ecl[i].endsWith(">"))
        {
            ecl[i].remove('<');
            ecl[i].remove('>');
            ajouterMot(ecl[i],_numMot + j,r);
            j++;
        }
    j = 0;
    for (int i = 0;i < ecl.size();i++)
        if (!ecl[i].startsWith("<") || !ecl[i].endsWith(">"))
        {
            // Je ne dois décimer les analyses qu'après avoir ajouté tous les mots.
            decimer(_numMot + j,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1]);
            j++;
        }
}

void MainWindow::separeEncli()
{
    QString m = _mots[_numMot]->getFTexte();
    QList<Fiche*> analyses;
    if (m.endsWith("que") || m.endsWith("cum"))
    {
        analyses = _fiches.values(m.mid(0,m.size()-3));
        if (analyses.isEmpty() && m[0].isUpper())
            analyses = _fiches.values(m.toLower().mid(0,m.size()-3));
    }
    else if (m.endsWith("ue") || m.endsWith("ne"))
    {
        analyses = _fiches.values(m.mid(0,m.size()-2));
        if (analyses.isEmpty() && m[0].isUpper())
            analyses = _fiches.values(m.toLower().mid(0,m.size()-2));
    }
    // Il faut supprimer les analyses à deux mots ou plus.
    foreach (Fiche * f, analyses)
    {
        QString c = f->getCmpl();
        if (!c.isEmpty()  && !c.startsWith("§ <")
                && (c.startsWith("§ ") || c.startsWith("<§ ")))
            analyses.removeOne(f);
    }
    if (!analyses.isEmpty())
    {
        if (analyses.size()>1)
            qSort (analyses.begin(), analyses.end(), plusFreq);
        // Je dois ajouter un nouveau mot.
        QString r = _mots[_numMot]->getRef();
        int rg = _mots[_numMot]->getRang();
        if (m.endsWith("que")) _mots[_numMot] = motQue;
        else if (m.endsWith("cum")) _mots[_numMot] = motCum;
        else if (m.endsWith("ue")) _mots[_numMot] = motVe;
        else if (m.endsWith("ne")) _mots[_numMot] = motNe;
        m = analyses[0]->getForme();
        // Il faut décimer les analyses qui nécessitent des mots qui ne sont pas dans la phrase.
        Mot *mm = new Mot(m,m,analyses,rg,r);
        _mots.insert(_numMot,mm);
        decimer(_numMot,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1] + 1);
        for (int i=0; i < _finsPhrase.size(); i++)
            if (_finsPhrase[i] > _numMot) _finsPhrase[i]++;
    }
}

void MainWindow::decimer(int i, int debPhr, int finPhr)
{
    QStringList mr = _mots[i]->motsRequis();
//    if (_mots[i]->getFTexte() == "ante")
//    qDebug() << debPhr << i << finPhr << _mots[i]->getFTexte() << mr;
    if (!mr.isEmpty())
    {
        // J'ai la liste des mots requis pour _mots[i].
        int j = 0;
        while (j < mr.size())
        {
            QString m = mr[j];
            m.remove("<");
            m.remove(">");
            bool existe = false;
            if (m.startsWith(" "))
            {
                // Le mot doit se trouver après i
                int k = i + 1;
                while ((k < finPhr) && !existe)
                {
                    existe = _mots[k]->contient(m);
                    if (!existe && (m.count(" ") > 1))
                    {
                        QStringList ecl = m.split(" ",QString::SkipEmptyParts);
                        if (_mots[k]->contient(" " + ecl[0]))
                            m.remove(0, ecl[0].size() + 1);
                    }
                    k++;
                }
            }
            else
            {
                // Le mot doit se trouver avant i
                int k = i - 1;
                while ((k >= debPhr) && !existe)
                {
                    existe = _mots[k]->contient(m);
                    if (!existe && (m.count(" ") > 1))
                    {
                        QStringList ecl = m.split(" ",QString::SkipEmptyParts);
                        if (_mots[k]->contient(ecl.last() + " "))
                            m.chop(ecl.last().size() + 1);
                    }
                    k--;
                }
            }
            // Si un mot de cette liste est dans la phrase, je le supprime de la liste.
            if (existe) mr.removeAt(j);
            else j++;
        }
//                        qDebug() << _mots[i]->getFTexte() << mr;
        // S'il reste des mots non-trouvés, je supprime les fiches correspondantes.
        if (!mr.isEmpty())
        {
//            if (_mots[i]->getFTexte() == "ante")
//            qDebug() << debPhr << i << finPhr << _mots[i]->getFTexte() << mr << _mots[i]->listeFiches().size();
            if (mr.size() == _mots[i]->listeFiches().size())
            {
                // Je dois supprimer toutes les fiches car toutes veulent un mot qui n'existe pas
                bool e = false;
                _analyses.clear();
                qDebug() << "Je pensais ne plus pouvoir arriver ici !" << i << _mots[i]->getFTexte();
                QList<Fiche*> na = appelCollatinus(_mots[i]->getFTexte(),&e);
                // J'appelle Collatinus à mon secours.
                qSort (na.begin(), na.end(), plusFreq);
                // Je trie les analyses pour avoir la plus fréquente d'abord.
                if (!na.isEmpty())
                    _mots[i]->ajouteFiches(na);
            }
            _mots[i]->suppr(mr);
        }
    }

}

void MainWindow::changerRef()
{
    // Pour modifier la référence du mot
    QString m = "";
    QString r = _mots[_numMot]->getRef();
    if (!r.isEmpty())
    {
        // Pour les enclitiques, la ref est vide et doit le rester.
        ficForme3->setText(r);
        dRef->exec();
        m = ficForme3->text();
    }
    // Pour bien faire, il faudrait vérifier que le format est bon...
    if (!m.isEmpty() && (m != r) && (m.count('/') > 0))
    {
        // r est l'ancienne ref et m est la nouvelle
        if (r.section("/",0,0) != m.section("/",0,0))
        {
            // La référenciation a changé, je dois l'appliquer à tout le texte
            if (r.left(3) != m.left(3))
                for (int i = _numMot + 1; i < _mots.size(); i++)
                {
                    _mots[i]->setRef(m.left(3) + _mots[i]->getRef().mid(3));
                }
            if (r.mid(4,4) != m.mid(4,4))
            {
                // Le numéro de phrase a changé
                int decal = m.mid(4,4).toInt() - r.mid(4,4).toInt();
                for (int i = _numMot + 1; i < _mots.size(); i++)
                    if (!_mots[i]->getRef().isEmpty())
                {
                    QString rc = _mots[i]->getRef();
                    int nouv = rc.mid(4,4).toInt() + decal + 10000;
                    rc.replace(4,4,QString::number(nouv).mid(1,4));
                    _mots[i]->setRef(rc);
                }
                if ((_numMot > 0) && (decal == 1))
                {
                    // J'ai ajouté une phrase
                    _finsPhrase.insert(_numPhrase - 1,_numMot);
                    if (!_fPhrEl.isEmpty())
                        _fPhrEl.insert(_numPhrase - 1,_mots[_numMot]->getRang() - 1);
                }
                if ((_numMot == _finsPhrase[_numPhrase - 2]) && (decal == -1))
                {
                    // J'ai supprimé une phrase
                    _finsPhrase.removeAt(_numPhrase - 2);
                    if (!_fPhrEl.isEmpty())
                        _fPhrEl.removeAt(_numPhrase - 2);
                    _numPhrase--;
                }
            }
        }
        if (!m.section("/",1,1).isEmpty() && (r.section("/",1,1) != m.section("/",1,1)))
        {
            // Quelque chose a changé dans le repérage du mot
            if (r.section("/",1,1).count(',') == m.section("/",1,1).count(','))
            {
                // Décalage d'un des indices ?
                QString r1 = r.section("/",1,1);
                QString m1 = m.section("/",1,1);
                if (r1.section(",",0,0) != m1.section(",",0,0))
                {
                    // Le premier numéro a changé
                    bool OK = false;
                    int decal = m1.section(",",0,0).toInt(&OK) - r1.section(",",0,0).toInt();
                    if (OK) for (int i = _numMot + 1; i < _mots.size(); i++)
                        if (!_mots[i]->getRef().isEmpty())
                    {
                        QString rc = _mots[i]->getRef();
                        int nouv = rc.section("/",1,1).section(",",0,0).toInt() + decal;
                        QString nrc = rc.section("/",0,0)+ "/" + QString::number(nouv);
                        if (rc.count(',') > 0) nrc += "," + rc.section("/",1,1).section(",",1);
                        if (rc.count('/') == 2) nrc += "/" + rc.section("/",2,2);
                        _mots[i]->setRef(nrc);
                    }
                }
                if ((r1.count(',') > 0) && (r1.section(",",1,1) != m1.section(",",1,1)))
                {
                    // Le 2e numéro a changé
                    bool OK = false;
                    int decal = m1.section(",",1,1).toInt(&OK) - r1.section(",",1,1).toInt();
                    if (OK) for (int i = _numMot + 1; i < _mots.size(); i++)
                        if (!_mots[i]->getRef().isEmpty())
                    {
                        QString rc = _mots[i]->getRef();
                        if (m1.section(",",0,0) == rc.section("/",1,1).section(",",0,0))
                        {
                            // Je ne décale le 2e n° que si le premier n'a pas changé.
                            int nouv = rc.section("/",1,1).section(",",1,1).toInt() + decal;
                            QString nrc = rc.section("/",0,0)+ "/";
                            nrc += m1.section(",",0,0) + ",";
                            nrc += QString::number(nouv);
                            if (rc.count(',') == 2) nrc += "," + rc.section("/",1,1).section(",",2);
                            if (rc.count('/') == 2) nrc += "/" + rc.section("/",2,2);
                            _mots[i]->setRef(nrc);
                        }
                    }
                }
                if ((r1.count(',') > 1) && (r1.section(",",2,2) != m1.section(",",2,2)))
                {
                    // Le 3e numéro a changé
                    bool OK = false;
                    int decal = m1.section(",",2,2).toInt(&OK) - r1.section(",",2,2).toInt();
                    if (OK) for (int i = _numMot + 1; i < _mots.size(); i++)
                        if (!_mots[i]->getRef().isEmpty())
                    {
                        QString rc = _mots[i]->getRef();
                        if (m1.section(",",0,1) == rc.section("/",1,1).section(",",0,1))
                        {
                            // Je ne décale le 3e n° que si les 2 premiers n'ont pas changé.
                            int nouv = rc.section("/",1,1).section(",",2,2).toInt() + decal;
                            QString nrc = rc.section("/",0,0)+ "/";
                            nrc += m1.section(",",0,1) + ",";
                            nrc += QString::number(nouv);
                            if (rc.count('/') == 2) nrc += "/" + rc.section("/",2,2);
                            _mots[i]->setRef(nrc);
                        }
                    }
                }
            }
            else
            {
                // Je ne sais pas encore changer le nombre de champs.
            }
        }
        if (m.count('/') > 1)
        {
            // J'ai un code de subordination
            if (m.section("/",2).size() > 3)
                m.chop(m.section('/',2).size() - 3);
            else while (m.section("/",2).size() < 3) m += " ";
            // Je suis sûr qu'il est sur 3 caractères
        }
        _mots[_numMot]->setRef(m);
    }
}

void MainWindow::fusionPhrase()
{
    // Je supprime une phrase
    // Renuméroter les ref.
    for (int i = _finsPhrase[_numPhrase - 1]; i < _mots.size(); i++)
        if (!_mots[i]->getRef().isEmpty())
    {
        QString rc = _mots[i]->getRef();
        int nouv = rc.mid(4,4).toInt() + 9999;
        rc.replace(4,4,QString::number(nouv).mid(1,4));
        _mots[i]->setRef(rc);
    }
    _finsPhrase.removeAt(_numPhrase - 1);
    if (!_fPhrEl.isEmpty())
        _fPhrEl.removeAt(_numPhrase - 1);
    afficher();
}

void MainWindow::taggage()
{
    QString nomFichier = _repertoire + "/" + _nomFichier+".log";
    bool blabla = false;
    QFile f(nomFichier);
    blabla = f.open(QFile::WriteOnly);
    QStringList aff;
    // J'ouvre un fichier "log"
    QString titre = "Etape n° %1 : j'ajoute le mot %2";
    QString lg = "proba %1 pour %2";
    QString l = "Tag %1 avec %2 occurrences";
    QString lChoix = "Pour le mot n° %1, je peux imposer le tag %2 avec le score %3 (proba %4 vs %5)\n";

    double epsilon = 1.;
    double epsK = epsilon * (_monogrammes.size() - 1);
    double eps2 = 0.1;
    double epsM = eps2 * _fiches.keys().size();
    // Un mot ne peut pas recevoir le tag "snt".

    QStringList sequences;
    QList<double> probabilites;
    sequences.append("snt");
    probabilites.append(1.0);
    double branches = 1.0; // Pour savoir combien de branches a l'arbre.
    qint64 logBr = 0; // Le log en base 2 du précédent.
    qint64 logPr = 0; // Le log en base 2 de la proba finale.
    int iMotAttente = 0; // L'indice du dernier mot en attente de tag.
    int pTagAttente = 4; // Position dans la séquence du tag en attente.
    int iMoins2 = 0; // L'indice du mot i-2.
    int sSeq = 1;
    int sTag = 1;
    // Je suis en début de texte et de phrase : je n'ai que le tag "snt" et une proba de 1.
    int phr = 0;
    int ratio = 1;
    int i = (_mots.size()-1) / 100;
    while (ratio < i) ratio *= 2;
    int j = 1;
    QProgressDialog progr("Tagage en cours...", "Arrêter", 0, (_mots.size() / ratio) + 1, this);
    progr.setWindowModality(Qt::WindowModal);
    progr.setMinimumDuration(1000);
    progr.setValue(0);

    for (i = 0; i < _mots.size(); i++)
    {
        if (j * ratio  < i)
        {
            j = i / ratio + 1;
            if (j >= progr.maximum()) qDebug() << "max atteint" << ratio << _mots.size() << j << i << _mots.size()/ratio;
            else progr.setValue(j);
            if (progr.wasCanceled())
                break;
            //... Stop !
        }
        QStringList nvlSeq; // Nouvelle liste des séquences possibles
        QList<double> nvlProba; // Nouvelle liste des probas.
        Mot *mot = _mots[i];
        aff << titre.arg(i).arg(mot->getFTexte());

        QStringList lTags = mot->tags(); // La liste des tags possibles pour le mot
        // en tenant compte des éventuelles formes conditionnelles.
        QList<int> lOcc = mot->nbOcc();
        // Je dois ajouter tous les tags possibles à toutes les sequences et calculer les nouvelles probas.
        sSeq = sequences.size();
        sTag = lTags.size();
        branches *= sTag;
        double nvlPMax = 0.;
        for (int k = 0; k < sTag; k++)
            aff << l.arg(lTags[k]).arg(lOcc[k]);
        aff << "";
        for (int j = 0; j < sSeq; j++)
        {
            QString bigr = sequences[j].right(7); // Le bigramme terminal
            for (int k = 0; k < sTag; k++)
            {
                QString seq = bigr + "," + lTags[k];
                double p = probabilites[j];
                p *= lOcc[k] + eps2;
                p *= (_trigrammes[seq] + epsilon);
                p /= _monogrammes[lTags[k]] + epsM;
                p /= (_bigrammes[bigr] + epsK);
                if (p > nvlPMax) nvlPMax = p;
                // Je retiens la plus haute valeur des nouvelles probas
                nvlSeq.append(sequences[j] + "," + lTags[k]);
                nvlProba.append(p);
            }
/*            long prTot = 0;
            QList<qint64> pr;
            for (int k = 0; k < sTag; k++)
            {
                QString seq = bigr + "," + lTags[k];
                qint64 p = lOcc[k] * (2 * _trigrammes[seq] + 1);
//                qint64 p = lOcc[k] * (4 * _trigrammes[seq] + 1) * 16384 / _monogrammes[lTags[k]];
                pr << p;
                prTot += p;
            }
            // J'ai tout ce qui dépend de k et la somme pour normaliser.
            if (prTot == 0)
            {
                prTot = 1;
                qDebug() << mot->getFLem() << "proba nulle ! " << sequences[j];
            }
            for (int k = 0; k < sTag; k++)
            {
                nvlSeq.append(sequences[j] + "," + lTags[k]);
                nvlProba.append(probabilites[j] * pr[k] / prTot);
                // Si j'avais gardé toutes les séquences, ce serait une vraie probabilité (normalisée à 1)
            }*/
        }
        // J'ai toutes les séquences de tags en tenant compte du mot n° i.
        if (iMoins2 == i - 2)
        {
            // Je parcours les tags associés au mot i-2 et je garde la meilleure proba.
            int recul = nvlSeq[0].size() - 11;
            if (nvlSeq[0].mid(recul,3) == "snt") recul -= 4;
            for (int k = 0; k < nvlSeq.size(); k++)
                _mots[iMoins2]->setBest(nvlSeq[k].mid(recul,3),nvlProba[k]);
            iMoins2++;
        }
        if (i == _finsPhrase[phr] - 1)
        {
            // Le mot que je viens de considérer est le dernier de la phrase.
            phr++;
            for (int k = 0; k < nvlSeq.size(); k++)
                nvlSeq[k] += ",snt";
            // Toutes les nouvelles séquences sont prolongées par le tag "snt".
            if (iMoins2 == i - 1)
            {
                // Comme j'ai ajouté un "snt", je dois traiter le mot i-1.
                int recul = nvlSeq[0].size() - 11;
                for (int k = 0; k < nvlSeq.size(); k++)
                    _mots[iMoins2]->setBest(nvlSeq[k].mid(recul,3),nvlProba[k]);
                iMoins2++;
            }
        }
        aff << "J'obtiens :";
        for (int k = 0; k < nvlSeq.size(); k++)
            aff << lg.arg(nvlProba[k]).arg(nvlSeq[k].right(59));

        sequences.clear();
        probabilites.clear();
        QString tAtt = "";
        bool tagTrouve = true;
//                qDebug() << mot->forme() << nvlProba << nvlSeq;
        for (int j = 0; j < nvlSeq.size(); j++) if (nvlProba[j] > 0)
        {
            QString bigr = nvlSeq[j].right(7); // Les deux derniers tags
            QString seq = "";
            double val = -1;
            for (int k = j; k < nvlSeq.size(); k += sTag)
                // Pour retrouver le bigramme terminal, il faut au moins le même dernier tag.
                if (bigr == nvlSeq[k].right(7))
                {
                    if (val < nvlProba[k])
                    {
                        // J'y passe au moins une fois au début.
                        val = nvlProba[k];
                        seq = nvlSeq[k];
                    }
                    if (nvlProba[k] > 0)
                        nvlProba[k] = - nvlProba[k]; // Pour ne pas considérer deux fois les mêmes séquences.
                }
            // val et seq correspondent aux proba et séquence avec le bigramme considéré qui ont la plus grande proba.
            sequences << seq;
            probabilites << val;
            if (tAtt == "") tAtt = seq.mid(pTagAttente,3);
            else tagTrouve = tagTrouve && (tAtt == seq.mid(pTagAttente,3));
        }
        while (nvlPMax < 0.0005)
        {
            // Si les probas deviennent petites, je les multiplie par 1024.
            logPr += 10;
            nvlPMax *= 1024.;
            for (int k = 0; k < sequences.size(); k++)
                probabilites[k] *= 1024.;
        }
        aff << "";
        aff << "Après élagage, j'obtiens :";
        for (int k = 0; k < sequences.size(); k++)
            aff << lg.arg(probabilites[k]).arg(sequences[k].right(59));
        aff << "";
        if (sequences.size() < 2)
        {
            aff << "Point fixe !\n";
            if (i > 0)
            {
                // Si le premier mot a un seul tag, ça forme un point fixe inutile.
                /*
                QString seq = sequences[0];
                // Taguer à rebours est devenu inutile.
                QString rebours = seq.right(7);
                double scRebours = retag(&rebours,i);
                if (!seq.endsWith(rebours))
                {
                    QString conflit = "Conflit entre les mots n° %1 et %2 !";
                    aff << conflit.arg(_pointsFixes.last()-1).arg(i);
                    aff << QString::number(scRebours) + " " + rebours;
                    aff << seq.right(rebours.size());
                    aff << "";
                }
                */
                _pointsFixes << i;
            }
            double pr = probabilites[0];
            double br = branches;
            while (pr < 1)
            {
                pr *= 2;
                logPr++;
            }
            while (br > 1)
            {
                br /= 2;
                logBr++;
            }
            probabilites[0] = 1.0;
            branches = 1.0;
            // À chaque point fixe, je peux remettre les compteurs à zéro.
        }

        while (tagTrouve && (pTagAttente < sequences[0].size()))
        {
            // Je n'ai plus qu'une possibilité pour le tag du mot iMotAttente
            double pMax = 0;
            double pPrime = 0;
            QStringList ns;
            QList<double> np;
            // Les nvlProba sont négatives
            for (int k = 0; k < nvlSeq.size(); k++)
            {
                if (tAtt == nvlSeq[k].mid(pTagAttente,3))
                {
                    if (nvlProba[k] < pMax) pMax = nvlProba[k];
                    ns << nvlSeq[k];
                    np << nvlProba[k];
                }
                else if (nvlProba[k] < pPrime) pPrime = nvlProba[k];
            }
            double score = (pMax - pPrime) / (pPrime + pMax);
            pMax = _mots[iMotAttente]->getBest(-1);
            pPrime = _mots[iMotAttente]->getBest(tAtt);
            if (pMax == pPrime)
            {
                // Le tag choisi est le plus probable. Je cherche le suivant.
                pMax = 0;
                Mot * m = _mots[iMotAttente];
                for (int k = 0; k< m->tags().size();k++)
                    if ((m->getBest(k) != pPrime) && (m->getBest(k) > pMax))
                            pMax = m->getBest(k);
            }
            aff << lChoix.arg(iMotAttente).arg(tAtt).arg(score)
                   .arg(pPrime).arg(pMax);
            _mots[iMotAttente]->setChoix(tAtt,score);
            // Le mot en attente voit son ambiguïté résolue.
            nvlSeq = ns;
            nvlProba = np;
            ns.clear();
            np.clear();
            // Pour traiter éventuellement les mot suivants, je filtre les nouvelles séquences.
            // Ça ne change pas les tags possibles, mais seulement les scores.
            iMotAttente++;
            pTagAttente += 4;
            while ((pTagAttente < sequences[0].size()) && (sequences[0].mid(pTagAttente,3) == "snt")) pTagAttente += 4;
            tAtt = sequences[0].mid(pTagAttente,3);
            for (int k = 0; k < sequences.size(); k++)
                tagTrouve = tagTrouve && (tAtt == sequences[k].mid(pTagAttente,3));
        }

        aff << "";
        if (blabla) f.write(aff.join("\n").toUtf8());
        aff.clear();
    } // Fin du texte
//    progr.setValue(progr.maximum()+1);
    j = 1;
    progr.setLabelText("Finalisation...");
    progr.reset();
    progr.setValue(0);
    // La "bonne séquence" de tags est celle qui a la plus grande proba.
    QString seq = "";
    double val = -1;
    for (int k = 0; k < sequences.size(); k++)
        if (val < probabilites[k] * (2 * _trigrammes[sequences[k].right(7)] + 1))
        {
            // J'y passe au moins une fois au début.
            val = probabilites[k] * (2 * _trigrammes[sequences[k].right(7)] + 1);
            seq = sequences[k];
        }
    aff << "";
    aff << "J'ai fini !";
    aff << lg.arg(logPr).arg(logBr) + " branches (en log en base 2).";
    aff << "";
    aff << seq;
    if (blabla) f.write(aff.join("\n").toUtf8());
    f.close();
    // Il me reste à reporter dans les derniers mots le tag retenu.
    for (int i = iMotAttente; i < _mots.size(); i++)
    {
        if (j * ratio  < i)
        {
            j = i / ratio + 1;
            if (j >= progr.maximum()) qDebug() << "max atteint" << ratio << _mots.size() << j << i << _mots.size()/ratio;
            else progr.setValue(j);
            if (progr.wasCanceled())
                break;
            //... Stop !
        }
        QString t = seq.mid(pTagAttente,3);
        _mots[i]->setChoix(t,1);
        pTagAttente += 4;
        while ((pTagAttente < seq.size()) && (seq.mid(pTagAttente,3) == "snt")) pTagAttente += 4;
    }
    progr.setValue(progr.maximum()+1);
}

/**
 * @brief MainWindow::retag
 * @param bigr : le bigramme terminal
 * @param iFixe : l'indice du mot qui donne le point fixe
 * @return la séquence obtenue à rebours
 */
double MainWindow::retag(QString * bitag, int iFixe)
{
    QStringList sequences;
    QList<double> probabilites;
    sequences.append(*bitag);
    probabilites.append(1.0);

    int sSeq = 1;
    int sTag = 1;
    int i = iFixe - 1;
    if (bitag->contains("snt")) i++;
    while (i > 0)
    {
        i--;
        QStringList nvlSeq; // Nouvelle liste des séquences possibles
        QList<double> nvlProba; // Nouvelle liste des probas.
        Mot *mot = _mots[i];
//        aff << titre.arg(i).arg(mot->getFTexte());

        QStringList lTags = mot->tags(); // La liste des tags possibles pour le mot
        // en tenant compte des éventuelles formes conditionnelles.
        QList<int> lOcc = mot->nbOcc();
        // Je dois ajouter tous les tags possibles à toutes les sequences et calculer les nouvelles probas.
        sSeq = sequences.size();
        sTag = lTags.size();
        for (int j = 0; j < sSeq; j++)
        {
            QString bigr = sequences[j].left(7); // Le bigramme terminal
            long prTot = 0;
            QList<qint64> pr;
            for (int k = 0; k < sTag; k++)
            {
                QString seq = lTags[k] + "," + bigr;
                qint64 p = lOcc[k] * (2 * _trigrammes[seq] + 1);
//                qint64 p = lOcc[k] * (4 * _trigrammes[seq] + 1) * 16384 / _monogrammes[lTags[k]];
                pr << p;
                prTot += p;
            }
            // J'ai tout ce qui dépend de k et la somme pour normaliser.
            if (prTot == 0)
            {
                prTot = 1;
                qDebug() << mot->getFLem() << "proba nulle ! " << sequences[j];
            }
            for (int k = 0; k < sTag; k++)
            {
                nvlSeq.append(lTags[k] + "," + sequences[j]);
                nvlProba.append(probabilites[j] * pr[k] / prTot);
                // Si j'avais gardé toutes les séquences, ce serait une vraie probabilité (normalisée à 1)
            }
        }
        // J'ai toutes les séquences de tags en tenant compte du mot n° i.
        if (_finsPhrase.contains(i))
        {
            // Le mot que je viens de considérer est le premier de la phrase.
            for (int k = 0; k < nvlSeq.size(); k++)
                nvlSeq[k].prepend("snt,");
            // Toutes les nouvelles séquences sont prolongées par le tag "snt".
        }

        sequences.clear();
        probabilites.clear();
        for (int j = 0; j < nvlSeq.size(); j++) if (nvlProba[j] > 0)
        {
            QString bigr = nvlSeq[j].left(7); // Les deux derniers tags
            QString seq = "";
            double val = -1;
            for (int k = j; k < nvlSeq.size(); k += sTag)
                // Pour retrouver le bigramme terminal, il faut au moins le même dernier tag.
                if (bigr == nvlSeq[k].left(7))
                {
                    if (val < nvlProba[k])
                    {
                        // J'y passe au moins une fois au début.
                        val = nvlProba[k];
                        seq = nvlSeq[k];
                    }
                    if (nvlProba[k] > 0)
                        nvlProba[k] = - nvlProba[k]; // Pour ne pas considérer deux fois les mêmes séquences.
                }
            // val et seq correspondent aux proba et séquence avec le bigramme considéré qui ont la plus grande proba.
            sequences << seq;
            probabilites << val;
        }
        if (sequences.size() < 2)
        {
            *bitag = sequences[0];
            double pMax = 0;
            double pPrime = 0;
            // Les nvlProba sont négatives.
            // Je dois garder les deux plus grandes (en valeur absolue)
            for (int k = 0; k < nvlSeq.size(); k++)
            {
                if (nvlProba[k] < pPrime)
                {
                    // Proba plus grande que la 2e.
                    if (nvlProba[k] < pMax)
                    {
                        // Proba plus grande que la 1ère
                        pPrime = pMax;
                        // La 1ère devient la 2e
                        pMax = nvlProba[k];
                    }
                    else pPrime = nvlProba[k];
                }
                qDebug() << i << iFixe << nvlSeq[k] << nvlProba[k] << pPrime << pMax;
            }
            double score = (pMax - pPrime) / (pPrime + pMax);
            return score;
        }
        // J'ai atteint le point fixe précédent.
    } // Fin du texte
    // La "bonne séquence" de tags est celle qui a la plus grande proba.
    QString seq = "";
    double pMax = 0;
    double pPrime = 0;
    for (int k = 0; k < sequences.size(); k++)
    {
        sequences[k].prepend("snt,");
        double pr = probabilites[k] * (2 * _trigrammes[sequences[k].left(11)] + 1);
        pr *= 2 * _trigrammes[sequences[k].left(7)] + 1;
        if (pr > pPrime)
        {
            // Proba plus grande que la 2e.
            if (pr > pMax)
            {
                // Proba plus grande que la 1ère
                pPrime = pMax;
                // La 1ère devient la 2e
                pMax = pr;
                seq = sequences[k];
            }
            else pPrime = pr;
        }
    }
    *bitag = seq;
    double score = (pMax - pPrime) / (pPrime + pMax);
    return score;
}

QList<Fiche*> MainWindow::appelCollatinus(QString m, bool* enclit)
{
    QList<Fiche*> analyses;
    // Appel à Collatinus : il faudrait qu'il réponde avec les codes en 9
    /* Je ne passe plus par le serveur
    QString req = "-K9 " + m;
    QTcpSocket * tcpSocket = new QTcpSocket();
    tcpSocket->abort();
    tcpSocket->connectToHost(QHostAddress::LocalHost, 5555);
    QByteArray ba = req.toUtf8();
    if (tcpSocket->waitForConnected(1000))
    {
        tcpSocket->write(ba);
        tcpSocket->waitForBytesWritten();
        tcpSocket->waitForReadyRead();
        ba = tcpSocket->readAll();
    }
    else
    {
        ba.clear();
        statusBar()->showMessage("Le serveur de Collatinus ne répond pas...");
    }
    tcpSocket->disconnectFromHost();
    tcpSocket->close();
    QString rep(ba);
    */
    QString rep = _lasla->k9(m);
    if (rep.size() > 1)
    {
//        if (rep.contains("-")) rep.replace("-","A68      ");
        if (rep.contains("-")) rep.replace("-","k9       ");
        // Les indéclinables ont une morpho "-" et pas de code en 9
        QFile fDic(dicoPerso);
        fDic.open(QIODevice::Append|QIODevice::Text);
        QStringList listRep = rep.split("\n");
        listRep.removeDuplicates();
        // Les différents genres pour les adjectifs donnent la même fiche
        foreach (QString lg, listRep)
        {
            // Chaque ligne est une analyse avec "forme,clef,espace,code"
            // Est-ce que la clef Collatinus a un équivalent LASLA ?
            QStringList eclats = lg.split(",");
            eclats[1].replace("v","u");
            eclats[1].replace("j","i");
            eclats[1].replace("J","I");
            eclats[1].replace("V","U");
            // J'avais adopté des notations non-Ramistes.
            eclats[0].replace("v","u");
            eclats[0].replace("j","i");
            eclats[0].replace("J","I");
            eclats[0].replace("U","V");
            // Les formes sont non-Ramistes, mais avec des "V" !
            if (eclats[3].startsWith("B") && eclats[3].endsWith(" 5   "))
            {
                // Le bon code pour l'adjectif verbal est 502 ou 503 (déponent)
                if (eclats[1].endsWith("r")) eclats[3].replace(" 5   "," 503 ");
                else eclats[3].replace(" 5   "," 502 ");
            }
            if (_corresp.contains(eclats[1]))
            {
                // La clef Collatinus a un équivalent au LASLA.
                QString clef = _corresp.value(eclats[1]).trimmed();
//                                clef.replace("U","V");
                if (clef.contains(" ")) clef.replace(" ",",");
                else clef.append(", ");
                // J'ai séparé l'indice d'homonymie
                clef.append("," + lg.section(",",3));
                clef.prepend(eclats[0] + ",");
                // "forme,clef,espace,code" --> "forme,lemme,indice,code"
                lg = clef;
            }
            else if (lg.section(",",1).startsWith("U"))
            {
                // "U" doit devenir "V".
                QString finLg = lg.section(",",1);
                finLg[0] = 'V';
                lg = eclats[0] + "," + finLg;
            }
            else lg = eclats[0] + "," + lg.section(",",1);
            // Vérifier s'il y a un enclitique !
            if (lg.contains("<"))
            {
                if (lg.contains("<st>"))
                {
                    // Cas particulier du "st"
                    // Je dois ajouter <blabla>st, mais comme SVM 1 ou SVM 2 ?
                    // 3/4 sont des SVM 1 parmi ceux trouvés
                    QString ligne = lg.section(">",0,0); // blabla<st
                    ligne.replace("<",">"); // blabla>st
                    QString fst = "<" + ligne;
                    ligne = fst + ",SVM,1,B6 1 1113,B11,2";
                    // <blabla>st,SVM,1,B6 1 1113,B11,1
                    Fiche * fiche = new Fiche(ligne);
                    if (!_fiches.keys().contains(fiche->getClef()))
                    {
                        // n'ajouter qu'une fois la forme <blabla>st
                        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
                        analyses.append(fiche);
                        ligne.append("\n");
                        fDic.write(ligne.toUtf8());
                        ligne = fst + ",SVM,2,#        ,#  ,1";
                        fiche = new Fiche(ligne);
                        _fiches.insert(fiche->getClef(),fiche);
                        analyses.append(fiche);
                        ligne.append("\n");
                        fDic.write(ligne.toUtf8());
                    }
                    // lg est l'analyse de blabla<st> et le reste
                }
                else
                {
                    *enclit = true;
                    lg = lg.section("<",0,0) + lg.section(">",1);
                }
            }
            QString t = "," + tag(lg.section(",",-1));
            QString ligne = lg + t + ",1";
            Fiche * fiche = new Fiche(ligne);
            // Avant, quand j'arrivais ici, c'était que le mot n'était pas connu.
            // Ce n'est plus le cas maintenant !
            // Avant d'ajouter la nouvelle fiche, je dois vérifier qu'elle n'a pas déjà été trouvée.
            bool nvle = true;
            if (fiche->getCode().startsWith("k9")) nvle = _analyses.isEmpty();
                // Si la conversion en code en 9 est incomplète et qu'il y a des candidats,
                // je laisse tomber cette nouvelle analyse.
            else foreach (Fiche* f, _analyses)
                {
                    // Je dois regarder si les fiches f et fiche décrivent la même solution
                    bool egales = (f->getLemme() == fiche->getLemme());
                    egales = egales && (f->getIndice() == fiche->getIndice());
                    egales = egales && (f->getCode() == fiche->getCode());
                    nvle = nvle && !egales;
                }
            if (nvle)
            {
                analyses.append(fiche);
                _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
                ligne.append("\n");
                fDic.write(ligne.toUtf8());
            }
        }
        fDic.close();
    }
    return analyses;
}

QList<Fiche*> MainWindow::getAnalyses(QString forme)
{
    QList<Fiche*> analyses;
    QString cle = Fiche::clef(forme);
    if (cle.contains("/") && !cle.startsWith("$"))
    {
        // Clef ambiguë
        analyses = _fiches.values(cle.section("/",0,0));
        analyses.append(_fiches.values(cle.section("/",1,1)));
        // Je prends les deux valeurs possibles.
    }
    else analyses = _fiches.values(cle);
    return analyses;
}
