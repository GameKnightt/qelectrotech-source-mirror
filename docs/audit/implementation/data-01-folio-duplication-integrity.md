# DATA-01 — Intégrité de la duplication de folios

## Résultat utilisateur

La commande **Dupliquer le folio** de l’arbre projet peut dupliquer un ou
plusieurs folios sans réutiliser les identifiants des éléments d’origine. Les
conducteurs, renvois et tables chaînées des folios copiés restent cohérents, la
base SQLite des sommaires et nomenclatures est reconstruite après l’opération,
et la duplication forme désormais une seule étape **Annuler/Rétablir**.

Le format `.qet` ne change pas. Seuls les nouveaux objets reçoivent les UUID
distincts qu’ils auraient toujours dû posséder.

## Défaut corrigé

L’issue amont
[#532](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/532)
montre que la duplication historique sérialisait puis réimportait le XML d’un
folio en conservant les UUID des éléments. Les clés primaires
`element.uuid` et `element_info.element_uuid` entraient alors en collision ;
des composants pouvaient manquer dans le sommaire ou la nomenclature.

Le correctif du fork remappe tout le groupe avant l’import :

- UUID de chaque élément et de chaque table graphique ;
- extrémités `element1` / `element2` des conducteurs ;
- renvois internes au groupe copié ;
- chaînes `previous_table` ;
- références externes laissées dirigées vers les éléments non copiés.

Les UUID sources ambigus sont rejetés avant toute mutation. Si un des folios
ne peut pas être importé, les copies déjà créées sont retirées et la base
dérivée est reconstruite.

## Annuler et rétablir

`DuplicateDiagramsCommand` conserve les documents déjà remappés, les
propriétés de bordure et de cartouche et la position de chaque nouveau folio.
Le premier `redo()` appelé par `QUndoStack::push()` ne rejoue pas une opération
déjà validée. Ensuite :

1. **Annuler** retire toutes les copies comme un seul lot et actualise les
   liens et la base projet ;
2. **Rétablir** recrée le lot depuis les mêmes instantanés XML ;
3. les UUID des copies restent donc stables pendant ce cycle ;
4. un échec partiel du rétablissement déclenche un rollback et une
   reconstruction de la base.

`ProjectView` écoute maintenant les suppressions provenant de `QETProject`.
Cette synchronisation évite les onglets orphelins lorsqu’une commande modèle
retire un folio, et reste idempotente pour le parcours de suppression manuel.

## Validation automatisée

La cible `diagram_duplicate_uuid_remapper_test` couvre les relations entre
folios, les références externes, les tables chaînées, les UUID ambigus et les
anciens éléments sans identifiant. Son scénario de stress ajoute :

- 100 duplications successives d’un groupe de deux folios liés ;
- 200 UUID d’éléments générés, tous distincts des sources et entre eux ;
- vérification des deux extrémités de conducteur à chaque itération ;
- insertion dans deux tables SQLite portant les mêmes contraintes primaires
  que `element` et `element_info` ;
- 202 éléments et 200 lignes d’information obtenus sans violation `UNIQUE`.

Sur Windows 11, l’application complète Qt 5/MSYS2 UCRT64 a été compilée avec
la commande Undo et la synchronisation de vues. Les 47 tests CTest du dépôt
passent en série, sans échec.

## Compatibilité et limites de qualification

Le lot ne modifie ni schéma XML, ni schéma SQLite, ni collection, ni export.
Il reste compatible Qt 5/Qt 6 au niveau des API utilisées. La proposition
amont #544 ne doit pas être cumulée avec ce remappage : renouveler une seconde
fois les UUID après import invaliderait les références déjà remappées.

Le stress automatisé exerce directement le contrat de remappage et les
contraintes SQLite ; il ne simule pas 100 clics dans l’interface. Le parcours
réel Annuler/Rétablir est qualifié dans la préversion Windows, tandis qu’une
qualification exhaustive sur projet métier anonymisé reste prévue dès qu’un
corpus représentatif électrique, pneumatique, hydraulique ou process est
disponible.
