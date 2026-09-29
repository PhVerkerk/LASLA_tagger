#include "token.h"

Token::Token(QString ligne, QObject *parent) : QObject(parent)
{
    QStringList eclats = ligne.split("\t");
    _forme = eclats[1];
    // C'est la forme simplifiée que je dois retrouver dans les prédictions.
    _reference = eclats[9];
    _lgTagueur = ligne.section("\t",2);
    _numero = ligne.section("\t",0,0);
    _choixIA = "|";
    _codeSub = "";
}

void Token::ajoute(QString ligne)
{
    if (ligne.startsWith("#!")) _choixTagueur = ligne.mid(3);
    else if (ligne.startsWith("#\t")) _possibles.append(ligne.mid(2));
    else if (ligne.size() == 3) _choixIA = ligne + "|" + _choixIA.section("|",1); // Tag
    else if (ligne.size() == 1) _choixIA = _choixIA.section("|",0,0) + "|" + ligne; // Indice
}

void Token::setCodeSub(QString code)
{
    _codeSub = code;
}

QString Token::getCodeSub()
{
    return _codeSub;
}

QString Token::getRef()
{
    return _reference;
}

QString Token::getForme()
{
    return _forme;
}

QString Token::getLgTagueur()
{
    return _lgTagueur;
}

QString Token::getChTagueur()
{
    return _choixTagueur;
}

QString Token::getChIA()
{
    return _choixIA;
}

