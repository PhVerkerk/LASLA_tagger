#include "mot.h"

Mot::Mot(QString ft, QString fl, QList<Fiche *> lf, int rg, QString refs)
{
    _fTexte = ft; // Forme du texte
    _fLem = fl; // Forme lemmatisée
    _rang = rg; // La position du mot dans le texte d'origine.
    _lFiches = lf; // Liste des fiches
    _refs = refs; // Références
    // Lorsque j'ouvre un APN, je récupère les 3 infos non-calculables
    // séparées par un '/' : ref de l'œuvre, de la ligne et le code de subordination.
    _lTags.clear();
    _lNbr.clear();
    _lBestProbas.clear();
    _tagChoisi = "";
    _score = 0;
    _choix = -1;

    _tagAI = "";
    _indAI = "";
    _subAI = "";
    _apres = "";
    _normal = true;
//    _lTags << lf[0]->getTag();
//    _lNbr << 0;
/*    foreach (Fiche * f, lf)
    {
        QString tag = f->getTag();
        if (_lTags.contains(tag))
        {
            int i=0;
            int imax = _lTags.size();
            while ((_lTags[i] != tag) && (i<imax)) i+=1;
            _lNbr[i] += f->getNbr();
            // Si le tag existe déjà, je somme les occurrences
        }
        else
        {
            _lTags << tag;
            _lNbr << f->getNbr();
            _lBestProbas << 0.0;
            // Le tag n'existe pas encore : je l'ajoute à la liste avec le nombre d'occurrences
        }
    }
*/
}

Mot::~Mot()
{
    // Pas sûr de savoir quoi faire...
}


void Mot::ajouteFiches(QList<Fiche*> lf)
{
    _lFiches.append(lf);
}

void Mot::ajouteFiche(Fiche *f)
{
    _lFiches.append(f);
/*    QString tag = f->getTag();
    if (_lTags.contains(tag))
    {
        int i=0;
        int imax = _lTags.size();
        while ((_lTags[i] != tag) && (i<imax)) i+=1;
        _lNbr[i] += f->getNbr();
        // Si le tag existe déjà, je somme les occurrences
    }
    else
    {
        _lTags << tag;
        _lNbr << f->getNbr();
        _lBestProbas << 0.0;
        // Le tag n'existe pas encore : je l'ajoute à la liste avec le nombre d'occurrences
    }*/
}

void Mot::videTags()
{
    // Je dois vider la liste de tags pour la reconstruire ultérieurement.
    _lTags.clear();
    _lNbr.clear();
    _lBestProbas.clear();
}

QStringList Mot::tags()
{
    if (_lTags.isEmpty())
    {
        // Si la liste est vide, je dois la construire.
        _lTags.clear();
        _lNbr.clear();
        _lBestProbas.clear();
        bool cond = _lFiches[0]->estCondit();
        foreach (Fiche * f, _lFiches)
            if ((cond && f->estCondit()) || (!cond && !f->estCondit()))
        {
                // Si la première fiche est conditionnelle,
                // je ne garde que les tags des fiches conditionnelles.
            QString tag = f->getTag();
            if (_lTags.contains(tag))
            {
                int i=0;
                int imax = _lTags.size();
                while ((_lTags[i] != tag) && (i<imax)) i+=1;
                _lNbr[i] += f->getNbr();
                // Si le tag existe déjà, je somme les occurrences
            }
            else
            {
                _lTags << tag;
                _lNbr << f->getNbr();
                _lBestProbas << 0.0;
                // Le tag n'existe pas encore : je l'ajoute à la liste avec le nombre d'occurrences
            }
        }
    }
    return _lTags;
}

QList<int> Mot::nbOcc()
{
    return _lNbr;
}

QList<Fiche*> Mot::listeFiches ()
{
    return _lFiches;
}

