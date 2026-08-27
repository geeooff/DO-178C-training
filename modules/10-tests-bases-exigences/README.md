# Module 10 — Tests basés sur les exigences

> **Durée estimée** : 1 à 2 journées
> **Prérequis** : modules 00 à 09

---

## Objectifs pédagogiques

1. Distinguer **tests normaux** et **tests de robustesse** au sens DO-178C.
2. Appliquer les quatre techniques de conception : classes d'équivalence,
   valeurs limites, couverture d'états et de transitions, séquences.
3. Concevoir une campagne complète pour une **machine à états**.
4. Vérifier qu'un test **teste vraiment**, par analyse de mutation.
5. Comprendre l'exigence d'**indépendance** entre auteur du code et auteur des
   tests.

---

## 1. Le cours

### 1.1 Le principe fondateur

> **En DO-178C, on ne teste pas le code. On teste les exigences.**

La différence n'est pas rhétorique :

| Test dérivé du **code** | Test dérivé des **exigences** |
|---|---|
| « cette fonction a un `if`, testons les deux branches » | « l'exigence dit *strictement supérieur*, testons la valeur exacte du seuil » |
| reproduit les erreurs du code | détecte les erreurs du code |
| ne détecte pas le comportement **manquant** | détecte le comportement manquant |
| oriente vers 100 % de couverture… d'un code faux | oriente vers la satisfaction du besoin |

La couverture structurelle (module 11) sert à mesurer **si les tests basés sur
les exigences ont bien tout exercé** — pas à concevoir les tests. C'est une
inversion que beaucoup font, et qui coûte cher.

### 1.2 Normal *versus* robustesse

DO-178C §6.4.2 impose **les deux** familles :

| | Tests **normaux** (§6.4.2.1) | Tests de **robustesse** (§6.4.2.2) |
|---|---|---|
| Entrées | domaine valide | hors domaine, aux limites, anormales |
| Objectif | le logiciel fait ce qu'il doit | le logiciel réagit correctement à l'imprévu |
| Exemples ici | rampe de montée, cycle complet | NaN, ±∞, seuils inversés, `confirm_cycles = 0` |

En pratique, **la moitié d'une campagne est constituée de tests de robustesse**
— et c'est cette moitié qui trouve les vrais défauts.

### 1.3 Les quatre techniques

#### a) Classes d'équivalence

Partitionner le domaine d'entrée en classes produisant un comportement
**qualitativement identique**, puis prendre **un** représentant par classe.

Pour `ALERT-MON` (seuil 100, retombée 90) :

| Classe | Domaine | Comportement |
|---|---|---|
| C1 | `v > 100` | contribue à la montée |
| C2 | `90 ≤ v ≤ 100` | **zone morte** (hystérésis) : ne contribue à rien |
| C3 | `v < 90` | contribue à la retombée |

Tester 150 *puis* 200 n'apporte rien : même classe. C2 est la plus
intéressante — c'est exactement ce que l'hystérésis doit produire.

#### b) Analyse des valeurs limites

Les défauts se concentrent aux frontières. Pour chaque seuil : la valeur
**exacte**, juste en dessous, juste au-dessus.

```cpp
monitor.update(100.000F);   // -> Inactive : la condition est "> 100"
monitor.update(100.001F);   // -> Pending
```

Un `>` écrit `>=` par erreur — le défaut le plus fréquent du métier — ne se
voit **que là**.

Les limites portent aussi sur les **paramètres** : `confirm_cycles = 1`
court-circuite l'état `Pending`, c'est un chemin de code distinct.

#### c) Couverture des états et des transitions

```
                 valeur > seuil_montee
        ┌──────────────────────────────────►┐
        │                                    │
  ┌─────┴──────┐                      ┌──────▼──────┐
  │  INACTIVE  │◄─────────────────────┤   PENDING   │
  └─────▲──────┘   annulation          └──────┬──────┘
        │                                    │ N cycles consécutifs
        │   M cycles consécutifs             ▼
        │                             ┌─────────────┐
        └─────────────────────────────┤   ACTIVE    │
                              ┌───────┤             │
                              │       └──────▲──────┘
                              ▼              │ annulation
                       ┌─────────────┐       │
                       │  CLEARING   ├───────┘
                       └─────────────┘
```

