# Module 03 — RAII, cycle de vie et sémantique de déplacement

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 02

---

## Objectifs pédagogiques

1. Comprendre RAII et savoir l'appliquer à une ressource matérielle.
2. Connaître l'ordre exact de construction et de destruction des objets.
3. Appliquer la **règle de 0 / 3 / 5** et savoir laquelle choisir.
4. Distinguer **copie** et **déplacement**, et écrire un type à propriété unique.
5. Expliquer pourquoi RAII est un argument **de certification**, pas seulement
   de confort.

---

## 1. Le cours

### 1.1 RAII en une phrase

> **La durée de vie d'une ressource est celle d'un objet.**

On acquiert dans le constructeur, on libère dans le destructeur. Le langage
garantit l'appel du destructeur à la sortie de portée, quel que soit le chemin :
`return` anticipé, `break`, ou même exception.

```cpp
avio::i32 traitement(avio::i32 value) noexcept {
    const CriticalSection guard;   // interruptions coupées

    if (value < 0) { return 0; }   // guard détruit ici
    if (value == 0) { return 1; }  // ou ici
    return 2;                      // ou ici
}
```

Trois sorties, **zéro** ligne de libération écrite à la main, **zéro** chemin
oublié. Écrivez la même chose avec `disable_interrupts()` /
`enable_interrupts()` : il faut trois appels, et l'oubli sur une seule branche
ne se verra qu'en intégration — voire en vol.

### 1.2 Comparaison avec C#

| C# | C++ |
|---|---|
| `using (var l = new Lock()) { … }` | `{ ScopedLock l; … }` |
| il faut **penser** à écrire `using` | impossible à oublier |
| `IDisposable` + `Dispose()` | destructeur |
| finaliseur : appelé *un jour*, par le GC, sur un thread quelconque | destructeur : appelé **maintenant**, ici, sur ce thread |
| peut ne jamais être appelé | garanti par la norme |

C'est ce **déterminisme** que recherche l'avionique. Sur un calculateur qui
tourne à 100 Hz, « la ressource sera libérée à la prochaine collecte » n'est
pas une réponse acceptable.

### 1.3 Ordre de construction et de destruction

Trois règles à connaître par cœur :

1. **Objets locaux** : détruits dans l'ordre **inverse** de leur construction.
2. **Membres d'une classe** : construits dans l'ordre de **déclaration**
   (pas dans l'ordre de la liste d'initialisation !), détruits en ordre inverse.
3. **Classe de base** : construite avant les membres, détruite après.

Le point 2 est un piège classique :

```cpp
class Piege {
public:
    Piege() : b_(1), a_(b_) {}   // a_ est construit AVANT b_ : a_ lit du vide
private:
    int a_;   // déclaré en premier -> construit en premier
    int b_;
};
```

MSVC (`C5038`) et GCC (`-Wreorder`) signalent ce cas. C'est l'une des raisons
pour lesquelles un standard de codage impose que **l'ordre de la liste
d'initialisation suive l'ordre de déclaration**.

### 1.4 Les règles de 0, 3 et 5

Les **opérations spéciales** d'une classe :

1. destructeur
2. constructeur de copie
3. opérateur d'affectation par copie
4. constructeur de déplacement
5. opérateur d'affectation par déplacement

| Règle | Énoncé | Quand l'appliquer |
|---|---|---|
| **Règle de 0** | N'en déclarez **aucune**. | Cas par défaut. La classe n'agrège que des types qui gèrent déjà leur ressource. |
| **Règle de 3** | Si vous en déclarez une des trois premières, déclarez les trois. | Code C++03 historique. |
| **Règle de 5** | Si vous en déclarez une, statuez sur les **cinq**. | Dès qu'une classe possède une ressource. |

**Visez toujours la règle de 0.** Une classe qui ne gère aucune ressource
directement n'a aucun risque de fuite ni de double libération. Quand vous devez
en écrire, faites-le dans une classe minuscule dédiée (comme `ChannelHandle`),
que tout le reste du code peut ensuite utiliser sans y penser.

Le mot-clé `= delete` interdit explicitement une opération :

```cpp
ChannelHandle(const ChannelHandle&) = delete;             // copie interdite
ChannelHandle& operator=(const ChannelHandle&) = delete;
```

