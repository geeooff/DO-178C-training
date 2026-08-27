# Outils : ce dont vous avez besoin

---

## 1. L'essentiel — déjà installé

Votre installation de **Visual Studio 2026 Community** avec la charge de
travail *Développement Desktop en C++* fournit **tout** ce dont la formation a
besoin. Aucune installation supplémentaire n'est nécessaire.

| Outil | Version détectée | Rôle | Emplacement |
|---|---|---|---|
| **MSVC** | 14.51 (cl 19.51) | compilateur C++ | `VC\Tools\MSVC\` |
| **CMake** | 4.3 | génération du build | `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\` |
| **Ninja** | fourni | moteur de build rapide | `…\CMake\Ninja\` |
| **CTest** | fourni avec CMake | exécution des campagnes de test | idem CMake |
| **clang-tidy** | 22 | analyse statique | `VC\Tools\Llvm\x64\bin\` |
| **clang-format** | 22 | formatage | idem |
| **Git** | 2.55 | gestion de configuration | installation séparée, déjà présente |
| **Python** | 3.13 | outils `trace_check.py` et `config_index.py` | installation séparée, déjà présente |

Le script [`scripts/build.ps1`](../scripts/build.ps1) localise tout cela
automatiquement via `vswhere` : vous n'avez **rien** à ajouter au `PATH`.

---

## 2. Optionnel mais recommandé

| Outil | Rôle | Installation |
|---|---|---|
| **OpenCppCoverage** | couverture d'instructions (module 11) | `winget install OpenCppCoverage.OpenCppCoverage` |

> ⚠️ OpenCppCoverage mesure la couverture **d'instructions**. Il ne mesure ni
> la couverture de décision, ni le MC/DC. Voir module 11 §1.8.

---

## 3. Trois façons de travailler

### 3.1 Ligne de commande (recommandé)

Depuis **n'importe quel** PowerShell, à la racine du dépôt :

```bash
.\scripts\build.ps1 -Preset debug -Test
```

Options :

| Commande | Effet |
|---|---|
| `.\scripts\build.ps1` | configure et compile en Debug |
| `.\scripts\build.ps1 -Test` | + lance toute la campagne de test |
| `.\scripts\build.ps1 -Preset strict -Test` | warnings = erreurs **et** clang-tidy |
| `.\scripts\build.ps1 -Clean` | supprime `build/` et repart de zéro |

### 3.2 Visual Studio — ouvrir le dossier

**Fichier → Ouvrir → Dossier…** et choisissez la racine du dépôt.

Visual Studio détecte `CMakePresets.json` et propose les configurations
`debug`, `release`, `strict`. L'explorateur de tests affiche les 19 campagnes.
C'est la façon la plus confortable de **déboguer** pas à pas.

### 3.3 Visual Studio — solution classique

Si vous préférez un `.sln` :

```bash
cmake --preset vs2026
```

La solution est générée dans `build/vs2026/`.

---

## 4. Les commandes du quotidien

```bash
ctest --preset debug --output-on-failure
```

```bash
ctest --preset debug -R 11-couverture --output-on-failure
```

Exécuter une campagne directement, avec le détail et la matrice de
traçabilité :

```bash
.\build\debug\bin\tests_16-projet-integre.exe --verbose --req
```

Vérifier la traçabilité de tout le dépôt :

```bash
python tools/trace_check.py
```

Produire le SCI et le SECI :

```bash
python tools/config_index.py
```

Mesurer la couverture d'instructions :

```bash
.\scripts\coverage.ps1
```

Reformater un fichier :

```bash
clang-format -i modules/16-projet-integre/src/fqms.cpp
```

---

## 5. Options du harnais `microtest`

Chaque exécutable de test accepte :

| Option | Effet |
|---|---|
| `--list` | liste les cas sans les exécuter |
| `--filter=<texte>` | n'exécute que les cas dont `suite.nom` contient `<texte>` |
| `--verbose` | affiche aussi les cas réussis |
| `--req` | affiche la **matrice exigence → cas de test** |
| `--csv=<fichier>` | exporte la traçabilité en CSV |

---

## 6. Synchroniser entre plusieurs postes

Le dépôt est un dépôt Git ordinaire. Pour le publier :

```bash
git remote add origin <url-de-votre-depot>
```

```bash
git push -u origin main
```

Sur le second poste :

```bash
git clone <url-de-votre-depot>
```

```bash
.\scripts\build.ps1 -Preset debug -Test
```

Le fichier `.gitignore` exclut `build/`, `reports/`, `.vs/` et tous les
artefacts : **seules les sources sont versionnées**. C'est exactement ce que
demande la gestion de configuration DO-178C (module 14) — le binaire se
reconstruit, il ne s'archive pas.

`.gitattributes` normalise les fins de ligne : les scripts PowerShell restent
en CRLF, tout le reste en LF. Vous ne verrez pas de différences fantômes entre
vos postes.

---

## 7. Et pour du « vrai » embarqué ?

Cette formation compile pour Windows/x64, ce qui est parfait pour apprendre.
Si vous voulez pousser plus loin :

| Besoin | Outil |
|---|---|
| Compilateur croisé ARM | GNU Arm Embedded Toolchain (gratuit) |
| Émulateur de cible | QEMU |
| Carte réelle | STM32 Nucleo (~20 €), Raspberry Pi Pico (~5 €) |
| RTOS certifiable | FreeRTOS, Zephyr — versions certifiées : SAFERTOS, µC/OS |
| RTOS ARINC 653 | Wind River VxWorks 653, SYSGO PikeOS, Lynx LynxOS-178 |
| Analyse WCET | AbsInt aiT, Rapita RapiTime |
| Couverture qualifiée | VectorCAST, LDRA Testbed, Rational Test RealTime, Cantata |

Ces outils commerciaux coûtent cher et s'apprennent en poste. **Ce n'est pas un
prérequis à l'embauche** : ce qu'on attend d'un candidat, c'est de comprendre
*ce qu'ils mesurent* et *pourquoi*. C'est précisément l'objet de cette
formation.

---

## 8. Diagnostic

| Symptôme | Cause probable | Solution |
|---|---|---|
| `vswhere.exe introuvable` | Visual Studio non installé, ou installation partielle | installer la charge *Développement Desktop en C++* |
| `cl.exe introuvable` | script lancé hors environnement développeur | utiliser `scripts\build.ps1`, qui l'installe |
| `clang-tidy demandé mais introuvable` | composant LLVM non installé | Visual Studio Installer → *Compilateur C++ Clang pour Windows* |
| Erreur d'exécution de script PowerShell | politique d'exécution restrictive | `Set-ExecutionPolicy -Scope Process RemoteSigned` |
| `python` non reconnu | Python absent du `PATH` | réinstaller Python en cochant *Add to PATH* |
| Accents mal affichés dans la console | page de codes | `chcp 65001`, ou utiliser Windows Terminal |
