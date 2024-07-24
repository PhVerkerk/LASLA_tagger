#include "fiche.h"

/**
 * @brief Fiche::Fiche
 * @param lg : la ligne du fichier.
 *
 * Créateur d'une fiche à partir d'une ligne
 * d'un des fichiers tirés des textes du LASLA.
 *
 * Les fichiers sont au format CSV et comptent six champs,
 * séparés par des virgules :
 * - la forme
 * - le lemme
 * - l'indice d'homonymie du lemme
 * - le code en 9
 * - le tag
 * - le nombre d'occurrences
 *
 * Comme la forme peut contenir plusieurs mots ou,
 * au contraire, donner plusieurs lemmes,
 * je dois en extraire une clef qui correspond
 * au mot que je vais rencontrer dans un texte
 * et qui va déclencher cette analyse.
 *
 * Je dois ensuite vérifier que les mots nécessaires
 * pour chaque analyse sont bien présents dans la phrase.
 *
 */
Fiche::Fiche(QString lg)
{
    QStringList eclats = lg.split(",");
    _forme = eclats[0];
    _lemme = eclats[1];
    _indice = eclats[2];
    _code = eclats[3];
    if (_code.size() > 9) _code = _code.left(9);
    else while (_code.size() < 9) _code += " ";
    _tag = eclats[4];
    if (_tag.size() > 3) _tag = _tag.left(3);
    else while (_tag.size() < 3) _tag += " ";
    _nbr = eclats[5].toInt();
    _jointe = testJointe(_forme);
    // _jointe est true s'il y a un crochet interne.
    _clef = clef(_forme);
    // La _clef est ce que je vais rencontrer dans le texte.
    if (_clef == _forme) _cmpl = "";
    else _cmpl = cmpl(_forme,_clef);
    // Si la clef est la forme, il n'y a pas de contrainte.
    // Sinon, il faut voir...
}

/**
 * @brief Fiche::clef
 * @param ff : la forme donnée par le LASLA
 * @return La clef
 *
 * Cette fonction est "static" car je dois pouvoir
 * l'appeler lorsque j'ouvre un fichier APN.
 *
 * Comme la forme peut contenir plusieurs mots ou,
 * au contraire, donner plusieurs lemmes,
 * je dois en extraire une clef qui correspond
 * au mot que je vais rencontrer dans un texte
 * et qui va déclencher cette analyse.
 *
 * Dans certains cas, la clef ne peut pas être
 * déterminée par la seule forme.
 * Par exemple, la clef "<bene dictum>st" est
 * "bene" car le groupe "bene dictum" se lemmatise
 * en BENEDICO.
 * Alors que celle de "<data nulla>st" est "nullast".
 * Je choisis donc de donner les deux possibilités,
 * séparées par un '/', et je devrai décider
 * plus tard laquelle est la bonne.
 * Pour les exemples ci-dessus, je vais trouver
 * la forme "<data> nulla<st>", associée à la
 * clef "nullast", alors que je ne trouverai pas
 * "<bene> dictum<st>" associée à "dictumst".
 */
QString Fiche::clef(QString ff)
{
    QString c = "";
    if (ff.contains(' '))
    {
        QStringList eclats = ff.split(' ',QString::SkipEmptyParts);
//        if (eclats.size() > 2) qDebug() << ff;
        // La forme de la fiche contient plusieurs mots.
        // Pour la plupart, il n'y en a que 2 : 44 en ont 3.

        if (eclats[0].startsWith("(")) return eclats[1];
        // Les 8 cas où le 1er mot est entre parenthèses n'ont que 2 mots.

        if (eclats[0].startsWith("<"))
        {
            // Il y a plusieurs cas
            if (eclats[0].contains(">") && !eclats[0].endsWith(">"))
            {
                // Le 1er mot est une forme jointe
                c = eclats[0];
            }
            else if (eclats[0].endsWith(">")) c = eclats[1];
            // Le 1er mot est entre crochet
            // --> Je prends le 2e, qui peut être une forme jointe, comme clef.
            else
            {
            // Le crochet se ferme plus loin.
            // --> il faut examiner le 2e mot.
                if (eclats[1].endsWith(">") && (eclats.size() == 3)) return eclats[2];
                // Ici se pose le problème de l'ambiguïté de la clef.
                // Par exemple, pour "<bene dictum>st" et "<data nulla>st",
                // les clefs doivent être respectivement "bene" et "nullast".
                else if (eclats[1].contains(">")) c = eclats[0] + "/" + eclats[1];
                else if (eclats.size() == 2) c = eclats[1];
                else if (eclats[2].contains(">")) c = eclats[0]; // Attention à "<quem ad modum>st" !
                else c = eclats[2];
            }
        }
        else if (eclats[0].contains("<"))
        {
            // Forme jointe
            c = eclats[0];
        }
        else c = eclats[0];
    }
    else
    {
        c = ff;
        // Un seul mot, grec ou lemmatisé en plusieurs morceaux : abductast.
    }
    c.remove("<");
    c.remove(">");
    return c;
}