Sans cela, la copie par défaut dupliquerait le numéro de canal, et **les deux
destructeurs libéreraient le même canal** — double libération, corruption
silencieuse.

### 1.5 Déplacement (move)

```cpp
ChannelHandle(ChannelHandle&& other) noexcept : channel_(other.channel_) {
    other.channel_ = 0U;   // la source ABANDONNE la propriété
}
```

Trois choses à retenir :

* `std::move` **ne déplace rien**. C'est un simple `static_cast` vers une
  référence *rvalue* qui autorise la surcharge « déplacement » à être choisie.
* Après un déplacement, la source doit rester dans un état **valide mais non
  spécifié**. Bonne pratique en embarqué : la **neutraliser explicitement**, ce
  qui rend son état testable.
* Ne pas neutraliser la source = double libération garantie.

> **Vocabulaire** : `T&&` est une *référence rvalue*. Un `T&&` nommé est
> lui-même une lvalue — d'où la nécessité de `std::move(other.membre)` à
> l'intérieur d'un constructeur de déplacement.

### 1.6 Le piège de l'initialisation statique

Deux objets globaux dans deux `.cpp` différents : **l'ordre de leur
construction n'est pas spécifié** (*static initialization order fiasco*). Si
l'un utilise l'autre dans son constructeur, le comportement dépend de l'ordre
d'édition de liens.

Parades :
* pas d'objet global à constructeur non trivial — c'est la règle en avionique ;
* ou le *singleton de Meyers* : `static T& instance() { static T t; return t; }`,
  construit au premier appel (utilisé dans `microtest::Registry`) ;
* ou une **initialisation explicite** appelée depuis `main()`, ce qui est le
  choix le plus courant en embarqué certifié, car l'ordre devient **visible et
  traçable**.

---

## 2. Ce que dit la DO-178C

| Sujet | Exigence / objectif | Conséquence |
|---|---|---|
| **Déterminisme temporel** | §6.3.4 (accuracy and consistency), analyses de temps d'exécution | Un destructeur s'exécute à un instant connu ; un finaliseur GC, non. Le GC est de fait inutilisable en DAL A/B. |
| **Gestion des ressources** | A-5.6 | Toute ressource acquise doit être libérée sur **tous** les chemins. RAII transforme cette obligation en propriété structurelle du code. |
| **DO-332 (OOT)** — *Dynamic memory management* | OO.6.8.2 | La DO-332 exige de démontrer l'absence de fuite, de fragmentation et d'épuisement mémoire. La discipline RAII est un prérequis (voir aussi module 08). |
| **Robustesse** | A-6.3 | Le comportement en cas d'échec d'acquisition (`is_valid() == false`) est une exigence, à écrire puis à tester. |
| **Couverture structurelle** | A-7.x | Attention : les destructeurs sont du **code exécutable**. Ils apparaissent dans le rapport de couverture et doivent être couverts. Un destructeur jamais appelé = code mort. |

**Point d'attention** : un destructeur ne doit **jamais** échouer. En C++, il
est implicitement `noexcept` ; si une exception s'en échappe, `std::terminate`
est appelé. En avionique, un destructeur doit être court, sans allocation, sans
boucle non bornée — c'est du code qui s'exécute sur tous les chemins, y compris
les chemins d'erreur.

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ |
|---|---|---|
| Libération des ressources | `using` / `IDisposable`, à écrire | destructeur, automatique |
| Instant de libération | non déterministe (finaliseur) | déterministe, à l'instruction près |
| Copie d'objet | référence partagée (classe) | copie profonde par défaut (règle de 5) |
| Interdire la copie | `private` ctor + convention | `= delete`, imposé par le compilateur |
| Transfert de propriété | affectation de référence | `std::move` + neutralisation de la source |
| Ordre de destruction | indéterminé | strictement inverse de la construction |

---

## 4. Manipulation

```powershell
.\build\debug\bin\demo_03-raii-cycle-de-vie.exe
```

