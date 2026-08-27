# Module 00 — Environnement, chaîne de compilation et vue d'ensemble DO-178C

> **Durée estimée** : 1 journée
> **Prérequis** : savoir programmer (C#), rien d'autre.

---

## Objectifs pédagogiques

À l'issue de ce module, vous saurez :

1. compiler et exécuter un projet C++ multi-fichiers avec CMake + MSVC ;
2. expliquer **pourquoi** C++ se compile différemment de C#, et quelles
   conséquences cela a au quotidien ;
3. situer la DO-178C dans le paysage de la certification aéronautique ;
4. nommer les 5 niveaux de criticité logicielle (DAL) et ce qu'ils changent ;
5. citer les grands processus de la norme et les tables d'objectifs qui les
   accompagnent.

---

## 1. La chaîne de compilation C++

### 1.1 Le modèle mental à changer

En C#, vous écrivez du code, le compilateur produit de l'IL, et le runtime
s'occupe du reste. Le compilateur voit **tout l'assembly d'un coup** : il
connaît chaque type, chaque méthode, et les métadonnées survivent dans le
binaire (c'est ce qui rend la réflexion possible).

En C++, rien de tout cela :

```
fichier.cpp
    │
    ├─► [préprocesseur]   #include, #define, #if : substitution de texte
    │        │
    │        ▼
    │   unité de traduction (translation unit) — un gros fichier texte
    │        │
    ├─► [compilateur]     analyse, optimisation, génération
    │        │
    │        ▼
    │   fichier.obj — code machine + table de symboles
    │
    └─► [éditeur de liens] résolution des symboles entre .obj et .lib
             │
             ▼
        programme.exe
```

Trois conséquences pratiques, immédiates :

| Symptôme | Cause | Équivalent C# |
|---|---|---|
| `C2065: identificateur non déclaré` | le **compilateur** ne connaît pas le nom : `#include` manquant | erreur de compilation |
| `LNK2019: unresolved external symbol` | le compilateur était content, mais **l'éditeur de liens** n'a trouvé aucune définition | ~ `TypeLoadException` au runtime |
| `LNK2005: symbol already defined` | violation de l'ODR : deux définitions du même symbole | duplication de classe |

La règle qui explique tout : **un `.hpp` contient des déclarations, un `.cpp`
contient des définitions**. Le `.hpp` dit *« cette fonction existe, voici sa
signature »*, le `.cpp` dit *« voici son code »*.

### 1.2 L'ODR (One Definition Rule)

Chaque entité doit avoir **exactement une** définition dans tout le programme.
D'où :

* les *include guards* (`#ifndef ... #define ... #endif`) : sans eux, inclure
  deux fois le même en-tête définit deux fois la même classe ;
* le mot-clé `inline` : autorise plusieurs définitions identiques (une par
  unité de traduction), c'est ce qui permet de définir des fonctions dans un
  en-tête ;
* le `namespace { }` anonyme : rend un symbole **local au fichier**, donc
  invisible de l'éditeur de liens. C'est l'équivalent d'un `private` de fichier.

### 1.3 Ce que fait CMake

CMake ne compile rien. Il **génère** les fichiers de build (Ninja, ou une
solution Visual Studio) à partir d'une description déclarative. On l'utilise
ici parce que c'est le standard de fait en C++, y compris chez les
équipementiers aéronautiques.

```
CMakeLists.txt  ──[cmake --preset debug]──►  build/debug/build.ninja
                                                    │
                                       [cmake --build --preset debug]
                                                    │
                                                    ▼
                                            build/debug/bin/*.exe
```

---

## 2. Vue d'ensemble de la DO-178C

### 2.1 Ce que c'est (et ce que ce n'est pas)

La **DO-178C** (« Software Considerations in Airborne Systems and Equipment
Certification », EUROCAE ED-12C en Europe) est le document que les autorités
— EASA, FAA — reconnaissent comme moyen acceptable de conformité pour le
logiciel embarqué à bord d'un aéronef.

Ce n'est **pas** :
* une méthode de développement (elle n'impose ni V, ni agile, ni MBD) ;
* un standard de codage (elle demande d'en avoir un, sans dire lequel) ;
* une norme de qualité produit au sens ISO 9001.

C'est une liste d'**objectifs** à satisfaire, avec pour chacun :
* qui doit le satisfaire selon le niveau de criticité ;
* avec quelle **indépendance** (l'auteur ne peut pas être son propre vérificateur) ;
* et quelles **données de vie du logiciel** (*life cycle data*) en apportent la
  preuve, avec quel niveau de contrôle de configuration.

> **Le mot-clé du domaine est : PREUVE.**
> Un code excellent mais sans preuve tracée ne passe pas. Un code moyen mais
> intégralement tracé, revu, testé et couvert passe. C'est le renversement
> culturel principal quand on vient du développement classique.

### 2.2 Les niveaux (DAL — Design Assurance Level)

Le niveau découle de l'**analyse de sécurité du système** (ARP4761), pas d'un
choix du développeur : on regarde ce qui se passe si le logiciel se comporte de
travers.

| DAL | Condition de panne | Effet | Objectifs (DO-178C) | dont avec indépendance |
|-----|--------------------|-------|--------------------:|------------------:|
| **A** | Catastrophique | perte de l'appareil | 71 | 30 |
| **B** | Dangereuse | blessés graves, forte réduction des marges | 69 | 18 |
| **C** | Majeure | inconfort, charge de travail accrue | 62 | 5 |
| **D** | Mineure | conséquences négligeables | 26 | 2 |
| **E** | Sans effet | aucun impact sur la sécurité | 0 | 0 |

Exemples concrets : commandes de vol électriques → A ; freinage → B ;
gestion carburant → B ou C ; FADEC moteur → A ; système de divertissement
en cabine → E.

La différence la plus coûteuse entre les niveaux tient en deux points :
* la **couverture structurelle** exigée (voir module 11) — le fameux **MC/DC**
  n'est requis qu'en DAL A ;
* l'**indépendance** entre celui qui produit et celui qui vérifie.

### 2.3 Les processus

```
                        ┌───────────────────────────┐
                        │   PROCESSUS DE PLANIFICATION  │  (section 4)
                        │   PSAC, SDP, SVP, SCMP, SQAP  │
                        └─────────────┬─────────────┘
                                      │
        ┌─────────────────────────────┴──────────────────────────────┐
        │        PROCESSUS DE DÉVELOPPEMENT (section 5)              │
        │                                                            │
        │  Exigences système                                         │
        │        │                                                   │
        │        ▼                                                   │
        │  Exigences de haut niveau (HLR) ──────► SRD                │
        │        │                                                   │
        │        ▼                                                   │
        │  Architecture + exigences de bas niveau (LLR) ──► SDD      │
        │        │                                                   │
        │        ▼                                                   │
        │  Code source ──────────────────────────► Source Code       │
        │        │                                                   │
        │        ▼                                                   │
        │  Code exécutable objet ────────────────► EOC               │
        └────────────────────────────┬───────────────────────────────┘
                                     │
     ┌───────────────────────────────┼─────────────────────────────┐
     │                               │                             │
┌────▼─────────────┐  ┌──────────────▼───────────┐  ┌──────────────▼──────┐
│  VÉRIFICATION    │  │  GESTION DE CONFIGURATION │  │  ASSURANCE QUALITÉ  │
│  (section 6)     │  │  (section 7)              │  │  (section 8)        │
│  revues,         │  │  baselines, traçabilité   │  │  audits, conformité │
│  analyses, tests │  │  des changements, CC1/CC2 │  │  au processus       │
└──────────────────┘  └───────────────────────────┘  └─────────────────────┘
                                     │
                         ┌───────────▼────────────┐
                         │  LIAISON CERTIFICATION │  (section 9)
                         │  PSAC ► ... ► SAS      │
                         └────────────────────────┘
```

### 2.4 Les tables d'objectifs (Annexe A)

C'est le cœur opérationnel de la norme. Dix tables, que vous verrez citées
partout dans le métier :

| Table | Sujet |
|-------|-------|
| A-1 | Processus de planification |
| A-2 | Processus de développement |
| A-3 | Vérification des **exigences de haut niveau** |
| A-4 | Vérification des **exigences de bas niveau et de l'architecture** |
| A-5 | Vérification du **code source** |
| A-6 | Vérification de l'**exécutable** (résultats des tests) |
| A-7 | Vérification de la **vérification** (couverture, dont MC/DC) |
| A-8 | Gestion de configuration |
| A-9 | Assurance qualité |
| A-10 | Liaison avec la certification |

Quand un collègue dit *« c'est un objectif A-7.4 »*, il parle de la couverture
MC/DC. Apprendre à lire ces tables est un investissement rentable.

### 2.5 Les suppléments

La DO-178C (2011) a introduit quatre suppléments qui **modifient** les objectifs
selon la technologie utilisée :

| Doc | Sujet | Pertinence pour nous |
|-----|-------|----------------------|
| **DO-330** | Qualification des outils | ★★★ — tout outil de dev ou de vérif |
| **DO-331** | Model-Based Development (Simulink, SCADE) | ★ |
| **DO-332** | **Technologies orientées objet** | ★★★ — c'est du C++ ! |
| **DO-333** | Méthodes formelles | ★★ |

La **DO-332** est celle qui vous concernera directement en C++ : elle traite de
l'héritage, du polymorphisme, de la gestion dynamique de la mémoire, du typage
et des exceptions. Elle fait l'objet du **module 05**.

---

## 3. Le SECI, ou pourquoi ce module existe

La DO-178C §11.16 impose de produire un **Software Life Cycle Environment
Configuration Index** : la liste exacte et versionnée de tout ce qui a servi à
produire et vérifier le logiciel.

* compilateur + **numéro de version exact** + **toutes les options** ;
* éditeur de liens, options, script d'édition de liens ;
* système d'exploitation hôte ;
* outils de test, de couverture, d'analyse ;
* et de quoi **reconstruire le binaire à l'identique** dans dix ans.

Pourquoi si strict ? Parce qu'un changement de version de compilateur peut
changer le code généré, donc invalider la couverture structurelle déjà obtenue.
Sur un programme certifié, on reste souvent sur un compilateur « ancien »
pendant toute la vie du produit — c'est un choix assumé, pas de la négligence.

C'est ce que matérialise le fichier
[`build_info.hpp`](include/mod00/build_info.hpp) : identifier précisément
l'environnement, depuis le code lui-même.

---

## 4. Manipulation

Depuis un PowerShell ordinaire, à la racine du dépôt :

```bash
.\scripts\build.ps1 -Preset debug -Test
```

Puis lancez la démonstration :

```bash
.\build\debug\bin\demo_00-environnement.exe
```

Et les tests seuls, avec le détail :

```bash
.\build\debug\bin\tests_00-environnement.exe --verbose --req
```

L'option `--req` affiche la **matrice de traçabilité** exigence → cas de test.
Regardez-la : c'est le genre de tableau qu'un auditeur demande.

---

## 5. Exigences du module

Format volontairement identique à celui d'un vrai document de conception.

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M00-001 | `current_build()` doit renvoyer un identifiant de compilateur non vide. | `BuildInfo.compilateur_identifie` |
| LLR-M00-002 | Le code doit être compilé selon la norme C++17 au minimum. | `BuildInfo.norme_cpp17_minimum` |
| LLR-M00-003 | `cpp_standard_name()` doit renvoyer le nom de norme correspondant à la valeur `__cplusplus` fournie, et une chaîne par défaut pour toute valeur antérieure à C++11. | `BuildInfo.nom_de_norme_par_intervalle`, `BuildInfo.robustesse_valeur_hors_domaine` |
| LLR-M00-004 | `current_build().pointer_bits` doit valoir 32 ou 64 et être cohérent avec `sizeof(void*)`. | `BuildInfo.largeur_pointeur_coherente` |
| LLR-M00-005 | `current_build().little_endian` doit refléter le boutisme réel de la machine. | `BuildInfo.boutisme_coherent` |

---

## 6. Exercices

**6.1 — Provoquer les trois erreurs**
Faites apparaître volontairement, puis corrigez :
a) une erreur `C2065` (oubli d'un `#include`) ;
b) une erreur `LNK2019` (déclarez une fonction dans le `.hpp`, ne la définissez
   pas, appelez-la depuis `main.cpp`) ;
c) une erreur de double inclusion (retirez les *include guards* de
   `build_info.hpp` et incluez-le deux fois).
Notez le message exact de chaque erreur : savoir les lire fait gagner des heures.

**6.2 — Compléter le SECI**
Créez `docs/seci-poste-local.md` et remplissez-y : version exacte du toolset
MSVC (visible dans la sortie CMake), version de CMake, version de Ninja,
version de Windows, options de compilation appliquées (lisez
`cmake/TrainingHelpers.cmake`). C'est un livrable réel de la norme.

**6.3 — Empreinte du dépôt**
`__DATE__` / `__TIME__` rendent le binaire non reproductible. Proposez (par
écrit, dans le même fichier) une manière d'injecter le hash du commit Git à la
place, et expliquez en quoi c'est meilleur pour la certification.

**6.4 — Lecture**
Repérez dans le tableau §2.2 le niveau que vous viseriez pour : un pilote
automatique, un calculateur de pression cabine, une application de préparation
de vol sur tablette (EFB) non certifiée. Justifiez en une phrase chacun.

---

## 7. Pour aller plus loin

* DO-178C, sections 1 à 4 et Annexe A (le document lui-même s'achète auprès de
  la RTCA ou de l'EUROCAE — la plupart des employeurs vous le fournissent).
* FAA AC 20-115D : l'avis de la FAA qui reconnaît la DO-178C.
* CAST papers (Certification Authorities Software Team) : notes de position
  publiques et gratuites, très éclairantes (CAST-6 sur le code mort, CAST-10 sur
  la traçabilité, CAST-12 sur le code désactivé).

---

➡️ Module suivant : [01 — Types, valeurs et mémoire](../01-types-et-memoire/README.md)
