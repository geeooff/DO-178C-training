# Module 13 — Standards de codage et analyse statique

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 12

---

## Objectifs pédagogiques

1. Comprendre ce que la DO-178C exige réellement d'un standard de codage.
2. Situer MISRA C++:2023, AUTOSAR C++14 et JSF++.
3. Reconnaître douze violations classiques et savoir les corriger.
4. Utiliser clang-tidy et clang-format en pratique.
5. Conduire un **processus de déviation** correct.

---

## 1. Le cours

### 1.1 Ce que la norme exige (et n'exige pas)

La DO-178C **n'impose aucun standard particulier**. Elle impose :

* d'**en avoir un**, décrit dans le SDP (*Software Development Plan*) ;
* que le code y soit **conforme** — objectif **A-5.4** ;
* que la conformité soit **vérifiée** (revue et/ou analyse) ;
* que toute **déviation** soit justifiée et approuvée.

> Un standard de codage ne parle **pas** de correction fonctionnelle. Il parle
> de **vérifiabilité**, de **lisibilité** et de **prévisibilité** — donc du
> coût de la vérification, de la maintenance et de la certification sur vingt
> ans.

C'est exactement ce que démontre ce module : deux implémentations du même
algorithme, l'une conforme et l'autre non, **passent les mêmes tests**.

### 1.2 Les trois références

| Standard | Année | Cible | Positionnement |
|---|---|---|---|
| **MISRA C++:2023** | 2023 | C++17 | **Le** standard actuel. Absorbe l'héritage d'AUTOSAR C++14. Payant. |
| **AUTOSAR C++14** | 2017 | C++14 | Automobile. Historique, largement repris par MISRA C++:2023. Était gratuit. |
| **JSF++** | 2005 | C++03 | Avionique militaire (F-35). **Public et gratuit** — une bonne première lecture, même si daté. |

Structure des règles MISRA :

| Catégorie | Signification |
|---|---|
| **Mandatory** | aucune déviation possible |
| **Required** | déviation possible, **avec justification formelle** |
| **Advisory** | recommandation ; l'écart se documente mais s'approuve plus simplement |

### 1.3 Le standard du projet

Extrait, tel qu'il figurerait au SDP. Chaque règle est **vérifiable**, par le
compilateur, par clang-tidy ou par revue.

| Id | Règle | Vérifiée par |
|---|---|---|
| R-01 | Pas de `using namespace` en portée de fichier | revue |
| R-02 | Aucun nombre magique : toute constante est nommée et tracée | revue |
| R-03 | Pas de macro de type fonction ; utiliser `constexpr` ou `inline` | revue, clang-tidy |
| R-04 | Toute variable est initialisée à sa déclaration | clang-tidy (`cppcoreguidelines-init-variables`) |
| R-05 | Types de largeur explicite (`avio::u16`), jamais `int`/`long` | revue |
| R-06 | Conversions par `static_cast`, jamais à la manière du C | revue |
| R-07 | Pas de variable globale mutable partagée | revue |
| R-08 | Pas de récursion | revue, analyse de pile |
| R-09 | Pas d'allocation dynamique après initialisation | revue |
| R-10 | Pas d'exception ; `noexcept` sur toute fonction | compilateur |
| R-11 | Boucles à bornes connues et constantes | revue |
| R-12 | Pas de `goto` ; `break`/`continue` à justifier | revue |
| R-13 | Un seul point de sortie par fonction (règle *advisory*) | revue |
| R-14 | Toute fonction non triviale porte une annotation `@satisfies` | `trace_check.py` |
| R-15 | Formatage conforme à `.clang-format` | clang-format |

> **Sur R-13** — le point de sortie unique est une règle *advisory*, et
> discutée. Elle simplifie l'instrumentation et la vérification des
> post-conditions ; elle peut aussi produire des fonctions plus profondément
> imbriquées. Beaucoup d'équipes acceptent un `return` anticipé **de garde** en
> tête de fonction. L'important est que **le choix soit écrit** dans le SDP.

### 1.4 Les douze violations

Le fichier [`src/nonconforming.cpp`](src/nonconforming.cpp) viole douze règles.
Il est **fonctionnellement correct** : il passe les mêmes tests que la version
conforme.

