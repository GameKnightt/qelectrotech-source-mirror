# SAVE-02 — État de sauvegarde fiable

## Résultat utilisateur

La barre d’état de l’éditeur indique maintenant, pour le projet actif,
**Modifié**, **Enregistrement…**, **Sauvegardé** ou
**Erreur d’enregistrement**. Le statut reste indépendant des messages
temporaires de QElectroTech et suit correctement le projet lorsque plusieurs
fenêtres sont ouvertes.

Le libellé n’annonce jamais un succès avant la fin de l’écriture canonique. Une
erreur conserve l’état non enregistré et expose son détail dans l’infobulle et
la description accessible. Un projet sans chemin reste **Modifié**, même si son
drapeau interne est encore propre.

## Contrat de vérité

`ProjectSaveStatusController` conserve un état par projet et publie uniquement
l’instantané du projet actif. Chaque sauvegarde canonique porte un identifiant
d’opération et la révision du projet au démarrage :

- une fin d’opération obsolète est ignorée ;
- une modification intervenue pendant l’écriture interdit un faux
  **Sauvegardé** ;
- un échec reste associé au bon projet, y compris lorsqu’il est inactif ;
- un nouvel essai réussi efface l’erreur précédente ;
- les sauvegardes manuelles, automatiques et déclenchées à la fermeture
  traversent le même cycle canonique.

La copie de récupération est suivie séparément de la sauvegarde canonique. Le
texte **Copie de récupération disponible** n’apparaît qu’après le résultat
positif de `QET::writeToFile`, avec le chemin effectivement produit. Un échec
est annoncé sans masquer l’état canonique et une modification de chemin invalide
l’ancienne preuve de récupération.

## Fiabilisation du moteur

Le lot corrige plusieurs divergences découvertes pendant l’audit du cycle réel :

- l’autosauvegarde et l’écriture de fermeture ne contournent plus les signaux de
  début et de fin ;
- l’autosauvegarde n’écrit que lorsqu’un chemin existe et que le projet contient
  réellement des changements ;
- une écriture réussie utilise `QUndoStack::setClean()` au lieu d’effacer
  l’historique Undo ;
- la sauvegarde de récupération asynchrone restitue son résultat au thread UI ;
- la branche Qt 6 exécute désormais la même écriture de récupération que Qt 5,
  au lieu de rester vide ;
- aucun `processEvents()` réentrant n’est nécessaire pour rendre
  **Enregistrement…** visible ; seul le composant de statut est repeint ;
- sous Windows, un dialogue modal de chargement peut temporairement effacer la
  sous-fenêtre MDI active. L’éditeur conserve donc le dernier `ProjectView`
  valide et utilise aussi `currentSubWindow()` pour que **Enregistrer** et le
  statut restent rattachés au bon projet.

## Ergonomie et accessibilité

- icônes standard du style Qt et taille `PM_SmallIconSize` ;
- composant permanent à droite de la barre d’état, sans nouveau thème ;
- aucune prise de focus clavier par le composant ou ses libellés ;
- nom accessible de la forme
  `État de sauvegarde : <état> — projet « <nom> »` ;
- description accessible contenant le chemin, l’erreur et la récupération
  vérifiée lorsqu’ils existent ;
- événements d’accessibilité `NameChanged` et `DescriptionChanged` ;
- libellés courts restant dans la barre d’état à 1 280 × 680 logiques avec texte
  à 150 %.

## Compatibilité

Les méthodes publiques `QETProject::write()` et `write(const QString &)` sont
conservées. Le lot ajoute des signaux de cycle et un contrôleur de présentation,
mais ne modifie ni `.qet`, ni `.elmt`, ni XML, ni SQLite, ni les collections,
les cartouches ou les exports. Une sauvegarde réussie conserve maintenant
l’historique Undo antérieur tout en marquant sa position propre.

## Validation automatisée

La cible `project_save_status_test` couvre le composant et le contrôleur. Cinq
alias CTest rendent les contrats visibles dans la CI :

- `project_save_status_state_machine_test` ;
- `project_save_status_multi_project_test` ;
- `project_save_status_recovery_test` ;
- `project_save_status_keyboard_test` ;
- `project_save_status_large_text_150_test`.

Les scénarios incluent succès, échec puis nouvel essai, fin obsolète,
modification pendant l’écriture, projet sans chemin, deux projets indépendants,
récupération réussie/échouée/invalidée, focus clavier, interface accessible et
grande police. Le workflow Windows exécute explicitement les variantes clavier
et 150 %.

Validation finale du 14 juillet 2026 :

- compilation des 18 cibles Windows prévues et de `qelectrotech.exe` : réussite ;
- suite complète en série : `35/35` tests réussis ;
- plateforme Windows native : `1/1` test clavier réussi ;
- plateforme Windows native à 150 % : `2/2` tests réussis ;
- `git diff --check` : réussite ; la collision historique de changelogs est
  désormais traitée par DEV-01.

## Validation Windows 11 réelle

La build Qt 5 a été copiée dans un déploiement portable existant, dépouillée de
ses symboles et lancée sous le nom de processus isolé
`qelectrotech-save02.exe`. Cette isolation évite que le mécanisme d’instance
unique redirige le test vers l’installation officielle de QElectroTech.

Le projet public `examples/industrial.qet` a été copié dans le dossier de
validation afin de ne jamais écrire dans la fixture du dépôt. Dans une fenêtre
de 1 269 × 840 pixels, le parcours a confirmé :

1. ouverture du projet et statut accessible **Sauvegardé** ;
2. conservation de la commande **Enregistrer** après les dialogues modaux de
   chargement et de copie facultative ;
3. ajout d’un folio sur la copie et passage à **Modifié** ;
4. clic sur **Enregistrer**, écriture du fichier de test et retour à
   **Sauvegardé** ;
5. fermeture sans demande de sauvegarde résiduelle.
Le premier parcours réel a révélé puis permis de corriger la perte de contexte
MDI décrite plus haut. Les états **Enregistrement…** et **Erreur** sont couverts
de façon déterministe par le contrôleur : l’écriture locale de 2,6 Mo est trop
brève pour garantir une capture intermédiaire, et aucune panne destructive n’a
été provoquée dans l’application complète.