/**
 * @brief Fiche::cmpl
 * @param ff : la forme donnée par le LASLA
 * @param c : la clef déterminée par la fonction Fiche::clef()
 * @return Le complément d'information
 *
 * Cette fonction est "static" car je dois pouvoir
 * l'appeler depuis n'importe où.
 *
 * J'ai extrait, de la forme, une clef qui correspond
 * au mot que je vais rencontrer dans un texte
 * et qui va déclencher cette analyse.
 * Je dois maintenant déterminer le complément
 * d'information qui validera cette analyse.
 * La position de la clef est conservée par un §.
 *
 * Cette routine doit rester cohérente avec celle
 * qui détermine la clef.
 *
 * Je devrai ensuite vérifier que les mots nécessaires
 * pour chaque analyse sont bien présents dans la phrase.
 *
 */
QString Fiche::cmpl(QString ff,QString c)
{
    // Il faut déduire la clef c de la forme ff pour avoir le complément d'information.
    if (ff.contains(c+" "))
    {
        // Attention, il y a un "ut ut" parmi les formes : garder " ut" !
        ff.replace(ff.indexOf(c+" "),c.size(),"§");
        // Je mets un signe § à l'emplacement de la clef.
        if (ff.startsWith("§ (") && ff.endsWith(")")) return "";
        // Si l'auxiliaire est sous-entendu, je n'ai pas de contrainte.
        if (ff.contains(" (")) ff=ff.section(" (",0,0);
        // J'ai un auxiliaire sous-entendu, mais aussi autre chose.
        if (testJointe(ff))
        {
            ff.remove(">");
            ff.remove("<");
        }
        return ff;
    }
    if (ff.contains(" "+c))
    {
        ff.replace(ff.indexOf(" "+c)+1,c.size(),"§");
        if (ff.startsWith("(") && ff.endsWith(") §")) return "";
        if (ff.contains(") ")) ff=ff.section(") ",0,0);
        return ff;
    }
    // Si je suis ici, c'est que la forme ne contient pas le mot qui sert de clef.
    // En bref, il y a une forme jointe.
    if (ff.contains(" "))
    {
        // plusieurs mots.
        QStringList eclats = ff.split(' ',QString::SkipEmptyParts);
        QString c = "";

        if (eclats[0].startsWith("<"))
        {
            // Il y a plusieurs cas
            if (eclats[0].contains(">") && !eclats[0].endsWith(">"))
            {
                // Le 1er mot est une forme jointe
                c = "§ " + eclats[1];
                if (eclats.size() == 3) c += eclats[2];
                return c;
            }
            else
            {
            // Le 1er mot est entre crochet ou le crochet se ferme plus loin.
            // --> il faut examiner le 2e mot.
                if (eclats[1].endsWith(">") && (eclats.size() == 3))
                    return eclats[0] + " " + eclats[1] + " §";
                else if (eclats[1].contains(">")  || (eclats.size() == 2))
                    return eclats[0] + "> §";
                else // c = eclats[2]; // Attention à "<quem ad modum>st" !
                    return eclats[0] + " " + eclats[1] + " §";
            }
        }
        else if (eclats[0].contains("<"))
        {
            // Forme jointe
            c = "§ " + eclats[1];
            if (eclats.size() == 3) c += eclats[2];
            return c;
        }
        else qDebug() << ff << c;
        // Normalement, je n'arrive jamais ici !
        return ff;
    }
    else return ""; // Une forme sur plusieurs lemmes.
}

/**
 * @brief Fiche::info
 * @return Toute l'information de la fiche
 *
 * Cette fonction met en forme l'information contenue
 * dans la fiche (forme, lemme indice et code en 9).
 * Le format retenu ressemble à celui du LASLA,
 * à l'ordre près, avec des champs de longueur fixe.
 *
 */