| # | Violation | Pourquoi c'est un problème |
|---|---|---|
| V01 | `using namespace` en portée de fichier | ambiguïtés de résolution de surcharge ; provenance des identifiants illisible |
| V02 | Macro de type fonction sans parenthèses protectrices | `DIGIT(x, 1+1)` devient `(x >> 1+1*4)` : **faux**. Pas de type, pas de portée, invisible au débogueur |
| V03 | Variable globale mutable | couplage de données invisible (module 12), fonction non réentrante |
| V04 | Récursion | profondeur de pile non triviale à borner (module 08) |
| V05 | Variables non initialisées | lecture d'une valeur indéterminée = **comportement indéfini** |
| V06 | Nombre magique (`4`) | intention non exprimée, modification risquée |
| V07 | Conversion à la manière du C | peut supprimer un `const` en silence ; non recherchable |
| V08 | Nombre magique (`9`) | idem V06 |
| V09 | Sorties multiples + valeur sentinelle | **rien dans le type ne distingue l'erreur du succès** |
| V10 | Effet de bord sur une globale | comportement dépendant de l'historique |
| V11 | Opérateur virgule | deux effets de bord dans une expression |
| V12 | Fonction jamais appelée | **code mort** — constat DO-178C §6.4.4.3 |

**V09 mérite un développement.** La version non conforme renvoie `0xFFFFFFFF`
en cas d'erreur : une valeur du **même type** qu'un résultat valide. L'appelant
*peut* l'ignorer — et il le fera. Ici, `0xFFFFFFFF` n'est pas atteignable par
une conversion valide (le maximum est 9999), mais c'est un **coup de chance**
lié au domaine restreint. Sur un `u16` dont le domaine irait jusqu'à 65535, il
n'y aurait plus de valeur libre. `Result<T>` n'a pas ce problème : le statut est
un **champ séparé** (module 07).

### 1.5 Isoler le code non conforme

`nonconforming.cpp` est compilé dans une **cible séparée**, avec clang-tidy
désactivé et les avertissements relâchés :

```cmake
add_library(mod_13_nonconforming STATIC src/nonconforming.cpp)
set_target_properties(mod_13_nonconforming PROPERTIES CXX_CLANG_TIDY "")
target_compile_options(mod_13_nonconforming PRIVATE /W1 /WX-)
```

C'est le traitement que l'on réserve en projet réel au code **hérité** ou
**tiers** que l'on ne peut pas modifier :

1. l'isoler dans sa propre unité de compilation et son propre espace de noms ;
2. documenter la dérogation **à l'endroit exact** où elle s'applique ;
3. ne pas laisser ses avertissements noyer ceux du code neuf.

> Le contre-exemple à ne **jamais** faire : désactiver `/W4` ou clang-tidy pour
> tout le projet parce qu'un fichier est bruyant.

MSVC émet alors deux messages `D9025` (« overriding /W4 with /W1 »). Ils sont
**attendus** : ils laissent dans le journal de build la trace que la dérogation
a lieu, et où. On ne les supprime pas — une dérogation silencieuse serait pire
qu'une dérogation bruyante.

### 1.6 Les quatre niveaux de vérification

| Niveau | Outil | Trouve | Coût |
|---|---|---|---|
| **1** | Compilateur `/W4 /permissive-` | conversions implicites, variables inutilisées, membres non initialisés, comparaisons signe/non signe | **nul** — à activer avant tout |
| **2** | clang-tidy | usage après déplacement, découpage, macros dangereuses, boucles à compteur flottant, règle de 5 incomplète, branches identiques | quelques minutes de build |
| **3** | clang-format | rien, mais supprime un sujet de débat en revue et rend les diffs Git lisibles | nul |
| **4** | **Revue humaine** | *le code fait-il ce que dit l'exigence ?* l'invariant est-il préservé ? le nom est-il juste ? | le plus cher |

> Les niveaux 1 à 3 existent pour que la revue humaine se concentre sur le
> niveau 4, et pas sur des points-virgules. C'est l'argument à donner quand on
> vous demande pourquoi investir dans l'outillage.

**Commandes** :

```bash
.\scripts\build.ps1 -Preset strict
```

```bash
clang-format -i modules/13-standards-codage/src/bcd.cpp
```

### 1.7 Le processus de déviation

Aucun standard n'est applicable à 100 % sans exception. Ce qui compte n'est pas
l'absence de déviation : c'est leur **maîtrise**.

