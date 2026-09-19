# Module 05 — Polymorphisme dynamique et supplément DO-332

> **Durée estimée** : 1 à 2 journées (module dense)
> **Prérequis** : modules 00 à 04

C'est **le** module qui distingue un développeur C++ d'un développeur C++
*avionique*. Le polymorphisme est une technique banale ailleurs ; ici, elle
déclenche des objectifs de certification supplémentaires.

---

## Objectifs pédagogiques

1. Écrire une hiérarchie polymorphe correcte : `virtual`, `override`, `final`,
   destructeur virtuel.
2. Comprendre le **coût réel** de la résolution dynamique (vtable, indirection).
3. Reconnaître et éviter le **découpage** (*slicing*).
4. Expliquer la **cohérence locale de type** de la DO-332 et savoir la
   **démontrer** par le test pessimiste.
5. Citer les six vulnérabilités identifiées par la DO-332 et les parades
   appliquées.

---

## 1. Le cours

### 1.1 Les mécanismes de base

```cpp
class Sensor {
public:
    virtual ~Sensor() noexcept = default;              // OBLIGATOIRE
    virtual avio::f32 to_engineering(avio::i32 raw) const noexcept = 0;  // pure
};

class PressureSensor final : public Sensor {           // `final` : pas d'héritage
public:
    avio::f32 to_engineering(avio::i32 raw) const noexcept override;  // `override`
};
```

| Mot-clé | Rôle | Équivalent C# |
|---|---|---|
| `virtual` | rend la méthode redéfinissable | `virtual` |
| `= 0` | méthode **pure** → classe abstraite | `abstract` |
| `override` | vérifie qu'on redéfinit bien | `override` |
| `final` | interdit toute redéfinition / dérivation | `sealed` |
| `virtual ~T()` | destruction polymorphe correcte | inutile (GC) |

**Différence majeure avec C# : en C++, une méthode n'est *pas* virtuelle par
défaut.** Oublier `virtual` ne produit aucune erreur : l'appel est simplement
résolu statiquement, et votre redéfinition n'est jamais appelée. `override`
est votre filet de sécurité — utilisez-le **systématiquement**.

> Le destructeur virtuel n'est pas facultatif. Détruire un objet dérivé à
> travers un pointeur vers une base sans destructeur virtuel est un
> **comportement indéfini**. Ce dépôt le vérifie à la compilation :
> `static_assert(std::has_virtual_destructor_v<Sensor>, ...)`.

### 1.2 Le coût

Une classe polymorphe contient un pointeur caché, le **vptr**, vers sa table
de fonctions virtuelles (*vtable*).

* **Mémoire** : +8 octets par *objet* sur cible 64 bits (+4 sur 32 bits).
* **Temps** : une indirection mémoire supplémentaire par appel, et
  l'**inlining devient impossible** — l'optimiseur ne sait pas quelle fonction
  sera appelée.
* **WCET** : le temps d'exécution au pire cas devient celui de la
  **redéfinition la plus lente**. C'est une contrainte réelle sur les analyses
  temporelles (module 15).

Vérifiez-le vous-même :
`sizeof(PressureSensor) == sizeof(void*)`, alors que la classe n'a **aucune**
donnée membre.

### 1.3 Le découpage (*slicing*)

```cpp
avio::u16 length_by_value(Message message);        // COPIE
avio::u16 length_by_reference(const Message& m);   // référence

ExtendedMessage msg(0x101, 20);
length_by_value(msg);      // -> 4  : la partie dérivée a été DÉCOUPÉE
length_by_reference(msg);  // -> 24 : correct
```

Passer un objet dérivé **par valeur** à une fonction qui attend la base copie
uniquement la partie base. Le sous-type disparaît, et avec lui la résolution
dynamique. Aucun avertissement du compilateur par défaut.

**Parade structurelle** : interdire la copie sur toute classe de base
polymorphe.

```cpp
Sensor(const Sensor&) = delete;
Sensor& operator=(const Sensor&) = delete;
```

Le problème devient alors une erreur de compilation. C'est ce que fait ce
module.

### 1.4 La cohérence locale de type (DO-332, objectif OO.6.7)

> *Partout où une référence vers un type T est utilisée, toute instance d'un
> sous-type de T doit se comporter conformément au **contrat** de T.*

C'est le principe de substitution de Liskov, promu au rang d'objectif de
certification. Concrètement, un sous-type ne doit pas :

