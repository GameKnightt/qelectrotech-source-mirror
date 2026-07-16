# Préversion portable Windows interne

Le script `packaging/windows/deploy-portable.ps1` transforme une compilation
Release Qt 5/UCRT64 en dossier candidat pour les essais internes. Il exécute le
déploiement Qt, complète les dépendances transitives MSYS2 et ajoute les
collections, cartouches et traductions QElectroTech.

Le script doit être lancé depuis PowerShell 64 bits avec des entrées de
confiance. Il refuse les builds non Release, vérifie les plugins Qt requis,
exécute `qelectrotech.exe --version` sans MSYS2 dans le `PATH` et génère un
manifeste SHA-256. Il déploie d’abord dans un dossier temporaire frère, puis
renomme le résultat seulement après validation complète.

Le dossier final contient `Launch-QElectroTech-Preview.bat`. Ce lanceur utilise
les collections, cartouches et traductions du dossier portable ainsi qu’un
répertoire `conf/` local. La préversion peut ainsi fonctionner à côté d’une
installation stable sans partager ses préférences ni son verrou d’instance.

## Préparer la compilation Release

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

C:\msys64\ucrt64\bin\cmake.exe -S . -B build-release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 `
  -DQt5_DIR=C:/msys64/ucrt64/lib/cmake/Qt5 `
  -DQT_VERSION_MAJOR=5 `
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON `
  -DBUILD_WITH_KF5=OFF `
  -DBUILD_TESTING=OFF `
  -DPACKAGE_TESTS=OFF `
  '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' `
  -DSQLite3_INCLUDE_DIR=C:/msys64/ucrt64/include `
  -DSQLite3_LIBRARY=C:/msys64/ucrt64/lib/libsqlite3.dll.a

C:\msys64\ucrt64\bin\cmake.exe --build build-release --target qelectrotech
```

Utiliser un dossier de build Release neuf. `cmake`, `ninja` et les compilateurs
doivent provenir du même environnement MSYS2 UCRT64.

## Générer le dossier portable

Le dossier de sortie doit être absent et son dossier parent doit déjà exister.
Pour éviter de polluer le dépôt, l’exemple écrit dans le dossier temporaire de
l’utilisateur.

```powershell
$output = Join-Path $env:TEMP 'qelectrotech-portable-preview'

.\packaging\windows\deploy-portable.ps1 `
  -BuildDir .\build-release `
  -SourceDir . `
  -OutputDir $output
```

Le script conserve seulement le pilote SQL SQLite utilisé par QElectroTech,
copie les licences du projet et de la collection, et échoue si une dépendance
native requise ne peut pas être résolue depuis le même environnement UCRT64.
Si une installation Qt 5 locale partiellement réparée ne contient plus le nom
`qmake-qt5.exe` attendu par `windeployqt-qt5`, le script copie explicitement le
petit ensemble de plugins et de bibliothèques graphiques Qt requis, puis utilise
la même fermeture PE pour résoudre leurs dépendances liées. Le panneau
**Automatisation et IA** utilise uniquement les primitives Qt Quick intégrées
au runtime, sans module de contrôles QML externe. Le chemin de secours déploie
toutefois les plugins cœur `QtQuick.2` et `QtQml` qui enregistrent ces types au
chargement. Le fichier `qt.conf` redirige également les chemins Qt compilés
dans MSYS2 vers le dossier portable, afin que ces modules restent trouvables
sur une machine où MSYS2 n'est pas installé. Le résultat passe les mêmes
validations dans les deux chemins. Le greffon de plateforme `qoffscreen.dll`
est également fourni pour que le serveur MCP fonctionne sans fenêtre dans les
clients d'automatisation.

Le manifeste est écrit en UTF-8 sans BOM afin que les chemins de composants
Unicode puissent être vérifiés sans altération.

Cette sortie reste une préversion interne. Une distribution publique exige en
plus un inventaire de toutes les licences tierces, la mise à disposition des
sources correspondantes et une validation sur une machine ou une Windows
Sandbox sans MSYS2 installé.

## Paquets publiés par la CI

Le workflow principal publie l'installateur NSIS `.exe` recommandé et l'archive
portable `.zip`. Le workflow WiX produit également un `.msi`. Si les identifiants
SignPath sont configurés, ce MSI est signé. Sinon, la compilation et la
publication continuent avec un nom se terminant par `-unsigned.msi` ; cette
indication est également affichée sur la page de téléchargement et Windows
présentera un avertissement d'éditeur. Le workflow ne doit jamais présenter un
paquet non signé comme signé.

Le déploiement GitHub Pages est optionnel. Il n'est exécuté que si Pages est
activé dans les paramètres du dépôt avec **GitHub Actions** comme source et si
la variable de dépôt `QET_DEPLOY_PAGES` vaut `true`. Sans cette configuration,
les paquets Nightly restent publiés normalement et l'étape Pages est ignorée au
lieu de faire échouer le workflow MSI.
