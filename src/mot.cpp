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
    if (_choix < 0) _choix = 0;
    if (_lFiches.isEmpty() || (_choix >= _lFiches.size())) return "????";
    if (_refs.isEmpty())
    {
        // Les enclitiques n'ont pas de ref propre.
        // Je reprends donc la ref du mot qui précède (qui porte l'enclitique).
        if (ref.count('/') == 2)
            ref = ref.section('/',0,1);
        // Toutefois, si j'ai un verbe avec un code de subordination,
        // je ne dois pas le recopier pour l'enclitique.
        return _lFiches[_choix]->Lasla(ref);
    }
    return _lFiches[_choix]->Lasla(_refs);
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

void Mot::setChoix(QString t, int sc)
{
    // Ce que je choisis, c'est un tag...
    _tagChoisi = t;
    _score = sc;
    // ... Mais, ce qui m'intéresse, c'est l'indice de la solution dans la liste des possibles.
    _choix = 0;
    if (_lFiches.size() > 1)
    {
    while ((_choix < _lFiches.size()) && (_lFiches[_choix]->getTag() != t))
        _choix++;
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
