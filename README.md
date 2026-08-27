# C++ et DO-178C — Formation par la pratique

> Une montée en compétences complète, pour un développeur **C#** qui vise le
> logiciel embarqué **avionique**.
>
> 17 modules · ~20 journées · 19 campagnes de test · 2 outils de vérification ·
> un projet intégré avec son dossier de certification.

---

## Le principe

Cette formation ne sépare pas « apprendre le C++ » et « apprendre la
DO-178C ». Chaque module associe **une notion du langage** et **l'objectif de
certification qui la contraint**. Vous n'apprenez pas seulement *qu'*une
règle existe, mais **pourquoi** — et c'est cela qui fait la différence en
entretien comme en poste.

> **Le mot-clé du domaine est : PREUVE.**
> Un code excellent sans preuve tracée ne passe pas. Un code moyen mais
> intégralement tracé, revu, testé et couvert passe. C'est le renversement
> culturel principal quand on vient du développement classique.

Tout est **exécutable** : chaque module compile, s'exécute et se teste. Les
deux outils Python (traçabilité, index de configuration) fonctionnent sur le
dépôt lui-même.

---

## Démarrage immédiat

**Windows** — depuis n'importe quel PowerShell, à la racine du dépôt :

```bash
.\scripts\build.ps1 -Preset debug -Test
```

**Linux, WSL, macOS** :

```bash
./scripts/build.sh -t
```

**Devcontainer** — ouvrez le dossier dans VS Code, puis *Reopen in Container*.

Lancez ensuite le premier module, puis le projet final pour voir où l'on va :

```bash
./build/debug/bin/demo_00-environnement
```

```bash
./build/debug/bin/demo_16-projet-integre
```

(ajoutez `.exe` sous Windows)

Sous Windows, **rien d'autre à installer** : Visual Studio 2026 Community
fournit le compilateur, CMake, Ninja, clang-tidy et clang-format. Sous Linux et
macOS, le script indique la commande d'installation de ce qui manque. Voir
[docs/03-outils.md](docs/03-outils.md).

---

## Les 17 modules

### Partie 0 — Mise en route

| # | Module | C++ | DO-178C |
|:-:|---|---|---|
| **00** | [Environnement](modules/00-environnement/) | chaîne de compilation, CMake, ODR | vue d'ensemble, DAL, tables A-1 à A-10, SECI |

### Partie 1 — Le langage, vu par un développeur C#

| # | Module | C++ | DO-178C |
|:-:|---|---|---|
| **01** | [Types et mémoire](modules/01-types-et-memoire/) | largeurs fixes, promotions, débordement, `enum class`, padding | A-5.6 — *accuracy and consistency*, **Ariane 5** |
| **02** | [Pointeurs et `const`](modules/02-pointeurs-references-const/) | référence vs pointeur, const-correctness, `Span`, durée de vie | code défensif, dead code, freedom from interference |
| **03** | [RAII](modules/03-raii-cycle-de-vie/) | ctor/dtor, règle de 0/3/5, sémantique de déplacement | libération déterministe, DO-332 OO.6.8.2 |
| **04** | [Classes et invariants](modules/04-classes-invariants/) | invariants, `explicit`, fabriques, **types forts** | A-4.1, **Air Canada 143**, Mars Climate Orbiter |
| **05** | [Polymorphisme](modules/05-polymorphisme-do332/) | `virtual`, vtable, découpage, LSP | **DO-332 OO.6.7** — cohérence locale de type |
| **06** | [Templates et `constexpr`](modules/06-templates-constexpr/) | templates, CRTP, calcul à la compilation | **couverture par instanciation** |
| **07** | [Erreurs sans exceptions](modules/07-erreurs-sans-exceptions/) | `Result<T>`, décodeur **ARINC 429** | pourquoi pas d'exceptions, code mort / désactivé |
| **08** | [Mémoire statique](modules/08-memoire-statique/) | `StaticVector`, réserve de blocs O(1), pile | DO-332 OO.6.8.2, analyse de pile, budget mémoire |

### Partie 2 — Les processus

