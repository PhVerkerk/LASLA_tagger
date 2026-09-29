#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "token.h"

/**
 * @brief APN
 * @param ligne : une ligne du fichier CSV contenant l'info APN
 * @param ref : le référencement du mot (début de la ligne APN sur 8 caractères
 * et "ch,par,lg" en moins de 12 caractères)
 * @return la ligne APN avec le bon nombre de caractères par champ.
 *
 * La @a ligne du fichier CSV (une fois normalisée) donne dans l'ordre :
 * - 0 : la forme
 * - 1 : le tag
 * - 2 : le lemme
 * - 3 : l'indice
 * - 4 : le code
 * - 5 : le nombre d'occurrences
 * - autre.
 *
 * Le séparateur est évidemment le tab.
 * La ligne APN se compose du début de @a ref (avant le "/"),
 * suivi du lemme sur 21 caractères,
 * puis son indice (1 caractère),
 * la forme (25 caractères),
 * la fin de @a ref (après le "/") sur 12 caractères,
 * le code et 3 caractères pour finir la ligne
 * ("** " si c'est un verbe, "   " sinon).
 */
QString APN(QString ligne, QString ref, QString code)
{
    QString res = ref.section("/",0,0);
    QStringList eclats = ligne.split("\t");
    res += eclats[2] + QString(21 - eclats[2].size(),' ');
    res +=  eclats[3] + eclats[0];
    res += QString(25 - eclats[0].size(),' ') + ref.section("/",1,1);
    res += QString(12 - ref.section("/",1,1).size(),' ') + eclats[4];
    if (!code.isEmpty() && (code.size() == 2)) res += code + " ";
    // Si le code n'est pas en deux caractères, il est absurde.
    else if (ref.count("/") == 2) res += ref.section("\t",2,2);
    else if (eclats[4].startsWith('B')) res += "** ";
    else res += "   ";
    // Si j'ai un code de subordination calculé par l'IA,
    // je le mets à la place des "** " éventuelles.
    // J'ai gardé la possibilité d'avoir 3 champs à la ref.

    return res;
}

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 *
 * Le but de ce programme est de reconstruire le fichier APN
 * à partir du fichier CSV créé par le Tagueur daemon.
 * Il a donc besoin du nom du fichier CSV à traiter.
 * Si le même fichier CSV est passé par Latin-BERT qui
 * a produit les fichiers de prédictions nom+"_tag.csv" et
 * nom+"_ind.csv" placés à côté de nom+".csv",
 * ce programme va remplacer les prédictions du tagueur
 * par celles de l'IA, ssi elles existent dans la
 * liste de lemmatisations possibles.
 *
 * J'ajoute aussi la possibilité de prédiction par l'IA
 * du code de subordination, que je n'avais pas encore essayé
 * de déterminer. Le fichier s'appellera nom+"_sub.csv".
 *
 * Je reprends le programme qui m'a servi à faire les stats.
 */

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString nom = "";
    // Je lis fichiers.txt.
    QFile fic("fichiers.txt");
    if (fic.open(QFile::ReadOnly | QFile::Text))
    {
        QByteArray toto = fic.readLine().trimmed();
        QString titi(toto);
        if (titi.contains("\n")) titi.remove("\n");
        nom = titi;
        fic.close();
    }
