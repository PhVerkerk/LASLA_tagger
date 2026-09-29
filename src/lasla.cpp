/*               lasla.cpp
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

#include "lasla.h"

#define VERIF
#define VERBEUX
/*
 * Je définis deux variables pour autoriser certains messages :
 * VERIF envoie sur le Terminal (d'où a été lancé le daemon)
 * des éléments que je souhaite vérifier lors du développement.
 * VERBEUX crée le fichier *.log qui décrit tous les faits et gestes
 * du tagueur pour permettre une vérification de toutes les étapes.
 *
 * À terme, ces deux lignes devront être commentées pour ne pas
 * polluer le serveur avec des éléments inutiles.
 *
 * Se pose la question du fichier *.ZPN et du *_00.APN.
 */
//#define LEGACY
/*
 * Je définis ou pas cette variable LEGACY
 * pour choisir si le programme exécute
 * l'ancienne ou la nouvelle version de taggage().
 */

#define AVEC_DICO
/* Le 17/02/2025
 * Pour pouvoir désactiver partout l'utilisation du dico perso
 * inutile dans la version daemon.
 * */

//#define FIX_GENRE
/* Le 28/01/2025
 *
 * Avec le fichier Coll_genre.csv préparé dans lireDonnees (il y a une dizaine de jours)
 * et corrigé par des Latinistes (pas encore à ma connaissance),
 * je connais le genre des substantifs.
 *
 * Le fichier des formes extraites (il y a presque 10 ans)
 * des textes analysés avec le code en 5
 * donne le genre des adjectifs et assimilés.
 *
 * Ces genres doivent maintenant être intégrés dans les fichiers lus au démarrage.
 * Pour ce faire, je vais devoir mettre plusieurs morceaux de code à divers endroits.
 * Je vais les encadrer entre des #ifdef FIX_GENRE et leur #endif.
 * Il suffira de commenter ce #define pour les faire tous disparaître.
 * */


#define MULTIPL 1
// Coeff pour les tri et bigrammes. Valait 1 dans l'ancien calcul.
// J'ai trouvé un optimum pour la valeur de 0.7 mais qui ralentit
// l'exécution : je reviens à 1, entier.
#define MuLTIPL1 15
// Coeff pour les formes et monogrammes. Valait 10 dans l'ancien calcul.

/**
 * @brief Créateur de la classe Lasla.
 * @param parent : pointeur vers le parent.
 * @param l : pointeur vers un lemmatiseur.
 * @param resDir : chemin complet du répertoire de données.
 *
 * Ce module utilise le lemmatiseur de Collatinus.
 * Il a pour fonction spécifique de donner la lemmatisation
 * et le code en 9 d'une forme.
 * Il ne sert donc que pour les
 * applications avec une coloration LASLA.
 *
 * Si le lemmatiseur est déjà créé par ailleurs,
 * il suffit de passer le pointeur en question.
 * Si le pointeur n'est pas donné,
 * le lemmatiseur sera créé ici.
 * Si l'application envisagée utilise plusieurs modules
 * intermédiaires (tagueur, scandeur...),
 * il vaut mieux créer un seul lemmatiseur commun.
 *
 * Pour déterminer la catégorie LASLA à un lemme,
 * le programme utilise le modèle qui lui est associé.
 * Le fichier CatLASLA donne les correspondances.
 * Si la correspondances n'est pas trouvée, le code
 * en 9 commencera par "k9" en guise de catégorie
 * et sous-catégorie.
 */
Lasla::Lasla(QObject *parent, LemCore *l, QString resDir) : QObject(parent)
{
    if (l==0)
    {
        _lemCore = new LemCore(this, resDir);
        // Je crée le lemmatiseur...
        _lemCore->setExtension(true);
        // ... et charge l'extension du lexique.
        _lemCore->setCible("k9,fr");
    }
    else _lemCore = l;
//    qDebug() << "lemcore";
    if (resDir == "")
        _resDir = qApp->applicationDirPath() + "/data/";
    else if (resDir.endsWith("/")) _resDir = resDir;
    else _resDir = resDir + "/";
    lireDonnees();
//    qDebug() << "lireDonnes";
    lisCat();
//    qDebug() << "lisCat";
    _medieval = false; // A priori, graphie classique.
    _preProc = false; // A priori, pas de préprocessing.
    _prose = true; // A priori, le texte est en prose.
    _primoL = true;
    _primoA = true;
    _date = "!!!!!\n";
    formatRef = "%1";
    _CR1 = true;
    _CR2 = false;
    _CR3 = false;
    _CR4 = false;
}

/**
 * @brief Lecture des correspondances Collatinus-LASLA
 *
 * Lecture des correspondances entre les modèles de Collatinus
 * et les catégories et sous-catégories du LASLA.
 * Les correspondances sont listées dans le fichier CatLASLA.txt
 * qu'il faut lire et sont rangées dans la QMap Lasla::_catLasla.
 *
 */
void Lasla::lisCat()
{
    QStringList lignes = _lemCore->lignesFichier(_resDir + "CatLASLA.txt");
//    qDebug() << lignes.size();
    foreach (QString lin, lignes)
        if (lin.contains(","))
        {
            QString code = lin.section(",",1,1);
            if (code.size() < 2) code.append(" ");
            if (code.size() > 2) code = code.left(2);
            _catLasla.insert(lin.section(",",0,0), code);
        }
}

/**
 * @brief Lemmatisation avec le code LASLA
 * @param m : une forme
 * @return le résultat de la lemmatisation avec le code en 9.
 *
 * Le résultat se présente comme une chaine de type CSV.
 * Chaque ligne est une des possibilité d'analyse et
 * est composée de quatre champs séparés par une virgule.
 * Le premier champ donne la forme et le deuxième la clef du lemme,
 * tous les deux sans quantité.
 * Le 3e champ reste vide, il est fait pour contenir l'indice
 * d'homonymie du LASLA. Ici, l'éventuel indice est collé
 * au lemme, sous la forme d'un chiffre.
 * Le lemme de Collatinus est en minuscule (sauf l'initiale),
 * ce qui le distingue des lemmes du LASLA, toujours en majuscules.
 * Le 4e champ done l'analyse sous la forme d'un code en 9.
 *
 * Cette version du code est obsolète.
 * Elle date en fait du temps où Collatinus était appelé
 * de temps en temps seulement et à travers le serveur.
 * En particulier elle joint les analyses (de QStringList à QString)
 * alors que l'appelant fait le split.
 * Je la déplace dans appelCollatinus.
 */
QString Lasla::k9(QString m)
{
//    qDebug() << m;
    QStringList res;
//    QString cibAct = _lemCore->cible();
//    _lemCore->setCible("k9,fr");
    // C'était utile quand je passais par le serveur en arrière-plan.
    MapLem mm = _lemCore->lemmatiseM(m);
    if (mm.isEmpty())
    {
        if (m.startsWith("V"))
        {
            m.replace("V","U");
            mm = _lemCore->lemmatiseM(m);
        }
        if (mm.isEmpty()) return "\n";
        // Il faut répondre quelque chose, sinon j'attends 30 secondes !
    }
    foreach (Lemme *l, mm.keys())
    {
        QString clef = l->cle() + ", ,";
        foreach (SLem s, mm.value(l))
        {
            QString code9 = _lemCore->morpho(s.morpho);
            QString forme = Ch::atone(s.grq);
            if (!s.sufq.isEmpty()) forme += "<" + Ch::atone(s.sufq) +">,";
            else forme += ",";
            if (_catLasla.contains(l->modele()->gr())) code9.replace("k9",_catLasla[l->modele()->gr()]);
//            qDebug() << clef << s.morpho << code9 << _catLasla[l->modele()->gr()];
            if ((code9[0] != 'C') && (code9[0] != 'M')) code9[4] = ' ';
            // Seuls les adjectifs et les adverbes peuvent avoir un degré.
            res << forme + clef + code9;
        }
    }

//    _lemCore->setCible(cibAct);
    return res.join("\n");
}

bool Lasla::optionMed()
{
    return _medieval;
}
void Lasla::graphMed(bool activees)
{
    // Fonction relais pour activer et désactiver la gestion des graphies médiévales.
    _lemCore->setMedieval(activees);
    _medieval = activees;
    if (activees && _ficMed.isEmpty())
    {
        // C'est la première fois que j'active les graphies médiévales.
        QStringList liste = _fiches.keys();
        liste.removeDuplicates();
        QString cleMed;
        foreach (QString clef, liste)
        {
            cleMed = transfMed(clef);
            if (clef != cleMed) _ficMed.insert(cleMed, clef);
        }
    }
}

QString Lasla::transfMed(QString f, bool rad)
{
    // Fonction relais pour la transformation des graphies médiévales.
    return _lemCore->transfMed(f, rad);
}


QString Lasla::traite(QString requete)
{
//    QString reponse = "Nb de caractères : %1\n";

    requete = requete.trimmed();
    if (requete.isEmpty() || !requete.contains(".txt"))
    {
        if (requete.contains(".csv"))
        {
            // J'ai un nom de fichier en .csv :
            // je dois procéder à une réconciliation.
            QString debut = requete.mid(0,requete.indexOf(".txt"));
            QString nom;
            // Je coupe l'extension !
            if (requete.contains(".csv\""))
                nom = debut.section("\"", -1);
            else nom = debut.section(" ", -1);
            _warnings = "";
//            return CVS2APN(nom);
            // La routine CSV2APN retourne les warnings.
            return _warnings;
        }
        else return "Il me faut au moins un nom de fichier !";
    // Un nom de fichier avec une extension .txt est requis.
    }

    QString debut = requete.mid(0,requete.indexOf(".txt"));
    // Je coupe l'extension !
    if (requete.contains(".txt\""))
        _nomFichier = debut.section("\"", -1);
    else _nomFichier = debut.section(" ", -1);
    // A priori, il n'y a pas de blanc dans un nom de fichier.
    // Toutefois, il peut y en avoir dans le chemin, auquel cas
    // il doit être encadré par des guillemets par exemple :
    // "/Users/Philippe/vieux textes/blabla.txt"
    // Bien qu'il soit préférable d'avoir :
    // /Users/Philippe/vieux_textes/blabla.txt avec ou sans guillemets.

    QFile f(_nomFichier + ".txt");
    if (!f.open(QFile::ReadOnly))
        return "Fichier introuvable " + _nomFichier + "\n";
    _texte = f.readAll();
    f.close();
    // Si le fichier existe, je le charge.
//    reponse = reponse.arg(_texte.size());

    requete.remove(_nomFichier + ".txt");
    requete.remove("\"\"");
    requete = requete.trimmed();
    // Le nom de fichier ayant été retiré de la requête,
    // je dois voir s'il y a des options.

    // Je donne des valeurs par défaut aux diverses variables
    _refOeuvre = "Zxz";
    _decalPhr = 10001; // Par défaut, la première phrase porte le n° 1.
    _CR1 = true; // Je compte les sauts de ligne simples.
    _CR2 = false; // Et j'ignore les autres.
    _CR3 = false;
    _CR4 = false;
    numLg = 1;
    numPar = 1;
    numCh = 1;
    numLvr = 1;

    if (requete.contains("-M") || requete.contains("-m"))
    {
        graphMed(true);
/*        _medieval = true;
        if (_ficMed.isEmpty())
        {
            // C'est la première fois que j'active les graphies médiévales.
            QStringList liste = _fiches.keys();
            liste.removeDuplicates();
            QString cleMed;
            foreach (QString clef, liste)
            {
                cleMed = transfMed(clef);
                if (clef != cleMed) _ficMed.insert(cleMed, clef);
            }
        }*/
    }
    else
    {
        graphMed(false);
//        _medieval = false;
    }

    if (requete.contains("-N"))
    {
        // La requête contient un code pour l'œuvre,
        // éventuellement suivi par le n° de la première phrase.
        QString option = requete.section("-N",1).section("-",0,0).trimmed();
        if (option.startsWith(" ")) option = option.mid(1); // le trimmed l'a probablement éliminé.
        // Je deviens tolérant : on peut écrire "-NBla" ou "-N Bla"

        if (option.contains("_"))
        {
            _refOeuvre = option.section("_",0,0);
            QString num = option.section("_",1,1);
            bool OK;
            int xx = num.toInt(&OK);
            if (OK && (xx < 10000)) _decalPhr = 10000 + xx;
            // Le 10000 est une astuce pour avoir le nombre avec les zéros devant.
        }
        else _refOeuvre = option;
        if (_refOeuvre.isEmpty()) _refOeuvre = "Zxz";
        else if (_refOeuvre.size() > 3) _refOeuvre = _refOeuvre.mid(0,3);
        else while (_refOeuvre.size() < 3) _refOeuvre.append(" ");
        // La variable _refOeuvre a ainsi trois caractères exactement.
    }

    if (requete.contains("-f"))
    {
        // La requête contient une indication de format
        QString option = requete.section("-f",1,1).section(" ",0,0).trimmed();
        if (!option.isEmpty())
        {
            QStringList el = option.split("_");
            bool OK;
            int xx = el[0].toInt(&OK);
            if (OK)
            {
                if (xx > 0) numLg = xx; // Par défaut, je compte les lignes.
                else _CR1 = false; // Un 0 ou un nombre négatif désactive le comptage.
            }
            if (el.size() > 1)
            {
                xx = el[1].toInt(&OK);
                if (OK)
                {
                    if (xx > 0)
                    {
                        numPar = xx; // Je compte les doubles sauts.
                        _CR2 = true;
                    }
                }
                else _CR2 = true;
                // Un non-nombre est considéré comme un 1.
            }
            if (el.size() > 2)
            {
                xx = el[2].toInt(&OK);
                if (OK)
                {
                    if (xx > 0)
                    {
                        numCh = xx; // Je compte les triples sauts.
                        _CR3 = true;
                    }
                }
                else _CR3 = true;
                // Un non-nombre est considéré comme un 1.
            }
            if (el.size() > 3)
            {
                xx = el[3].toInt(&OK);
                if (OK)
                {
                    if (xx > 0)
                    {
                        numLvr = xx; // Je compte les quadruples sauts.
                        _CR4 = true;
                    }
                }
                else _CR4 = true;
                // Un non-nombre est considéré comme un 1.
            }
            if (!_CR1 && !_CR2 && !_CR3 && !_CR4) _CR1 = true;
            // Je n'ai pas le droit de ne rien compter...
            if (_CR1 && _CR2 && _CR3 && _CR4) _CR1 = false;
            // Trois champs maximum.
        }
    }

    if (requete.contains("-F"))
    {
        // La requête contient une indication de format
        QString option = requete.section("-F",1,1).section(" ",0,0).trimmed();
        _CR1 = false;
        _CR2 = true;
        if (!option.isEmpty())
        {
            QStringList el = option.split("_");
            bool OK;
            int xx = el[0].toInt(&OK);
            if (OK)
            {
                if (xx > 0) numPar = xx; // Par défaut, je compte les paragraphes.
                else _CR2 = false; // Un 0 ou un nombre négatif désactive le comptage.
            }
            if (el.size() > 1)
            {
                xx = el[1].toInt(&OK);
                if (OK)
                {
                    if (xx > 0)
                    {
                        numCh = xx; // Je compte les doubles sauts.
                        _CR3 = true;
                    }
                }
                else _CR3 = true;
                // Un non-nombre est considéré comme un 1.
            }
            if (el.size() > 2)
            {
                xx = el[2].toInt(&OK);
                if (OK)
                {
                    if (xx > 0)
                    {
                        numLvr = xx; // Je compte les triples sauts.
                        _CR4 = true;
                    }
                }
                else _CR4 = true;
                // Un non-nombre est considéré comme un 1.
            }
            if (!_CR2 && !_CR3 && !_CR4) _CR2 = true;
            // Je n'ai pas le droit de ne rien compter...
        }
    }

    // Définir le format pour les références.
    // Ce qui compte ici, c'est le nombre de champs.
    int nChamps = 0;
    if (_CR1) nChamps++;
    if (_CR2) nChamps++;
    if (_CR3) nChamps++;
    if (_CR4) nChamps++;
    switch (nChamps) {
    case 3:
        formatRef = "%1,%2,%3";
        break;
    case 2:
        formatRef = "%1,%2";
        break;
    case 1:
        formatRef = "%1";
        break;
    }
//    qDebug() << formatRef;

    QString reponse = nouveau();
//    Essai d'appel à simple_pred et CVS2APN

    // Je peux maintenant appeler simpl_pred...

    QFile fic("fichiers.txt");
    if (fic.open(QFile::WriteOnly | QFile::Text))
    {
        QString toto = _nomFichier;
        toto.replace(".txt",".csv");
        fic.write(toto.toUtf8());
        fic.close();

        QProcess *proc = new QProcess(this);
        proc->setWorkingDirectory(qApp->applicationDirPath());
        proc->setProcessChannelMode(QProcess::MergedChannels);
//        proc->start("simple_pred.exe");
        proc->start("python3 scripts/simple_pred.py");
        proc->waitForFinished(-1);
        QString blabla = proc->readAll();

        if (proc->exitCode())
        {
            reponse.append("\n Pred error " + QString::number(proc->exitCode()) + "\n");
        }
//                delete proc;

        /* Pour l'instant, j'appelle CSV2APN comme process externe.
         * C'est assez absurde, car toutes les données sont
         * en mémoire et il est idiot d'aller les relire.
         * Toutefois, comme le programme existait,
         * il m'a suffi de le copier au bon endroit...
         *
         * */

//        proc->start("CSV2APN.exe");
        proc->start("./CSV2APN");
        proc->waitForFinished(-1);
        blabla = proc->readAll();

        toto.replace(".csv","_W.txt");
        fic.setFileName(toto);
        if (fic.open(QFile::WriteOnly | QFile::Text))
        {
            fic.write(blabla.toUtf8());
            fic.close();
        }

        if (proc->exitCode())
        {
            reponse.append("\n CSV2APN error " + QString::number(proc->exitCode()) + "\n");
        }
        delete proc;

    }

    return reponse;
}

