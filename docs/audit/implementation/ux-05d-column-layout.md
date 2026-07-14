# UX-05D — Disposition configurable des colonnes conducteurs

## Résultat utilisateur

L’éditeur « Modifier les conducteurs en tableau » permet maintenant d’adapter
sa grille au travail en cours. Le bouton **Colonnes…** affiche les sept champs
existants ; l’utilisateur peut masquer les colonnes secondaires et réordonner
les en-têtes par glisser-déposer. La disposition est restaurée à la prochaine
ouverture du dialogue et une commande remet immédiatement la vue par défaut.

Cette vue compacte réduit le défilement horizontal sur un écran 1920 × 1080,
notamment lorsqu’une équipe ne renseigne qu’une partie des propriétés. La
colonne **Potentiel / conducteur** reste toujours visible pour conserver le
contexte de chaque ligne et au moins une colonne éditable est obligatoire.

## Contrat de sécurité

Le masquage et l’ordre des colonnes ne changent ni le projet, ni le brouillon,
ni les valeurs mixtes ou volontairement vides. Les préférences sont enregistrées
dans `QSettings` sous des clés techniques stables et versionnées :

- `conductor-bulk-editor/column-layout/version` ;
- `conductor-bulk-editor/column-layout/order` ;
- `conductor-bulk-editor/column-layout/visible`.

Une préférence absente, incomplète, dupliquée ou inconnue revient à la vue
complète. Les indices binaires opaques de `QHeaderView::saveState()` ne sont pas
utilisés, afin que l’ajout futur d’une colonne et la transition Qt 6 puissent
être migrés explicitement.

Le collage TSV et **Recopier vers le bas** n’utilisent plus un intervalle
logique implicite. Le dialogue leur transmet la liste exacte des colonnes
éditables visibles dans leur ordre à l’écran. Une colonne masquée ne peut donc
pas être modifiée silencieusement lorsque deux colonnes visibles voisines ne le
sont plus dans le modèle. Les anciens appels par intervalle restent disponibles
comme adaptateurs internes pour limiter la divergence avec l’amont.

Comme pour UX-05A à UX-05C, toutes les modifications restent dans le brouillon
jusqu’à **Vérifier**, puis passent par l’aperçu et une transaction Undo unique.
Aucun format `.qet`, `.elmt`, XML, SQLite ou export n’est modifié.

## Ergonomie et accessibilité

- bouton de 32 pixels minimum dans le bandeau d’actions existant ;
- menu entièrement accessible au clavier, libellés issus des en-têtes traduits ;
- action obligatoire « Potentiel / conducteur » cochée et désactivée avec une
  explication ;
- refus textuel si l’utilisateur tente de masquer le dernier champ éditable ;
- ordre de tabulation : Colonnes, Recopier, tableau, Réinitialiser, Vérifier,
  Annuler ;
- description accessible du tableau mise à jour ;
- le résumé annonce les modifications situées dans des colonnes masquées ;
- la disposition reste locale au poste et n’est pas incorporée au projet.

## Validation automatisée

Les tests Qt couvrent :

- les clés techniques et la disposition par défaut ;
- la persistance de l’ordre et de la visibilité entre deux dialogues ;
- le repli complet après une préférence corrompue ;
- l’impossibilité de masquer le potentiel et le dernier champ éditable ;
- le collage suivant l’ordre visuel après déplacement d’une section ;
- la recopie excluant une colonne masquée au milieu de la sélection ;
- la conservation des champs non transmis au modèle ;
- l’annonce d’une modification masquée et les noms accessibles ;
- 1 000 potentiels, le clavier et l’éditeur actif hérités des lots précédents ;
- une fenêtre utilisable dans 1280 × 680 pixels logiques.

La matrice ciblée réussit sous Qt 5.15 en mode hors écran et sous Windows natif,
à 100 % et avec `QT_SCALE_FACTOR=1.5`. L’application complète est également
compilée et liée avec l’incrément.

## Limites et suite

UX-05D ne crée pas encore de nouvelles propriétés métier. L’export CSV de revue
peut être ajouté sans identifier durablement les lignes, mais l’import doit
attendre un identifiant de potentiel stable entre deux sessions : les clés
actuelles reposent sur des pointeurs `quintptr` et ne peuvent pas servir à
réassocier sûrement un fichier externe.

Le prochain sous-lot proposé est donc : export CSV UTF-8 atomique et protégé
pour Excel, puis définition d’identités persistantes et parseur robuste avant
tout import. L’extension aux éléments, borniers, câbles ou E/S automate restera
séparée.
