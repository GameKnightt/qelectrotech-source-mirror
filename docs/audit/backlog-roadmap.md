# Backlog consolidé et roadmap du fork QElectroTech

**Référence :** audit du 13 juillet 2026  
**Principe :** compatibilité amont et formats existants préservés  
**Échelle :** P0 intégrité/blocage, P1 friction métier forte, P2 productivité/cohérence/accessibilité, P3 évolution stratégique

## Règles de sélection

Chaque entrée est estimée relativement : **S** (quelques jours), **M** (1 à 3 semaines), **L** (plusieurs semaines), **XL** (programme multi-lots). Une estimation détaillée nécessite un prototype technique et des projets de test.

Un ticket n'entre en développement que si sa preuve est reproductible, son contrat de compatibilité est identifié et son test d'acceptation peut être automatisé ou rejoué de façon déterministe.

## P0 — intégrité et confiance

### DATA-01 — Garantir l'unicité après duplication de folio

- **Preuve :** GitHub #532 ; erreur de contrainte `UNIQUE` SQLite et composants manquants du sommaire signalés.
- **Utilisateurs touchés :** tous les projeteurs dupliquant des folios modèles.
- **Fréquence :** régulière dans les projets répétitifs.
- **Impact :** données ou rapports incomplets, défaut potentiellement silencieux.
- **Effort :** M.
- **Risque :** élevé, logique d'identité et relations croisées.
- **Dépendances :** modèle `QETProject`/`Diagram`, import XML, base SQLite, sommaire et nomenclature.
- **Compatibilité amont :** forte si seuls les nouveaux objets reçoivent de nouveaux UUID valides.
- **Critères d'acceptation :**
  - dupliquer 100 fois un folio contenant éléments, renvois et conducteurs sans UUID répété ;
  - aucune violation SQLite ;
  - mêmes nombres d'objets dans le document, le sommaire, la nomenclature et la base projet ;
  - annuler/rétablir la duplication sans résidu ;
  - ouvrir le fichier produit dans la stable sans perte.

### DATA-02 — Définir la priorité des variables projet/folio

- **Preuve :** GitHub #531 ; variables de folio vides créées automatiquement et masquant celles du projet.
- **Utilisateurs touchés :** entreprises utilisant des cartouches personnalisés et variables globales.
- **Fréquence :** fréquente dans les modèles standardisés.
- **Impact :** cartouches et exports incorrects.
- **Effort :** S à M.
- **Risque :** élevé, changement possible du rendu historique.
- **Dépendances :** `assignvariables.cpp`, propriétés projet/folio, cartouches, exports.
- **Compatibilité amont :** nécessite une règle explicite sans migration destructive.
- **Critères d'acceptation :**
  - une variable de projet est utilisée si la variable de folio n'existe pas ou suit la règle de vide décidée ;
  - une surcharge de folio non vide reste prioritaire ;
  - aucune propriété projet personnalisée n'est supprimée à l'ouverture/enregistrement ;
  - tests couvrant ancien fichier, nouveau fichier, import et export PDF/CSV.

### SAVE-01 — Tester sauvegarde atomique et récupération

- **Preuve :** récupération `KAutoSaveFile` présente ; risque intrinsèque de la persistance projet/base.
- **Utilisateurs touchés :** tous.
- **Fréquence :** rare, impact maximal.
- **Impact :** perte de travail en cas de crash, disque plein ou interruption.
- **Effort :** M.
- **Risque :** élevé.
- **Dépendances :** sauvegarde projet, tâche asynchrone, base SQLite, récupération.
- **Compatibilité amont :** aucun format nouveau requis.
- **Critères d'acceptation :** matrice automatisée crash simulé, disque plein, chemin non accessible, fermeture pendant sauvegarde ; récupération explicite et fichier original toujours ouvrable.

## P1 — stabilisation et parcours métier centraux

### DEV-01 — Rendre le clone Windows reproductible

