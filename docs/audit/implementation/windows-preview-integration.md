# Préversion Windows intégrée

## Objectif

Cette branche réunit les premiers lots de stabilisation et d’ergonomie dans une
base unique, compilable et testable sous Windows 11. Elle sert de porte d’entrée
à la modernisation visuelle de l’interface et aux prochaines fonctions métier.

## Composition

La branche `codex/windows-preview-integration` part de
`codex/qet-audit-baseline` et intègre les sommets suivants :

- `codex/save-02-visible-state`, qui contient aussi SAVE-01 ;
- `codex/ux-01-responsive-dialogs` ;
- `codex/ux-02b-large-project-performance`, qui contient aussi UX-02 ;
- `codex/ux-03-collection-search` ;
- `codex/ux-04-conductor-selection`.

Les branches parentes SAVE-01 et UX-02 ne sont pas intégrées une seconde fois.
Les conflits de fusion étaient limités au workflow Windows, à la liste des
sources CMake et au manifeste QtTest. Leur résolution conserve toutes les cibles
et toutes les suites de tests.

## Validation locale du 13 juillet 2026

Environnement : Windows 11, MSYS2 UCRT64, Qt 5, CMake et Ninja.

- configuration Debug avec `BUILD_TESTING=ON` et `PACKAGE_TESTS=ON` ;
- compilation complète de `qelectrotech.exe` réussie ;
- exécutable Debug produit : environ 284 Mo ;
- CTest : 10 entrées réussies sur 10 ;
- dialogue responsive rejoué avec `QT_SCALE_FACTOR=1.5` : réussite ;
- budgets déterministes de navigation et de recherche inclus dans la suite ;
- compilation Release et démarrage de l’application portable : réussite ;
- dépendances UCRT64 transitives résolues et pilote SQLite conservé ;
- CI Windows de la branche d’intégration : deux exécutions réussies ;
- aucun changement de format `.qet`, `.elmt` ou XML introduit par la fusion.

Suites exécutées :

1. tests Catch2 historiques et sauvegarde atomique ;
2. tests Qt historiques ;
3. remappage des UUID lors de la duplication ;
4. dialogues adaptatifs ;
5. état visible de sauvegarde ;
6. recherche de composants ;
7. interaction avec les conducteurs ;
8. cache d’index ordonné ;
9. navigateur de folios ;
10. navigateur de folios avec texte agrandi à 150 %.

La version Release a également été parcourue dans l’application réelle sous
Windows 11. Les vérifications ont couvert le démarrage avec les collections,
la création d’un projet, l’état de sauvegarde visible, le navigateur de folios,
la recherche d’un composant hydraulique et le démarrage puis l’annulation de
son placement au clavier. Le projet temporaire n’a pas été enregistré.

Le script `packaging/windows/deploy-portable.ps1` rend ce déploiement répétable
avec le même instantané d’outils. Il complète `windeployqt-qt5` avec la fermeture
transitive des dépendances MSYS2/UCRT64, retire les pilotes SQL inutilisés et
ajoute les collections, cartouches, traductions et licences. Il valide les
plugins requis, démarre l’exécutable sans MSYS2 dans le `PATH` et produit un
manifeste SHA-256. Sa procédure est documentée dans
`docs/development/windows-portable-preview.md`. Cette sortie reste réservée aux
essais internes tant que l’inventaire complet des licences tierces et un test
sur une machine sans MSYS2 ne sont pas terminés.

## Contraintes de dépôt résolues par DEV-01

La préversion initiale avait confirmé deux blocages de contribution sous
Windows : la collision de casse entre `ChangeLog.MD` et `ChangeLog.md`, puis le
pointeur Git LFS de 530 Mo vers une aide Qt Creator générée. DEV-01 déplace les
deux historiques Markdown sous des noms distincts dans `docs/history/`, conserve
le `ChangeLog` installé et retire l’artefact `.qch` du checkout. Un clone normal
ne nécessite donc plus de désactiver Git LFS et ne doit plus apparaître modifié.

## Porte d’entrée vers la modernisation visuelle

La compilation, la CI et le parcours Release portable sont validés. La phase
suivante peut donc commencer sur cette base : système visuel Qt commun,
hiérarchie des actions, panneaux et barres d’outils, densité, thèmes, icônes,
clavier et accessibilité. Des projets anonymisés représentatifs resteront
nécessaires avant de stabiliser cette préversion pour des utilisateurs métier.