| # | Module | Contenu | DO-178C |
|:-:|---|---|---|
| **09** | [Exigences et traçabilité](modules/09-exigences-tracabilite/) | HLR/LLR, exigences dérivées, **`trace_check.py`** | A-3.x, A-4.x, A-5.5, §5.1.2.h |
| **10** | [Tests basés sur les exigences](modules/10-tests-bases-exigences/) | équivalence, valeurs limites, états, **mutation** | A-6.x, §6.4.2, indépendance |
| **11** | [Couverture structurelle](modules/11-couverture-structurelle/) | statement, decision, **MC/DC + analyseur** | **A-7.5 à A-7.9**, §6.4.4.3 |
| **12** | [Couplage données/contrôle](modules/12-couplage-donnees-controle/) | matrices de couplage, tests d'intégration instrumentés | **A-7.8**, CAST-19 |
| **13** | [Standards de codage](modules/13-standards-codage/) | MISRA, clang-tidy, **déviations justifiées** | A-5.4, §11.8 |
| **14** | [Configuration et qualité](modules/14-configuration-qualite/) | baselines, CC1/CC2, **`config_index.py`** | §7, §8, §11, §12.2, **DO-330** |

### Partie 3 — Contraintes de l'embarqué

| # | Module | Contenu | DO-178C |
|:-:|---|---|---|
| **15** | [Déterminisme et temps réel](modules/15-determinisme-temps-reel/) | IEEE-754, virgule fixe, **ARINC 653**, WCET, `volatile` | A-5.6, §6.3.4.f, CAST-32A |

### Partie 4 — Synthèse

| # | Module | Contenu |
|:-:|---|---|
| **16** | [**Projet intégré — FQMS**](modules/16-projet-integre/) | système de gestion carburant DAL B, avec SRD, SDD, deux campagnes de test, traçabilité, matrices de couplage |

---

## Documentation transverse

