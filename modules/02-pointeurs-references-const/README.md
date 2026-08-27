# Module 02 — Pointeurs, références et const-correctness

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 et 01

---

## Objectifs pédagogiques

1. Choisir en connaissance de cause entre **référence**, **pointeur** et
   **valeur** dans une signature.
2. Lire et écrire correctement les quatre formes de `const`.
3. Comprendre le *decay* des tableaux et pourquoi il est la cause première des
   débordements de tampon.
4. Concevoir des API où **la taille voyage avec la donnée** (`avio::Span`).
5. Identifier les fautes de durée de vie (*dangling*) que le compilateur ne
   détecte pas.
6. Relier ces choix aux contraintes MISRA C++ et à la notion de *freedom from
   interference*.

---

## 1. Le cours

### 1.1 Valeur, référence, pointeur

```cpp
void par_valeur(Mesure m);              // copie
void par_reference(Mesure& m);          // alias modifiable, jamais nul
void par_reference_const(const Mesure& m);  // alias en lecture, jamais nul
void par_pointeur(Mesure* m);           // peut être nul, peut être redirigé
```

**Règle de décision** :

| Situation | Choix |
|---|---|
| Type petit (≤ 2 mots machine) lu seulement | **par valeur** |
| Type volumineux lu seulement | **`const T&`** |
| L'objet doit être modifié, il existe forcément | **`T&`** |
| L'absence de valeur a un sens métier | **`T*`** (+ test de nullité) |
| L'objet est optionnel *et* la propriété est transférée | à proscrire ici (voir modules 03 et 08) |

Pour un développeur C#, la surprise est que **le défaut est la copie**.
`void f(Mesure m)` copie l'objet entier, y compris ses tableaux membres. En C#,
une classe est toujours passée par référence d'objet ; seuls les `struct` sont
copiés. En C++, `class` et `struct` sont **strictement identiques** sauf pour la
visibilité par défaut (`private` contre `public`) : tous deux sont des types
valeur.

### 1.2 Pourquoi un pointeur coûte cher en DO-178C

Un `T*` dans une signature, c'est :

* un test `if (p == nullptr)` à écrire → une **décision** de plus ;
* donc une **branche** de plus à couvrir (statement, decision, et MC/DC en
  DAL A) ;
* donc un **cas de test** de plus, à tracer vers une exigence de robustesse ;
* et une justification à fournir si vous *ne* testez *pas* la nullité.

Une référence évite tout cela : elle ne peut pas être nulle, la question ne se
pose pas. Choisir `T&` plutôt que `T*` supprime littéralement du travail de
certification. C'est un exemple concret de ce que veut dire « concevoir pour la
vérifiabilité ».

### 1.3 Le *decay* des tableaux

```cpp
const i32 array[5] = {1, 2, 3, 4, 5};
const i32* pointer  = array;   // conversion implicite

sizeof(array);   // 20 — le tableau connaît sa taille
sizeof(pointer);  //  8 — juste une adresse
```

Dès qu'un tableau est passé à une fonction, **la taille est perdue**. Pire :

```cpp
void f(int t[10]);   // le "10" est purement décoratif
int small[3];
f(small);            // compile sans le moindre avertissement
```

C'est la source de la moitié des CVE mémoire du monde C/C++. La contre-mesure
est structurelle : ne jamais séparer le pointeur de sa taille.

```cpp
avio::usize copy_bounded(avio::Span<const avio::u8> source,
                         avio::Span<avio::u8> destination) noexcept;
```

Cette signature **rend le débordement impossible** : la fonction connaît les
deux tailles, l'appelant ne peut pas mentir. Comparez avec
`memcpy(dst, src, n)`, où `n` est une promesse verbale.

### 1.4 Les quatre `const`

Lisez de **droite à gauche** :

```cpp
avio::u8*             p1;  // pointeur modifiable vers octet modifiable
const avio::u8*       p2;  // pointeur modifiable vers octet constant
avio::u8* const       p3;  // pointeur constant vers octet modifiable
const avio::u8* const p4;  // tout est constant
```

Sur les méthodes :

