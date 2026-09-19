# Module 12 — Couplage données et couplage contrôle

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 11

---

## Objectifs pédagogiques

1. Définir précisément **couplage de données** et **couplage de contrôle**.
2. Comprendre pourquoi l'objectif **A-7.8** existe séparément de MC/DC.
3. Établir les deux **matrices de couplage** d'un sous-système.
4. **Démontrer** par instrumentation que les tests d'intégration exercent
   toutes les interfaces.
5. Concevoir pour un couplage faible et **analysable**.

---

## 1. Le cours

### 1.1 Deux notions distinctes

Les définitions du glossaire DO-178C, mot pour mot :

> **Data coupling** — *The dependence of a software component on data not
> exclusively under the control of that software component.*

Tout ce qui **transite** entre composants : paramètres, valeurs de retour,
variables globales, mémoire partagée, messages de bus.
*Question type : « cette valeur est-elle bien en mètres à l'arrivée comme au
départ ? »*

> **Control coupling** — *The manner or degree by which one software component
> influences the execution of another software component.*

**Qui** appelle **qui**, dans quel **ordre**, sous quelle **condition**.
*Question type : « `reset()` est-il vraiment appelé après trois rejets, et
jamais avant ? »*

### 1.2 Pourquoi un objectif séparé

> On peut atteindre **100 % de MC/DC sur chaque composant** pris isolément, et
> n'avoir **jamais testé leur assemblage**.

Les défauts d'intégration — ordre d'appel inversé, donnée non initialisée au
premier cycle, unité non convertie à la frontière, état résiduel après une
panne — n'existent **qu'à l'assemblage**. Ce sont aussi les plus coûteux à
corriger, parce qu'ils sont découverts tard.

D'où l'objectif **A-7.8**, requis en DAL **A, B et C**.

### 1.3 Le sous-système du module

```
   ┌───────────────┐   read()      ┌───────────────┐
   │  SUPERVISOR   │ ────────────► │  ACQUISITION  │
   │               │ ◄──────────── │               │
   │               │  Result<f32>  └───────────────┘
   │               │
   │               │   push(f32)   ┌───────────────┐
   │               │ ────────────► │    FILTER     │
   │               │   average()   │  (moyenne     │
   │               │ ◄───────────► │   glissante   │
   │               │   reset()     │   sur 4)      │
   │               │ ┄┄┄┄┄┄┄┄┄┄┄►  │               │
   └───────────────┘ (conditionnel)└───────────────┘
```

### 1.4 Matrice de couplage de CONTRÔLE

| # | Appelant | Appelé | Condition d'appel | Exercé par |
|---|---|---|---|---|
| I1 | Supervisor | `Acquisition::read()` | **inconditionnel** | tous les cycles |
| I2 | Supervisor | `Filter::push()` | lecture réussie | `Coupling.cycle_nominal` |
| I3 | Supervisor | `Filter::average()` | lecture réussie | `Coupling.cycle_nominal` |
| I4 | Supervisor | `Filter::reset()` | **3 rejets consécutifs** | `Coupling.purge_after_rejections` |

> **I4 est le cas critique.** Couplage **conditionnel**, déclenché par une
> **séquence**. Aucun test unitaire de `Filter` ni d'`Acquisition` ne peut
> l'exercer : il n'existe qu'à l'assemblage. C'est exactement ce que vise A-7.8.

Notez aussi le test `Coupling.read_in_error` : il vérifie une **absence
d'appel** (le filtre ne doit pas être sollicité si la lecture a échoué).
Vérifier qu'une interface **n'est pas** exercée dans un cas donné fait partie
de l'analyse.

### 1.5 Matrice de couplage de DONNÉES

| # | Producteur | Consommateur | Donnée | Unité | Domaine | Validité |
|---|---|---|---|---|---|---|
| D1 | Acquisition | Supervisor | mesure convertie | unité capteur | [0 ; 1023,75] | `Status::Ok` seulement |
| D2 | Supervisor | Filter | échantillon | unité capteur | idem D1 | non finie rejetée |
| D3 | Filter | Supervisor | moyenne | unité capteur | [0 ; 1023,75] | `NotReady` si fenêtre incomplète |

**La colonne « unité » n'est pas décorative.** C'est à la frontière D1 que
l'échelle est fixée (`kScaleUnitsPerCount = 0,25`). Une erreur d'échelle à cet
endroit se propage silencieusement dans toute la chaîne aval — le scénario
Mars Climate Orbiter (module 04), à l'échelle d'un sous-système.

Le test `Coupling.data_forwarded_without_alteration` vérifie précisément que la
valeur produite par `Acquisition` est **exactement** celle reçue par `Filter`.

### 1.6 Démontrer, pas affirmer

L'outillage du module (`coupling_trace.hpp`, **non embarqué**) enregistre
chaque appel et chaque donnée échangée. Les tests peuvent alors **prouver** :

```cpp
CHECK_EQ(CouplingTrace::unexercised_count(), usize{0});
CHECK(CouplingTrace::all_interfaces_exercised());
```

Et — point de méthode essentiel — un test vérifie que **l'outil détecte
vraiment** une interface manquante (`Coupling.interface_not_exercised_detected`).
Sans lui, `all_interfaces_exercised()` pourrait renvoyer vrai en permanence et
la démonstration ne prouverait rien. C'est la DO-330 appliquée à l'outillage.

### 1.7 La décision de conception qui rend tout cela possible