* **renforcer** une précondition (exiger plus de l'appelant que la base) ;
* **affaiblir** une postcondition (garantir moins que la base) ;
* **casser** un invariant de la base.

La DO-332 propose **deux moyens** de le démontrer :

| Moyen | Description | Coût |
|---|---|---|
| **(a) Vérification formelle du LSP** | Démontrer par analyse que chaque redéfinition respecte les pré/postconditions. | Élevé, nécessite des contrats formalisés. |
| **(b) Test pessimiste** | Rejouer, pour **chaque** sous-type, la totalité des cas de test écrits pour le type de base. | Modéré, mécanisable. |

Ce module outille le moyen (b). Le contrat de `Sensor` est écrit **en toutes
lettres** dans l'en-tête, en sept clauses C1 à C7, et
`verify_contract(const Sensor&, u32)` les vérifie sur n'importe quelle
instance.

```cpp
TEST_REQ(DO332, pressure_sensor_is_substitutable, "LLR-M05-001,OO.6.7") {
    const mod05::PressureSensor sensor;
    check_conforming_subtype(sensor);   // la campagne de la BASE
}
```

Ajouter un nouveau capteur au système coûte alors **une ligne de test**.

### 1.5 Le capteur volontairement fautif

`BrokenSensor` compile, s'utilise normalement, et passerait n'importe quel test
nominal écrit rapidement. Il viole pourtant :

* **C5** — sa conversion n'est plus monotone au-delà de 3000 (une « correction »
  ajoutée après coup sans relire le contrat : le scénario le plus fréquent en
  maintenance) ;
* **C6** — `to_engineering(raw_max)` ne vaut plus `value_max` ;
* **C7** — aucun écrêtage hors domaine.

Il n'existe que pour **prouver que le harnais détecte réellement les
violations**. Sans ce test, les deux tests précédents ne démontreraient rien :
un `verify_contract` qui renverrait toujours « conforme » les ferait passer
aussi. C'est le principe du *test du test*, que la DO-330 applique aux outils
de vérification.

### 1.6 Les six vulnérabilités de la DO-332

| # | Vulnérabilité | Parade appliquée ici | Module |
|---|---|---|---|
| 1 | Héritage et polymorphisme | contrat explicite + test pessimiste ; hiérarchies plates | 05 |
| 2 | Polymorphisme paramétrique (templates) | couverture **par instanciation** | 06 |
| 3 | Surcharge de fonctions | pas de surcharge sur types convertibles | 06 |
| 4 | Conversions de type | pas de `dynamic_cast`, pas de RTTI, pas de copie polymorphe | 05 |
| 5 | Gestion dynamique de la mémoire | aucune allocation après initialisation | 08 |
| 6 | Exceptions | interdites, `noexcept` partout | 07 |

**Règles pratiques du domaine**, appliquées dans ce dépôt :

