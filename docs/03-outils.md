# Outils : ce dont vous avez besoin

> La formation se compile et se teste **à l'identique** sous Windows, Linux et
> macOS, avec MSVC, GCC ou Clang. Les commandes changent de nom ; les presets,
> les résultats et les messages sont les mêmes.

---

## 1. Pourquoi le multi-plateforme, dans une formation DO-178C ?

Ce n'est pas du confort. C'est un outil de qualité, et un point de cours.

* **Chaque chaîne détecte ce que les autres laissent passer.** MSVC `/W4` ne
  voit pas ce que GCC `-Wconversion` voit, et inversement. Compiler avec deux
  compilateurs, c'est **deux analyses statiques pour le prix d'une** — la façon
  la moins chère d'augmenter la confiance.
* **Un code qui ne compile que sur une chaîne** porte des hypothèses
  implicites qu'on ne connaît pas.
* **Le compilateur cible change** au cours de la vie d'un programme. Le
  portage est alors déjà fait.

> ⚠️ **Attention au contresens.** Compiler sur trois chaînes ne dispense de
> rien. Le **SECI** (modules 00 et 14) fige **une** chaîne, **une** version,
> **un** jeu d'options. La portabilité est un outil de qualité *pendant le
> développement*, pas une propriété du produit certifié.

---

## 2. Windows — Visual Studio 2026

Votre installation avec la charge de travail *Développement Desktop en C++*
fournit **tout**. Aucune installation supplémentaire.

