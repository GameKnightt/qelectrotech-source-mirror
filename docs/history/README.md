# Changelogs historiques

Le changelog canonique installé avec QElectroTech reste le fichier
[`ChangeLog`](../../ChangeLog) à la racine.

Deux changelogs Markdown supplémentaires sont conservés ici pour référence :

- `changelog-github-generated.md` est le journal généré depuis les tickets et
  les pull requests ;
- `changelog-development.md` est le journal de développement conventionnel.

Ils se trouvaient auparavant à la racine sous les noms `ChangeLog.MD` et
`ChangeLog.md`. Ces noms ne diffèrent que par la casse et ne peuvent pas être
matérialisés côte à côte sur le système de fichiers Windows par défaut. Leur
déplacement conserve leurs blobs Git exacts tout en rendant le clone Windows
propre.
