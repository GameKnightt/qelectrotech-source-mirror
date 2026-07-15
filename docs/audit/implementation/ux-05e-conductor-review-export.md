# UX-05E — Export CSV de revue des conducteurs

## Résultat utilisateur

L’éditeur **Modifier les conducteurs en tableau** propose maintenant
**Exporter pour revue…**. Le fichier reprend toutes les lignes du brouillon,
uniquement les colonnes visibles et exactement dans leur ordre à l’écran. Une
équipe peut ainsi préparer une vue compacte, l’ouvrir dans Excel ou un autre
tableur et la transmettre pour contrôle sans appliquer les modifications au
projet.

L’action reste disponible lorsque le brouillon contient des valeurs multiples,
des champs volontairement vidés ou des cellules invalides : le CSV est un
instantané de revue, pas une validation ni une application. Le résumé affiché
dans le dialogue précise les lignes, colonnes, modifications visibles ou
masquées, valeurs multiples, cellules invalides et protections tableur utiles.

## Contrat du fichier

- encodage UTF-8 avec BOM pour l’ouverture directe sous Windows et Excel ;
- séparateur point-virgule et fins de lignes CRLF déterministes ;
- guillemets CSV pour les séparateurs, guillemets et retours à la ligne ;
- en-têtes traduits issus du modèle affiché ;
- marqueurs stables `__QET_MIXED_VALUE__` et
  `__QET_EXPLICIT_EMPTY__` pour ne pas confondre une valeur multiple intacte,
  un effacement préparé et une chaîne réellement vide ;
- préfixe réversible `__QET_LITERAL__` pour neutraliser les cellules commençant
  par `=`, `+`, `-` ou `@` et éviter leur interprétation comme formule ;
- échappement du même préfixe et des marqueurs lorsqu’ils appartiennent à une
  valeur réelle.

UX-05E est volontairement limité à l’export. Un import fiable devra disposer
d’une identité persistante des potentiels et d’un contrat de version explicite ;
les pointeurs internes actuels ne peuvent pas réassocier un fichier externe à
une session ultérieure.

## Sécurité et compatibilité

Le sérialiseur reçoit un `const ConductorBulkEditModel &` et ne connaît ni le
projet, ni le plan de changement, ni la pile Undo. L’éditeur actif est validé
avant l’instantané, puis le modèle reste un brouillon. L’écriture utilise
`QSaveFile` en mode binaire, sans repli d’écriture directe : un échec conserve
le fichier existant et ne laisse pas de succès partiel.

Aucun format `.qet`, `.elmt`, XML ou SQLite n’est modifié. L’export ne crée
aucune donnée persistante dans le projet et ne change pas le comportement de
**Vérifier**, de l’aperçu avant/après ou de la transaction Undo unique des lots
UX-05A à UX-05D.

## Ergonomie et accessibilité

- bouton texte de 32 pixels minimum dans le bandeau existant ;
- ordre de tabulation : Colonnes, Recopier, Exporter, tableau, Réinitialiser,
  Vérifier, Annuler ;
- nom et description accessibles explicitant que le projet n’est pas modifié ;
- sélecteur de fichier Windows natif, nom proposé `conducteurs-revue.csv` ;
- aucun dialogue de succès supplémentaire : le résumé réutilise la zone d’état
  et conserve le chemin complet dans son infobulle accessible ;
- annuler le sélecteur ne crée rien et ne remplace pas le message courant.

## Validation automatisée

La cible Qt `conductor_bulk_edit_csv_test` couvre :

- l’ordre visuel et l’exclusion des colonnes masquées ;
- la validation de l’éditeur actif avant l’export ;
- le BOM UTF-8, les accents, le point-virgule, les guillemets et les CRLF ;
- la neutralisation des formules de tableur ;
- la distinction entre valeurs multiples et effacements explicites ;
- l’échec d’écriture atomique sans mutation du modèle ;
- deux sérialisations identiques de 1 000 potentiels ;
- les matrices hors écran et Windows natif à 100 % et 150 %.

## Validation Windows 11 réelle

La build Qt 5 de débogage a été lancée sous un nom de processus isolé afin de
ne pas être redirigée vers l’installation officielle par le mécanisme
d’instance unique de QElectroTech. Le parcours suivant a ensuite été reproduit
sur le projet public `examples/industrial.qet`, dans une fenêtre de
1 269 × 840 pixels :

1. ouverture du projet et refus de la copie de sauvegarde facultative ;
2. ouverture de la recherche avec `Ctrl+F` ;
3. vérification de la présence et de l’accessibilité de
   **Modifier les conducteurs en tableau…** ;
4. ouverture du brouillon sur les potentiels réels du projet ;
5. vérification du bandeau **Colonnes…**, **Recopier vers le bas** et
   **Exporter pour revue…** sans rognage ;
6. ouverture du sélecteur Windows **Exporter les conducteurs pour revue**, avec
   `conducteurs-revue.csv` présélectionné et le filtre `Fichiers CSV (*.csv)`.

Le point d’entrée de la recherche avancée avait initialement une hauteur nulle
dans cette configuration réelle. UX-05E impose désormais une ligne de 32
pixels et une hauteur minimale de 150 pixels au panneau avancé ; le bouton est
visible, utilisable et exposé dans l’arbre d’accessibilité Windows. Aucun
projet n’a été modifié pendant ce parcours.

La validation de l’écriture elle-même reste automatisée et déterministe : elle
est exécutée hors écran et avec la plateforme Windows native, à 100 % et 150 %,
et vérifie les octets du fichier plutôt que l’apparence du sélecteur natif.