//qDebug() << "Coucou !";
//qDebug() << nom;
    if (nom == "") qDebug() << "Il me faut un nom de fichier !\n";
    else
    {
        // nom contient le nom complet.
        if (nom.endsWith(".csv")) nom.chop(4);
        // Je supprime le ".csv" car le nom de base va me servir
        // à créer les noms des autres éléments.

        Token * _point = new Token("0\t.\t.\tPNT\tPNT\t\t\t\t\t\t");
        // Je crée une seule fois le token de fin de phrase.
        QList<Token*> _tokens;
        QList<int> _finsPhrase;
        QFile CSV(nom + ".csv");
        CSV.open(QIODevice::ReadOnly|QIODevice::Text);
        QTextStream flux(&CSV);
        QString linea;
        QString forme;
        QFile ficTag(nom + "_tag.csv");
        bool tagOK = ficTag.open(QIODevice::ReadOnly|QIODevice::Text);
        QTextStream fluTag(&ficTag);
        QFile ficInd(nom + "_ind.csv");
        bool indOK = ficInd.open(QIODevice::ReadOnly|QIODevice::Text);
        QTextStream fluInd(&ficInd);
        QFile ficCod(nom + "_sub.csv");
        bool codOK = ficCod.open(QIODevice::ReadOnly|QIODevice::Text);
        QTextStream fluCod(&ficCod);
        Token * tok;
        while (!flux.atEnd ())
        {
            linea = flux.readLine ();
            if (linea.endsWith("\n")) linea.chop(1);
            if (linea.isEmpty()) continue;
            if (linea.startsWith("#"))
            {
                tok->ajoute(linea);
            }
            else
            {
                if (linea.contains(".\tPNT"))
                {
                    _finsPhrase.append(_tokens.size());
                    _tokens.append(_point);
                    tok = _point;
                }
                else
                {
                    tok = new Token(linea);
                    _tokens.append(tok);
                }
                forme = linea.section("\t",1,1).toLower().replace("v","u");
                if (tagOK)
                {
                    linea = fluTag.readLine ();
                    if (linea.endsWith("\n")) linea.chop(1);
                    while (linea.isEmpty() || linea.startsWith("#"))
                    {
                        linea = fluTag.readLine ();
                        if (linea.endsWith("\n")) linea.chop(1);
                    }
                    if (forme != linea.section("\t",0,0))
                    {
                        qDebug() << "Probleme " << forme << linea;
                    }
                    else tok->ajoute(linea.section("\t",1,1));
                }
                if (indOK)
                {
                    linea = fluInd.readLine ();
                    if (linea.endsWith("\n")) linea.chop(1);
                    while (linea.isEmpty() || linea.startsWith("#"))
                    {
                        linea = fluInd.readLine ();
                        if (linea.endsWith("\n")) linea.chop(1);
                    }
                    if (forme != linea.section("\t",0,0))
                    {
                        qDebug() << "Probleme " << forme << linea;
                    }
                    else tok->ajoute(linea.section("\t",1,1));
                }
                if (codOK)
                {
                    linea = fluCod.readLine ();
                    if (linea.endsWith("\n")) linea.chop(1);
                    while (linea.isEmpty() || linea.startsWith("#"))
                    {
                        linea = fluCod.readLine ();
                        if (linea.endsWith("\n")) linea.chop(1);
                    }
                    if (forme != linea.section("\t",0,0))
                    {
                        qDebug() << "Probleme " << forme << linea;
                    }
                    else tok->setCodeSub(linea.section("\t",1,1));
                }
            }
        }
        CSV.close();
        ficCod.close();
        ficInd.close();
        ficTag.close();

        CSV.setFileName(nom + "_03.APN");
        CSV.open(QIODevice::WriteOnly|QIODevice::Text);
        ficTag.setFileName(nom + "_IA.APN");
        ficTag.open(QIODevice::WriteOnly|QIODevice::Text);
        for (int i = 0; i < _tokens.size(); i++)
        {
            tok = _tokens[i];
            if (tok != _point)
            {
                flux << APN(tok->getLgTagueur(), tok->getRef(), tok->getCodeSub()) << "\n";
                linea = tok->getChIA();
                if (tagOK)
                fluTag << APN(tok->getPossible(linea.section("|",0,0),linea.section("|",1,1)),
                              tok->getRef(), tok->getCodeSub()) << "\n";
            }
        }
        CSV.close();
        ficTag.close();
        // Il faut maintenant faire un _IA.ZPN
        // en reprenant _02.ZPN et _IA.APN. Ou le faire au-dessus en même temps que les deux APN.
        ficTag.open(QIODevice::ReadOnly|QIODevice::Text);
        CSV.setFileName(nom + "_02.ZPN");
        CSV.open(QIODevice::ReadOnly|QIODevice::Text);
        ficCod.setFileName(nom + "_IA.ZPN");
        ficCod.open(QIODevice::WriteOnly|QIODevice::Text);
        linea = ficTag.readLine();
        QString linez = CSV.readLine();
        while (!CSV.atEnd())
        {
            if (linez.startsWith("!"))
            {
                fluCod << linez;
                linez = CSV.readLine();
            }
            else
            {
                fluCod << linea;
                linea = ficTag.readLine();
                linez = CSV.readLine();
            }
        }
        CSV.close();
        ficTag.close();
        ficCod.close();
    }
