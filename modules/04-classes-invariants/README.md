# Module 04 — Classes, invariants et types forts

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 03

---

## Objectifs pédagogiques

1. Concevoir une classe autour d'un **invariant** et le garantir par
   construction.
2. Maîtriser `explicit`, les **fabriques nommées** et le constructeur privé.
3. Surcharger les opérateurs à bon escient — et savoir quand **ne pas** le faire.
4. Appliquer les **types forts** (unités physiques) et comprendre pourquoi
   ils coûtent zéro à l'exécution.
5. Rédiger un invariant sous forme d'exigence de bas niveau vérifiable.

---

## 1. Le cours

### 1.1 `class` et `struct`

En C++, la seule différence est la visibilité par défaut : `private` pour
`class`, `public` pour `struct`. **Les deux sont des types valeur.**

Ce n'est pas du tout la distinction C#, où `class` est un type référence
(alloué sur le tas, copié par référence) et `struct` un type valeur. En C++,
`class Foo` et `struct Foo` se copient tous les deux à l'affectation.

Convention du projet, courante dans l'industrie :
* `struct` → agrégat de données sans invariant (une trame, un point) ;
* `class` → un invariant à maintenir, donc des membres privés.

### 1.2 L'invariant

> Un **invariant** est une propriété vraie à chaque instant où l'objet est
> observable de l'extérieur.

Exemple : `0 ≤ quantité ≤ capacité`.

Trois conditions pour le garantir :

1. **Données membres privées** — personne ne peut écrire directement.
2. **Tout constructeur l'établit** — y compris le constructeur par défaut.
3. **Toute méthode publique le préserve** — y compris face à des entrées
   absurdes.

C'est le point 3 qui demande de la rigueur. `add(9 000 000 g)` dans un
réservoir de 5 kg ne doit pas violer l'invariant : on ajoute ce qui rentre, et
on **renvoie la quantité effectivement ajoutée**. Le contrat est explicite,
l'appelant sait ce qui s'est passé.

> **Correspondance DO-178C** : un invariant est exactement ce qu'on écrit dans
> une **exigence de bas niveau** et dans le SDD. « `FuelTank` garantit à tout
> instant que la quantité est comprise entre 0 et la capacité » est une phrase
> de document de conception, directement vérifiable par test.

### 1.3 Constructeur privé + fabriques nommées

```cpp
class Mass {
public:
    static bool from_grams(avio::i32 grams, Mass& out) noexcept;
    static bool from_kilograms(avio::f32 kg, Mass& out) noexcept;
    static bool from_pounds(avio::f32 lb, Mass& out) noexcept;
private:
    explicit constexpr Mass(avio::i32 grams) noexcept : grams_(grams) {}
};
```

Deux problèmes résolus d'un coup :

* **On ne peut pas surcharger un constructeur sur l'unité** : `Mass(f32)` en
  kilogrammes et `Mass(f32)` en livres ont la même signature. Le **nom** de la
  fabrique porte l'information.
* **La validation ne peut pas être contournée** : le constructeur étant privé,
  toute instance existante a forcément traversé un contrôle de domaine.

Le couple `(bool, out)` plutôt qu'un lancement d'exception : c'est le style
imposé en avionique (module 07).

### 1.4 `explicit`

```cpp
class Distance { public: Distance(avio::f32 m); };   // SANS explicit
void voler(Distance d);
voler(3.5F);   // compile ! conversion implicite float -> Distance
```

Sans `explicit`, un constructeur à un argument devient un **opérateur de
conversion implicite**. C'est presque toujours indésirable. Règle du projet
(et de MISRA) : **tout constructeur à un seul argument est `explicit`**, sauf
justification.

### 1.5 Types forts : le retour sur investissement

Les deux accidents cités en tête de `units.hpp` :

* **Air Canada 143 (1983)** — Boeing 767 en panne sèche à 12 500 m. La quantité
  de carburant a été calculée avec le facteur **livres**/litre au lieu de
  **kilogrammes**/litre. L'avion embarquait 45 % du carburant nécessaire.
* **Mars Climate Orbiter (1999)** — un logiciel produisait des impulsions en
  livres-force·seconde, l'autre les lisait en newton·seconde. Sonde perdue.

