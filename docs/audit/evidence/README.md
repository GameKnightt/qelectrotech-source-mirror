# Registre des preuves visuelles

Audit réalisé le 13 juillet 2026 sous Windows 11, sur un écran de validation 1920×1080, avec QElectroTech stable 0.100.0 installé dans `C:\Program Files\QElectroTech`.

## Statut des captures

Les vues ci-dessous ont été ouvertes et contrôlées pendant la session d'audit. L'outil de contrôle Windows a fourni des captures transitoires validées à l'écran, mais leur export binaire local n'a pas pu être fiabilisé sans recourir à une méthode de capture non autorisée par le protocole d'audit. Ce registre constitue donc la preuve textuelle numérotée de la session ; chaque absence de fichier PNG est explicitement classée comme limite de collecte et non comme validation visuelle définitive.

Avant le démarrage de la phase de développement, les scénarios marqués **À rejouer** devront être reproduits manuellement et enregistrés sous les noms proposés dans cette table.

| ID | Parcours et état observé | Résultat | Fichier attendu |
|---|---|---|---|
| E01 | Démarrage de la stable 0.100.0 | Barres d'outils denses, majorité d'actions sous forme d'icônes, grande zone centrale vide, trois zones latérales concurrentes | `01-demarrage-espace-de-travail.png` |
| E02 | Création d'un projet vierge | Création réussie ; folio et cartouches visibles dans le navigateur | `02-projet-vierge.png` |
| E03 | Propriétés du projet à 1920×1080 | Dialogue mesuré à environ 1402×1032 ; boutons proches du bord inférieur, navigation verticale et onglets horizontaux imbriqués, espace vide important | `03-proprietes-projet-1920x1080.png` |
| E04 | Ouverture de `industrial.qet` (2,6 Mo, 150 folios) | Ouverture réussie en environ 7,5 s ; navigateur très long avec libellés tronqués | `04-projet-industriel-150-folios.png` |
| E05 | Sélection d'un composant sur le folio « Mains Power Supply » | Propriétés disponibles dans un dock étroit nécessitant un défilement long | `05-proprietes-composant.png` |
| E06 | Recherche de `contacteur` | Déploiement simultané des arbres projet et collection ; doublons perçus, libellés tronqués, absence de compteur et de liste de résultats plate | `06-recherche-contacteur.png` |
| E07 | Configuration de l'application | Dialogue proche de 1402×1032 ; catégories verticales plus onglets horizontaux et forte surface inutilisée | `07-configuration-1920x1080.png` |
| E08 | Génération de nomenclature | Dialogue 694×655 ; options génératrices de folio pré-cochées, actions charger/enregistrer principalement iconographiques | `08-nomenclature.png` |
| E09 | Menu Projet | Sommaire, nomenclature, CSV, noms de conducteurs, gestionnaire de borniers DEV, greffon de bornier et export de base dispersés | `09-menu-projet.png` |
| E10 | Menu Fichier | Export générique, export PDF et impression disponibles comme actions séparées | `10-menu-fichier.png` |
| E11 | Export graphique | Dialogue d'au moins 800×590 ; paramètres par folio, format, rendu et dossier présents ; audit annulé avant toute écriture | `11-export-graphique.png` |
| E12 | Recherche de `pneumatique` | Résultats textuels comprenant des contacts électriques à actionnement pneumatique ; pas de filtre de discipline explicite constaté | `12-recherche-pneumatique.png` |
| E13 | À propos / informations système | 0.100.0, Qt 5.15.18, GCC 14.3.0, compilation du 25 janvier 2026, Windows 11 noyau 10.0.26200 | `13-a-propos-version.png` |
| E14 | Folio terminal TB1 de l'exemple | Folio ouvert mais contenu vide dans l'exemple fourni ; insuffisant pour valider le parcours bornier | `14-folio-bornier-incomplet.png` |
| E15 | Sélection et modification d'un conducteur | Cible fine difficile à accrocher par automatisation ; résultat partiel, à rejouer avec un opérateur et un projet réel | `15-selection-conducteur.png` |
| E16 | Glisser-déposer d'un symbole | Automatisation non concluante ; ne constitue pas un défaut produit, à rejouer manuellement | `16-glisser-deposer-element.png` |