| # | Critère | Concrètement |
|---|---|---|
| 1 | **Localisée** | limitée à la ligne, la fonction ou la cible concernée ; jamais désactivée globalement |
| 2 | **Justifiée** | pourquoi la règle ne s'applique pas **ici** |
| 3 | **Analysée** | quel risque, quelle mesure compensatoire |
| 4 | **Approuvée** | par le responsable qualité logicielle |
| 5 | **Tracée** | enregistrée, comptée, revue périodiquement |

Exemples réels dans ce dépôt — cherchez `DEVIATION` :

| Fichier | Déviation | Justification |
|---|---|---|
| `common/CMakeLists.txt` | `_CRT_SECURE_NO_WARNINGS` | limité à une cible ; `fopen_s` est une extension non portable |
| `modules/03/.../main.cpp` | `NOLINT(bugprone-use-after-move)` | l'état post-déplacement **est** le sujet du test |
| `modules/05/.../sensors.cpp` | `NOLINT(performance-unnecessary-value-param)` | le découpage **est** le défaut à démontrer |

Notez la forme : `NOLINTNEXTLINE`, **une seule ligne**, précédée d'un
commentaire qui explique **pourquoi**.

> Un `// NOLINT` nu, sans justification, est un constat de revue.

---

## 2. Ce que dit la DO-178C

| Objectif | Intitulé | Application |
|---|---|---|
| **A-5.4** | Le code source est conforme aux standards | revue + clang-tidy + compilateur |
| **A-5.6** | *Accuracy and consistency* | R-04, R-05, R-06, R-08, R-09 |
| **A-1.x** | Les plans définissent les standards | le tableau §1.3 appartient au SDP |
| **A-9.x** | L'assurance qualité vérifie la conformité au processus | audits, revue des déviations |
| **§11.8** | *Software Code Standards* | document livrable |

---

## 3. Manipulation

```bash
.\build\debug\bin\demo_13-standards-codage.exe
```

```bash
.\build\debug\bin\tests_13-standards-codage.exe --verbose --req
```

---

## 4. Exercices

**4.1 — Corriger les douze violations**
Réécrivez `nonconforming.cpp` en respectant le standard §1.3, **sans regarder**
`bcd.cpp`. Puis comparez. Combien de vos corrections coïncident ? Lesquelles
diffèrent, et pourquoi ?

**4.2 — Ce que le compilateur voit**
Recompilez `nonconforming.cpp` avec `/W4 /WX` (retirez la dérogation du
`CMakeLists.txt`). Combien de violations sur douze le compilateur détecte-t-il ?
Puis réactivez clang-tidy : combien de plus ? Quelles violations ne sont
détectées par **aucun** outil ? Que conclure sur la place de la revue humaine ?

**4.3 — Écrire une déviation**
Vous devez utiliser une bibliothèque tierce qui expose une macro de type
fonction (violation R-03) et que vous ne pouvez pas modifier. Rédigez la fiche
de déviation complète : règle, portée, justification, analyse de risque, mesure
compensatoire, approbateur.

**4.4 — Étendre le standard**
Ajoutez trois règles au tableau §1.3, tirées de ce que vous avez appris dans
les modules 01 à 12. Pour chacune : l'énoncé, **comment elle se vérifie**, et
un exemple de violation. Une règle non vérifiable n'a pas sa place dans un
standard.

**4.5 — clang-format**
Cassez volontairement le formatage de `bcd.cpp` (indentation, longueur de
lignes), puis lancez `clang-format -i`. Comparez avec `git diff`. Combien de
temps de revue cet outil vous fait-il gagner sur un fichier de 500 lignes ?

---

## 5. Pour aller plus loin

* **JSF++ Coding Standards** (Lockheed Martin, 2005) — public et gratuit, la
  seule référence du domaine librement accessible. Lisez au moins la section
  sur les exceptions et celle sur l'héritage.
* MISRA C++:2023 — la référence actuelle, à demander à votre employeur.
* **C++ Core Guidelines** — <https://isocpp.github.io/CppCoreGuidelines/> —
  gratuit, moderne, et c'est la base de la moitié des règles clang-tidy.
* Documentation clang-tidy : <https://clang.llvm.org/extra/clang-tidy/checks/>

---

⬅️ [12 — Couplage données et contrôle](../12-couplage-donnees-controle/README.md) |
➡️ [14 — Configuration, qualité et outils](../14-configuration-qualite/README.md)
