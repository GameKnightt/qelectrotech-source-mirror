# Compiler QElectroTech sous Windows 11 avec MSYS2 UCRT64

Ce guide décrit la compilation de la branche `master` (Qt 5) dans un
environnement Windows 11 actuel. Les paquets et les options CMake sont alignés
sur le workflow `.github/workflows/windows-build.yml` du dépôt.

La branche Qt 6 est une trajectoire distincte. Ne mélangez pas les dépendances
Qt 5 et Qt 6 dans le même répertoire de build.

## 1. Installer MSYS2

1. Installer [MSYS2](https://www.msys2.org/) dans son emplacement par défaut,
   `C:\msys64`.
2. Ouvrir **MSYS2 UCRT64** depuis le menu Démarrer. Le titre et l'invite doivent
   indiquer `UCRT64`, et non `MSYS`, `MINGW64` ou `CLANG64`.
3. Mettre le système à jour :

```bash
pacman -Syu
```

Si MSYS2 demande de fermer le terminal, le rouvrir en mode UCRT64 puis relancer
`pacman -Syu` jusqu'à ce qu'aucune mise à jour système ne reste à appliquer.

Installer ensuite les dépendances utilisées par le build Windows officiel :

```bash
pacman -S --needed \
  git \
  mingw-w64-ucrt-x86_64-ccache \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt5-base \
  mingw-w64-ucrt-x86_64-qt5-svg \
  mingw-w64-ucrt-x86_64-qt5-tools \
  mingw-w64-ucrt-x86_64-qt5-translations \
  mingw-w64-ucrt-x86_64-sqlite3 \
  mingw-w64-ucrt-x86_64-pkg-config \
  mingw-w64-ucrt-x86_64-kwidgetsaddons \
  mingw-w64-ucrt-x86_64-kcoreaddons \
  mingw-w64-ucrt-x86_64-extra-cmake-modules \
  mingw-w64-ucrt-x86_64-nsis \
  mingw-w64-ucrt-x86_64-angleproject
```

Vérifier que tous les outils proviennent de l'environnement UCRT64 :

```bash
which cmake ninja g++ qmake
cmake --version
qmake --version
```

Les chemins retournés doivent commencer par `/ucrt64/bin/`.

## 2. Récupérer le dépôt et ses sous-modules

Un chemin court hors de OneDrive, par exemple `C:\dev\qelectrotech`, réduit les
risques liés aux chemins longs et à la synchronisation de fichiers pendant la
compilation.

Depuis le terminal UCRT64 :

```bash
mkdir -p /c/dev
cd /c/dev
GIT_LFS_SKIP_SMUDGE=1 git clone --recursive https://github.com/GameKnightt/qelectrotech-source-mirror.git qelectrotech
cd qelectrotech
git remote add upstream https://github.com/qelectrotech/qelectrotech-source-mirror.git
git submodule update --init --recursive
```

Pour un autre fork, remplacer uniquement l'URL de la commande `git clone`.

Le fichier de documentation Qt Creator `doc/QElectroTech.qch` est géré par Git
LFS mais n'est pas nécessaire à la compilation. Si son téléchargement est
indisponible, conserver le pointeur LFS et poursuivre :

```bash
GIT_LFS_SKIP_SMUDGE=1 git checkout -- doc/QElectroTech.qch
```

## 3. Configurer un build Qt 5

Toujours lancer CMake depuis le terminal UCRT64. La configuration suivante
produit un build de développement avec symboles de débogage et sans les tests :

```bash
cmake -S . -B build-ucrt64 -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH=/ucrt64 \
  -DQt5_DIR=/ucrt64/lib/cmake/Qt5 \
  -DQT_VERSION_MAJOR=5 \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DBUILD_TESTING=OFF \
  -DPACKAGE_TESTS=OFF \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DSQLite3_INCLUDE_DIR=/ucrt64/include \
  -DSQLite3_LIBRARY=/ucrt64/lib/libsqlite3.dll.a
```

Les deux options de tests sont volontairement indiquées : `BUILD_TESTING` est
l'option CMake standard et `PACKAGE_TESTS` reste prise en charge pour la
compatibilité avec les anciens scripts QElectroTech. Mettre l'une des deux à
`OFF` désactive les tests du projet.

Ne réutilisez pas ce répertoire pour la branche Qt 6. Créez un autre répertoire
de build si vous changez de branche, de générateur ou de version de Qt.

## 4. Compiler et lancer

```bash
cmake --build build-ucrt64 --parallel "$(nproc)"
./build-ucrt64/qelectrotech.exe
```

Le lancement depuis UCRT64 donne automatiquement accès aux DLL présentes dans
`/ucrt64/bin`. Une exécution depuis l'Explorateur nécessite d'abord de déployer
les DLL Qt et les bibliothèques MinGW ; le workflow GitHub Actions contient la
procédure de création du paquet portable et de l'installateur NSIS.

## 5. Compiler et exécuter les tests

Utiliser un répertoire séparé afin de conserver le build applicatif précédent :

```bash
cmake -S . -B build-ucrt64-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/ucrt64 \
  -DQt5_DIR=/ucrt64/lib/cmake/Qt5 \
  -DQT_VERSION_MAJOR=5 \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DBUILD_TESTING=ON \
  -DPACKAGE_TESTS=ON \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DSQLite3_INCLUDE_DIR=/ucrt64/include \
  -DSQLite3_LIBRARY=/ucrt64/lib/libsqlite3.dll.a

cmake --build build-ucrt64-tests --parallel "$(nproc)" --target \
  C_unittests \
  qt_unittests \
  diagram_duplicate_uuid_remapper_test \
  config_dialog_ux_test
ctest --test-dir build-ucrt64-tests --output-on-failure
QT_SCALE_FACTOR=1.5 ctest --test-dir build-ucrt64-tests \
  -R '^config_dialog_ux_test$' --output-on-failure
```

## Diagnostic rapide

- **CMake trouve Qt 6** : vérifier que le terminal est bien UCRT64, conserver
  `CMAKE_DISABLE_FIND_PACKAGE_Qt6=ON` et repartir dans un nouveau répertoire de
  build.
- **Un sous-module est vide** : exécuter
  `git submodule update --init --recursive`.
- **Une DLL manque au lancement** : lancer l'exécutable depuis UCRT64 pour le
  développement. Pour distribuer le programme, utiliser `windeployqt-qt5` et
  la procédure du workflow Windows.
- **Le chemin contient des espaces** : les commandes CMake ci-dessus restent
  valides si elles sont exécutées à la racine du dépôt. Pour un chemin fourni
  manuellement, toujours l'entourer de guillemets.
- **Le cache CMake vient d'une autre configuration** : utiliser un nouveau
  répertoire `build-*` au lieu de réemployer le cache existant.

## Correspondance avec l'intégration continue

La compilation locale et le job Windows partagent :

- MSYS2 UCRT64 et GCC ;
- Qt 5 explicitement sélectionné ;
- CMake avec le générateur Ninja ;
- SQLite fourni par `/ucrt64` ;
- les tests désactivés pour la construction de l'installateur.

Le workflow ajoute ensuite `ccache`, déploie les DLL avec `windeployqt-qt5`,
assemble un paquet portable et produit l'installateur NSIS. Ces opérations ne
sont pas nécessaires pour la boucle quotidienne compilation-lancement-test.
