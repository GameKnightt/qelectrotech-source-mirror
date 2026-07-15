# DEV-01 — Clone Windows reproductible

## Résultat utilisateur

Un clone neuf du fork sous Windows 11 peut être effectué avec la commande Git
standard, sous-modules inclus. Le checkout ne dépend plus d'un objet Git LFS
indisponible et ne crée plus de modification fantôme liée à la casse.

## Corrections

- `ChangeLog` reste le changelog canonique utilisé par CMake et les paquets.
- Les blobs exacts de `ChangeLog.MD` et `ChangeLog.md` sont conservés sous
  `docs/history/changelog-github-generated.md` et
  `docs/history/changelog-development.md`.
- `doc/QElectroTech.qch`, artefact Doxygen de documentation Qt Creator, n'est
  plus versionné. Il reste générable localement et ignoré par Git.
- Le workflow Doxygen publie la documentation HTML sans recréer de pointeur LFS.
- La CI Windows refuse désormais toute collision de chemins insensible à la
  casse et tout pointeur Git LFS suivi dans le checkout courant.

## Compatibilité

Aucun format `.qet`, `.elmt`, XML, SQLite, export ou paramètre utilisateur n'est
modifié. Les deux changelogs historiques conservent leurs empreintes Git
d'origine ; seuls leurs chemins changent.

## Critères de validation

1. exécuter `scripts/check-repository-contracts.ps1` ;
2. cloner le fork avec `git clone --recursive` sur NTFS, sans variable LFS ;
3. vérifier que `git status --porcelain` est vide ;
4. exécuter la CI Windows Qt 5.
