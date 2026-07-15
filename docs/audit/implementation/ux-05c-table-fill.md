# UX-05C — Recopie vers le bas dans l'édition des conducteurs

## Résultat utilisateur

L'éditeur tabulaire introduit par UX-05B propose maintenant l'action
**Recopier vers le bas**. L'utilisateur sélectionne une plage rectangulaire
d'au moins deux lignes, puis utilise le bouton, le menu contextuel ou
`Ctrl+D`. Pour chaque colonne sélectionnée, la première ligne devient la
référence et sa valeur est recopiée sur les lignes suivantes.

Cette première commande de remplissage rapproche le tableau des usages d'un
tableur sans ajouter de second mode d'édition. Elle fonctionne simultanément
sur la fonction, la tension ou le protocole, la couleur et la section. La
sélection, la cellule courante et le focus sont conservés après l'opération.

## Sémantique et sécurité du brouillon

- une valeur commune, y compris vide, peut être recopiée ;
- une cellule source vidée volontairement efface explicitement les cibles ;
- une source affichant « Valeurs multiples » et laissée intacte est refusée
  avant toute mutation ;
- cette même source devient utilisable dès que l'utilisateur lui attribue une
  valeur explicite ;
- une sélection discontinue, sur une seule ligne, hors limites ou comprenant
  une colonne en lecture seule est refusée en totalité ;
- toutes les valeurs sources sont validées avant la première écriture ;
- les cellules cibles seulement sont marquées comme modifiées ;
- le modèle émet une seule notification couvrant toute la zone cible.

La commande ne modifie jamais le projet directement. Elle agit seulement sur
le brouillon du dialogue UX-05B. **Vérifier** ouvre toujours l'aperçu UX-05A et
le projet ne change qu'après la confirmation finale. Annuler le dialogue après
une recopie abandonne donc l'intégralité du remplissage.

## Ergonomie et accessibilité

Le bouton est placé juste au-dessus du tableau et reste visible dans une
fenêtre de 1280 × 680 pixels logiques. L'action unique est partagée par le
bouton, le raccourci et le menu contextuel afin d'éviter des comportements
divergents. Son état, son infobulle et sa description accessible expliquent
pourquoi une sélection géométriquement invalide ne peut pas être utilisée.

Lorsqu'une source est ambiguë ou invalide, le raccourci reste déclenchable et
affiche une erreur textuelle précise plutôt que de ne rien faire. Si une cellule
est encore en cours d'édition, `Ctrl+D` valide d'abord sa nouvelle valeur avant
de remplir la plage. Les messages d'état sont aussi annoncés aux technologies
d'assistance. L'ordre de tabulation suit l'ordre visuel : action, tableau,
réinitialisation, vérification et annulation.

L'arbre d'accessibilité de la compilation de débogage n'a pas été exposé par
l'outil d'inspection Windows utilisé pendant le contrôle manuel. Les noms,
descriptions, raccourcis, états et événements accessibles sont donc couverts
par les tests Qt, mais ce contrôle ne constitue pas une certification avec un
lecteur d'écran réel.

## Compatibilité

- aucun changement des formats `.qet`, `.elmt`, XML, SQLite ou des exports ;
- aucune nouvelle préférence ou donnée persistante ;
- aucune modification de l'API publique historique ;
- réutilisation du modèle de brouillon, de l'aperçu et de la transaction Undo
  unique des incréments UX-05A et UX-05B ;
- comportement additif : le collage TSV, l'édition cellule par cellule et le
  parcours historique restent disponibles.

## Validation automatisée

Les tests Qt couvrent notamment :

- les plages mono- et multi-colonnes et l'unique notification du modèle ;
- la recopie d'un effacement explicite ;
- le rejet atomique des sources mixtes intactes, des valeurs invalides et des
  sélections hors contrat ;
- la source mixte rendue explicite par l'utilisateur ;
- le bouton, le menu contextuel, `Ctrl+D` et les propriétés accessibles ;
- la validation de l'éditeur actif avant le remplissage ;
- la conservation du focus et de la sélection ;
- Réinitialiser après une recopie ;
- l'absence de mutation du projet avant la confirmation finale ;
- 1 000 lignes et quatre colonnes avec une seule notification ;
- la fenêtre à 100 % et 150 % de mise à l'échelle.

La matrice ciblée a réussi sous Qt 5.15 en mode hors écran, sous Windows natif
et sous Windows natif avec `QT_SCALE_FACTOR=1.5`. L'application complète
`0.100.1-dev` a ensuite été compilée et liée avec l'incrément.

## Validation manuelle Windows 11

Le scénario a été rejoué dans la compilation cumulative avec une copie
temporaire de `examples/ArduinoLCD.qet` :

1. ouverture du projet sans modifier l'installation stable ;
2. accès à la recherche avancée puis à l'éditeur tabulaire ;
3. saisie de `UX05C` dans la fonction de la première ligne ;
4. sélection des trois premières lignes de la colonne ;
5. déclenchement de `Ctrl+D` ;
6. présence de `UX05C` sur les trois lignes et message
   « Recopie effectuée sur 2 cellule(s) » ;
7. fermeture par Annuler, sans appliquer ni enregistrer le brouillon.

Les captures correspondantes sont conservées dans
`docs/audit/evidence/ux-05c-table-fill/`.

## Suite UX-05

UX-05D pourra ajouter les colonnes configurables, l'import/export de feuilles
de calcul et des commandes de remplissage supplémentaires. L'extension du
modèle tabulaire aux éléments, borniers et câbles restera un incrément séparé
afin de conserver des changements relisibles et compatibles avec l'amont.
