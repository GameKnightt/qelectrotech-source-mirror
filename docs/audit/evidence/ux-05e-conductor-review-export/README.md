# UX-05E — Preuves de validation

Validation manuelle réalisée sous Windows 11 avec la build Qt 5 locale et le
projet public `examples/industrial.qet` :

- le point d’entrée **Modifier les conducteurs en tableau…** est visible dans
  le panneau de recherche avancée à 1 269 × 840 pixels ;
- le dialogue contient les trois actions **Colonnes…**, **Recopier vers le
  bas** et **Exporter pour revue…**, sans contrôle rogné ;
- le bouton d’export possède le nom accessible **Exporter le brouillon pour
  revue** ;
- son activation ouvre le sélecteur de fichier Windows avec le titre
  **Exporter les conducteurs pour revue**, le nom `conducteurs-revue.csv` et le
  filtre `Fichiers CSV (*.csv)` ;
- le statut initial confirme qu’aucune modification n’est préparée et que le
  projet reste inchangé.

Les preuves octet par octet sont maintenues dans
`tests/qttest/tst_conductorbulkedit.cpp` : BOM UTF-8, point-virgule, CRLF,
échappement, neutralisation des formules, valeurs multiples, effacement
explicite, écriture atomique et stabilité sur 1 000 lignes. La matrice CI les
exécute hors écran et avec la plateforme Windows native, à 100 % et 150 %.

La session de validation a été arrêtée sans enregistrer le fichier proposé :
aucun fichier de projet ni fichier utilisateur n’a été créé ou remplacé.
