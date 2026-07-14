# UI-03 — Centre de démarrage

## Résultat produit

Lorsque la fenêtre d'édition ne contient aucun projet, la zone centrale vide est
remplacée par un accueil opérationnel. Il permet de créer un projet, d'ouvrir un
fichier ou de reprendre l'un des six projets récents. Dès qu'un projet est
effectivement ouvert, l'espace MDI historique reprend sa place. Le centre revient
après la fermeture du dernier projet.

Le lot reste une façade d'interface : il ne modifie ni les formats `.qet` et
`.elmt`, ni les données SQLite, ni les moteurs de sauvegarde, d'export ou d'Undo.

## Contrat ergonomique

- les boutons **Nouveau projet** et **Ouvrir un projet…** déclenchent les
  `QAction` historiques ; aucune logique de création ou de dialogue n'est
  dupliquée ;
- les récents présentent séparément le nom et le dossier, conservent le chemin
  exact dans leurs données et exposent le chemin complet à l'accessibilité ;
- Entrée, double-clic et menu contextuel ouvrent le récent sélectionné ;
- **Oublier de la liste** ne supprime jamais le fichier ;
- un chemin absent ou un partage momentanément indisponible n'est jamais retiré
  silencieusement : QElectroTech propose de le conserver ou de l'oublier ;
- aucun test d'existence n'est effectué lors de la construction de la liste,
  afin de ne pas bloquer le démarrage sur un chemin réseau ;
- le contenu passe de deux colonnes à une disposition verticale sous 760 pixels
  logiques et reste défilable sans défilement horizontal ;
- la police, les couleurs, la sélection et le focus proviennent de la palette
  et des métriques Qt ; aucun thème global ou couleur fixe n'est introduit.

Les exemples et modèles restent hors de ce premier incrément. Ils ne seront
affichés que lorsqu'ils pourront être empaquetés sous Windows et ouverts comme
copies non enregistrées, sans risque d'écraser leur source.

## Architecture

`QETDiagramEditor` utilise désormais un `QStackedWidget` central :

1. `StartCenterWidget` pour l'accueil ;
2. le `QSplitter` historique contenant le `QMdiArea` et la recherche.

`StartCenterPageController` porte la transition Qt-only et la remise du focus
sur l'action principale. Le retour après fermeture est différé d'un tour de
boucle, car `ProjectView::projectClosed` est émis avant le retrait effectif de la
sous-fenêtre MDI.

Lorsqu'un fichier est fourni en ligne de commande, la page éditeur est
présélectionnée avant le premier `show()`. L'accueil ne clignote donc pas avant
l'ouverture. Si toutes les ouvertures échouent, l'état réel est recalculé et
l'accueil apparaît.

`RecentFiles` expose maintenant `files()`, `filesChanged()` et `forgetFile()`.
Les changements sont persistés immédiatement, les anciennes clés sont effacées
et tous les centres ainsi que les menus **Récemment ouverts** des fenêtres actives
sont rafraîchis.

## Validation automatisée

La cible `start_center_test` et ses variantes couvrent :

- déclenchement exact et unique des actions Nouveau/Ouvrir ;
- ordre MRU, limite à six lignes, chemins Unicode et activation clavier ;
- persistance, déduplication, oubli et vidage des récents ;
- état vide et chemin indisponible sans suppression automatique ;
- noms accessibles, descriptions et ordre de focus ;
- budget logique 1280×720 avec texte à 150 % ;
- cent transitions du contrôleur accueil → éditeur → accueil sans page ni
  connexion dupliquée. La fermeture d'un vrai `ProjectView` reste un contrôle
  d'intégration Windows.

La cible est ajoutée explicitement au workflow Windows ainsi qu'aux matrices
clavier et DPI, afin que les alias CTest ne puissent pas rester enregistrés sans
exécutable construit.

## Validation Windows du 14 juillet 2026

Le binaire Release portable `0.100.1-dev` a été produit avec sa fermeture de
dépendances UCRT64 complète, puis lancé sur Windows 11 en parallèle de la stable
installée et de la préversion UX précédente. Les contrôles réalisés confirment :

- lancement sans projet : accueil visible dans la vraie fenêtre principale ;
- actions Nouveau/Ouvrir et liste de quatre projets récents exposées à UI
  Automation avec leurs noms et descriptions accessibles ;
- activation de **Nouveau projet** : création d'un projet avec premier folio et
  bascule immédiate vers l'espace d'édition historique ;
- 24/24 tests CTest réussis, dont les quatre variantes UI-03 ;
- budget 1280×720 avec texte à 150 % réussi sans défilement horizontal ou
  vertical dans le test déterministe ;
- coexistence de la préversion et de l'installation stable confirmée en
  utilisant l'exécutable portable explicitement nommé.

Restent à capturer dans un incrément de validation visuelle : fermeture manuelle
du dernier vrai `ProjectView`, lancement direct d'un `.qet` sans flash, dialogue
de récent absent, palettes sombre et contraste élevé. La transition de fermeture
est déjà couverte par cent cycles automatisés du contrôleur ; elle n'est pas
présentée comme validée visuellement ici.
