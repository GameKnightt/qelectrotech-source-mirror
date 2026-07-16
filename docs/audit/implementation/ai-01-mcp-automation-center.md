# AI-01 — Serveur MCP et centre Automatisation et IA

## Résultat utilisateur

QElectroTech peut maintenant être lancé comme serveur MCP local afin qu’un
assistant compatible inspecte, valide ou produise des copies de projets
QElectroTech. Le menu **Projet > Automatisation et IA…** ouvre un panneau
moderne qui :

- reprend automatiquement le projet actif et son répertoire autorisé ;
- affiche les nombres de folios, éléments et conducteurs ;
- démarre en lecture seule ;
- prépare la commande locale et une configuration JSON neutre ;
- permet de copier les deux valeurs ;
- décrit les quatre outils disponibles et les garanties d’écriture.

La vue est le premier composant QML du fork, hébergé dans un `QDockWidget` sans
migration du reste de l’application.

## Contrats préservés

- aucun changement de schéma `.qet`, `.elmt` ou cartouche ;
- aucun fournisseur IA ni secret ajouté à QElectroTech ;
- aucune écriture sans `--mcp-write`, `confirm=true` et nouvelle destination ;
- aucune fenêtre de course ne permet de remplacer un fichier créé pendant
  l’opération : la copie temporaire est publiée avec une primitive exclusive
  (renommage sans remplacement sous Windows, lien atomique sous Unix) ;
- aucun dialogue de compatibilité ne peut bloquer le serveur sans interface ;
- aucune modification du projet actuellement ouvert par le serveur ;
- mode graphique historique et instance unique inchangés ;
- Qt 5 Widgets reste l’architecture principale.

## Implémentation

- `sources/ai/qetmcpserver.*` : transport JSON-RPC/MCP sur `stdio` ;
- `sources/ai/qetmcpprojectservice.*` : outils fondés sur le modèle QET ;
- `sources/ai/ui/automationcentercontroller.*` : pont C++ typé ;
- `sources/ai/ui/automationcenterdock.*` : intégration Qt Widgets ;
- `sources/ai/ui/qml/AutomationCenter.qml` : présentation Qt Quick native,
  sans module de contrôles externe ;
- `scripts/test-mcp-stdio.ps1` : scénario d’intégration de bout en bout ;
- `tests/qml/tst_AutomationCenter.qml` : comportement, clavier et largeur ;
- `tests/qttest/tst_automationcenterqml.cpp` : lanceur Qt Quick Test.

Le mode MCP est traité avant `SingleApplication` afin qu’un client puisse
ouvrir plusieurs processus serveurs sans déclencher ni contacter l’interface
graphique déjà ouverte.

## Critères d’acceptation

| Critère | Résultat |
|---|---|
| Initialisation MCP et négociation de version | Réussi |
| Liste des quatre outils et schémas JSON | Réussi |
| Inspection d’un projet par le vrai modèle QET | Réussi |
| Création de deux folios dans un nouveau fichier | Réussi |
| Mise à jour de cartouches dans une copie | Réussi |
| Validation du fichier produit | Réussi |
| Refus implicite de remplacement de fichier | Couvert par le contrat de service |
| Périmètre limité aux racines canoniques | Couvert par le service |
| Autre volume Windows / chemin absolu relatif | Refusé |
| Destination créée pendant l’écriture | Refusée lors de la publication exclusive |
| Projet futur ou QET 0.6 et antérieur | Refusé sans dialogue modal |
| Enveloppes JSON-RPC invalides et message > 1 Mio | Refusés, session préservée |
| Panneau visible dans l’application Windows réelle | Réussi à 1920×1080 |
| Panneau rendu depuis le paquet portable isolé | Réussi sans MSYS2 dans le `PATH` |
| Cycle MCP depuis le paquet portable en mode hors écran | Réussi |
| Navigation clavier, défilement, focus et largeur de 360 px | Réussi |
| Mise à l’échelle 150 % | Réussi |

## Preuves

- [Capture Windows 1920×1080](../evidence/ai-01-mcp-automation-center/01-automation-center.png)
- [Capture du paquet portable isolé](../evidence/ai-01-mcp-automation-center/02-portable-runtime.png)
- [Registre du lot](../evidence/ai-01-mcp-automation-center/README.md)
- [Architecture et guide MCP](../../architecture/ai-mcp-qml.md)

## Limites

- les clients MCP n’utilisent pas tous le même emplacement de configuration ;
- aucune génération automatique de schéma complet n’est encore annoncée ;
- les écritures actuelles couvrent la création de projets/folios et les
  cartouches, pas le placement d’éléments ni le routage ;
- le panneau ne constitue pas un agent conversationnel embarqué ;
- le transport distant, les identités persistantes d’objets et le diff
  sémantique restent des lots futurs ;
- Linux, macOS et Qt 6 ne sont pas qualifiés par ce lot Windows.
