<p align="center">
  <img src="logo.png" alt="Logo QElectroTech" width="160">
</p>

# QElectroTech — fork ergonomie et productivité

[![Tests Windows Qt 5](https://github.com/GameKnightt/qelectrotech-source-mirror/actions/workflows/windows-build.yml/badge.svg?branch=master)](https://github.com/GameKnightt/qelectrotech-source-mirror/actions/workflows/windows-build.yml)
[![Licence GPL v2](https://img.shields.io/badge/licence-GPL--2.0--only-2f6f44)](LICENSE)
[![État](https://img.shields.io/badge/%C3%A9tat-pr%C3%A9version%20Windows-d97706)](#tester-sous-windows-11)

> Préversion communautaire non officielle, basée sur QElectroTech 0.100.1.
> Elle vise à moderniser progressivement l’expérience de travail sans rompre
> les formats ni les habitudes du projet amont.

QElectroTech est un logiciel libre de CAO/IAO destiné à la création de schémas
électriques, d’automatisme, pneumatiques, hydrauliques et process. Ce fork
conserve ce socle métier et se concentre sur quatre objectifs : réduire les
frictions sous Windows 11, rendre les opérations critiques plus fiables,
accélérer les tâches répétitives et clarifier l’interface.

- [Projet officiel QElectroTech](https://qelectrotech.org/)
- [Sources officielles](https://github.com/qelectrotech/qelectrotech-source-mirror)
- [Forum QElectroTech](https://qelectrotech.org/forum/)
- [Documentation du fork](docs/audit/qet-audit.md)

## Aperçu

### Démarrer depuis un exemple

![Centre de démarrage avec exemples métier](docs/audit/evidence/ui-03b-curated-examples/01-start-center.png)

*Quatre projets publics sont proposés et toujours ouverts comme copies non
enregistrées ; le premier Ctrl+S passe par Enregistrer sous.*

### Espace de travail et projet ouvert

![Projet Arduino ouvert avec la recherche avancée du fork](docs/audit/evidence/ux-05c-table-fill/03-advanced-search.jpg)

*Windows 11, projet public `ArduinoLCD.qet`, espace de travail et recherche avancée.*

### Édition tabulaire des conducteurs

![Éditeur tabulaire des conducteurs avec colonnes configurables](docs/audit/evidence/ux-05d-column-layout/05-persisted-layout.png)

*Windows 11, projet public `industrial.qet`, édition par potentiel et disposition
de colonnes personnalisable et persistante.*

### Borniers et câbles

![Vue consolidée Borniers et câbles](docs/audit/evidence/ind-01a-terminal-cable-overview/01-overview-populated.png)

*Table en lecture seule, recherche sans accent, filtres, tri naturel, contrôle
des informations câble/âme et navigation vers le folio.*

![Catalogue et diagnostics des câbles](docs/audit/evidence/ind-01b-cable-diagnostics/01-catalogue-diagnostics.png)

*Catalogue hiérarchique couvrant tous les folios, diagnostics prudents,
navigation vers le conducteur et export CSV.*

![Éditeur exact des conducteurs du câble C20](docs/audit/evidence/ind-01c/02-exact-conductor-editor.png)

*Sélection exacte depuis le catalogue, modification de la couleur et de la
référence de câble, aperçu avant application et Undo atomique.*

## Ce que le fork ajoute

| Domaine | Changements disponibles |
|---|---|
| Fiabilité | Duplication de folios sans collision d’UUID avec Annuler/Rétablir groupé, écritures d’export atomiques, base projet SQLite reconstruite dans une transaction, état de sauvegarde fiable, copies de récupération vérifiées, historique Undo conservé après sauvegarde |
| Windows et DPI | Dialogues adaptatifs, contrôles clavier et texte à 150 %, build MSYS2/UCRT64 reproductible, préversion portable isolée |
| Démarrage et interface | Centre de démarrage orienté tâche, quatre exemples métier ouverts en copies non enregistrées, profils **Essentiel** et **Classique**, shell contextuel, actions principales hiérarchisées |
| Navigation | Navigation rapide entre folios, prise en charge déterministe des grands projets, recherche des collections séparée de leur exploration |
| Propriétés | Inspecteur contextuel, sélection et édition des conducteurs plus prévisibles |
| Exports | Centre d’export unifié, erreurs visibles, export PDF/PNG/SVG et données métier protégés contre les faux succès |
| Conducteurs | Aperçu avant application, édition tabulaire groupée, collage TSV, recopie vers le bas, colonnes configurables et export CSV de revue |
| Borniers et câbles | Point d’entrée stabilisé, vue consolidée des bornes, catalogue câble → conducteurs, diagnostics, recherche, filtres, navigation, export CSV et édition exacte multi-sélection protégée par aperçu/Undo |

Le détail technique, les critères d’acceptation et les limites de chaque lot sont
réunis dans l’[index des implémentations](docs/audit/implementation/README.md).

## Tester sous Windows 11

La préversion Windows est portable, non signée et n’écrase pas l’installation
officielle. Utilisez d’abord une copie de vos projets importants.

1. Téléchargez la dernière archive de préversion depuis les
   [Releases du fork](https://github.com/GameKnightt/qelectrotech-source-mirror/releases).
2. Extrayez l’archive dans un chemin local court.
3. Fermez les autres instances de QElectroTech.
4. Double-cliquez sur `Launch-QElectroTech-Preview.bat`.
5. Ouvrez une **copie** d’un projet existant.

Le lanceur utilise le dossier local `conf/` pour séparer les préférences de la
préversion. Les collections, cartouches, traductions, plugins Qt et dépendances
Windows sont inclus dans le paquet.

### Contrôle rapide en cinq minutes

1. Vérifiez le centre de démarrage puis ouvrez **Arduino et écran LCD** : le
   projet doit être indiqué **Modifié** et `Ctrl+S` doit ouvrir Enregistrer sous.
2. Passez entre les profils **Essentiel** et **Classique**.
3. Naviguez entre les folios et recherchez un composant.
4. Ouvrez **Projet > Borniers et câbles…**, vérifiez les onglets **Bornes** et
   **Câbles**, sélectionnez un câble et utilisez **Modifier les conducteurs…**
   (`Alt+M`) ; le brouillon ne doit contenir que la sélection explicite.
5. Ouvrez le centre d’export.
6. Utilisez la recherche avancée puis **Modifier les conducteurs en tableau…**.
7. Modifiez une copie du projet : le statut doit passer à **Modifié**.
8. Enregistrez avec `Ctrl+S` : le statut ne doit revenir à **Sauvegardé**
   qu’après la fin réelle de l’écriture.
9. Vérifiez que Undo/Redo reste disponible après la sauvegarde.
10. Dans l’arbre projet, utilisez **Dupliquer le folio**, puis
    utilisez Annuler et Rétablir : le folio et son onglet doivent disparaître
    et réapparaître ensemble, sans composant manquant dans la nomenclature.

Consultez le [guide de la préversion portable](docs/development/windows-portable-preview.md)
pour le packaging et les contrôles de manifeste.

## Compatibilité et niveau de confiance

Les incréments actuels ne modifient pas les contrats de fichiers existants :

- projets `.qet` ;
- éléments `.elmt` ;
- cartouches XML ;
- collections et traductions ;
- base SQLite interne et exports existants.

Le profil **Classique** et les actions métier historiques restent disponibles.
Les changements de données sont protégés par Undo lorsque le parcours le permet.

Validation de cette préversion :

- compilation de l’application complète sous Windows 11 / Qt 5 / UCRT64 ;
- **48/48 tests CTest** réussis en série ;
- stress DATA-01 : 100 duplications de deux folios liés, 200 UUID nouveaux et
  aucune collision sur les clés SQLite `element` / `element_info` ;
- **13/13** contrats IND-01C, dont la portée exacte, la multi-sélection et le
  brouillon Câble/Couleur ;
- **17/17** parcours clavier Windows natifs et **25/25** contrôles natifs à
  150 % réussis ;
- ouverture CLI d’un projet public de 50 folios : 618 éléments et 671
  conducteurs détectés ;
- parcours graphique réel : ouverture, modification, enregistrement et retour à
  **Sauvegardé** ;
- paquet portable contrôlé par manifeste SHA‑256.

### Limites actuelles

- préversion Windows non signée ;
- qualification principale sous Windows 11 et Qt 5 ;
- parité Linux, macOS et Qt 6 non revendiquée pour cette distribution ;
- inventaire final des licences tierces, signature et test sur machine Windows
  propre encore requis avant une publication stable ;
- le catalogue utilise les champs historiques libres de QElectroTech : ses
  diagnostics assistent la revue sans certifier la conformité électrique ;
- les réserves et destinations structurées de câbles, les E/S automate, le
  pneumatique, l’hydraulique et le process exigent encore des projets
  anonymisés représentatifs ;
- thème sombre, contraste élevé et certains scénarios à 200 % restent à
  compléter.

## Documentation

| Document | Contenu |
|---|---|
| [Audit complet](docs/audit/qet-audit.md) | Architecture, parcours, forces, risques et limites observées |
| [Backlog et roadmap](docs/audit/backlog-roadmap.md) | Priorités P0 à P3, critères d’acceptation et horizons produit |
| [Registre des preuves](docs/audit/evidence/README.md) | Captures, tests et résultats des validations |
| [Implémentations](docs/audit/implementation/README.md) | Résultat et contrat de chaque lot livré |
| [Notes de préversion](docs/releases/preview-2026-07.md) | Changements, validation, installation et limites de la préversion Windows |
| [Build Windows 11](docs/development/windows-msys2-build.md) | Compilation Qt 5 avec MSYS2 UCRT64 |
| [Paquet portable](docs/development/windows-portable-preview.md) | Déploiement, isolation et manifeste SHA‑256 |

## Roadmap résumée

- **Maintenant :** stabiliser l’intégration Windows, le paquet portable, les
  licences et les tests de compatibilité de formats.
- **Ensuite :** poursuivre la refonte progressive des parcours, les modèles
  paramétrables, les propriétés et les exports.
- **Fonctions industrielles :** l’édition exacte des conducteurs de câble avec
  aperçu/Undo est livrée ; poursuivre avec réserves et destinations structurées,
  puis E/S automate, désignation IEC 81346, routage intelligent et bus.
- **Architecture :** convergence Qt 6/KF6, automatisation CLI/API et
  intégrations externes.

La roadmap détaillée reste la source de vérité pour les priorités, dépendances
et critères d’acceptation : [backlog-roadmap.md](docs/audit/backlog-roadmap.md).

## Compiler et contribuer

Pour éviter le téléchargement Git LFS de la documentation Qt Creator, qui n’est
pas nécessaire au build, le clone Windows recommandé est :

```bash
GIT_LFS_SKIP_SMUDGE=1 git clone --recursive \
  https://github.com/GameKnightt/qelectrotech-source-mirror.git
cd qelectrotech-source-mirror
git remote add upstream \
  https://github.com/qelectrotech/qelectrotech-source-mirror.git
```

Le guide [Windows MSYS2/UCRT64](docs/development/windows-msys2-build.md) contient
les dépendances, les options CMake et les commandes de test. Les contributions
doivent rester découpées en incréments vérifiables, préserver les formats et
ajouter une preuve Windows pour tout changement d’interface.

Consultez également [CONTRIBUTING.md](CONTRIBUTING.md).

## Projet amont, attribution et licence

Ce dépôt est un fork de QElectroTech, projet développé depuis 2007 par l’équipe
et la communauté QElectroTech. Il ne constitue pas une distribution officielle
et ses modifications spécifiques ne sont pas nécessairement prises en charge
par l’équipe amont.

QElectroTech et les modifications de ce fork sont distribués selon les termes
de la GNU General Public License version 2. Consultez [LICENSE](LICENSE). Les
sous-modules, dépendances et collections conservent leurs propres avis de droit
d’auteur et de licence.

- [Site officiel](https://qelectrotech.org/)
- [Sources officielles](https://github.com/qelectrotech/qelectrotech-source-mirror)
- [Wiki officiel](https://qelectrotech.org/wiki_new/)
- [Soutenir le projet QElectroTech amont](https://www.paypal.com/donate/?cmd=_s-xclick&hosted_button_id=ZZHC9D7C3MDPC)