void Lasla::lireDonnees()
{
    QString prefixe = qApp->applicationDirPath() + "/data/";
    dicoPerso = prefixe + "perso.csv";
    dicoAuto = prefixe + "auto.csv";
    QList<Fiche*> fCherche;
    QString ligne;
    Fiche * fiche;

#ifdef FIX_GENRE
    QList<Fiche*> listForm9;
    QList<Fiche*> listComp9;
    // Lire la liste des lemmes genrés par Collatinus.
    QMap<QString,QString> lemGenr;
    QFile fic (prefixe + "Coll_genre.csv");
    fic.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream flux (&fic);
    while (!flux.atEnd ())
    {
        ligne = flux.readLine ();
        if (ligne.startsWith("!")) continue;
        if (!ligne.section(",",2,2).trimmed().isEmpty())
        {
            // J'ai un genre
            QString gHumain = ligne.section(",",2,2);
            QString gLASLA;
            // 1 = comm. ; 2 = f. ; 3 = m./f. ; 4 = m. ; 5 = m./n. ; 6 = n.
            if (gHumain.contains("féminin"))
            {
                if (gHumain.contains("neutre")) gLASLA = "1";
                else if (gHumain.contains("masculin")) gLASLA = "3";
                else gLASLA = "2";
            }
            else if (gHumain.contains("neutre"))
            {
                if (gHumain.contains("masculin")) gLASLA = "5";
                else gLASLA = "6";
            }
            else gLASLA = "4";
            QString l = ligne.section(",",1,1); // le lemme LASLA avec son indice.
            lemGenr.insert(l,gLASLA);
        }
    }
    // J'ai une QMap avec le lemme LASLA et son indice en clef et son genre en valeur.
    fic.close();
#endif

    // Lecture de la liste des formes et création des fiches
    QFile fListe (prefixe + "listForm9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    fluxL.setCodec("UTF-8");
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (ligne.isEmpty() || ligne.startsWith("!")) continue;
        fiche = new Fiche(ligne);
//        cntChar(fiche->getForme());
        _fiches.insert(fiche->getClef(),fiche);
        QString tmp = fiche->getTag();
        if (tmp[1] == 'e')
        {
            // C'est un enclitique qui ne peut se trouver seul.
            fCherche << fiche; // Un mot contient une liste de fiche*
            if (fiche->getClef() == "que")
            {
                motQue = new Mot("-", "que", fCherche);
                motQue->setNotNormal();
            }
            if (fiche->getClef() == "ue")
            {
                motVe = new Mot("-", "ue", fCherche);
                motVe->setNotNormal();
            }
            if (fiche->getClef() == "ne")
            {
                motNe = new Mot("-", "ne", fCherche);
                motNe->setNotNormal();
            }
            if (fiche->getClef() == "cum")
            {
                motCum = new Mot("-", "cum", fCherche);
                motCum->setNotNormal();
            }
            fCherche.clear(); // Je laisse la liste vide pour le prochain usage.
        }
#ifdef FIX_GENRE
        listForm9.append(fiche);
        QString lem = fiche->getLemme() + " " + fiche->getIndice();
        if (lemGenr.contains(lem))
            fiche->setGenre(lemGenr[lem]);
        else if (tmp.startsWith("A")) qDebug() << lem << ligne;
        // Pas sûr de vouloir garder cette dernière ligne...
#endif
    }
    // Au fur et à mesure de la lecture des formes simples,
    // j'attribue le genre des substantifs...
    fListe.close();
#ifdef FIX_GENRE
    qDebug() << "Fin de lecture de listForm9";
#endif
    //    qDebug() << _fiches.size();
    // Pour l'instant, que les formes simples...
    // Les formes composées sont dans "../LASLA_data/listComp9.csv"

    fListe.setFileName(prefixe + "listComp9.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (ligne.isEmpty() || ligne.startsWith("!")) continue;
        // Le 30 décembre 2024, j'élimine les formes avec un terme sous-entendu.
//        if (!ligne.contains("("))
//        {
        // Je les remets trois mois plus tard...
            fiche = new Fiche(ligne);
            QString str = fiche->getClef();
            if (str.contains("/") && !str.startsWith("$"))
            {
                _fComplx.insert(str,fiche); // Il y a ambiguïté.
                // qDebug() << str << fiche->getForme() << fiche->getCmpl();
            }
            else _fiches.insert(str,fiche);
#ifdef FIX_GENRE
            listComp9.append(fiche);
        QString lem = fiche->getLemme() + " " + fiche->getIndice();
        if (lemGenr.contains(lem))
            fiche->setGenre(lemGenr[lem]);
        else if (fiche->getTag().startsWith("A")) qDebug() << lem << ligne;
        // Pas sûr de vouloir garder cette dernière ligne...
#endif
//        }
    }
    // Au fur et à mesure de la lecture des formes simples,
    // j'attribue le genre des substantifs...
    fListe.close();
#ifdef FIX_GENRE
    qDebug() << "Fin de lecture de listComp9";
#endif
    // Fin de la lecture des formes officielles.
//    qDebug() << _fComplx.size();
#ifdef AVEC_DICO
    fListe.setFileName(dicoPerso);
    if (fListe.open (QIODevice::ReadOnly|QIODevice::Text))
    {
        // Si le dico perso existe, je le lis.
        while (!fluxL.atEnd ())
        {
            ligne = fluxL.readLine ();
            if (!ligne.isEmpty() && !ligne.startsWith("!"))
            {
            fiche = new Fiche(ligne);
            QString str = fiche->getClef();
            if (str.contains("/") && !str.startsWith("$"))
            {
                _fComplx.insert(str,fiche); // Il y a ambiguïté.
                // qDebug() << str << fiche->getForme() << fiche->getCmpl();
            }
            else _fiches.insert(str,fiche);
//            _fiches.insert(fiche->getClef(),fiche);
            }
        }
    }
    fListe.close();
    fListe.setFileName(dicoAuto);
    if (fListe.open (QIODevice::ReadOnly|QIODevice::Text))
    {
        QStringList listInc;
        // Si le dico auto existe, je le lis.
        while (!fluxL.atEnd ())
        {
            ligne = fluxL.readLine ();
            if (!ligne.isEmpty() && !ligne.startsWith("!"))
            {
                if (ligne.contains("VNKNOWN"))
                    listInc << ligne; // Pour un traitement ultérieur.
                else
                {
                    fiche = new Fiche(ligne);
                    QString str = fiche->getClef();
                    if (str.contains("/") && !str.startsWith("$"))
                    {
                        _fComplx.insert(str,fiche); // Il y a ambiguïté.
                        // qDebug() << str << fiche->getForme() << fiche->getCmpl();
                    }
                    else _fiches.insert(str,fiche);
//                    _fiches.insert(fiche->getClef(),fiche);
                }
            }
        }
        QString precedent = "";
        for (int i = 0; i < listInc.size(); i++)
        {
            ligne = listInc[i];
            /* Je dois traiter ici les formes inconnues
             * qui ont été sauvées automatiquement avec le lemme VNKNOWN.
             * Si tout se passe bien, toutes les fiches VNKNOWN
             * d'une forme se suivent dans la liste.
             * Si j'ai créé ultéreurement une vraie fiche
             * pour cette forme inconnue,
             * elle figure dans la variable _fiches.
             * Dans ce cas, j'ignore les fiches avec VNKNOWN.
             * Sinon, je mets toutes les fiches VNKNOWN
             * de cette même forme dans _fiches.
             * */
            QString f = ligne.section(",",0,0); // la forme
            if (!_fiches.contains(f) ||
                    (precedent == f))
            {
                precedent = f;
                fiche = new Fiche(ligne);
                _fiches.insert(fiche->getClef(),fiche);
            }
        }
    }
    fListe.close();
#endif
//qDebug() << "dico perso";
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
/* Le 18/01/2025
 * Quelques lignes pour attribuer un genre aux lemmes du LASLA.
 * Commentées après usage...
    QFile nouveaugenre(prefixe + "Coll_genre.csv");
    nouveaugenre.open(QIODevice::WriteOnly|QIODevice::Text);
    QString bla;*/
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        if (!ligne.startsWith("!"))
        {
            ligne.remove("-");
            _corresp.insert(ligne.section(",",0,0),ligne.section(",",1,1));
            // On a ici le lemme Collatinus en clef et le lemme LASLA en valeur.
            // L'indice d'homonymie est avec le lemme après une espace.
/*            bla = ligne.section(",",0,1) + "," + _lemCore->genreLem(ligne.section(",",0,0)) + "\n";
            nouveaugenre.write(bla.toUtf8());
Fin du 2e morceau commenté
*/
        }
    }
//    nouveaugenre.close(); // 3e et dernier morceau commenté
    fListe.close();
#ifdef FIX_GENRE
    /*
     * J'ai maintenant rempli la QMap _fiches avec les données en APN.
     * Je vais ouvrir le fichier des données en DAT
     * et regader si la forme a un genre.
     * Si tel est le cas, je vais chercher cette forme
     * dans _fiches en comparant aussi le lemme et le code en 9.
     * J'ai pour cela créé la fonction de conversion code5a9.
    */
    fListe.setFileName(prefixe + "listForm.csv");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    while (!fluxL.atEnd ())
    {
        ligne = fluxL.readLine ();
        QStringList eclats = ligne.split(",");
        if (eclats.size() != 5) qDebug() << "erreur " << ligne;
        else if (eclats[3].size() != 8) qDebug() << "erreur " << ligne << eclats[3];
        else if (eclats[3].at(7) != ' ')
        {
            // J'ai un genre !
            QString f = eclats[0].toLower(); // la forme
            QString l = eclats[1];
            QString ind = eclats[2];
            QString e = eclats[3];
            while (f[f.size() - 1].isDigit()) f.chop(1);
            f = f.trimmed();
            while (l[l.size() - 1].isDigit()) l.chop(1);
            l = l.trimmed();
            if (ind == "C") ind = "N"; // En principe, ça ne devrait pas arriver.
            if ((ind == "N") || (ind == "A"))
                f[0] = f[0].toUpper();
            if ((ind == "N") && e.startsWith("2")) ind = "A";
            // Dans les fichiers DAT, beaucoup d'adjectifs dérivés de noms propres ont un indice N.
            // En APN, il est devenu systématiquement A.
            f.replace("v","u");
            f.replace("U","V");
            f.remove("(");
            f.remove(")");
            QString gDAT = e.mid(7,1);
            QString c9 = codes5a9(e.mid(0,5));
            // J'ai mis la forme au format APN et j'ai les lemme, indice et code-en-9.
            QList<Fiche*> ensemble = _fiches.values(f);
            // L'ensemble des fiches associées à ma forme.
            // Si ma forme contient plusieurs mots, les prendre séparément ?
            bool OK = ensemble.isEmpty();
            if (OK && f.contains(" "))
            {
                QStringList morceaux = f.split(" ", QString::SkipEmptyParts);
                for(int j = 0; j < morceaux.size(); j++)
                    ensemble.append(_fiches.values(morceaux[j]));
                OK = ensemble.isEmpty();
            }
            Fiche* bonneFiche;
            int i = 0;
            while(!OK && (i < ensemble.size()))
            {
                // Je prends les fiches de l'ensemble et
                // je regarde si elles ont les bons lemme, indice et code-en-9.
                bonneFiche = ensemble[i];
                OK = (bonneFiche->getLemme() == l) && (bonneFiche->getIndice() == ind) && (bonneFiche->getCode() == c9);
                i++;
            }
            if (OK && !ensemble.isEmpty()) bonneFiche->setGenre(gDAT);
            else qDebug() << "Inconnue " << ligne << f << ensemble.size();
        } // Fin du traitement d'une ligne avec genre
    } // Fin de la boucle sur les lignes de listForm.csv.
    fListe.close();
    qDebug() << "Fin de listForm.csv";

    /* Pour mettre le genre dans les fichiers listForm9.csv et listComp9.csv
     * je vais les recopier en les renommant ...9G.csv
     * et en regardant si la fiche correspondante a acquis un genre.
    */
    QString bla;
    QFile nouveaugenre(prefixe + "listForm9_G.csv");
    nouveaugenre.open(QIODevice::WriteOnly|QIODevice::Text);
    for (int ii = 0; ii < listForm9.size(); ii++)
    {
        fiche = listForm9[ii];
        bla = fiche->getLigne() + "," + fiche->getGenre() + "\n";
        nouveaugenre.write(bla.toUtf8());
    }
    nouveaugenre.close();
    nouveaugenre.setFileName(prefixe + "listComp9_G.csv");
    nouveaugenre.open(QIODevice::WriteOnly|QIODevice::Text);
    for (int ii = 0; ii < listComp9.size(); ii++)
    {
        fiche = listComp9[ii];
        bla = fiche->getLigne() + "," + fiche->getGenre() + "\n";
        nouveaugenre.write(bla.toUtf8());
    }
    nouveaugenre.close();
#endif
}

/**
 * @brief Lasla::nouveau
 * Ouvre un nouveau fichier texte et lemmatise l'ensemble des mots.
 * Doit aussi sauver un fichier XML pour conserver l'état intermédiaire.
 */
