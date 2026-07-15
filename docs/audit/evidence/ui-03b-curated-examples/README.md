# Preuves UI-03B — Exemples curatés

Validation réalisée le 15 juillet 2026 sous Windows 11 avec le build Qt 5
UCRT64 de la branche UI-03B.

| Fichier | État | Résultat |
|---|---|---|
| `01-start-center.png` | Centre de démarrage, deux récents et quatre exemples installés | Mise en page native Qt, métadonnées métier visibles, aucune barre de défilement horizontale |

La capture est produite par le rendu natif opaque de `StartCenterWidget` dans
le test Qt `rendersTemplateEvidence`. Elle instancie le composant réel compilé,
les vraies métadonnées du catalogue et les quatre fichiers `.qet` du dépôt.
Elle ne réimplémente pas l'interface.

Le parcours complet a également été contrôlé dans l'exécutable Windows :
ArduinoLCD s'ouvre avec le titre et la barre d'état **Modifié**, le message
« Exemple ouvert comme copie non enregistrée », puis `Ctrl+S` ouvre
**Enregistrer sous** sur `sansnom.qet`.
