# UX-05B — Édition tabulaire des conducteurs

## Résultat utilisateur

La recherche avancée propose maintenant **Modifier les conducteurs en
tableau…**. Le dialogue présente une ligne par potentiel électrique complet,
y compris lorsqu’il contient plusieurs segments ou traverse plusieurs folios.
Les quatre propriétés suivantes sont éditables directement :

- fonction ;
- tension ou protocole ;
- couleur du conducteur ;
- section du conducteur.

Les colonnes folio, potentiel/conducteur et nombre de segments restent en
lecture seule. Plusieurs cellules peuvent être remplies depuis un tableur avec
Ctrl+V au format TSV. Une zone qui dépasse les quatre colonnes éditables ou le
nombre de lignes disponible est refusée en totalité.

## Contrat de brouillon

Le tableau est un brouillon sans effet sur le projet :

1. une valeur commune est affichée normalement ;
2. des valeurs différentes dans un même potentiel sont signalées par
   « Valeurs multiples » ;
3. une cellule mixte laissée intacte conserve la valeur propre à chaque
   segment ;
4. modifier cette cellule harmonise uniquement la propriété concernée sur le
   potentiel ;
5. vider volontairement une cellule efface explicitement la propriété ;
6. Réinitialiser restaure tout le brouillon ;
7. Annuler ou Échap ferme le dialogue sans mutation ;
8. Vérifier ne devient disponible qu’en présence d’au moins une modification
   valide.

Les caractères de contrôle et les textes de plus de 255 caractères sont
signalés dans la cellule et bloquent la vérification. Une ancienne valeur qui
ne respecterait plus ces règles reste toutefois compatible tant qu’elle n’est
pas modifiée : ouvrir puis annuler le tableau ne bloque et ne réécrit aucune
donnée historique.

## Application atomique

Le premier dialogue produit seulement les transformations ciblées. Le plan
`ConductorChangePlan` calcule ensuite l’état exact de chaque segment et ouvre
l’aperçu en lecture seule introduit par UX-05A. Le projet n’est modifié qu’après
la seconde confirmation **Appliquer les modifications**.

L’application conserve les garanties UX-05A : expansion et déduplication des
potentiels, contrôle du projet et de son thread, refus des projets en lecture
seule, revalidation des conducteurs, des propriétés, des formules et de la
composition des potentiels. Toutes les cellules du tableau sont regroupées
dans un unique `ChangeConductorsPropertiesCommand`, donc dans une seule entrée
Undo/Redo.

L’API existante de `ConductorChangePlan` reste compatible. Une surcharge
`TargetTransform` ajoute la cible `Conductor*` à la transformation afin que le
brouillon puisse appliquer une valeur différente à chaque potentiel ; la
transformation historique, commune à toutes les cibles, est conservée.

## Compatibilité

- aucun changement des formats `.qet`, `.elmt`, XML, SQLite ou des exports ;
- aucune nouvelle donnée persistante ;
- aucune écriture avant la seconde confirmation ;
- les champs non édités restent propres à chaque segment ;
- le parcours historique de remplacement reste disponible ;
- ajout limité au modèle de brouillon, à son adaptateur projet et au dialogue.

## Accessibilité et dimensions

- noms et descriptions accessibles sur le dialogue, le tableau, l’état et les
  boutons ;
- navigation clavier entre les cellules et les actions ;
- Ctrl+V pris en charge lorsque le tableau ou son éditeur possède le focus ;
- cellules invalides signalées visuellement et textuellement ;
- dialogue redimensionnable, contenu défilable et taille minimale 720 × 430 ;
- emprise vérifiée sous 1280 × 720 logiques avec un facteur de texte de 150 % ;
- 1 000 potentiels traités avec une hauteur d’écran bornée.

## Validation automatisée

La cible `conductor_bulk_edit_test` et ses variantes clavier/DPI vérifient :

- la conservation des valeurs mixtes jusqu’à leur modification ;
- le collage TSV sur les quatre propriétés ;
- le refus atomique d’un collage hors limites ;
- la compatibilité des anciennes valeurs invalides laissées intactes ;
- le blocage des cellules invalides et l’effacement explicite ;
- Réinitialiser, Annuler, Échap et Ctrl+V ;
- l’emprise à 150 % et 1 000 potentiels déterministes.

Le 14 juillet 2026, les trois variantes UX-05B ont réussi sous Qt 6 isolé puis
sous Qt 5. La matrice Qt 5 cumulative a réussi **20 tests sur 20**. L’application
complète Qt 5 a ensuite été compilée et liée avec les nouveaux adaptateurs.

La configuration Qt 5 locale nécessitait de réparer le chemin `qmake` du
paquet MSYS2 : `qmake5.exe` a été restauré et la configuration CMake Qt5Core
locale a été pointée vers cet exécutable. Cette correction concerne uniquement
le poste de développement et n’appartient pas au code du fork. Les avertissements
`lupdate` provenant de scripts JavaScript de sous-modules restent bruyants mais
n’empêchent ni la compilation ni les tests.

## Validation manuelle Windows 11

La compilation cumulative a été lancée à côté de la stable 0.100 avec une
configuration et une copie de projet séparées. Le scénario a utilisé la copie
temporaire de `examples/ArduinoLCD.qet` :

- l’action tabulaire est visible dans les options avancées de recherche ;
- 37 conducteurs demandés ont produit un brouillon par potentiel ;
- une fonction a été modifiée sur un seul potentiel ;
- l’aperçu a analysé 47 conducteurs, indiqué 1 modification et 46 inchangés ;
- 10 conducteurs associés aux mêmes potentiels ont été inclus dans le contrôle
  de cohérence ;
- l’état avant vide et l’état après `UX05B-APPLIQUE` ont été affichés avant
  toute mutation ;
- l’application a créé exactement une entrée
  « Chercher/remplacer les propriétés de conducteurs » ;
- Annuler a retiré la transaction complète et Refaire l’a rétablie ;
- seul le projet temporaire a été modifié et il n’a pas été enregistré.

## Suite UX-05

Les incréments suivants pourront ajouter la sélection rectangulaire, le
remplissage vers le bas, des colonnes configurables, l’import/export de
feuilles de calcul et l’extension du modèle tabulaire aux éléments, borniers et
câbles. Ces évolutions devront réutiliser le même contrat de brouillon, aperçu
et transaction unique.
