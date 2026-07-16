# Préversion Windows — juillet 2026

Cette préversion communautaire non officielle rassemble les premiers lots du
fork ergonomie et productivité sur la base QElectroTech 0.100.1. Elle reste
compatible avec les formats historiques et peut être utilisée à côté de la
version officielle grâce à son profil de configuration portable isolé.

## Points forts

- serveur MCP local et neutre vis-à-vis des fournisseurs pour connecter Claude,
  ChatGPT, Gemini ou un autre client compatible, avec lecture seule par défaut,
  périmètre explicite et écritures uniquement vers de nouvelles copies ; le
  chargement est non interactif, les entrées sont bornées et la publication
  exclusive ne remplace jamais une destination apparue pendant l’opération ;
- panneau QML **Automatisation et IA** intégré à la fenêtre Qt Widgets pour
  préparer la commande et la configuration du client ;
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
- catalogue hiérarchique des câbles couvrant tous les folios, diagnostics
  prudents, navigation vers le conducteur et export CSV UTF-8 ;
- édition exacte d’un ou plusieurs câbles/conducteurs depuis le catalogue,
  limitée aux champs Couleur et Câble, avec aperçu et Undo atomique ;
- sauvegarde et récupération renforcées, statut **Modifié/Sauvegardé** fidèle à
  l’écriture réelle et historique Undo conservé ;
- reconstruction transactionnelle de la base SQLite du projet ;
- duplication de folios sans collision d’identifiants, avec liens croisés
  préservés et une étape Annuler/Rétablir atomique ;
- dialogues Windows adaptatifs et contrats clavier / grande police à 150 %.

## Livraison Windows

- le workflow MSI ne bloque plus la publication lorsque les secrets SignPath ne
  sont pas encore configurés ; le paquet de repli porte explicitement le suffixe
  `-unsigned.msi` et la page de téléchargement signale l'avertissement éditeur ;
- le workflow MSI utilise exactement le commit du build portable déclencheur,
  afin d'éviter tout mélange entre sources et binaires.

## Validation

- application complète compilée en Release sous Windows 11, Qt 5 et MSYS2
  UCRT64 ;
- 51/51 tests CTest réussis en série ;
- 13/13 contrats IND-01C réussis, plus le scénario exact sous Windows natif à
  100 % et 150 % ;
- 12/12 contrats grande police à 150 % hors écran ;
- 17/17 contrats clavier sur la plateforme Windows native ;
- 25/25 contrats Windows natifs à 150 % ;
- commande `qelectrotech.exe --version` vérifiée sans démarrage graphique ;
- cycle MCP complet vérifié : initialisation, liste des outils, création,
  inspection, cartouches, validation et arrêt ;
- centre QML vérifié à largeur minimale, au clavier, à 150 % et dans
  l’application complète à 1920×1080 ;
- paquet portable vérifié avec ses dépendances, ses ressources, ses quatre
  exemples curatés et un manifeste SHA-256 ; le ZIP est réextrait avec
  `Expand-Archive` et tous les chemins, y compris accentués, sont revérifiés ;
- cycle MCP hors écran et panneau QML vérifiés depuis ce paquet avec un `PATH`
  limité à Windows, sans dépendre de l’installation MSYS2 de compilation ;
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

Le serveur MCP n’embarque aucun modèle et ne stocke aucune clé API. Les clients
externes restent responsables de l’envoi éventuel de données à leur
fournisseur. Le mode écriture doit être activé au lancement et chaque opération
produit une destination distincte après confirmation explicite.

Avant une version stable, il reste notamment à terminer l’inventaire des
licences tierces, la signature, les essais sur une machine Windows propre et la
qualification de projets métier représentatifs en automatisme, pneumatique,
hydraulique et process.

Les vues Bornes et Câbles sont fonctionnelles sans changement de format. Le
catalogue s’appuie sur les champs historiques libres et ne constitue pas une
certification métier des plans de câblage. IND-01C permet maintenant leur
correction exacte avec aperçu et Undo ; réserves, destinations structurées et
objet `Cable` persistant restent hors de cette préversion.

Les détails et preuves sont disponibles dans le
[README du fork](../../README.md), l’[audit](../audit/qet-audit.md) et l’[index
des implémentations](../audit/implementation/README.md).