QString Fiche::info()
{
    QString res = _forme;
    res += QString(25 - _forme.size(),' ') + _lemme;
    res += QString(21 - _lemme.size(),' ') + _indice;
    res += " " + _code + " ";
//    res += _tag + " ";
    return res;
}

/**
 * @brief Fiche::Lasla
 * @param ref : pour repérer le mot dans le texte.
 * @return Toute l'info au format APN
 *
 * Cette fonction met en forme l'information contenue
 * dans la fiche (forme, lemme indice et code en 9).
 * Le format retenu est celui des fichiers APN du LASLA
 * avec des champs de longueur fixe.
 *
 * La référence de l'œuvre et le numéro de la phrase,
 * ainsi que le repérage du mot, ne sont pas liés
 * à la fiche, mais au mot. Ces informations viennent
 * donc de l'extérieur dans le paramètre ref.
 * Ce paramètre contient au moins 2 champs séparés par '/'.
 * Lorsque les données viennent d'un fichier APN existant,
 * l'éventuel code de subordination est conservé
 * comme 3e champ. Si ce 3e champ n'existe pas,
 * je termine la ligne avec un "** " pour les formes verbales.
 * Cela doit rappeler au philologue qu'il doit déterminer
 * ce code de subordination (je ne suis pas capable de le faire).
 *
 */
QString Fiche::Lasla(QString ref)
{
//    if (ref != "/")
//    qDebug() << ref << _lemme << _indice << _forme << _code;
    QStringList eclats = ref.split("/");
    if (eclats.size() == 1)
    {
        eclats << " ";
    }
    QString res = eclats[0] + _lemme;
    res += QString(21 - _lemme.size(),' ') + _indice + _forme;
    res += QString(25 - _forme.size(),' ') + eclats[1];
    res += QString(12 - eclats[1].size(),' ') + _code;
    if (eclats.size() == 3) res += eclats[2];
    else if (_code.startsWith('B')) res += "** ";
    else res += "   ";
    return res;
}

/**
 * @brief Fiche::cats
 * Les catégories du LASLA pour la traduction du code en 9
 */
const QStringList Fiche::cats = QStringList ()
    << "" << "Subst. " << "Verbe " << "Adj. " << "Num. "
    << "Pr. pers. " << "Pr. poss. " << "Pr. refl. " << "Pr. poss. refl. "
    << "Pr. dem. " << "Pr. rel. " << "Pr. inter. " << "Pr. indef. "
    << "Adv. " << "Adv. rel. " << "Adv. inter. " << "Adv. neg. " << "Adv. inter. neg. "
    << "Prep. " << "Conj. coord. " << "Conj. subord. " << "Interj. " << "Verbe "
    << "" << "Inconnu "; // Vers une simplification ?

/**
 * @brief Fiche::cass
 * Les cas pour la traduction du code en 9
 */
const QStringList Fiche::cass = QStringList ()
    << "" << "nom." << "uoc." << "acc."
    << "gen." << "dat." << "abl." << "loc." << "indecl.";

/**
 * @brief Fiche::nombres
 * Les nombres pour la traduction du code en 9
 */
const QStringList Fiche::nombres = QStringList ()
        << "" << "sing." << "plur.";

/**
 * @brief Fiche::personnes
 * Les personnes pour la traduction du code en 9
 */
const QStringList Fiche::personnes = QStringList ()
        << "" << "prim." << "sec." << "tert.";

/**
 * @brief Fiche::degres
 * Les degrés pour la traduction du code en 9
 */
const QStringList Fiche::degres = QStringList ()
        << "" << "posit." << "compar." << "superl.";

/**
 * @brief Fiche::modes
 * Les modes pour la traduction du code en 9
 */
const QStringList Fiche::modes = QStringList ()
    << "" << "ind." << "imper." << "coniunct." << "part." // 0 - 4
    << "adiect. verb." << "gerund." << "infin." << "supin en -UM" << "supin en -U"; // 5 - 9

/**
 * @brief Fiche::voixs
 * Les voix pour la traduction du code en 9
 */
const QStringList Fiche::voixs = QStringList ()
    <<"" << "act." << "pass." << "dep." << "semi-dep.";

/**
 * @brief Fiche::tempss
 * Les temps pour la traduction du code en 9
 */
const QStringList Fiche::tempss = QStringList ()
    << "" << "praes." << "imperf." << "fut." << "perf."
    << "plus-quam-perf." << "fut. ant." << "périphr. parf."
    << "périphr. pqp" << "périphr. fut. ant.";

