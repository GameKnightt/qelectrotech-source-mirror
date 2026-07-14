# UI-04 — Shell contextuel et profil Essentiel lisible

## Objectif

UI-04 donne à l'accueil UI-03 un véritable mode sans projet et rend les
commandes centrales du profil Essentiel immédiatement compréhensibles. Le lot
reste une évolution du shell Qt : aucune action métier, aucun raccourci et aucun
format de document ne sont remplacés.

## Mode accueil contextuel

Lorsque la fenêtre ne contient aucun `ProjectView`, les cinq barres d'outils et
les cinq panneaux sont masqués et leurs actions d'affichage temporairement
désactivées. L'accueil peut ainsi utiliser toute la largeur disponible sans
présenter de commandes inactives.

`WorkspaceProfileController` mémorise séparément, pour chaque composant, sa
visibilité et l'état de son action. Dès qu'un premier projet est effectivement
ouvert, cet instantané est restauré à l'identique. Un changement de profil
effectué depuis l'accueil reconstruit d'abord l'état réel, applique le profil,
puis capture le nouvel état avant de revenir à l'accueil.

La sauvegarde de `QMainWindow::saveState()` quitte elle aussi temporairement le
mode accueil. Les barres et panneaux cachés uniquement parce qu'aucun projet
n'est ouvert ne peuvent donc jamais contaminer l'état persistant Essentiel ou
Classique. Au démarrage sans fichier, ce mode est appliqué avant l'affichage de
la fenêtre afin d'éviter un flash visuel.

## Hiérarchie du profil Essentiel

La barre **Projet et édition** affiche le texte à côté de l'icône pour les quatre
commandes primaires : Nouveau, Ouvrir, Enregistrer et Centre d'export. Annuler,
Rétablir, Couper, Copier et Coller restent compactes et conservent leur
infobulle, leur raccourci et leur `QAction` historique. La barre **Folio et
câblage** reste iconographique.

Les panneaux portent des noms métier plus explicites : **Projets et folios**,
**Composants**, **Propriétés**, **Historique** et **Numérotation automatique**.
Le menu **Affichage** expose directement :

- **Espace de travail** pour Essentiel et Classique ;
- **Panneaux** pour les cinq panneaux ;
- **Barres d'outils** pour les cinq barres ;
- **Réinitialiser la disposition**.

Le profil Classique conserve son contenu, ses icônes seules et ses onglets de
panneaux en position historique. Les noms d'objet utilisés par
`QMainWindow::saveState()` n'ont pas changé.

## Thèmes, dimensions et accessibilité

Le style ajouté est strictement structurel : espacements, marges et dimensions
minimales. Il ne fixe ni couleur, ni fond, ni police et reste donc compatible
avec les palettes Qt, les feuilles de style personnalisées, le mode sombre et
le contraste élevé. La taille des icônes provient de
`QStyle::PM_ToolBarIconSize`.

Les barres disposent d'un nom accessible. Le focus central est conservé lors
d'un changement de profil. À 150 % de taille de police et dans une fenêtre
1280 × 680, les treize actions représentatives restent accessibles sans
débordement et la zone de dessin conserve au moins 640 × 420 pixels.

## Validation effectuée

Validation locale sous Windows 11 avec Qt 5 :

- compilation complète Debug de `qelectrotech.exe` ;
- suite CTest complète : 27 tests réussis sur 27 ;
- quatre alias UI-04 en échelle normale et trois à `QT_SCALE_FACTOR=1.5` ;
- 100 cycles accueil/édition sans dérive d'état ;
- état persistant vérifié indépendamment du masquage temporaire ;
- focus vérifié après changement de profil ;
- inspection visuelle réelle du démarrage, de la création d'un projet, de la
  restauration du shell, du profil Essentiel et du menu Affichage.

## Limites et suite

Les parcours visuels réels en mode sombre, contraste élevé Windows et mise à
l'échelle système 150 % restent à rejouer sur la version portable finale. La
fermeture du dernier projet doit également être confirmée sans abandonner un
document non enregistré. Un incrément ultérieur pourra extraire entièrement la
persistance de fenêtre vers un contrôleur dédié et gérer explicitement les
données de disposition corrompues.
