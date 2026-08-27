# Module 14 — Gestion de configuration, assurance qualité et outils

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 13

Module principalement **documentaire** : c'est la partie du métier qui ne
s'apprend pas en écrivant du code, et celle sur laquelle un entretien
d'embauche vous distinguera immédiatement.

---

## Objectifs pédagogiques

1. Savoir ce qu'est une **baseline**, un **SCI**, un **SECI**.
2. Comprendre les catégories de contrôle **CC1 / CC2** et ce qui les distingue.
3. Utiliser Git conformément à la section 7 — et savoir ce qu'il ne couvre pas.
4. Distinguer **vérification** et **assurance qualité**.
5. Déterminer si un outil doit être **qualifié**, et à quel **TQL**.

---

## 1. Le cours

### 1.1 Les données de vie du logiciel (section 11)

Vingt documents constituent le dossier logiciel. Le module les liste dans
`life_cycle_data()`, avec leur catégorie de contrôle :

| Acronyme | Document | § | DAL A/B | DAL C/D |
|---|---|:--:|:--:|:--:|
| **PSAC** | Plan for Software Aspects of Certification | 11.1 | CC1 | CC1 |
| SDP | Software Development Plan | 11.2 | CC1 | CC2 |
| SVP | Software Verification Plan | 11.3 | CC1 | CC2 |
| SCMP | Software Configuration Management Plan | 11.4 | CC1 | CC2 |
| SQAP | Software Quality Assurance Plan | 11.5 | CC1 | CC2 |
| SRS | Software Requirements Standards | 11.6 | CC1 | CC2 |
| SDS | Software Design Standards | 11.7 | CC1 | CC2 |
| SCS | Software Code Standards | 11.8 | CC1 | CC2 |
| **SRD** | Software Requirements Data | 11.9 | CC1 | CC1 |
| SDD | Design Description | 11.10 | CC1 | CC2 |
| **SRC** | Source Code | 11.11 | CC1 | CC1 |
| **EOC** | Executable Object Code | 11.12 | CC1 | CC1 |
| SVCP | Software Verification Cases and Procedures | 11.13 | CC1 | CC2 |
| SVR | Software Verification Results | 11.14 | CC2 | CC2 |
| **SECI** | Software Life Cycle Environment Configuration Index | 11.15 | CC1 | CC1 |
| **SCI** | Software Configuration Index | 11.16 | CC1 | CC1 |
| SCR | Problem Reports | 11.17 | CC2 | CC2 |
| SCMR | Software Configuration Management Records | 11.18 | CC2 | CC2 |
| SQAR | Software Quality Assurance Records | 11.19 | CC2 | CC2 |
| **SAS** | Software Accomplishment Summary | 11.20 | CC1 | CC1 |

**Deux enseignements dans ce tableau :**

* **CC1 et CC2 ne classent pas l'importance d'un document.** Ils classent le
  **niveau de rigueur de son contrôle** :

  | | CC1 | CC2 |
  |---|:---:|:---:|
  | Identification | ✓ | ✓ |
  | Traçabilité des changements | ✓ | ✓ |
  | Protection contre modification non autorisée | ✓ | ✓ |
  | **Revue des changements** | ✓ | — |
  | **Contrôle des baselines** | ✓ | — |
  | **Archivage et restitution** | ✓ | — |
  | **Chargement contrôlé** | ✓ | — |

* **La catégorie dépend du niveau DAL.** Le SDD est CC1 en DAL A/B et CC2 en
  DAL C/D. Passer de DAL C à DAL B, ce n'est pas seulement plus de tests :
  c'est **plus de rigueur sur les mêmes documents**.

Six données restent CC1 à tous les niveaux : **PSAC, SRD, SRC, EOC, SECI,
SCI** — celles sans lesquelles on ne peut ni identifier, ni reconstruire le
produit.

### 1.2 SCI et SECI, générés depuis le dépôt

```bash
python tools/config_index.py
```

Deux fichiers sont écrits dans `reports/` :

| Fichier | Contenu |
|---|---|
| `SCI.md` | part number, version, commit, étiquette, **SHA-256 de chaque fichier suivi**, empreinte globale, procédure de reconstruction |
| `SECI.md` | système hôte, Visual Studio, CMake, Ninja, clang-tidy, Python, Git, **options de compilation**, statut de qualification de chaque outil |

L'outil refuse implicitement de produire un SCI « propre » si l'arbre de
travail est modifié : il l'écrit en toutes lettres — **NON BASELINABLE**. Un
SCI ne s'établit que sur un état figé.

> **Pourquoi tant d'insistance sur la reproductibilité ?** Parce qu'un
> aéronef vole 30 à 40 ans. Il faudra peut-être corriger une anomalie en 2050
> sur un logiciel compilé en 2026. Sans SCI et SECI complets, c'est
> impossible : on ne saurait plus quel compilateur, quelles options, quelles
> sources.