- **Preuve :** quota LFS épuisé sur `doc/QElectroTech.qch` ; collision `ChangeLog.MD`/`ChangeLog.md`.
- **Utilisateurs touchés :** contributeurs Windows et CI.
- **Fréquence :** à chaque nouvel environnement.
- **Impact :** impossibilité de démarrer.
- **Effort :** S à M, avec coordination amont.
- **Risque :** faible pour le produit, moyen pour l'historique Git.
- **Dépendances :** hébergement LFS, documentation, convention de noms.
- **Compatibilité amont :** proposer d'abord un correctif amont minimal.
- **Critères d'acceptation :** clone neuf avec sous-modules sur NTFS sans fichier modifié au checkout et sans téléchargement indisponible.

### DEV-02 — Documenter et automatiser le build Windows actuel

- **Preuve :** page Qt 4/SVN obsolète ; workflow CI MSYS2 moderne ; absence de bootstrap local.
- **Utilisateurs touchés :** contributeurs et mainteneurs.
- **Fréquence :** chaque installation/mise à niveau.
- **Impact :** temps perdu, environnements divergents.
- **Effort :** M.
- **Risque :** faible.
- **Dépendances :** versions de paquets, CI, packaging.
- **Compatibilité amont :** documentation et scripts additifs.
- **Critères d'acceptation :** machine Windows 11 propre jusqu'au test lancé en moins d'une heure, commandes identiques en local et CI, versions verrouillées ou contrôlées.

### DEV-03 — Corriger l'activation des tests et créer une matrice de contrats

- **Preuve :** CI utilise `BUILD_TESTING` mais CMake déclare `PACKAGE_TESTS`; couverture métier limitée.
- **Utilisateurs touchés :** mainteneurs et tous les utilisateurs indirectement.
- **Fréquence :** chaque changement.
- **Impact :** régressions non détectées.
- **Effort :** M puis continu.
- **Risque :** faible.
- **Dépendances :** CMake, fixtures anonymisées.
- **Compatibilité amont :** forte.
- **Critères d'acceptation :** option unique documentée, CI prouvant le nombre de tests, fixtures XML/SQLite, tests stable→fork→stable, duplication, variables, sauvegarde, Undo/Redo et exports.

### EXPORT-01 — Garantir l'intégrité des exports interactifs

- **État :** implémenté et validé sur la branche `codex/export-01-integrity`.
- **Preuve :** suppressions préalables de CSV, erreurs d'écriture ignorées, succès partiels silencieux, état de rendu non restauré et arrêt du processus dans `Createdxf`.
- **Utilisateurs touchés :** tous ceux produisant des livrables PDF, image, SVG, DXF ou CSV.
- **Fréquence :** à chaque remise de dossier ou révision.
- **Impact :** ancien livrable détruit, fichier incomplet présenté comme valide, perte de confiance ou fermeture de l'application.
- **Effort :** M.
- **Risque :** moyen, car les générateurs historiques restent utilisés sans changement de format.
- **Dépendances :** `ExportDialog`, `Createdxf`, impression/PDF, BOM, export des conducteurs et `QSaveFile`.
- **Compatibilité amont :** forte ; mêmes formats et mêmes points d'entrée, erreurs désormais bloquantes.
- **Critères d'acceptation :** ancien fichier préservé sur échec, aucun succès sans rendu/écriture/commit valides, UTF-8 déterministe, CSV échappé, état UI restauré et erreur DXF sans arrêt du processus.

### DATA-03 — Rendre le rafraîchissement de la base projet vérifiable

- **Preuve :** `projectDataBase::updateDB()` retourne `void`, abandonne après certains échecs et plusieurs insertions journalisent leurs erreurs sans les propager.
- **Utilisateurs touchés :** utilisateurs de nomenclatures, sommaires et vues fondées sur la base interne.
- **Fréquence :** à chaque génération ou rafraîchissement de rapport.
- **Impact :** rapport cohérent en apparence mais construit à partir de données antérieures ou incomplètes.
- **Effort :** M.
- **Risque :** élevé, transactions et synchronisation de plusieurs tables.
- **Dépendances :** `projectDataBase`, requêtes BOM, modèle de projet et CLI.
- **Compatibilité amont :** aucun format nouveau ; évolution du contrat interne de retour d'erreur.
- **Critères d'acceptation :** résultat typé pour chaque rafraîchissement, rollback sur première erreur, export refusé avec diagnostic, tests injectant une erreur SQL dans chaque table et aucune donnée partielle observable.