QString Mot::Lasla(QString ref)
{
    int chx = _choix;
    if (_choix < 0) chx = 0;
    /*
     * Le choix est fait par le tagueur, avant _choix = -1.
     * Mais, les solutions sont ordonnées :
     * la plus probable est la première, d'indice 0.
     * En passant par la variable intermédiaire chx,
     * je conserve la valeur -1 qui indique que le tagueur n'a pas encore choisi.
     * Inutile ?
     */
    if (_lFiches.isEmpty() || (_choix >= _lFiches.size()))
        return "????";
    if (_refs.isEmpty())
    {
        // Les enclitiques n'ont pas de ref propre.
        // Je reprends donc la ref du mot qui précède (qui porte l'enclitique).
        if (ref.count('/') == 2)
            ref = ref.section('/',0,1);
        // Toutefois, si j'ai un verbe avec un code de subordination,
        // je ne dois pas le recopier pour l'enclitique.
        return _lFiches[chx]->Lasla(ref);
    }
    return _lFiches[chx]->Lasla(_refs);
}

QString Mot::CSV(QString ref)
{
    if ((_choix < 0) && (_lFiches.size() == 1)) _choix = 0;
    if (_lFiches.isEmpty() || (_choix >= _lFiches.size()) || (_choix < 0))
        return "????\n";
    // Ça ne devrait pas arriver !
    if (ref.count('/') == 2)
        ref = ref.section('/',0,1);
    // J'enlève le "/**" ou autre code de subordination que je n'ai pas à ce stade.
    ref.prepend(_lFiches[_choix]->CSV() + "\t");
    ref += "\t" + _apres + "\n#!\t";
    ref += _lFiches[_choix]->getTag() + "|" + _lFiches[_choix]->getIndice() + "\t";
    ref += getAIresults() + "\n";
    // Je termine la ligne principale avec la ponctuation.
    // Attention ! Les enclitiques n'ont pas d'après :
    // la ponctuation est sur le mot qui les porte.
    // Je prépare aussi
    // la ligne où ajouter les prédictions de Latin-Bert.
    // J'y mets au début les choix du tagueur.
    foreach (Fiche *fm, _lFiches)
    {
        ref += "#\t" + fm->CSV() + "\t";
        // Je voudrais ajouter des infos liés à la fiche dans ce mot...
        // Par exemple, la proba associée à son tag.
//        QString tag = fm->getTag();
        ref += nombre.arg(getBest(fm->getTag())) + "\n";
    }
    return ref;
}

QString Mot::formeFiche()
{
    if (_choix < 0) _choix = 0;
    return _lFiches[_choix]->getForme();
}

QString Mot::getInfo(int nFiche, bool complet)
{
    if (_lFiches.count() > nFiche)
    {
        QString rep = _lFiches[nFiche]->info();
        if (complet) rep.append(_refs);
        QString t = _lFiches[nFiche]->getTag();
        int i=0;
        int imax = _lTags.size();
        while ((i<imax) && (_lTags[i] != t)) i+=1;
        if (i<imax)
        {
            double p1 = _lBestProbas[i];
            double p2 = getBest(-i-2);
            if (p1 + p2 == 0) return rep;
            rep.append(" %1");
            return rep.arg((p1-p2)/(p1+p2));
        }
        return rep;
    }
    return _fTexte+" ??? " + _refs;
}

void Mot::ajoute(QString t)
{
    _fTexte += " " + t;
}

void Mot::setFTexte(QString t)
{
    _fTexte = t;
}

void Mot::setFLem(QString t)
{
    // Pour pouvoir corriger la forme lemmatisée, qui n'a pas forcément sa forme classique.
    _fLem = t;
}

void Mot::setChoix(QString t, double sc)
{
    // Ce que je choisis, c'est un tag...
    _tagChoisi = t;
    _score = sc;
    // ... Mais, ce qui m'intéresse, c'est l'indice de la solution dans la liste des possibles.
    _choix = 0;
    // À revoir !
    // S'il y a deux solutons avec le même tag, je dois choisir la plus probable !
    if (_lFiches.size() > 1)
    {
//    while ((_choix < _lFiches.size()) && (_lFiches[_choix]->getTag() != t))
  //      _choix++;
        QList<int> indices;
        for (int ii = 0; ii < _lFiches.size(); ii++)
            if (_lFiches[ii]->getTag() == t) indices.append(ii);
        // J'ai la liste des indices des fiches qui ont ce tag.
        int nMax = -1;
        if (indices.isEmpty())
        {
            _choix = -1;
            qDebug() << t << " introuvable pour " << _fTexte << " n° " << _rang;
        }
        else for (int ii = 0; ii < indices.size(); ii++)
            if (_lFiches[indices[ii]]->getNbr() > nMax)
            {
                _choix = indices[ii];
                nMax = _lFiches[indices[ii]]->getNbr();
            }
    }
}

