# UI-03B — Démarrer depuis un exemple curaté

## Résultat produit

Le centre de démarrage propose désormais quatre projets représentatifs :
Arduino et écran LCD, séquence GRAFCET, habitat unifilaire et installation
industrielle. Chaque exemple est présenté avec sa discipline et son nombre de
folios, sans analyse du fichier au démarrage.

Une activation à la souris ou avec Entrée charge le projet comme une **copie non
enregistrée**. La copie est immédiatement éditable et marquée **Modifié**. Son
premier `Ctrl+S` ouvre obligatoirement **Enregistrer sous** ; le fichier livré
dans `examples/` n'est jamais la cible de sauvegarde.

## Contrats de sûreté

- le catalogue est une liste blanche fixe de quatre identifiants et noms de
  fichiers `.qet` ; aucun chemin fourni par l'utilisateur n'est accepté ;
- le chemin canonique doit rester sous une racine `examples` autorisée, y
  compris en présence d'un lien symbolique ou d'une installation incomplète ;
- le chargement ne passe pas par `openAndAddProject()`, donc le chemin livré
  n'est ajouté ni aux récents ni au dialogue de copie de sauvegarde ;
- `QETProject::detachAsUntitledCopy()` attend la fin d'une éventuelle sauvegarde
  de récupération, invalide le chemin canonique, retire la lecture seule,
  efface `savedfilename` / `savedfilepath` et marque la copie comme modifiée ;
- une ressource manquante reste visible mais désactivée tant qu'au moins un
  autre exemple est disponible ; si les quatre manquent, la section disparaît
  sans affecter Nouveau, Ouvrir ou les récents ;
- aucun format `.qet`, `.elmt`, cartouche ou SQLite n'est modifié.

## Interface et accessibilité

La section réutilise `QGroupBox`, `QCommandLinkButton`, la palette et les
métriques du centre existant. Les quatre entrées occupent deux colonnes sur un
écran large et une seule sous 760 pixels logiques. Les noms et descriptions
accessibles annoncent explicitement l'ouverture en copie non enregistrée.

Entrée et Retour activent exactement une fois l'exemple focalisé. À 150 %, le
contenu conserve un défilement horizontal nul ; le défilement vertical existant
permet d'atteindre toutes les entrées sur un écran logique 1280×680.

## Packaging Windows

`deploy-portable.ps1` copie uniquement les quatre exemples curatés dans
`portable/examples/`, vérifie leur présence avant et après déploiement et les
inscrit dans `manifest-sha256.txt`. Le ZIP portable ne dépend donc plus du
pipeline NSIS historique pour rendre cette fonction utilisable.

## Validation du 15 juillet 2026

- application complète compilée sous Windows 11, Qt 5 et UCRT64 ;
- sept variantes CTest du centre de démarrage réussies ;
- catalogue, chemins sûrs, structure XML des quatre fichiers, absence de
  ressource, activation souris/clavier, accessibilité et texte à 150 % couverts ;
- parcours dans l'application complète : accueil affiché, ArduinoLCD ouvert en
  copie, titre et barre d'état **Modifié**, puis `Ctrl+S` ouvrant
  **Enregistrer sous** avec `sansnom.qet` ;
- capture Qt reproductible enregistrée dans
  `docs/audit/evidence/ui-03b-curated-examples/01-start-center.png`.

## Limites et suite

Les exemples sont des ressources publiques de démonstration, pas des modèles
normatifs ni une bibliothèque métier certifiée. La suite de la roadmap doit
ajouter des modèles paramétrables et la vue consolidée Borniers et câbles sans
élargir ce lot à une migration de format.