QString Lasla::nouveau()
{
    if (_nomFichier.endsWith(".txt")) _nomFichier.chop(4);
    QString reponse = "Nb de caractères : %1\nNb de phrases : %5\nNb de mots : %2\nNb de tokens : %3\nNb d'inconnus : %4\n";
    int nb_inc = 0;
    QList<int> auxil;
    QList<int> partic;
    /*
     * Au moment de la lecture de la phrase,
     * je repère les éventuels auxiliaires et participes
     * pour essayer d'identifier des formes verbales composées.
     * Je ne garde que le numéro du mot.
     * Dans les auxiliaires, il y a les infinitifs "esse" et "iri"
     * et n'importe quelle forme conjuguée de SVM_1.
     * Pour l'infinitif futur passif (avec "iri") c'est un supin,
     * mais il a la même forme qu'un PPP en "um".
     * Pour l'infinitif futur actif, c'est un participe futur
     * que je dois trouver.
     * Pour les infinitifs avec "esse", le participe passé ou futur peut être
     * au nominatif ou à l'acusatif et je dois conserver ses cas et nombre.
     * Avec un SVM_1 conjugué, j'attends un PPP au nominatif,
     * accordé en nombre avec l'auxiliaire.
     * */
    if (!_texte.isEmpty())
    {
        Mot * mot;
        QString ref;
        QString refO;
        ref = formatRef;
        if (_CR4) ref = ref.arg(numLvr);
        if (_CR3) ref = ref.arg(numCh);
        if (_CR2) ref = ref.arg(numPar);
        if (_CR1) ref = ref.arg(numLg);

        if (!_mots.isEmpty())
        {
            // Avant d'effacer les mots, qui sont des pointeurs,
            // je dois détruire les objets pointés.
            for (int i = 0; i < _mots.size(); i++)
                if (_mots[i]->getNormal()) delete _mots[i];
            // Je ne dois pas détruire les motQue et autres enclitiques.
            _mots.clear();
        }
        _finsPhrase.clear();
        _fPhrEl.clear();
        _pointsFixes.clear();
        _pointsFixes << 0; // Le début du texte est un point fixe. (?)
        // J'ai chargé mon texte. Je dois le lemmatiser entièrement.
        _elements = _texte.split(QRegExp("\\b"));

        if (_texte.contains(QRegExp("[0-9$]"))) grec();
        // Les $ encadrent des mots grecs
        // S'il y a des chiffres, ils seront placés en séparateurs.
        // Le test du point (abréviation ou fin de phrase) ainsi que
        // le traitement de l'apostrophe pourraient être aussi
        // traités en amont de l'analyse, puisqu'il s'agit de modifier
        // les éléments. On gagnerait en lisibilité pour l'analyse.

//        qDebug() << _elements.length();

        int i = 1;
        while (i < _elements.length())
        {
            QString np = QString::number(_finsPhrase.size() + _decalPhr);
            np[0] = '&'; // Par défaut. Je ne gère pas les '#' et '+'.
            refO = _refOeuvre.mid(0,3) + np + "/" + ref;

            QString m = _elements.at(i); // les mots dans l'ordre.
            if (m.isEmpty()) qDebug() << "Alerte mot vide !" << i;
            _analyses.clear();
//            if (m == "creaturae")
  //          qDebug() << m;
            bool enclit = false;
            // A priori, sans enclitique.
            // Attention aux v, U, j et J !
            m.replace("v","u");
            m.replace("U","V");
            m.replace("j","i");
            m.replace("J","I");

            QString sep = _elements[i+1];
            // C'est le séparateur qui suit le mot examiné.
            //                qDebug() << m << sep;

            if (sep.contains("."))
            {
                // Attention !
                // Je viens de découvrir que "AS" s'écrit ".x." :
                // il faut donc examiner le cas où le séparateur finit par un ".".
                // Mais c'est compliqué car il faut aller voir s'il est suivi d'un "x.".
                // Il est peut-être plus simple de remplacer ".x." dans le texte par "ZZ" (par exemple)
                // et tester ensuite si j'ai "ZZ", le remplacer par ".x."...
                // Cela dit, d'après le fichier "listForm9.csv", il n'y a que trois occurrences de ".x."
                // toute dans CesBHisp !
                if (sep.startsWith("."))
                {
                    // Ce point peut être celui d'une abréviation ou celui d'une fin de phrase.
                    // Il faut faire attention aux nombres (en chiffres romains) en fin de phrases.
                    // Je sépare à la main le nombre du point par un espace.
                    _analyses = _fiches.values(m + ".");
                    if (!_analyses.isEmpty() || (m.size() == 1))
                    {
                        // Je considère que tout mot d'une seule lettre n'existe pas en fin de phrase.
                        m += "."; // C'est une abréviation
//                        qDebug() << "abréviation" << m;
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
                        Fiche * fiche = new Fiche(m + ",nbr, ,D28      ,D8 ,1");
                        _analyses  << fiche;
                        ajouterFiche(fiche);
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
//            if (m == "creaturae")
  //          qDebug() << "2" << m;

            if (_analyses.isEmpty()) _analyses = _fiches.values(m);
//            qDebug() << m << _analyses.size();
            if (_medieval && (m != "mecum") && (m != "tecum")) ajoutsMed(m);
            // Les cas de mecum et de tecum doivent être traités à part.
            // "moechum" devient "mecum" en médiéval.
            if ((_fPhrEl.last() == (i - 1)) && (!m.isEmpty() && m[0].isUpper()))
            {
                // Je suis en début de phrase et j'ai une majuscule
                QString mMin = m.toLower();
                if (m[0] == 'V') mMin[0] = 'u';
                _analyses.append(_fiches.values(mMin));
                if (_medieval) ajoutsMed(mMin);
            }
            // Traiter les majuscules et les enclitiques
            if (_analyses.isEmpty() && (!m.isEmpty() && m[0].isUpper()))
            {
                if (estRomain(m))
                {
                    Fiche * fiche = new Fiche(m + ",nbr, ,D18      ,D8,1");
                    ajouterFiche(fiche);
                    _analyses  << fiche;
                }
                else
                {
                    QString mMin = m.toLower();
                    if (m[0] == 'V') mMin[0] = 'u';
                    _analyses = _fiches.values(mMin);
                    if (_medieval) ajoutsMed(mMin);
                }
            }
            if (_analyses.isEmpty())
            {
                // Je ne cherche l'enclitique que si la forme entière n'a pas été trouvée.
                if (m.endsWith("que") || m.endsWith("cum"))
                {
                    _analyses = _fiches.values(m.mid(0,m.size()-3));
                    //                       qDebug() << "que-cum" << m << m.mid(0,m.size()-3) << _analyses.size();
                    if (_medieval) ajoutsMed(m.mid(0,m.size()-3));
                    if (_analyses.isEmpty() && m[0].isUpper())
                    {
                        QString mMin = m.toLower().mid(0,m.size()-3);
                        if (m[0] == 'V') mMin[0] = 'u';
                        _analyses = _fiches.values(mMin);
                        if (_medieval) ajoutsMed(mMin);
                    }
                    if (_analyses.isEmpty() && m.endsWith("cumque"))
                    {
                        // Le cas des doubles enclitiques du genre "secumque".
                        // Je le traite après plutôt qu'avant à cause
                        // des formes en "cumque" qui ne seraient pas des doubles enclitiques.
                        // Par exemple, "amicumque".
                        _analyses = _fiches.values(m.mid(0,m.size()-6));
                        //                       qDebug() << "cumque" << m << m.mid(0,m.size()-6) << _analyses.size();
                        if (_medieval) ajoutsMed(m.mid(0,m.size()-6));
                        if (_analyses.isEmpty() && m[0].isUpper())
                        {
                            QString mMin = m.toLower().mid(0,m.size()-6);
                            if (m[0] == 'V') mMin[0] = 'u';
                            _analyses = _fiches.values(mMin);
                            if (_medieval) ajoutsMed(mMin);
                        }
                    }
                    enclit = !_analyses.isEmpty();
                    // Je devrai ensuite savoir si j'ai retiré "cumque" ou seulement "que".
                }
                else if (m.endsWith("ue") || m.endsWith("ne"))
                {
                    _analyses = _fiches.values(m.mid(0,m.size()-2));
                    if (_medieval) ajoutsMed(m.mid(0,m.size()-2));
                    if (_analyses.isEmpty() && m[0].isUpper())
                    {
                        QString mMin = m.toLower().mid(0,m.size()-2);
                        if (m[0] == 'V') mMin[0] = 'u';
                        _analyses = _fiches.values(mMin);
                        if (_medieval) ajoutsMed(mMin);
                    }
                    enclit = !_analyses.isEmpty();
                }
            }
//            if (m == "creaturae")
  //          qDebug() << "3" << m;
            // Appel systématique à Collatinus
            bool e = enclit;
            bool exceptions = (m == "mecum") || (m == "tecum") || (m == "secum");
            exceptions = exceptions || (m == "nobiscum") || (m == "uobiscum");
            QList<Fiche*> na;
            // Les abréviations semblent poser problème ToDo !
            // mecum et tecum sont à éviter aussi
            if (!m.contains(".") && !exceptions)
                na = appelCollatinus(m, &e);
//            if (m == "creaturae")
//            qDebug() << "2" << m;
 // qDebug() << m << _analyses.size() << na.size() << enclit;
            if (_analyses.isEmpty())
            {
                _analyses = na;
                enclit = e;
                // S'il n'y a pas d'analyse du LASLA, je prends celles de Collatinus,
                // avec ou sans enclitique.
            }
            else if (!na.isEmpty())
            {
                if ((e && enclit) || (!e && !enclit))
                    _analyses.append(na);
                else if (!e && enclit)
                {
                    // Collatinus a une analyse sans enclitique.
                    _analyses = na;
                    enclit = false;
                    // Ce cas pose un problème avec tecum qui est lemmatisé,
                    // par Collatinus, en Tecum2, fleuve de la Narbonnaise.
                    // Les autres mecum, tecum etc. de Collatinus ont été commentés.
                }
            }
            // Dans le 4e cas (e && !enclit), je ne fais rien.
            // Si la liste d'analyses n'est pas vide,
            // je ne dois créer que les nouvelles analyses.
            if (_analyses.isEmpty())
            {
                qDebug() << "inconnu" << m;
                // En dernier recours, la saisie manuelle avec GUI.
                // Mais pour être autonome, le programme doit introduire
                // un lemme VNKNOWN avec tous les tags possibles (?) !
                unknown(m); // Peuple directement _analyses.
                nb_inc++;
            }
            else
            {
                if (_preProc)
                {
                    // Je dois regarder si la forme a été reconnue comme VNKNOWN.
                    bool toutInc = true;
                    for (int i = 0; i < _analyses.size(); i++)
                    {
                        Fiche * fiche = _analyses.at(i);
                        if (fiche->getLemme() != "VNKNOWN") toutInc = false;
                    }
                    if (toutInc)
                        _inconnus.append(m + ":" +
                                         QString::number(_finsPhrase.size()));
                }
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
//                     qDebug() << m << c << f->getClef() << f->getForme() << egaux << ecl;
                }
            }
//            if (m == "creaturae")
//            qDebug() << "2" << m;
  //          qDebug() << m << _analyses.size() << aMultiple.size() << aRetirer.size();
            if (aMultiple.isEmpty())
            {
                if (!aRetirer.isEmpty())
                {
                    // Il y a des analyses qui n'ont pas trouvé le mot nécessaire.
                    foreach (Fiche * f, aRetirer)
                    {
                        _analyses.removeOne(f);
                    }
//                    qDebug() << _analyses.size();
                }
                if (_analyses.size() > 1)
                {
                    qSort (_analyses.begin(), _analyses.end(), plusFreq);
                    // Je trie les analyses pour avoir la plus fréquente d'abord.
                    if (_analyses[0]->jointe())
                    {
//                        if (m == "creaturae")
 //                       qDebug() << "5 : jointe" << m;
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
                          //  qDebug() << f << lf1.size() << lf2.size() << motInter;
                        }
                        // La liste lf1 vient d'abord
// ToDo : Les lignes qui suivent ne peuvent pas fonctionner !
                        if (lf1.isEmpty()) qDebug() << "lf1 est vide";
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
                            if (!lf2.isEmpty())
                            mot = new Mot (m,lf2[0]->getForme(),lf2, i, r);
                            else qDebug() << "lf2 est vide !";
                        }
                    } // Forme jointe
                    else
                    {
//                        if (m == "creaturae")
//                        qDebug() << "6 : non jointe" << m;
                        mot = new Mot (m,nettoie(_analyses[0]->getForme()),_analyses, i, refO);
                        if (_medieval && (m != mot->getFLem()))
                        {
                            qDebug() << m << mot->getFLem();
                            // Deux possibilités : enclitique ou forme médiévale
                            int tailleM = transfMed(m).size();
                            int tailleCl = transfMed(mot->getFLem()).size();
                            if (tailleCl == tailleM) mot->setFLem(m);
                            else if (tailleCl < tailleM)
                            {
                                // Enclitique.
                                QString mm = m;
                                mm.chop(tailleM - tailleCl);
                                qDebug() << m << mm << m.right(tailleM - tailleCl);
                                mot->setFLem(mm);
                            }
                            else qDebug() << "Je ne sais pas quoi faire :" << m << mot->getFLem();
                        }
                    }
//                    if (m == "creaturae")
//                    qDebug() << "7 : survie" << m;
                }
                else
                {
                    if (_analyses.isEmpty())
                    {
                        unknown(m); // Peuple directement _analyses.
                        nb_inc++;
                    }
                    else if (_preProc) // Doublon ?
                    {
                        // Je dois regarder si la forme a été reconnue comme VNKNOWN.
                        bool toutInc = true;
                        for (int i = 0; i < _analyses.size(); i++)
                        {
                            Fiche * fiche = _analyses.at(i);
                            if (fiche->getLemme() != "VNKNOWN") toutInc = false;
                        }
                        if (toutInc)
                            _inconnus.append(m + ":" +
                                             QString::number(_finsPhrase.size()));
                    }
                   // mot = new Mot (m,"???",_analyses, i, refO);
                    mot = new Mot (m,nettoie(_analyses[0]->getForme()),_analyses, i, refO);
                    /* Il y a ici une erreur fondamentale
                     * dans le cas d'un texte médiéval.
                     * En réalité, pas tant ici que quelques lignes au-dessus,
                     * où on a plusieurs analyses possibles.
                     * En effet, une forme comme "pene" peut être l'adverbe
                     * "paene" ou "pene", mais aussi le mot "poenae".
                     * La forme analysée classicisée n'est pas unique,
                     * et il n'y a aucune raison de prendre la première (comme ici)
                     * ou la dernière forme classicisée.
                     * Si on ne veut pas influencer l'IA, il vaut mieux garder
                     * la forme du texte médiéval.
                     * Ce n'est d'ailleurs pas sûr, car le sub_word_tokenizer
                     * dirige peut-être une terminaison en "ae" vers une forme déclinée
                     * et un en "e" vers un adverbe...
*/
                    if (_medieval && (m != mot->getFLem()))
                    {
                        // Deux possibilités : enclitique ou forme médiévale
                        int tailleM = transfMed(m).size();
                        int tailleCl = transfMed(mot->getFLem()).size();
                        if (tailleCl == tailleM) mot->setFLem(m);
                        else if (tailleCl < tailleM)
                        {
                            QString mm = m;
                            mm.chop(tailleM - tailleCl);
                            qDebug() << m << mm << m.right(tailleM - tailleCl);
                            mot->setFLem(mm);
                        }
                        else qDebug() << "Je ne sais pas quoi faire :" << m << mot->getFLem();
                    }
                }
                _mots.append(mot);
                if (enclit)
                {
//                    if (m.endsWith("cumque")) qDebug() << m << _analyses[0]->getClef();
                    bool cumque = m == _analyses[0]->getClef() + "cumque";
                    // Même problème potentiel si la clef contient "ae" et la forme médiévale "e".
                    if (_medieval && !cumque)
                        cumque = transfMed(m) == transfMed(_analyses[0]->getClef()) + "cumque";
                    if (cumque)
                    {
                        _mots << motCum;
                        _mots << motQue;
                    }
                    else if (m.endsWith("que")) _mots << motQue;
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
//            if (m == "creaturae")
  //          qDebug() << "4" << m;

            // Suis-je à la fin d'une phrase ?
            if (sep.contains("?") || sep.contains("!")/* || sep.contains(";")
                    || sep.contains(":")*/ || sep.contains("."))
            {
                _finsPhrase.append(_mots.size()); // C'est le rang du mot courant.
                _fPhrEl.append(i + 1); // C'est le rang du mot dans _elements.
//                qDebug() << _elements[i] << _elements[i+1] << _mots.size() << _finsPhrase.size();
            }

            // Suis-je à la fin d'une ligne ?
            if (sep.contains("\n"))
            {
                // Oui ! Je dois mettre à jour la référenciation
                int cnt = sep.count("\n");
                switch (cnt) {
                case 2:
                    if (_CR2){
                        numLg = 1;
                        numPar++;
                    }
                    else numLg++;
                    // Si je ne tiens pas compte des doubles sauts, je les considère comme des simples
                    break;
                case 3:
                    if (_CR3){
                        numLg = 1;
                        numPar = 1;
                        numCh++;
                    }
                    else if (_CR2){
                        // Si je ne tiens pas compte des triples sauts, je les considère comme des doubles
                        numLg = 1;
                        numPar++;
                    }
                    else numLg++;
                    break;
                case 4:
                    if (_CR4){
                        numLg = 1;
                        numPar = 1;
                        numCh = 1;
                        numLvr++;
                    }
                    else if (_CR3){
                        numLg = 1;
                        numPar = 1;
                        numCh++;
                    }
                    else if (_CR2){
                        numLg = 1;
                        numPar++;
                    }
                    else numLg++;
                    break;
                default:
                    numLg++; // simple saut de ligne
                    break;
                }
                if (_finsPhrase.last() != _mots.size())
                {
                    // Je n'avais pas de marque de fin de phrase,
                    // mais j'ai un saut de ligne.
//                    qDebug() << numCh << numPar << numLg;
                    if ((numLg == 1) || (_prose && _CR1))
                    {
                        // La 1ère condition correspond à un changement d'ordre supérieur.
                        // Si j'ai un texte en prose, je dois vérifier que le saut de ligne est pertinent.
                        _finsPhrase.append(_mots.size()); // C'est le rang du mot courant.
                        _fPhrEl.append(i + 1); // C'est le rang du mot dans _elements.
                    }
                }
                ref = formatRef;
                if (_CR4) ref = ref.arg(numLvr);
                if (_CR3) ref = ref.arg(numCh);
                if (_CR2) ref = ref.arg(numPar);
                if (_CR1) ref = ref.arg(numLg);
            }
            i += 2; // Pour pouvoir regrouper des mots, j'ai dû passer de for(;;) à while ().
        } // Le texte est lemmatisé.
        if (_finsPhrase.last() < _mots.size()) _finsPhrase.append(_mots.size());
        // Si jamais il n'y avait pas de ponctuation à la fin du texte...
        // qDebug() << "Fin du texte";

        // Je dois vérifier que les analyses conditionnelles sont OK.
        // Par exemple, l'auxiliaire entre crochets doit être présent dans la phrase.
        int deb = 0;
        // Je parcours le texte phrase par phrase
        for (int iPhrase = 0; iPhrase < _finsPhrase.size();iPhrase++)
        {
            auxil.clear();
            partic.clear();
            // Je parcours la phrase mot par mot.
            for (int i = deb; i < _finsPhrase[iPhrase];i++)
            {
                decimer(i,deb,_finsPhrase[iPhrase]);
                if (_mots[i]->estAux()) auxil << i;
                else if (_mots[i]->estPart()) partic << i;
                // Je cherche les potentielles formes verbales composées.
            }
            if (!auxil.isEmpty() && !partic.isEmpty())
            {
                // J'ai a priori un auxiliaire et un participe :
                // vont-ils ensemble ?
//                qDebug() << auxil.size() << partic.size() << iPhrase;
                for (int ii = 0; ii < auxil.size(); ii++)
                    for (int jj = 0; jj < partic.size(); jj++)
                        vontEnsemble(auxil[ii], partic[jj]);
            }
            deb = _finsPhrase[iPhrase];
        }
        // qDebug() << "Fin de decimer";
        if (_preProc) return "fini !";
        // J'ai fini les vérifs. Je sauve l'état brut.
        sauver(_nomFichier + "_00.APN");
        // qDebug() << _finsPhrase.size() << _mots.size() << _finsPhrase.last() << _mots.last()->formeFiche();

        // Début du taggage.
#ifdef LEGACY
        taggage_old();
        //            qDebug() << _finsPhrase.size() << _mots.size() << _finsPhrase.last();
        sauver(_nomFichier + "_01.APN");
#else
        taggage();
        //            qDebug() << _finsPhrase.size() << _mots.size() << _finsPhrase.last();
        sauver(_nomFichier + "_02.APN");
#endif
        sauveCSV(_nomFichier + ".csv");
        reponse = reponse.arg(_texte.size()).arg(_elements.size()/2 - 1)
                .arg(_mots.size()).arg(nb_inc).arg(_finsPhrase.size());

    } // Fin du traitement d'un texte non-vide.
    else reponse = "Texte vide !";
    return reponse;
}

void Lasla::ajoutsMed(QString f)
{
    QString fMed = transfMed(f);
    if (_ficMed.contains(fMed))
    {
        foreach(QString fClass, _ficMed.values(fMed))
            foreach (Fiche* ficMed, _fiches.values(fClass))
                if (!_analyses.contains(ficMed)) _analyses.append(ficMed);
            // Les analyses médiévales du mot en cours.
    }
}

/**
 * @brief Lasla::appelCollatinus
 * @param m : le mot
 * @param enclit : avec enclitique
 * @return une liste de Fiches
 */
QList<Fiche*> Lasla::appelCollatinus(QString m, bool* enclit)
{
    // Il y a un problème avec l'enclitique, s'il y a une majuscule (de début de phrase).
    // Par exemple, Ioue est lemmatisé Juppiter, puis on essaie "ioue"
    // qui n'a pas de lemmatisation sous sa forme entière, mais en a une en "io + ue".
    // Il faut donc séparer les solutions sans et avec enclitique.
    // Inversement, supposons que le LASLA ne connaisse pas l'ablatif "mentione"
    // mais connaisse le nominatif "mentio" : la première analyse donnerait donc
    // "mentio + ne", alors que Collatinus va analyser "mentione".
    // Pour être cohérent, il faudrait éliminer la solution avec enclitique
    // et ne garder que la solution sans...

    QList<Fiche*> analyses;
    bool sans_encl = !_analyses.isEmpty() && !*enclit;
    /* Contrairement à ce qui se faisait dans la première version,
     * des analyses peuvent avoir été trouvées avec les données du LASLA.
     *
     * Si j'ai déjà des analyses sans enclitique,
     * je ne dois garder que les analyses de Collatinus sans enclitique.
     * Inversement, si Collatinus donne des solutions sans enclitique,
     * il faudrait jeter les solutions avec.
     * Si je n'ai encore aucune analyse, je dois prendre celles de Collatinus
     * comme elles viennent.
     * */

// Début de l'ancien k9
    // J'y intègre ce que je faisais plus loin.
    QStringList listRep;
    MapLem mm = _lemCore->lemmatiseM(m);
    if (mm.isEmpty())
    {
        // 17/04/25 je remplace startWith par contains.
        if (m.contains("V"))
        {
            m.replace("V","U");
            mm = _lemCore->lemmatiseM(m);
        }
    }
    if (mm.isEmpty())
    {
//        qDebug() << "Ici";
        return analyses;
    }
    foreach (Lemme *l, mm.keys())
    {
        QString clef = l->cle();
        bool enR = clef.endsWith("r");
        if (clef[clef.size()-1].isDigit())
            enR = clef[clef.size() - 2] == "r";
        // Pour savoir, plus tard, si on a un verbe déponent.
        bool semiDep = false;
        clef.replace("v","u");
        clef.replace("j","i");
        clef.replace("J","I");
        clef.replace("V","U");
        // J'avais adopté des notations non-Ramistes.
        if (_corresp.contains(clef))
        {
            // La clef Collatinus a un équivalent au LASLA.
            clef = _corresp.value(clef).trimmed();
            semiDep = estSemidep(clef);
            // Je ne connais que les semi-déponents du LASLA.
            if (clef.contains(" ")) clef.replace(" ",",");
            else clef.append(", ");
            // J'ai une clef qui est le lemme LASLA et son indice.
        }
        else
        {
 //           clef.replace("U","V");
            // "U" doit devenir "V" : pas convaincu !
            // C'est fait à la création de la fiche.
            clef.append(", ");
            // La clef est le lemme Collatinus et un indice vide.
        }

        foreach (SLem s, mm.value(l))
        {
            QString code9 = _lemCore->morpho(s.morpho);
            QString forme = Ch::atone(s.grq);
            if (!s.sufq.isEmpty()) forme += "<" + Ch::atone(s.sufq) +">,";
            else
            {
                forme += ",";
                sans_encl = true;
                // Il y a au moins une analyse de Collatinus sans enclitique.
            }
            forme.replace("v","u");
            forme.replace("j","i");
            forme.replace("J","I");
            forme.replace("U","V");
            // Les formes sont non-Ramistes, mais avec des "V" !

            if (s.morpho == 416)
            {
                // J'ai un invariable !
                if (!l->genre().isEmpty()) code9 = "A68      ";
                // Si le lemme a un genre, c'est un substantif (cas probable des noms hébreux).
                // qDebug() << forme << code9;
            }

            QString cat = "";
            if (_catLasla.contains(l->modele()->gr())) cat = _catLasla[l->modele()->gr()];
            if (cat.endsWith("7"))
            {
                // Les paradigmes grecs comptent des désinences latines.
                QString pere = l->pereLatin(s.grq,s.morpho);
                if (_catLasla.contains(pere)) cat = _catLasla[pere];
            }
            if (!cat.isEmpty()) code9.replace("k9", cat);

            if ((code9[0] != 'C') && (code9[0] != 'M')) code9[4] = ' ';
            // Seuls les adjectifs et les adverbes peuvent avoir un degré.
            if (code9.startsWith("B") && code9.endsWith(" 5   "))
            {
                // Le bon code pour l'adjectif verbal est 502 ou 503 (déponent)
                if (enR) code9.replace(" 5   "," 503 ");
                else code9.replace(" 5   "," 502 ");
            }
            if (code9.startsWith("B") && semiDep) code9[7] = '4';
            listRep << forme + clef + "," + code9;
        }
    }
// Fin de l'ancien k9 augmenté.

    if (listRep.isEmpty()) return analyses;
    // inutile en principe, car mm n'était pas vide.
  //  {
    listRep.removeDuplicates();
    // Les différents genres pour les adjectifs donneraient la même fiche

/*        if (!sans_encl) foreach (QString lg, listRep)
            if ((lg.count(",") == 3) && !lg.contains("<")) sans_encl = true;
        // Il y a au moins une analyse de Collatinus sans enclitique.
*/
    if (!_analyses.isEmpty() && !*enclit) sans_encl = true;
    // J'ai une solution LASLA sans enclitique :
    // je ne garde que celles de Collatinus sans enclitique

    foreach (QString lg, listRep)
    {
        // La transformation du "-" était faite globalement.
        if (lg.contains("-"))
            lg.replace("-","k9       ");
        // Je pense que ce segment est devenu inutile...

        if ((lg.count(",") == 3) && !(sans_encl && lg.contains("<")))
        {
            // Chaque ligne est une analyse avec "forme,clef,espace,code"
            // ou "forme,lemme,indice,code".
            // La première condition est une condition de validité de la ligne.
            // Si j'ai dans le lot une solution sans enclitique, je ne traite pas celles avec.
/* Fait plus haut !
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
                */
            QString forme = lg.section(",",0,0);
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
                    Fiche * fiche = creerFiche(ligne);
                    if ((fiche != nullptr) && !sans_encl &&
                            !_analyses.contains(fiche))
                        analyses.append(fiche);
                    /* Quand je crée une fiche avec creerFiche,
                     * je récupère une fiche qui est la nouvelle fiche
                     * ou qui existait déjà dans _fiches.
                     * Dans ce dernier cas, elle est aussi dans _analyses
                     * et je n'ai pas besoin d'en tenir compte.
                     * */
                    //                        qDebug() << lg << ligne << fiche->getClef();
 /*                   if (!_fiches.contains(fiche->getClef()))
                    {
                        // n'ajouter qu'une fois la forme <blabla>st
                        ajouterFiche(fiche);
                        if (!sans_encl) analyses.append(fiche);*/
/* Pas sûr que ce soit une bonne idée :
 * SVM 2 suppose que j'ai repéré le PPP associé...
                        ligne = fst + ",SVM,2,#        ,#  ,1";
                        fiche = new Fiche(ligne);
                        ajouterFiche(fiche);
                        if (!sans_encl) analyses.append(fiche);*/
//                    }
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
                //                    bool egales = (f->getLemme() == fiche->getLemme());
                //                    egales = egales && (f->getIndice() == fiche->getIndice());
                //                    egales = egales && (f->getCode() == fiche->getCode());
                nvle = nvle && !fichesEgales(f, fiche);
            }
            if (nvle)
            {
                if (!sans_encl) analyses.append(fiche);
                else if (!forme.contains("<")) analyses.append(fiche);
                // Si j'ai une solution sans enclitique, je ne garde pas celle avec.

                ajouterFiche(fiche);
                // La sauvegarde dans le dico perso est faite dans ajouterFiche.
            }
            else delete fiche;
        }
    }
    //        fDic.close();
//}
//    if (m == "quest")
  //      qDebug() << "quest 2" << _fiches.size() << _ficMed.size() << analyses.size() << *enclit << sans_encl;

    *enclit = !sans_encl;
    return analyses;
}

/**
 * Fonction de test pour les nombres
 * écrits en chiffres romains.
 *
 */
bool Lasla::estRomain(QString r)
{
    return !(r.contains(QRegExp ("[^IVXLCDM]"))
             || r.contains("IL")
             || r.contains("IVI"));
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
void Lasla::grec()
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
                    // Attention ! il peut y avoir des mots grecs à la suite.
                    // Le séparateur contient alors deux "$" et le code actuel n'en considère qu'un.
                    // Comme le commence à la fin, il ignore le $ de fin du mot qui précède,
                    // si bien qu'il va considérer le $ de début comme celui qui termine un autre mot grec...

                    // J'ai un mot grec entre les indices j et i (j < i)
                    QString motGrec = _elements.at(j);
                    // qDebug() << _elements[j] << _elements[j+1] << _elements[j+2];
                    int p = motGrec.lastIndexOf('$');
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
                    qDebug() << _elements[j] << _elements[j+1] << _elements[j+2] << _elements[j+3];
                    i = j;
                }
                else
                {
                    qDebug() << i << j << _elements[i] << _elements[j];
                    i = j - 2;
                }
            }
        }
    }
}

