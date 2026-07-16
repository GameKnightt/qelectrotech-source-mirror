# Audit complet et cadrage du fork QElectroTech

**Date de référence :** 13 juillet 2026  
**Branche auditée :** `master` à `5c65bf648627d112d0a09ad7ac80981b5efeef67`  
**Version source :** 0.100.1  
**Version observée sous Windows 11 :** 0.100.0  
**Branche de travail :** `codex/qet-audit-baseline`

## Synthèse exécutive

QElectroTech possède une base fonctionnelle solide pour un fork industriel compatible avec l'amont : formats XML ouverts, collection riche, éditeur multi-folios, références croisées, numérotation, nomenclature, bornier, impression/export, traductions et automatisation en ligne de commande en progression. Le bon choix n'est pas une réécriture, mais une modernisation incrémentale qui préserve les contrats de fichiers et le modèle mental existant.

La priorité immédiate n'est pas esthétique. Deux risques d'intégrité signalés sur la version actuelle doivent être sécurisés avant toute refonte : duplication de folios pouvant réutiliser des UUID et perdre des lignes de sommaire/nomenclature, puis priorité incorrecte entre variables personnalisées de projet et de folio. Viennent ensuite quatre frictions Windows répétées : dialogues trop hauts, navigation difficile dans les grands projets, recherche arborescente peu exploitable et édition/sélection des conducteurs trop coûteuse.

La phase 1 peut démarrer sans décision produit supplémentaire avec ce séquencement : tests de non-régression des formats et de l'intégrité, assainissement du build Windows, correction responsive des dialogues, puis améliorations ciblées de recherche, navigation et propriétés.

## Périmètre, méthode et niveau de preuve

L'audit combine quatre sources :

1. inspection statique de `master` 0.100.1 et comparaison avec les branches Qt 6 visibles ;
2. exécution de la stable 0.100.0 installée sous Windows 11 ;
3. étude des tickets GitHub, du forum, de la documentation et de la roadmap ;
4. comparaison limitée de modèles d'interaction documentés officiellement chez KiCad, EPLAN et AutoCAD Electrical.

Chaque constat utilise au moins une preuve de type `UI`, `CODE` ou `COMMUNAUTÉ`. Les preuves visuelles sont indexées dans [evidence/README.md](evidence/README.md). La collecte a validé les écrans en direct, mais l'export local des PNG n'a pas pu être fiabilisé ; cette limite est conservée comme blocage de preuve et les scénarios à rejouer sont nommés.

Cet audit n'est ni une certification d'accessibilité, ni une validation métier exhaustive. Les résultats pneumatiques, hydrauliques, process, câbles, borniers et E/S automate restent partiels faute de projets anonymisés représentatifs.

## État de référence et compatibilité

### Stable Windows 0.100.0

L'écran À propos de l'installation auditée indique QElectroTech 0.100.0, Qt 5.15.18, GCC 14.3.0, compilation du 25 janvier 2026 et Windows 11 noyau 10.0.26200. Le projet d'exemple installé `industrial.qet` pèse environ 2,6 Mo, contient 150 folios et s'est chargé en environ 7,5 secondes sur la machine d'audit.

### `master` 0.100.1

Le dépôt déclare C++17, Qt 5, SQLite3, SingleApplication et pugixml. `master` ajoute notamment une interface d'export en ligne de commande, des améliorations PDF et de références, la récupération de sauvegarde automatique, des fonctions de duplication/macro et des correctifs de stabilité. Ces fonctions n'ont pas été attribuées à la stable lorsqu'elles n'y ont pas été observées.

### Contrats à préserver

- projets `.qet` et définitions `.elmt` XML ;
- cartouches et collections existantes ;
- UUID, références et relations entre éléments, conducteurs, folios et base projet ;
- paramètres utilisateur, traductions et raccourcis ;
- nomenclatures, CSV, PDF, images et impression ;
- ouverture et réenregistrement sans perte de contenu ou dérive silencieuse.

Toute évolution de schéma devra être additive, versionnée, testée en aller-retour et lisible par la version amont tant qu'aucune migration explicite n'est décidée.

## Architecture cartographiée

