# Antisèche C# → C++

> Conçue pour un développeur C# expérimenté. Les pièges sont signalés par ⚠️,
> les points où C++ est *différent sans être pire* par ↔️.

---

## 1. Modèle de compilation

| | C# | C++ |
|---|---|---|
| Unité | assembly | **unité de traduction** (un `.cpp`) |
| Le compilateur voit… | tout l'assembly | **un seul fichier à la fois** |
| Métadonnées dans le binaire | oui (réflexion) | **non** |
| Résolution des noms | compilateur | compilateur **puis** éditeur de liens |
| `#include` | n'existe pas (`using` ≠ `#include`) | **copier-coller de texte** |

⚠️ **Les trois erreurs à savoir lire :**

| Erreur | Cause |
|---|---|
| `C2065: identificateur non déclaré` | `#include` manquant — le **compilateur** ne connaît pas le nom |
| `LNK2019: unresolved external symbol` | déclaré mais jamais défini — l'**éditeur de liens** ne trouve rien |
| `LNK2005: symbol already defined` | violation de l'ODR — deux définitions |

**Règle** : `.hpp` = déclarations, `.cpp` = définitions.

---

## 2. Types et valeurs

| Sujet | C# | C++ |
|---|---|---|
| `int` | 32 bits **garantis** | ⚠️ **≥ 16 bits**, dépend de la cible |
| `long` | 64 bits garantis | ⚠️ 32 bits (MSVC) ou 64 (GCC Linux) |
| Largeur explicite | `Int32`, `UInt16` | `<cstdint>` : `int32_t`, `uint16_t` |
| Débordement signé | défini (wrap ou `checked`) | ⚠️ **COMPORTEMENT INDÉFINI** |
| Débordement non signé | défini | défini (modulo 2ᴺ) |
| Variable locale non initialisée | erreur de compilation | ⚠️ **autorisé**, lecture = UB |
| `char` | 16 bits, Unicode | 8 bits, **signe non spécifié** |
| Conversion rétrécissante | cast explicite exigé | ⚠️ souvent **silencieuse** |
| `bool` | ne se convertit pas en int | se convertit implicitement |

⚠️ **Le piège des promotions** :
```cpp
uint8_t a = 200, b = 100;
auto s = a + b;    // s est un `int` valant 300, pas un uint8_t valant 44
```

⚠️ **Signe et comparaison** :
```cpp
int  minus_one = -1;
unsigned int one = 1U;
bool r = (minus_one < one);   // FALSE : -1 devient 4294967295
```

---

## 3. `class` et `struct`

| | C# | C++ |
|---|---|---|
| `class` | type **référence** (tas, GC) | ⚠️ type **valeur** |
| `struct` | type valeur | type valeur — **identique à `class`** sauf visibilité par défaut |
| Passage par défaut | référence d'objet | ⚠️ **copie** |
| Visibilité par défaut | `private` (membres) | `private` (`class`), `public` (`struct`) |

```cpp
void f(Mesure m);          // COPIE l'objet entier
void g(const Mesure& m);   // ce que vous vouliez probablement
```

---

## 4. Mémoire et durée de vie

| Sujet | C# | C++ |
|---|---|---|
| Allocation | `new`, GC derrière | pile, statique, ou `new` (⚠️ **interdit** en avionique) |
| Libération | non déterministe | **déterministe** (RAII) |
| `IDisposable` + `using` | à écrire, oubliable | **destructeur**, impossible à oublier |
| Finaliseur | appelé *un jour*, par le GC | destructeur : **maintenant, ici, sur ce thread** |
| Référence pendante | impossible | ⚠️ **possible et silencieuse** |
| `null` | possible partout | `T*` peut être nul, `T&` **jamais** |

↔️ **RAII, la bonne surprise** :
```cpp
{
    ScopedLock verrou;   // acquis ici
    if (erreur) { return; }
    ...
}                        // libéré ici, sur TOUS les chemins
```
Pas de `using`, pas de `finally`, pas d'oubli possible.

---

## 5. Const et immutabilité

| C# | C++ |
|---|---|
| `readonly` — porte sur le **champ** | `const` — porte sur **ce qui est désigné**, et se propage |
| `const` — constante de compilation | `constexpr` |
| `in` (paramètre) | `const T&` |
| `record`, immutabilité par convention | `const` **imposé par le compilateur** |

⚠️ Quatre formes à lire **de droite à gauche** :
```cpp
uint8_t*             p1;  // pointeur modifiable → octet modifiable
const uint8_t*       p2;  // pointeur modifiable → octet CONSTANT
uint8_t* const       p3;  // pointeur CONSTANT   → octet modifiable
const uint8_t* const p4;  // tout constant
```

