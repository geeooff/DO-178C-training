# Module 15 — Déterminisme, arithmétique flottante et temps réel

> **Durée estimée** : 1 à 2 journées
> **Prérequis** : modules 00 à 14

Ce module rassemble les contraintes **temporelles et numériques** de
l'embarqué critique. C'est aussi celui où l'on comprend enfin **pourquoi**
toutes les interdictions des modules précédents existent.

---

## Objectifs pédagogiques

1. Connaître les trois propriétés d'IEEE-754 qui piègent tout le monde.
2. Choisir entre flottant et **virgule fixe** en connaissance de cause.
3. Comprendre l'**ordonnancement à fenêtres fixes** (ARINC 653) et la
   *freedom from interference*.
4. Savoir ce qu'est un **WCET** et ce qui le rend calculable.
5. Utiliser `volatile` correctement — et savoir quand il ne suffit pas.

---

## 1. Arithmétique flottante

### 1.1 Trois faits, démontrés par des tests

Le flottant IEEE-754 n'est pas « imprécis » : il est parfaitement défini. Le
problème est ailleurs.

#### a) La précision dépend de la magnitude

| Valeur | ULP (écart au flottant suivant) |
|---:|---:|
| 1,0 | 1,19 × 10⁻⁷ |
| 1 024,0 | 1,22 × 10⁻⁴ |
| 65 536,0 | 7,81 × 10⁻³ |
| 8 388 608,0 (2²³) | 1,0 |
| 16 777 216,0 (2²⁴) | **2,0** |

Conséquence — l'**absorption** :

```cpp
16777216.0F + 1.0F == 16777216.0F   // VRAI
```

Et elle n'est pas réservée aux grands nombres : `1.0F + 1e-9F == 1.0F` aussi.
Ce qui compte est le **rapport** entre les deux opérandes.

> **En pratique** : un compteur de temps en `float` incrémenté de 10 ms à
> 100 Hz **cesse de progresser** au bout de ~48 jours de fonctionnement
> continu. Ce défaut n'apparaît jamais en test.

#### b) L'addition n'est pas associative

```
(1 + (−1)) + 1e-8  =  1e-8
 1 + ((−1) + 1e-8) =  0        car −1 + 1e-8 s'arrondit à −1
```

Le compilateur n'a donc **pas le droit** de réordonner une somme flottante…
sauf avec `/fp:fast`, où il se l'autorise. **Deux jeux d'options, deux
résultats.** D'où `/fp:precise`, imposé dans
[`cmake/TrainingHelpers.cmake`](../../cmake/TrainingHelpers.cmake) — et écrit
explicitement plutôt que laissé au défaut, pour que le choix soit **traçable**.

#### c) L'erreur s'accumule

| N termes de 0,1F | Résultat | Erreur |
|---:|---:|---:|
| 10 | 1,000000119 | 1,2 × 10⁻⁷ |
| 100 | 10,000002 | 2 × 10⁻⁶ |
| 1 000 | 99,999046 | −9,5 × 10⁻⁴ |
| 1 000 (**Kahan**) | 100,000001 | **1,5 × 10⁻⁶** |

La **somme de Kahan** récupère l'erreur d'arrondi à chaque étape : la dérive ne
croît plus avec le nombre de termes.

> ⚠️ Kahan **exige** `/fp:precise`. Avec `/fp:fast`, le compilateur simplifie
> algébriquement `(t − total) − y` en `0` et **annule la compensation**. C'est
> pourquoi l'implémentation utilise `volatile` sur les variables
> intermédiaires.

### 1.2 Comparer des flottants

| Type de tolérance | Convient à | Piège |
|---|---|---|
| **Absolue** — `\|a−b\| ≤ t` | grandeurs à domaine borné (altitude en ft) | absurde si le domaine couvre plusieurs ordres de grandeur |
| **Relative** — `\|a−b\| / max(\|a\|,\|b\|) ≤ t` | grandeurs multi-échelles | indéfinie près de zéro |

