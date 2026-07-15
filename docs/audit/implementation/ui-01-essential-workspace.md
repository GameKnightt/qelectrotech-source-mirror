# UI-01 — Espace de travail Essentiel

## Objectif

Ce lot ouvre la modernisation progressive de QElectroTech sans modifier les
actions métier ni les formats de projet. Il réduit la charge visuelle initiale,
conserve un accès direct aux commandes fréquentes et permet de revenir à
l'interface historique à tout moment.

## Profils

Le profil **Essentiel** est utilisé lors d'une nouvelle installation. Il affiche
deux barres d'outils :

- projet et édition : Nouveau, Ouvrir, Enregistrer, Centre d'export, Annuler,
  Rétablir, Couper, Copier et Coller ;
- schéma : atteindre un folio, ajouter un folio, propriétés du folio et création
  automatique des conducteurs.

Les panneaux Projets, Collections et Propriétés restent visibles. Collections
et Propriétés partagent la même zone à onglets. Les barres avancées et les
panneaux Annulations et Numérotation automatique restent enregistrés dans la
fenêtre principale : ils peuvent être réaffichés depuis le menu standard
**Afficher**, avec l'intégralité de leurs commandes. Cette conservation est
nécessaire pour les outils avancés qui n'ont pas d'entrée de menu équivalente.

Le profil **Classique** restitue les cinq barres d'outils et les panneaux de la
version historique, avec les mêmes instances de `QAction`, raccourcis et états.
Le menu **Affichage > Espace de travail** permet de changer de profil ou de
réinitialiser uniquement la disposition courante.

## Migration et compatibilité

Les clés de réglages sont :

- `diagrameditor/workspace_profile` pour le profil sélectionné ;
- `diagrameditor/essential_state` pour la disposition Essentiel ;
- `diagrameditor/state` pour la disposition Classique historique.

Une installation possédant déjà `diagrameditor/state`, mais aucune sélection de
profil, est considérée comme Classique. Son état est restauré sans migration
destructive. Une installation sans état démarre en Essentiel. La géométrie de
la fenêtre reste indépendante du profil et n'est pas effacée lors d'une
réinitialisation.

Les noms d'objet historiques des barres et panneaux sont conservés pour
`QMainWindow::saveState()`. Le panneau de numérotation automatique reçoit aussi
un nom stable afin que sa disposition soit désormais persistée.

Ce lot ne change aucun format `.qet`, `.elmt`, XML ou SQLite, ni la pile Undo,
les exports ou les raccourcis existants.

## Thème et accessibilité

Les palettes claires forcées des deux arbres de collections ont été retirées.
Ils héritent maintenant de la palette Qt choisie par l'utilisateur, notamment
pour le thème sombre et le contraste élevé Windows. La taille des icônes des
barres provient du style Qt, sans dimension fixe, et les barres ainsi que le
menu de profil exposent des noms accessibles.

## Validation

Validation locale sous Windows 11, Qt 5 et Ninja, avec les tests graphiques
exécutés sur la plateforme Qt `offscreen` comme dans la CI :

- compilation complète Debug de `qelectrotech.exe` ;
- 11 tests CTest réussis sur 11 ;
- test du profil répété sur 100 allers-retours sans duplication d'action ;
- aller-retour d'un état Classique personnalisé vérifié octet pour octet ;
- tests du profil et des dialogues rejoués avec `QT_SCALE_FACTOR=1.5` ;
- parcours réel au clavier : Essentiel vers Classique, retour vers Essentiel,
  fermeture et relance avec persistance du profil.

Le test `workspace_profile_test` est ajouté à la CI Windows. Il vérifie la
composition des deux profils, la stabilité des valeurs enregistrées, la
disponibilité des commandes omises via les menus et la présence de chaque
commande d'affichage standard.