```cpp
avio::usize size() const noexcept;   // ne modifie pas *this
void push(avio::i32 v) noexcept;     // le modifie
```

Une méthode `const` est appelable sur une `const MeasurementLog&`. Une méthode
non-`const` ne l'est pas. Le compilateur transforme ainsi une intention de
conception en **garantie vérifiée** — ce qui, en DO-178C, remplace avantageusement
un point de revue manuel.

> Différence avec C# : `readonly` porte sur le champ, pas sur l'objet désigné.
> Un `readonly List<int>` interdit de remplacer la liste, pas de la modifier.
> C++ distingue les deux, et `const` est **transitif à travers les méthodes**.

### 1.5 Durée de vie : ce que le GC faisait pour vous

Trois fautes que le compilateur ne détecte pas (ou seulement parfois) :

```cpp
// 1. Adresse d'une locale
const int* f() { int x = 42; return &x; }        // x meurt à la sortie

// 2. Vue vers un tampon détruit
avio::Span<int> s;
{ int t[4] = {}; s = avio::make_span(t); }        // t meurt ici
s[0] = 1;                                          // corruption mémoire

// 3. Pointeur vers un élément d'un conteneur qui se réalloue
```

En C#, tant qu'une référence existe, l'objet vit. En C++, **la durée de vie est
une propriété que vous devez démontrer**. C'est un point de revue de code
obligatoire, et l'une des raisons pour lesquelles MISRA C++ encadre si
strictement les pointeurs.

Le module 03 apporte la réponse systématique : **RAII**.

### 1.6 `noexcept`

Vous verrez `noexcept` partout dans ce dépôt. Cela signifie : *cette fonction
ne lance jamais d'exception*. En avionique, les exceptions sont généralement
interdites (module 07), et `noexcept` documente ce contrat tout en permettant
au compilateur de supprimer le code de déroulement de pile.

---

## 2. Ce que dit la DO-178C

| Objectif / notion | Lien avec ce module |
|---|---|
| **A-5.6** — accuracy and consistency | Couvre explicitement l'**arithmétique de pointeurs** et l'usage des tableaux. |
| **A-4.x** — robustesse de l'architecture | Le comportement pour une entrée vide ou un pointeur nul est une **décision de conception**, à documenter dans le SDD, pas à improviser. |
| **Freedom from interference** (§2.4, et surtout CAST-32A pour le multicœur) | Un débordement de tampon dans une fonction DAL D peut corrompre une fonction DAL A partageant la même mémoire. La partition ne tient que si le code respecte ses bornes. |
| **Dead / deactivated code** (§6.4.4.3) | Un `if (p == nullptr)` jamais atteignable est du **code mort** → constat de non-conformité en DAL A/B/C. Il faut soit le tester, soit prouver qu'il est inutile et le supprimer. |

**Point d'attention métier** : un `if` défensif « au cas où » est un piège
classique. En développement classique, c'est une bonne pratique. En DO-178C,
c'est du code non tracé à une exigence, non couvert par un test, donc un
constat en revue. La bonne démarche est : *écrire l'exigence de robustesse,
puis le code, puis le test*.

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ |
|---|---|---|
| Passage d'un objet | référence d'objet (classe) | **copie** par défaut |
| `class` vs `struct` | référence vs valeur | identiques, sauf visibilité par défaut |
| Nullité | `null` possible, `?`/`!` documentent | `T&` ne peut pas être nul, `T*` oui |
| Taille d'un tableau | portée par l'objet (`.Length`) | **perdue** dès le passage en paramètre |
| Immutabilité | `readonly`, `in`, records | `const`, transitif via les méthodes |
| Durée de vie | garantie par le GC | **à démontrer** |
| Débordement de tampon | `IndexOutOfRangeException` | corruption silencieuse |

---

## 4. Manipulation

```bash
.\build\debug\bin\demo_02-pointeurs-references-const.exe
```