**Atteindre les 4 états ne suffit pas.** Les défauts se cachent dans les
transitions d'**annulation** — `Pending → Inactive` et `Clearing → Active` —
qui sont aussi les plus souvent oubliées. Ce sont elles qui garantissent que
le comptage porte sur des cycles **consécutifs**.

#### d) Séquences

Enchaînements réalistes, qui détectent ce que les tests unitaires par
transition laissent passer :

| Séquence | Ce qu'elle vérifie |
|---|---|
| Bruit (un cycle sur deux au-dessus) | l'anti-rebond filtre bien |
| Oscillation autour du seuil bas | l'hystérésis tient, et le compteur d'activations ne gonfle pas |
| Rampe montante puis descendante | les délais de confirmation sont respectés |
| Capteur intermittent (NaN) | l'état ne dérive pas |

### 1.4 Le test du test : l'analyse de mutation

> Un test qui passe ne prouve rien s'il passerait **aussi** avec un code faux.

Technique : introduire volontairement un défaut (une **mutation**) et vérifier
qu'**au moins un test échoue**. Si aucun n'échoue, la campagne a un trou.

Mutations à essayer sur `alert_monitor.cpp` :

| # | Mutation | Test censé la détecter |
|---|---|---|
| M1 | `sample > raise` → `sample >= raise` | `Limits.exact_raise_threshold_does_not_trigger` |
| M2 | supprimer `progress_ = 0U` dans l'annulation `Pending` | `Transitions.pending_to_inactive_cancellation` |
| M3 | incrémenter `activations_` aussi depuis `Clearing` | `Sequences.oscillation_in_dead_band` |
| M4 | `>= confirm_cycles` → `> confirm_cycles` | `Transitions.pending_to_active` |
| M5 | traiter un NaN comme une valeur sous le seuil | `Robustness.non_finite_during_active_alert` |

C'est l'exercice 5.1, et c'est la meilleure façon d'apprendre à écrire des
tests qui servent à quelque chose.

### 1.5 L'indépendance

Les tables A-6 et A-7 de la DO-178C comportent une colonne **« with
independence »**. Concrètement, en DAL A et B :

* la personne qui **écrit** le code ne peut pas être celle qui **vérifie** que
  le code satisfait ses exigences ;
* l'indépendance peut être **organisationnelle** (une autre personne) ou
  **outillée** (un outil qualifié).

Pourquoi ? Parce qu'on teste ce qu'on a écrit, pas ce qui était demandé. Un
auteur qui a mal compris l'exigence écrira un test qui confirme sa mauvaise
compréhension. Ce n'est pas une question de compétence, c'est un biais
cognitif documenté.

En DAL C, l'indépendance est requise sur moins d'objectifs ; en DAL D, presque
plus.

### 1.6 Procédures, cas et résultats

La DO-178C distingue trois artefacts (§11.13 et §11.14) :

| Artefact | Contenu | Ici |
|---|---|---|
| **Cas de test** (*test case*) | entrées, résultats attendus, exigence tracée | un `TEST_REQ(...)` |
| **Procédure de test** | comment exécuter, dans quel environnement | `ctest --preset debug` + le SECI |
| **Résultats de test** | ce qui s'est réellement passé, archivé | sortie de CTest, `Testing/Temporary/LastTest.log` |

Les résultats doivent être **reproductibles** et **archivés sous contrôle de
configuration**. Un test qui dépend de l'ordre d'exécution, de l'horloge ou
d'un état résiduel n'est pas une preuve.

> C'est pourquoi chaque cas de test de ce module reconstruit son moniteur
> depuis zéro : aucun état ne fuit d'un cas à l'autre.

---

## 2. Ce que dit la DO-178C

