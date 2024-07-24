#ifndef FICHE_H
#define FICHE_H

#include <QString>
#include <QStringList>
#include <QtWidgets>

/**
 * @brief The Fiche class represents a line of data
 * in the texts lemmatized by the LASLA
 *
 * Le traitement des textes lemmatisés au LASLA
 * m'a conduit à créer des "fiches" qui associent
 * un forme, un lemme avec son indice d'homonymie
 * et un code en 9. J'y ai ajouté un tag (qui
 * pourrait être recalculé à partir du code en 9)
 * et le nombre d'occurrences observé dans le corpus.
 * J'avais séparé les cas simples (la forme est le mot)
 * des cas plus complexes (formes composées, jointes ou autres).
 *
 * Il s'agit maintenant d'utiliser ces fiches pour
 * lemmatiser un texte et ensuite faire tourner un
 * tagueur probabiliste pour choisir la solution
 * la plus probable.
 * Jusqu'à présent, je considérais tous les cas simples
 * et j'essayais de traiter les cas complexes par
 * approximations successives. Avec l'idée que les
 * formes composées allaient nécessiter d'introduire
 * un deuxième type de fiches qui seraient conditionnelles.
 * Par exemple, "uicta <est>" est une analyse possible
 * du mot "uicta" à condition que la phrase contienne
 * aussi le mot "est". Si la phrase contient "erat" et
 * pas "est", "uicta <est>" ne convient pas, mais la fiche
 * "uicta <erat>" devra être retenue.
 * Cela crée aussi des contraintes au niveau du tagueur.
 *
 * Je voudrais changer d'approche et essayer de traiter
 * tous les cas de façon unifiée. J'ajoute pour cela à la clef
 * un complément d'information. La clef est la forme que je
 * dois reconnaître dans un texte pour déclencher l'analyse.
 * Pour les cas simples, le complément d'information est vide,
 * puisqu'il n'y a pas de restriction. En revanche, pour
 * certains cas complexes, ce complément désignera le (ou les)
 * mot(s) qu'il faudra trouver dans la phrase pour valider
 * cette analyse. La lemmatisation se fera donc en deux étapes :
 * d'abord on garde toutes les analyses, et ensuite on examine
 * l'ensemble de la phrase pour éliminer celles qui ne sont pas
 * possibles.
 *
 * Le 10 janvier 2017,
 *
 * Philippe.
 *
 * Le LASLA semble être passé au format BPN où le genre,
 * qui existait dans les codes en 5, a été réintroduit.
 * Je peux donc me demander s'il ne faudrait pas m'y mettre.
 * Avec le travail que j'ai déjà fait pour associer les lemmes
 * du LASLA avec ceux de Collatinus, ça ne doit pas être trop
 * compliqué à faire. Il faut analyser la forme et quand on trouve
 * le bon code en 9 (ou le bon tag), on ajoute le ou les genres.
 * Dans les codes en 5, ils sont numérotés de 1 à 6 :
 * commun, féminin, masculin et féminin, masculin,
 * masculin et neutre, neutre.
 * Les sondages que j'ai faits dans un BPN montrent que ça a été conservé.
 * Spontanément, j'aurais plutôt codé le genre en binaire sur 3 bits :
 * bit 0 pour le féminin, 1 masculin et 2 neutre. Ça introduit deux codes
 * supplémentaires : 0 = sans genre et 5 = féminin et neutre.
 * Ça donnerait un genre aussi aux noms...
 *
 * 11 juin 2017.
 */
class Fiche
{
public:
    Fiche(QString lg);
    ~Fiche();
    QString getForme ();
    QString getLemme ();
    QString getIndice ();
    QString getCode ();
    int getNbr ();
    QString getClef();
    QString getCmpl();
    QString getTag();
    QString info();
    QString Lasla(QString ref);
    QString humain();
    QStringList static const cats ;
    QStringList static const cass ;
    QStringList static const nombres ;
    QStringList static const degres ;
    QStringList static const personnes ;
    QStringList static const modes ;
    QStringList static const voixs ;
    QStringList static const tempss ;
    QString static clef(QString ff);
    // Pour la forme donnée par la fiche, je dois calculer
    // la forme du texte qui déclenche cette analyse.
    // Cette fonction est statique et publique, car
    // j'en aurai besoin pour ouvrir un fichier APN.
    QString static cmpl(QString ff, QString c);
    // Le complément d'information
    bool static testJointe(QString ff);
    bool jointe();
    void setClef(QString c);
    void setCmpl(QString c);
    bool estCondit();

private:
    QString _forme;
    QString _lemme;
    QString _indice;
    QString _code;
    QString _tag;
    int _nbr;
    QString _clef;
    QString _cmpl;
    bool _jointe;
};

#endif // FICHE_H
