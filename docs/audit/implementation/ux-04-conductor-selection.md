# UX-04 — Sélection et propriétés des conducteurs

## Périmètre livré

Cet incrément répond au besoin décrit dans
[l’issue amont #500](https://github.com/qelectrotech/qelectrotech-source-mirror/issues/500),
tout en couvrant les lacunes laissées hors du prototype
[#502](https://github.com/qelectrotech/qelectrotech-source-mirror/pull/502) :
multi-sélection et tolérance de pointage indépendante du zoom.

- cible interactive minimale de 10 pixels logiques à tous les zooms ;
- surbrillance bleue stable au survol, distincte de la géométrie du projet ;
- priorité conservée aux bornes, poignées, textes et autres objets exactement visés ;
- détection de proximité suspendue pendant le placement, le routage, le
  déplacement de la vue et la sélection libre ;
- panneau persistant compact pour un ou plusieurs conducteurs ;
- affichage explicite des valeurs mixtes ;
- modification limitée au champ réellement touché ;
- une transaction Undo atomique pour toute la sélection ;
- restauration des valeurs initiales propres à chaque conducteur ;
- synchronisation du panneau après une modification externe ou un Undo ;
- dialogue historique conservé derrière « Propriétés avancées… ».

## Contrats préservés

Aucun attribut XML, format `.qet`, comportement d’enregistrement ou API de
collection n’est modifié. La sélection élargie n’altère ni le chemin, ni les
segments, ni les profils de routage. La portée par défaut est strictement la
sélection courante : aucune propagation silencieuse au potentiel entier.

## Validation automatisée

La cible `conductor_interaction_test` vérifie la même zone de pointage à 25 %,
50 %, 100 %, 200 % et 400 %, les transformations avec translation et les
conducteurs dont l’épaisseur visuelle dépasse la cible minimale. Elle est
ajoutée à la CI Windows Qt 5.

La compilation complète Qt 5 réussit avec MinGW/UCRT64 sous Windows 11. Les
quatre suites CTest sont vertes : `C_unittests`, `qt_unittests`,
`diagram_duplicate_uuid_remapper_test` et `conductor_interaction_test`.

Une validation dans l’exécutable construit, sur le projet d’exemple
`ArduinoLCD.qet`, confirme les points suivants :

- un clic placé environ 4 pixels sous un trait fin sélectionne le conducteur ;
- le panneau persistant s’affiche sans dialogue modal et reste lisible dans une
  fenêtre de 1269 × 840 pixels ;
- une formule saisie dans le panneau est appliquée immédiatement et son texte
  résolu est synchronisé ;
- `Ctrl+Z`, après retour du focus au schéma, restaure la valeur initiale et
  l’état non modifié du projet.

Le pilote d’automatisation Windows utilisé ne maintient pas `Ctrl` pendant un
clic souris : la multi-sélection par clic doit donc encore être rejouée
manuellement, ainsi que le mode lecture seule, le placement d’un élément et le
déplacement d’une poignée de routage. La logique de cible indépendante du zoom
reste couverte automatiquement de 25 % à 400 %.

## Choix reporté

« Appliquer à tout le potentiel » reste une action avancée du dialogue modal.
Son ajout au panneau devra rendre explicites la portée inter-folios et le
nombre de conducteurs affectés avant validation.
