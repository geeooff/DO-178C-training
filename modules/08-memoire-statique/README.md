# Module 08 — Mémoire statique, réserve de blocs et analyse de pile

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 07

---

## Objectifs pédagogiques

1. Expliquer **techniquement** pourquoi `new`/`malloc` sont interdits après
   l'initialisation.
2. Concevoir un conteneur à **capacité fixe** (`StaticVector`).
3. Implémenter une **réserve de blocs** déterministe, avec détection de double
   libération.
4. Comprendre l'**analyse de pile** et ce qui la rend possible.
5. Établir un **budget mémoire** vérifiable.

---

## 1. Le cours

### 1.1 Quatre raisons d'interdire l'allocation dynamique

| # | Raison | Détail |
|---|---|---|
| 1 | **Non-déterminisme temporel** | `new` et `malloc` parcourent des listes de blocs libres ; leur durée dépend de l'historique **complet** des allocations. Non bornable utilement, alors que le WCET doit être démontré. |
| 2 | **Fragmentation** | Après des heures de vol, la mémoire libre peut être suffisante *en total* mais morcelée : l'allocation échoue « alors qu'il reste de la place ». Ce défaut apparaît après des dizaines d'heures — **jamais pendant les tests**. |
| 3 | **Échec ingérable** | Que faire si `new` échoue à 10 000 m ? Il n'existe pas de bonne réponse. On rend donc l'échec **impossible**. |
| 4 | **DO-332 OO.6.8.2** | Le supplément OO impose de démontrer l'absence de fuite, de fragmentation, d'épuisement et de référence pendante. En allocation statique, ces objectifs sont satisfaits **par construction**. |

> **La règle du domaine** : toute la mémoire est réservée **à la compilation**.
> L'occupation RAM devient une constante, calculable par `sizeof`, inscrite au
> budget mémoire du calculateur.

Nuance importante : l'allocation dynamique **pendant la phase d'initialisation**
est parfois acceptée, à condition de démontrer qu'elle est bornée, qu'elle
réussit toujours, et qu'aucune libération n'a lieu ensuite. C'est le modèle
« allouer une fois, ne jamais libérer ».

### 1.2 `StaticVector<T, N>`

La **taille** varie à l'exécution, la **capacité** est fixée à la compilation.

```cpp
mod08::StaticVector<avio::i32, 4U> measurements;
measurements.push_back(10);   // true
…
measurements.push_back(50);   // FALSE : plein, l'élément n'est pas ajouté
```

La différence avec `std::vector` est décisive : là où `std::vector` réallouerait
**silencieusement**, ici le dépassement est un **événement observable et
compté** (`rejected_count()`). C'est le signe que le dimensionnement mérite
d'être réexaminé — une information qui remonte jusqu'à la maintenance.

### 1.3 `MemoryPool<BlockSize, BlockCount>`

Certains systèmes ont *besoin* d'allouer et de libérer à l'exécution : files de
messages, contextes de communication, tampons de réception. La réponse
avionique n'est pas `new`, c'est une **réserve de blocs de taille fixe**,
réservée statiquement.

| Propriété | Comment elle est obtenue |
|---|---|
| Allocation/libération en **O(1)** | liste chaînée de blocs libres |
| **Aucune fragmentation** possible | tous les blocs ont la même taille |
| Épuisement **détectable et borné** | capacité connue à la compilation, `allocate()` renvoie `nullptr` |
| **Double libération** détectée | bitmap d'occupation |
| Pointeur **étranger** refusé | vérification d'appartenance + alignement |
| Dimensionnement **validé** | `high_water_mark()` relevé en essais |

