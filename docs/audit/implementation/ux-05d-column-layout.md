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

Le point d’entrée tolère désormais un index de recherche vide ou périmé. Quand
aucun conducteur vivant n’est disponible dans l’arbre, le dialogue retrouve le
projet ouvert et reconstruit sa portée depuis les scènes puis, pour les anciens
projets, depuis les bornes des éléments. Une sélection explicitement décochée
reste respectée. Cette récupération réutilise les objets `Conductor` existants
et ne crée aucune représentation parallèle des données.

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

## Validation manuelle Windows 11

Validation rejouée le 14 juillet 2026 avec la préversion portable
`QElectroTech-UX05D-Preview-5b32d9ff3`, puis avec le binaire Debug intégrant la
correction du point d’entrée, dans une fenêtre logique de
1269 × 840 pixels correspondant au poste Windows 11 de validation. Aucun projet
n’a été enregistré ou modifié pendant ce contrôle.

| Étape | Contrôle | État | Preuve |
|---|---|---|---|
| 1 | Ouverture de `examples/industrial.qet`, chargement du folio câblé « Mains Power Supply », recherche avancée puis rafraîchissement, avant correction | **Dégradé reproduit** : la catégorie Conducteurs reste vide et bloque initialement l’entrée vers le tableau | [`00-entry-limitation-industrial.png`](../evidence/ux-05d-column-layout/00-entry-limitation-industrial.png) |
| 2 | Affichage du dialogue avec 36 potentiels réalistes en utilisant directement le composant compilé du lot | **Sain** : les sept colonnes, le défilement, les textes d’aide et les actions sont lisibles | [`01-default-columns.png`](../evidence/ux-05d-column-layout/01-default-columns.png) |
| 3 | Ouverture de **Colonnes…** | **Sain** : les sept champs sont nommés, le potentiel est obligatoire et la restauration est visible | [`02-columns-menu.png`](../evidence/ux-05d-column-layout/02-columns-menu.png) |
| 4 | Masquage de quatre colonnes puis déplacement de **Couleur** avant **Fonction** | **Sain** : la grille compacte conserve le contexte du potentiel et accepte le glisser-déposer | [`03-compact-reordered.png`](../evidence/ux-05d-column-layout/03-compact-reordered.png) |
| 5 | Modification d’une fonction puis masquage de sa colonne | **Sain** : le résumé annonce explicitement une modification dans une colonne masquée | [`04-hidden-change-announcement.png`](../evidence/ux-05d-column-layout/04-hidden-change-announcement.png) |
| 6 | Fermeture par **Annuler**, puis réouverture | **Sain** : l’ordre et la visibilité sont restaurés sans réappliquer le brouillon annulé | [`05-persisted-layout.png`](../evidence/ux-05d-column-layout/05-persisted-layout.png) |
| 7 | **Rétablir la disposition par défaut** | **Sain** : les sept colonnes et leur ordre initial sont restaurés | [`06-reset-default.png`](../evidence/ux-05d-column-layout/06-reset-default.png) |
| 8 | Rejeu dans l’application complète après récupération de l’index vide : `industrial.qet` → folio 4 → recherche avancée → tableau | **Sain** : le dialogue s’ouvre sur les potentiels réels du projet (`423`, `411N`, `308`, etc.) | [`07-end-to-end-recovered.png`](../evidence/ux-05d-column-layout/07-end-to-end-recovered.png) |

Le banc visuel des étapes 2 à 7 instancie exactement
`ConductorBulkEditDialog` et son modèle depuis les sources de la préversion,
avec des données de validation détachées. L’étape 8 complète cette couverture
par une validation de bout en bout dans l’application complète. La capture 00
est conservée comme preuve du défaut reproduit avant correction.

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
