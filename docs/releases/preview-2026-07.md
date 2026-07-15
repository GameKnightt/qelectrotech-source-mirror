# Préversion Windows — juillet 2026

Cette préversion communautaire non officielle rassemble les premiers lots du
fork ergonomie et productivité sur la base QElectroTech 0.100.1. Elle reste
compatible avec les formats historiques et peut être utilisée à côté de la
version officielle grâce à son profil de configuration portable isolé.

## Points forts

- centre de démarrage orienté tâches, quatre exemples métier toujours ouverts
  comme copies non enregistrées et profils d’espace de travail **Essentiel** /
  **Classique** ;
- shell contextuel, navigation rapide entre folios et recherche de collections
  clarifiée ;
- inspecteur de propriétés contextuel et sélection des conducteurs fiabilisée ;
- centre d’export unifié avec écritures atomiques et erreurs explicites ;
- édition groupée des conducteurs avec aperçu, tableau, collage TSV, recopie
  vers le bas, colonnes persistantes et export CSV de revue ;
- vue consolidée **Borniers et câbles** avec recherche, filtres, tri naturel,
  agrégation des conducteurs et navigation vers le folio ;
- sauvegarde et récupération renforcées, statut **Modifié/Sauvegardé** fidèle à
  l’écriture réelle et historique Undo conservé ;
- reconstruction transactionnelle de la base SQLite du projet ;
- dialogues Windows adaptatifs et contrats clavier / grande police à 150 %.

## Validation

- application complète compilée en Release sous Windows 11, Qt 5 et MSYS2
  UCRT64 ;
- 43/43 tests CTest réussis en série ;
- 14/14 contrats clavier sur la plateforme Windows native ;
- 21/21 contrats Windows natifs à 150 % ;
- commande `qelectrotech.exe --version` vérifiée sans démarrage graphique ;
- paquet portable vérifié avec ses dépendances, ses ressources, ses quatre
  exemples curatés et un manifeste SHA-256 ; le ZIP est réextrait avec
  `Expand-Archive` et tous les chemins, y compris accentués, sont revérifiés ;
- parcours réel vérifié : démarrage, ouverture d’un projet, modification,
  sauvegarde et retour au statut **Sauvegardé**.

## Installation portable

1. Télécharger l’archive `readytouse.zip` de la release du fork.
2. Extraire entièrement l’archive dans un dossier local court.
3. Fermer toute autre instance de QElectroTech.
4. Lancer `Launch-QElectroTech-Preview.bat`.
5. Pour les premiers essais, ouvrir une copie d’un projet existant.

Le sous-dossier `conf/` conserve les préférences de cette préversion sans
écraser celles de l’installation officielle.

## Compatibilité et limites

Cette préversion ne modifie pas les formats `.qet`, `.elmt`, les cartouches XML
ni les collections existantes. Elle n’est pas signée et Windows peut afficher
un avertissement de réputation. La qualification principale couvre Windows 11
et Qt 5 ; Linux, macOS et Qt 6 ne sont pas encore annoncés comme distributions
validées de ce fork.

Avant une version stable, il reste notamment à terminer l’inventaire des
licences tierces, la signature, les essais sur une machine Windows propre et la
qualification de projets métier représentatifs en automatisme, pneumatique,
hydraulique et process.

La première vue Borniers et câbles est fonctionnelle sans changement de format.
Elle ne constitue pas encore un modèle complet de câble ni une validation
métier des plans de câblage ; ces extensions sont prévues dans IND-01B/IND-01C.

Les détails et preuves sont disponibles dans le
[README du fork](../../README.md), l’[audit](../audit/qet-audit.md) et l’[index
des implémentations](../audit/implementation/README.md).