/**
 * @brief Fiche::humain
 * @return La traduction humaine du code en 9
 *
 * Cette routine prend le code en 9 de la fiche
 * et le transforme en quelque chose de compréhensible
 * par un utilisateur non-averti.
 * Pour cela, elle utilise les tableaux précédents
 * pour exprimer les cas-nombres ou autres.
 *
 */
QString Fiche::humain()
{
    QString res = "";
    if (_tag[0]=='V')
    {
        // C'est un participe, d'où un ordre différent (cas et nombre à la fin)
        res = "Verbe ";
        if (_code[5].isDigit()) res+=modes[_code[5].digitValue()] + " ";
        if (_code[6].isDigit()) res+=tempss[_code[6].digitValue()] + " ";
        if (_code[7].isDigit()) res+=voixs[_code[7].digitValue()] + " ";
        if (_code[2].isDigit()) res+=cass[_code[2].digitValue()] + " ";
        if (_code[3].isDigit()) res+=nombres[_code[3].digitValue()] + " ";
        if (_code[4].isDigit()) res+=degres[_code[4].digitValue()] + " ";
        return res;
    }
    if (_tag[0]=='#') res = "Auxiliaire ";
    else res = cats[_tag[0].unicode()-64];
    if (_code[2].isDigit()) res+=cass[_code[2].digitValue()] + " ";
    if (_code[8].isDigit()) res+=personnes[_code[8].digitValue()] + " ";
    if (_code[3].isDigit()) res+=nombres[_code[3].digitValue()] + " ";
    if (_code[4].isDigit()) res+=degres[_code[4].digitValue()] + " ";
    if (_code[5].isDigit()) res+=modes[_code[5].digitValue()] + " ";
    if (_code[6].isDigit()) res+=tempss[_code[6].digitValue()] + " ";
    if (_code[7].isDigit()) res+=voixs[_code[7].digitValue()] + " ";
    return res;
}

/**
 * @brief Fiche::testJointe
 * @param ff : la forme à tester.
 * @return un booléen qui dit si la forme
 * doit conduire à plusieurs lemmes
 *
 * Cette fonction dit si la forme ff est une
 * forme jointe. Par exemple, le mot "abductast"
 * est lemmatisé en "abducta<st>" et "<abducta>st".
 * La première forme est associée à "ABDUCO"
 * et la seconde à l'auxiliaire "SVM 2".
 *
 * La réponse est true si la forme contient
 * un '<' qui n'est pas précédé d'un espace
 * ou un '>' qui n'est pas suivi par un espace.
 * Les première et dernière lettres sont ignorées.
 *
 */
bool Fiche::testJointe(QString ff)
{
    bool j = false;
    if (ff.contains("<"))
        for (int i=1; i<ff.size()-1;i++)
            j = j || ((ff[i] == '<') && (ff[i-1] != ' '))
                    || ((ff[i] == '>') && (ff[i+1] != ' '));
    return j;
}

/**
 * @brief Fiche::setClef
 * @param c la nouvelle clef
 *
 * Lorsque la clef était ambiguë (cf. la fonction Fiche::clef),
 * une recherche a posteriori me permet de choisir la bonne.
 * Je dois donc indiquer sa nouvelle valeur.
 *
 */
void Fiche::setClef(QString c)
{
    _clef = c;
}

/**
 * @brief Fiche::setCmpl
 * @param c le nouveau complément d'information
 *
 * Lorsque la clef était ambiguë (cf. la fonction Fiche::clef),
 * une recherche a posteriori me permet de choisir la bonne.
 * Une fois déterminée la clef, j'évalue le complément d'information.
 * Je dois donc indiquer sa nouvelle valeur.
 *
 */
void Fiche::setCmpl(QString c)
{
    _cmpl = c;
}

// Divers accesseurs.
/**
 * @brief Fiche::getNbr
 * @return : nombre d'occurrences
 */
int Fiche::getNbr()
{
    return _nbr;
}

QString Fiche::getTag()
{
    return _tag;
}

QString Fiche::getClef()
{
    return _clef;
}

QString Fiche::getCmpl()
{

    return _cmpl;
}

QString Fiche::getCode()
{
    return _code;
}

QString Fiche::getForme()
{
    return _forme;
}

QString Fiche::getLemme()
{
    return _lemme;
}

QString Fiche::getIndice()
{
    return _indice;
}

bool Fiche::jointe()
{
    return _jointe;
}

bool Fiche::estCondit()
{
    return (_cmpl.contains(" <") || _cmpl.contains("> "));
}