↔️ Une méthode `const` est appelable sur une référence constante ; une méthode
non-`const` ne l'est pas. Le compilateur transforme une intention de
conception en **garantie vérifiée**.

---

## 6. Généricité

| | C# | C++ |
|---|---|---|
| Compilation | **une fois** | ⚠️ **une fois par instanciation** |
| Contraintes | `where T : IComparable` | aucune avant C++20 → `static_assert` |
| Vérification | à la **définition** | à l'**instanciation** (messages longs) |
| Paramètre valeur | impossible | ✅ `RingBuffer<T, 16>` |
| Spécialisation | impossible | ✅ totale et partielle |
| Calcul à la compilation | quasi inexistant | ✅ `constexpr`, `if constexpr` |

⚠️ **Conséquence DO-178C** : la couverture structurelle doit être obtenue
**par instanciation** (module 06).

---

## 7. Polymorphisme

| | C# | C++ |
|---|---|---|
| Virtualité | `virtual` explicite | idem, mais ⚠️ oublier `override` ne prévient pas |
| `abstract` | `abstract` | `= 0` (méthode pure) |
| `sealed` | `sealed` | `final` |
| Destruction polymorphe | GC | ⚠️ `virtual ~T()` **obligatoire**, sinon UB |
| Découpage (*slicing*) | impossible | ⚠️ **possible et silencieux** |
| `as` / `is` | idiomatique | `dynamic_cast` — **interdit** en avionique |
| Interface | `interface` | classe abstraite pure |

**Toujours écrire `override`.** C'est votre seul filet.

---

## 8. Gestion d'erreurs

| | C# | C++ avionique |
|---|---|---|
| Signalement | exception | valeur de retour `Result<T>` |
| Propagation | automatique | **explicite**, à chaque niveau |
| Oubli de traitement | remonte au sommet | ⚠️ **compile quand même** → `[[nodiscard]]` |
| Nettoyage | `finally` / `using` | RAII |
| Coût | acceptable | non borné → **interdit** |

```cpp
const Result<f32> r = calculer();
if (r.is_error()) { return Result<f32>::error(r.status()); }
utiliser(r.value());
```

Plus verbeux qu'un `try`/`catch`, mais **tout le flot de contrôle est
visible** — ce que la DO-178C exige.

---

## 9. Conteneurs

| C# | C++ standard | C++ avionique |
|---|---|---|
| `List<T>` | `std::vector<T>` | ⚠️ alloue → `StaticVector<T, N>` |
| `T[]` | `std::array<T, N>` | ✅ `std::array` |
| `Dictionary<K,V>` | `std::unordered_map` | ⚠️ alloue → table statique triée |
| `Span<T>` | `std::span` (C++20) | `avio::Span<T>` |
| `string` | `std::string` | ⚠️ alloue → `char[N]` borné |
| `IEnumerable<T>` | itérateurs | itérateurs (pas de LINQ) |

---

## 10. Syntaxe : correspondances rapides

| C# | C++ |
|---|---|
| `namespace X { }` | `namespace X { }` (pas de `;`) |
| `using System;` | `using namespace std;` ⚠️ à proscrire en portée de fichier |
| `var x = 5;` | `auto x = 5;` |
| `foreach (var x in c)` | `for (const auto& x : c)` |
| `x is Foo f` | pas d'équivalent direct |
| `nameof(x)` | `#x` (macro) |
| `?.` | pas d'équivalent |
| `??` | pas d'équivalent |
| `$"{a} {b}"` | `std::format` (C++20) ou `snprintf` |
| `static readonly` | `static constexpr` ou `inline const` |
| `partial class` | n'existe pas |
| `internal` | `namespace { }` anonyme |
| `out int x` | `int& x` |
| `ref int x` | `int& x` |
| `params` | template variadique |
| `lock` | `std::lock_guard` |
| `async`/`await` | pas d'équivalent (et hors sujet en avionique) |

---

## 11. Ce que vous allez regretter, et ce que vous allez apprécier

**Regretter :**
- pas de garbage collector — la durée de vie est votre problème ;
- pas de réflexion — pas de sérialisation automatique ;
- messages d'erreur de templates ;
- pas de gestionnaire de paquets universel (`NuGet` n'a pas d'équivalent
  unanime) ;
- deux fichiers par composant.

**Apprécier :**
- `const` réellement transitif et vérifié par le compilateur ;
- RAII : la libération déterministe, garantie, non oubliable ;
- `constexpr` : le calcul déplacé à la compilation ;
- les types forts à **coût nul** ;
- savoir exactement ce que fait la machine ;
- des temps de démarrage et une empreinte mémoire sans commune mesure.