QStringList Mot::motsRequis()
{
    QStringList mr;
    foreach (Fiche *f, _lFiches)
    {
        if (f->getForme().contains(" <")) mr.append(" <" + f->getForme().section(" <",1));
        if (f->getForme().contains("> ")) mr.append(f->getForme().section("> ",0,0) + "> ");
    }
    mr.removeDuplicates();
//    if (!mr.isEmpty())
//    qDebug() << _fTexte << "requis" << mr;
    return mr;
}

bool Mot::contient(QString m)
{
    bool c = false;
    foreach (Fiche *f, _lFiches)
    {
        if (m.startsWith(" "))
            c = c || (f->getForme().endsWith(m));
        else c = c || (f->getForme().startsWith(m));
        // Il peut y avoir des problèmes si m contient plusieurs mots.
        // Pour "<satis esse> facturum", je dois trouver "satis" et "esse" séparément :
        // dans "<satis> esse <facturum>" et "satis <esse facturum>".
    }
//    if (m.contains("bene")) qDebug() << m << c;
    return c;
}

void Mot::suppr(QStringList motsAbsents)
{
//    qDebug() << _fTexte << "absents" << motsAbsents;
    QList<Fiche*> backup = _lFiches;
    foreach (QString m, motsAbsents)
    {
        // Je dois supprimer toutes les fiches qui demandent l'un de ces mots.
        int i = 0;
        while (i < _lFiches.size())
        {
            Fiche * f = _lFiches[i];
            if (f->getForme().contains(m))
            {
                if (_choix == i) i++; // Je ne peux pas supprimer la fiche choisie.
                else
                {
                    if (_choix > i) _choix--;
                    // Si j'ai choisi une fiche et que j'en supprime une avant,
                    // je dois diminuer _choix de 1 pour pointer sur la bonne fiche.
                    _lFiches.removeAt(i);
                }
            }
            else i++;
        }
    }
    if (_lFiches.isEmpty()) _lFiches = backup;
}

QString Mot::courte(QString f)
{
    // Il faut distinguer les mots requis ailleurs (entre crochets),
    // sous-entendus (entre parenthèses) et ceux qui suivent.
    // f.contains(" <") || f.contains("> ") || f.contains(") ") || f.contains(" (")
    QString fCourte = f;
    if (fCourte.contains(" <")) fCourte = fCourte.section(" <",0,0);
    if (fCourte.contains(" (")) fCourte = fCourte.section(" (",0,0);
    if (fCourte.contains("> ")) fCourte = fCourte.section("> ",1);
    if (fCourte.contains(") ")) fCourte = fCourte.section(") ",1);
    // Il me reste donc le ou les mots essentiels.
    return fCourte;
}

void Mot::designe(QString f, QString l, QString ind, QString c)
{
    // J'ai créé ce mot en lisant un fichier APN.
    // Je dois choisir l'analyse qui a les bons éléments (forme, lemme, indice et code).
    // La forme sauvée peut contenir plusieurs mots.
    QString fCourte = courte(f);
    int i = 0;
    while (i < _lFiches.size())
    {
        bool egaux = (f == _lFiches[i]->getForme());
        egaux = egaux && (l == _lFiches[i]->getLemme());
        egaux = egaux && (ind == _lFiches[i]->getIndice());
        egaux = egaux && (c == _lFiches[i]->getCode());
        if (egaux)
        {
            _choix = i;
            i++;
        }
        else
        {
            // Dois-je supprimer la fiche ?
            bool suppr = !_lFiches[i]->getForme().contains(fCourte,Qt::CaseInsensitive);
            if (suppr)
            {
                // Encore une vérif !
                QString ff = _lFiches[i]->getForme();
                if (ff.contains("<") && ff.contains(">"))
                {
                    ff.remove("<");
                    ff.remove(">");
                    if (!ff.contains(fCourte,Qt::CaseInsensitive))
                         _lFiches.removeAt(i); // oui !
                    else i++; // non !
                }
                else if ((ff.startsWith("V") && fCourte.startsWith("u")) ||
                        (ff.startsWith("u") && fCourte.startsWith("V"))) i++; // non !
                else _lFiches.removeAt(i); // oui !
            }
            else
            {
                // La forme de la fiche contient le ou les mots recherchés.
                if (fCourte.toLower() == courte(_lFiches[i]->getForme()).toLower())
                    i++; // non !
                else _lFiches.removeAt(i); // oui !
            }
        }
    }
}