| Domaine | Responsabilités principales | Zones de code représentatives | Risque de modification |
|---|---|---|---|
| Application et fenêtre | cycle de vie, projets, menus, actions, préférences | `sources/main.cpp`, `sources/qetapp.*`, `sources/qetdiagrameditor.*` | élevé : surface centrale et état global |
| Projet et folios | chargement XML, propriétés, UUID, navigation, base associée | `sources/qetproject.*`, `sources/diagram.*`, `sources/projectview.*` | critique : compatibilité et intégrité |
| Éditeur graphique | scène, vue, sélection, déplacement, copier-coller, événements | `sources/diagramview.*`, `sources/diagramevent/`, `sources/UndoCommand/` | élevé : interactions et annulation |
| Éléments et collections | recherche, modèles d'arbres, cache, import, éditeur d'élément | `sources/ElementsCollection/`, `sources/editor/`, `sources/elementscollectioncache.*` | moyen à élevé |
| Conducteurs et connexions | création, routage, propriétés, numérotation, export | `sources/conductor*`, `sources/autoNum/` | élevé : parcours métier central |
| Propriétés et édition groupée | panneaux, dialogues, recherche/remplacement | `sources/ui/`, `sources/SearchAndReplace/`, `sources/properties/` | moyen ; bon point d'entrée UX |
| Données projet | SQLite, sommaire, nomenclature, requêtes | `sources/dataBase/` | critique : cohérence XML/SQLite |
| Borniers | modèle, numérotation, éditeur/greffon | `sources/TerminalStrip/` | élevé : fonction en maturation |
| Sorties | impression, PDF, images, CSV, CLI | `sources/print*`, `sources/export*`, `sources/cli_export.*` | moyen à élevé |
| Sauvegarde | sauvegarde projet, fichier automatique, récupération, tâche asynchrone | `sources/qetproject.*`, classes `KAutoSaveFile` et sauvegarde asynchrone | critique |
| Tests | Catch, QtTest, GoogleTest/Mock | `tests/`, activation CMake | couverture insuffisante des contrats |
| Packaging | CMake, workflow MSYS2, installateur | `CMakeLists.txt`, `.github/workflows/windows-build.yml`, `packaging/` | élevé pour l'expérience développeur |

Le modèle reste fortement couplé autour de `QETProject`, `Diagram`, de la fenêtre éditeur et des modèles Qt. Pour préserver l'amont, la refonte UX doit d'abord extraire des modèles de présentation et des commandes réutilisables, pas déplacer massivement le modèle métier.

## Build Windows 11 reconstitué

### Procédure actuelle dérivée des scripts réels

La documentation historique fournie utilise encore Qt 4.4, Subversion, qmake et `mingw32-make`; elle ne doit pas guider un nouveau contributeur. Le workflow GitHub actuel utilise MSYS2 UCRT64, GCC, Ninja, Qt 5/KF5, SQLite, CMake et les sous-modules Git.

Procédure de référence proposée pour `master` :

1. installer MSYS2 et ouvrir **MSYS2 UCRT64** ;
2. installer Git, CMake, Ninja, GCC UCRT64, Qt 5, KF5 et SQLite selon le workflow CI ;
3. cloner normalement le fork avec ses sous-modules ;
4. configurer un répertoire `build` avec Ninja et les préfixes UCRT64 ;
5. compiler puis exécuter les tests activés par `PACKAGE_TESTS` ;
6. déployer les DLL Qt/KF et valider un lancement hors environnement de build.

Depuis l'audit initial, la procédure a été exécutée sous Windows 11 avec MSYS2
UCRT64 et Qt 5. L'application complète, les tests CTest et la préversion
portable sont également reconstruits par le workflow Windows du fork.

### Défauts d'expérience développeur confirmés

- Le quota Git LFS du dépôt amont était épuisé pour `doc/QElectroTech.qch`
  (~530 Mo). DEV-01 supprime ce pointeur généré du checkout courant : le fichier
  `.qch` reste reproductible par Doxygen et n'est requis par aucun build ou
  parcours utilisateur.
- `ChangeLog.MD` et `ChangeLog.md` entraient en collision sur un système de
  fichiers Windows insensible à la casse. Les copies Markdown redondantes ont
  été retirées ; `ChangeLog` reste le changelog canonique installé.
- Le workflow Windows transmet `-DBUILD_TESTING=OFF`, alors que le projet pilote ses tests avec `PACKAGE_TESTS`. Le paramètre CI peut donc ne pas produire l'effet attendu.
- La documentation Windows moderne existe surtout sur une branche Qt 6, pas comme parcours contributeur cohérent sur `master`.
- Les tests présents couvrent quelques utilitaires et la récupération automatique, mais pas les invariants critiques UUID, variables, aller-retour XML/SQLite et duplication de folios.

## Trajectoire Qt 6

