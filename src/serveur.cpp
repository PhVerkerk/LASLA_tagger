/*   serveur.cpp
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
 *
 * Je reprends le serveur du daemon de Collatinus-web
 * pour l'utiliser dans le tagueur à intégrer à HyperBase.
 * Philippe Septembre 2022.
 *
 */

#include "serveur.h"
#include <QDateTime>

/**
 * \fn Server::Server()
 * \brief Créateur du serveur pour le démon.
 */
Server::Server()
{
    _lasla = new Lasla(this);
    /*
     * Comme je n'ai pas besoin d'avoir accès à LemCore,
     * ni aux autres modules de Collatinus,
     * je ne crée qu'un Lasla, qui crée lui-même le LemCore
     * dont il a besoin.
     * Peut-être faudra-t-il créer quelques autres fonctions relais,
     * par exemple, pour activer la gestion des graphies médiévales...
     *
     * En réalité, le module Lasla ne sert plus du tout dans Collatinus.
     * Je répartis la matière différemment entre les deux.
     * Ça pourrait simplifier la lecture et les possibles mises à jour.
     */

    // Définition et lancement du server
    serveur = new QTcpServer (this);
    connect (serveur, SIGNAL(newConnection()), this, SLOT (connexion ()));
    if (!serveur->listen (QHostAddress::LocalHost, 5555))
    {
        std::cout << "Ne peux écouter.\n";
        return;
    }
    std::cout << "Le Tagueur tourne en console. Ctrl-C pour terminer.\n";

}

void Server::connexion ()
{
    soquette = serveur->nextPendingConnection ();
    connect (soquette, SIGNAL (readyRead ()), this, SLOT (exec ()));
}

void Server::exec ()
{
    QByteArray octets = soquette->readAll ();
    QString requete = QString (octets).trimmed();
#ifdef VERIF
    std::cout << requete.toStdString() << "\n";
#endif

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

    QString nomBidon = "busy.txt";
    QFile blabla(nomBidon);
    blabla.open(QFile::WriteOnly|QFile::Text);
    blabla.write("blabla");
    blabla.close();
    /* Afin de pouvoir éventuellement implémenter une file d'attente
     * du côté du client de cette lemmatisation,
     * je crée un fichier bidon "busy.txt" dans le répertoire courant.
     * On pourrait aussi le mettre ailleurs :
     * - soit en donnant un chemin complet dont le dossier existe
     * - soit en utilisant une chaine comme QDir::homePath() ou QDir::tempPath()
     * Le but étant que le monde extérieur puisse voir ce fichier,
     * on évitera les endroits trop improbables.
    */

    QString rep = _lasla->traite(requete);

    if (!rep.endsWith("\n")) rep.append("\n");
    // Pour la lisibilité du code, je sors le traitement de la requête
    // de ce qui est la gestion du serveur lui-même.

    QByteArray ba = rep.toUtf8();
#ifdef VERIF
    std::cout << rep.toStdString() << "\n";
#endif

    debut = QDateTime::currentDateTime().toString(Qt::ISODate);
    entier_deb = QDateTime::currentMSecsSinceEpoch() - entier_deb;
    stats.open(QFile::Append|QFile::Text);
    debut.prepend("Fin : ");
    debut.append("\n");
    stats.write(debut.toUtf8());
    debut = "Durée : %1 ms.\n";
    stats.write(debut.arg(entier_deb).toUtf8());
    rep.append("\n");
    stats.write(rep.toUtf8());
    stats.close();

    QFile::remove(nomBidon);
    // Supprime le fichier bidon créé précédemment.
    // Pour signaler au monde extérieur que le serveur est à nouveau disponible.

    soquette->write(ba);
    soquette->close ();
}