```powershell
.\build\debug\bin\tests_03-raii-cycle-de-vie.exe --verbose --req
```

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M03-001 | Les objets locaux d'une portée sont détruits dans l'ordre inverse de leur construction. | `LifeCycle.destruction_in_reverse_order` |
| LLR-M03-002 | Le nombre de destructions égale le nombre de créations (construction, copie, déplacement). | `LifeCycle.balanced_constructions_destructions` |
| LLR-M03-003 | Après déplacement, la source de `Traced` porte le tag 0 ; la destination porte le tag d'origine. | `LifeCycle.copy_then_move` |
| LLR-M03-004 | L'affectation d'un objet à lui-même laisse son état inchangé. | `LifeCycle.self_assignment_is_harmless` |
| LLR-M03-010 | `ChannelHandle` réserve un canal à la construction et le libère à la destruction. | `RAII.automatic_acquisition_and_release` |
| LLR-M03-011 | Lorsque les 4 canaux sont occupés, une nouvelle poignée est invalide (`channel() == 0`). | `RAII.channel_exhaustion` |
| LLR-M03-012 | `release()` est idempotent : un second appel n'a aucun effet. | `RAII.explicit_release_is_idempotent` |
| LLR-M03-013 | Le déplacement transfère la propriété : la source devient invalide, une seule libération a lieu. | `RAII.move_transfers_ownership` |
| LLR-M03-014 | L'affectation par déplacement libère la ressource détenue avant d'acquérir la nouvelle. | `RAII.move_assignment_releases_the_old` |
| LLR-M03-015 | `DeviceBank::release()` ignore tout identifiant de canal hors domaine. | `RAII.robustness_release_invalid_identifier` |
| LLR-M03-020 | `CriticalSection` désactive les interruptions à la construction et les rétablit à la destruction. | `CriticalSection.interrupts_restored` |
| LLR-M03-021 | Les sections critiques imbriquées ne rétablissent les interruptions qu'à la sortie de la plus externe. | `CriticalSection.nesting` |
| LLR-M03-022 | Les interruptions sont rétablies sur **tous** les chemins de sortie de `multi_exit_processing()`. | `CriticalSection.release_on_all_paths` |

---

## 6. Exercices

**6.1 — `ScopedTimer`**
Écrivez une classe RAII qui mesure le temps passé dans une portée et
l'accumule dans un compteur statique (utilisez un compteur de « ticks »
simulé, incrémenté à la main, pour rester déterministe et testable). Exigence,
tests, puis code.

**6.2 — Casser puis réparer**
Retirez `= delete` du constructeur de copie de `ChannelHandle`, recompilez,
puis écrivez un test qui copie une poignée. Observez : combien de libérations
pour un seul canal ? Expliquez pourquoi c'est une corruption mémoire dans un
vrai système, puis restaurez le `= delete`.

**6.3 — L'ordre des membres**
Écrivez une classe avec deux membres `Traced` et vérifiez par un test l'ordre
exact de construction et de destruction. Inversez ensuite l'ordre de la liste
d'initialisation (sans toucher aux déclarations) : que dit le compilateur ?
Que se passe-t-il réellement ?

**6.4 — Compteur d'imbrication**
`InterruptState` ne rétablit les interruptions qu'à la sortie de la section la
plus externe. Écrivez l'exigence correspondante, puis un test qui échouerait si
l'implémentation rétablissait les interruptions à chaque sortie.

**6.5 — Analyse de conception**
Un collègue propose d'ajouter à `ChannelHandle` une méthode
`ChannelHandle clone() const` qui réserve un second canal. Est-ce compatible
avec un type à propriété unique ? Quels tests faut-il ajouter ? Quel risque si
plus aucun canal n'est libre ?

---

## 7. Pour aller plus loin

* DO-332, section OO.6.8.2 — *Dynamic Memory Management*.
* C++ Core Guidelines, sections **R** (Resource management) et **C**
  (Classes) : R.1 (« gérer les ressources automatiquement »), C.20 (« si vous
  pouvez éviter de définir les opérations par défaut, faites-le »).
* Scott Meyers, *Effective Modern C++*, items 17 à 25 (opérations spéciales et
  sémantique de déplacement).

---

⬅️ [02 — Pointeurs et références](../02-pointeurs-references-const/README.md) |
➡️ [04 — Classes, invariants et types forts](../04-classes-invariants/README.md)