void Mot::setChoix(int n)
{
    // Ici, j'impose un choix manuel.
    if (n < _lFiches.size())
    {
        _choix = n;
        _tagChoisi = "man";
        _score = 0;
    }
}

int Mot::getChoix()
{
    if (_choix < 0) return 0;
    return _choix;
}

int Mot::getScore()
{
    return _score;
}

QString Mot::getFLem()
{
    return _fLem;
}

QString Mot::getRef()
{
    return _refs;
}

void Mot::setRef(QString ref)
{
    _refs = ref;
}

QString Mot::getFTexte()
{
    return _fTexte;
}

QString Mot::getTag(int n)
{
    if (n < _lFiches.size()) return _lFiches[n]->getTag();
    return "";
}

QString Mot::bulle(int n)
{
    if (n < _lFiches.size())
    {
        QString t = _lFiches[n]->getTag();
        int i=0;
        int imax = _lTags.size();
        while ((i<imax) && (_lTags[i] != t)) i+=1;
        QString rep = _lFiches[n]->humain() + " : %1 occ.";
        if (i<imax)
        {
            rep.append(" Score %2, probas %3 / %4");
            double p1 = _lBestProbas[i];
            double p2 = getBest(-i-2);
            if ((p1 + p2) == 0.0)
            {
                return rep.arg(_lFiches[n]->getNbr()).arg("?").arg(p1).arg(p2);
            }
            return rep.arg(_lFiches[n]->getNbr()).arg((p1-p2)/(p1+p2)).arg(p1).arg(p2);
        }
        return rep.arg(_lFiches[n]->getNbr());
    }
    return "";
}

int Mot::getOcc(int n)
{
    if (n < _lFiches.size()) return _lFiches[n]->getNbr();
    return 0;
}

int Mot::getNbr(int n)
{
    if (n < _lNbr.size()) return _lNbr[n];
    return 0;
}

/**
 * @brief Mot::setBest
 * @param t : un tag
 * @param pr : une probabilité
 * Associe la probabilité pr au tag t si pr est meilleure que la valeur actuelle de _lBestProbas
 */
void Mot::setBest(QString t, double pr)
{
    int i=0;
    int imax = _lTags.size();
    if (!_lTags.contains(t))
        qDebug() << getFTexte() << _lTags << t << pr;
    else
    while ((i<imax) && (_lTags[i] != t)) i+=1;
    if ((i<imax) && (pr > _lBestProbas[i]))
        _lBestProbas[i] = pr; // Je conserve la plus forte proba associée au tag
}

/**
 * @brief Mot::getBest
 * @param n : un entier
 * @return une probabilité.
 * Cette proba vient de la liste _lBestProbas qui a été établie lors du taggage.
 * si n ≥ 0, retourne la proba associée au tag n° n (dans la liste)
 * si n = -1, retourne la plus forte proba
 * si n < -1, retourne la plus forte proba exceptée celle de -n-2
 */
double Mot::getBest(int n)
{
    if ((_lBestProbas.size() == 1) && (_lBestProbas[0] == 0))
    {
        if (n > -1) return 1.0;
        else return 0.0;
    }
    if ((n < _lBestProbas.size()) && (n > -1)) return _lBestProbas[n];
    else
    {
        double pr = 0.0;
        for (int i=0;i<_lBestProbas.size();i++)
            if ((pr < _lBestProbas[i]) && (n+i+2 != 0)) pr = _lBestProbas[i];
        return pr;
    }
}

