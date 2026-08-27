# Module 01 — Types, valeurs et mémoire

> **Durée estimée** : 1 journée
> **Prérequis** : module 00

---

## Objectifs pédagogiques

1. Maîtriser les types entiers de largeur fixe et les raisons de ne jamais
   utiliser `int` en embarqué.
2. Comprendre les **promotions** et **conversions arithmétiques usuelles**,
   sources n°1 de bugs subtils en C++.
3. Distinguer débordement **défini** (non signé) et **comportement indéfini**
   (signé) — et savoir pourquoi l'UB est plus dangereux qu'un résultat faux.
4. Écrire de l'arithmétique **saturante** et des conversions **vérifiées**.
5. Savoir lire une disposition mémoire (taille, alignement, bourrage).
6. Rattacher tout cela aux objectifs **A-5.6** (« accuracy and consistency »)
   de la DO-178C.

---

## 1. Le cours

### 1.1 `int` n'existe pas (en embarqué)

En C#, `int` fait 32 bits. Toujours. Partout. C'est dans la spécification CLI.

En C++, la norme garantit seulement des **tailles minimales** :

| Type | Garanti par la norme | MSVC x64 | GCC Linux x64 | Cible embarquée 32 bits typique |
|------|----------------------|----------|---------------|-------------------------------|
| `char` | ≥ 8 bits | 8 | 8 | 8 |
| `short` | ≥ 16 bits | 16 | 16 | 16 |
| `int` | ≥ 16 bits | 32 | 32 | 32 |
| `long` | ≥ 32 bits | **32** | **64** | 32 |
| `long long` | ≥ 64 bits | 64 | 64 | 64 |

Le piège classique : `long` fait 32 bits sur Windows et 64 bits sur Linux.
Un code qui suppose l'un des deux casse au portage.

**Règle du projet** (et de MISRA C++) : utilisez `avio::u8/u16/u32/i8/i16/i32`,
définis dans [`common/include/avio/types.hpp`](../../common/include/avio/types.hpp)
à partir de `<cstdint>`. La largeur est alors dans le nom.

Détail perturbant : `char`, `signed char` et `unsigned char` sont **trois types
distincts**, et le signe de `char` dépend du compilateur. Pour manipuler des
octets, utilisez `avio::u8`.

### 1.2 Promotions et conversions arithmétiques usuelles

Avant toute opération arithmétique, C++ applique deux étapes :

1. **Promotion entière** : tout type plus petit qu'`int` (donc `u8`, `i8`,
   `u16`, `i16`, `bool`, `char`) est promu en `int`.
2. **Conversions arithmétiques usuelles** : les deux opérandes sont ramenés à
   un type commun. En cas de mélange, **le non signé l'emporte** à rang égal.

Conséquences à connaître par cœur :

```cpp
avio::u8 a = 200U, b = 100U;
auto s = a + b;              // s est un `int` valant 300, pas un u8 valant 44

int   moins_un = -1;
unsigned int un = 1U;
bool  r = (moins_un < un);   // r vaut FALSE : -1 devient 4294967295
```

Et le classique qui boucle à l'infini :

```cpp
for (avio::u32 i = n - 1U; i >= 0U; --i) { ... }   // i >= 0 est TOUJOURS vrai
```

MISRA C++ interdit les conversions implicites qui changent la signature ou
perdent de l'information ; `/W4` et clang-tidy
(`cppcoreguidelines-narrowing-conversions`, activé dans ce dépôt) les signalent.

### 1.3 Débordement : deux mondes très différents

| | Débordement non signé | Débordement signé |
|---|---|---|
| Comportement | **défini** : arithmétique modulo 2ᴺ | **INDÉFINI** (UB) |
| Reproductible | oui | non |
| Détectable a posteriori | oui | non |

L'UB n'est pas « une valeur bizarre ». Le compilateur a le droit de **supposer
que cela n'arrive jamais** et d'optimiser en conséquence. Le test de garde
`if (x + 1 < x)` peut être purement et simplement supprimé par l'optimiseur,
parce que la seule façon qu'il soit vrai est un UB — donc, du point de vue du
compilateur, il est faux.

Comparaison avec C# : `int` déborde silencieusement (`unchecked`, le défaut) ou
lève `OverflowException` (`checked`). Dans les deux cas c'est **défini**.
En C++, il n'y a pas de filet.

**Les trois stratégies possibles**, à choisir *par exigence*, pas par habitude :

| Stratégie | Quand | Implémentation ici |
|---|---|---|
| **Saturer** | grandeur physique bornée (angle de gouverne, consigne) | `saturating_add/sub/mul` |
| **Signaler** | le débordement traduit une anomalie à remonter | `checked_add`, `checked_div` |
| **Prévenir par preuve** | on démontre que les entrées rendent le débordement impossible | analyse statique / preuve formelle (DO-333) |