Détail élégant : la liste des blocs libres est stockée **dans les blocs
eux-mêmes** (un bloc libre contient l'adresse du suivant). Coût mémoire
supplémentaire : zéro. C'est pourquoi `BlockSize ≥ sizeof(void*)`.

Le pointeur est écrit via `std::memcpy` plutôt que par un `reinterpret_cast` :
aucune violation des règles d'aliasing, aucun comportement indéfini.

C'est exactement le fonctionnement des *buffer pools* d'un noyau ARINC 653.

> **`high_water_mark()` est la métrique qui compte.** Une réserve dont le pic
> atteint 100 % de la capacité en essais est sous-dimensionnée : rien ne dit
> qu'un cas non testé n'en demandera pas un de plus.

### 1.4 Analyse de pile

La pile est une ressource **finie**, dimensionnée à la conception (typiquement
4 à 64 ko par tâche dans un noyau ARINC 653). Son débordement ne produit pas
une exception propre : il écrase la mémoire voisine, souvent celle d'une autre
tâche. C'est la perte de la *freedom from interference*.

La DO-178C ne cite pas la pile explicitement, mais l'objectif **A-5.6**
(*accuracy and consistency*) couvre le *stack usage*. En pratique, tout dossier
de certification contient une **analyse de pile** démontrant que l'usage maximal
reste sous le budget alloué, avec une marge (souvent 30 %).

**Ce qui rend l'analyse possible :**

| Condition | Interdit correspondant |
|---|---|
| Profondeur d'appel bornée et connue | pas de **récursion** |
| Taille de trame connue | pas de **VLA**, pas d'`alloca` |
| Graphe d'appel statiquement déterminable | pas de pointeur de fonction non résolu, pas de `dynamic_cast` |
| Pas de trames imprévues | pas d'**exceptions** |

> On voit ici que la plupart des interdits de ce cours convergent vers **un
> seul** objectif : rendre le graphe d'appel **statiquement analysable**.

Le module mesure la différence : `factorial(20)` en version itérative atteint
une profondeur de **1**, la version récursive une profondeur de **20**. Même
résultat, vingt fois plus de pile. Sur une cible où chaque trame fait 48 octets :
960 octets contre 48.

La récursion n'est pas *interdite par principe* : elle l'est parce qu'elle
oblige à **démontrer** une borne, à mesurer la taille de trame, et à inscrire
le produit au budget. Autant écrire la version itérative.

### 1.5 Le budget mémoire

```
StaticVector<i32,100> (journal mesures)     416 octets
MemoryPool<32,8> (tampons messages)         288 octets
TOTAL alloué statiquement                   704 octets
```

Chaque octet de RAM est connu **avant** l'exécution. On peut donc écrire dans
le dossier de certification : *« occupation RAM du composant : 704 octets,
budget alloué : 2 ko, marge : 65 % »* — et le **démontrer** par un simple
`sizeof`.

### 1.6 Quand le compilateur ne peut pas vous croire

Voici un cas rencontré **dans ce dépôt**, pas un exemple inventé. Le code de
`StaticVector::erase()` était celui-ci :

```cpp
bool erase(avio::usize index) noexcept {
    if (index >= size_) {
        return false;
    }
    for (avio::usize k = index; (k + 1U) < size_; ++k) {
        storage_[k] = storage_[k + 1U];
    }
    size_ -= 1U;
    return true;
}
```

Ce code est **correct**. Aucune méthode de la classe ne permet à `size_` de
dépasser `Capacity`, donc `k + 1 < size_ <= Capacity` : l'écriture est toujours
dans les bornes. Il passe sous MSVC, sous Clang, et sous GCC en `-O0`.

Compilé par **GCC 15 en `-O2`**, il produit ceci :

```
static_vector.hpp:104:35: warning: array subscript 100 is above array bounds
                                   of 'int [5]' [-Warray-bounds=]
   inlined from 'mt_body_StaticVector_removal_by_index()'
        at tests/test_memory.cpp:86
```

La ligne 86 du test est `CHECK_FALSE(vector.erase(100U));` — un test de
robustesse tout à fait légitime, sur un vecteur de capacité 5.

**Le compilateur n'a pas tort.** Il inline l'appel avec `index = 100`, entre
dans la fonction, et cherche à savoir si la boucle peut s'exécuter. Pour cela
il lui faudrait savoir que `size_ <= 5`. Or cette information n'existe **nulle
part dans le corps de la fonction** : elle est répartie dans `push_back()`,
dans le constructeur, dans l'ensemble de la classe. Le compilateur ne raisonne
que sur ce qu'il voit. Il doit donc envisager un `size_` valant 200, cas où
`storage_[100]` sortirait effectivement des bornes.

Retenez la formulation : **un invariant qui n'est vrai que « globalement »
n'est pas exploitable localement**, ni par le compilateur, ni par un analyseur
statique, ni par un relecteur qui n'a pas la classe entière en tête.

#### Les trois façons de le traiter, et leur prix

| Option | Effet | Prix |
|---|---|---|
| Ne rien faire | L'avertissement reste | Un avertissement toléré aujourd'hui en masque un vrai demain |
| `-Wno-array-bounds` | Silence global | On éteint le détecteur, pas le problème. Inacceptable |
| **Borner explicitement** | L'écriture devient *prouvable* | Une branche inatteignable, donc non couvrable |

C'est la troisième qui est retenue ici :

```cpp
const avio::usize last = (size_ < Capacity) ? size_ : Capacity;
for (avio::usize k = index; (k + 1U) < last; ++k) {
```

#### Le prix à payer, et pourquoi on l'assume

`size_` étant toujours `<= Capacity`, la branche `: Capacity` **ne s'exécute
jamais**. C'est du code défensif : une protection contre une situation que la
conception rend impossible.

Et vous venez de créer, volontairement, un problème de **couverture
structurelle** (module 11) : une décision dont un seul résultat est atteignable
ne sera jamais couverte à 100 %, quel que soit le jeu de tests.

Ce conflit est explicite dans la norme (§6.4.4.3) et fait l'objet d'un position
paper dédié, **CAST-17**. La réponse admise n'est pas de supprimer la
protection, mais de la **justifier par analyse** : on documente que le code est
inatteignable, on explique pourquoi il est là, et on démontre qu'il ne peut pas
nuire. C'est une justification écrite, pas une case à cocher.

> **Ce qu'un entretien peut en tirer.** « Vous avez du code défensif non
> couvrable. Que faites-vous ? » La mauvaise réponse est « je le supprime pour
> avoir 100 % ». La bonne : « je le justifie par analyse au titre de CAST-17,
> ou je démontre qu'il est réellement atteignable. Supprimer une protection
> pour améliorer une métrique, c'est optimiser la métrique contre le produit. »

---

## 2. Ce que dit la DO-178C

| Objectif / doc | Application |
|---|---|
| **DO-332 OO.6.8.2** — Dynamic Memory Management | Absence de fuite, fragmentation, épuisement, référence pendante : acquis par construction. |
| **A-5.6** — accuracy and consistency | Couvre explicitement le *stack usage* et l'usage mémoire. |
| **A-6.3** — robustesse | Épuisement de la réserve, double libération, pointeur étranger : tous testés. |
| **A-4.11** — architecture vérifiable | Le budget mémoire est une donnée d'architecture, à figer dans le SDD. |
| **§2.4 / CAST-32A** — freedom from interference | Un débordement de pile ou de tampon casse la ségrégation entre partitions. |

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ avionique |
|---|---|---|
| Allocation | `new` partout, GC derrière | **aucune** après initialisation |
| Conteneur qui grossit | `List<T>` réalloue | `StaticVector` **refuse** |
| Dépassement de capacité | invisible | événement compté |
| Occupation mémoire | estimée, variable | **constante**, calculée par `sizeof` |
| Libération | non déterministe (GC) | déterministe (RAII, module 03) |
| Double libération | impossible | **détectée** explicitement |
| Récursion | courante | bannie (analyse de pile) |

---

## 4. Manipulation

```bash
.\build\debug\bin\demo_08-memoire-statique.exe
```

```bash
.\build\debug\bin\tests_08-memoire-statique.exe --verbose --req
```

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M08-001 | Un `StaticVector` neuf est vide, de capacité `N`, sans rejet. | `StaticVector.initial_state` |
| LLR-M08-002 | `push_back()` / `pop_back()` respectent l'ordre LIFO. | `StaticVector.add_and_remove` |
| LLR-M08-003 | À capacité atteinte, `push_back()` renvoie faux, n'altère pas le contenu et incrémente le compteur de rejets. | `StaticVector.saturated_capacity_without_realloc` |
| LLR-M08-004 | `pop_back()` sur un conteneur vide renvoie faux sans modifier la sortie. | `StaticVector.robustness_remove_on_empty` |
| LLR-M08-005 | `erase(i)` décale les éléments suivants ; tout indice ≥ taille est refusé. | `StaticVector.removal_by_index` |
| LLR-M08-006 | `at()` est vérifié ; `operator[]` hors domaine notifie le gestionnaire d'anomalie et renvoie l'élément 0. | `StaticVector.checked_and_unchecked_access` |
| LLR-M08-007 | `view()` expose exactement `size()` éléments, pas la capacité. | `StaticVector.view_limited_to_size` |
| LLR-M08-008 | L'occupation mémoire est bornée par `N × sizeof(T) + 64` octets. | `StaticVector.known_memory_footprint` |
| LLR-M08-010..011 | La réserve part avec tous les blocs libres ; allocation puis libération restituent l'état. | `MemoryPool.*` |
| LLR-M08-012 | À épuisement, `allocate()` renvoie `nullptr` ; `high_water_mark()` mémorise le pic. | `MemoryPool.bounded_and_detectable_exhaustion` |
| LLR-M08-013 | Deux blocs alloués sont distincts et espacés de `BlockSize`. | `MemoryPool.distinct_and_aligned_blocks` |
| LLR-M08-014 | Une seconde libération du même bloc est refusée. | `MemoryPool.double_release_detected` |
| LLR-M08-015 | Un pointeur n'appartenant pas à la réserve, ou nul, est refusé. | `MemoryPool.foreign_pointer_rejected` |
| LLR-M08-016 | Un pointeur non aligné sur un début de bloc est refusé. | `MemoryPool.misaligned_pointer_rejected` |
| LLR-M08-017 | Après 400 allocations/libérations entrelacées, la réserve retrouve son état initial. | `MemoryPool.reuse_without_fragmentation` |
| LLR-M08-020 | `factorial()` a une profondeur d'appel de 1 quelle que soit l'entrée. | `Stack.iterative_factorial_constant_depth` |
| LLR-M08-021 | `factorial(n)` renvoie faux et 0 pour tout `n > 20`. | `Stack.factorial_robustness_out_of_domain` |
| LLR-M08-022..023 | La version récursive atteint une profondeur `n` et produit le même résultat. | `Stack.*` |
| LLR-M08-024 | La profondeur revient à 0 sur tous les chemins, y compris les sorties anticipées. | `Stack.depth_returns_to_zero` |
| LLR-M08-025..026 | `sum_iterative()` et `binary_search()` sont itératives et robustes au pointeur nul et au tableau vide. | `Stack.*` |

---

## 6. Exercices

**6.1 — `StaticQueue<T, N>`**
File FIFO à capacité fixe : `enqueue`, `dequeue`, `size`, `full`. Réutilisez
l'idée du tampon circulaire (module 06). Exigences, tests (dont saturation et
file vide), puis code. Comptez le nombre de branches à couvrir.