### UX-01 — Rendre les dialogues adaptatifs à Windows/DPI

- **Preuve :** E03, E07 ; forum #3073 ; minima 800×650 et 800×590 dans le code.
- **Utilisateurs touchés :** Windows, petits écrans, forte mise à l'échelle, accessibilité.
- **Fréquence :** fréquente.
- **Impact :** validation/annulation difficile ou impossible.
- **Effort :** M.
- **Risque :** moyen, nombreux formulaires `.ui`.
- **Dépendances :** gabarit commun de dialogue, tests visuels.
- **Compatibilité amont :** excellente, aucun format modifié.
- **Critères d'acceptation :** actions visibles à 1366×768/100 %, 1920×1080/150 % ; contenu défilable, entête et pied fixes ; navigation clavier ; aucune taille minimale supérieure à l'espace utile.

### UX-02 — Navigateur de projet efficace sur grands dossiers

- **Preuve :** E04, exemple de 150 folios.
- **Utilisateurs touchés :** projeteurs sur machines et installations complexes.
- **Fréquence :** continue.
- **Impact :** temps de navigation et erreurs de contexte.
- **Effort :** M à L.
- **Risque :** moyen.
- **Dépendances :** modèle projet, sélection synchronisée, raccourcis.
- **Compatibilité amont :** modèle de présentation additif.
- **Critères d'acceptation :** recherche instantanée titre/numéro/champ, groupes, récents/favoris, précédent-suivant, sélection conservée ; projet de 500 folios fluide.

### UX-03 — Séparer recherche et exploration des collections

- **Preuve :** E06, E12.
- **Utilisateurs touchés :** toutes disciplines.
- **Fréquence :** très fréquente.
- **Impact :** choix lent et résultats ambigus.
- **Effort :** M.
- **Risque :** moyen, indexation/cache.
- **Dépendances :** modèle collection, métadonnées, aperçu.
- **Compatibilité amont :** nouvelle vue au-dessus du modèle actuel.
- **Critères d'acceptation :** liste plate avec compteur, chemin, aperçu, discipline, provenance ; tri clavier ; double-clic/Entrée place l'élément ; l'arbre ne se déploie pas automatiquement.

### UX-04 — Améliorer sélection et propriétés des conducteurs

- **Preuve :** E15 partielle, GitHub #500.
- **État du fork :** la sélection tolérante, le survol et l'édition groupée des
  conducteurs sont complétés par PROPS-01 : le dock de propriétés suit le folio
  actif, distingue les états vide/mixte/non pris en charge et place les
  éditeurs existants dans un conteneur défilable. Le contrat est documenté dans
  `docs/audit/implementation/props-01-contextual-properties.md`.
- **Utilisateurs touchés :** électricité et automatisme en priorité.
- **Fréquence :** continue.
- **Impact :** friction majeure du dessin.
- **Effort :** M.
- **Risque :** moyen à élevé, événements graphiques.
- **Dépendances :** hit-testing, sélection, panneau propriétés, Undo.
- **Compatibilité amont :** s'appuyer sur la proposition amont #500.
- **Critères d'acceptation :** surbrillance au survol ; tolérance écran stable quel que soit le zoom ; panneau persistant ; multi-édition ; chaque mutation annulable ; aucune régression de placement.

### UX-05 — Ajouter une édition tabulaire groupée

- **Preuve :** formulaires longs E05 ; benchmark KiCad/EPLAN ; demandes de productivité.
- **État du fork :** UX-05A fournit le socle transactionnel : aperçu
  avant/après, expansion et déduplication des potentiels, revalidation
  anti-périmé et une seule commande Undo. UX-05B ajoute la grille de brouillon
  par potentiel pour la fonction, la tension ou le protocole, la couleur et la
  section, avec collage TSV, validation par cellule, conservation des valeurs
  mixtes et aucune mutation avant la seconde confirmation. Voir
  `docs/audit/implementation/ux-05a-conductor-change-preview.md` et
  `docs/audit/implementation/ux-05b-conductor-bulk-editor.md`.