Choisir la mauvaise, c'est soit laisser passer une erreur, soit déclarer un
échec sans raison. **La tolérance appartient à l'exigence**, pas au code
(module 04).

### 1.3 Options de compilation à connaître

| Option MSVC | Effet | En avionique |
|---|---|---|
| `/fp:precise` | conforme IEEE-754, pas de réordonnancement | **le défaut à exiger** |
| `/fp:strict` | + respect des modes d'arrondi et des exceptions FP | rare, coûteux |
| `/fp:fast` | réordonnancement et simplifications algébriques autorisés | **proscrit** |
| `/arch:AVX2` | active FMA — change les résultats | à figer dans le SECI |

Sur d'autres cibles, deux pièges classiques :

* le registre **x87 80 bits** : un calcul intermédiaire conservé en registre a
  plus de précision qu'un calcul stocké en mémoire — le résultat dépend alors
  de l'allocation des registres, donc du niveau d'optimisation ;
* l'instruction **FMA** (*fused multiply-add*) : `a*b+c` calculé en une seule
  opération, sans arrondi intermédiaire, donne un résultat **différent** —
  souvent meilleur, mais différent.

---

## 2. Virgule fixe

Format retenu : **Q16.16** sur 32 bits signés.

| Caractéristique | Valeur |
|---|---|
| Partie entière | 16 bits signés → [−32768 ; +32767] |
| Partie fractionnaire | 16 bits → résolution 1/65536 = 1,5 × 10⁻⁵ |
| Représentation de 1,0 | 65536 |

Quatre propriétés :

1. la résolution est **constante** sur tout le domaine ;
2. l'arithmétique est **entière**, donc associative et exacte tant qu'il n'y a
   pas de débordement ;
3. le résultat est **identique sur toute cible** disposant d'entiers 32/64 bits ;
4. l'erreur maximale est **calculable à la main**.

### 2.1 L'exemple qui résume tout

Accumuler 1000 fois 0,1 :

| | Résultat | Comment le prévoir |
|---|---|---|
| Flottant | 99,999046… | analyse numérique de la propagation d'erreur |
| Virgule fixe | 100,006103515625 | **1000 × 6554 / 65536**, calculable de tête |

> **La virgule fixe n'est pas forcément plus précise** — ici, son erreur est
> même plus grande. Ce qu'elle apporte, c'est que le résultat vaut
> **exactement 6 554 000** en représentation interne, identique sur toute
> cible, et vérifiable par une comparaison **d'entiers**.
>
> **C'est la prévisibilité que l'on achète, pas la précision.** Savoir
> formuler cela est ce qui distingue une réponse d'ingénieur d'une réponse
> apprise.

### 2.2 Détails d'implémentation qui comptent

| Choix | Pourquoi |
|---|---|
| Multiplication via `i64` puis décalage de 16 | le produit de deux Q16.16 est un Q32.32 |
| Saturation systématique | un débordement silencieux est pire qu'une valeur bornée |
| `to_int()` tronque vers **−∞** (−1,5 → −2) | comportement du décalage arithmétique, **spécifié** et non subi |
| Arrondi écrit à la main, pas `std::lround` | pas de dépendance au mode d'arrondi courant, WCET trivial |
| NaN → 0, ±∞ → bornes | déterministe, jamais de comportement indéfini |

### 2.3 Quand choisir quoi

| Utiliser… | Quand |
|---|---|
| **Virgule fixe** | domaine borné et connu, égalité exacte utile, cible sans FPU, WCET critique, résultat devant être identique bit à bit |
| **Flottant** | domaine très étendu, calculs trigonométriques ou transcendants, FPU disponible et budget d'erreur analysé |

---

## 3. Ordonnancement à fenêtres fixes (ARINC 653)

### 3.1 Le principe

L'ARINC 653 définit l'interface d'un noyau temps réel partitionné, utilisé par
la quasi-totalité de l'avionique modulaire (**IMA**). Deux garanties :