| Document | Contenu |
|---|---|
| [Plan de formation](docs/00-plan-de-formation.md) | progression, rythmes, ce que vous saurez faire — et ce qui n'est pas couvert |
| [Glossaire](docs/01-glossaire.md) | tous les acronymes du métier, et l'anglais utile |
| [**Antisèche C# → C++**](docs/02-csharp-vers-cpp.md) | correspondances, pièges, ce que vous allez regretter et apprécier |
| [Outils](docs/03-outils.md) | installation, commandes, synchronisation multi-postes, diagnostic |
| [**Préparation aux entretiens**](docs/04-entretien.md) | les questions réelles et comment y répondre |
| [Ressources](docs/05-ressources.md) | bibliographie, en priorisant le gratuit |

---

## Structure du dépôt

```
├── modules/            17 modules : README, include/, src/, tests/, requirements/
├── common/             microtest (harnais sans allocation) + avio (types, Span, assertions)
├── tools/              trace_check.py, config_index.py
├── scripts/            build.ps1 / build.sh, coverage.ps1 / coverage.sh
├── templates/          checklists de revue, fiches de déviation et d'anomalie
├── docs/               documentation transverse
├── cmake/              fonctions du build
├── .devcontainer/      environnement Ubuntu figé (le SECI, en exécutable)
└── .github/workflows/  intégration continue Linux (GCC et Clang)
```

Chaque module suit la même convention :

```
modules/NN-nom/
├── README.md           le cours du jour, avec exercices
├── include/modNN/      en-têtes publics
├── src/                implémentation + un exécutable de démonstration
├── tests/              campagne de test tracée aux exigences
├── requirements/       SRD et SDD (modules 09 à 16)
└── CMakeLists.txt
```

---

## L'outillage produit dans la formation

Ce ne sont pas des jouets : ils fonctionnent sur ce dépôt et produisent des
artefacts réels.

### `microtest` — harnais de test

Écrit dans le module 00. **Aucune allocation dynamique, aucune exception,
~300 lignes auditables.** Chaque cas de test porte sa traçabilité :

```cpp
TEST_REQ(Limits, exact_threshold_does_not_trigger, "LLR-ALERT-020") { … }
```

```bash
.\build\debug\bin\tests_16-projet-integre.exe --verbose --req
```

> Pourquoi pas GoogleTest ? Parce qu'un harnais de test est un **outil de
> vérification** au sens DO-330. Qualifier GoogleTest est un chantier ;
> beaucoup d'équipes avioniques écrivent le leur, volontairement minuscule.
> C'est l'esprit du domaine (module 00).

### `trace_check.py` — matrice de traçabilité

```bash
python tools/trace_check.py
```

Reconstruit la traçabilité **bidirectionnelle** à partir des fichiers
d'exigences, des annotations `@satisfies` et des `TEST_REQ`. Signale les quatre
défauts : exigence sans code, exigence sans test, référence inconnue, test
orphelin. Identifie les **exigences dérivées**.

> Il a trouvé six vrais manques pendant l'écriture de ce dépôt. C'est
> exactement son rôle.

### `config_index.py` — SCI et SECI

```bash
python tools/config_index.py
```

Produit les deux documents §11.15 et §11.16 : part number, commit, SHA-256 de
chaque fichier, versions de la chaîne d'outils, options de compilation, statut
de qualification de chaque outil.

### `mcdc.cpp` — analyseur MC/DC

Module 11. Recherche les **paires d'indépendance** et démontre qu'un jeu de
tests atteint — ou n'atteint pas — le MC/DC. Avec le contre-exemple qui
compte : 100 % de couverture de décision, 0 % de MC/DC.

### `coupling_trace.hpp` — démonstration du couplage

Module 12. Enregistre chaque appel inter-composants et chaque donnée échangée,
**sans modifier le code de production** (substitution par paramètre de
template). Le binaire vérifié reste le binaire embarqué.

---

## Trois plateformes, trois chaînes, un seul comportement

Le dépôt se compile et se teste à l'identique avec **MSVC**, **GCC** et
**Clang**, sous Windows, Linux, WSL et macOS. Les presets CMake portent les
mêmes noms partout ; seul le lanceur change (`build.ps1` ou `build.sh`).

Ce n'est pas du confort : **chaque compilateur détecte ce que les autres
laissent passer**. MSVC `/W4` ne voit pas ce que GCC `-Wconversion` voit. Faire
tourner les deux, c'est deux analyses statiques pour le prix d'une.

| Preset | Où | Rôle |
|---|---|---|
| `debug` / `release` | partout | travail quotidien |
| `strict` | partout | **warnings = erreurs + clang-tidy** |
| `asan` | partout | sanitizers — rend le module 01 **tangible** |
| `gcc-strict` / `clang-strict` | Linux, macOS | force un compilateur |
| `coverage` | Linux, macOS | couverture instructions **et branches** |
| `vs2026` | Windows | solution `.sln` |

> ⚠️ **Ne pas confondre.** Compiler sur trois chaînes ne dispense de rien : le
> **SECI** (modules 00 et 14) fige **une** chaîne, **une** version, **un** jeu
> d'options. La portabilité est un outil de qualité pendant le développement,
> pas une propriété du produit certifié.

Le [devcontainer](.devcontainer/Dockerfile) est l'illustration la plus concrète
du SECI : un environnement de production figé, versionné et **reconstructible**.

## Vérifier que tout fonctionne

```bash
.\scripts\build.ps1 -Preset strict -Test
```

```bash
./scripts/build.sh -p strict -t
```

Le preset `strict` active **warnings = erreurs** et **clang-tidy**. Le dépôt
compile sans un seul avertissement, et les 19 campagnes passent.

```bash
python tools/trace_check.py
```

Zéro défaut de traçabilité. Les *observations* restantes sont documentées et
constituent un exercice (module 16 §7).

L'[intégration continue](.github/workflows/ci.yml) rejoue tout cela sous Linux
à chaque poussée, avec GCC **et** Clang, plus les sanitizers, la couverture et
la vérification du formatage.

---

## Conventions

- **Langue** : français dans les README, les commentaires et les noms de tests.
  Les identifiants de code sont en anglais, comme dans l'industrie.
- **Accents** : absents des fichiers `.cpp`/`.hpp` (portabilité des chaînes de
  compilation embarquées), présents dans les Markdown.
- **Norme** : C++17, cible de MISRA C++:2023 et choix réaliste en avionique.
- **Style** : imposé par `.clang-format`, vérifié par `.clang-tidy`.

---

## Ce que cette formation ne couvre pas

Par honnêteté (le détail est dans le [plan de formation](docs/00-plan-de-formation.md)) :
tests sur cible réelle, couverture du code objet, analyse WCET réelle,
multicœur (CAST-32A), DO-331 (Simulink/SCADE), DO-333 (méthodes formelles),
rédaction complète des plans, relation avec l'autorité de certification.

Ces sujets sont **cités** là où ils s'insèrent, avec des références. Savoir
qu'ils existent et où ils s'appliquent est déjà beaucoup.

---

## Par où commencer

1. Lisez le [plan de formation](docs/00-plan-de-formation.md) — 10 minutes.
2. Gardez l'[antisèche C# → C++](docs/02-csharp-vers-cpp.md) ouverte en
   permanence.
3. Attaquez le [module 00](modules/00-environnement/).
4. **Faites les exercices.** Ce sont eux qui construisent la compétence — et
   qui alimenteront vos réponses en entretien.

Bon vol.