- **Utilisateurs touchés :** projets répétitifs et contrôle qualité.
- **Fréquence :** régulière.
- **Impact :** gain de temps élevé.
- **Effort :** L.
- **Risque :** élevé si validation/Undo incomplets.
- **Dépendances :** modèle commun de propriétés, sélection, filtres, Undo.
- **Compatibilité amont :** vue additive sans modifier le stockage.
- **Critères d'acceptation :** colonnes configurables, édition multi-cellules, validation avant application, prévisualisation du nombre d'objets, transaction Undo unique.
- **Incrément suivant UX-05C :** sélection rectangulaire, remplissage vers le
  bas, colonnes configurables et import/export de feuilles de calcul, puis
  extension progressive aux éléments, borniers et câbles.

### IND-01 — Unifier le parcours borniers et câbles

- **Preuve :** E09, E14 ; GitHub #405 et #409.
- **Utilisateurs touchés :** tableautiers, automaticiens, maintenance.
- **Fréquence :** centrale dans les projets concernés.
- **Impact :** données éclatées et sorties incomplètes.
- **Effort :** L à XL.
- **Risque :** élevé, modèles encore en évolution.
- **Dépendances :** TerminalStrip, conducteurs, câbles, nomenclature, fabricant.
- **Compatibilité amont :** privilégier métadonnées existantes et schéma additif.
- **Critères d'acceptation :** un point d'entrée stable, table complète, tri/multi-sélection, destinations, ponts, réserves, câbles groupés, aperçu avant génération et export déterministe.

### DIST-01 — Signer et fiabiliser l'installateur Windows

- **Preuve :** GitHub #445.
- **Utilisateurs touchés :** entreprises, écoles et postes gérés.
- **Fréquence :** installation/mise à jour.
- **Impact :** blocage SmartScreen et manque de confiance.
- **Effort :** M organisationnel.
- **Risque :** gestion sécurisée de certificat.
- **Dépendances :** CI release et financement du certificat.
- **Compatibilité amont :** coordination amont recommandée.
- **Critères d'acceptation :** binaire et installateur signés, horodatés, signature vérifiée en CI et procédure de rotation documentée.

## P2 — cohérence, accessibilité et productivité

### UI-01 — Simplifier les barres d'outils sans casser les habitudes

- **Preuve :** E01, forte densité iconographique.
- **Utilisateurs touchés :** nouveaux utilisateurs et utilisateurs occasionnels.
- **Fréquence :** permanente.
- **Impact :** découvrabilité et charge cognitive.
- **Effort :** M.
- **Risque :** adoption des utilisateurs experts.
- **Dépendances :** inventaire des actions, télémétrie locale opt-in ou étude terrain.
- **Compatibilité amont :** profils de disposition, mode classique conservé.
- **Critères d'acceptation :** profil Essentiel par défaut, profil Expert disponible, noms/infobulles/raccourcis cohérents, migration non destructive des barres personnalisées.

### UI-02 — Centre d'export et de génération avec aperçu d'impact

- **Preuve :** E08 à E11.
- **État du fork :** premier incrément launcher-only réalisé. Le menu Fichier et
  le profil Essentiel ouvrent un centre commun pour six sorties Documents et
  Données. Les mêmes `QAction` et dialogues historiques restent utilisés ; les
  chemins directs et le profil Classique sont conservés. Compilation Qt 5,
  tests Windows/DPI et inspection visuelle sont documentés dans
  `docs/audit/implementation/ui-02-export-center.md`.
- **Utilisateurs touchés :** tous ceux produisant des livrables.
- **Fréquence :** fin de révision et diffusion.
- **Impact :** prévention d'actions inattendues et reproductibilité.
- **Effort :** M.
- **Risque :** moyen.
- **Dépendances :** export PDF/image/CSV, nomenclature, impression, CLI.
- **Compatibilité amont :** façade commune autour des commandes existantes.
- **Critères d'acceptation du premier incrément :** six sorties regroupées,
  disponibilité synchronisée, navigation clavier, déclenchement différé sûr,
  aucun changement de format ni suppression des commandes existantes.
- **Cible restante :** préréglages, résumé fichiers/folios créés, avertissements,
  annulation globale, composition multi-livrables et équivalence interface/CLI.

### A11Y-01 — Socle d'accessibilité et navigation clavier

