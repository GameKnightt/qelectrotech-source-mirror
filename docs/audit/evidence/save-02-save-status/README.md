# Preuves SAVE-02 — état de sauvegarde

## Session Windows 11 réelle

Date : 14 juillet 2026. La build Qt 5 de la branche
`codex/save-02-save-state` a été lancée depuis un dossier portable temporaire
sous le nom `qelectrotech-save02.exe`. Le processus observé provenait bien de ce
dossier et non de `C:\Program Files\QElectroTech`.

Le fichier `examples/industrial.qet` a été dupliqué sous le nom
`industrial-save02-test.qet` avant toute modification.

| Étape | Preuve UI Automation | Résultat |
|---|---|---|
| ouverture | `Groupe État de sauvegarde : Sauvegardé — projet « Example project »` | sain |
| dialogues de chargement fermés | boutons `Enregistrer` et `Enregistrer sous` exposés et actifs | sain après correction MDI |
| ajout d’un folio sur la copie | `Groupe État de sauvegarde : Modifié — projet « Example project »` | sain |
| enregistrement | message `Projet Example project enregistré…` et écriture du fichier temporaire | sain |
| résultat | `Groupe État de sauvegarde : Sauvegardé — projet « Example project »` | sain |

La fenêtre mesurait 1 269 × 840 pixels. Le composant restait en bas à droite,
lisible, non rogné et hors de l’ordre de tabulation.

## Captures

Les captures Windows Graphics Capture ont été affichées et contrôlées pendant
la session, notamment les états **Modifié** et **Sauvegardé**. Le protocole du
contrôleur Windows interdit leur décodage et leur écriture manuelle sur disque ;
aucun PNG local n’est donc fabriqué artificiellement. La preuve durable repose
sur l’arbre d’accessibilité ci-dessus, les tests déterministes et la validation
du fichier écrit. Cette limite de collecte est explicite et ne transforme pas
les captures transitoires en fichiers prétendument archivés.

## Preuve automatisée

Commandes finales :

```powershell
ctest --test-dir C:\Users\alexy\AppData\Local\Temp\qet-save-02-test-build `
  --output-on-failure -j 1

$env:QT_QPA_PLATFORM = 'windows'
ctest --test-dir C:\Users\alexy\AppData\Local\Temp\qet-save-02-test-build `
  -R '^project_save_status_keyboard_test$' --output-on-failure

$env:QT_SCALE_FACTOR = '1.5'
ctest --test-dir C:\Users\alexy\AppData\Local\Temp\qet-save-02-test-build `
  -R '^(project_save_status_keyboard_test|project_save_status_large_text_150_test)$' `
  --output-on-failure
```

Résultats : `35/35`, puis `1/1`, puis `2/2`, sans échec.

Sources de preuve :

- `sources/ui/projectsavestatuscontroller.*` ;
- `sources/ui/projectsavestatuswidget.*` ;
- `sources/qetproject.*` ;
- `sources/qetdiagrameditor.*` ;
- `tests/qttest/tst_projectsavestatus.cpp` ;
- labels CTest `save;save-02;state-machine;recovery;accessibility`.
