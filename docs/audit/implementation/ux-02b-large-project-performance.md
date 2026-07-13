# UX-02b — Performance des grands projets

## Périmètre

Cette phase optimise les parcours de folios introduits par UX-02 sans modifier
les formats `.qet`, le XML, les exports ni les données utilisateur. Elle reste
empilée sur `codex/ux-02-folio-navigation` afin que la revue UX-02 conserve un
périmètre lisible.

Les optimisations ciblent les coûts structurels qui devenaient quadratiques
avec le nombre de folios : recherche de position dans `QETProject`, association
onglet/vue dans `ProjectView`, titres d'onglets, groupes du navigateur et lots
SQLite lors d'un réordonnancement.

## Changements réalisés

- `OrderedIndexCache` transforme les recherches répétées de position en une
  reconstruction O(n) suivie de consultations O(1). Le cache vérifie toujours
  que la position mémorisée correspond au conteneur courant ; ajout,
  suppression, remplacement de même taille et déplacement restent sûrs sans
  dépendre d'une invalidation manuelle.
- `ProjectView::loadDiagrams` utilise directement la position de boucle.
- `rebuildDiagramsMap` parcourt les onglets une seule fois et reconstruit
  atomiquement les index position, vue, diagramme et UUID.
- l'activation par UUID, la résolution diagramme/vue et l'historique utilisent
  ces index ; Page précédente/suivante ne reconstruit plus la table.
- les titres sont mis à jour avec l'index déjà connu. Le réglage d'affichage du
  folio n'est lu qu'une fois par lot.
- les rafraîchissements du navigateur sont regroupés dans la boucle
  d'événements. Une activation ferme le dialogue avant de changer de folio,
  ce qui évite une reconstruction complète juste avant sa fermeture.
- les chaînes normalisées de recherche sont calculées une fois lors du reset
  du modèle. Une requête vide dans la portée complète ne trie plus une liste
  déjà ordonnée.
- les groupes sont dédupliqués par `QSet`, puis triés, au lieu d'une recherche
  linéaire dans une `QStringList` pour chaque folio.
- les mises à jour des positions SQLite parcourent un index de boucle et sont
  exécutées dans une transaction.

## Budgets déterministes

Les tests n'utilisent plus un unique seuil chronométrique permissif. Ils
comptent les opérations sur 500 et 1000 entrées :

- une requête vide visite exactement n entrées, sans évaluation de score ni
  comparaison de tri ;
- une requête absente effectue exactement n évaluations et n contrôles de
  jeton ;
- une requête large à trois jetons effectue 3n contrôles ;
- le nombre de comparaisons du tri est borné par une enveloppe n log²(n) ;
- 1000 groupes distincts restent correctement dédupliqués et triés ;
- 1000 recherches successives de position ne provoquent qu'une seule
  reconstruction du cache ;
- déplacement et remplacement de même taille forcent une auto-validation et
  conservent les mêmes résultats que `QList::indexOf`.

Le job Windows compile maintenant aussi `qelectrotech.exe`, en plus des cibles
de test. La validation locale du 13 juillet 2026 donne 6 tests sur 6 en 0,72 s.

## Validation Windows 11

Deux projets synthétiques temporaires ont été générés à partir d'un folio vide,
avec respectivement 150 et 500 folios, 10 installations et 25 localisations.
Ils ne sont pas versionnés et ne contiennent aucune donnée utilisateur.

| Parcours observé | Résultat |
|---|---:|
| Ouverture 150 folios jusqu'à l'arbre accessible | 2,138 s |
| Ouverture 500 folios jusqu'à l'arbre accessible | 7,199 s |
| Ouverture Ctrl+G sur 500, observation accessible comprise | 709 ms |
| Filtre exact `500/500`, observation accessible comprise | 604 ms |
| Activation du folio 500, observation accessible comprise | 416 ms |

Le ratio d'ouverture 500/150 est 3,37 pour un ratio de volume de 3,33. Sur ce
jeu vide et cette machine, la croissance observée est donc proche du nombre de
folios. Les durées du navigateur sont des bornes hautes : la capture complète
de l'arbre d'accessibilité de 500 lignes est incluse et n'isole pas le seul
temps CPU de QElectroTech.

Les scénarios suivants ont également été validés dans l'application réelle :

1. recherche et activation du dernier folio sur 150 puis 500 entrées ;
2. retour au premier folio avec `Alt+Gauche` ;
3. fermeture du dialogue avant activation, sans reconstruction visible ;
4. déplacement du premier folio à la deuxième position sur 500 entrées ;
5. recherche `1/500` immédiatement cohérente avec le nouvel ordre ;
6. enregistrement et fermeture sans erreur.

La validation a détecté un cas où le premier folio, déjà sélectionné par Qt
pendant la création des onglets, n'était pas inscrit dans l'historique. Le
chargement l'enregistre désormais explicitement après la reconstruction des
index.

## Limites et suite

Cette phase élimine les parcours quadratiques identifiés dans les index,
onglets, groupes et positions SQLite. Elle ne rend pas le chargement paresseux :
QElectroTech parse encore tous les diagrammes, reconstruit la base interne et
crée un `DiagramView` pour chaque folio avant la première interaction.

La prochaine étape de performance devra mesurer séparément parsing XML,
`refresh`, population SQLite, création des vues, export et mémoire sur des
projets industriels anonymisés. Un prototype de `DiagramView` différé doit
rester isolé, car les renvois, modes d'édition, impressions et exports supposent
actuellement que toutes les vues existent.