bool Lasla::ouvrir(QString nomFichier)
{
    /* Ici, je voudrais distinguer le chargement d'un fichier APN,
     * de celui d'un ZPN.
     * Quand je lis un APN, je ne prends que les fiches dans _fiches.
     * En particulier, je n'interroge pas Collatinus.
     * C'est pensé pour fonctionner sur l'ordinateur qui
     * a servi à créer l'APN. Si ce n'est pas le cas,
     * et que je n'ai pas récupéré les dicos perso et auto
     * du créateur, il va me manquer des analyses possibles.
     *
     * Dans ce second cas (du ZPN), je dois juste lire
     * les fiches enregistrées et regarder si elles existent.
     * Si oui, je mets le pointeur associé là où il faut.
     * Sinon, je crée la fiche correspondante.
     *
     * */
    if (!_mots.isEmpty())
    {
        // Avant d'effacer les mots, qui sont des pointeurs,
        // je dois détruire les objets pointés.
        for (int i = 0; i < _mots.size(); i++)
            if (_mots[i]->getNormal()) delete _mots[i];
        // Je ne dois pas détruire les motQue et autres enclitiques.
        _mots.clear();
    }
    _elements.clear();
    _finsPhrase.clear();
    QString linea;
    QString tete; // La ref de l'œuvre et n° de phrase.
    bool debPhrase = true;
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
    QString lignePrinc = "";
    QString ligneInfos = "";

    QFile f(nomFichier);
    f.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxM (&f);

    if (nomFichier.endsWith(".ZPN"))
    {
        // Si c'est un ZPN, j'ai toute l'info.
        Fiche *fiche;
        while (!fluxM.atEnd ())
        {
            //                linea = ligne;
            linea = fluxM.readLine ();
            if (linea.isEmpty ()) continue;
            if (linea.startsWith("!#"))
            {
                // C'est des infos complémentaires.
                ligneInfos = linea;
            }
            else if (linea.startsWith("!"))
            {
                // C'est une analyse possible.
                lemme = linea.mid(2,21).trimmed();
                indice = linea.mid(23,1);
                forme = linea.mid(24,25).trimmed();
                code9 = linea.mid(61,9);
                t = tag(code9);
                QString lg = forme + "," + lemme + "," +
                        indice + "," + code9 + "," + t + ",1";
//                qDebug() << lg << linea;
                fiche = creerFiche(lg);
                if (fiche != nullptr) analyses << fiche;
            }
            else
            {
                // C'est la ligne de l'APN qui donne le choix fait
                if (!lignePrinc.isEmpty())
                {
                    // J'ai de quoi créer le mot.
                    if (car8 != lignePrinc[7])
                    {
                        // Je commence une nouvelle phrase;
                        debPhrase = true;
                        car8 = lignePrinc[7];
                        if (!_mots.isEmpty())
                            _finsPhrase << _mots.size();
                    }
                    else debPhrase = false;
                    tete = lignePrinc.mid(0,8);
                    car4 = lignePrinc.mid(3,1);
                    lemme = lignePrinc.mid(8,21).trimmed();
                    indice = lignePrinc.mid(29,1);
                    forme = lignePrinc.mid(30,25).trimmed();
                    ref = lignePrinc.mid(55,12).trimmed();
                    code9 = lignePrinc.mid(67,9);
                    fin = lignePrinc.mid(76);
                    // Créer le mot...
                    Mot * mot = new Mot (Fiche::clef(forme),forme,analyses,-1, tete + "/" + ref + "/" + fin);
                    _mots.append(mot);
                    // Et y choisir l'analyse désignée dans la ligne.
                    mot->designe(forme,lemme,indice,code9);
//                    qDebug() << forme << analyses.size() << mot->getChoix();

                    // Reste à récupérer les infos complémentaires.
                    QStringList eclats = ligneInfos.split("\t");
                    if (eclats.size() == 6)
                    {
                        mot->setFcsv(eclats[1]);
                        mot->setApres(eclats[2]);
                        mot->setAItag(eclats[3]);
                        mot->setAIind(eclats[4]);
                        mot->setAIsub(eclats[5]);
                    }
                    else qDebug() << "ligne d'infos " << ligneInfos;
                }
                lignePrinc = linea;
                analyses.clear();
                // Je la garde pour le prochain mot.
            }
        }
        f.close();
        // Le dernier mot !
        if (!lignePrinc.isEmpty())
        {
            // J'ai de quoi créer le mot.
            if (car8 != lignePrinc[7])
            {
                // Je commence une nouvelle phrase;
                debPhrase = true;
                car8 = lignePrinc[7];
                if (!_mots.isEmpty())
                    _finsPhrase << _mots.size();
            }
            else debPhrase = false;
            tete = lignePrinc.mid(0,8);
            car4 = lignePrinc.mid(3,1);
            lemme = lignePrinc.mid(8,21).trimmed();
            indice = lignePrinc.mid(29,1);
            forme = lignePrinc.mid(30,25).trimmed();
            ref = lignePrinc.mid(55,12).trimmed();
            code9 = lignePrinc.mid(67,9);
            fin = lignePrinc.mid(76);
            // Créer le mot...
            Mot * mot = new Mot (Fiche::clef(forme),forme,analyses,-1, tete + "/" + ref + "/" + fin);
            _mots.append(mot);
            // Et y choisir l'analyse désignée dans la ligne.
            mot->designe(forme,lemme,indice,code9);
            qDebug() << forme << analyses.size() << mot->getChoix();

            // Reste à récupérer les infos complémentaires.
            QStringList eclats = ligneInfos.split("\t");
            if (eclats.size() == 6)
            {
                mot->setFcsv(eclats[1]);
                mot->setApres(eclats[2]);
                mot->setAItag(eclats[3]);
                mot->setAIind(eclats[4]);
                mot->setAIsub(eclats[5]);
            }
            else qDebug() << "ligne d'infos " << ligneInfos;
        }
        _finsPhrase << _mots.size();
        return !_mots.isEmpty();
    }
    while (!fluxM.atEnd ())
    {
        //                linea = ligne;
        linea = fluxM.readLine ();
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
            if (!_mots.isEmpty())
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
                /*                    QMessageBox::about(
                                this, tr("LASLA_tagger"),
                                tr("Ligne incorrecte : \n")+ linea); */
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
                // À revoir !
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
        /*            if (mot->cnt() == 0)
                QMessageBox::about(
                    this, tr("LASLA_tagger"),
                    tr("Ligne incomprise : \n")+ linea +
                            tr("\nL'erreur peut provoquer un plantage lors de la prochaine sauvegarde !"));
*/
        //                if (debPhrase)
        //                if (forme=="uide<n>") qDebug() << forme << analyses.size() << mot->listeFiches().size();
    }
    f.close();
    _finsPhrase << _mots.size();
    // La fin du texte est aussi la fin de la dernière phrase.

    // Je dois supprimer les analyses impossibles.
    if (!_mots.isEmpty())
    {
        int deb = 0;
        for (int iPhrase = 0; iPhrase < _finsPhrase.size();iPhrase++)
        {
            for (int i = deb; i < _finsPhrase[iPhrase];i++)
                decimer(i,deb,_finsPhrase[iPhrase]);
            deb = _finsPhrase[iPhrase];
        }
        return true;
    }
    else return false;
}

/**
 * @brief Pour sauver le résultat au format APN.
 * @param nomFichier : une chaîne de caractères avec le nom complet du fichier à sauver
 * @return false si le fichier de sortie n'a pas pu être créé.
 *
 * Sauve aussi un fichier intermédiaire au format ZPN avec tous les choix possibles.
 */
