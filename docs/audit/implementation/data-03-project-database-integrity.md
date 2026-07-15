# DATA-03 — Intégrité de la base de données projet

## Résultat utilisateur

Les sommaires, nomenclatures et exports BOM ne peuvent plus continuer sur une
base SQLite partiellement reconstruite. Si une suppression, une insertion ou
le commit échoue, QElectroTech restaure exactement l'état précédent, n'annonce
aucune actualisation et affiche un diagnostic au lieu de produire un livrable
potentiellement incomplet.

Ce lot sécurise une base **dérivée** du projet : le document `.qet` demeure la
source de vérité et n'est jamais modifié par la transaction DATA-03.

## Défaut corrigé

La protection introduite précédemment ne couvrait que `diagram`. Après son
commit, `diagram_info`, `element` et `element_info` étaient chacune vidées puis
remplies sans transaction globale. Leurs erreurs `DELETE` ou `INSERT` étaient
seulement journalisées, puis `dataBaseUpdated()` était émis malgré tout.

Les exports GUI et CLI appelaient ensuite leur requête sans pouvoir distinguer
un rafraîchissement réussi d'un échec. Une ancienne base cohérente ou une base
partielle pouvait donc être exportée comme si elle correspondait au projet
courant.

## Transaction et résultat typé

`ProjectDatabaseWriter` reçoit un instantané indépendant du modèle graphique et
remplace les quatre tables dans une seule transaction :

1. ouverture de la transaction ;
2. suppression enfant→parent : `element_info`, `element`, `diagram_info`,
   `diagram` ;
3. insertion parent→enfant : `diagram`, `diagram_info`, `element`,
   `element_info` ;
4. commit unique ;
5. rollback immédiat au premier échec de requête ou de commit.

`ProjectDatabaseUpdateResult` distingue la disponibilité de la base, le début
de transaction, la requête fautive et le commit. Il conserve la table,
l'opération, l'UUID de l'objet si disponible, l'erreur `QSqlError`, les comptes
de folios/éléments et le résultat du rollback. Le diagnostic reste accessible
même si le rollback lui-même échoue.

`projectDataBase::updateDB()` construit l'instantané, appelle le writer et
émet `dataBaseUpdated()` exactement une fois après un commit réussi. Les appels
historiques qui ignorent le retour restent compatibles au niveau source.

## Propagation dans l'application

- le dialogue de nomenclature construit d'abord les données et refuse même
  d'ouvrir le sélecteur de destination si la base n'est pas actualisée ;
- l'export BOM en ligne de commande écrit le diagnostic sur `stderr`, retourne
  un code non nul et ne touche pas au fichier cible ;
- l'export SQLite de développement s'arrête avant de demander une destination ;
- le modèle de sommaire/nomenclature conserve sa requête et ses enregistrements
  précédents si une nouvelle actualisation échoue ;
- le bouton de rafraîchissement affiche que les données précédentes sont
  conservées ;
- une duplication de folios réussie n'est pas annulée à cause de cette base
  dérivée, mais l'utilisateur est averti que sommaire et nomenclature n'ont pas
  été actualisés ;
- le chargement d'un projet reste non destructif et journalise le diagnostic
  pour permettre une nouvelle tentative.

## Validation automatisée

La cible Qt 5 `project_database_atomicity_test` utilise une base SQLite en
mémoire avec les quatre tables et leurs clés étrangères. Elle couvre :

- remplacement complet réussi ;
- huit erreurs SQL forcées par triggers : `DELETE` et `INSERT` sur chacune des
  quatre tables ;
- comparaison exacte des quatre tables avant/après chaque échec ;
- transaction entièrement fermée après rollback ;
- suppression du trigger puis réussite de la tentative suivante ;
- échec réel du commit par contrainte étrangère différée, suivi d'un rollback ;
- base fermée retournant une erreur typée sans tentative de transaction.

Résultat local Windows 11 : 1 cible CTest réussie, 0 échec, en 0,10 seconde.

## Validation de compilation Windows 11

- Qt 5.15/MSYS2 UCRT64, C++17 et Ninja ;
- configuration CMake Debug régénérée à partir de la branche DATA-03 ;
- 272 étapes de compilation/liaison réussies ;
- `qelectrotech.exe` produit et lié avec le writer, les exports, le modèle et
  les dialogues modifiés ;
- deux exports BOM successifs de `examples/industrial.qet` terminés avec le
  code `0`, 355 lignes et le même SHA-256
  `C298FC9BB008E85BEC53A3515211037ED043089A4E5BBF4214D21D8BA323CFEF` ;
- seuls des avertissements de dépréciation historiques hors DATA-03 subsistent.

## Compatibilité et limites

Aucun format `.qet`, `.elmt`, XML, CSV ni schéma SQLite n'est modifié. Le writer
utilise seulement C++17 et QtSql disponibles sous Qt 5 et Qt 6. DATA-03 ne
change pas encore les mutations SQLite incrémentales (`addElement`,
`removeElement`, etc.) : il garantit la reconstruction complète, qui constitue
le point de contrôle utilisé avant les rapports et exports.

Une panne matérielle de la base SQLite en mémoire est difficile à reproduire
manuellement ; les triggers et la contrainte différée fournissent donc la
preuve déterministe principale. L'interface réelle sera rejouée dans une future
préversion contenant un point d'injection de diagnostic réservé aux builds de
test si une capture visuelle de ces erreurs rares devient nécessaire.
