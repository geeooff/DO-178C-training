# Contexte du projet

## Ce qu'est ce dépôt

Un **support d'auto-formation**, pas un produit. Il sert à un développeur
principalement **C#** à monter en compétences sur **C++** et sur l'impact de la
**DO-178C** sur la façon de développer, en vue d'une candidature dans le
logiciel embarqué avionique.

Cible marché : les donneurs d'ordre de l'aviation **française** — Thales,
Airbus, ATR, Safran, Dassault Aviation. Le code doit ressembler à ce qui se lit
en projet chez eux.

Conséquences directes sur toute contribution :

- Le lecteur est un développeur C# expérimenté mais **débutant en C++**.
  Expliciter les écarts avec C#, pas seulement la syntaxe C++.
- La valeur est **pédagogique** : les commentaires font partie du cours. Ils
  expliquent le *pourquoi* — quel objectif DO-178C, quel accident historique —
  jamais seulement le *quoi*.
- Tout doit rester **exécutable et vérifié**. Un exemple qui ne compile pas n'a
  aucune valeur ici.
- La formation prépare aussi l'**entretien d'embauche** : voir
  [`docs/04-entretien.md`](docs/04-entretien.md).

## Convention de langue — impérative

| En **anglais** | En **français** |
|---|---|
| Identifiants : types, fonctions, paramètres, variables | Commentaires de code |
| Noms de cas de test et de suites (`TEST_REQ(Suite, cas, …)`) | README, `docs/`, `requirements/`, `templates/` |
| | Prose des tables de traçabilité |
| | Messages de commit |

**Pourquoi :** le code doit coller au marché ; le support de formation reste en
français parce que son lecteur lit et applique plus vite dans cette langue.

Corollaires appris à l'usage :

- Un extrait ` ```cpp ` dans un document français n'est traduit **que s'il
  devient entièrement cohérent**. Quatre blocs purement illustratifs
  (`par_pointeur`, `formater_message`, `utiliser`) restent en français : un
  extrait à moitié traduit est moins lisible que l'original.
- Se méfier de la traduction mot à mot : elle suit l'ordre des mots français et
  **supprime les négations**. `decision_ne_suffit_pas` → `decision_suffices`
  affirme le contraire de ce que le test vérifie. Un nom de cas de test est
  cité par les exigences : il fait partie des données de vie, un nom faux est
  un défaut documentaire.

## Protocole de vérification — non négociable

Aucune modification n'est considérée terminée avant que **tout** ceci passe.
C'est ce qui donne au dépôt sa crédibilité ; ne jamais commiter « en attendant ».

```powershell
# Windows / MSVC
.\scripts\build.ps1 -Preset strict -Test
```

```bash
# Linux, macOS, WSL — les deux chaînes
./scripts/build.sh -p gcc-strict -t
./scripts/build.sh -p clang-strict -t
./scripts/build.sh -p asan -t          # ASan + UBSan
./scripts/coverage.sh                  # gcovr, instructions et branches
python tools/trace_check.py --strict   # matrice de traçabilité
clang-format --dry-run -Werror <fichiers>
```

Attendu : **0 avertissement**, **19/19 campagnes**, **0 défaut de traçabilité**,
**0 fichier non conforme** au format.

Le preset `strict` active `TRAINING_WARNINGS_AS_ERRORS` **et** clang-tidy.
Presets disponibles : `debug`, `release`, `strict`, `asan`, `vs2026` (Windows),
`gcc-strict`, `clang-strict`, `coverage` (non-Windows).

**Exceptions attendues, à ne pas « corriger » :**

- `modules/13-standards-codage/src/nonconforming.cpp` viole volontairement le
  standard de codage. Cible séparée, clang-tidy désactivé, avertissements
  relâchés. Les `D9025` de MSVC à la compilation viennent de là.
- `refs/original` a été purgé après la réécriture d'historique ; l'adresse
  d'auteur est l'adresse *noreply* GitHub.

## Environnements

Le dépôt doit se compiler et se tester **à l'identique** sur les trois chaînes.
Les presets CMake portent les mêmes noms partout.

| Environnement | Chaîne |
|---|---|
| Windows | Visual Studio 2026 Community, MSVC 14.51 |
| WSL Ubuntu 26.04 | GCC 15.2, Clang 21.1.8, CMake 4.2.3, Ninja |
| CI GitHub Actions | `ubuntu-26.04` (préversion), 5 jobs |
| devcontainer | `ubuntu:26.04` |

**Ne pas installer Docker** sur le poste de développement.

Sous Debian et Ubuntu, `libclang-rt-dev` est **obligatoire** pour les
sanitizers Clang : le métapaquet `clang` ne fournit pas les runtimes de
compiler-rt, la compilation passe et c'est l'édition de liens qui échoue.

## Pièges vérifiés, à ne pas redécouvrir

- **clang-cl définit `_MSC_VER` ET `__clang__`.** Toujours tester `__clang__` /
  `__GNUC__` **avant** `_MSC_VER`, sinon la mauvaise branche est prise.
- **Diagnostics GCC sous Windows** : clang-tidy peut les faire remonter avec
  `--checks="clang-diagnostic-*"` et `--extra-arg=/clang:-Wshadow`. Le préfixe
  `/clang:` est requis — sans lui, `-Wall` est lu comme `/Wall`, donc
  `-Weverything`. Utile quand aucun compilateur Linux n'est disponible.
- **`-Warray-bounds` n'apparaît qu'en compilation optimisée.** Les presets
  `strict` sont en `-O0` : passer aussi `release` avant de conclure.
- **Un invariant vrai « globalement » n'est pas exploitable localement** par le
  compilateur. Voir module 08, section 1.6, qui documente le cas et le conflit
  §6.4.4.3 / CAST-17 qui en découle.
- **`trace_check.py` est le filet de sécurité de tout renommage.** Il vérifie
  que les citations `` `Suite.cas` `` des documents désignent des tests réels.
  Le lancer *avant* et *après* toute opération de masse.

## Conventions Git

- Messages de commit **en français**, à l'impératif ou au présent descriptif.
  L'historique existant est majoritairement sans accents : c'est une
  inconstance, pas une règle. Écrire accentué, comme le reste de la
  documentation.
- Expliquer le **pourquoi** et le prix payé, pas seulement le quoi. Les
  messages de ce dépôt font partie de sa valeur pédagogique.
- **Un sujet par commit.** Ne jamais mélanger un renommage de masse avec une
  correction de fond : `git blame` doit rester exploitable.
- Ne rien pousser sans demande explicite.

## Hors périmètre, assumé

Tests sur cible réelle, couverture du code objet, analyse WCET réelle,
multicœur (CAST-32A), DO-331 (Simulink/SCADE), DO-333 (méthodes formelles),
rédaction complète des plans, relation avec l'autorité de certification.

Ces sujets sont **cités** là où ils s'insèrent, avec des références. Ne pas les
traiter sans demande explicite.

## Projet frère

`../DO-178C-training-ADA` — même démarche, mêmes objectifs, en **Ada/SPARK**.
Les modules de processus (09 à 12, 14) sont indépendants du langage et font
autorité ici ; le dépôt Ada les référence plutôt que de les réécrire.