* hiérarchies **plates** : une interface abstraite, des feuilles `final` ;
* **pas d'héritage multiple d'implémentation** (l'héritage multiple
  d'interfaces pures est parfois toléré) ;
* **pas de `dynamic_cast`, pas de `typeid`** — RTTI désactivée en production
  (`/GR-` sous MSVC, `-fno-rtti` sous GCC/Clang) : le coût mémoire et le
  caractère non déterministe de la recherche de type sont rédhibitoires ;
* destructeur virtuel obligatoire ;
* classes de base polymorphes **non copiables** ;
* `override` systématique, `final` dès que possible.

### 1.7 Faut-il vraiment du polymorphisme dynamique ?

Question honnête, à se poser à chaque fois. Le dispatch dynamique se justifie
quand le nombre de types varie *à l'exécution* et que le code appelant doit
rester générique. Si l'ensemble des types est **connu à la compilation** — ce
qui est le cas de la quasi-totalité des systèmes embarqués certifiés — deux
alternatives coûtent moins cher en vérification :

* la **composition** + un `switch` sur un `enum class` — parfaitement
  analysable, WCET calculable ;
* le **polymorphisme statique** (templates, CRTP) — coût nul, mais couverture
  par instanciation (module 06).

Beaucoup d'équipes n'utilisent le polymorphisme dynamique que pour les
**interfaces matérielles**, précisément parce que c'est là que la substitution
sert vraiment (banc de test contre calculateur réel).

---

## 2. Ce que dit la DO-332

| Objectif | Intitulé | Application |
|---|---|---|
| **OO.6.7** | Local Type Consistency verification | Le cœur du module : `verify_contract` + rejeu sur chaque sous-type. |
| **OO.6.8.1** | Type conversion | Pas de downcast, pas de RTTI, pas de slicing. |
| **OO.6.8.2** | Dynamic memory management | Aucune allocation : les capteurs vivent sur la pile (module 08). |
| **A-7.x** | Couverture structurelle | Chaque redéfinition virtuelle est du code à couvrir. Une méthode virtuelle jamais appelée dans les tests = code non couvert. |
| **A-4.9, A-4.11** | Architecture cohérente et vérifiable | La profondeur de la hiérarchie et le nombre de niveaux virtuels sont des paramètres d'architecture à justifier dans le SDD. |

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ |
|---|---|---|
| Virtualité par défaut | non virtuelle (`virtual` explicite) | **idem**, mais oublier `override` ne prévient pas |
| Destruction polymorphe | prise en charge par le GC | `virtual ~T()` obligatoire, sinon UB |
| Découpage | impossible (types référence) | **possible et silencieux** |
| `sealed` / `final` | optimisation | optimisation **et** exigence de conception |
| `as` / `is` | idiomatique | `dynamic_cast` — **interdit** en avionique |
| Interfaces | `interface`, héritage multiple | classe abstraite pure ; héritage multiple encadré |
| Coût | le JIT dévirtualise souvent | vtable, pas d'inlining, WCET pénalisé |

---

## 4. Manipulation

```powershell
.\build\debug\bin\demo_05-polymorphisme-do332.exe
```

```powershell
.\build\debug\bin\tests_05-polymorphisme-do332.exe --verbose --req
```

Notez dans la sortie `--req` la traçabilité vers `OO.6.7` : les objectifs de
la DO-332 se tracent comme n'importe quelle exigence.

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M05-001 | `PressureSensor` satisfait les sept clauses du contrat de `Sensor`. | `DO332.pressure_sensor_is_substitutable` |
| LLR-M05-002 | `TemperatureSensor` satisfait les sept clauses du contrat de `Sensor`. | `DO332.temperature_sensor_is_substitutable` |
| LLR-M05-003 | `verify_contract()` détecte les violations des clauses C5, C6 et C7 et désigne la première clause violée. | `DO332.the_harness_detects_a_violation` |
| LLR-M05-004 | `verify_contract()` ramène tout `sample_count` inférieur à 2 à la valeur 2. | `DO332.robustness_degenerate_sampling` |
| LLR-M05-010..011 | `PressureSensor` convertit linéairement 0..4095 vers 0..1200 hPa et écrête hors domaine. | `Pressure.*` |
| LLR-M05-012 | `TemperatureSensor` convertit linéairement 0..4095 vers −60..+80 °C. | `Temperature.conversion_with_shift` |
| LLR-M05-013 | `Sensor::is_in_range()` renvoie vrai si et seulement si `raw ∈ [raw_min ; raw_max]`. | `Sensor.common_non_virtual_method` |
| LLR-M05-020 | L'appel via une référence vers `Sensor` sélectionne la redéfinition du type dynamique. | `Polymorphism.dynamic_resolution` |
| LLR-M05-021 | Une classe polymorphe sans donnée membre occupe la taille d'un pointeur. | `Polymorphism.vtable_pointer_memory_cost` |
| LLR-M05-030..031 | Le passage par valeur découpe le sous-type ; le passage par référence le préserve. | `Slicing.*` |

---

## 6. Exercices

**6.1 — Un troisième capteur conforme**
Ajoutez `AngleOfAttackSensor` : brut 0..4095 → −20,0..+45,0 degrés. Écrivez
**une seule** ligne de test qui rejoue le contrat. Vérifiez qu'elle passe du
premier coup si votre implémentation est correcte.

**6.2 — Trois violations, trois détections**
Créez trois sous-types fautifs, chacun violant **exactement une** clause
(C4, C6, C7). Vérifiez que `verify_contract` désigne la bonne clause à chaque
fois. C'est de l'analyse de mutation appliquée à un harnais de vérification.

**6.3 — Le destructeur manquant**
Retirez `virtual` du destructeur de `Sensor`. Que dit le `static_assert` ?
Que dirait clang-tidy (`cppcoreguidelines-virtual-class-destructor`) ?
Expliquez en trois phrases ce qui se passerait à l'exécution si les capteurs
étaient alloués dynamiquement.

**6.4 — Le cas du `switch`**
Réécrivez le système sans aucune fonction virtuelle : une `struct SensorConfig`
portant un `enum class SensorKind` et les bornes, plus une fonction libre
`f32 to_engineering(const SensorConfig&, i32 raw)`. Comparez :
taille mémoire, nombre de branches à couvrir, prévisibilité du WCET,
facilité d'ajout d'un capteur. Quelle solution défendriez-vous en revue de
conception ?

**6.5 — Écrire un contrat**
Prenez une interface de votre choix (par exemple `IFilter` avec
`f32 apply(f32 input)`) et rédigez son contrat en clauses numérotées, comme
`Sensor`. Puis écrivez le `verify_contract` correspondant. C'est l'exercice le
plus proche du travail réel en équipe DO-178C.

---

## 7. Pour aller plus loin

* **DO-332**, sections OO.6.7 (Local Type Consistency) et OO.D (rationale) —
  le document de référence, à demander à votre employeur.
* Barbara Liskov, *Data Abstraction and Hierarchy* (1987) — l'origine du LSP.
* AUTOSAR C++14, règles A10-x (héritage) et A12-x (opérations spéciales).
* CAST-32A pour les aspects multicœur, où la question du déterminisme des
  appels virtuels se pose à nouveau.

---

⬅️ [04 — Classes et invariants](../04-classes-invariants/README.md) |
➡️ [06 — Templates et polymorphisme statique](../06-templates-constexpr/README.md)