- **Preuve :** inspection des arbres accessibles, E03/E07/E08.
- **Utilisateurs touchés :** clavier, basse vision, fatigue motrice, tous les utilisateurs experts.
- **Fréquence :** continue.
- **Impact :** accès et efficacité.
- **Effort :** M puis continu.
- **Risque :** faible.
- **Dépendances :** conventions UI, tests automatisés et manuels.
- **Compatibilité amont :** excellente.
- **Critères d'acceptation :** nom/role/état de chaque contrôle, ordre de tabulation, raccourcis non conflictuels, focus visible, contraste WCAG AA lorsque applicable, aucun contrôle sous modal exposé à l'interaction.

### SAVE-02 — Rendre l'état de sauvegarde explicite

- **Preuve :** état peu saillant dans la stable ; récupération présente dans `master`.
- **Utilisateurs touchés :** tous.
- **Fréquence :** continue.
- **Impact :** confiance et prévention des pertes.
- **Effort :** S.
- **Risque :** faible.
- **Dépendances :** signaux de sauvegarde et d'erreur.
- **Compatibilité amont :** UI uniquement.
- **Critères d'acceptation :** états Modifié/Enregistrement/Sauvegardé/Erreur visibles et annoncés, chemin de récupération expliqué, aucune fausse confirmation.

### PERF-01 — Budget de performance grands projets

- **Preuve :** E04, exemple 150 folios en ~7,5 s sans instrumentation ; UX-02b
  ajoute des budgets d'opérations et des mesures Windows synthétiques 150/500.
- **État du fork :** première passe UX-02b réalisée sur les index de folios,
  onglets, navigateur et positions SQLite. Budgets d'opérations 500/1000 et
  mesures Windows synthétiques documentés dans
  `docs/audit/implementation/ux-02b-large-project-performance.md`. Le parsing,
  l'export, la mémoire et les projets industriels réels restent à profiler.
- **Utilisateurs touchés :** grands projets.
- **Fréquence :** ouverture, recherche, génération.
- **Impact :** productivité.
- **Effort :** M pour instrumentation, variable pour corrections.
- **Risque :** faible.
- **Dépendances :** corpus anonymisé et benchmark reproductible.
- **Compatibilité amont :** aucun format modifié.
- **Critères d'acceptation :** seuils documentés pour 150/500 folios, métriques ouverture/recherche/changement de folio/export, profil avant toute optimisation.

## P3 — fonctions industrielles et modernisation

### IND-02 — Données E/S automate et affectation tabulaire

- **Preuve :** roadmap amont et besoins métiers du fork ; non validé sur projet réel.
- **Utilisateurs touchés :** automaticiens.
- **Effort :** XL.
- **Risque :** élevé.
- **Dépendances :** modèle fabricant, adressage, API d'import/export, édition tabulaire.
- **Compatibilité amont :** extension versionnée/additive indispensable.
- **Critères d'acceptation :** import/export déterministe, validation doublons/plages, génération de folios prévisualisée, historique de changements.

### IND-03 — Désignation IEC 81346 et structures fonction/localisation/produit

- **Preuve :** roadmap amont.
- **Utilisateurs touchés :** ingénierie industrielle normalisée.
- **Effort :** XL.
- **Risque :** très élevé, transversal.
- **Dépendances :** identifiants, cartouches, numérotation, recherche, exports.
- **Compatibilité amont :** ne pas surcharger les champs historiques sans version de schéma.
- **Critères d'acceptation :** règles configurables, migration réversible, validation et conservation exacte des projets non IEC.

### IND-04 — Routage intelligent, bus et ponts de croisement

- **Preuve :** roadmap amont, GitHub #436.
- **Utilisateurs touchés :** dessinateurs de schémas denses.
- **Effort :** XL.
- **Risque :** élevé, géométrie et lisibilité.
- **Dépendances :** moteur de routage, contraintes, Undo, performances.
- **Compatibilité amont :** géométrie stockée dans le format existant si possible.
- **Critères d'acceptation :** aperçu avant application, contraintes manuelles respectées, résultat stable, annulation atomique, benchmark sur schémas denses.

### TECH-01 — Converger vers Qt 6/KF6