`master` reste Qt 5. Les branches `qt6-cmake`, `qt6_cmake_joshua` et `msys2` montrent un port actif mais divergent. La branche Qt 6 inspectée remplace Qt 5/KF5 par Qt 6/KF6, rend les tests désactivés par défaut et fournit un guide MSYS2/Qt Creator plus actuel. Un ticket ouvert signale encore des défauts de compilation et des correctifs existent hors de `master`.

Décision : ne pas bloquer les gains UX immédiats sur Qt 6. Toute nouvelle couche de présentation doit toutefois éviter les API Qt 5 obsolètes, isoler les dépendances KDE et être testable sur les deux lignes jusqu'à convergence.

## Audit des parcours Windows

### 1. Démarrage et espace de travail — santé : moyenne

**Preuves :** E01, E02, CODE `qetdiagrameditor`, docks Qt.  
**Points forts :** création rapide d'un projet, canevas immédiatement disponible, docks personnalisables.  
**Friction :** le premier écran présente une forte densité de petites icônes, peu de hiérarchie et trois zones latérales concurrentes. La grande surface centrale vide n'oriente pas vers les premières actions.  
**Recommandation :** conserver les actions et raccourcis, mais livrer une configuration par défaut simplifiée, une barre d'actions contextuelle et un état vide proposant projet, modèle, exemple et récent.

### 2. Folios, cartouches, sommaires et navigation — santé : fragile à grande échelle

**Preuves :** E04, E09, UI projet de 150 folios.  
**Points forts :** visibilité permanente de l'arborescence, accès direct au folio.  
**Friction :** liste très longue, libellés tronqués, hiérarchie visuelle faible et outils de sommaire/nomenclature dispersés.  
**Recommandation :** recherche plate des folios, groupes repliables, favoris/récents, historique précédent-suivant et indicateur de position.

### 3. Recherche, placement et création d'éléments — santé : moyenne

**Preuves :** E06, E12, E16 ; modèles `ElementsCollection`.  
**Points forts :** bibliothèque riche et recherche simultanée.  
**Friction :** la recherche développe les arbres, mélange contexte et résultats, n'affiche pas de compteur ni de classement et tronque les noms. La discipline n'est pas un filtre de premier rang.  
**Recommandation :** liste de résultats dédiée avec aperçu, chemin complet, discipline, provenance, favoris et navigation clavier. Conserver l'arbre comme mode exploration.

### 4. Conducteurs, connexions, routage et numérotation — santé : à améliorer

**Preuves :** E15 partielle ; tickets #500 et #436 ; code `conductor*` et `autoNum`.  
**Points forts :** numérotation et propriétés riches, création automatique disponible.  
**Friction :** cible de sélection fine, propriétés peu visibles, absence d'édition tabulaire/groupée intégrée et routage aux croisements encore demandé.  
**Recommandation :** tolérance de sélection indépendante du zoom, surbrillance avant clic, panneau Conducteur persistant, édition multi-sélection et aperçu du reroutage.

### 5. Liaisons, renvois et références croisées — santé : moyenne

La base existe et `master` améliore les liens PDF. Le parcours n'a pas été validé de bout en bout sur un projet utilisateur. Il faut ajouter un diagnostic de liens cassés, une navigation aller-retour et un test d'export PDF conservant les renvois.

### 6. Propriétés, modifications groupées, copier-coller et annulation — santé : moyenne

**Preuves :** E05 ; ticket #413 ; piles d'annulation du projet/diagramme.  
**Points forts :** panneau de propriétés et infrastructure Undo disponibles.  
**Friction :** dock étroit, formulaire long, séparation peu lisible entre identifiant, affichage, fabricant et données métier ; couverture d'annulation hétérogène.  
**Recommandation :** sections repliables, champs essentiels en tête, largeur minimale adaptative, multi-édition avec états mixtes et matrice de tests Undo/Redo.

### 7. Borniers, câbles, E/S automate et fabricant — santé : partielle

**Preuves :** E09, E14 ; tickets #405 et #409.  
Le menu expose plusieurs entrées de bornier dont une marquée DEV, ce qui fragmente la confiance. L'exemple installé ne permet pas de valider TB1. Les métadonnées de câbles existent mais la nomenclature groupée par câble est demandée.  
**Recommandation :** un seul espace Borniers et câbles avec état stable/expérimental explicite, représentation tabulaire, aperçu graphique et validation avant génération.

### 8. Pneumatique, hydraulique et process — santé : non concluant

**Preuves :** E12.  
La collection contient des éléments liés à ces disciplines, mais la recherche textuelle ne prouve ni un workflow dédié ni une gestion sémantique de fluides. Validation bloquée faute de projets représentatifs. Ne pas revendiquer une couverture industrielle complète avant rejeu.