### 1.3 Git au service de la section 7

| Exigence DO-178C §7 | Moyen | Couvert par Git ? |
|---|---|:---:|
| Identification de configuration | commit SHA-1 / SHA-256 | ✓ |
| Établissement de baselines | `git tag` **signé** | ✓ |
| Traçabilité des changements | historique + message | ✓ |
| Contrôle des changements | revue de fusion obligatoire | ✓ |
| Archivage et restitution | dépôt miroir hors ligne | ✓ |
| **Rapports de problème (SCR)** | système de tickets **séparé** | ✗ |

Le sixième point est celui que Git ne couvre pas : le suivi des anomalies est
un processus à part entière (voir
[`templates/fiche-anomalie.md`](../../templates/fiche-anomalie.md)).

**Pratiques qui paient en audit :**

* un commit = **un changement atomique**, avec son motif ;
* le message référence l'exigence ou l'anomalie traitée ;
* les baselines sont des **étiquettes signées**, jamais des branches ;
* **aucune réécriture d'historique après baseline** — pas de `rebase`, pas de
  `push --force` : la piste d'audit doit rester intacte ;
* les binaires produits ne sont **pas** dans le dépôt : ils se reconstruisent
  à partir du SCI.

### 1.4 Intégrité du logiciel chargé

Le composant `mod14::verify_load()` illustre ce que fait tout calculateur
certifié au démarrage :

```cpp
if (!is_valid_part_number(identity.part_number)) { return false; }
if (image.empty())                               { return false; }
return crc32(image) == identity.expected_crc;
```

Trois vérifications, dans un **ordre spécifié** : identité, présence,
intégrité. Le chargement logiciel d'un équipement en atelier peut échouer
partiellement, la Flash peut se dégrader, un technicien peut charger la
mauvaise version. C'est la dernière barrière.

Le test `Crc32.detects_any_single_bit_alteration` bascule chacun des 72 bits
d'une image, un par un, et vérifie que le CRC change à chaque fois. C'est la
propriété qui fait d'un CRC un contrôle d'intégrité — et elle se démontre.

### 1.5 Vérification ≠ assurance qualité

C'est la distinction que beaucoup de candidats manquent en entretien :

| | Vérification (§6) | Assurance qualité (§8) |
|---|---|---|
| Question | *« Le logiciel est-il correct ? »* | *« Avons-nous suivi nos plans ? »* |
| Objet | le **produit** | le **processus** |
| Moyens | revues, analyses, tests | audits, revues de conformité |
| Livrables | SVR | SQAR, contribution au SAS |

Concrètement, la SQA :

* audite la conformité aux plans (PSAC, SDP, SVP, SCMP) ;
* vérifie que les revues ont eu lieu, **avec les bonnes personnes et
  l'indépendance requise** ;
* vérifie que les anomalies sont tracées et clôturées ;
* conduit la **conformity review** avant livraison ;
* dispose d'une **autorité et d'une indépendance organisationnelles**.

### 1.6 DO-330 : faut-il qualifier l'outil ?

> **La question n'est jamais « l'outil est-il bon ? »** mais :
> **« son résultat remplace-t-il une activité que la norme exige ? »**

**Trois critères** (DO-178C §12.2.1) :

| Critère | Description | Exemples |
|---|---|---|
| **1** | L'outil produit du code embarqué **sans que sa sortie soit vérifiée** | générateur de code qualifié, compilateur qualifié |
| **2** | L'outil automatise une vérification **et pourrait ne pas détecter une erreur** | outil de couverture, analyseur statique remplaçant une revue |
| **3** | L'outil permet de **réduire** une autre activité | générateur de cas de test |

**Cinq niveaux (TQL)** :

| TQL | Quand | Coût |
|---|---|---|
| **1–3** | Critère 1 (outils de développement), selon le DAL | un compilateur qualifié TQL-1 se compte en **millions d'euros** |
| **4** | Critères 2 ou 3, DAL A ou B | élevé |
| **5** | Critères 2 ou 3, DAL C ou D | modéré |

**Application aux outils de ce dépôt** :

| Outil | Statut | Pourquoi |
|---|---|---|
| `microtest` | non qualifié | complète la revue, ne la remplace pas |
| `clang-tidy` | non qualifié | n'élimine aucune activité exigée |
| `tools/trace_check.py` | non qualifié | complète la revue manuelle de la matrice |
| `tools/config_index.py` | non qualifié | produit une donnée, relue et approuvée |
| `OpenCppCoverage` | **TQL-5 si** son résultat remplace l'analyse manuelle de couverture | critère 2 |

