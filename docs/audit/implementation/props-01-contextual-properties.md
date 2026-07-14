# PROPS-01 — Inspecteur de propriétés contextuel

## Objectif et contrat

PROPS-01 modernise le conteneur **Propriétés de la sélection** sans remplacer
les éditeurs métier existants. Le dock doit toujours expliquer son contexte,
rester utilisable dans une largeur réduite et ne jamais conserver un éditeur
lié à un folio qui n'est plus actif.

Le lot ne change ni la fabrique d'éditeurs, ni le mode d'édition directe, ni les
commandes Undo. Il ne crée aucune nouvelle propriété et ne modifie aucun format.

## Problèmes corrigés

- un changement de folio ou de projet pouvait laisser le dock synchronisé avec
  l'ancienne vue jusqu'au prochain signal de sélection ;
- aucune sélection, une sélection de types différents ou un type non pris en
  charge produisait un panneau entièrement blanc ;
- les formulaires longs étaient placés directement dans le dock sans zone de
  défilement propre ;
- l'état et le cardinal de la sélection n'étaient pas annoncés par le
  conteneur.

## États explicites

| État | Résumé affiché | Suite proposée |
| --- | --- | --- |
| Aucun folio | Aucun folio actif | Ouvrir ou créer un folio |
| Aucune sélection | Aucune sélection | Sélectionner un objet du schéma |
| Sélection non prise en charge | Cardinal de la sélection | Sélectionner des objets du même type ou utiliser le parcours historique |
| Éditeur | Titre de l'éditeur et cardinal | Modifier avec l'éditeur existant |

Les messages sont textuels et accessibles ; la couleur seule ne porte aucune
information. Le changement d'état ne déplace pas le focus depuis le canevas.

## Synchronisation et durée de vie

`QETDiagramEditor` resynchronise le dock à l'activation d'un folio, à
l'activation d'une sous-fenêtre projet et lors d'un changement de sélection.
`DiagramPropertiesEditorDockWidget` garde la scène de sélection — le `Diagram`
en production — par `QPointer<QGraphicsScene>` et repasse à l'état « Aucun
folio actif » lorsque l'objet disparaît. Cette interface interne permet aussi
de tester les changements et destructions de folios sans dupliquer tout le
graphe de dépendances de `QETProject`.

La fabrique reste l'autorité pour déterminer si une sélection homogène possède
un éditeur. Lorsque la classe d'éditeur ne change pas, l'instance existante est
réutilisée comme auparavant. Lorsqu'elle change, l'ancien éditeur est détruit
avant d'afficher le nouveau.

## Conteneur responsive

L'en-tête de contexte reste fixe. L'éditeur est placé dans un `QScrollArea`
redimensionnable, ce qui conserve l'accès aux champs lorsque le dock est étroit
ou que la police est agrandie. La largeur minimale logique reste bornée à
260 pixels et le profil Essentiel conserve le tabifiage Collections/Propriétés.
Le profil Classique et l'identité historique
`diagram_properties_editor_dock_widget` restent inchangés.

## Compatibilité

- aucun changement `.qet`, `.elmt`, XML, SQLite, CSV, image ou PDF ;
- aucune clé `QSettings` ajoutée ou renommée ;
- mêmes classes d'éditeurs, mêmes règles live-edit et mêmes commandes Undo ;
- mêmes actions et dialogues pour les propriétés du projet et du folio ;
- widgets et API communs à Qt 5 et Qt 6.

Les nouvelles chaînes gardent le français comme langue source. Comme dans le
cycle amont, la mise à jour générée des catalogues `.ts`/`.qm` reste regroupée
dans un lot de traduction séparé afin de ne pas mélanger une fonctionnalité et
des milliers de lignes de catalogues encore non traduites.

## Hors périmètre

- réorganisation des champs de chaque éditeur spécialisé ;
- nouvelles capacités d'édition groupée ;
- sections repliables et recherche dans les propriétés ;
- ouverture automatique forcée du dock à chaque sélection ;
- modification des validations métier, de l'Undo ou de la lecture seule ;
- remplacement des dialogues Propriétés du projet et du folio.

## Validation

- [x] compilation Debug complète de `qelectrotech.exe` sous Windows 11 ;
- [x] test dédié des états, de la zone défilable et du cycle de vie des
  éditeurs ;
- [x] test de non-duplication et de délégation Apply/Reset ;
- [x] vérification que le changement de contexte ne vole pas le focus ;
- [x] géométrie étroite et police à 150 % ;
- [x] backend Windows natif à 100 % et 150 % ;
- [x] suite CTest complète ;
- [x] inspection visuelle dans le vrai binaire Windows ;
- [x] revue indépendante du diff final.