Dans les deux cas, **tous les nombres étaient corrects**. C'est leur *unité*
qui ne l'était pas. Un `double` ne porte pas son unité ; un type fort, si.

Coût : `sizeof(Altitude) == sizeof(float)`, et le compilateur génère
**exactement** le même code machine. C'est de l'abstraction à coût nul — la
promesse fondatrice de C++.

### 1.6 Surcharge d'opérateurs : ce qu'il faut et ce qu'il ne faut pas

| Opérateur | `Mass` (entier) | `Altitude` (flottant) | Pourquoi |
|---|---|---|---|
| `==`, `!=` | **oui** | **non** | L'égalité exacte n'a de sens que sur une représentation entière. |
| `<`, `>`, `<=`, `>=` | oui | oui | L'ordre est bien défini sur les flottants finis. |
| `+`, `-` | oui, **saturants** | non fourni | Une masse est bornée ; additionner deux altitudes n'a pas de sens physique. |
| `*`, `/` | non | non | Une masse × une masse n'est pas une masse : il faudrait un système d'unités complet. |

**Ne pas fournir `operator==` sur `Altitude` est un choix de conception
délibéré.** Cela oblige l'appelant à écrire `a.is_close(b, tolerance)`, donc à
choisir — et à documenter — sa tolérance. En DO-178C, cette tolérance appartient
à l'exigence, pas au code. Un `==` implicite la cacherait.

Règles générales de surcharge :
* ne surchargez que si la sémantique est **évidente** pour le lecteur ;
* respectez les propriétés attendues (symétrie de `==`, transitivité de `<`) ;
* préférez des **fonctions membres `const`** ou des fonctions libres `friend`.

---

## 2. Ce que dit la DO-178C

| Sujet | Objectif | Application |
|---|---|---|
| **A-4.1** — LLR conformes aux HLR | Les invariants de classe sont des LLR. | « 0 ≤ quantité ≤ capacité » se trace vers une HLR système. |
| **A-4.7** — Algorithmes exacts | La conversion pieds/mètres, le facteur livre/kg | Toute constante de conversion doit être **tracée** à une source normative (ici : la définition légale de la livre, 0,45359237 kg). |
| **A-5.6** — accuracy and consistency | Domaine de validité, NaN, débordement | Les fabriques rejettent NaN, ±∞ et hors domaine. |
| **A-6.2 / A-6.4** — robustesse de l'exécutable | Comportement pour entrées hors domaine | Testé explicitement (`robustesse_*`). |
| **DO-332 OO.6.8.1** — cohérence de type | Typage fort | Les types forts sont un moyen direct de satisfaire l'objectif : la confusion de types devient impossible. |

**À retenir pour un entretien** : savoir dire *« l'invariant de cette classe
est X ; il est établi par le constructeur, préservé par chaque méthode
publique, et vérifié par le test Y »* montre immédiatement que vous avez
compris ce que la norme attend.

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ |
|---|---|---|
| `class` / `struct` | référence / valeur | **les deux sont valeur** |
| Propriétés | `public int X { get; private set; }` | méthode `x()` const + membre privé |
| Constructeur validant | lève une exception | `(bool, out)` ou fabrique statique |
| Conversion implicite | `implicit operator` (opt-in) | constructeur à 1 argument, **opt-out** via `explicit` |
| Records / égalité de valeur | `record`, `==` généré | à écrire à la main, et parfois à **ne pas** écrire |
| Types forts | `readonly struct` + opérateurs | même chose, coût nul aussi |

---

## 4. Manipulation

```powershell
.\build\debug\bin\demo_04-classes-invariants.exe
```