> Le compilateur MSVC n'est **pas** qualifié — et c'est la situation normale.
> On ne qualifie pas le compilateur : on **vérifie sa sortie** (tests sur
> cible, couverture du code objet en DAL A). C'est précisément pourquoi la
> DO-178C exige des tests sur le **code exécutable**, et non sur le code
> source.

### 1.7 Les revues

Une revue DO-178C n'est pas une relecture informelle. Elle a :

* une **checklist** — voir [`templates/`](../../templates/) ;
* des **participants identifiés**, avec l'indépendance requise ;
* des **constats enregistrés**, classés par sévérité ;
* un **suivi de clôture** de chaque constat ;
* une **signature**.

Quatre checklists sont fournies dans ce dépôt :

| Fichier | Objectifs couverts |
|---|---|
| [`checklist-revue-exigences.md`](../../templates/checklist-revue-exigences.md) | A-3.x, A-4.x |
| [`checklist-revue-code.md`](../../templates/checklist-revue-code.md) | A-5.x |
| [`checklist-revue-tests.md`](../../templates/checklist-revue-tests.md) | A-7.1 à A-7.4 |
| [`fiche-deviation.md`](../../templates/fiche-deviation.md) | A-5.4, §11.8 |
| [`fiche-anomalie.md`](../../templates/fiche-anomalie.md) | §7.2.4, §11.17 |

---

## 2. Ce que dit la DO-178C

| Section / objectif | Sujet |
|---|---|
| **§7** | Processus de gestion de configuration |
| **§7.2.4** | Analyse de changement |
| **§7.2.7 et table 7-1** | Catégories CC1 / CC2 |
| **§8** | Processus d'assurance qualité |
| **§9** | Liaison avec la certification |
| **§11** | Données de vie du logiciel |
| **§12.2** et **DO-330** | Qualification des outils |
| **A-8.x** | Objectifs de gestion de configuration |
| **A-9.x** | Objectifs d'assurance qualité |
| **A-10.x** | Objectifs de liaison certification |

---

## 3. Manipulation

```bash
.\build\debug\bin\demo_14-configuration-qualite.exe
```

```bash
python tools/config_index.py --print
```

```bash
.\build\debug\bin\tests_14-configuration-qualite.exe --verbose --req
```

---

## 4. Exercices

**4.1 — Produire une baseline**
Validez tout votre travail, créez une étiquette annotée
`git tag -a v0.1-baseline -m "Baseline formation"`, puis régénérez le SCI.
Comparez avec le SCI précédent : que change la mention *NON BASELINABLE* ?

**4.2 — Analyse d'impact**
Une anomalie est remontée : *« l'alerte du module 10 ne retombe jamais quand
`clear_cycles` vaut 1 »*. Remplissez
[`fiche-anomalie.md`](../../templates/fiche-anomalie.md) **en entier**, y
compris la section 5 (analyse d'impact). Combien d'artefacts faut-il
re-vérifier ?

**4.3 — Qualifier ou non**
Pour chacun de ces outils, dites s'il faut le qualifier, sous quel critère et
à quel TQL, pour un projet **DAL B** :
a) un générateur de code depuis Simulink dont la sortie est relue ligne à ligne ;
b) le même, dont la sortie n'est pas relue ;
c) un outil qui mesure le MC/DC et dont le résultat est le seul élément de
   preuve de l'objectif A-7.5 ;
d) un script qui met en forme les rapports de test.

**4.4 — Revue de code réelle**
Prenez [`checklist-revue-code.md`](../../templates/checklist-revue-code.md) et
appliquez-la **intégralement** à `modules/12-.../src/chain.cpp`. Combien de
constats ? Combien de temps ? Extrapolez à 50 000 lignes.

**4.5 — Le SECI de votre poste**
Complétez `reports/SECI.md` avec les éléments que l'outil ne détecte pas :
version exacte du toolset MSVC (visible dans la sortie CMake), version du SDK
Windows, variables d'environnement influant sur le build. Que manque-t-il
encore pour reconstruire le binaire à l'identique dans dix ans ?

---

## 5. Pour aller plus loin

* DO-178C §7, §8, §9, §11 et §12.2.
* **DO-330** — *Software Tool Qualification Considerations*. Le supplément
  dédié ; sa lecture change la façon dont on choisit ses outils.
* CAST-13 — *Automatic Code Generation Tools Development Assurance*.
* EASA CM-SWCEH-002 — *Software Aspects of Certification* : le point de vue de
  l'autorité européenne, public et gratuit.

---

⬅️ [13 — Standards de codage](../13-standards-codage/README.md) |
➡️ [15 — Déterminisme et temps réel](../15-determinisme-temps-reel/README.md)