## Règles de rejeu

- Utiliser Windows 11 à 100 %, 125 % et 150 % de mise à l'échelle, avec une fenêtre utile de 1920×1080.
- Conserver les projets de test anonymisés hors du dépôt s'ils contiennent des données métier.
- Capturer l'état avant action, l'état pendant l'action si pertinent, puis le résultat ou le blocage.
- Ne jamais classer E15 ou E16 comme défaut avant reproduction manuelle.
- Ajouter pour chaque PNG la version exacte de QElectroTech et le nom anonymisé du projet dans les métadonnées du registre.

## Couverture

Les parcours démarrage, projet, folios, composants, recherche, propriétés,
nomenclature, impression/export, préférences et affichage ont été observés au
moins partiellement. La vue initiale Borniers et câbles est maintenant raccordée
et couverte par un état vide réel plus une vue peuplée déterministe. La
qualification complète des câbles et borniers, les E/S d'automate, le
pneumatique, l'hydraulique et le process restent **À rejouer** sur des projets
représentatifs fournis par l'utilisateur.

## Incrément UX-05D — 14 juillet 2026

Le dossier [`ux-05d-column-layout`](ux-05d-column-layout/) contient les captures
PNG exactes de la validation Windows 11 du dialogue de modification tabulaire :

| ID | État observé | Santé | Fichier |
|---|---|---|---|
| UX05D-00 | Avant correction, l’exemple public `industrial.qet` n’expose aucun potentiel dans la catégorie Conducteurs de la recherche avancée | Défaut reproduit | `00-entry-limitation-industrial.png` |
| UX05D-01 | Vue complète du composant compilé avec 36 potentiels | Sain | `01-default-columns.png` |
| UX05D-02 | Menu **Colonnes…** et potentiel obligatoire | Sain | `02-columns-menu.png` |
| UX05D-03 | Vue compacte et ordre déplacé | Sain | `03-compact-reordered.png` |
| UX05D-04 | Brouillon présent dans une colonne masquée et annoncé | Sain | `04-hidden-change-announcement.png` |
| UX05D-05 | Disposition restaurée après fermeture et réouverture | Sain | `05-persisted-layout.png` |
| UX05D-06 | Retour aux sept colonnes par défaut | Sain | `06-reset-default.png` |
| UX05D-07 | Après récupération de l’index vide, l’application complète ouvre le tableau avec les potentiels réels de `industrial.qet` | Sain, parcours de bout en bout | `07-end-to-end-recovered.png` |

Les vues UX05D-01 à UX05D-06 proviennent d’un banc temporaire qui instancie le
dialogue et le modèle réels de la préversion ; aucune réimplémentation visuelle
n’est utilisée. UX05D-00 et UX05D-07 proviennent de l’application complète,
respectivement avant et après correction. Le détail du protocole et ses limites se trouvent dans
[`implementation/ux-05d-column-layout.md`](../implementation/ux-05d-column-layout.md).

## Incrément UI-03B — 15 juillet 2026

Le dossier [`ui-03b-curated-examples`](ui-03b-curated-examples/) contient la
capture native du centre de démarrage enrichi. Les quatre exemples livrés y
sont visibles avec leur discipline et leur nombre de folios, sans débordement
horizontal. Le parcours Windows complet a aussi vérifié qu'un exemple s'ouvre
comme copie **Modifié** et que son premier `Ctrl+S` propose `sansnom.qet` au
lieu du fichier source livré.

## Incrément IND-01A — 15 juillet 2026

Le dossier
[`ind-01a-terminal-cable-overview`](ind-01a-terminal-cable-overview/) contient
la capture reproductible de la nouvelle vue consolidée avec six bornes
déterministes. Le parcours Windows dans l’application complète vérifie en plus
le libellé **Borniers et câbles…**, l’ouverture du gestionnaire, son état vide
réel sur `ArduinoLCD.qet`, le focus `Ctrl+F` et le rechargement `F5`.
