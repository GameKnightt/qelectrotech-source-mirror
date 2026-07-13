# UI-02 — Centre d'export unifié

## Objectif et contrat

UI-02 ajoute une porte d'entrée unique vers les sorties de fichiers déjà
disponibles dans QElectroTech. Ce premier incrément est strictement
**launcher-only** : le centre présente les possibilités, leur disponibilité et
leur résultat attendu, puis déclenche la `QAction` historique choisie. Il ne
réimplémente aucun moteur d'export, ne modifie aucun format et ne remplace aucun
parcours existant.

Le centre est disponible lorsqu'un projet est ouvert. Les livrables conservent
individuellement les règles d'activation de leurs actions d'origine : certains
nécessitent un folio courant, d'autres seulement un projet ouvert ou modifiable.

## Périmètre du premier incrément

Six `QAction` existantes sont réutilisées :

| Groupe | Livrable | Action historique | Parcours réutilisé |
| --- | --- | --- | --- |
| Documents | Images, SVG ou DXF | `m_export_to_images` | `ProjectView::exportProject()` puis `ExportDialog` |
| Documents | Document PDF | `m_export_to_pdf` | `ProjectPrintWindow` en mode PDF |
| Documents | Impression | `m_print` | `ProjectPrintWindow` en mode imprimante |
| Données | Nomenclature CSV | `m_csv_export` | `BOMExportDialog` |
| Données | Noms de conducteurs CSV | `m_project_export_conductor_num` | `ConductorNumExport::toCsv()` |
| Données | Plan de câblage CSV | `m_project_export_wiring_list` | `WiringListExport::toCsv()` |

L'impression ne crée pas nécessairement un fichier, mais elle partage le même
parcours de sélection des folios et de mise en page que le PDF ; elle reste donc
présentée avec les autres sorties de documents.

## Intégration non destructive

`QETDiagramEditor` expose une nouvelle action **Centre d'export…** dans le menu
Fichier et dans la barre principale du profil Essentiel. Le profil Classique
conserve sa composition historique. Les commandes directes restent présentes
dans les menus Fichier et Projet, avec leurs raccourcis, libellés et règles
d'activation inchangés.

`ExportCenterDialog` reçoit des entrées contenant un `QPointer<QAction>`. Il
observe le signal `QAction::changed` afin d'actualiser l'état Disponible ou
Indisponible, sans prendre possession des actions. Lorsqu'une commande est
indisponible, le panneau explique la précondition attendue (folio courant,
projet ouvert ou projet modifiable). Le dialogue ne produit aucun fichier et ne
modifie aucun projet.

Après validation, le dialogue est détruit avant le lancement du parcours
historique. `triggerActionDeferred()` diffère le `trigger()` au tour suivant de
la boucle d'événements et revérifie le `QPointer` ainsi que l'état activé de
l'action. Cette séquence évite d'empiler un second dialogue modal sur le centre
et protège contre la destruction ou la désactivation tardive de l'action.

## Accessibilité, clavier et DPI

Le centre utilise exclusivement des widgets Qt standards : arbre de livrables,
libellés, séparateur redimensionnable et boutons de dialogue. Les commandes
indisponibles restent visibles avec un état textuel ; la couleur seule ne porte
aucune information. Le premier livrable disponible est sélectionné à
l'ouverture, `Entrée` active la sélection et le bouton Fermer reste accessible
au clavier.

Les noms d'objet stables permettent les tests automatisés. Les icônes utilisent
les métriques du style Qt, les descriptions passent à la ligne et la fenêtre est
redimensionnable. La taille initiale est bornée à l'espace disponible sur l'écran
du parent. Aucun pixel fixe n'est utilisé pour les icônes ; la géométrie reste à
contrôler sous Windows 11 aux facteurs DPI usuels.

## Compatibilité

