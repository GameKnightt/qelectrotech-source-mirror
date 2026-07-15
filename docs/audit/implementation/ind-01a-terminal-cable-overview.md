# IND-01A — Vue consolidée Borniers et câbles

## Résultat utilisateur

Le menu **Projet > Borniers et câbles…** ouvre désormais un espace de travail
nommé et directement utile, à la place de la grande page centrale vide de
l’ancien gestionnaire en développement. La page d’accueil du gestionnaire
regroupe les bornes affectées et indépendantes dans un tableau en lecture seule.

Chaque ligne présente l’état, le bornier, l’installation, la localisation, la
position, l’étage, le repère, les conducteurs, la référence croisée, les câbles,
les âmes ou couleurs, le type, la fonction et la présence d’un pont. La recherche
est insensible aux accents et à la casse. Deux filtres isolent l’affectation et
les points à vérifier. Le tri naturel place notamment `X2` avant `X10`.

Une ligne peut être ouverte dans son folio avec **Entrée**, un double-clic ou le
bouton **Afficher dans le folio**. `Ctrl+F` atteint la recherche, Flèche bas
rejoint le tableau, Échap efface progressivement la recherche puis les filtres,
et `F5` recharge les données depuis le projet.

## Contrat de données et compatibilité

IND-01A n’introduit aucun objet « câble » persistant et ne change aucun format.
La vue prend un instantané des objets métier existants sur le thread graphique :

- les borniers, positions et étages proviennent de `TerminalStrip`,
  `PhysicalTerminal` et `RealTerminal` ;
- les bornes indépendantes proviennent de `ElementProvider::freeTerminal()` ;
- les numéros de conducteur, câbles et âmes/couleurs proviennent des
  `ConductorProperties` déjà enregistrées dans le projet ;
- plusieurs conducteurs raccordés à une borne sont dédupliqués, triés
  naturellement puis affichés ensemble ;
- le tableau ne conserve ni pointeur métier ni droit d’écriture ; la navigation
  résout à nouveau l’UUID de l’élément dans le projet actif.

Le correctif des accesseurs historiques `RealTerminal::cable()` et
`RealTerminal::cableWire()` permet aussi à l’ancien éditeur de bornier de lire
le premier conducteur raccordé au lieu de présenter systématiquement des
cellules vides. La vue d’ensemble reste la source d’affichage complète lorsque
plusieurs conducteurs sont raccordés.

Les contrats `.qet`, `.elmt`, cartouches XML, collections, SQLite et exports
restent inchangés. Le projet en lecture seule peut ouvrir la vue ; les commandes
de création, suppression et application du gestionnaire restent désactivées.

## États, accessibilité et prévention des erreurs

- états distincts pour projet indisponible, projet sans borne et filtres sans
  résultat ;
- signalement prudent d’une source disparue ou d’un couple câble/âme incomplet ;
- aucune prétention de validation électrique ou de qualification du câble ;
- sélection stable après tri, filtre et rechargement ;
- libellés accessibles sur la recherche, les filtres, le tableau et les actions ;
- fenêtre bornée à l’écran, minimum 640 × 360, contrôlée à 1280 × 680 pixels
  logiques avec police à 150 % ;
- lecture seule annoncée dans un bandeau et absence d’édition dans le modèle.

## Validation du 15 juillet 2026

- application complète compilée et liée sous Windows 11, Qt 5 et UCRT64 ;
- **43/43** tests CTest réussis, dont quatre contrats IND-01A ;
- exécution Windows native du clavier et de la non-mutation ;
- exécution Windows native à `QT_SCALE_FACTOR=1.5` ;
- parcours dans l’application complète : exemple Arduino ouvert comme copie,
  nouveau libellé du menu vérifié, gestionnaire ouvert, état vide explicite,
  `Ctrl+F` focalisant la recherche et `F5` rechargeant la vue ;
- capture reproductible du composant réel et de son modèle de validation :
  [`01-overview-populated.png`](../evidence/ind-01a-terminal-cable-overview/01-overview-populated.png).

La capture peuplée utilise six lignes déterministes couvrant deux borniers,
deux étages, plusieurs conducteurs, des accents, des bornes indépendantes et un
câble incomplet. Elle prouve le rendu du composant compilé ; le parcours dans
l’application complète prouve séparément son raccordement au projet et l’état
vide. Aucun projet industriel réel n’est simulé ni présenté comme preuve.

## Limites et suite

Le signal « câble incomplet » compare les agrégats non vides d’une borne ; il ne
valide ni la composition d’un câble, ni l’unicité des âmes, ni la continuité, ni
la conformité d’un plan de câblage. Le dépôt ne possède pas encore de modèle
métier `Cable` autonome.

IND-01B devra définir les identités, règles de validation et regroupements de
câbles sans casser les anciens projets. IND-01C pourra ensuite ajouter édition
groupée, aperçu de génération et export déterministe. Ces sous-lots exigent des
projets anonymisés représentatifs avant toute promesse de qualification métier.