/**
 * @brief Mot::getBest
 * @param t : un tag
 * @return la probabilité associée à ce tag.
 */
double Mot::getBest(QString t)
{
    for (int i = 0; i<_lBestProbas.size();i++)
        if (t == _lTags[i]) return _lBestProbas[i];
    return 0.0;
}

int Mot::getRang()
{
    return _rang;
}

int Mot::cnt()
{
    return _lFiches.size();
}

const QString Mot::nombre = "%1";

/**
 * @brief Mot::estAux sert à repérer un auxiliaire
 * qui pourrait former une forme composée avec un participe.
 * @return Un booléen qui est true si une des analyses possibles
 * est le verbe EO sous sa forme "iri"
 * ou le verbe SVM à l'indicatif, au subjonctif ou à l'infinitif.
 */
bool Mot::estAux()
{
    if (_lFiches.isEmpty()) return false;
    if (_lFiches[0]->getClef() == "iri") return true;
    bool rep = false;
    for (int i = 0; i < _lFiches.size(); i++)
    {
        Fiche *f = _lFiches[i];
        if (f->getLemme() == "SVM" && !f->getForme().contains("<")
                && !f->getForme().contains("("))
        {
            // Je ne veux pas de formes composées !
            QString code = f->getCode();
            if (code.startsWith("B") &&
                    ((code[5] == '1') || (code[5] == '3') || (code[5] == '7')))
             rep = true;
            // Le verbe SVM à l'indicatif, au subjonctif ou à l'infinitif.
        }
    }
    return rep;
}

/**
 * @brief Mot::estPart sert à repérer un participe
 * qui pourrait former une forme composée avec un auxiliaire.
 * @return Un booléen qui est true si une des analyses possibles
 * est un participe passé ou futur au nominatif ou à l'accusatif.
 *
 * En principe, je devrais aussi chercher un supin en -um,
 * mais il a la même forme qu'un PPP.
 */
bool Mot::estPart()
{
    if (_lFiches.isEmpty()) return false;
    bool rep = false;
    for (int i = 0; i < _lFiches.size(); i++)
    {
        QString code = _lFiches[i]->getCode();
        if (code.startsWith("B") && ((code[2] == '1') || (code[2] == '3'))
                && (code[5] == '4') && ((code[6] == '3') || (code[6] == '4')))
            rep = true;
        // J'ai un verbe au nominatif ou à l'accusatif
        // au participe futur ou parfait.
    }
    return rep;
}

/**
 * @brief Mot::existe conclut la recherche et l'appariement
 * d'un auxiliaire et d'un participe.
 * @param ligne : une chaîne de caractère qui est le prototype
 * de la fiche à créer si elle n'existe pas.
 * @return un entier qui est le rang de la nouvelle fiche dans _lFiches
 * si ladite fiche n'existait pas encore, -1 si elle existait déjà.
 *
 * J'ai créé le prototype d'une fiche pour les formes verbales composées.
 * Je dois regarder si cette fiche existe déjà.
 * Si elle existe dans l'ensemble des fiches, elle existe aussi
 * dans _lFiches de ce mot.
 * Si elle n'existe pas dans _lFiches, je dois la créer
 * et retourner le rang qu'elle occupe dans _lFiches
 * pour pouvoir l'introduire dans les fiches de Lasla.
 *
 */
int Mot::existe(QString ligne)
{
    int rep = -1;
    Fiche *fiche = new Fiche(ligne);
    // Pour ajouter une fiche dans _fiches si elle n'est pas encore présente
    bool nvle = true;
    for(int i = 0; i < _lFiches.size(); i++)
        nvle = nvle && !fiche->egale(_lFiches[i]);
    // Je dois regarder si les fiches f et fiche décrivent la même solution
    if (nvle)
    {
        rep = _lFiches.size();
        _lFiches.append(fiche); // Pour les suivants
    }
    else
    {
        delete fiche;
    }
    // J'ai créé une fiche qui ne servira pas, je dois la supprimer !
//    QStringList eclats = ligne.split(",");
    // forme, lemme, indice, code en 9, tag, 1
    return rep;
}