| Objectif | Intitulé | Application |
|---|---|---|
| **A-6.1 / A-6.2** | L'exécutable satisfait les HLR (normal et robustesse) | suites de haut niveau |
| **A-6.3 / A-6.4** | L'exécutable satisfait les LLR (normal et robustesse) | suites `Transitions`, `Limites`, `Robustesse` |
| **A-6.5** | L'exécutable est compatible avec la cible | hors périmètre de cette formation |
| **A-7.1** | Les procédures de test sont correctes | revue des cas de test |
| **A-7.2** | Les résultats de test sont corrects, les écarts expliqués | sortie CTest archivée |
| **§6.4.2.1 / §6.4.2.2** | Tests normaux et de robustesse | organisation des suites |
| **§6.4.4.2** | Analyse de couverture des exigences | `trace_check.py` |

---

## 3. Manipulation

```bash
.\build\debug\bin\demo_10-tests-bases-exigences.exe
```

```bash
.\build\debug\bin\tests_10-tests-bases-exigences.exe --verbose --req
```

```bash
python tools/trace_check.py
```

---

## 4. Exigences du module

Elles sont dans [`requirements/srd.md`](requirements/srd.md) (7 HLR) et
[`requirements/sdd.md`](requirements/sdd.md) (10 LLR). Deux exigences dérivées
y sont identifiées : `HLR-ALERT-007` (traitement des échantillons non finis) et
`LLR-ALERT-051` (compteur de rejets).

`LLR-ALERT-051` mérite qu'on s'y arrête : sans compteur, un capteur qui n'émet
que des NaN laisserait l'alerte **éternellement inactive** sans que personne ne
s'en aperçoive. Le compteur transforme un comportement silencieux en un
comportement observable. C'est une **exigence de sécurité**, et c'est
exactement le genre d'exigence dérivée que le processus système doit examiner.

---

## 5. Exercices

**5.1 — Analyse de mutation (l'exercice central)**
Appliquez les cinq mutations du tableau §1.4, une par une. Pour chacune :
recompilez, lancez les tests, notez **quel** test échoue et **avec quel
message**. Puis restaurez le code. Si une mutation ne fait échouer aucun test,
écrivez le cas de test manquant.

**5.2 — Une exigence de plus**
Le système demande maintenant : *« l'alerte doit être inhibée pendant les
30 premiers cycles suivant la mise sous tension »* (inhibition au démarrage,
pratique courante pour éviter les alertes transitoires à l'allumage).
Rédigez la HLR, les LLR, implémentez, testez. Combien de nouveaux états ou
transitions cela ajoute-t-il ? Combien de cas de test ?

**5.3 — Table de décision**
Construisez la table de décision complète de `update()` :
états (4) × classes d'entrée (3) = 12 combinaisons. Pour chacune, notez l'état
résultant attendu. Comparez avec les tests existants : toutes les cases
sont-elles couvertes ? (Indice : il en manque au moins deux.)

**5.4 — Test de non-régression**
Un défaut est remonté du terrain : *« l'alerte reste active alors que la valeur
est revenue à la normale depuis longtemps »*. Écrivez le cas de test qui
reproduirait ce défaut **avant** de chercher la cause. Puis vérifiez si le code
actuel le passe. C'est la démarche imposée en maintenance certifiée : pas de
correction sans test de non-régression.

**5.5 — Indépendance**
Relisez les tests de ce module en vous demandant, pour chacun : *« l'aurais-je
écrit ainsi si je n'avais vu que les exigences, jamais le code ? »*. Identifiez
au moins un test qui trahit une connaissance de l'implémentation. Comment le
réécririez-vous ?

---

## 6. Pour aller plus loin

* DO-178C §6.4 (processus de test) et §6.5 (analyse des résultats).
* CAST-17 — *Structural Coverage of Object Code*.
* ISTQB Foundation, chapitre sur les techniques de conception de tests : le
  vocabulaire y est le même (classes d'équivalence, valeurs limites, tables de
  décision, transitions d'état).
* *Mutation testing* : la littérature académique est abondante et directement
  applicable.

---

⬅️ [09 — Exigences et traçabilité](../09-exigences-tracabilite/README.md) |
➡️ [11 — Couverture structurelle et MC/DC](../11-couverture-structurelle/README.md)