/*    // Par défaut, la liste des fichiers est dans ../LASLA_data/
    // et les textes sont dans ../Textes_analyse9/.
    // Mais si je donne un nom de répertoire (pour un autre corpus),
    // la liste et les textes sont supposés y être.
    qDebug() << nom << racine;
    QString forme;
    QString lemme;
    QString indice;
    QString code9;
    QString car4;
    QString tag;
    QString sep0 = "?";
    QString phrase;
    int num_tok = 0;

    QString nomFichier;
    QFile fListe ("../"+nom+"/liste9.txt");
//    QFile fListe ("/Users/Philippe/Documents/TiteLive_BPN/liste9.txt");
    fListe.open (QIODevice::ReadOnly|QIODevice::Text);
    QTextStream fluxL (&fListe);
    while (!fluxL.atEnd ())
    {
        nomFichier = fluxL.readLine ();
        if (nomFichier.isEmpty () || nomFichier[0] == '!') continue;

        QString fichier = racine + nomFichier;
//        QString fichier = "/Users/Philippe/Documents/TiteLive_BPN/"+nomFichier;
        QFile fmr (fichier);
        fmr.open (QIODevice::ReadOnly|QIODevice::Text);
        QTextStream fluxM (&fmr);
        QString separ = "?";
        QString ref = "?";

        fichier = cible + nomFichier;
        fichier.replace(".APN",".txt");
        fichier.replace(".BPN",".txt");
        QFile f_out(fichier);
        f_out.open(QIODevice::WriteOnly|QIODevice::Text);
        QTextStream fl_out(&f_out);

        fichier = ciblComp + nomFichier;
        fichier.replace(".APN",".csv");
        fichier.replace(".BPN",".csv");
        QFile comp_out(fichier);
        comp_out.open(QIODevice::WriteOnly|QIODevice::Text);
        QTextStream fl_comp(&comp_out);

        fichier = ciblConllu + nomFichier;
        fichier.replace(".APN",".csv");
        fichier.replace(".BPN",".csv");
        QFile conllu_out(fichier);
        conllu_out.open(QIODevice::WriteOnly|QIODevice::Text);
        QTextStream fl_conllu(&conllu_out);

        fl_conllu << "# Pseudo conllu " << nomFichier << "\n";

        while (!fluxM.atEnd ())
        {
//            linea = ligne;
            linea = fluxM.readLine ();
            if (linea.isEmpty () || linea[0] == '!') continue;

            // Si le numéro de la phrase en cours est
            // différent de celui enregistré,
            // je dois insérer un point de fin de phrase.
            if (linea.mid(4,4) != separ)
            {
                if (separ != sep0)
                {
                    fl_out << ".";
                    fl_conllu << num_tok + 1 << "\t.\t.\tPNT\tPNT\n";
                    fl_conllu << "# Text=\"" << phrase.simplified() << ".\" \n\n";
                    // Une ligne blanche pour séparer les phrases.
                }
                separ = linea.mid(4,4);
                fl_conllu << "# " << linea.mid(0,3) << "\t" << separ << "\t" << linea.mid(55,12) << "\n";
                // Les références pour la prochaine phrase.
                num_tok = 0;
                phrase = "";
            }
            if (linea.mid(55,12) != ref)
            {
                if (ref != sep0)
                {
                    // La référence a changé.
                    if (ref.contains(","))
                    {
                        // Je dois savoir combien de sauts de ligne je dois insérer.
                        QStringList ll = linea.mid(55,12).trimmed().split(",");
                        QStringList lr = ref.trimmed().split(",");
                        int minSize = ll.size();
                        if (lr.size() < minSize) minSize = lr.size();
                        for (int ii = 0; ii < minSize; ii++)
                            if (ll[ii] != lr[ii]) fl_out << "\n";
                    }
                    else fl_out << "\n";
                }
                ref = linea.mid(55,12);
            }

            /* Le traitement de la ligne dépend du caractère n°4
            & : par défaut,
            = : enclitique,
            # : une forme pour plusieurs lemmes -> formes type 'abducast' ou nombres,
            + : je ne sais plus, il n'y en a que 6 chez Pline, probablement pas très important
            ? : il en traine un.
            Il semblerait que + et ? soient des erreurs et que l'on peut les ignorer.
            linea est la ligne que je suis en train de traiter
            */