QString Token::getPossible(QString tag, QString indice)
{
    int i = 0;
    QList<int> liste;
    int max = _possibles.size();
/* Dans une première version, je prenais la première solution avec le bon tag et indice.
 * C'est peut-être insuffisant, en particulier s'il y a plusieurs possibles.
 *
    while ((i < max) && ((tag != _possibles[i].section("\t",1,1)) || (indice != _possibles[i].section("\t",3,3))))
        i++;
    if (i < max) return _possibles[i];
    // S'il y a une solution avec les tag et indice choisis par l'IA,
    // je la donne.
    i = 0;
    while ((i < max) && (tag != _possibles[i].section("\t",1,1)))
        i++;
    if (i < max)
    {
        qDebug() << "Indice inconnu pour" << _reference << _numero << _forme << indice;
        return _possibles[i];
    // À défaut, s'il y a une solution avec le seul tag choisi par l'IA,
    // je la donne. Sinon, je donne la solution choisie par le tagueur.
    }
    */
    for (i = 0; i < max; i++)
        if ((tag == _possibles[i].section("\t",1,1)) && (indice == _possibles[i].section("\t",3,3)))
                liste.append(i);
    if (liste.isEmpty())
    {
        // Je n'ai pas trouvé de solution avec tag et indice.
        for (i = 0; i < max; i++)
            if (tag == _possibles[i].section("\t",1,1))
                    liste.append(i);
        if (liste.isEmpty())
        {
            QString bla = _reference + " ";
            bla += _numero + " ";
            bla += _forme + " ";
            bla += tag + " ";
            bla += indice + " ";
            printf("Tag et indice inconnus pour %s\n", qPrintable(bla));
//            qDebug() << "Tag et indice inconnus pour" << _reference << _numero << _forme << tag << indice;
            return _lgTagueur;
        }
        QString bla = _reference + " ";
        bla += _numero + " ";
        bla += _forme + " ";
        bla += tag + " ";
        bla += indice + " ";
        printf("Indice inconnu pour %s\n", qPrintable(bla));
//        qDebug() << "Indice inconnu pour" << _reference << _numero << _forme << tag << indice;
    }
    // L'IA a choisi un tag qui existe.
    // Est-il compatible avec le code de subordination ?
    if (tag.startsWith("B"))
    {
        if (((_codeSub == "  ") || _codeSub.isEmpty()) &&
                (tag.startsWith("B1") || tag.startsWith("B2") || tag.startsWith("B3")))
        {
            _codeSub = "**";
            QString bla = _reference + " ";
            bla += _numero + " ";
            bla += _forme + " ";
            bla += tag + " ";
            bla += indice + " ";
            printf("Code manquant pour %s\n", qPrintable(bla));
        }
        else if ((_codeSub != "  ") && (tag.startsWith("B8")  || tag.startsWith("B9")))
        {
            QString bla = _reference + " ";
            bla += _numero + " ";
            bla += _forme + " ";
            bla += tag + " ";
            bla += indice + " " + _codeSub;
            printf("Code inattendu pour %s\n", qPrintable(bla));
            _codeSub = "  ";
        }
        // B7 (verbe à l'infinitif) peut avoir un code, mais pas nécessairement.
        // D'où pas de test et pas de message.
    }
    else if ((_codeSub != "  ") && !tag.startsWith("V6"))
    {
        // Les participes, adjectifs verbaux et gérondifs ont un tag en V
        // Les ablatifs absolus ont un code,
        // mais tous les ablatifs ne sont pas absolus.
        // D'où pas de test et pas de message.
        QString bla = _reference + " ";
        bla += _numero + " ";
        bla += _forme + " ";
        bla += tag + " ";
        bla += indice + " " + _codeSub;
        printf("Code inattendu pour %s\n", qPrintable(bla));
        _codeSub = "  ";
    }
    if (liste.size() == 1) return _possibles[liste[0]];
    // J'ai plusieurs possibles, je dois choisir le plus probable.
    // ATTENTION au cas où il y aurait une majuscule à la forme !
    // Par exemple : Lea vs lea. Le 2e est plus probable, mais...
    // ATTENTION au cas où il y aurait un auxiliaire sous-entendu.
    bool maj = false;
    if (!_forme.isEmpty())
        maj = _forme[0].isUpper();
    int iMax = -1;
    int vMax = -1;
    for (i = 0; i < liste.size(); i++)
    {
        QString bla = _possibles[liste[i]];
        int v = bla.section("\t",5,5).toInt();
        bool OK = !(bla.contains("(") && bla.contains(")"));
        // false s'il y a un sous-entendu.
        if (!bla.section("\t", 0, 0).isEmpty() && OK)
            OK = (maj && bla.section("\t", 0, 0).at(0).isUpper()) ||
                    (!maj && !bla.section("\t", 0, 0).at(0).isUpper());
        // Vrai, ssi la forme du texte et la forme lemmatisée
        // portent en même temps une majuscule ou pas.
        if ((v > vMax) && OK)
        {
            iMax = liste[i];
            vMax = v;
        }
    }
    if (vMax > 0)
        return _possibles[iMax];
    // Si les conditions sur la majuscule et l'absence de sous-entendu
    // étaient trop contaignantes, je recommence avec la majuscule seule.
    for (i = 0; i < liste.size(); i++)
    {
        QString bla = _possibles[liste[i]];
        int v = bla.section("\t",5,5).toInt();
        bool OK = false;
        if (!bla.section("\t", 0, 0).isEmpty())
            OK = (maj && bla.section("\t", 0, 0).at(0).isUpper()) ||
                    (!maj && !bla.section("\t", 0, 0).at(0).isUpper());
        // Vrai, ssi la forme du texte et la forme lemmatisée portent une majuscule ou pas.
        if ((v > vMax) && OK)
        {
            iMax = liste[i];
            vMax = v;
        }
    }
    if (vMax > 0)
        return _possibles[iMax];
    // Si la condition sur la majuscule était trop contaignante, je recommence sans.
    iMax = liste[0];
    vMax = _possibles[iMax].section("\t",5,5).toInt();
    for (i = 1; i < liste.size(); i++)
        if (_possibles[liste[i]].section("\t",5,5).toInt() > vMax)
        {
            iMax = liste[i];
            vMax = _possibles[iMax].section("\t",5,5).toInt();
        }
    return _possibles[iMax];

}

