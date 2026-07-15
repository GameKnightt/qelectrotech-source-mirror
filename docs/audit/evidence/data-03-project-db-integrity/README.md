# Preuves DATA-03 — base projet atomique

## Preuve automatisée

Commande Windows 11 :

```powershell
ctest --test-dir C:\Users\alexy\AppData\Local\Temp\qet-data-03-test-build `
  -R '^project_database_atomicity_test$' --output-on-failure
```

Résultat : `1/1` test réussi, `0` échec. La cible exécute onze scénarios : un
succès, huit erreurs de requête forcées, un échec de commit et une base fermée.
Chaque erreur transactionnelle compare l'intégralité des quatre tables avec
l'instantané antérieur et vérifie qu'une nouvelle transaction peut commencer
immédiatement après le rollback.

Sources de preuve :

- `tests/qttest/tst_projectdatabasewriter.cpp` ;
- `sources/dataBase/projectdatabasewriter.h` ;
- `sources/dataBase/projectdatabasewriter.cpp` ;
- label CTest `database;data-03;atomic;rollback`.

## Preuve d'intégration

La cible complète `qelectrotech.exe` a été configurée puis compilée avec Qt 5,
MSYS2 UCRT64 et Ninja. Les 272 étapes se terminent par :

```text
[271/272] Linking CXX executable qelectrotech.exe
```

Binaire validé :

```text
C:\Users\alexy\AppData\Local\Temp\qet-data-03-test-build\qelectrotech.exe
```

Smoke test métier :

```powershell
qelectrotech.exe --export-bom examples\industrial.qet `
  C:\Users\alexy\AppData\Local\Temp\qet-data03-bom-smoke-verified.csv
```

Résultat : code de sortie `0`, fichier de 18 176 octets et 355 lignes. Deux
exécutions indépendantes produisent le même SHA-256 :
`C298FC9BB008E85BEC53A3515211037ED043089A4E5BBF4214D21D8BA323CFEF`.

## Pourquoi il n'y a pas de capture d'écran

DATA-03 traite des pannes SQL rares et injectées de manière déterministe. Une
capture du dialogue d'erreur prouverait seulement son texte, pas le rollback
des quatre tables. La preuve principale est donc l'état SQLite avant/après et
la compilation des parcours GUI/CLI qui consomment le résultat typé.