```cpp
template <typename AcquisitionT, typename FilterT>
class Supervisor { … };

using FlightSupervisor = Supervisor<Acquisition, Filter>;      // embarqué
using TracedSupervisor  = Supervisor<TracingAcquisition, TracingFilter>;  // vérif.
```

Trois bénéfices :

1. le couplage est **explicite dans le type** — il ne peut pas être introduit
   en catimini par une variable globale ;
2. les tests substituent des composants **instrumentés** sans modifier le code
   de production : **le binaire vérifié reste rigoureusement le binaire
   embarqué**, ce qui n'est *pas* le cas d'une instrumentation par `#ifdef` ;
3. aucune vtable, aucun coût à l'exécution (module 06).

Le point 2 est un argument de certification, pas de confort. Si vous
instrumentez le code de production avec des `#ifdef TRACE`, vous vérifiez un
binaire qui n'est pas celui qui vole — et il faut alors justifier l'équivalence.

### 1.8 Les cinq règles qui réduisent le couplage

| # | Règle | Pourquoi |
|---|---|---|
| 1 | **Aucune variable globale mutable** partagée | une globale crée un couplage de données **invisible dans les signatures** : la matrice devient impossible à établir par lecture. La règle la plus rentable du lot. |
| 2 | **Dépendances explicites dans le type** | le couplage est dans la signature, et les tests peuvent substituer |
| 3 | **Interfaces étroites** | chaque méthode publique est une ligne de la matrice à exercer |
| 4 | **Pas de rappel non résolu statiquement** | un pointeur de fonction rend le graphe d'appel indéterminable — donc l'analyse de couplage *et* l'analyse de pile (module 08) impossibles |
| 5 | **Ordre d'appel spécifié dans le SDD** | « après 3 rejets consécutifs » est une **exigence** (LLR-CHAIN-031), donc testable et traçable |

---

## 2. Ce que dit la DO-178C

| Objectif | Intitulé | Niveau |
|---|---|---|
| **A-7.8** | *Verification of Software Integration Process* — couplage données et contrôle vérifié | A, B, C |
| **§6.4.4.2.c** | L'analyse de couverture confirme le couplage données/contrôle | — |
| **A-5.2** | Le code source est conforme à l'architecture | — |
| **A-4.11** | L'architecture logicielle est vérifiable | — |
| **§2.4** | *Freedom from interference* entre composants | — |

**Formulation d'auditeur, à savoir anticiper** :

> *« Montrez-moi la liste des interfaces entre vos composants, et pour chacune,
> le ou les cas de test qui l'exercent. »*

C'est exactement ce que produit ce module.

---

## 3. Manipulation

```powershell
.\build\debug\bin\demo_12-couplage-donnees-controle.exe
```

```powershell
.\build\debug\bin\tests_12-couplage-donnees-controle.exe --verbose --req
```

```bash
python tools/trace_check.py
```

---

## 4. Exercices

**4.1 — Ajouter une interface**
Ajoutez à `Filter` une méthode `f32 last_sample() const` et faites-la appeler
par le superviseur en cas d'erreur de moyenne. Mettez à jour : la matrice de
contrôle, l'énumération `Interface`, le décorateur `TracingFilter`, et les
tests. Combien de fichiers avez-vous touchés ? C'est le **coût réel** d'une
interface supplémentaire en projet certifié.

**4.2 — L'erreur d'unité**
Changez `kScaleUnitsPerCount` de `0,25` à `0,025` dans `Acquisition`. Quels
tests échouent ? Combien de temps pour identifier la cause ? Puis imaginez le
même défaut sans le test `Coupling.data_forwarded_without_alteration` : comment
l'auriez-vous trouvé ?

**4.3 — La variable globale**
Remplacez le passage par paramètre entre `Supervisor` et `Filter` par une
variable globale `g_dernier_echantillon`. Le code compile et les tests passent.
Établissez maintenant la matrice de couplage de données **par lecture des
signatures** : que constatez-vous ? Rédigez l'argument que vous opposeriez à ce
changement en revue de code.

**4.4 — Le couplage temporel caché**
`Filter::average()` renvoie `NotReady` tant que la fenêtre n'est pas pleine.
C'est un couplage de contrôle **implicite** : le superviseur dépend de
l'historique du filtre. Écrivez l'exigence qui le rend explicite, puis le test
qui vérifie qu'aucune valeur n'est produite avant le 4ᵉ cycle.

**4.5 — Analyse d'un vrai système**
Prenez le module 16 (projet intégré) et établissez ses deux matrices de
couplage. Combien d'interfaces ? Combien sont conditionnelles ? Lesquelles
demandent une **séquence** pour être exercées ?

---

## 5. Pour aller plus loin

* DO-178C §6.4.4.2.c et glossaire (définitions de *data coupling* et *control
  coupling*).
* **CAST-19** — *Clarification of Structural Coverage Analyses of Data Coupling
  and Control Coupling*. Le document de référence sur le sujet, public et
  gratuit. À lire avant tout entretien sur ce thème.
* **CAST-32A** — *Multi-core Processors* : le couplage y prend une dimension
  supplémentaire (ressources partagées, interférences temporelles).
* ARINC 653 : la ségrégation par partitions est la réponse architecturale au
  couplage non maîtrisé.

---

⬅️ [11 — Couverture structurelle](../11-couverture-structurelle/README.md) |
➡️ [13 — Standards de codage et analyse statique](../13-standards-codage/README.md)