bool Lasla::sauver(QString nomFichier)
{
    QFile f(nomFichier);
    if (f.open(QFile::WriteOnly))
    {
#ifdef VERIF
        QString nf = nomFichier;
        if (nomFichier.endsWith(".APN"))
            nf.replace(".APN",".ZPN");
        QFile v(nf);
        v.open(QFile::WriteOnly);
#endif
        //            QStringList aff;
        //            int numPhr = 0;
        for (int i = 0; i < _mots.size(); i++)
        {
            QString ref = _mots[i]->getRef();
            if (ref.isEmpty())
            {
                ref = _mots[i-1]->getRef();
                if (ref.isEmpty()) ref = _mots[i-2]->getRef(); // double enclitique
                ref[3] = '='; // enclitique
            }
            //                aff << _mots[i]->Lasla(ref);
            QString toto = _mots[i]->Lasla(ref) + "\n";
            f.write(toto.toUtf8());
            //                qDebug() << toto;
#ifdef VERIF
            v.write(toto.toUtf8());
            // Il serait bon de garder la ponctuation dans le ZPN.
            toto = "!#\t" + _mots[i]->getFcsv() + "\t";
            toto.append(_mots[i]->getApres() + "\t");
            toto.append(_mots[i]->getAIresults() + "\n");
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
#ifdef VERIF
        v.close();
#endif
        return true;
    }
    return false;
}

/**
 * @brief Pour sauver le résultat au format CSV.
 *
 * Le format CSV pour le fichier produit a été introduit pour permettre
 * à l'IA (LatinBERT) de donner son avis sur le tag et l'indice d'homonymie.
 * Le séparateur est comme toujours le tab.
 * Une ligne blanche sépare les phrases.
 * Les lignes commençant par un # seront ignorées par l'IA.
 * J'en profite pour mettre toutes les solutions possibles après un # (une par ligne).
 * Je prévois une ligne commençant par #! pour y mettre les tags et indices trouvés
 * par plusieurs modèles différents. C'est l'idée, à terme, d'attirer l'attention
 * du relecteur sur les mots où les divers modèles ont donné des prédictions différentes.
 * En effet, j'ai observé que les différents modèles ne se trompent pas aux mêmes endroits.
 *
 * Mais je n'en suis pas là. Pour l'instant, je voudrais être sûr de pouvoir faire une prédiction
 * et de réconcilier cette prédiction avec les choix proposés.
 * Ça doit se conclure par la production d'un fichier APN standard.
 * Pour cela, je dois écrire un autre programme que je nommerai CSV2APN.
 * Je devrai m'inspirer de la fonction Fiche::Lasla(ref) !
 * À la fin des phrases, j'ai ajouté un . avec le tag PNT, entité qui doit disparaître.
 *
 * Le deuxième champ d'une ligne vraie (ne commençant pas par un #) est le mot
 * que LatinBert va utiliser. Il peut être différent de la forme de l'APN.
 * En particulier, si la forme est en plusieurs mots (bene dico) ou avec
 * des crochets ou des parenthèses (avec ou sans espace).
 */
void Lasla::sauveCSV(QString nomFichier)
{
        QFile f(nomFichier);
        if (f.open(QFile::WriteOnly))
        {
//            QStringList aff;
//            int numPhr = 0;
            QString prec = _mots[0]->getRef().mid(4,4);
            QString lgVide = "\t.\t.\tPNT\tPNT\n\n";
            QString nombre = "%1";
            int numTok = 0;
            QString toto;
            for (int i = 0; i < _mots.size(); i++)
            {
                numTok++;
                QString ref = _mots[i]->getRef();
                if (ref.isEmpty())
                {
                    ref = _mots[i-1]->getRef();
                    if (ref.isEmpty()) ref = _mots[i-2]->getRef(); // double enclitique
                    ref[3] = '='; // enclitique
                }
                if (prec != ref.mid(4,4))
                {
                    // Je commence une nouvelle phrase
                    toto = nombre.arg(numTok) + lgVide;
                    f.write(toto.toUtf8());
                    numTok = 1;
                    prec = ref.mid(4,4);
                }
//                aff << _mots[i]->Lasla(ref);
                toto = _mots[i]->CSV(ref);
                QString forme = toto.section("\t",0,0);
                bool simple = true;
                // la forme de la fiche choisie
                if (forme.contains("<"))
                {
                    // C'est le cas où j'ai une forme composée
                    if (forme.startsWith("<")) forme = forme.mid(forme.indexOf(">")+1).trimmed();
                    if (forme.endsWith(">")) forme = forme.mid(0, forme.indexOf("<")-1).trimmed();
                    simple = false;
                }
                if (forme.contains("("))
                {
                    // C'est le cas où j'ai une forme composée avec auxiliaire sous-entendu
                    if (forme.startsWith("(")) forme = forme.mid(forme.indexOf(")")+1).trimmed();
                    if (forme.endsWith(")")) forme = forme.mid(0, forme.indexOf("(")-1).trimmed();
                    simple = false;
                }
                if (forme.contains(" "))
                {
                    forme.remove(" "); // bene dico => benedico
                    simple = false;
                }
                if (simple && _medieval && (forme != _mots[i]->getFLem()))
                    forme = _mots[i]->getFLem();
                // Si la forme de la fiche choisie est différente de la forme lemmatisée,
                // je remets la forme d'origine. Je néglige les cas compliqués (rares ?).
                _mots[i]->setFcsv(forme);
                toto.prepend(nombre.arg(numTok) + "\t" + forme + "\t");
                f.write(toto.toUtf8());
                // Si je ne me suis pas trompé, j'ai la forme simplifiée en 2e colonne et le tag en 4e.
/*
 * Je déplace ce morceau dans Mot::CSV
                foreach (Fiche *fm, _mots[i]->listeFiches())
                {
                    toto = "#" + fm->CSV();
                    // Je voudrais ajouter des infos liés à la fiche dans ce mot...
                    // Par exemple, la proba associée à son tag.
                    QString tag = fm->getTag();
                    toto += "\t" + nombre.arg(_mots[i]->getBest(tag));
                    toto += "\n";
                    f.write(toto.toUtf8());
                }
                */
            }
            toto = nombre.arg(numTok) + lgVide;
            f.write(toto.toUtf8());
            // Je finis proprement le fichier avec une fin de phrase et une ligne blanche.
            f.close();
        }
}

QString Lasla::nettoie(QString forme)
{
    if (forme.contains("<"))
    {
        // C'est le cas où j'ai une forme composée
        if (forme.startsWith("<")) forme = forme.mid(forme.indexOf(">")+1).trimmed();
        if (forme.endsWith(">")) forme = forme.mid(0, forme.indexOf("<")-1).trimmed();
    }
    if (forme.contains("("))
    {
        // C'est le cas où j'ai une forme composée avec auxiliaire sous-entendu
        if (forme.startsWith("(")) forme = forme.mid(forme.indexOf(")")+1).trimmed();
        if (forme.endsWith(")")) forme = forme.mid(0, forme.indexOf("(")-1).trimmed();
    }
    return forme;
}

void Lasla::decimer(int i, int debPhr, int finPhr)
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
/*            if (mr.size() == _mots[i]->listeFiches().size())
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
                // Pas sûr de bien comprendre, Collatinus est appelé systématiquement au début.
                // Pourquoi l'appeler maintenant ? Relicat du premier tagueur ?
                // Déclarer ce mot comme inconnu ?
            } */
            _mots[i]->suppr(mr);
            if (_mots[i]->listeFiches().isEmpty())
            {
                unknown(_mots[i]->getFTexte()); // Peuple directement _analyses.
                _mots[i]->ajouteFiches(_analyses);
//                nb_inc++;
            }
            else if (_preProc) // Doublon ?
            {
                // Je dois regarder si la forme a été reconnue comme VNKNOWN.
                bool toutInc = true;
                for (int ii = 0; ii < _mots[i]->listeFiches().size(); ii++)
                {
                    Fiche * fiche = _mots[i]->listeFiches().at(ii);
                    if (fiche->getLemme() != "VNKNOWN") toutInc = false;
                }
                if (toutInc)
                    _inconnus.append(_mots[i]->getFTexte() + ":#" +
                                     QString::number(finPhr));
            }
        }
    }

}

/**
 * @brief Nouvelle version de la fonction de taggage
 *
 * Quand j'ai travaillé sur le tagueur de Collatinus,
 * j'ai changé d'avis sur le mode de calcul des probabilités.
 * Je ne sais pas si c'est mieux ou pire.
 *
 * Après une brève comparaison, je suis arrivé à la conclusion
 * que la version de Collatinus était mauvaise.
 * J'ai refait le calcul des probas conditionnelles du HMM,
 * et je me suis convaincu que l'ancien calcul était bon.
 * Sauf que le sélection de la "bonne" solution était étrange,
 * tout comme le traitement des fins de phrase sommaire.
 * Je garde donc une nouvelle version qui diffère moins
 * de l'ancienne que celle tirée de Collatinus.
 * Qu'il faudrait corriger !
 *
 * Dans le doute, je conserve l'ancienne version et, idéalement,
 * il faudrait évaluer les résultats obtenus avec chacune.
 * À un moment ou à un autre, je devrai compiler les deux versions
 * et les faire tourner sur les mêmes textes.
 * Je définis ou pas en tête de programme une variable LEGACY :
 * - quand elle sera définie, le programme appellera taggage_old
 * et nommera le fichier de sortie _01.APN (comme précédemment)
 * - quand sa définition sera commentée, on appellera cette nouvelle version
 * et le fichier de sortie sera nommé _02.APN.
 * Je pourrai ainsi garder les deux résultats sans me poser trop de question.
 */
void Lasla::taggage()
{
    bool blabla = false;
#ifdef VERBEUX
    QString nomFichier = _nomFichier+"_02.log";
    QFile f(nomFichier);
    blabla = f.open(QFile::WriteOnly);
    // J'ouvre un fichier "log"
//    QStringList aff;
    QTextStream aff(&f);
    aff.setCodec("UTF-8");
    QString titre = "\nEtape n° %1 : j'ajoute le mot %2\n";
    QString lg = "proba %1 pour %2\n";
    QString l = "Tag %1 avec %2 occurrences\n";
    QString ptFix = "Je détermine les tags des mots entre %1 et %2\n";
    QString lChoix = "Pour le mot n° %1, je peux imposer le tag %2 avec le score %3 (proba %4 vs %5)\n";
#endif


//    int iMoins2 = 0; // L'indice du mot i-2.
    int debSeq = 0;
    /* Chaque fois que j'arriverai à un point fixe,
     * je déterminerai les tags de tous les mots entre
     * le mot n° debSeq et le mot précédant le mot courant.
     * Cette variable debSeq sera alors actualisée.
     * */
    QStringList sequences;
    QList<double> probabilites;
    QStringList seqPart;
    QList<double> probPart;
    sequences.append("snt");
    probabilites.append(1.0);
    double branches = 1.0; // Pour savoir combien de branches a l'arbre.
    int phr = 0;
    double prMax = 1.;
    // Je suis en début de phrase : je n'ai que le tag "snt" et une proba de 1.
//    qDebug() << _mots.size();
    for (int i = 0; i < _mots.size(); i++)
    {
//                qDebug() << "Traitement du mot : " <<  i;
        Mot *mot = _mots[i];
        QStringList lTags = mot->tags(); // La liste des tags possibles pour le mot
        QList<int> lOcc = mot->nbOcc(); // Les nombres d'occurrences, dans le même ordre.
        QStringList nvlSeq; // Nouvelle liste des séquences possibles
        QList<double> nvlProba; // Nouvelle liste des probas.
        // Je dois ajouter tous les tags possibles à toutes les sequences et calculer les nouvelles probas.
        int sSeq = sequences.size();
        int sTag = lTags.size();
        if (sTag == 0) continue; // J'ignore pour l'instant les mots inconnus, cf. plus bas.
        branches *= sTag;
//        qDebug() << mot->getFTexte() << sTag;
#ifdef VERBEUX
        aff << titre.arg(i).arg(mot->getFTexte());
        for (int k = 0; k < sTag; k++)
            aff << l.arg(lTags[k]).arg(lOcc[k]);
//        aff << "";
#endif
        for (int j = 0; j < sSeq; j++)
        {
/*            QString bigr = sequences[j].right(7); // Le bigramme terminal
            qint64 prTot = 0;
            QList<qint64> pr;
            for (int k = 0; k < sTag; k++)
            {
                QString seq = bigr + "," + lTags[k];
                qint64 p = MULTIPL * lOcc[k] + 1;
                if (i > 0) p *= (MULTIPL * _trigrammes[seq] + 1);
                else p *= (MULTIPL * _bigrammes[seq] + 1);
                // Au tout début, je commence avec un snt + un tag, donc un bigramme.
                pr << p;
                prTot += p;
            }
            // J'ai tout ce qui dépend de k et la somme pour normaliser.
            if (prTot == 0)
            {
                prTot = 1;
#ifdef VERBEUX
                QString alerte = QString::number(i) + " " + QString::number(j) + " ";
                aff << alerte + mot->getFTexte() + " proba nulle ! " + sequences[j];
#endif
            }*/
            for (int k = 0; k < sTag; k++)
            {
                QString seq = sequences[j] + "," + lTags[k];
                nvlSeq.append(seq);
                nvlProba.append(probabilites[j] * proba(seq, lOcc[k]) / prMax);
                // Ce n'est pas une vraie probabilité (pas normalisée à 1)
                // car il manque la probabilité du texte.
                // Pour que ces probas ne deviennent pas trop vite trop petites,
                // je les normalise avec la plus grande des probas repérée
                // au mot précédent.
            }
        }
        // J'ai toutes les sequences et leur proba.
        // Pour chaque bigramme terminal, je ne dois garder que la séquence la plus probable.
        // En faisant ce tri, je fais une sélection sur le tag i-2.

        // Si je veux garder une info sur l'ordre des tags du mot i-2, c'est maintenant !
        if (!nvlSeq[0].right(11).startsWith("snt"))
        {
            // Toutes les nouvelles séquences sont synchrones pour les "snt".
            // Je n'en teste qu'une :
            // si le trigramme final commence par "snt", je ne fais rien.
            // Quand je suis sur le premier mot, les séquences n'ont que 7 caractères,
            // et elles commencent par "snt" : je ne fais rien.
            bool debPhrase = nvlSeq[0].right(7).startsWith("snt");
            // Je suis sur le premier mot de la phrase.
            QString tCourant;
            for (int k = 0; k < nvlSeq.size(); k++)
            {
                tCourant = nvlSeq[k].right(11).left(3);
                // C'est le tag qui commence le trigramme final.
                if (debPhrase)
                    _mots[i - 1]->setBest(tCourant, nvlProba[k]);
                else _mots[i - 2]->setBest(tCourant, nvlProba[k]);
                // Si le trigramme terminal contient un "snt" (en position médiane),
                // le premier tag qui le compose est celui du mot n° i-1.
                // Sinon, c'est le cas standard et ce tag est celui du n° i-2.
            }
        }

        if (i == _finsPhrase[phr] - 1)
        {
            // Le mot que je viens de considérer est le dernier de la phrase.
            phr++;
//            double prTot = 0.;
            for (int k = 0; k < nvlSeq.size(); k++)
            {
                nvlSeq[k] += ",snt";
                // Toutes les nouvelles séquences sont prolongées par le tag "snt"
                nvlProba[k] *= proba(nvlSeq[k],_monogrammes["snt"]);
                // et les probabilités mises à jour.
                // Une fin de phrase est le seul mot (virtuel) qui a ce tag.
            }

            // Si je veux garder une info sur l'ordre des tags du mot i-1, c'est maintenant !
            if (!nvlSeq[0].right(11).startsWith("snt"))
            {
                // Toutes les nouvelles séquences sont synchrones pour les "snt".
                // Je n'en teste qu'une : si elle commence par "snt", je ne fais rien.
                QString tCourant;
                for (int k = 0; k < nvlSeq.size(); k++)
                {
                    tCourant = nvlSeq[k].right(11).left(3);
                    // C'est le tag qui commence le trigramme final.
                        _mots[i - 1]->setBest(tCourant, nvlProba[k]);
                    // Le premier tag du trigramme terminal est celui du mot n° i-1.
                }
            }

        } // Fin du traitement d'une fin de phrase.
        // C'est le seul cas où j'ajoute deux tags d'un seul coup.
#ifdef VERBEUX
#ifdef TRES_VERBEUX
        aff << "\nJ'obtiens :\n";
        for (int k = 0; k < nvlSeq.size(); k++)
            aff << lg.arg(nvlProba[k]).arg(nvlSeq[k].right(59));
#else
        aff << "\nJ'obtiens " << nvlSeq.size() << " solutions.\n";
#endif
#endif

        // En partant de toutes les séquences enregistrées jusqu'au mot n°i,
        // je ne dois en garder qu'une (la plus probable) avec chaque
        // bigramme terminal possible.
        sequences.clear();
        probabilites.clear();
        prMax = 0.;
        // Je vais relever la plus grande des probas.
//                qDebug() << mot->forme() << nvlProba << nvlSeq;
        for (int j = 0; j < nvlSeq.size(); j++) if (nvlProba[j] > 0)
        {
            QString bigr = nvlSeq[j].right(7); // Les deux derniers tags
            QString seq = "";
            double val = -1;
            QString seq2 = "";
            double val2 = -1;
            for (int k = j; k < nvlSeq.size(); k += sTag) // Pour retrouver le bigramme terminal, il faut au moins le même dernier tag.
                if (bigr == nvlSeq[k].right(7))
                {
                    if (val2 < nvlProba[k])
                    {
                        // J'y passe au moins une fois au début.
                        // La séquence mérite la 1ère ou la 2e place.
                        if (val < nvlProba[k])
                        {
                            // 1ère place !
                            val2 = val;
                            seq2 = seq;
                            val = nvlProba[k];
                            seq = nvlSeq[k];
                        }
                        else
                        {
                            // Seulement la 2e place
                            val2 = nvlProba[k];
                            seq2 = nvlSeq[k];
                        }
                    }
                    nvlProba[k] = -1; // Pour ne pas considérer deux fois les mêmes séquences.
                }
            // val et seq correspondent aux proba et séquence qui finissent
            // avec le bigramme considéré et qui ont la plus grande proba.
            sequences << seq;
            probabilites << val;
            if (prMax < val) prMax = val;
            // val est la plus grande proba pour un bigramme terminal donné.
            // prMax sera, à la fin, la plus grande des plus grandes.
            if (val2 > 0)
            {
                // J'ai une deuxième séquence assez probable.
                sequences << seq2;
                probabilites << val2;
            }
        }
/* J'ai retenu les deux meilleures séquences pour chaque bigramme terminal.
* Si (sequences.size() < 3), cela signifie probablement que je suis arrivé
* à un "point fixe" (sauf pathologie) : un seul bigramme terminal.
* La suite de la phrase est indépendante de ce qui précède,
* puisque je considère les probabilités des trigrammes.
* Je peux choisir le meilleur tag pour chaque mot de cette séquence.
* Et recommencer avec un bigramme unique associé à la probabilité 1.
*/
//        qDebug() << mot->forme() << sSeq << sTag << nvlSeq.size() << sequences.size();
#ifdef VERBEUX
//        aff << "";
        QString b = "\nAprès élagage, j'obtiens :\n";
        aff << b;
        for (int k = 0; k < sequences.size(); k++)
            aff << lg.arg(probabilites[k]).arg(sequences[k].right(59));
        aff << "Proba max : " << prMax << "\n";
#endif

        if (sequences.size() == 0)
        {
            // Je ne devrais pas arriver ici
#ifdef VERBEUX
                QString alerte = QString::number(i) + " disparition des séquences : STOP !";
                aff << alerte;
#endif
            break;
        }
        // Puis-je avoir un seul élément ?
        // Oui, si un mot avec un tag unique suit un point fixe.
        // C'est encore un point fixe, mais sans intérêt.
        if ((sequences.size() == 2) && (i > 0) &&
                (sequences[0].right(7) == sequences[1].right(7)))
        {
            // J'ai un seul bigramme terminal.
            // Ça n'a pas d'intérêt si ce n'est que le premier mot.
            // Juste après un point fixe, je peux n'avoir que deux
            // séquences possibles sans que ce soit un point fixe.
#ifdef VERBEUX
            aff << "Point fixe !\n";
            aff << ptFix.arg(debSeq).arg(i);
#endif
            QString seq0 = sequences[0];
            QString seq1 = sequences[1];
            double pr0 = probabilites[0];
            double pr1 = probabilites[1];
            seqPart << seq0;
            probPart << pr0;
            seqPart << seq1;
            probPart << pr1;
            double score = (pr0 - pr1) / (pr0 + pr1);
            QString bigr = seq0.right(7);
            for (int ii = debSeq; ii < i; ii++)
            {
                // La sequence retenue me permet de déterminer le choix des tags.
                QString tt0 = seq0.left(3);
                QString tt1 = seq1.left(3);
                seq0 = seq0.mid(4);
                seq1 = seq1.mid(4);
                if (tt0 == "snt")
                {
                    // "snt" est un faux tag : fin de phrase.
                    tt0 = seq0.left(3);
                    tt1 = seq1.left(3);
                    seq0 = seq0.mid(4);
                    seq1 = seq1.mid(4);
                }
                double sc = score;
                if (tt0 == tt1) sc = 1.0;
                _mots[ii]->setChoix(tt0, sc);
#ifdef VERBEUX
            aff << lChoix.arg(ii).arg(tt0).arg(sc)
                   .arg(pr0).arg(pr1);
            aff << _mots[ii]->getChoix() << "\t" << _mots[ii]->Lasla(" ") << "\n";
#endif
            }
            if (bigr.contains("snt")) debSeq = i;
            else debSeq = i - 1;
            // S'il y a le pseudo-tag "snt" dans le bigramme terminal,
            // c'est le tag du mot n° i qui y figure (et lui seulement).
            // Sinon, il commence par le tag du mot n° i-1.
            sequences.clear();
            probabilites.clear();
            sequences << bigr;
            probabilites << 1.0;
        }
    } // fin du texte.

    int iFin = _mots.size() - 1;
    QString tCourant;
    // Normalement, le texte se termine avec une fin de phrase.
    if (!sequences[0].endsWith("snt"))
    {
        for (int k = 0; k < sequences.size(); k++)
        {
            sequences[k].append(",snt");
            probabilites[k] *= proba(sequences[k],_monogrammes["snt"]);
        }
        if (!sequences[0].right(11).startsWith("snt"))
        {
            // Toutes les nouvelles séquences sont synchrones pour les "snt".
            // Je n'en teste qu'une : si elle commence par "snt", je ne fais rien.
            for (int k = 0; k < sequences.size(); k++)
            {
                tCourant = sequences[k].right(11).left(3);
                // C'est le tag qui commence le trigramme final.
                _mots[iFin - 1]->setBest(tCourant, probabilites[k]);
                // Le premier tag du trigramme terminal est celui du mot n° i-1.
            }
        }
        // Si jamais j'avais oublié le point final de mon texte,
        // je l'ajoute ici et je détermine les probas du mot n° iFin-1.
    }
    for (int k = 0; k < sequences.size(); k++)
    {
        tCourant = sequences[k].right(7).left(3);
        // C'est le tag qui commence le bigramme final.
        _mots[iFin]->setBest(tCourant, probabilites[k]);
        // Le premier tag du bigramme terminal est celui du mot n° iFin (le dernier).
    }
    // Je détermine les probas du mot final (n° iFin).

    // Le tri final !
    QString seq = "";
    double val = -1;
    QString seq2 = "";
    double val2 = -1;
    for (int i = 0; i < sequences.size(); i++)
        if (val2 < probabilites[i])
        {
            if (val < probabilites[i])
            {
                val2 = val;
                seq2 = seq;
                val = probabilites[i];
                seq = sequences[i];
            }
            else
            {
                val2 = probabilites[i];
                seq2 = sequences[i];
            }
        }
    seqPart << seq;
    probPart << val;
    double score = 1.;
    if (val2 > 0)
    {
        seqPart << seq2;
        probPart << val2;
        score = (val - val2) / (val + val2);
    }
    else seq2 = seq; // Je n'ai pas de deuxième séquence...
    for (int ii = debSeq; ii < _mots.size(); ii++)
    {
        // La sequence retenue me permet de déterminer le choix des tags.
        QString tt0 = seq.left(3);
        QString tt1 = seq2.left(3);
        seq = seq.mid(4);
        seq2 = seq2.mid(4);
        if (tt0 == "snt")
        {
            // "snt" est un faux tag : fin de phrase.
            tt0 = seq.left(3);
            tt1 = seq2.left(3);
            seq = seq.mid(4);
            seq2 = seq2.mid(4);
        }
        double sc = score;
        if (tt0 == tt1) sc = 1.0;
        _mots[ii]->setChoix(tt0, sc);
#ifdef VERBEUX
        aff << lChoix.arg(ii).arg(tt0).arg(sc)
               .arg(val).arg(val2);
        aff << _mots[ii]->getChoix() << "\t" << _mots[ii]->Lasla(" ") << "\n";
#endif
    }

}

