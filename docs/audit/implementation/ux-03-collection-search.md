# UX-03 — Recherche distincte de l’exploration des collections

## Résultat livré

La recherche de composants dispose d’une vue de résultats plate, distincte de
l’arbre d’exploration. Chaque ligne expose le nom, la discipline, la provenance
et le chemin de l’élément. Le nombre de résultats et le tri restent visibles sans
déployer automatiquement l’arbre des collections.

Les interactions clavier principales sont prises en charge :

- `Bas` lance immédiatement la recherche en attente et sélectionne le premier
  résultat ;
- `Entrée` depuis le champ de recherche lance la recherche en attente puis
  demande le placement du résultat sélectionné ;
- `Échap` efface la recherche et revient à l’arbre d’exploration ;
- le changement de tri conserve la sélection lorsque le résultat existe encore ;
- le double-clic demande le placement du composant.

La demande de placement transporte l’URI du résultat. La résolution et la
validation de l’élément sont effectuées au dernier moment par la vue du folio,
afin de ne pas coupler l’interface de recherche à l’état instantané des
collections.

## Durcissement Windows et compatibilité

Le chemin de la collection commune et celui des traductions sont maintenant
résolus relativement à l’exécutable dans une compilation CMake Windows, comme
ils l’étaient déjà dans la configuration qmake. Une application déplacée dans
un répertoire d’exécution autonome retrouve ainsi `elements` et `lang` sans
dépendre du répertoire courant du processus.

Les URI de collections de projet acceptent désormais les indices à plusieurs
chiffres (`project12+embed://…` et suivants). Aucun format `.qet`, `.elmt` ou XML
n’est modifié.

Le rechargement des collections attend l’arrêt du traitement asynchrone avant
de détruire ou réinitialiser son modèle. Les demandes de rechargement répétées
sont regroupées tant qu’un nouveau modèle est encore en construction.

## Validation du 13 juillet 2026

- compilation CMake Qt 5/UCRT64 de l’exécutable complet `qelectrotech.exe` ;
- 4 suites CTest réussies sur 4 : Catch2, tests Qt historiques, remappage UUID et
  recherche de collection ;
- test déterministe sur 10 000 composants : 10 000 entrées visitées, 30 000
  vérifications de jetons pour une requête de trois termes et moins de 200 000
  comparaisons de tri ;
- contrôle Windows 11 de la collection réelle électrique, logique, hydraulique,
  pneumatique et énergie ;
- recherche `pompe debit fixe`, retour par `Échap`, placement par `Entrée` et par
  double-clic vérifiés dans un projet temporaire ;
- en l’absence de folio modifiable, le message d’état demande explicitement
  d’ouvrir un folio avant le placement.

## Limites et suite technique

La logique de recherche est couverte automatiquement, mais le widget complet et
le geste de placement ne disposent pas encore d’un test d’intégration Qt. Ils ont
été contrôlés dans l’application Windows réelle.

Le chargement historique construit encore certains `QStandardItem` dans un
traitement de fond. L’attente ajoutée sécurise leur durée de vie, mais une refonte
ultérieure devra produire uniquement des données dans les workers, puis créer et
attacher tous les items sur le thread d’interface. Cette évolution est à traiter
comme un chantier de stabilité séparé afin de limiter le risque de régression.
