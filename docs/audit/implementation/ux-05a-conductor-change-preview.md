# UX-05A — Aperçu sûr des modifications groupées de conducteurs

## Résultat utilisateur

L’action **Remplacer tout** ne modifie plus immédiatement les propriétés des
conducteurs. Elle construit d’abord un aperçu en lecture seule qui indique :

- le nombre de conducteurs demandés et réellement affectés ;
- l’extension de portée aux membres des mêmes potentiels ;
- les folios et potentiels concernés ;
- chaque champ modifié avec sa valeur avant et après ;
- les conducteurs analysés qui resteraient inchangés.

Un double-clic ou Entrée sur une ligne ouvre le folio correspondant et
sélectionne le conducteur sans fermer l’aperçu. Annuler ou presser Échap ne
modifie rien. Le remplacement simple et le remplacement avancé des conducteurs
sont consolidés dans le même aperçu lorsqu’ils sont configurés ensemble.

## Contrat d’intégrité

`ConductorChangePlan` est un instantané éphémère et immuable :

1. chaque potentiel est parcouru une seule fois et ses membres sont dédupliqués ;
2. chaque segment calcule son propre état final à partir de ses propriétés
   initiales, afin de préserver ses champs non ciblés ;
3. les états avant/après affichés sont ceux remis à la commande Undo ;
4. les formules sont résolues sans mutation avec les propriétés candidates,
   notamment pour `%wf`, `%wv`, `%wc` et `%ws` ;
5. avant application, le projet, son UUID, son thread, son mode lecture seule,
   le folio exact, les propriétés, le contexte des formules et la composition
   de chaque potentiel sont revalidés ;
6. toute cible supprimée, déplacée ou modifiée refuse l’ensemble de la
   transaction ;
7. l’application pousse un unique `ChangeConductorsPropertiesCommand` sur la
   pile Undo du projet.

Le plan est volontairement limité à la session courante : `Conductor` ne
possède pas encore d’UUID persistant permettant de réassocier une cible après
fermeture ou relecture du projet.

## Compatibilité

- aucun changement des formats `.qet`, `.elmt`, XML ou SQLite ;
- aucune nouvelle donnée persistante ;
- les sentinelles et règles de `SearchAndReplaceWorker::applyChange()` restent
  utilisées par la transformation pure partagée ;
- le texte dérivé d’une formule est affiché tel qu’il sera appliqué, puis son
  contexte est revalidé avant la confirmation ;
- le remplacement unitaire réutilise également le plan atomique, consolide le
  réglage simple et le remplacement avancé, sans ouvrir le dialogue de
  confirmation.

## Accessibilité et ergonomie

- tableau en lecture seule avec six colonnes nommées ;
- noms et descriptions accessibles pour le dialogue, le résumé, la portée,
  le tableau et les boutons ;
- navigation Tab hors du tableau, Échap pour annuler et Entrée dans le tableau
  réservée à la navigation vers la cible ;
- contenu défilable, pied d’action toujours visible et taille minimale sous
  1280 × 720 logiques à 150 % ;
- 1 000 lignes prises en charge sans agrandir le dialogue au-delà de l’écran.

## Validation automatisée

La cible `conductor_change_preview_test` vérifie :

- la description exacte des champs avant/après et les no-op ;
- la résolution du texte dérivé d’une formule ;
- la combinaison propriété simple + remplacement avancé et la préservation
  des champs non ciblés ;
- la déduplication de potentiels qui se recouvrent ;
- le refus atomique d’un instantané périmé, l’échec d’application et le cas
  sans changement ;
- une seule transaction dans une vraie `QUndoStack`, suivie de undo/redo ;
- les compteurs, états, noms accessibles et le mode lecture seule du tableau ;
- Annuler, Échap et l’activation explicite du bouton Appliquer ;
- le contrat clavier qui empêche Entrée dans le tableau d’appliquer ;
- les textes longs, le DPI 150 % et une volumétrie déterministe de 1 000 lignes.

La CI exécute le test hors écran et sous le backend Windows natif, à 100 % et
150 %. La compilation de l’application complète vérifie également la liaison
de l’adaptateur `Conductor`/`QETProject` et de la commande Undo réelle. Les
tests purs couvrent le garde-fou transactionnel partagé ; ils ne remplacent
pas un scénario fonctionnel avec projet métier représentatif.

## Validation manuelle Windows 11

Le 14 juillet 2026, la version issue du commit `db050e643` a été lancée sous
Windows 11 et testée avec `examples/ArduinoLCD.qet`, projet public de trois
folios :

- le remplacement groupé d’une fonction de conducteur s’applique aux cibles
  sélectionnées et crée une seule entrée dans l’historique Undo ;
- Undo retire exactement la nouvelle valeur, ce qui a été contrôlé après
  rechargement de l’index de recherche ;
- Redo rétablit exactement cette valeur et les mêmes correspondances ;
- le projet reste ouvrable et aucun format persistant n’est modifié tant qu’il
  n’est pas enregistré.

Le contrôleur d’interface Windows utilisé pour ce smoke test invoque et valide
immédiatement certains dialogues modaux Qt. Le chemin Annuler n’a donc pas pu
être observé manuellement avec cet outil ; il reste couvert par les tests Qt
qui vérifient Annuler, Échap et l’absence de mutation avant confirmation.

## Suite UX-05

Ce lot est la fondation transactionnelle de l’édition tabulaire future. Les
cellules éditables, le collage TSV/Excel, les colonnes configurables, les
éléments et les sélections hétérogènes restent dans UX-05B et suivants.
