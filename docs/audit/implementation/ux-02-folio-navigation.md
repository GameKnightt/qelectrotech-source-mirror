# UX-02 — Navigation rapide entre les folios

## Résultat livré

UX-02 ajoute un navigateur de folios accessible depuis le menu **Projet** ou
avec `Ctrl+G`. Il permet de retrouver et d'ouvrir un folio sans parcourir une
longue rangée d'onglets.

Le navigateur propose :

- une recherche instantanée par position, numéro, titre, installation,
  localisation, nom de fichier et champs supplémentaires du cartouche ;
- un classement déterministe donnant la priorité aux correspondances exactes ;
- des groupes virtuels `installation / localisation` sans modifier le projet ;
- des vues **Tous les folios**, **Favoris** et **Récents** ;
- un historique arrière/avant avec `Alt+Gauche` et `Alt+Droite` ;
- une activation explicite par Entrée ou par le bouton d'ouverture ;
- des libellés accessibles qui indiquent la position, le groupe, l'état actif
  et l'état favori sans dépendre uniquement de la couleur.

## Compatibilité

Aucun format de fichier, attribut XML, paramètre utilisateur ou export n'est
modifié. Les identifiants des folios étant internes à une session dans la base
actuelle, les favoris restent volontairement limités à la session. Les rendre
persistants nécessiterait d'abord un contrat d'identité stable et compatible
avec les anciens `.qet` ; ce choix est reporté à une évolution séparée.

Le chemin d'activation réutilise `ProjectView::showDiagram` et la sélection
d'onglet existante. Le chargement d'un projet reste compatible avec la phase où
les onglets Qt existent avant la reconstruction de la table interne des folios.

## Validation automatisée

Le test `folio_navigator_test` couvre :

- normalisation Unicode, accents, casse et espaces ;
- classement des numéros et titres, métadonnées multi-termes et doublons ;
- groupes, favoris et ordre des récents ;
- identité stable de la sélection et signalement du folio actif ;
- navigation clavier, Entrée, Échap et absence d'activation sans résultat ;
- noms accessibles et dimensions compatibles avec l'écran disponible ;
- filtrage de 500 folios pré-indexés sous un seuil volontairement large de
  1 000 ms.

Un second passage CTest exécute le contrôle de dimensions avec une police
agrandie à 150 % et vérifie la cible logique 1280×720 correspondant à un écran
1920×1080 réglé à 150 %. Ce stress de mise en page reste déterministe sur le
rendu `offscreen` de la CI Windows, qui ne fournit pas de bureau interactif. La
validation de l'application native complète le contrôle automatisé.

Résultat local Windows 11, Qt 5 / MSYS2 : **5 tests sur 5 réussis**, dont les
deux variantes du navigateur. L'exécutable complet `qelectrotech.exe` est
également compilé et lié avec succès.

## Validation dans l'application Windows 11

Projet public utilisé : `examples/industrial.qet`, 50 folios.

Parcours vérifiés dans le binaire de cette branche :

1. ouverture du projet et création visible des 50 onglets ;
2. ouverture du navigateur avec `Ctrl+G` ;
3. recherche `A5/2`, réduite au folio 32 `A5/2 Ana Input Module` ;
4. ajout du folio aux favoris et filtre **Favoris** réduit à une entrée ;
5. filtre **Récents** contenant les folios 32 et 1 dans le bon ordre ;
6. ouverture du folio 32 par Entrée depuis le champ de recherche ;
7. retour au folio 1 avec `Alt+Gauche`, puis avance au folio 32 avec
   `Alt+Droite` ;
8. fermeture du sélecteur par Échap, sans activation parasite.

Cette validation a détecté puis permis de corriger une régression du premier
prototype : pendant le chargement, `tabChanged` consultait la table des folios
avant sa reconstruction et affichait à tort l'état « projet vide ». La version
finale résout directement la vue depuis l'onglet Qt actif pendant cette phase.

## Suite recommandée

UX-02b traite séparément le coût des index, onglets, groupes et positions
SQLite sur les très grands projets. Les résultats, budgets 500/1000 et mesures
Windows sont consignés dans
`docs/audit/implementation/ux-02b-large-project-performance.md`. Le chargement
reste entièrement anticipé : le parsing et la création différée des
`DiagramView` constituent le chantier de performance suivant.
