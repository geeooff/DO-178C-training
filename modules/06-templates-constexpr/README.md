# Module 06 — Templates, `constexpr` et polymorphisme statique

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 05

---

## Objectifs pédagogiques

1. Écrire et instancier des templates de fonction et de classe.
2. Contraindre un template avec `static_assert` (l'équivalent C++17 des
   `concepts`).
3. Utiliser `constexpr` et `if constexpr` pour déplacer le calcul vers la
   compilation.
4. Mettre en œuvre le **CRTP** et comparer objectivement statique et dynamique.
5. Expliquer la **couverture par instanciation** exigée par la DO-332.

---

## 1. Le cours

### 1.1 Templates ≠ génériques C#

| | C# | C++ |
|---|---|---|
| Compilation | **une fois** | **une fois par instanciation** |
| Contraintes | `where T : IComparable` — vérifiées à la **définition** | aucune avant C++20 — vérifiées à l'**instanciation** |
| Code généré | partagé pour les types référence | dupliqué pour chaque type |
| Valeurs en paramètre | impossible | **oui** : `RingBuffer<T, 16>` |
| Spécialisation | impossible | oui, partielle et totale |

`RingBuffer<i32, 4>` et `RingBuffer<f32, 8>` sont **deux types distincts**, avec
deux codes machine distincts. C'est la clé de tout ce module.

L'absence de contrainte déclarée explique les messages d'erreur de plusieurs
centaines de lignes : le compilateur ne découvre le problème qu'au moment où il
tente réellement de générer le code. La parade tient en deux lignes :

```cpp
static_assert(N > 0U, "RingBuffer : la capacite doit etre strictement positive");
static_assert(std::is_trivially_copyable_v<T>,
              "RingBuffer : T doit etre trivialement copiable");
```

Le message devient celui que **vous** avez écrit. En C++20, les `concepts`
formalisent cela ; en C++17 (notre cible), `static_assert` fait très bien
l'affaire.

### 1.2 `constexpr` : calculer à la compilation

```cpp
inline constexpr Crc8Table kCrc8Table{};   // 256 octets, calculés par le compilateur
static_assert(kCrc8Table.values[255] == 0xF3U, "CRC-8 : entree 255 incorrecte");
```

Quatre bénéfices, tous directement monnayables en certification :

| Bénéfice | Conséquence DO-178C |
|---|---|
| Aucun code d'initialisation dans le binaire | rien à tracer, rien à tester, rien à couvrir |
| La table est en ROM | impossible à corrompre en RAM ; aide à la ségrégation |
| Erreur de calcul = erreur de **compilation** | vérification la plus précoce et la moins chère |
| Zéro cycle à l'exécution | le WCET n'en souffre pas |

Le contre-exemple à connaître : une table CRC construite au démarrage par une
boucle. Ce code d'initialisation est du code embarqué comme un autre — il faut
le tracer à une exigence, le tester et le couvrir. En `constexpr`, **il n'existe
pas dans le binaire**.

`if constexpr` (C++17) va plus loin : la branche non retenue **n'est pas
compilée** et n'apparaît donc pas dans le code exécutable.

```cpp
if constexpr (std::is_integral_v<T>) { /* accumulation i64 */ }
else                                 { /* accumulation double */ }
```

Conséquence directe : `average<i32,4>` et `average<f32,8>` sont deux codes
différents, à couvrir séparément.

### 1.3 Le CRTP

```cpp
template <typename Derived>
class SensorBase {
    avio::f32 to_engineering(avio::i32 raw) const noexcept {
        return derived().to_engineering_impl(raw);   // résolu à la compilation
    }
    const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }
};

class StaticPressureSensor final : public SensorBase<StaticPressureSensor> { … };
```

La base connaît son dérivé **par son paramètre de template**. Elle peut donc
l'appeler directement, sans vtable.

| | Dynamique (module 05) | Statique (CRTP) |
|---|---|---|
| Taille de l'objet | + 1 pointeur (vptr) | **0 octet** |
| Coût d'un appel | indirection mémoire | inlinable |
| WCET | le pire des dérivés | exact, par instanciation |
| Types connus | à l'exécution | à la compilation |
| Conteneur hétérogène | possible | **impossible** |
| Objectifs DO-332 | OO.6.7 (cohérence locale de type) | couverture par instanciation |