### 1.4 Le cas d'école : Ariane 5, vol 501

4 juin 1996. 37 secondes après le décollage, le lanceur s'autodétruit.
Cause racine :

* du code du **système de référence inertielle d'Ariane 4** est réutilisé tel
  quel ;
* une vitesse horizontale, flottant 64 bits, est convertie en **entier signé
  16 bits** ;
* la trajectoire d'Ariane 5 produit des valeurs bien plus grandes que celles
  d'Ariane 4 ;
* la conversion déborde, l'exception matérielle n'est pas rattrapée, le
  calculateur passe en panne… ainsi que son redondant, qui exécute le même code.

Trois leçons, toutes encore d'actualité :

1. **Une conversion rétrécissante non vérifiée est une bombe à retardement.**
   D'où `checked_cast` dans ce module.
2. **Le domaine de validité fait partie de l'exigence.** Réutiliser du code
   hors de son domaine, c'est du code non vérifié.
3. **La redondance identique ne protège pas d'une erreur de conception.**
   Elle protège des pannes matérielles aléatoires, pas des fautes systématiques
   (d'où la notion de *dissimilarité* en DAL A).

### 1.5 Initialisation : le défaut n'existe pas

```cpp
void f() {
    int x;          // VALEUR INDÉTERMINÉE. Lire x = comportement indéfini.
    int y{};        // 0
    int z = 42;     // 42
    avio::u32 t[4] = {};  // les 4 éléments valent 0
}
```

En C#, le compilateur **refuse** de compiler la lecture d'une variable locale
non assignée, et tous les champs sont zéro-initialisés. En C++, `int x;` local
est de la mémoire brute prise sur la pile — elle contient ce qu'y a laissé
l'appel précédent.

C'est pernicieux : en Debug, MSVC remplit souvent la pile de `0xCC`, ce qui rend
le bug reproductible. En Release, la valeur est aléatoire. Un bug qui
n'apparaît qu'en Release est presque toujours de cette famille.

**Règle du projet** : toujours initialiser à la déclaration. Vérifiée par
`cppcoreguidelines-init-variables` dans notre configuration clang-tidy.

### 1.6 `enum class`

```cpp
enum class SensorId : avio::u8 { LeftPitot = 0U, /* ... */ };
```

* portée propre (`SensorId::LeftPitot`), comme un `enum` C# ;
* **pas** de conversion implicite vers un entier (contrairement à l'`enum` C
  hérité, que MISRA proscrit) ;
* type sous-jacent explicite → **taille garantie**, indispensable dès qu'une
  valeur traverse un bus ou une frontière mémoire.

⚠️ **Une `enum class` ne garantit pas la validité de la valeur.**
`static_cast<SensorId>(200)` compile parfaitement. Toute valeur venant de
l'extérieur (bus ARINC, mémoire partagée, EEPROM) doit être **validée** :
c'est la fonction `is_valid()`.

### 1.7 Disposition mémoire

```cpp
struct TrameNaive     { u8 header; u32 payload; u8 checksum; };  // 12 octets
struct TrameCompacte  { u32 payload; u8 header; u8 checksum; };  //  8 octets
```

Le compilateur insère du **bourrage** (*padding*) pour respecter l'alignement
de chaque champ. Ranger les champs du plus large au plus étroit économise ici
33 % de RAM — ce qui compte sur un calculateur qui en a 2 Mo.

Deux pièges à retenir :
* le bourrage **n'est pas initialisé** → comparer deux structures avec
  `memcmp` compare aussi les octets de bourrage : résultat non fiable ;
* transmettre une `struct` telle quelle sur un bus suppose que l'émetteur et le
  récepteur ont le **même** alignement, le même boutisme et le même
  compilateur. En pratique, on sérialise champ par champ.

---

## 2. Ce que dit la DO-178C

| Objectif | Intitulé | Lien avec ce module |
|---|---|---|
| **A-5.6** | Source Code is accurate and consistent | Couvre explicitement : débordement, résolution, usage des types, initialisation, arithmétique de pointeurs. C'est **l'objectif** de ce module. |
| **A-5.1** | Source Code complies with low-level requirements | Une exigence « la consigne est bornée à ±30° » doit se voir dans le code (saturation), pas seulement dans la doc. |
| **A-5.4** | Source Code conforms to standards | Le standard de codage interdit les conversions implicites : `checked_cast` est le moyen de s'y conformer. |
| **A-4.x** | Robustesse de l'architecture | Le comportement aux bornes est une décision de conception, à écrire dans le SDD. |

**Formulation typique d'une exigence de bas niveau bien écrite :**

> *LLR-CTRL-021 — La fonction `compute_command()` doit borner la commande
> calculée à l'intervalle [−30,0 ; +30,0] degrés. Toute valeur calculée hors de
> cet intervalle est ramenée à la borne la plus proche et positionne
> l'indicateur `saturated` à vrai.*

Remarquez : elle est **vérifiable**, **non ambiguë**, et elle précise le
comportement **aux bornes** et **hors bornes**. C'est ce niveau de précision qui
rend le test possible.

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ |
|---|---|---|
| Taille de `int` | 32 bits garantis | ≥ 16 bits, dépend de la cible |
| Débordement signé | défini (wrap ou exception) | **comportement indéfini** |
| Variable locale non initialisée | erreur de compilation | autorisé, lecture = UB |
| `enum` | pas de conversion implicite | `enum class` : idem ; `enum` C : conversion implicite |
| Validité d'une valeur d'enum | non garantie non plus | non garantie |
| Disposition des champs | décidée par le runtime | décidée par l'ABI, calculable |
| Conversion rétrécissante | cast explicite exigé | souvent silencieuse |

---

## 4. Manipulation

```bash
.\build\debug\bin\demo_01-types-et-memoire.exe
```

```bash
.\build\debug\bin\tests_01-types-et-memoire.exe --verbose --req
```

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M01-001 | `is_valid()` renvoie vrai pour tout membre déclaré de `SensorId` autre que `Count`. | `Enum.valeurs_nominales_valides` |
| LLR-M01-002 | `is_valid()` renvoie faux pour toute valeur brute ≥ `Count`. | `Enum.robustesse_valeur_hors_domaine` |
| LLR-M01-003 | `name_of()` renvoie le libellé du capteur, ou `"Inconnu"` si l'identifiant est hors domaine. Elle ne renvoie jamais `nullptr`. | `Enum.nom_de_chaque_membre` |
| LLR-M01-004 | `SensorId` occupe exactement 8 bits. | `Enum.type_sous_jacent_est_un_octet` |
| LLR-M01-010..014 | Les opérations `saturating_add/sub/mul` renvoient le résultat exact s'il est représentable sur `i16`, sinon la borne la plus proche. Elles sont évaluables à la compilation. | `Saturation.*` |
| LLR-M01-020..022 | `checked_add()` renvoie faux et met le résultat à 0 si la somme n'est pas représentable sur `i32`. | `Checked.addition_*` |
| LLR-M01-023..024 | `checked_div()` renvoie faux pour un diviseur nul et pour `INT_MIN / −1`. | `Checked.division_*` |
| LLR-M01-030..034 | `checked_cast()` n'écrit la destination que si la conversion préserve exactement la valeur et le signe. | `Cast.*` |
| LLR-M01-040..041 | `in_range()` teste un intervalle **fermé** ; `clamp()` ramène aux bornes. | `Range.*` |
| LLR-M01-050 | Les tailles de `TrameNaive` et `TrameCompacte` sont respectivement de 12 et 8 octets sur la cible de référence. | `Layout.bourrage_observable` |

---

## 6. Exercices

**6.1 — `saturating_abs`**
Implémentez `constexpr avio::i16 saturating_abs(avio::i16 v)`. Attention :
`abs(kI16Min)` n'est pas représentable. Écrivez d'abord l'**exigence**, puis les
cas de test (dont les bornes), puis le code. Dans cet ordre.

**6.2 — `checked_sub` et `checked_mul`**
Sur le modèle de `checked_add`, en testant **avant** l'opération. Pour la
multiplication, réfléchissez aux quatre combinaisons de signes.

**6.3 — Le piège du décompte**
Cette boucle est fausse. Trouvez pourquoi, corrigez-la de deux façons
différentes, et dites laquelle vous préférez :

```cpp
void traiter(const avio::u32* donnees, avio::u32 taille) {
    for (avio::u32 i = taille - 1U; i >= 0U; --i) {
        consommer(donnees[i]);
    }
}
```

**6.4 — Sérialisation portable**
Écrivez `void serialize(const TrameCompacte& t, avio::u8 out[6])` qui produit
une trame **big-endian**, indépendante de l'ABI, et son inverse `deserialize`.
Testez le couple aller-retour. C'est exactement ce que l'on fait pour un bus
ARINC 429.

**6.5 — Analyse d'exigence**
Réécrivez cette exigence pour la rendre vérifiable :
> *« Le module doit gérer correctement les grandes valeurs d'altitude. »*

Indices : quel domaine ? quelle résolution ? que veut dire « gérer » aux bornes
et hors bornes ? qui est le producteur de la donnée ?

---

## 7. Pour aller plus loin

* MISRA C++:2023, sections sur les conversions implicites et les types.
* *CWE-190* (Integer Overflow) et *CWE-197* (Numeric Truncation Error).
* Rapport de la commission d'enquête sur le vol 501 d'Ariane 5 (J.-L. Lions,
  1996) : dix pages, à lire une fois dans sa carrière.

---

⬅️ [00 — Environnement](../00-environnement/README.md) |
➡️ [02 — Pointeurs, références, const-correctness](../02-pointeurs-references-const/README.md)
