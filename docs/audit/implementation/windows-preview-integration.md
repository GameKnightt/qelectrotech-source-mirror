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
- CTest : 10 scénarios réussis sur 10 ;
- dialogue responsive rejoué avec `QT_SCALE_FACTOR=1.5` : réussite ;
- budgets déterministes de navigation et de recherche inclus dans la suite ;
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

## Contraintes de dépôt observées

Le dépôt contient trois chemins ne différant que par la casse (`ChangeLog`,
`ChangeLog.MD` et `ChangeLog.md`). Un worktree Windows ne peut pas matérialiser
ces trois fichiers simultanément sans apparaître modifié. Aucun de ces chemins
n’est inclus dans la branche d’intégration.

Le budget Git LFS du dépôt est dépassé pour `doc/QElectroTech.qch` (environ
530 Mo). Le worktree a donc été créé avec le smudge LFS désactivé ; ce fichier
d’aide n’est requis ni pour la compilation de l’application ni pour les tests.

## Porte de sortie avant modernisation visuelle

Après validation de la CI de cette branche, la prochaine phase peut commencer
sur cette base : système visuel Qt commun, hiérarchie des actions, panneaux et
barres d’outils, densité, thèmes, icônes, clavier et accessibilité. Une version
Release portable devra ensuite être rejouée sur des projets anonymisés avant de
stabiliser cette préversion pour des utilisateurs métier.
