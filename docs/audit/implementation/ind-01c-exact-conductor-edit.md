# IND-01C — Édition exacte des conducteurs d’un câble

## Résultat utilisateur

L’onglet **Projet > Borniers et câbles… > Câbles** permet maintenant de
sélectionner un câble, un conducteur ou plusieurs lignes, puis d’ouvrir
**Modifier les conducteurs…** (`Alt+M`).

Le brouillon affiche uniquement les conducteurs explicitement sélectionnés. Il
autorise la modification de deux propriétés adaptées à ce parcours :

- **Couleur** — le champ historique « âme / couleur » du conducteur ;
- **Câble** — la référence de câble existante.

Les colonnes fonction, tension/protocole et section restent masquées et
inaccessibles dans ce mode. Les folios et numéros de conducteur restent visibles
comme contexte en lecture seule.

Après **Vérifier**, l’aperçu récapitule le nombre exact de conducteurs et les
propriétés modifiées. La confirmation applique l’ensemble dans une commande Undo
unique ; `Ctrl+Z` restaure atomiquement toutes les valeurs précédentes.

## Portée exacte et sécurité

Le parcours réutilise le moteur de changement de propriétés, avec une nouvelle
portée `ExactConductors`. Contrairement au mode historique par potentiel
électrique, cette portée ne traverse jamais les autres segments reliés. Une
sélection parent « câble » est développée vers ses membres ; une sélection de
conducteur ne cible que cette ligne ; les doublons parent/enfant sont éliminés.

Juste avant l’aperçu puis l’application, chaque cible est résolue et validée à
nouveau. Une cible supprimée, déplacée vers un autre projet ou devenue ambiguë
interrompt le parcours sans modification partielle.

Le bouton d’édition est désactivé lorsque :

- le projet est en lecture seule ;
- le chargement du catalogue a échoué ;
- le catalogue est vide ou aucune cible valable n’est sélectionnée.

## Architecture

- `CableCatalogWidget` active la multi-sélection et produit des cibles
  dédupliquées ;
- `TerminalStripEditorWindow` orchestre résolution, brouillon, aperçu,
  application atomique et rafraîchissement ;
- `ConductorBulkEditModel` fournit le mode `ExactConductors` et la colonne
  `CableColumn` sans modifier le mode historique par potentiel ;
- `ConductorChangePlan` distingue `ElectricalPotential` et `ExactConductors` ;
- `ConductorChangePreviewDialog` annonce explicitement la portée exacte et le
  nombre de groupes sélectionnés.

Les surcharges historiques sont conservées. Aucun sérialiseur ni format de
fichier n’est modifié : les valeurs restent stockées dans les propriétés de
conducteur existantes.

## Validation

- application complète compilée sous Windows 11 / Qt 5 / UCRT64 ;
- **48/48** tests CTest réussis dans le mode isolé de la CI ;
- **13/13** contrats étiquetés `ind-01c` réussis ;
- scénario exact réussi sur la plateforme Windows native à 100 % et 150 % ;
- tests du modèle : seules Couleur et Câble sont éditables, les autres
  propriétés restent bit à bit inchangées ;
- tests du catalogue : sélection parent, enfant, multi-sélection, déduplication,
  lecture seule et indisponibilité ;
- tests de plan et d’aperçu : aucune expansion au potentiel, revalidation,
  comptage et libellés exacts ;
- parcours graphique réel sur un projet public de 50 folios : sélection de
  `C20`, ouverture par le bouton et par `Alt+M`, deux conducteurs affichés,
  annulation sans mutation.

## Preuves Windows

- [Câble C20 sélectionné dans le catalogue](../evidence/ind-01c/01-cable-catalog-c20-selected.png)
- [Éditeur exact limité aux deux conducteurs de C20](../evidence/ind-01c/02-exact-conductor-editor.png)

## Limites et suite

IND-01C corrige les champs historiques existants ; il n’introduit pas encore un
objet `Cable` persistant, des réserves, des destinations structurées ou une base
fabricant. La prochaine étape industrielle doit être fondée sur des projets
anonymisés représentatifs avant d’ajouter ces contrats de données.