- **Preuve :** branches Qt 6 actives, GitHub #402, `master` Qt 5.
- **Utilisateurs touchés :** mainteneurs et distributions.
- **Effort :** XL.
- **Risque :** élevé.
- **Dépendances :** CI multi-OS, dépendances KDE, packaging et tests visuels.
- **Compatibilité amont :** coordonner la branche existante ; éviter un port parallèle concurrent.
- **Critères d'acceptation :** build Windows/Linux/macOS, tests de contrats identiques, ouverture/réenregistrement sans diff sémantique, parité UI et performance au moins équivalente.

### TECH-02 — API d'automatisation et intégrations

- **Preuve :** CLI d'export déjà présente sur `master`.
- **Utilisateurs touchés :** bureaux d'études, CI documentaire, PLM/ERP.
- **Effort :** L à XL.
- **Risque :** surface API à maintenir.
- **Dépendances :** stabilisation CLI, erreurs structurées, schéma de données public.
- **Compatibilité amont :** commencer par versionner la CLI existante.
- **Critères d'acceptation :** codes de sortie, JSON machine-readable, opérations sans interface, documentation versionnée et compatibilité sur deux versions mineures.

## Roadmap en cinq horizons

### H0 — Stabilisation, build et données (0 à 6 semaines)

- DATA-01, DATA-02 et SAVE-01 ;
- DEV-01 à DEV-03 ;
- corpus minimal de fixtures anonymisées ;
- CI Windows reproductible et règle stricte « aucun changement de format non testé ».

**Porte de sortie :** aucune violation d'identité/SQLite connue, tests de compatibilité verts, clone/build documenté.

### H1 — Corrections rapides Windows et ergonomie (4 à 12 semaines)

- UX-01 dialogues responsive ;
- SAVE-02 état de sauvegarde ;
- premières améliorations UX-03 et UX-04 ;
- A11Y-01 sur les écrans touchés ;
- DIST-01 si certificat disponible.

**Porte de sortie :** parcours centraux utilisables à 150 % DPI, recherche et propriétés mesurées en test utilisateur.

### H2 — Refonte progressive des parcours (3 à 8 mois)

- UX-02 navigateur projet ;
- UX-03 recherche complète ;
- UX-05 édition tabulaire ;
- UI-01 profils de barres et UI-02 centre d'export ;
- mesure PERF-01.

**Porte de sortie :** mode classique conservé, temps de tâches de référence réduit sans régression format.

### H3 — Fonctions industrielles avancées (6 à 18 mois)

- IND-01 bornier/câbles stabilisé ;
- IND-02 E/S automate ;
- IND-03 IEC 81346 ;
- IND-04 routage/bus ;
- données fabricant et implantations selon validation métier.

**Porte de sortie :** chaque discipline dispose d'un projet de référence, d'un propriétaire métier et de tests d'export.

### H4 — Qt 6 et intégrations (en parallèle après H0, convergence 9 à 24 mois)

- TECH-01 Qt 6/KF6 ;
- TECH-02 API/CLI ;
- architecture modulaire, diff/merge et collaboration seulement après stabilisation des identités.

**Porte de sortie :** parité fonctionnelle, packaging signé, compatibilité démontrée et procédure de migration réversible.

## Premier lot prêt à développer

Le premier lot ne requiert aucune nouvelle décision produit :

1. écrire les tests de reproduction DATA-01 et DATA-02 ;
2. corriger les deux invariants sans évolution de schéma ;
3. aligner `PACKAGE_TESTS` et la CI ;
4. créer un gabarit de dialogue responsive et migrer Configuration puis Propriétés du projet ;
5. vérifier stable→fork→stable sur les fixtures ;
6. proposer les correctifs minimaux à l'amont pour limiter la divergence.

## Indicateurs de réussite

- zéro perte ou omission dans les tests de duplication et rapports ;
- zéro bouton hors écran à 150 % DPI sur les parcours audités ;
- recherche d'un folio ou élément en moins de trois actions ;
- baisse mesurée du temps de modification groupée ;
- build Windows reproductible depuis une machine propre ;
- taux de tests de contrats exécutés visible en CI ;
- aucune évolution non versionnée des formats `.qet`, `.elmt` et cartouches.