```powershell
.\build\debug\bin\tests_04-classes-invariants.exe --verbose --req
```

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M04-001 | Une `Mass` construite par défaut vaut 0 g et respecte son invariant. | `Mass.valid_default_state` |
| LLR-M04-002..004 | Les fabriques `from_grams/kilograms/pounds` convertissent selon les facteurs 1 kg = 1000 g et 1 lb = 453,59237 g. | `Mass.from_*` |
| LLR-M04-005 | Toute valeur négative est refusée et la sortie reste inchangée. | `Mass.robustness_negative_value` |
| LLR-M04-006 | Toute valeur supérieure à `kMaxGrams` (200 t) est refusée ; `kMaxGrams` lui-même est accepté. | `Mass.robustness_out_of_domain_high` |
| LLR-M04-007 | NaN et ±∞ sont refusés par toutes les fabriques flottantes. | `Mass.robustness_nan_and_infinity` |
| LLR-M04-010..012 | Les comparaisons sont exactes ; l'addition sature à `kMaxGrams` ; la soustraction est bornée à 0. | `Mass.*` |
| LLR-M04-020..022 | `Altitude` accepte le domaine [−2000 ; +60000] ft bornes incluses et convertit à 3,280839895 ft/m. | `Altitude.*` |
| LLR-M04-023 | NaN et ±∞ sont refusés. | `Altitude.robustness_nan_and_infinity` |
| LLR-M04-024..025 | `is_close()` compare à une tolérance explicite et renvoie faux pour toute tolérance négative ou NaN. | `Altitude.comparison_with_tolerance`, `Altitude.robustness_invalid_tolerance` |
| LLR-M04-026..027 | L'ordre est total sur les altitudes finies ; la conversion aller-retour pieds→mètres→pieds reste dans une tolérance de 0,1 ft. | `Altitude.*` |
| LLR-M04-030 | `FuelTank::create()` produit un réservoir vide de la capacité demandée. | `FuelTank.nominal_creation` |
| LLR-M04-031 | Une capacité nulle est refusée ; `fill_ratio_percent()` vaut alors 0 (aucune division par zéro). | `FuelTank.robustness_zero_capacity` |
| LLR-M04-032..034 | `add()` et `remove()` renvoient la quantité **effective**, bornée par la place disponible et par le contenu. | `FuelTank.*` |
| LLR-M04-035 | L'invariant `0 ≤ quantité ≤ capacité` est vrai après toute séquence d'opérations. | `FuelTank.long_sequence_invariant_always_true` |
| LLR-M04-040 | Deux masses issues d'unités différentes sont comparables sans ambiguïté après construction. | `StrongType.same_mass_two_units` |

---

## 6. Exercices

**6.1 — `Temperature`**
Créez un type fort `Temperature` stockant des centièmes de kelvin (entier).
Fabriques : `from_celsius`, `from_fahrenheit`, `from_kelvin`. Domaine :
[0 K ; 1000 K]. Interdisez les températures sous le zéro absolu **par
construction**. Exigences, tests (dont bornes et NaN), puis code.

**6.2 — Casser l'invariant**
Rendez `quantity_` public dans `FuelTank`, écrivez un test qui viole
l'invariant, constatez que `invariant_holds()` renvoie faux. Puis remettez le
membre en privé. Conclusion à écrire en deux phrases.

**6.3 — L'opérateur qui manque**
Faut-il ajouter `Mass operator*(Mass, avio::f32)` (masse × facteur) ? Et
`Mass operator*(Mass, Mass)` ? Justifiez chaque réponse en termes d'**unités**
et de **domaine de validité**. Implémentez celui qui a du sens, avec sa
saturation et ses tests.

**6.4 — Deux réservoirs**
Écrivez une fonction
`bool transfer(FuelTank& source, FuelTank& destination, Mass demande, Mass& transfere)`
qui déplace du carburant. Quels sont les cas de robustesse ? (source vide,
destination pleine, source et destination identiques…). Écrivez les exigences
d'abord.

**6.5 — Rédaction d'exigence**
Reformulez en exigence vérifiable :
> *« Le système doit refuser les altitudes aberrantes. »*

Puis écrivez les cas de test correspondants, en distinguant nominal et
robustesse.

---

## 7. Pour aller plus loin

* Rapport final du Bureau de la sécurité des transports du Canada sur le
  vol Air Canada 143.
* NASA, *Mars Climate Orbiter Mishap Investigation Board Phase I Report* (1999).
* C++ Core Guidelines, section **C** (Classes) : C.2, C.41, C.46, C.161.
* Bibliothèques d'unités réelles : `mp-units`, `units` — même principe, poussé
  jusqu'à l'analyse dimensionnelle complète.

---

⬅️ [03 — RAII](../03-raii-cycle-de-vie/README.md) |
➡️ [05 — Polymorphisme et DO-332](../05-polymorphisme-do332/README.md)