### 9. Nomenclatures, câblage, impression et exports — santé : bonne base, garde-fous faibles

**Preuves :** E08 à E11 ; `cli_export.*` sur `master`.  
**Points forts :** nomenclature, CSV, PDF, images, impression et automatisation CLI.  
**Friction :** les options de nomenclature peuvent ajouter des folios par défaut sans aperçu d'impact ; sorties réparties entre menus et dialogues ; presets peu visibles.  
**Recommandation :** centre d'export unique, préréglages nommés, résumé des fichiers/folios créés, aperçu et journal des avertissements.

### 10. Sauvegarde, récupération, préférences, raccourcis et affichage — santé : moyenne

**Preuves :** E03, E07 ; code de récupération `KAutoSaveFile`.  
**Points forts :** sauvegarde automatique et récupération améliorées dans `master`.  
**Friction :** état de sauvegarde peu saillant, configuration trop haute et navigation de réglages imbriquée.  
**Recommandation :** statut Sauvegardé/En cours/Erreur dans la fenêtre, récupération expliquée, recherche dans les préférences et restauration des réglages par section.

## Windows 11, DPI et accessibilité

Le problème historique de dialogues trop hauts est toujours pertinent sur la stable auditée : Propriétés du projet et Configuration approchent 1402×1032 à 1920×1080. Le code de `configdialog.cpp` limite la fenêtre à 94 % de l'écran, mais demande jusqu'à 1400×1000, tandis qu'une page de configuration impose au moins 800×650. L'export impose au moins 800×590. À 125 % ou 150 %, les marges de sécurité deviennent insuffisantes.

Risques accessibles observables, sans prétendre à une certification :

- ordre de focus et lecture clavier difficiles à prédire dans les dialogues à double navigation ;
- actions iconographiques sans libellé visible dans certaines barres et dans la nomenclature ;
- arborescences très denses et libellés tronqués ;
- contraste et états sélection/survol à vérifier dans les thèmes système ;
- arbre d'accessibilité exposant parfois le dialogue modal et les contrôles sous-jacents ;
- champs dont le nom accessible n'était pas toujours évident pendant l'inspection.

Critère transversal : toutes les nouvelles interfaces doivent fonctionner clavier seul, annoncer nom/état/erreur, respecter le thème et le contraste système, et rester utilisables à 150 % sans bouton hors écran.

## Performance et stabilité

Le projet d'exemple de 150 folios donne un premier ordre de grandeur, pas un benchmark. Son chargement d'environ 7,5 s est acceptable pour un fichier d'exemple complexe mais la navigation reste lourde visuellement. Aucun profil CPU/mémoire n'a été capturé.

Les priorités de stabilité viennent des invariants plutôt que de mesures prématurées : UUID uniques, cohérence XML/SQLite, sauvegarde atomique, récupération, annulation et génération de sorties déterministe. Un ancien ticket de crash pendant la fermeture avant fin de chargement de collection n'a pas été reproduit ; il indique néanmoins une fragilité asynchrone à couvrir par test.

## Constatations prioritaires

| ID | Priorité | Constat | Preuve | Résultat attendu |
|---|---|---|---|---|
| DATA-01 | P0 | Duplication de folio susceptible de conserver des UUID et d'entraîner des contraintes SQLite ou des composants absents du sommaire | GitHub #532, code projet/DB | chaque objet dupliqué reçoit un UUID unique et tous les rapports restent complets |
| DATA-02 | P0 | Variables personnalisées de folio pouvant écraser à vide celles du projet | GitHub #531, `assignvariables.cpp` | priorité explicite et stable, aucun champ rendu vide sans action utilisateur |
| UX-01 | P1 | Dialogues principaux trop hauts à 1920×1080 et risqués en DPI | E03, E07, forum, code | actions toujours visibles à 150 %, défilement uniquement dans le contenu |
| UX-02 | P1 | Navigation peu efficace sur 150 folios | E04 | atteindre un folio par clavier/recherche en moins de trois actions |
| UX-03 | P1 | Recherche de composants non structurée | E06, E12 | liste classée, compteur, aperçu et filtres sans perdre l'arbre |
| UX-04 | P1 | Sélection et édition de conducteurs coûteuses | E15, #500 | survol, zone de prise tolérante et propriétés modifiables en groupe |
| IND-01 | P1 | Bornier/câbles fragmentés ou incomplets | E09, E14, #405, #409 | parcours unique avec validation et nomenclature câble |
| DEV-01 | P1 | Clone/build Windows fragile | LFS, collision de casse, CI | clone reproductible et build documenté sur machine propre |
| TECH-01 | P1 | Divergence Qt 5/Qt 6 | branches Qt 6, #402 | matrice CI commune et trajectoire de fusion mesurable |
| UI-01 | P2 | Densité des barres et actions iconographiques | E01, E08 | profils de barres, libellés/infobulles cohérents et découverte améliorée |
| A11Y-01 | P2 | Focus, noms accessibles, contraste et thèmes non systématiquement garantis | inspection UI, #467 | tests clavier et accessibilité automatisés sur écrans critiques |