**6.2 — Détecter une fuite**
Écrivez un test qui alloue 5 blocs, n'en libère que 3, et vérifie que
`in_use()` vaut 2. Puis proposez un mécanisme de surveillance qui signalerait
cette fuite en vol (indice : comparer `in_use()` à un seuil attendu en fin de
cycle).

**6.3 — Dimensionner une réserve**
Un système reçoit des messages à 50 Hz. Chaque message occupe un bloc pendant
au plus 3 cycles. Combien de blocs faut-il au minimum ? Quelle marge
prendriez-vous, et comment la justifieriez-vous dans le dossier ? Écrivez un
test qui simule 1000 cycles et relève le `high_water_mark()`.

**6.4 — Analyse de pile manuelle**
Compilez `stack_analysis.cpp` avec `/FAsc` (MSVC produit l'assembleur annoté
dans `build/…/*.asm`). Repérez l'instruction `sub rsp, N` en tête de
`factorial_recursive` : c'est la taille de trame. Calculez la pile consommée
pour `n = 20`. Comparez avec la version itérative.

**6.5 — Le tri qui alloue**
`std::sort` alloue-t-il ? Et `std::stable_sort` ? Cherchez la réponse dans la
documentation, puis proposez une alternative acceptable en avionique pour trier
un `StaticVector<i32, 64>` (indice : tri par insertion, WCET en O(n²) mais
borné et sans allocation).

---

## 7. Pour aller plus loin

* **DO-332**, section OO.6.8.2 — *Dynamic Memory Management*, et son annexe
  sur les techniques acceptables.
* ARINC 653 Part 1 — gestion des partitions et des ressources mémoire.
* AUTOSAR C++14, règles A18-5-x (allocation dynamique).
* Documentation de votre outil d'analyse de pile (StackAnalyzer d'AbsInt,
  `-fstack-usage` de GCC) : la lecture d'un rapport réel vaut dix explications.

---

⬅️ [07 — Erreurs sans exceptions](../07-erreurs-sans-exceptions/README.md) |
➡️ [09 — Exigences et traçabilité](../09-exigences-tracabilite/README.md)