/*            car4 = linea.mid(3,1);
            lemme = linea.mid(8,21).trimmed();
            indice = linea.mid(29,1);
            forme = linea.mid(30,25).trimmed();
            code9 = linea.mid(67,9);
            num_tok++;

            // création du tag à partir du code 9 (copie de LASLA_cnt)
            if ((code9[0] == ' ') || (code9[0] == '0')) tag = "X  ";
            else if (code9[0] == 'B')
            {
                if ((code9[5] == '4') || (code9[5] == '5') || (code9[5] == '6')) tag = "V" + code9[2] + code9[3];
                // Formes déclinées d'un verbe (= participes)
                else if (code9[6] == '1') tag = "B" + code9[5] + "1"; // tentative pour distinguer legimus présent et parfait.
           //     else if (code9[5] == '9') tag = "B8 "; // Je regroupe les 2 supins
                else tag = "B" + code9[5] + " ";
            }
            else
            {
                tag = code9.mid(0,4);
                tag.remove(1,1);
            }
            if ((car4 == "=") || ((car4 == "#") && (lemme == "NE")) || (lemme == "QVE"))
                tag[1] = 'e'; // Pour distinguer la conjonction de coordination "que" qui est après les coordonnés.

            fl_comp << forme << "\t" << lemme << "\t" << indice << "\t" << code9 << "\t" << tag << "\n";

            if (car4 == "&")
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
                fl_out << " " << forme; // Cas normal
                phrase.append(" " + forme);
            }
            else if (car4 == "=")
            {
                fl_out << forme;// Enclitique (collé à la forme précédente)
                phrase.append(forme);
            }
            else if (car4 == "#")
            {
                // C'est le cas où la forme est décomposée en plusieurs lemmes
                // Il a déjà été écrit.
            }
            else
            {
                fl_out << " " << forme;// Pour les 7 cas pathologiques
                phrase.append(" " + forme);
            }

/*            if (indice == " ")
                fl_conllu << num_tok << "\t" << forme << "\t" << lemme << "\t" << tag << "\t" << code9 << "\n";
            else
                fl_conllu << num_tok << "\t" << forme << "\t" << lemme << "_" << indice << "\t" << tag << "\t" << code9 << "\n";
  */          // J'ai besoin de la forme dans la 2e colonne et du tag dans la 4e.
/*            fl_conllu << num_tok << "\t" << forme << "\t" << lemme << "\t" << indice << "\t" << tag << "\t" << code9 << "\n";
            // Je mets l'indice en 4e colonne !? dans LASLA_APN_indice

        }
        fmr.close ();
        // Fin du traitement du texte en cours
        fl_out << ".";
        fl_conllu << num_tok + 1 << "\t.\t.\tPNT\tPNT\n";
        fl_conllu << "# Text=\"" << phrase.simplified() << ".\" \n\n";

        f_out.close();
        comp_out.close();
    }
    fListe.close();
    // J'ai fini l'ensemble des textes
*/
    a.quit();
//    return a.exec();
}