/**
 * @brief Ancienne version de la routine de taggage.
 *
 * Je l'ai remplacée par celle que j'ai recopiée de Collatinus::tagueur.cpp
 * Il pourrait être intéressant de comparer les résultats obtenus avec chacune
 * de ces versions : il suffit de modifier un taggage() en taggage_old()
 * à la presque fin de la routine Serveur::traite() –aujourd'hui à la ligne 910.
 *
 */
void Lasla::taggage_old()
{
    bool blabla = false;
#ifdef VERBEUX
    QString nomFichier = _nomFichier+"_01.log";
    QFile f(nomFichier);
    blabla = f.open(QFile::WriteOnly);
    // J'ouvre un fichier "log"
    QStringList aff;
    QString titre = "Etape n° %1 : j'ajoute le mot %2";
    QString lg = "proba %1 pour %2";
    QString l = "Tag %1 avec %2 occurrences";
    QString lChoix = "Pour le mot n° %1, je peux imposer le tag %2 avec le score %3 (proba %4 vs %5)\n";
#endif
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
    int i = (_mots.size()-1) / 100;

    for (i = 0; i < _mots.size(); i++)
    {
        QStringList nvlSeq; // Nouvelle liste des séquences possibles
        QList<double> nvlProba; // Nouvelle liste des probas.
        Mot *mot = _mots[i];

        QStringList lTags = mot->tags(); // La liste des tags possibles pour le mot
        // en tenant compte des éventuelles formes conditionnelles.
        QList<int> lOcc = mot->nbOcc();
        // Je dois ajouter tous les tags possibles à toutes les sequences et calculer les nouvelles probas.
        sSeq = sequences.size();
        sTag = lTags.size();
        branches *= sTag;
        double nvlPMax = 0.;
#ifdef VERBEUX
        aff << titre.arg(i).arg(mot->getFTexte());
        for (int k = 0; k < sTag; k++)
            aff << l.arg(lTags[k]).arg(lOcc[k]);
        aff << "";
#endif
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
/*            qint64 prTot = 0;
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
#ifdef VERBEUX
        aff << "J'obtiens :";
        for (int k = 0; k < nvlSeq.size(); k++)
            aff << lg.arg(nvlProba[k]).arg(nvlSeq[k].right(59));
#endif
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
#ifdef VERBEUX
        aff << "";
        aff << "Après élagage, j'obtiens :";
        for (int k = 0; k < sequences.size(); k++)
            aff << lg.arg(probabilites[k]).arg(sequences[k].right(59));
        aff << "";
#endif
        if (sequences.size() < 2)
        {
#ifdef VERBEUX
            aff << "Point fixe !\n";
#endif
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
#ifdef VERBEUX
            aff << lChoix.arg(iMotAttente).arg(tAtt).arg(score)
                   .arg(pPrime).arg(pMax);
#endif
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

#ifdef VERBEUX
        aff << "";
        if (blabla) f.write(aff.join("\n").toUtf8());
        aff.clear();
#endif
    } // Fin du texte

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
#ifdef VERBEUX
    aff << "";
    aff << "J'ai fini !";
    aff << lg.arg(logPr).arg(logBr) + " branches (en log en base 2).";
    aff << "";
    aff << seq;
    if (blabla) f.write(aff.join("\n").toUtf8());
    f.close();
#endif
    // Il me reste à reporter dans les derniers mots le tag retenu.
    for (int i = iMotAttente; i < _mots.size(); i++)
    {
        QString t = seq.mid(pTagAttente,3);
        _mots[i]->setChoix(t,1);
        pTagAttente += 4;
        while ((pTagAttente < seq.size()) && (seq.mid(pTagAttente,3) == "snt")) pTagAttente += 4;
    }
}

/**
 * @brief Lasla::plusFreq
 * @param f1
 * @param f2
 * @return si la fiche f1 est plus fréquente que f2
 *
 * Cette fonction ordonne les fiches selon plusieurs critères.
 * Elle met les formes multiples avant les formes simples.
 * Elle met les formes elliptiques tout en bas.
 * Sinon c'est l'ordre donné par le nombre d'occurrences.
 *
 */
bool Lasla::plusFreq (Fiche * f1, Fiche * f2)
{
    if (f1->getCmpl().isEmpty() && !f2->getCmpl().isEmpty())
        return false;
    if (!f1->getCmpl().isEmpty() && f2->getCmpl().isEmpty())
        return true;
    if (f1->getCmpl().isEmpty() && f2->getCmpl().isEmpty())
    {
        if (f1->getForme().contains("(") && !f2->getForme().contains("("))
            return false; // Ellipse
        if (!f1->getForme().contains("(") && f2->getForme().contains("("))
            return true;
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
QString Lasla::tag(QString code9)
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

void Lasla::unknown(QString m)
{
    // Une fonction pour générer toutes les analyses possibles pour une forme inconnue.
    if (_preProc)
    {
        // Je dois recueillir la forme et le numéro de la phrase où elle est.
        QString inc = m + ":";
        inc.append(QString::number(_finsPhrase.size()));
        _inconnus.append(inc);
        // Rmq : la phrase n'est pas finie.
    }

    bool adverb = (m.endsWith("e") || m.endsWith("o") || m.endsWith("iter") || m.endsWith("im"));
    // Pour les adverbes, je regarde si la forme se termine par une fin courante...

    m.append(",VNKNOWN, ,%3,%1,%2");
    // m était la forme et devient le prototype de la ligne pour créer une Fiche.

    foreach (QString t, _monogrammes.keys())
//        if (t[0].isUpper() && !t.contains("e") && (_monogrammes[t] > 2000))
        if (((t[0] == 'A') || (t[0] == 'B') || (t[0] == 'C'))
                && (_monogrammes[t] > 2000))
        {
            // Je ne considére que les noms, verbes et adjectifs.
            QString cc9 = t;
            if (cc9[0] == 'B')
            {
                // C'est un verbe.
                cc9.insert(1, "    ");
                cc9.append("  ");
            }
            else
            {
                // Nom ou adjectif.
                cc9.insert(1, " ");
                cc9.append("     ");
            }
            Fiche * fiche = new Fiche(m.arg(t).arg(_monogrammes[t] / 100).arg(cc9));
//            _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
            if (!_preProc) ajouterFiche(fiche);
            _analyses.append(fiche);
        }
    if (adverb)
    {
        // J'ajoute l'adverbe
        Fiche * fiche = new Fiche(m.arg("M  ").arg(_monogrammes["M  "] / 100).arg("M        "));
//        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
        if (!_preProc) ajouterFiche(fiche);
        /* Lors du préprocessing, je veux garder tous les inconnus et les phrases
         * qui les contiennent. Il ne faut donc pas créer les fiches correspondantes.
         * Je vais mettre un booléen global pour décider quoi faire.
         * */
        _analyses.append(fiche);
    }
// J'ai choisi d'attribuer les tags qui apparaissent plus de 2000 fois
// dans les textes du LASLA, dans les catégories noms, verbes, adjectifs et adverbes.
// Pour cette dernière catégorie, je regarde si la forme se termine
// comme la majorité des adverbes (-e, -iter, -im et -o).
// C'est un peu au dessus de la médiane qui se situe aux environs de 1600.
// J'insère les fiches obtenues dans la collection _fiches
// pour que cette forme soit "reconnue" si elle figure une 2e fois dans le texte,
// sauf lors du préprocessing.
}

void Lasla::setPreProc(bool val)
{
    _preProc = val;
    if (val) _inconnus.clear();
    // Quand je commence une phase de préprocessing,
    // j'efface les inconnus rencontrés précédemment.
}

QStringList Lasla::inconnus()
{
    QMultiMap <QString,QString> range;
    QString clef;
    QString valeur;
    int numPhrase;
    QString inc;
    _inconnus.removeDuplicates();
    // Si une forme inconnue apparaît deux fois dans la même phrase,
    // Je n'en garde qu'une.
    for (int j=0; j<_inconnus.size(); j++)
    {
        inc = _inconnus[j];
        clef = inc.section(":", 0, 0); // Le mot non-reconnu.
        valeur = inc.section(":", 1, 1); // Le numéro de la phrase le contenant.
        if (valeur.startsWith("#"))
        {
            // C'est le numéro du mot qui est à la fin de la phrase.
            // Peut-être que je n'arriverai jamais ici.
            int finPhr = valeur.mid(1).toInt();
            for(int i = 0; i < _finsPhrase.size() ; i++)
                if (finPhr == _finsPhrase[i]) numPhrase = i - 1;
        }
        else
            numPhrase = valeur.toInt();
        valeur = "";
        int i=0;
        if (numPhrase > 1) i = _fPhrEl[numPhrase - 1] + 1;
        for (; i < _fPhrEl[numPhrase]; i++)
            valeur += _elements[i];
        QString sep = _elements[_fPhrEl[numPhrase]];
        if (sep.contains(" ")) sep = sep.section(" ",0,0);
        if (sep.contains("\n")) sep = sep.section("\n",0,0);
        valeur += sep;
// C'est la phrase.
        valeur.replace("\n", " § ");
        valeur.prepend(clef + "\t");
        range.insertMulti(clef.toLower(), valeur);
    }
    return range.values();
}

bool Lasla::fichesEgales(Fiche *f, Fiche *fiche)
{
    // Je dois regarder si les fiches f et fiche décrivent la même solution
    if (f->getLemme() != fiche->getLemme()) return false;
    if (f->getIndice() != fiche->getIndice()) return false;
    if (f->getCode() != fiche->getCode()) return false;
    if (f->getForme() != fiche->getForme()) return false;
    if (f->getClef() != fiche->getClef()) return false;
    return true;
}

/**
 * @brief Lasla::creerFiche Je crée une nouvelle fiche
 * @param ligne : la chaîne de caractères qui me permet de
 * créer la fiche.
 * @return un pointeur vers la fiche, nouvelle ou pas
 *
 * Je crée une nouvelle fiche avec la @a ligne qui m'est donnée.
 * Je vérifie que cette fiche n'existe pas encore dans _fiches.
 * Si elle existe déjà, je retourne le pointeur vers cette fiche existante
 * (en ayant supprimé celle nouvellement créée).
 * Si elle n'existait pas encore dans _fiches, je l'y mets
 * et j'écris la ligne qu'il faut dans le dico auto
 * pour qu'elle existe la prochaine fois.
 */
Fiche * Lasla::creerFiche(QString ligne)
{
    Fiche *fiche = new Fiche(ligne);
    // Pour ajouter une fiche dans _fiches si elle n'est pas encore présente
    bool nvle = true;
    QList<Fiche*> lf = _fiches.values(fiche->getClef());
//    Fiche* f;
    int i = 0;
//    foreach (Fiche* f, _fiches.values(fiche->getClef()))
  //      nvle = nvle && !fichesEgales(f, fiche);
    while ((i < lf.size()) && nvle)
    {
        if (fiche->egale(lf[i])) nvle = false;
        else i++;
    }
    // Je dois regarder si les fiches f et fiche décrivent la même solution
    if (nvle)
    {
        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
        QString nomDico = dicoAuto;
//        if (manuel) nomDico = dicoPerso;
#ifdef AVEC_DICO
        ligne.append("\n");
        QFile fDic(nomDico);
        if (fDic.open(QIODevice::Append|QIODevice::Text))
        {
 /*           if (_primoL && manuel)
            {
                fDic.write(_date.toUtf8());
                _primoL = false;
            }*/
            if (_primoA/* && !manuel*/)
            {
                fDic.write(_date.toUtf8());
                _primoA = false;
            }
            fDic.write(ligne.toUtf8());
            fDic.close();
        }
#endif
        if (!_ficMed.isEmpty())
        {
            // Je dois ajouter le nouvelle clef !
            QString clef = fiche->getClef();
            QString cleMed = transfMed(clef);
            if ((clef != cleMed) && !_ficMed.values(cleMed).contains(clef))
                _ficMed.insert(cleMed, clef);
        }
        return fiche;
    }
    else
    {
        delete fiche;
        if (i < lf.size()) return lf[i];
        else return nullptr;
    }
    // J'ai créé une fiche qui ne servira pas, je dois la supprimer !
}

void Lasla::ajouterFiche(Fiche *fiche, bool manuel)
{
    // Pour ajouter une fiche dans _fiches si elle n'est pas encore présente
    bool nvle = true;
    foreach (Fiche* f, _fiches.values(fiche->getClef()))
        nvle = nvle && !fichesEgales(f, fiche);
    // Je dois regarder si les fiches f et fiche décrivent la même solution
    if (nvle)
    {
        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
        QString nomDico = dicoAuto;
        if (manuel) nomDico = dicoPerso;
#ifdef AVEC_DICO
        QString ligne = fiche->getLigne() + "\n";
        QFile fDic(nomDico);
        if (fDic.open(QIODevice::Append|QIODevice::Text))
        {
            if (_primoL && manuel)
            {
                fDic.write(_date.toUtf8());
                _primoL = false;
            }
            if (_primoA && !manuel)
            {
                fDic.write(_date.toUtf8());
                _primoA = false;
            }
            fDic.write(ligne.toUtf8());
            fDic.close();
        }
#endif
        if (!_ficMed.isEmpty())
        {
            // Je dois ajouter le nouvelle clef !
            QString clef = fiche->getClef();
            QString cleMed = transfMed(clef);
            if ((clef != cleMed) && !_ficMed.values(cleMed).contains(clef))
                _ficMed.insert(cleMed, clef);
        }
    }
//    else delete fiche;
    // J'ai créé une fiche qui ne servira pas, je dois la supprimer !
    // Pas si sûr...
}

double Lasla::proba(QString seqTags, qint64 nbOcc)
{
    // Pour une séquence de tags, je calcule la proba conditionnelle
    // P(t_k | t_k-2, t_k-1) . P(f_k | t_k)
    // La première est estimée par le rapport entre les nombres du
    // trigramme final et du bigramme qui le commence.
    // La seconde est le rapport entre les nombres d'occurrences du
    // la forme avec ce tag et de celles du tag.
    // Ce n'est pas une "vraie" probabilité conditionnelle,
    // car il manque une normalisation qui serait la probabilité
    // que l'auteur ait écrit le texte.

    double p = MuLTIPL1 * nbOcc + 1;
    p /= (MuLTIPL1 * _monogrammes[seqTags.right(3)] + 120000);
//    p /= (MuLTIPL1 * _monogrammes[seqTags.right(3)] + _fiches.size());
    // Si j'ajoute 1 au nombre d'occurrences d'une forme, je dois ajouter
    // le nombre de formes différentes possibles pour que la probabilité
    // P(f_k | t_k) soit normée.
    // Il y a ~120 000 formes différentes dans listForm9.csv
    // Faut-il remplacer ce nombre de formes total par celui
    // des formes qui acceptent ce tag ?
    // Double problème : Collatinus connaît beaucoup plus de formes
    // et comment évaluer un tel nombre.

    if (seqTags.size() > 10)
    {
        p *= (MULTIPL * _trigrammes[seqTags.right(11)] + 1);
        p /= (MULTIPL * _bigrammes[seqTags.right(11).left(7)] + 178);
//        p /= (MULTIPL * _bigrammes[seqTags.right(11).left(7)] + _monogrammes.size() - 1);
        // Si j'ajoute 1 au nombre d'occurrences d'un trigramme, je dois ajouter
        // le nombre de monogrammes possibles (sauf "snt")
        // pour que la probabilité P(t_k | t_k-2, t_k-1) soit normée.
    }
    else
    {
        // Au tout début, je n'ai qu'un bigramme !
        p *= (MULTIPL * _bigrammes[seqTags] + 1);
        p /= (MULTIPL * _monogrammes[seqTags.left(3)] + 178);
//        p /= (MULTIPL * _monogrammes[seqTags.left(3)] + _monogrammes.size() - 1);
    }
    return p;
}

/**
 * @brief Lasla::ajoute
 * @param lg : une chaîne décrivant une fiche LASLA
 * ou un lemme Collatinus avec traduction.
 *
 * Les formes inconnues sont traitées dans MainWindow
 * avec comme résultat une nouvelle fiche LASLA ou
 * un nouveau lemme pour Collatinus.
 * Les lignes correspondantes sont sauvées dans le dico perso
 * ou à la fin de lem_ext.la et lem_ext.fr.
 * Mais pour que ce soit immédiatement utilisable,
 * il faut les charger (sinon, il faudra attendre le prochain démarrage).
 *
 */
void Lasla::ajoute(QString lg, int numMot)
{
    if (lg.isEmpty()) return;
    if (lg.contains("\t"))
    {
        // C'est un lemme pour Collatinus avec traduction après le Tab
        _lemCore->ajouteLem(lg);
    }
    else
    {
        // C'est une fiche LASLA à ajouter (abréviation).
        if (lg.endsWith("\n")) lg.chop(1);
        Fiche * fiche = new Fiche(lg);
        ajouterFiche(fiche, numMot == -1);
        if (numMot != -1)
        {
            // Je dois ajouter cette nouvelle fiche au mot d'indice numMot
            // et à tous les mots qui ont la même forme.
            QString m = _mots[numMot]->getFTexte();
            // Il faut parcourir le texte pour ajouter cette analyse à tous les mots identiques.
            for (int i = 0; i < _mots.size(); i++)
                if (_mots[i]->getFTexte() == m)
                    _mots[i]->ajouteFiche(fiche);
            _mots[numMot]->setChoix(_mots[numMot]->cnt() - 1);
            // Pour le mot en cours d'examen, je valide cette dernière fiche.
        }
    }

}

/**
 * @brief Lasla::listeModeles
 * @return la liste des modèles de _lemCore
 *
 * C'est une fonction relais qui appelle
 * la fonction correspondante dans LemCore
 * et ajoute le pseudo-modèle "LASLA"
 * qui permet de saisir une fiche LASLA pour une forme anomale.
 *
 * Une autre possibilité serait d'ajouter des entrées dans irreg.la
 */
QStringList Lasla::listeModeles(QString forme)
{
    QStringList res = _lemCore->listModels(forme);
    res.prepend("LASLA");
    return res;
}

QString Lasla::codes5a9(QString code5)
{
    QString code9 = QString(9, ' ');
    if (code5.size() != 5) return "Error";
    char c0 = code5[0].toLatin1();
    char c1 = code5[1].toLatin1();
    char c2 = code5[2].toLatin1();
    if ((c0 > '0') && (c0 < '6') && code5[2].isLetter())
    {
        // Les Cas/Personne et Nombre sont codés de la même façon.
        int casPers = 2;
        if ((c0 == '5') && (code5[3] > '0') &&(code5[3] < '4'))
            casPers = 8;
        // Pour un verbe conjugué, la personne va à la fin.
        // Les formes verbales déclinées ont un cas en 3e position.
        if (c2 == 'Z') code9[casPers] = '8';
        else if (c2 < 'J')
        {
            code9[casPers] = QChar(code5[2].unicode() - 16);
            code9[3] = '1'; // Singulier
        }
        else
        {
            code9[casPers] = QChar(code5[2].unicode() - 25);
            code9[3] = '2'; // Pluriel
        }
    }
    switch (c0)
    {
    case '1' : code9[0] = 'A'; // Substantif
        code9[1] = code5[1];
        break;
    case '2' : code9[0] = 'C'; // Adjectif
        code9[4] = '1'; // positif
        if (code5[1].isDigit()) code9[1] = code5[1];
        else if ((c1 != '&') && (c1 != '-'))
        { // Les formes comme "ulterior" et "ultimus" ont un code "C cn1    ".
            if (c1 < 'J')
            {
                code9[1] = QChar(code5[1].unicode() - 16);
                code9[4] = '2'; // Comparatif
            }
            else
            {
                code9[1] = QChar(code5[1].unicode() - 25);
                code9[4] = '3'; // Superlatif
            }
        }
        break;
    case '3' : code9[0] = 'D'; // Numéral
        break;
    case '4' : code9[0] = QChar(code5[1].unicode() + 20); // Pronoms
        // à vérifier ! '1' -> 'E' à '8' -> 'L'.
        break;
    case '5' : code9[0] = 'B'; // Verbe
        code9[5] = code5[3]; // mode
        code9[6] = code5[4]; // temps
        code9[7] = '1'; // voix = active
        if (code5[1].isDigit()) code9[1] = code5[1];
        else if (c1 < 'J')
            {
                code9[1] = QChar(code5[1].unicode() - 16);
                code9[7] = '2'; // passive
            }
            else if (c1 < 'P')
            {
                code9[1] = QChar(code5[1].unicode() - 25);
                code9[7] = '3'; // déponent
            }
        else if (c1 == 'S')
        {
            code9[1] = '2';
            code9[7] = '4'; // semi-déponent
        }
        else
        {
            code9[1] = '3';
            code9[7] = '4'; // semi-déponent
        }
        break;
    case '6' : code9[0] = 'M'; // Adverbes
        if (c1 == '&') code9[4] = '2'; // Comparatif
        else if (c1 == '-') code9[4] = '3'; // Superlatif
        else if (c1 < '6') code9[4] = '1'; // Positif
        else code9[0] = QChar(code5[1].unicode() + 24); // adv. rel., interr., neg. etc.
        break;
    case '7' : code9[0] = 'R'; // Préposition
        break;
    case '8' : if (code5[1] == '1') code9[0] = 'S'; // Conj. coord.
        else code9[0] = 'T'; // Conj. subord.
        break;
    case '9' : code9[0] = 'U'; // Interjection
        break;
    default : code9[0] = '#';
    }
    return code9;
}

QList<Fiche*> Lasla::getAnalyses(QString forme)
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

void Lasla::separEncl(int numMot, int numPhrase)
{
    QString m = _mots[numMot]->getFTexte();
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
        QString r = _mots[numMot]->getRef();
        int rg = _mots[numMot]->getRang();
        if (m.endsWith("que")) _mots[numMot] = motQue;
        else if (m.endsWith("cum")) _mots[numMot] = motCum;
        else if (m.endsWith("ue")) _mots[numMot] = motVe;
        else if (m.endsWith("ne")) _mots[numMot] = motNe;
        m = analyses[0]->getForme();
        // Il faut décimer les analyses qui nécessitent des mots qui ne sont pas dans la phrase.
        Mot *mm = new Mot(m,m,analyses,rg,r);
        _mots.insert(numMot,mm);
        decimer(numMot,_finsPhrase[numPhrase - 2],_finsPhrase[numPhrase - 1] + 1);
        for (int i=0; i < _finsPhrase.size(); i++)
            if (_finsPhrase[i] > numMot) _finsPhrase[i]++;
    }

}

void Lasla::fusionPhrase(int numPhrase)
{
    // Je supprime une phrase
    // Renuméroter les ref.
    for (int i = _finsPhrase[numPhrase - 1]; i < _mots.size(); i++)
        if (!_mots[i]->getRef().isEmpty())
    {
        QString rc = _mots[i]->getRef();
        int nouv = rc.mid(4,4).toInt() + 9999;
        rc.replace(4,4,QString::number(nouv).mid(1,4));
        _mots[i]->setRef(rc);
    }
    _finsPhrase.removeAt(numPhrase - 1);
    if (!_fPhrEl.isEmpty())
        _fPhrEl.removeAt(numPhrase - 1);

}

bool Lasla::ajouterMot(QString m, int nm, int numPhrase, QString r, QString ligne)
{
    if (ligne.isEmpty())
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
        return false;
/*        QString ligne = saisie(m);
        Fiche * fiche = new Fiche(ligne);
        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
        _analyses << fiche;*/
    }
    if (_analyses.size()>1)
        qSort (_analyses.begin(), _analyses.end(), plusFreq);
    Mot *mm = new Mot(m,m,_analyses,-1,r);
    _mots.insert(nm,mm);
//    decimer(nm,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1] + 1);
    // Quand je sépare un mot, j'en ajoute deux et je ne dois décimer qu'à la fin.
    for (int i=numPhrase - 1; i < _finsPhrase.size(); i++)
        _finsPhrase[i]++;
    }
    else
    {
        Fiche * fiche = new Fiche(ligne);
        ajouterFiche(fiche);
//        _fiches.insert(fiche->getClef(),fiche); // Pour les suivants
        _analyses << fiche;
        Mot *mm = new Mot(m,m,_analyses,-1,r);
        _mots.insert(nm,mm);
    //    decimer(nm,_finsPhrase[_numPhrase - 2],_finsPhrase[_numPhrase - 1] + 1);
        // Quand je sépare un mot, j'en ajoute deux et je ne dois décimer qu'à la fin.
        for (int i=numPhrase - 1; i < _finsPhrase.size(); i++)
            _finsPhrase[i]++;
    }
    return true;
}

void Lasla::changerRef(int numMot, int numPhrase, QString m)
{
    QString r = _mots[numMot]->getRef();
    if (!m.isEmpty() && (m != r) && (m.count('/') > 0))
    {
        // r est l'ancienne ref et m est la nouvelle
        if (r.section("/",0,0) != m.section("/",0,0))
        {
            // La référenciation a changé, je dois l'appliquer à tout le texte
            if (r.left(3) != m.left(3))
                for (int i = numMot + 1; i < _mots.size(); i++)
                {
                    _mots[i]->setRef(m.left(3) + _mots[i]->getRef().mid(3));
                }
            if (r.mid(4,4) != m.mid(4,4))
            {
                // Le numéro de phrase a changé
                int decal = m.mid(4,4).toInt() - r.mid(4,4).toInt();
                for (int i = numMot + 1; i < _mots.size(); i++)
                    if (!_mots[i]->getRef().isEmpty())
                {
                    QString rc = _mots[i]->getRef();
                    int nouv = rc.mid(4,4).toInt() + decal + 10000;
                    rc.replace(4,4,QString::number(nouv).mid(1,4));
                    _mots[i]->setRef(rc);
                }
                if ((numMot > 0) && (decal == 1))
                {
                    // J'ai ajouté une phrase
                    _finsPhrase.insert(numPhrase - 1,numMot);
                    if (!_fPhrEl.isEmpty())
                        _fPhrEl.insert(numPhrase - 1,_mots[numMot]->getRang() - 1);
                }
                if ((numMot == _finsPhrase[numPhrase - 2]) && (decal == -1))
                {
                    // J'ai supprimé une phrase
                    _finsPhrase.removeAt(numPhrase - 2);
                    if (!_fPhrEl.isEmpty())
                        _fPhrEl.removeAt(numPhrase - 2);
                   // _numPhrase--;
                    // Je ne sais pas remonter cette information.
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
                    if (OK) for (int i = numMot + 1; i < _mots.size(); i++)
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
                    if (OK) for (int i = numMot + 1; i < _mots.size(); i++)
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
                    if (OK) for (int i = numMot + 1; i < _mots.size(); i++)
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
        _mots[numMot]->setRef(m);
    }

}

void Lasla::setDate(QString date)
{
    _date = date;
    _primoL = true;
    _primoA = true;
    // Je sépare deux séries de fiches LASLA : perso.csv et auto.csv.
    // Le premier pour les fiches crées par le philologue.
    // Le deuxième pour les fiches crées automatiquement par Collatinus.
}

/**
 * @brief Lasla::estAux sert à repérer un auxiliaire
 * qui pourrait former une forme composée avec un participe.
 * @return Un booléen qui est true si une des analyses possibles
 * est le verbe EO sous sa forme "iri"
 * ou le verbe SVM à l'indicatif, au subjonctif ou à l'infinitif.
 */
/*bool Lasla::estAux()
{
    if (_analyses.isEmpty()) return false;
    if (_analyses[0]->getClef() == "iri") return true;
    bool rep = false;
    for (int i = 0; i < _analyses.size(); i++)
//        if ((_analyses[i]->getLemme() == "SVM") &&
//                (_analyses[i]->getClef() != "quem"))
            // En réalité, il y a un tas de formes en "st" qui comportent
            // un verbe SVM qui seront purgées plus tard.
            // Je pourrais essayer de déplacer la création des listes,
            // mais je ne suis pas sûr de connaître le bon endroit.
        if (_analyses[i]->getLemme() == "SVM")
        {
            QString code = _analyses[i]->getCode();
            if (code.startsWith("B") &&
                    ((code[5] == '1') || (code[5] == '3') || (code[5] == '7')))
             rep = true;
            // Le verbe SVM à l'indicatif, au subjonctif ou à l'infinitif.
        }
    return rep;
}
*/
/**
 * @brief Lasla::estPart sert à repérer un participe
 * qui pourrait former une forme composée avec un auxiliaire.
 * @return Un booléen qui est true si une des analyses possibles
 * est un participe passé ou futur au nominatif ou à l'accusatif.
 *
 * En principe, je devrais aussi chercher un supin en -um,
 * mais il a la même forme qu'un PPP.
 */
/*bool Lasla::estPart()
{
    if (_analyses.isEmpty()) return false;
    bool rep = false;
    for (int i = 0; i < _analyses.size(); i++)
    {
        QString code = _analyses[i]->getCode();
        if (code.startsWith("B") && ((code[2] == '1') || (code[2] == '3'))
                && (code[5] == '4') && ((code[6] == '3') || (code[6] == '4')))
            rep = true;
        // J'ai un verbe au nominatif ou à l'accusatif
        // au participe futur ou parfait.
    }
    return rep;
}
*/
/**
 * @brief Lasla::vontEnsemble
 * @param numAux : un entier qui est l'indice du mot qui est potentiellement auxiliaire
 * @param numPart : un entier qui est l'indice du mot qui est potentiellement participe
 *
 * Cette routine vérifie si les auxiliare et participe putatifs
 * peuvent aller ensemble.
 * Je vérifie également que ces deux mots ne figurent pas déjà
 * dans la liste de fiches de l'autre.
 * Si tel est le cas, je crée les formes composées
 * correspondantes et je les mets dans les possibilités des mots.
 */
void Lasla::vontEnsemble(int numAux, int numPart)
{
    // J'ai, en principe, deux mots qui pourraient être un auxiliaire
    // et un participe qui iraient ensemble pour faire une forme verbale composée.
    // Si ça marche, je dois vérifier que les fiches correspondantes n'existent pas encore
    // pour les créer et les ajouter à _fiches et aux listes de fiches des deux mots.
    Mot * mAux = _mots[numAux];
    Mot * mPart = _mots[numPart];
    QList<Fiche*> fAux = mAux->listeFiches();
    QList<Fiche*> fPart = mPart->listeFiches();
    Fiche *fiche;
    // Avant de commencer, je devrais regarder si la forme composée
    // n'a pas déjà été identifiée comme telle.
/* Non ! c'est une mauvaise idée.
 * Si j'ai un groupe PPP en a et "esse", il peut y avoir
 * des analyses diverses : s'il y en a une, cela ne
 * signifie pas qu'elles y sont toutes.
 * Je dois donc faire ce test à la fin,
 * quand j'ai trouvé des solutions.
 *     bool contenue = false;
    for(int i = 0; i < fAux.size(); i++)
    {
        QString fa = fAux[i]->getForme();
        if (fa.contains(" ") && fa.contains("<"))
        {
            fa.remove("<");
            fa.remove(">");
            for(int j = 0; j < fPart.size(); j++)
            {
                QString fp = fPart[j]->getForme();
                fp.remove("<");
                fp.remove(">");
                if (fa == fp) contenue = true;
            }
        }
    }
    if (contenue) return;
    */
//    qDebug() << numAux << mAux->getFTexte() << numPart << mPart->getFTexte() << mPart->getFLem();

    QString lAux;
    QString lPart;
    if ((mAux->getFLem() == "iri") && (mPart->getFLem().endsWith("um")))
    {
        // Pourrait être un infinitif futur passif, avec un supin en um.
        int iSup = -1;
        for(int i = 0; i < fPart.size(); i++)
        {
            // Je cherche un supin.
            QString code = fPart[i]->getCode();
            if ((code[0] == 'B') && (code[5] == '8')) iSup = i;
        }
        if (iSup == -1) return;
        QString code = fPart[iSup]->getCode().mid(0,2);
        QString lem = fPart[iSup]->getLemme();
        if (lem.endsWith("OR"))
            code.append("   733 ");
        else if (estSemidep(lem))
            code.append("   734 ");
        else code.append("   732 ");
        lAux = ",EO,2,#        ,#  ,1";
        lPart = "," + lem + ","
                + fPart[iSup]->getIndice() + "," + code + ",B8 ,1";
        if (fPart[iSup]->getForme().contains("<"))
            lAux = "";
        else if (numAux < numPart)
        {
            lAux.prepend("iri <" + fPart[iSup]->getForme() + ">");
            lPart.prepend("<iri> " + fPart[iSup]->getForme());
        }
        else
        {
            lAux.prepend("<" + fPart[iSup]->getForme() + "> iri");
            lPart.prepend(fPart[iSup]->getForme() + " <iri>");
        }
//        qDebug() << lPart << lAux;
        /* Normalement, j'ai mes deux lignes pour créer les fiches.
         * Je dois vérifier si elles existent déjà.
         * Si la fiche existe dans l'ensemble des fiches connues,
         * elle doit être dans les analyses du mot correspondant.
         * Il suffit donc de la chercher dans les fiches du mot,
         * mais je dois retourner un entier (>0 si j'ai créé une nouvelle fiche)
         * pour pouvoir ajouter cette même fiche à _fiches.
         * */
/*        int iii = mAux->existe(lAux);
//        if (iii != -1)
        {
            Fiche *fiche = mAux->listeFiches().at(iii); // C'est la nouvelle fiche.
            QString str = fiche->getClef();
            if (str.contains("/") && !str.startsWith("$"))
            {
                // Il y a ambiguïté.
                ficheAmbigue(str, fiche);
                // qDebug() << str << fiche->getForme() << fiche->getCmpl();
            }
            else ajouterFiche(fiche);
            // Je dois vérifier à nouveau, car je peux avoir
            // deux fois la même forme composée dans le texte.
              //  _fiches.insert(str, fiche);
        }
        iii = mPart->existe(lPart);
        if (iii != -1)
        {
            Fiche *fiche = mPart->listeFiches().at(iii); // C'est la nouvelle fiche.
            QString str = fiche->getClef();
            if (str.contains("/") && !str.startsWith("$"))
            {
                // Il y a ambiguïté.
                ficheAmbigue(str, fiche);
                // qDebug() << str << fiche->getForme() << fiche->getCmpl();
            }
            else ajouterFiche(fiche);
        }
*/
        if (!lAux.isEmpty())
        {
            fiche = creerFiche(lAux);
            if ((fiche != nullptr) && !mAux->listeFiches().contains(fiche))
                mAux->ajouteFiche(fiche);
            fiche = creerFiche(lPart);
            if ((fiche != nullptr) && !mPart->listeFiches().contains(fiche))
                mPart->ajouteFiche(fiche);
        }
        return; // J'ai fini avec l'infinitif futur passif
    }
    // Pour éviter un niveau de {}, j'ai fait un return plutôt qu'un else.
    for(int i = 0; i < fAux.size(); i++)
    {
        QString ca = fAux[i]->getCode();
        if ((fAux[i]->getLemme() == "SVM") && (ca[0] == 'B') &&
                ((ca[5] == '1') || (ca[5] == '3')))
        {
            // J'ai la bonne fiche pour une forme conjuguée.
            QString fa = fAux[i]->getForme();
            // J'ai tout ce qu'il faut pour l'auxiliaire.
            for(int j = 0; j < fPart.size(); j++)
            {
                QString cp = fPart[j]->getCode();
                if ((cp[0] == 'B') && (cp[2] == '1') && (cp[3] == ca[3])
                        && (cp[5] == '4') && (cp[6] == '4'))
                {
                    // Participe parfait au nominatif accordé en nombre avec l'auxiliaire.
                    QString code = cp.mid(0,5);
                    code[2] = ' ';
                    code[4] = ' ';
                    // Sans cas, ni degré
                    code.append(ca[5] + QChar(ca[6].unicode() + 3));
                    // mode de l'auxiliaire. temps augmenté de 3.
                    if (fPart[j]->getLemme().endsWith("OR"))
                        code.append("3" + ca[8]);
                    else if (estSemidep(fPart[j]->getLemme()))
                        code.append("4" + ca[8]);
                    else code.append("2" + ca[8]);
                    // Déponent ou passif; personne de l'auxiliaire
                    QString fp = fPart[j]->getForme();
                    lPart = "," + fPart[j]->getLemme() + ","
                            + fPart[j]->getIndice() + "," + code + ",B"
                            + ca[5] + " ,1";
                    lAux = ",SVM,2,#        ,#  ,1";
                    if (fa.contains("<") || fp.contains("<"))
                    {
                        // Cas de <clipeo>st et autre <xxx>st
                        // à traiter en allant chercher le mot précédent.
                        if ((fa.contains("<") && fp.contains("<")) && (numAux == numPart + 1))
                        {
                            // Faire des tests supplémentaires ?
                            QString f1 = fa;
                            QString f2 = fp;
                            f1.remove("<");
                            f1.remove(">");
                            f2.remove("<");
                            f2.remove(">");
                        }
                        else lAux = "";
                        // C'est un moyen expéditif pour dire que je n'en veux pas.
                    }
                    else if (numAux < numPart)
                    {
                        lAux.prepend(fa + " <" + fp + ">");
                        lPart.prepend("<" + fa + "> " + fp);
                    }
                    else
                    {
                        lAux.prepend("<" + fp + "> " + fa);
                        lPart.prepend(fp + " <" + fa + ">");
                    }
//                    qDebug() << lPart << lAux;
                    // Normalement, j'ai mes deux lignes pour créer les fiches
                    if (!lAux.isEmpty())
                    {
                        fiche = creerFiche(lAux);
                        if ((fiche != nullptr) && !mAux->listeFiches().contains(fiche))
                            mAux->ajouteFiche(fiche);
                        fiche = creerFiche(lPart);
                        if ((fiche != nullptr) && !mPart->listeFiches().contains(fiche))
                            mPart->ajouteFiche(fiche);
                    }
                }
            } // Fin de boucle sur les analyses du PPP
        }
        else if ((fAux[i]->getLemme() == "SVM") &&
                 (ca[0] == 'B') && (ca[5] == '7'))
         {
             // J'ai la bonne fiche pour un infinitif.
             QString fa = fAux[i]->getForme();
             for(int j = 0; j < fPart.size(); j++)
             {
                 QString cp = fPart[j]->getCode();
                 if ((cp[0] == 'B') && (cp[5] == '4')
                         && ((cp[2] == '1') || (cp[2] == '3')))
                 {
                     QString code = cp.mid(0,5);
                     cp[4] = ' ';
                     if (cp[6] == '4')
                     {
                         // Participe parfait au nominatif ou à l'accusatif.
                         code.append(ca[5] + QChar(ca[6].unicode() + 3));
                         // mode de l'auxiliaire. temps augmenté de 3.
                         if (fPart[j]->getLemme().endsWith("OR"))
                             code.append("3 ");
                         else if (estSemidep(fPart[j]->getLemme()))
                             code.append("4 ");
                         else code.append("2 ");
                         // Déponent ou passif; pas de personne
                         QString fp = fPart[j]->getForme();
                         lPart = "," + fPart[j]->getLemme() + ","
                                 + fPart[j]->getIndice() + "," + code + ",B7 ,1";
                         lAux = ",SVM,2,#        ,#  ,1";
                         if (fa.contains("<") || fp.contains("<"))
                             lAux = "";
                         else if (numAux < numPart)
                         {
                             lAux.prepend(fa + " <" + fp + ">");
                             lPart.prepend("<" + fa + "> " + fp);
                         }
                         else
                         {
                             lAux.prepend("<" + fp + "> " + fa);
                             lPart.prepend(fp + " <" + fa + ">");
                         }
//                         qDebug() << lPart << lAux;
                         if (!lAux.isEmpty())
                         {
                             fiche = creerFiche(lAux);
                             if ((fiche != nullptr) && !mAux->listeFiches().contains(fiche))
                                 mAux->ajouteFiche(fiche);
                             fiche = creerFiche(lPart);
                             if ((fiche != nullptr) && !mPart->listeFiches().contains(fiche))
                                 mPart->ajouteFiche(fiche);
                         }
                     }
                     else if (cp[6] == '3')
                     {
                         // Participe futur au nominatif ou à l'accusatif.
                         if (fPart[j]->getLemme().endsWith("OR"))
                             code.append("733 "); // exclu ?
                         else if (estSemidep(fPart[j]->getLemme()))
                             code.append("734 ");
                         else code.append("731 ");
                         // Déponent ou actif; pas de personne
                         QString fp = fPart[j]->getForme();
                         lPart = "," + fPart[j]->getLemme() + ","
                                 + fPart[j]->getIndice() + "," + code + ",B7 ,1";
                         lAux = ",SVM,2,#        ,#  ,1";
                         if (fa.contains("<") || fp.contains("<"))
                             lAux = "";
                         else if (numAux < numPart)
                         {
                             lAux.prepend(fa + " <" + fp + ">");
                             lPart.prepend("<" + fa + "> " + fp);
                         }
                         else
                         {
                             lAux.prepend("<" + fp + "> " + fa);
                             lPart.prepend(fp + " <" + fa + ">");
                         }
//                         qDebug() << lPart << lAux;
                         if (!lAux.isEmpty())
                         {
                             fiche = creerFiche(lAux);
                             if ((fiche != nullptr) && !mAux->listeFiches().contains(fiche))
                                 mAux->ajouteFiche(fiche);
                             fiche = creerFiche(lPart);
                             if ((fiche != nullptr) && !mPart->listeFiches().contains(fiche))
                                 mPart->ajouteFiche(fiche);
                         }
                     }
                 }
             } // Fin de boucle sur les analyses du PPP
         }
    }
}

void Lasla::ficheAmbigue(QString str, Fiche *fiche)
{
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
        ajouterFiche(fiche);
// _fiches.insert(ecl[1],fiche);
    }
    else
    {
        // Je n'ai pas retrouvé la forme reconstruite, donc la clef est "bene".
        fiche->setClef(ecl[0]);
        fiche->setCmpl(Fiche::cmpl(fiche->getForme(),ecl[0]));
        ajouterFiche(fiche);
// _fiches.insert(ecl[0],fiche);
    }
}

bool Lasla::estSemidep(QString lem)
{
    return Fiche::semidep.contains(lem);
}

void Lasla::ecrireDico(QString ligne, bool manuel)
{
    QString nomDico = dicoAuto;
    if (manuel) nomDico = dicoPerso;
    ligne.append("\n");
    QFile fDic(nomDico);
    if (fDic.open(QIODevice::Append|QIODevice::Text))
    {
        if (_primoL && manuel)
        {
            fDic.write(_date.toUtf8());
            _primoL = false;
        }
        if (_primoA && !manuel)
        {
            fDic.write(_date.toUtf8());
            _primoA = false;
        }
        fDic.write(ligne.toUtf8());
        fDic.close();
    }
    /*        if (!_ficMed.isEmpty())
{
    // Je dois ajouter le nouvelle clef !
    QString clef = fiche->getClef();
    QString cleMed = transfMed(clef);
    if ((clef != cleMed) && !_ficMed.values(cleMed).contains(clef))
        _ficMed.insert(cleMed, clef);
}
return fiche;*/
}

/**
 * @brief Lasla::CSV2APN
 * @param nom : le nom du fichier à traiter (avec le chemin)
 * @return une chaîne de caractères ?
 *
 * Je reprends ici ce que f(ais)ait le programme indépendant CSV2APN.
 * Ce dernier avait été écrit pour tourner avec le démon,
 * et reprenait les données d'origine dans le fichier @a nom .csv,
 * ainsi que les prédictions de l'IA dans les fichiers
 *  @a nom _tag.csv, @a nom _ind.csv et @a nom _sub.csv.
 *  Ici, les mots et leurs analyses possibles sont déjà en mémoire,
 *  ce qui simplifie grandement le problème.
 *  Ça permettra aussi d'examiner les paires auxiliaires + PPP,
 *  pour choisir la solution la plus proche.
 */
QString Lasla::CSV2APN(QString nom)
{
    _warnings.clear();
    QString rep = "";
    QString linea;
    QString forme;
    if (nom.endsWith(".csv")) nom.chop(4);
    QFile ficTag(nom + "_tag.csv");
//    bool tagOK = ficTag.open(QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluTag(&ficTag);
    int i = 0;
    if (ficTag.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        while (!fluTag.atEnd())
        {
            linea = fluTag.readLine ();
            if (linea.endsWith("\n")) linea.chop(1);
            while (!fluTag.atEnd() &&
                   (linea.isEmpty() || linea.startsWith("#") || linea.startsWith(".\t")))
            {
                linea = fluTag.readLine ();
                if (linea.endsWith("\n")) linea.chop(1);
            }
            if (i < _mots.size())
            {
                forme = _mots[i]->getFcsv().toLower();
                if (forme != linea.section("\t",0,0))
                {
                    qDebug() << "Pb tag " << forme << linea;
                }
                /*else*/ _mots[i]->setAItag(linea.section("\t",1,1));
                i++;
            }
        }
        ficTag.close();
    }
    if (i != _mots.size()) qDebug() << "Pb tag " << i << _mots.size();

    i = 0;
    ficTag.setFileName(nom + "_ind.csv");
    if (ficTag.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        while (!fluTag.atEnd())
        {
            linea = fluTag.readLine ();
            if (linea.endsWith("\n")) linea.chop(1);
            while (!fluTag.atEnd() &&
                   (linea.isEmpty() || linea.startsWith("#") || linea.startsWith(".\t")))
            {
                linea = fluTag.readLine ();
                if (linea.endsWith("\n")) linea.chop(1);
            }
            if (i < _mots.size())
            {
                forme = _mots[i]->getFcsv().toLower();
                if (forme != linea.section("\t",0,0))
                {
                    qDebug() << "Pb ind " << forme << linea;
                }
                /*else*/ _mots[i]->setAIind(linea.section("\t",1,1));
                i++;
            }
        }
        ficTag.close();
    }
    if (i != _mots.size()) qDebug() << "Pb ind " << i << _mots.size();

    i = 0;
    ficTag.setFileName(nom + "_sub.csv");
    if (ficTag.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        while (!fluTag.atEnd())
        {
            linea = fluTag.readLine ();
            if (linea.endsWith("\n")) linea.chop(1);
            while  (!fluTag.atEnd() &&
                    (linea.isEmpty() || linea.startsWith("#") || linea.startsWith(".\t")))
            {
                linea = fluTag.readLine ();
                if (linea.endsWith("\n")) linea.chop(1);
            }
            if (i < _mots.size())
            {
                forme = _mots[i]->getFcsv().toLower();
                if (forme != linea.section("\t",0,0))
                {
                    qDebug() << "Pb sub " << forme << linea;
                }
                /*else*/ _mots[i]->setAIsub(linea.section("\t",1,1));
                i++;
            }
        }
        ficTag.close();
    }
    if (i != _mots.size()) qDebug() << "Pb sub " << i << _mots.size();
    for (i = 0; i < _mots.size(); i++)
    {
        rep = _mots[i]->getPossible();
        if (rep.startsWith("@"))
        {
            choisir(i, rep);
            // J'ai probablement plusieurs paires auxiliaire + PPP dans la phrase.
            rep = rep.section("\n",1);
        }
        if (!rep.isEmpty())
            _warnings.append(rep);
    }

    return _warnings;
}

void Lasla::choisir(int iMot, QString rep)
{
    // J'ai probablement plusieurs paires auxiliaire + PPP dans la phrase.
    QStringList eclats = rep.section("\n", 0, 0).split("\t");
    // C'est la liste des items dans _lFiches du mot.
    // Il s'agit de mesurer la distance entre les éléments de la paire,
    // et de choisir la plus courte.
    QList<int> indice;
    QList<int> dist;
    QList<Fiche*> listeF = _mots[iMot]->listeFiches();
    for(int i = 1; i < eclats.size(); i++)
    {
        int j = eclats[i].toInt();
        indice << j;
        Fiche *fic = listeF.at(j);
        QString forme = fic->getForme();
        QString autre = forme.section("<", 1, 1).section(">", 0, 0);
        int dd = -1;
        if (forme.startsWith("<"))
        {
            // Chercher l'autre moitié avant.
            int im = 1;
            while((im < 11) && (dd < 0) && (im <= iMot))
            {
                bool OK = _mots[iMot - im]->getFLem() == autre;
                if (OK) dd = im;
//                else qDebug() << autre << _mots[iMot - im]->getFLem();
                im++;
            }
            if (dd > 0) dist << dd;
            else dist << 20;
        }
        else if (forme.endsWith(">"))
        {
            // Chercher l'autre moitié après.
            int im = 1;
            while((im < 11) && (dd < 0) && (im + iMot < _mots.size()))
            {
                bool OK = _mots[iMot + im]->getFLem() == autre;
                if (OK) dd = im;
//                else qDebug() << autre << _mots[iMot + im]->getFLem();
                im++;
            }
            if (dd > 0) dist << dd;
            else dist << 20;
        }
        else if (!forme.contains("("))
            qDebug() << "Pb forme " << forme << iMot << j;
    }

    int dMin = 20;
    int iMin = -1;
    for(int i = 0; i < dist.size(); i++)
        if (dMin > dist[i])
        {
            iMin = i;
            dMin = dist[i];
        }
    if (iMin > 0)
    {
        _mots[iMot]->setChoix(indice[iMin]);
    }
}
