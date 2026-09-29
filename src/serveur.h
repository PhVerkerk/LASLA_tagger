/*            server.h
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
 * Je supprime tout ce qui était lié aux fenêtres pour ne garder que le serveur.
 * Philippe Janvier 2019.
 *
 */

#ifndef SERVEUR_H
#define SERVEUR_H

#include <QtNetwork>

#include "lasla.h"

class Server : public QObject
{
    Q_OBJECT

   public:
    Server();


   private slots:
    // Slots du serveur
    void connexion ();
    void exec ();

   private:
    // Pour le serveur
    QTcpServer * serveur;
    QTcpSocket * soquette;

    // cœur
    Lasla *_lasla;

    /*
     * Le 22 avril 2023, je sors tout ce qui n'avait rien à voir
     * avec le serveur pour le transférer dans Lasla.h et Lasla.cpp.
     * Ces modules vont quitter Collatinus puisqu'ils ne servent plus.
     * */

};

#endif