| Outil | Rôle | Emplacement |
|---|---|---|
| **MSVC** 14.51 | compilateur | `VC\Tools\MSVC\` |
| **CMake** 4.3 | génération du build | `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\` |
| **Ninja** | moteur de build | `…\CMake\Ninja\` |
| **clang-tidy**, **clang-format** 22 | analyse statique, formatage | `VC\Tools\Llvm\x64\bin\` |
| **Git**, **Python 3** | configuration, outillage | installations séparées |

```bash
.\scripts\build.ps1 -Preset debug -Test
```

| Commande | Effet |
|---|---|
| `.\scripts\build.ps1` | configure et compile en Debug |
| `.\scripts\build.ps1 -Test` | + lance toute la campagne |
| `.\scripts\build.ps1 -Preset strict -Test` | warnings = erreurs **et** clang-tidy |
| `.\scripts\build.ps1 -Clean` | supprime `build/` |

Le script localise Visual Studio via `vswhere` : rien à ajouter au `PATH`.

---

## 3. Linux (Ubuntu, Debian, WSL) et macOS

### 3.1 Installation

**Ubuntu / Debian / WSL**

```bash
sudo apt-get update && sudo apt-get install -y build-essential clang clang-tidy clang-format cmake ninja-build gcovr git python3
```

**Fedora**

```bash
sudo dnf install -y gcc-c++ clang clang-tools-extra cmake ninja-build gcovr git python3
```

**macOS** (outils Xcode en ligne de commande + Homebrew)

```bash
xcode-select --install && brew install cmake ninja llvm gcovr
```

> Sur macOS, `clang-tidy` et `clang-format` viennent du paquet `llvm` de
> Homebrew et ne sont pas dans le `PATH` par défaut. Ajoutez
> `$(brew --prefix llvm)/bin` à votre `PATH`, sinon le preset `strict`
> compilera sans analyse statique — et vous le dira.

### 3.2 Utilisation

```bash
./scripts/build.sh -t
```

| Commande | Effet |
|---|---|
| `./scripts/build.sh` | configure et compile en Debug |
| `./scripts/build.sh -t` | + lance les tests |
| `./scripts/build.sh -p strict -t` | warnings = erreurs + clang-tidy |
| `./scripts/build.sh -p asan -t` | **sanitizers** (ASan + UBSan) |
| `./scripts/build.sh -p gcc-strict -t` | force GCC |
| `./scripts/build.sh -p clang-strict -t` | force Clang |
| `./scripts/build.sh -c` | supprime `build/` |
| `./scripts/build.sh -l` | liste les presets disponibles ici |
| `./scripts/build.sh -h` | aide |

Le script vérifie l'outillage **avant** de démarrer et, s'il manque quelque
chose, affiche la commande d'installation adaptée à votre distribution.

---

## 4. Devcontainer — l'environnement figé

Si vous avez Docker et VS Code (ou tout autre client devcontainer) :

**Ouvrir le dossier → « Reopen in Container »**

L'image est décrite dans [`.devcontainer/Dockerfile`](../.devcontainer/Dockerfile) :
Ubuntu 24.04, GCC, Clang, CMake, Ninja, clang-tidy, gcovr, lcov, gdb,
valgrind, Python 3.

> **C'est le SECI le plus concret de toute la formation.** La section 11.15 de
> la DO-178C demande de figer et d'identifier tout ce qui a servi à produire le
> logiciel, pour pouvoir le reconstruire dans trente ans. Un Dockerfile
> **est** cette description — en exécutable.
>
> Le Dockerfile explique aussi ce qu'il faudrait faire **en plus** sur un vrai
> programme : épingler l'image par empreinte plutôt que par étiquette, épingler
> chaque paquet à sa version, archiver l'image construite. Savoir *pourquoi on
> ne le fait pas ici* est exactement ce qu'un auditeur attend.

---

## 5. Les presets, identiques partout

`cmake --list-presets` n'affiche que ceux applicables à votre machine.

| Preset | Disponible | Rôle |
|---|---|---|
| `debug` | partout | travail quotidien, avertissements non bloquants |
| `release` | partout | optimisé |
| `strict` | partout | **warnings = erreurs + clang-tidy** — ce que la CI exécute |
| `asan` | partout | AddressSanitizer (+ UBSan hors MSVC) |
| `gcc-strict` | Linux, macOS | force GCC |
| `clang-strict` | Linux, macOS | force Clang |
| `coverage` | Linux, macOS | instrumentation gcov |
| `vs2026` | Windows | génère une solution `.sln` |

Utilisables directement, sans passer par les scripts :

```bash
cmake --preset strict && cmake --build --preset strict && ctest --preset strict
```

---

## 6. Couverture structurelle

| Plateforme | Commande | Ce qui est mesuré |
|---|---|---|
| Windows | `.\scripts\coverage.ps1` | instructions (OpenCppCoverage) |
| Linux, macOS | `./scripts/coverage.sh` | instructions **et branches** (gcovr ou lcov) |

Installation d'OpenCppCoverage sous Windows :

```bash
winget install OpenCppCoverage.OpenCppCoverage
```

> ⚠️ **Aucun de ces outils ne mesure le MC/DC**, et la couverture de
> *branches* de gcov n'est **pas** la couverture de *décision* au sens
> DO-178C : gcov compte les branches du code **généré**, pas les décisions du
> code **source**. Un `&&` à court-circuit produit plusieurs branches pour une
> seule décision. Utilisable comme indicateur, jamais comme preuve de
> conformité. Voir module 11 §1.8.

---

## 7. Sanitizers — disponible seulement hors Windows en version complète

```bash
./scripts/build.sh -p asan -t
```

| Sanitizer | Ce qu'il détecte | Module concerné |
|---|---|---|
| **UBSan** | débordement d'entier **signé**, décalage invalide, déréférencement nul, conversion hors domaine | **01** |
| **ASan** | débordement de tampon, usage après libération, fuites | **02**, **08** |

> C'est l'outil qui rend **tangible** tout le module 01. Le débordement signé y
> est présenté comme un « comportement indéfini » ; UBSan le transforme en
> échec de test, avec la ligne exacte.

MSVC ne fournit qu'AddressSanitizer (pas d'UBSan), et il est incompatible avec
les vérifications d'exécution du mode Debug.

---

## 8. Intégration continue

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) — Linux, à chaque
poussée :

| Job | Ce qu'il vérifie | Objectif DO-178C |
|---|---|---|
| `build` (GCC **et** Clang) | compilation stricte + 19 campagnes | A-5.4, A-6.x |
| `sanitizers` | aucun comportement indéfini | A-5.6 |
| `tracabilite` | `trace_check.py --strict`, production du SCI/SECI | A-3.6, A-4.6, A-5.5 |
| `couverture` | instructions et branches | A-7.6, A-7.7 |
| `format` | conformité à `.clang-format` | A-5.4 (règle R-15) |

Les artefacts (résultats de test, SCI, SECI, rapport de couverture) sont
archivés — ce sont des **données de vie du logiciel**.

---

## 9. Synchroniser entre machines

```bash
git remote add origin <url-de-votre-depot>
```

```bash
git push -u origin main --tags
```

`.gitignore` exclut `build/`, `reports/`, `.vs/` : **seules les sources sont
versionnées**. C'est ce que demande la gestion de configuration DO-178C — le
binaire se reconstruit, il ne s'archive pas.

`.gitattributes` normalise les fins de ligne : LF partout, CRLF pour les
scripts PowerShell. Vous ne verrez pas de différences fantômes entre un poste
Windows et un poste Linux.

> **WSL : un conseil qui évite des heures perdues.** Clonez le dépôt dans le
> système de fichiers **Linux** (`~/dev/...`), pas dans `/mnt/c/...`. Les accès
> à travers `/mnt/c` sont dix fois plus lents, et les permissions POSIX
> (notamment le bit exécutable des scripts) ne s'y comportent pas correctement.

---

## 10. Et pour du « vrai » embarqué ?

| Besoin | Outil |
|---|---|
| Compilateur croisé ARM | GNU Arm Embedded Toolchain (gratuit) |
| Émulateur de cible | QEMU |
| Carte réelle | STM32 Nucleo (~20 €), Raspberry Pi Pico (~5 €) |
| RTOS certifiable | SAFERTOS, µC/OS ; FreeRTOS et Zephyr pour apprendre |
| RTOS ARINC 653 | Wind River VxWorks 653, SYSGO PikeOS, Lynx LynxOS-178 |
| Analyse WCET | AbsInt aiT, Rapita RapiTime |
| Couverture qualifiée | VectorCAST, LDRA Testbed, Rational Test RealTime, Cantata |

Ces outils commerciaux s'apprennent en poste. **Ce n'est pas un prérequis à
l'embauche** : ce qu'on attend, c'est de comprendre *ce qu'ils mesurent* et
*pourquoi*.

---

## 11. Diagnostic

| Symptôme | Cause | Solution |
|---|---|---|
| `vswhere.exe introuvable` | VS absent ou installation partielle | installer *Développement Desktop en C++* |
| `cl.exe introuvable` | hors environnement développeur | utiliser `scripts\build.ps1` |
| `ninja introuvable` (Linux) | paquet manquant | `sudo apt-get install ninja-build` |
| `clang-tidy demandé mais introuvable` | composant LLVM absent | Windows : *Compilateur C++ Clang* ; macOS : ajouter `$(brew --prefix llvm)/bin` au `PATH` |
| `Permission denied` sur `build.sh` | bit exécutable perdu | `chmod +x scripts/*.sh` |
| `bad interpreter: ^M` | script converti en CRLF | `git config core.autocrlf input` puis re-cloner |
| Script PowerShell bloqué | politique d'exécution | `Set-ExecutionPolicy -Scope Process RemoteSigned` |
| Accents mal affichés | page de codes | `chcp 65001`, ou Windows Terminal |
| SECI incomplet (`non detecte`) | généré hors environnement de build | Windows : régénérer depuis une *Developer PowerShell* |
