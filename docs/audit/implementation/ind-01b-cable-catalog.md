# IND-01B — Catalogue et diagnostics des câbles

## Résultat utilisateur

Le gestionnaire **Projet > Borniers et câbles…** comporte désormais deux
onglets Qt natifs : **Bornes**, qui conserve la vue IND-01A, et **Câbles**, qui
analyse tous les conducteurs de tous les folios du projet.

Le nouvel onglet fournit :

- une arborescence câble → conducteurs ;
- une recherche insensible aux majuscules et aux accents ;
- des filtres par état et type de diagnostic ;
- un accès optionnel aux conducteurs sans référence de câble ;
- une navigation vers le conducteur source dans son folio ;
- un export CSV UTF-8 du catalogue et de ses diagnostics ;
- des états vides, indisponibles et sans résultat distincts.

Le parcours reste entièrement consultatif. L’ouverture du catalogue, les
filtres, le tri, la navigation et l’export ne modifient ni le projet ni sa pile
Undo.

## Modèle et compatibilité

QElectroTech ne possède pas encore d’objet `Cable` persistant. La référence de
câble, l’âme ou couleur, la section, la fonction et la tension/protocole sont
déjà portées par `ConductorProperties`. IND-01B les lit directement depuis
chaque `Diagram::conductors()` et ne touche à aucun sérialiseur.

Les formats `.qet`, `.elmt`, les cartouches, collections, traductions et
préférences restent donc inchangés. La clé d’un conducteur combine l’UUID du
projet, l’UUID du folio et la paire canonique de ses deux extrémités. La cible
interactive est en plus résolue par un jeton de session et un `QPointer` : une
cible supprimée ou provenant d’un autre projet échoue sans navigation
arbitraire.

## Diagnostics prudents

Les contrôles sont conçus comme des aides à la revue, pas comme une
certification électrique :

- **Âme / couleur non renseignée** : une référence de câble existe mais le
  champ correspondant est vide ;
- **Référence d’âme / couleur réutilisée** : la même valeur apparaît sur des
  potentiels disjoints du même câble ; c’est un avertissement, car une couleur
  comme `BK` peut être légitime plusieurs fois ;
- **Libellé de câble proche** : deux références distinctes ne diffèrent que par
  la casse, les accents ou les espaces normalisés ; elles restent séparées ;
- **Extrémité ou identité ancienne** : la navigation structurelle ne peut pas
  être entièrement stabilisée ;
- **Sections ou protocoles multiples** : information de cohérence, jamais
  erreur automatique.

Un conducteur coloré sans câble est valide et n’est pas signalé. Plusieurs
segments d’un même potentiel ramifié ne créent pas de faux doublon. IND-01B
corrige aussi le faux positif historique de la vue Bornes : une couleur seule
n’est plus assimilée à un câble incomplet.

## Architecture

- `sources/cablecatalog/` contient les snapshots, le builder déterministe, le
  loader du modèle vivant et l’exporteur CSV ;
- `CableCatalogModel` expose une hiérarchie en lecture seule et des rôles
  stables pour le tri, les filtres, l’accessibilité et la navigation ;
- `CableCatalogWidget` réutilise les métriques, cadres, palette et interactions
  du gestionnaire existant ;
- `TerminalStripEditorWindow` recharge les deux onglets ensemble et résout les
  cibles de navigation au dernier moment.
- le loader vérifie désormais le type C++ réel des éléments de borne. Les
  sous-classes d'`Element` partageant le même identifiant `QGraphicsItem`, un
  `qgraphicsitem_cast` pouvait auparavant réinterpréter un symbole quelconque
  comme une borne et provoquer un crash à l'ouverture d'un projet complet.

## Validation

La cible `cable_catalog_test` couvre notamment :

- 13 conducteurs de référence, six câbles et deux conducteurs non affectés ;
- tri naturel `C2` avant `C10` ;
- câble incomplet, réutilisation à vérifier et libellés `C1` / `c1` proches ;
- contre-exemple d’un potentiel ramifié sans faux doublon ;
- modèle hiérarchique strictement non éditable ;
- recherche, filtres combinés, clavier, navigation et états vides ;
- export CSV déterministe ;
- 10 000 conducteurs avec résultat stable après permutation ;
- fenêtre logique 1280 × 680 avec police à 150 % ;
- rendu PNG déterministe du composant compilé.

La cible et ses variantes clavier, gros volume et grande police sont ajoutées à
la CI Windows Qt 5.

Le parcours réel sous Windows 11 a aussi été exécuté sur un projet public de
50 folios : ouverture sans crash, détection de `C1`, `c1`, `C2`, `C10` et
`C20`, recherche `C20`, remise à zéro par Échap et navigation par Entrée vers
le conducteur sélectionné. La matrice finale réussit 47/47 tests, 12/12 tests
grande police hors écran, 16/16 contrats natifs et 24/24 contrats natifs à
150 %.

## Limites et suite IND-01C

Le champ historique `m_wire_color` peut représenter une couleur ou un repère
d’âme ; aucune unicité absolue n’est donc imposée. Les renvois, épissures et
câbles hybrides exigent encore des projets métier anonymisés pour affiner les
règles.

IND-01C pourra ajouter la correction groupée avec aperçu et Undo atomique. Elle
devra modifier uniquement les conducteurs explicitement sélectionnés : propager
une référence de câble à tout un potentiel électrique pourrait traverser
plusieurs câbles physiques.