| Partitionnement | Garantie | Moyen |
|---|---|---|
| **Spatial** | une partition ne peut pas corrompre la mémoire d'une autre | MMU |
| **Temporel** | une partition ne peut pas voler du temps à une autre | fenêtres fixes définies **hors ligne** |

C'est cela, la *freedom from interference* : une fonction **DAL D** ne peut ni
corrompre la mémoire, ni retarder l'exécution d'une fonction **DAL A** qui
partage le même calculateur.

> **Sans partitionnement, tout le calculateur devrait être développé au niveau
> le plus sévère.** C'est l'argument économique qui a fait naître l'IMA.

### 3.2 La trame majeure

```
  0 ────────── 3000 ──── 4500 ──── 6000 ── 7000 ─────────── 10000 µs
  │ CommandesDeVol │Carburant│Affichage│Maint.│     marge     │
  │    DAL A       │  DAL B  │  DAL C  │DAL D │     30 %      │
```

Le plan est une **donnée de configuration**, fixée à la conception et vérifiée
hors ligne. Il n'y a **ni priorités globales, ni préemption entre partitions,
ni file d'attente**.

Ce que le module valide :

* aucune fenêtre vide ;
* aucune fenêtre débordant de la période ;
* **aucun chevauchement** — c'est la propriété qui garantit le
  partitionnement ;
* une **marge** minimale.

> Une trame allouée à 100 % n'a **aucune** marge : le moindre défaut de cache
> ou interruption matérielle provoque un dépassement. Les programmes réels
> exigent typiquement **20 à 30 %** de marge.

### 3.3 Ce qui se passe en cas de dépassement

La partition fautive est **interrompue à la fin de sa fenêtre**. Elle ne vole
pas une microseconde aux suivantes. Le dépassement est enregistré et remonté au
*health monitoring*, qui décide : redémarrage de la partition, passivation,
bascule sur le calculateur redondant.

Le test `Scheduling.overrun_detected_and_localized` le démontre : la
partition Maintenance (DAL D) déborde de 500 µs, elle est identifiée, et les
commandes de vol ne sont pas affectées.

---

## 4. Le WCET

> **WCET** = *Worst-Case Execution Time*. Il faut démontrer que chaque
> partition tient dans sa fenêtre **dans le pire cas** — pas en moyenne.

| Approche | Principe | Limite |
|---|---|---|
| **Mesure** | exécuter et mesurer | on ne mesure que les chemins que l'on a su déclencher ; le pire cas réel peut n'avoir jamais été atteint |
| **Analyse statique** | analyser le binaire + le modèle du processeur (caches, pipeline, prédicteur de branchement) | borne **sûre** mais pessimiste |

Outils du marché : **aiT** (AbsInt), **RapiTime** (Rapita). En pratique, on
fait les deux et on compare : un écart important signale soit un chemin non
testé, soit un modèle de processeur trop pessimiste.

### Ce qui rend le WCET calculable — la synthèse du cours

| Règle | Module |
|---|---|
| Boucles à bornes connues | 08, 15 |
| Pas de récursion | 08 |
| Pas d'allocation dynamique | 08 |
| Pas d'exceptions | 07 |
| Pas d'appel virtuel non résolu | 05, 06 |
| Pas d'attente active non bornée | 15 |
| Graphe d'appel statiquement analysable | 12 |

> Chacune de ces règles a semblé arbitraire quand on l'a rencontrée. Elles
> convergent toutes vers le même objectif : rendre le comportement du logiciel
> **prévisible et démontrable**.

---

## 5. `volatile`

| | `volatile` | `std::atomic` |
|---|---|---|
| S'adresse à | **le matériel** | **les autres fils d'exécution** |
| Garantit | chaque lecture/écriture produit un accès mémoire réel | atomicité, ordre, barrières mémoire |
| Ne garantit pas | atomicité, ordre vis-à-vis des accès non volatiles, exclusion mutuelle | rien sur les registres matériels |

Sans `volatile`, ce code est une boucle infinie après optimisation :

```cpp
while (registre_etat == 0U) { }   // le compilateur lit UNE fois, puis met en cache
```