Vérifiez-le : `sizeof(StaticPressureSensor) == 1`, contre
`sizeof(mod05::PressureSensor) == 8`.

> **Point de conception à savoir défendre en entretien** : si l'ensemble des
> types est connu à la compilation — ce qui est le cas de la quasi-totalité des
> systèmes embarqués certifiés — le CRTP donne la même factorisation de code
> pour un coût nul et une vérification plus simple. Le dynamique se justifie
> surtout aux frontières matérielles.

### 1.4 Les quatre pièges

1. **Gonflement du code** (*code bloat*). Dix instanciations d'un conteneur de
   2 ko, c'est 20 ko de Flash. Sur une cible qui en a 512 ko, cela se planifie.
2. **Messages d'erreur illisibles.** Parade : `static_assert` en tête de
   template.
3. **Couverture par instanciation** — voir §2.
4. **Temps de compilation.** Anecdotique ici, majeur sur un projet réel.

---

## 2. Ce que dit la DO-332 : la couverture par instanciation

> **Vulnérabilité n° 2 de la DO-332 : le polymorphisme paramétrique.**

Le raisonnement, en trois étapes :

1. La couverture structurelle (objectifs **A-7.5 à A-7.7**) porte sur le **code
   exécutable objet**, pas sur le code source.
2. Un template produit **une copie du code par instanciation**.
3. Donc chaque instanciation embarquée doit atteindre l'objectif de couverture
   applicable au niveau DAL.

Avoir couvert `RingBuffer<i32, 4>` ne dit **rien** de `RingBuffer<f32, 8>` : le
code généré diffère (comparaisons flottantes, tailles, déroulage de boucle).

Deux conséquences pratiques, appliquées dans ce module :

* **Limiter volontairement le nombre d'instanciations embarquées** et les
  **lister** dans le document de conception ;
* rendre cette liste explicite dans le code, via des **instanciations
  explicites** :

```cpp
template class RingBuffer<avio::i32, 4U>;
template class RingBuffer<avio::f32, 8U>;
template class RingBuffer<avio::u8, 16U>;
```

Ces trois lignes, en bas de `compile_time.cpp`, sont un **artefact de
certification** : elles centralisent en un point revisable la liste de ce qui
part réellement dans le binaire.

| Objectif | Application |
|---|---|
| **A-7.5/6/7** — couverture statement / decision / MC/DC | par instanciation |
| **OO.6.7** (DO-332) | s'applique aussi au CRTP : chaque dérivé doit respecter le contrat de la base |
| **A-4.8** — architecture vérifiable | la liste des instanciations est une donnée d'architecture |
| **A-5.6** — accuracy and consistency | `static_assert` sur les contraintes et sur les valeurs de table |

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ |
|---|---|---|
| Généricité | `List<T>`, un seul code | template, un code par instanciation |
| Contrainte | `where T : …` | `static_assert` (C++17), `concept` (C++20) |
| Paramètre valeur | impossible | `template <usize N>` |
| Calcul à la compilation | quasi inexistant | `constexpr`, `if constexpr`, `consteval` |
| Spécialisation | impossible | totale et partielle |
| Message d'erreur | clair | à apprivoiser |

---

## 4. Manipulation

```bash
.\build\debug\bin\demo_06-templates-constexpr.exe
```

```bash
.\build\debug\bin\tests_06-templates-constexpr.exe --verbose --req
```

