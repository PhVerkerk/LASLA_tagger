# LASLA_tagger
Code and data for a HMM3 tagger (version 2) 
that transforms plain text files into APN-files 
(standard annotated files from the LASLA in Liège).

The code is written in C++ using the Qt libraries.
The free installation of Qt (https://www.qt.io/download-open-source)
is sufficient. I recommend the use of QtCreator.
This code has been successfully deployed on Mac (OS 10.13.6)
and Windows 10 and 11. 
It should work also on Linux, provided that a version
of Qt is availlable for the target system.

## Description
Ce programme associe une double lemmatisation et
une désambiguïsation par un modèle de Markov Caché du 3e ordre (HMM3).
La lemmatisation est faite à la fois à partir
de la liste des formes utilisées dans les textes annotés du LASLA
et à l'aide d'un lemmatiseur dérivé de Collatinus 11
(https://outils.biblissima.fr/fr/collatinus/
et https://github.com/biblissima/collatinus).

Une interface graphique guide le philologue dans l'utilisation du programme
et permet l'édition et la correction du fichier APN produit.

## References
LASLA Texts : https://dataverse.uliege.be/dataverse/lasla

Published description of the tagger : 
*L.A.S.L.A. and Collatinus: a convergence in lexica* ;
    Philippe Verkerk,
    Yves Ouvrard,
    Margherita Fantoli,
    Dominique Longrée
**in** Studi e Saggi Linguistici, V. 58 N. 1 (2020)
https://www.studiesaggilinguistici.it/index.php/ssl/article/view/275