```bash
.\build\debug\bin\tests_02-pointeurs-references-const.exe --verbose --req
```

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M02-001 | `swap_values()` échange les valeurs des deux entiers référencés. | `Reference.value_swap` |
| LLR-M02-002 | `increment_if_valid()` incrémente de 1 la valeur pointée et renvoie vrai lorsque le pointeur est non nul. | `Pointer.valid_increment` |
| LLR-M02-003 | `increment_if_valid()` renvoie faux sans effet de bord lorsque le pointeur est nul. | `Pointer.robustness_null_pointer` |
| LLR-M02-010..012 | `find_max()` place dans `out_max` la plus grande valeur du tampon et renvoie vrai. | `FindMax.*` |
| LLR-M02-013 | `find_max()` renvoie faux et laisse `out_max` inchangé si le tampon est vide. | `FindMax.robustness_empty_buffer` |
| LLR-M02-020..022 | `checksum16()` calcule une somme de contrôle 16 bits déterministe, sensible à toute modification d'octet, y compris pour une longueur impaire. | `Checksum.*` |
| LLR-M02-023 | `checksum16()` d'un tampon vide vaut `0xFFFF`. | `Checksum.robustness_empty_buffer` |
| LLR-M02-030..032 | `copy_bounded()` copie `min(taille source, taille destination)` octets et renvoie ce nombre. Elle n'écrit jamais au-delà de la destination. | `Copy.*` |
| LLR-M02-040..041 | `fill()` affecte la valeur à tous les octets ; `equals()` compare taille puis contenu. | `Fill.*`, `Equals.*` |
| LLR-M02-050..053 | `MeasurementLog` mémorise au plus 8 mesures, écrase la plus ancienne au-delà, et signale l'écrasement. | `Log.*` |
| LLR-M02-054 | `MeasurementLog::at()` renvoie faux sans modifier la sortie si l'index est ≥ à la taille courante. | `Log.robustness_index_out_of_domain` |
| LLR-M02-055..056 | `clear()` remet le journal à l'état initial ; `raw_storage()` ne donne qu'un accès en lecture. | `Log.*` |

---

## 6. Exercices

**6.1 — `find_min_max` en une passe**
Écrivez `bool find_min_max(Span<const i32>, i32& out_min, i32& out_max)`.
Contraintes : une seule traversée, aucune modification des sorties si le tampon
est vide. Écrivez l'exigence puis les tests avant le code.

**6.2 — Corriger une API dangereuse**
Voici une signature réelle, telle qu'on en trouve dans du code hérité :
```cpp
void formater_message(char* sortie, const char* modele, int valeur);
```
Listez tout ce qui peut mal se passer, puis proposez une signature sûre.
Combien de tests de robustesse la version d'origine impose-t-elle ? Et la
vôtre ?

**6.3 — Rendre `const` ce qui doit l'être**
Reprenez `MeasurementLog` et vérifiez que chaque méthode qui ne modifie pas
l'objet est `const`. Ajoutez ensuite une méthode `i32 average() const`
(moyenne entière des mesures présentes, 0 si le journal est vide) et son test.
Attention au débordement de l'accumulateur — réutilisez le module 01.

**6.4 — Chasse au *dangling***
Ce code compile et « marche » en Debug. Expliquez pourquoi il est faux, et
proposez deux corrections différentes :
```cpp
avio::Span<const avio::u8> preparer_trame() {
    avio::u8 tampon[4] = {1U, 2U, 3U, 4U};
    return avio::make_const_span(tampon);
}
```

**6.5 — Analyse de couverture anticipée**
Comptez, dans `copy_bounded`, le nombre de **décisions** (points où le flot
peut bifurquer). Combien de cas de test faut-il au minimum pour couvrir toutes
les décisions ? Vérifiez ensuite que les tests existants y parviennent.

---

## 7. Pour aller plus loin

* MISRA C++:2023 — règles sur les pointeurs, l'arithmétique de pointeurs et les
  tableaux.
* C++ Core Guidelines, sections **F** (fonctions) et **ES** (expressions) —
  la référence moderne dont s'inspire clang-tidy.
* CWE-787 (Out-of-bounds Write), CWE-476 (NULL Pointer Dereference).

---

⬅️ [01 — Types et mémoire](../01-types-et-memoire/README.md) |
➡️ [03 — RAII et cycle de vie](../03-raii-cycle-de-vie/README.md)
