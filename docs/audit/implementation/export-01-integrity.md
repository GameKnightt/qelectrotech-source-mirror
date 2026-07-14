# EXPORT-01 — Intégrité des exports interactifs

## Objectif

Ce lot sécurise les exports déjà présents avant de les exposer dans le futur
centre d'export UI-02. Il ne change ni les formats de projet ni les commandes
historiques. Son contrat est simple : un export ne doit être annoncé comme
réussi que si son fichier final est effectivement écrit, et une tentative
échouée ne doit jamais détruire la version précédente.

## Écritures atomiques et erreurs visibles

Les exports raster, SVG, DXF, nomenclature et numéros de conducteurs écrivent
désormais par fichier temporaire puis remplacement atomique. Le repli en
écriture directe de `QSaveFile` est désactivé : si le système ne peut pas
remplacer la destination en sécurité, l'ancien fichier reste intact et
l'utilisateur reçoit l'erreur réelle.

Le DXF est d'abord produit dans le dossier cible, validé, puis copié dans un
`QSaveFile`. `Createdxf` mémorise maintenant la première erreur d'ouverture,
d'ajout ou de vidage du flux. Toutes ses primitives cessent alors d'écrire,
l'enveloppe finale n'est pas ajoutée et l'export refuse le commit. L'ancien
appel `exit(0)` a été supprimé : une destination DXF inaccessible ne peut plus
fermer QElectroTech.

Lors d'un export de plusieurs folios, les erreurs sont regroupées avec le nom
de chaque fichier. Le dialogue reste ouvert afin de corriger la destination et
de relancer. Les fichiers déjà réussis restent valides ; le lot garantit
l'atomicité de chaque fichier, pas encore une transaction globale multi-fichiers.
Un export vide est refusé et, sous Windows, deux chemins qui ne diffèrent que
par la casse ou par leur normalisation sont reconnus comme une même destination.

## Fidélité des données

Les CSV de nomenclature et de numéros de conducteurs sont écrits en UTF-8 sans
BOM, indépendamment de la locale Windows. Leurs valeurs contenant un
point-virgule, un guillemet ou un retour à la ligne sont échappées selon les
règles CSV. La case **Inclure les en-têtes** est désormais réellement respectée
et les erreurs SQL d'exécution ou de parcours interrompent la génération.

## Restauration exacte de l'interface

Des gardes RAII restaurent les propriétés de rendu après chaque sortie, y
compris en cas d'erreur :

- couleur de fond globale après SVG ;
- propriétés d'export temporaires des folios ;
- sélection, élément focalisé, drapeaux de focus et interactivité de chaque vue
  après aperçu, impression ou PDF.

L'aperçu et la copie vers le presse-papiers affichent également une erreur au
lieu de continuer avec une image vide ou un SVG incomplet.

## Compatibilité

Aucun format `.qet`, `.elmt`, XML ou SQLite n'est modifié. Les extensions,
actions, menus et raccourcis existants restent identiques. Les CSV conservent
le séparateur historique `;` ; seules leur fidélité Unicode et leur conformité
d'échappement sont renforcées.

## Validation Windows 11

Validation locale avec Qt 5.15.19, MSYS2 UCRT64, Ninja et un build Debug propre :

- compilation et édition de liens complètes de `qelectrotech.exe` ;
- smoke test `qelectrotech.exe --version` : `0.100.1-dev` ;
- 12 tests CTest réussis sur 12 avec `QT_QPA_PLATFORM=offscreen` ;
- tests UI-01 et UX-01 réussis avec `QT_SCALE_FACTOR=1.5` ;
- tests byte-for-byte de l'UTF-8 sans BOM ;
- remplacement atomique d'un fichier existant et préservation sur ouverture
  impossible ou commit refusé par un verrou Windows ;
- document DXF complet, erreur d'ouverture et erreur intermédiaire après
  création du document couvertes sans arrêt du processus.

Le test `export_integrity_test` est ajouté à la CI Windows.

## Limites et suites

- DATA-03 résout désormais le résultat global et le rollback de
  `projectDataBase::updateDB()`. Une erreur de rafraîchissement interrompt les
  exports BOM GUI/CLI avant toute écriture et conserve l'ancienne base intacte.
- Les pannes internes de `QImage`/`QPainter`/SVG après ouverture et le commit
  refusé dans ces parcours graphiques nécessitent encore des points d'injection
  dédiés. Le commit commun CSV est déjà couvert par un verrou Windows réel.
- `Createdxf` reste un générateur historique à état global et séquentiel. UI-02
  devra converger vers une session d'export réentrante commune à l'interface et
  à la CLI avant tout export parallèle.
- La transaction globale, l'annulation et le résumé prévisionnel de plusieurs
  livrables relèvent de UI-02, pas de ce correctif d'intégrité par fichier.