Le détail priorisé, les dépendances et critères d'acceptation sont dans [backlog-roadmap.md](backlog-roadmap.md).

## Benchmark d'interactions, sans transposition aveugle

| Produit | Modèle documenté utile | Application possible à QElectroTech |
|---|---|---|
| KiCad 10 | panneau/table de résultats triables, filtres avancés, clic qui ouvre la feuille et sélectionne l'objet ; édition groupée des champs | séparer résultat et arbre, navigation directe, édition tabulaire |
| EPLAN 2026 | édition en tableau de fonctions/connexions, schémas de colonnes adaptés à l'utilisateur ou à la discipline | vues nommées et colonnes configurables pour conducteurs, éléments et folios |
| AutoCAD Electrical 2025 | éditeur de bornier global, tri, multi-sélection, destinations internes/externes, cavaliers, accessoires et aperçu graphique/tabulaire | espace Borniers et câbles unifié avec aperçu et validation |

Ces références inspirent des patterns d'interaction ; elles ne prouvent aucun défaut QElectroTech et ne doivent pas importer leur complexité ou leur modèle de licence.

## Points forts à préserver

- formats ouverts et inspectables ;
- large collection multi-discipline et traductions nombreuses ;
- espace de travail à docks adaptable ;
- numérotation, renvois, nomenclature, bornier et données fabricant déjà présents ;
- annulation et commandes structurées dans de nombreuses zones ;
- exports graphiques, PDF, CSV et automatisation CLI sur `master` ;
- récupération de sauvegarde et correctifs actifs en amont ;
- communauté francophone et historique industriel réel.

## Limites de vérification

- aucun projet utilisateur anonymisé n'était disponible ;
- le build local n'a pas été exécuté faute de toolchain ;
- aucun test de charge supérieur au projet d'exemple n'a été réalisé ;
- la capture visuelle a été validée en session mais pas exportée en PNG de manière fiable ;
- E15 et E16 sont des blocages d'automatisation, pas des défauts confirmés ;
- les branches et tickets représentent l'état observé le 13 juillet 2026 et peuvent évoluer.

## Conclusion de cadrage

Le fork doit rester proche de l'amont et commencer par la confiance : intégrité des données, build reproductible et tests de compatibilité. La refonte ergonomique peut ensuite avancer verticalement, parcours par parcours, en conservant l'interface classique disponible pendant la transition. Le premier lot de développement recommandé contient DATA-01, DATA-02, DEV-01 et UX-01 ; il réduit le risque avant d'introduire les nouvelles vues de recherche, navigation et édition tabulaire.

## Sources publiques

- [Dépôt QElectroTech](https://github.com/qelectrotech/qelectrotech-source-mirror)
- [Documentation historique de compilation](https://qelectrotech.org/wiki_new/en/doc/test_dev_version)
- [Roadmap officielle](https://qelectrotech.org/wiki_new/doku.php?id=roadmap)
- [Forum : dialogues trop hauts sous Windows 11](https://qelectrotech.org/forum/viewtopic.php?id=3073)
- [GitHub #532 : UUID après duplication](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/532)
- [GitHub #531 : variables de cartouche](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/531)
- [GitHub #500 : propriétés des conducteurs](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/500)
- [GitHub #405 : nomenclature des câbles](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/405)
- [GitHub #409 : gestionnaire de borniers](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/409)
- [GitHub #402 : build Qt 6](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/402)
- [Documentation KiCad 10, Schematic Editor](https://docs.kicad.org/10.0/en/eeschema/eeschema.html)
- [EPLAN 2026, Edit in table](https://www.eplan.help/en-us/Infoportal/Content/Plattform/2026/Content/htm/functiondatagridgui_h_tabellarischbearbeiten.htm)
- [AutoCAD Electrical 2025, Terminal Strip Editor](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-Electrical/files/GUID-A6F3EEF1-5099-4642-80C4-439E68D302E4.htm)