void Mot::setApres(QString separateur)
{
    _apres = separateur;
}

QString Mot::getApres()
{
    return _apres;
}

void Mot::setAIind(QString ind)
{
    _indAI = ind;
}

void Mot::setAIsub(QString sub)
{
    _subAI = sub;
}

void Mot::setAItag(QString tag)
{
    _tagAI = tag;
}

QString Mot::getAIresults()
{
    return _tagAI + "\t" + _indAI + "\t" + _subAI;
}

void Mot::setNotNormal()
{
    _normal = false;
}

bool Mot::getNormal()
{
    return _normal;
}

void Mot::setFcsv(QString forme)
{
    _fCSV = forme;
}

QString Mot::getFcsv()
{
    return _fCSV;
}

/**
 * @brief Mot::getPossible
 * @return Les warnings générés
 *
 * Je reprends la fonction getPossible des token du programme CSV2APN
 * pour tenir compte des choix de l'IA.
 * Dans le programme d'origine, j'envoyais les warnings en qDebug,
 * alors qu'ici je dois les conserver précieusement.
 * Je voudrais aussi ajouter la possibilité de mieux choisir
 * les paires auxiliaire + PPP quand il y a plusieurs possibilités.
 * Ici, dans Mot, ça ne peut être qu'une alerte
 * (puisque je n'ai pas accès aux autres mots)
 * qui devrait être traitée dans Lasla.
 */