Expérience à faire : ajoutez `template class RingBuffer<avio::i16, 32U>;` dans
`compile_time.cpp` et observez la taille du binaire avant/après. C'est le
gonflement du code, rendu tangible.

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M06-001 | `RingBuffer<T,N>` mémorise au plus N éléments, écrase le plus ancien au-delà et compte les écrasements. | `RingBuffer_i32_4.complete_life_cycle` |
| LLR-M06-002 | `pop()` et `peek()` renvoient faux sans modifier la sortie sur un tampon vide. | `RingBuffer_i32_4.robustness_empty_buffer` |
| LLR-M06-003 | `clear()` remet taille et compteur d'écrasements à zéro. | `RingBuffer_i32_4.reset_to_zero` |
| LLR-M06-004 | L'instanciation `RingBuffer<f32,8>` respecte le même contrat. | `RingBuffer_f32_8.float_instantiation` |
| LLR-M06-005 | L'instanciation `RingBuffer<u8,16>` respecte le même contrat. | `RingBuffer_u8_16.byte_instantiation` |
| LLR-M06-006 | L'occupation mémoire d'une instanciation est au moins celle de son stockage. | `RingBuffer.memory_size_per_instantiation` |
| LLR-M06-010..012 | `average()` calcule la moyenne entière (troncature) ou flottante selon `T`, et renvoie `T{}` sur un tampon vide. | `Average.*` |
| LLR-M06-020..023 | La table CRC-8 est calculée à la compilation ; `crc8()` produit 0xF4 sur le vecteur « 123456789 », détecte toute altération, et renvoie la valeur initiale sur un tampon vide. | `Constexpr.*` |
| LLR-M06-024 | `ipow`, `popcount` et `even_parity` sont utilisables à la compilation comme à l'exécution. | `Constexpr.functions_usable_at_both_times` |
| LLR-M06-025 | La table de linéarisation couvre −60,0 à +80,0 °C en 16 points strictement croissants. | `Constexpr.linearization_table` |
| LLR-M06-030 | Les capteurs CRTP produisent les mêmes valeurs que leurs équivalents virtuels du module 05. | `CRTP.behavior_identical_to_dynamic` |
| LLR-M06-031 | Un capteur CRTP sans donnée membre occupe 1 octet (aucun pointeur de vtable). | `CRTP.no_memory_cost` |
| LLR-M06-032 | `is_in_range()` et `to_engineering_clamped()` sont fournis par la base à tous les dérivés. | `CRTP.factored_common_behavior` |
| LLR-M06-033..034 | `read_average()` moyenne les valeurs écrêtées ; elle renvoie `value_min()` pour un pointeur nul ou un compte nul. | `CRTP.*` |

---

## 6. Exercices

**6.1 — `StaticVector<T, N>`**
Écrivez un tableau à taille variable mais **capacité fixe** : `push_back`,
`pop_back`, `size`, `capacity`, `at` (avec vérification de bornes),
`operator[]` (sans). Contraintes par `static_assert`. Tests pour **au moins
deux instanciations**, et listez-les en instanciation explicite. (Ce type est
repris et approfondi au module 08.)

**6.2 — Casser une contrainte**
Essayez `RingBuffer<std::string, 4>` puis `RingBuffer<i32, 0>`. Notez le
message d'erreur obtenu, comparez-le à celui que vous auriez sans les
`static_assert` (commentez-les temporairement). Combien de lignes d'écart ?

**6.3 — Table CRC-16**
Sur le modèle de `Crc8Table`, écrivez une table CRC-16/CCITT (polynôme 0x1021)
`constexpr`, avec `static_assert` sur au moins trois valeurs de référence.
Vecteur de test : CRC-16/CCITT-FALSE de « 123456789 » vaut `0x29B1`.

**6.4 — Un troisième capteur CRTP**
Ajoutez `StaticAngleOfAttackSensor` (0..4095 → −20,0..+45,0 degrés). Combien de
lignes avez-vous écrites ? Combien de nouvelles instanciations de
`read_average` cela crée-t-il ? Que faut-il ajouter au document de conception ?

**6.5 — Analyse de conception**
Un collègue propose de remplacer toutes les hiérarchies virtuelles du projet
par du CRTP. Rédigez une réponse argumentée d'une demi-page : dans quels cas
est-ce un gain net, dans quels cas est-ce impossible, et quel impact sur les
objectifs DO-332 ?

---

## 7. Pour aller plus loin

* **DO-332**, section sur le *parametric polymorphism* et la couverture par
  instanciation.
* AUTOSAR C++14, règles A14-x (templates).
* Jason Turner, *C++ Weekly* — épisodes sur `constexpr` et le CRTP.
* `std::span`, `std::array`, `std::optional` : de bons exemples de templates de
  la bibliothèque standard, dont on peut lire l'implémentation.

---

⬅️ [05 — Polymorphisme et DO-332](../05-polymorphisme-do332/README.md) |
➡️ [07 — Gestion d'erreurs sans exceptions](../07-erreurs-sans-exceptions/README.md)