> La confusion entre `volatile` et `std::atomic` est l'une des erreurs les
> plus répandues du développement embarqué.

**Et surtout** : `wait_for_bit()` a un **budget d'itérations borné**. Une
attente active non bornée laisse le chien de garde redémarrer le calculateur —
événement bien plus grave, en vol, que la panne du périphérique attendu.

Note : sur un calculateur monocœur à fenêtres fixes, le problème de
concurrence se pose peu, puisque les partitions ne s'exécutent jamais
simultanément. **Encore un bénéfice du partitionnement temporel.**

---

## 6. Ce que dit la DO-178C

| Objectif / section | Sujet |
|---|---|
| **A-5.6** | *accuracy and consistency* — couvre l'arithmétique flottante, le débordement et l'usage de la pile |
| **§6.3.4.f** | Les algorithmes sont exacts — inclut l'analyse de précision numérique |
| **A-4.x** | L'architecture est compatible avec les contraintes temporelles |
| **§2.4** | *Freedom from interference* entre composants |
| **CAST-32A** | Processeurs multicœurs — interférences temporelles sur les ressources partagées |
| **A-6.5** | L'exécutable est compatible avec la cible — tests **sur cible réelle** |

---

## 7. Manipulation

```bash
.\build\debug\bin\demo_15-determinisme-temps-reel.exe
```

```bash
.\build\debug\bin\tests_15-determinisme-temps-reel.exe --verbose --req
```

---

## 8. Exercices

**8.1 — Le compteur de temps qui s'arrête**
Écrivez un compteur `f32 t` incrémenté de 0,01 à chaque cycle. Au bout de
combien d'incréments cesse-t-il de progresser ? Vérifiez par le calcul, puis
par l'exécution. Refaites en `f64`, puis en `u32` de millisecondes. Quelle
solution retiendriez-vous, et pourquoi ?

**8.2 — Un filtre en virgule fixe**
Implémentez un passe-bas du premier ordre `y[n] = y[n-1] + a·(x[n] − y[n-1])`
en `Fixed`, avec `a = 0,1`. Comparez sa sortie à la version flottante sur
1000 échantillons. Bornez l'écart **par le calcul** avant de le mesurer.

**8.3 — Dimensionner une trame majeure**
Quatre partitions : commandes de vol (DAL A, 2 ms, 200 Hz), carburant
(DAL B, 3 ms, 50 Hz), affichage (DAL C, 5 ms, 20 Hz), maintenance
(DAL D, 1 ms, 10 Hz). Construisez la trame majeure. Quelle période choisir ?
Quel taux d'occupation ? La marge de 20 % est-elle tenable ?

**8.4 — L'erreur de conception**
Un collègue propose de trier les partitions par priorité et de laisser
l'ordonnanceur préempter. Rédigez une réponse argumentée : qu'est-ce que cela
casse, exactement ? Quel objectif DO-178C n'est plus démontrable ?

**8.5 — `/fp:fast`**
Recompilez le module avec `/fp:fast` (ajoutez-le dans le `CMakeLists.txt` du
module). Quels tests échouent ? Lesquels passent alors qu'ils ne devraient
pas ? Concluez sur la place des options de compilation dans le SECI.

---

## 9. Pour aller plus loin

* David Goldberg, *What Every Computer Scientist Should Know About
  Floating-Point Arithmetic* (1991) — **gratuit, et la référence absolue**.
* ARINC 653 Part 1 — *Required Services*.
* **CAST-32A** — *Multi-core Processors*. Incontournable si vous visez les
  programmes récents.
* Documentation d'aiT (AbsInt) ou de RapiTime : lire un rapport WCET réel vaut
  dix explications.
* Microsoft Learn, *Microsoft Visual C++ Floating-Point Optimization* : ce que
  fait exactement chaque option `/fp:`.

---

⬅️ [14 — Configuration et qualité](../14-configuration-qualite/README.md) |
➡️ [16 — Projet intégré](../16-projet-integre/README.md)