QString Mot::getPossible()
{
    QString rep = "";
    int i = 0;
    QList<int> liste;
    int max = _lFiches.size();
    for (i = 0; i < max; i++)
        if ((_tagAI == _lFiches[i]->getTag()) && (_indAI == _lFiches[i]->getIndice()))
                liste.append(i);
    if (liste.isEmpty())
    {
        // Je n'ai pas trouvé de solution avec tag et indice.
        for (i = 0; i < max; i++)
            if (_tagAI == _lFiches[i]->getTag())
                    liste.append(i);
        if (liste.isEmpty())
        {
            QString bla = "Tag et indice inconnus pour ";
            bla += _refs + " " + QString::number(_rang) + " ";
            bla += _fTexte + " ";
            bla += _tagAI + " ";
            bla += _indAI + "\n";
            rep.append(bla);
        }
        else
        {
            QString bla = "Indice inconnu pour ";
            bla += _refs + " " + QString::number(_rang) + " ";
            bla += _fTexte + " ";
            bla += _tagAI + " ";
            bla += _indAI + "\n";
            rep.append(bla);
        }
    }
    QString tag = _tagAI;
    if (liste.isEmpty()) tag = _lFiches[_choix]->getTag();
    // Si l'IA a choisi un tag qui n'existe pas,
    // je garderai celui du tagueur probabiliste.
    // Le tag choisi est-il compatible avec le code de subordination ?
    // Je me permet de corriger le code prédit par l'IA,
    // tout en gardant sa valeur.
    QString subAI = _subAI;
    if (tag.startsWith("B"))
    {
        if (((_subAI == "  ") || _subAI.isEmpty()) &&
                (tag.startsWith("B1") || tag.startsWith("B2") || tag.startsWith("B3")))
        {
            subAI = "**";
            QString bla = "Code manquant pour ";
            bla += _refs + " " + QString::number(_rang) + " ";
            bla += _fTexte + " ";
            bla += _tagAI + " ";
            bla += _indAI + "\n";
            rep.append(bla);
        }
        else if ((_subAI != "  ") && (tag.startsWith("B8")  || tag.startsWith("B9")))
        {
            QString bla = "Code inattendu pour ";
            bla += _refs + " " + QString::number(_rang) + " ";
            bla += _fTexte + " ";
            bla += _tagAI + " ";
            bla += _indAI + " " + _subAI + "\n";
            rep.append(bla);
            subAI = "  ";
        }
        // B7 (verbe à l'infinitif) peut avoir un code, mais pas nécessairement.
        // D'où pas de test et pas de message.
    }
    else if ((_subAI != "  ") && (!tag.startsWith("V6") || _refs.isEmpty()))
    {
        // Les participes, adjectifs verbaux et gérondifs ont un tag en V
        // Les ablatifs absolus ont un code,
        // mais tous les ablatifs ne sont pas absolus.
        // D'où pas de test et pas de message.
        QString bla = "Code inattendu pour ";
        bla += _refs + " " + QString::number(_rang) + " ";
        bla += _fTexte + " ";
        bla += _tagAI + " ";
        bla += _indAI + " " + _subAI + "\n";
        rep.append(bla);
        subAI = "  ";
    }
    // Pas sûr que les codes de subordinations soient bons,
    // mais je dois les mettre dans la référence.
    if (!_refs.isEmpty())
    {
        // Si _refs est vide, je suis sur un enclitique et _refs doit rester vide.
        if (_refs.count("/") == 1) _refs.append("/" + subAI);
        else _refs = _refs.section('/',0,1) + "/" + subAI;
    }
    // J'y ai mis le code éventuellement corrigé.

    if (liste.size() == 1) _choix = liste[0];
    // S'il n'y a qu'un choix, c'est le bon.
    // S'il n'y en a aucun, je reste sur le choix du tagueur probabiliste

    if (liste.size() > 1)
    {
    // J'ai plusieurs possibles, je dois choisir le plus probable.
    // ATTENTION au cas où il y aurait une majuscule à la forme !
    // Par exemple : Lea vs lea. Le 2e est plus probable, mais...
    // ATTENTION au cas où il y aurait un auxiliaire sous-entendu.
        bool maj = false;
        if (!_fTexte.isEmpty())
            maj = _fTexte[0].isUpper();
        int iMax = -1;
        int vMax = -1;
        for (i = 0; i < liste.size(); i++)
        {
            Fiche *bla = _lFiches[liste[i]];
            int v = bla->getNbr();
            QString f = bla->getForme();
            bool OK = !(f.contains("(") && f.contains(")"));
            // false s'il y a un sous-entendu.
            if (!f.isEmpty() && OK)
                OK = (maj && f.at(0).isUpper()) ||
                        (!maj && !f.at(0).isUpper());
            // Vrai, ssi la forme du texte et la forme lemmatisée
            // portent en même temps une majuscule ou pas.
            if ((v > vMax) && OK)
            {
                iMax = liste[i];
                vMax = v;
            }
        }
        if (vMax > 0) _choix = iMax;
        else
        {
        // Si les conditions sur la majuscule et l'absence de sous-entendu
        // étaient trop contaignantes, je recommence avec la majuscule seule.
            for (i = 0; i < liste.size(); i++)
            {
                Fiche *bla = _lFiches[liste[i]];
                int v = bla->getNbr();
                QString f = bla->getForme();
                bool OK = false;
                if (!f.isEmpty())
                    OK = (maj && f.at(0).isUpper()) ||
                            (!maj && !f.at(0).isUpper());
                // Vrai, ssi la forme du texte et la forme lemmatisée
                // portent en même temps une majuscule ou pas.
                if ((v > vMax) && OK)
                {
                    iMax = liste[i];
                    vMax = v;
                }
            }
            if (vMax > 0) _choix = iMax;
            else
            {
            // Si la condition sur la majuscule était trop contaignante,
                // je recommence sans.
                for (i = 0; i < liste.size(); i++)
                {
                    Fiche *bla = _lFiches[liste[i]];
                    if (bla->getNbr() > vMax)
                    {
                        iMax = liste[i];
                        vMax = bla->getNbr();
                    }
                }
                if (vMax > 0) _choix = iMax;
            }
        }
        if ((iMax > -1) && (iMax < max) &&
                _lFiches[iMax]->getForme().contains("<"))
        {
            QString toto = "@" + QString::number(liste.size());
            for (i = 0; i < liste.size(); i++)
                toto.append("\t" + QString::number(liste[i]));
            rep.prepend(toto + "\n");
        }
    } // Fin de (liste.size() > 1)

    _alerte = rep;
    return rep;
}

QString Mot::getAlerte()
{
    return _alerte;
}

void Mot::setAlerte(QString w)
{
    _alerte = w;
}