Le lot ne change aucun format `.qet`, `.elmt`, XML, SQLite, CSV, PDF, image ou
DXF. Il n'ajoute aucune clé `QSettings`. Les mêmes instances de `QAction` et les
mêmes backends sont utilisées depuis le centre et depuis les menus historiques.
Les API employées (`QDialog`, `QPointer`, `QAction`, `QTreeWidget` et
`QTimer::singleShot`) sont communes à Qt 5 et Qt 6.

## Exclusions du premier incrément

- génération de nomenclatures ou sommaires graphiques dans le projet ;
- export conditionnel de la base SQLite interne ;
- lancement direct ou configuration de la CLI headless ;
- file d'attente multi-livrables, export parallèle ou transaction globale ;
- préréglages nommés, historique des destinations et profils d'entreprise ;
- progression, annulation globale et rapport consolidé de plusieurs exports ;
- suppression, déplacement ou changement de raccourci des commandes existantes ;
- refonte des dialogues et moteurs d'export historiques.

## Incréments futurs

1. Ajouter des préréglages de destination et d'options par type de livrable.
2. Extraire des services communs réentrants pour rapprocher GUI et CLI sans
   exécuter un second processus.
3. Composer plusieurs livrables avec prévalidation, progression, annulation et
   résumé final vérifiable.
4. Ajouter les générations qui modifient le projet avec avertissement explicite,
   intégration Undo et confirmation séparée.
5. Étudier l'export de la base interne seulement si son contrat devient public
   et disponible dans toutes les configurations prises en charge.

## Validation

- [x] Compilation Debug complète de `qelectrotech.exe` avec Qt 5.15.19 sous
  Windows 11 et démarrage `0.100.1-dev`.
- [x] Revue statique des API utilisées pour la trajectoire Qt 6, sans faire de
  la compilation Qt 6 un prérequis de ce lot.
- [x] Test du nombre, de l'ordre, du regroupement et de la disponibilité de six
  entrées, y compris sections vides et actions détruites.
- [x] Test du déclenchement différé et des gardes action désactivée, détruite ou
  nulle, y compris la destruction du centre avant le déclenchement et l'absence
  de second dialogue modal empilé.
- [x] Test UI-01 confirmant Centre d'export dans Essentiel et la composition
  Classique inchangée après cent bascules.
- [x] Parcours clavier, focus initial, commandes indisponibles sélectionnables et
  expliquées, et bouton Continuer inaccessible lorsque l'action ne peut pas être
  lancée.
- [x] Noms et descriptions accessibles, libellés d'état indépendants de la
  couleur, zone de détails défilable et géométrie compacte testée à 100 % et
  150 %.
- [x] Inspection visuelle du vrai binaire Windows en 1920×1080 à 100 % : menu
  Fichier, six livrables, changement de sélection au clavier, détails et boutons.
- [x] Suite CTest complète : 13/13 en plate-forme Qt déterministe ; test UI-02
  également vert avec la plate-forme Windows native à 100 % et 150 %.
- [x] Aucun moteur n'est appelé et aucun fichier ou projet n'est modifié avant
  validation ; le test vérifie que la `QAction` exacte est seulement retournée.
- [ ] Vérification manuelle des six dialogues historiques jusqu'à leur étape
  d'annulation, sans créer de livrable.
- [ ] Inspection visuelle à 200 %, contraste en thème sombre et lecteur d'écran
  réel. Ces contrôles restent des limites explicites, pas une certification.

Sur la machine locale multi-écran, l'ancien `config_dialog_ux_test` dépend des
contraintes natives de l'écran secondaire et échoue hors de son environnement
déterministe. Il passe avec la plate-forme Qt hors écran ; `export_center_test`
passe dans les deux environnements. La CI Windows exécute la suite complète sur
la plate-forme déterministe puis `export_center_test` avec le backend graphique
Windows natif à 100 % et 150 %. Elle constitue l'autorité distante pour ce
contrôle UI ciblé, sans étendre cette affirmation à toute la suite native.
