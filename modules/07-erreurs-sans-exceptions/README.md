# Module 07 — Gestion d'erreurs sans exceptions

> **Durée estimée** : 1 journée
> **Prérequis** : modules 00 à 06

---

## Objectifs pédagogiques

1. Expliquer **techniquement** pourquoi les exceptions sont interdites en
   avionique.
2. Mettre en œuvre le motif `Result<T>` et la propagation explicite d'erreurs.
3. Concevoir un **catalogue d'erreurs** traçable à des exigences de robustesse.
4. Distinguer **code mort**, **code désactivé** et **code défensif**.
5. Décoder un message de bus réel (ARINC 429) avec une gestion d'erreur
   complète.

---

## 1. Le cours

### 1.1 Cinq raisons techniques, pas du conservatisme

| # | Raison | Conséquence DO-178C |
|---|---|---|
| 1 | **Temps d'exécution non borné.** Le déroulement de pile parcourt des tables générées par le compilateur ; le coût dépend de la profondeur d'appel et du nombre d'objets à détruire. | Le WCET doit être **démontré** (module 15). Aucun outil du marché ne borne utilement l'*unwinding*. |
| 2 | **Flot de contrôle implicite.** `f(); g();` — si `f` peut lancer, `g` peut ne jamais s'exécuter, sans que rien ne l'indique. | Objectif **A-4.8** : architecture de flot de contrôle vérifiable. L'analyse de couplage de contrôle (module 12) devient très difficile. |
| 3 | **Allocation dynamique.** Sur la plupart des ABI, l'objet exception est alloué sur un tas dédié. | Interdit après l'initialisation (module 08), **DO-332 OO.6.8.2**. |
| 4 | **Taille du code.** Les tables de déroulement pèsent typiquement 10 à 30 % du binaire. | Budget Flash. |
| 5 | **DO-332, vulnérabilité 6.** Le supplément OO ajoute des objectifs spécifiques à la gestion des exceptions. | Les éviter, c'est éviter ces objectifs. |

En pratique, les projets compilent avec :

```
MSVC       : /EHs-c-   (+ /D_HAS_EXCEPTIONS=0 pour la STL)
GCC/Clang  : -fno-exceptions
```

Ce dépôt les laisse actives (la formation tourne sur PC), mais **tout le code
est écrit comme si elles étaient interdites**.

> ⚠️ `noexcept` n'**empêche** pas de lancer : il **promet** de ne pas le faire.
> Si une exception s'échappe malgré tout d'une fonction `noexcept`,
> `std::terminate()` est appelé — en vol, cela signifie un redémarrage du
> calculateur.

### 1.2 Le catalogue d'erreurs

```cpp
enum class Status : avio::u8 {
    Ok, InvalidArgument, OutOfRange, ChecksumError, NotReady, HardwareFault, Timeout
};
```

Un `enum class` unique pour tout le composant. Trois bénéfices :

* la liste des erreurs possibles est **exhaustive et revisable** ;
* chaque valeur se trace à une **exigence de robustesse** ;
* un `switch` **sans `default`** force le compilateur à signaler tout oubli
  quand une nouvelle erreur est ajoutée (C4061/C4062 sous MSVC, `-Wswitch`
  ailleurs).

Notez la distinction faite par `is_fault()` : `NotReady` **n'est pas une
panne**, c'est un état transitoire normal au démarrage. Confondre les deux
déclenche des alarmes intempestives au sol. C'est une décision de conception, à
écrire dans le SDD — pas une subtilité d'implémentation.

### 1.3 `Result<T>`

Équivalent de `std::expected<T, E>` (C++23) ou du `Result<T, E>` de Rust,
réécrit pour C++17.

```cpp
Result<i32> resultat = mod07::extract_altitude_feet(mot);
if (resultat.is_ok()) { utiliser(resultat.value()); }
else                  { compteurs.record(resultat.status()); }
```

Trois choix de conception à savoir défendre :

1. **Le défaut est une *erreur*** (`Status::NotReady`). Un `Result` oublié ne
   peut jamais passer pour un succès. C'est la différence entre un défaut sûr
   et un défaut discret.
2. **Pas d'union, pas de placement `new`.** La valeur et le statut coexistent.
   Quelques octets de plus, mais une disposition mémoire triviale, donc
   entièrement analysable, et aucun comportement indéfini possible en cas de
   mauvais usage. En avionique, **la simplicité d'analyse prime sur l'économie
   d'octets**.
3. **`value()` sur une erreur notifie le gestionnaire d'anomalie** et renvoie
   une valeur neutre. Jamais de comportement indéfini.

### 1.4 Propagation explicite

```cpp
Result<avio::i32> extract_altitude_feet(avio::u32 raw_word) noexcept {
    const Result<Arinc429Word> decoded = decode(raw_word);
    if (decoded.is_error()) {
        return Result<avio::i32>::error(decoded.status());   // remontée
    }
    …
}
```

C'est plus verbeux qu'un `try`/`catch`. C'est aussi **entièrement visible** :
tous les chemins de sortie sont dans le code, ce qui rend l'analyse de couplage
de contrôle directe et la couverture structurelle mesurable.

Certaines équipes ajoutent une macro de style `TRY` pour réduire la verbosité :

```cpp
#define RESULT_TRY(var, expr)                        \
    auto var##_r = (expr);                           \
    if (var##_r.is_error()) { return …; }            \
    const auto& var = var##_r.value()
```

MISRA restreint fortement les macros fonctionnelles ; à discuter en revue,
au cas par cas, avec justification.

### 1.5 ARINC 429

Le bus de données de l'avionique civile depuis 1977 : liaison point à point
unidirectionnelle, 12,5 ou 100 kbit/s, mots de 32 bits. Présent sur A320,
A350, B737, B787…

```
 bit  32 | 31 30 | 29 ................ 11 | 10  9 | 8 .......... 1
 --------+-------+------------------------+-------+----------------
 parité  |  SSM  |      DONNÉE (19 bits)  |  SDI  |  LABEL (8 bits)
```

* **LABEL** — nature de la donnée, noté traditionnellement en **octal**
  (203 = altitude barométrique).
* **SDI** — distingue plusieurs équipements identiques (gauche/droite).
* **DONNÉE** — 19 bits, format BNR (binaire signé) ou BCD.
* **SSM** (Sign/Status Matrix) — état de la donnée :
  `FailureWarning`, `NoComputedData`, `FunctionalTest`, `NormalOperation`.
* **PARITÉ** — bit 32, parité **impaire** sur le mot complet.

Deux points de sécurité à retenir :

* **`FunctionalTest` n'est pas `NormalOperation`.** Une donnée produite pendant
  un test de l'équipement source est techniquement valide mais **ne doit pas
  être utilisée en vol**. Les confondre est un défaut de sécurité classique.
* **La parité correcte ne garantit pas la vraisemblance.** Une altitude de
  200 000 pieds passe le contrôle d'intégrité. La validation de **domaine**
  reste indispensable.

> 💡 **Note MISRA** : les littéraux **octaux** sont interdits (`0203` vaut 131,
> pas 203). Or les labels ARINC sont *toujours* notés en octal dans les
> documents d'interface. On écrit donc la valeur en décimal avec l'octal en
> commentaire. C'est exactement le genre de détail où une règle de codage évite
> un défaut réel.

### 1.6 Code mort, code désactivé, code défensif

| Notion | Définition | Statut en DO-178C |
|---|---|---|
| **Code mort** (*dead code*) | Code qui ne peut **jamais** s'exécuter, quelle que soit l'entrée. | **Défaut.** À supprimer, et à analyser : il révèle souvent une exigence oubliée ou un bug (§6.4.4.3). |
| **Code désactivé** (*deactivated code*) | Code intentionnellement non exécutable dans cette configuration : option non retenue, variante d'un autre programme, mode maintenance. | **Acceptable**, à condition de l'identifier dans la conception, de démontrer qu'il ne peut pas être activé par erreur, et de justifier son absence de couverture. |
| **Code défensif** | `if (p == nullptr)` alors que `p` ne peut pas être nul. | **Piège.** Excellente pratique ailleurs, défaut ici : branche non tracée, jamais couverte. |

**La bonne démarche** : écrire **l'exigence de robustesse** d'abord, puis le
code, puis le test qui l'atteint. C'est pourquoi chaque cas d'erreur de ce
module porte un identifiant `LLR-M07-0xx` **et** un test qui l'exerce.

---

## 2. Ce que dit la DO-178C

| Objectif | Application |
|---|---|
| **A-4.8** — architecture vérifiable | Le flot de contrôle est explicite : aucun chemin caché. |
| **A-6.3 / A-6.4** — robustesse de l'exécutable | Chaque cas d'erreur du catalogue est exercé par un test. |
| **§6.4.4.3** — analyse du code non couvert | Distinction code mort / code désactivé. |
| **DO-332 vulnérabilité 6** | Évitée par construction. |
| **§6.3.4.f** — algorithmes exacts | La validation de domaine complète le contrôle d'intégrité. |

---

## 3. C# → C++ : ce qui change

| Sujet | C# | C++ avionique |
|---|---|---|
| Signalement d'erreur | exception | valeur de retour `Result<T>` / `Status` |
| Propagation | automatique | **explicite**, à chaque niveau |
| Oubli de traitement | remonte jusqu'au sommet | **compile quand même** — d'où `[[nodiscard]]` et la revue |
| Nettoyage | `finally` / `using` | RAII (module 03) |
| Coût | acceptable | non borné, donc interdit |
| Erreur non gérée | `UnhandledException` | `std::terminate` si `noexcept` |

> 💡 Marquer `Result<T>` avec `[[nodiscard]]` (C++17) force le compilateur à
> signaler tout résultat ignoré. C'est l'exercice 6.1.

---

## 4. Manipulation

```bash
.\build\debug\bin\demo_07-erreurs-sans-exceptions.exe
```

```bash
.\build\debug\bin\tests_07-erreurs-sans-exceptions.exe --verbose --req
```

---

## 5. Exigences du module

| Id | Exigence | Vérifiée par |
|----|----------|--------------|
| LLR-M07-001 | `status_name()` renvoie un libellé unique par valeur de `Status`. | `Status.label_for_each_value` |
| LLR-M07-002 | `status_name()` renvoie `"StatutInconnu"` pour toute valeur hors énumération, et ne renvoie jamais `nullptr`. | `Status.robustness_value_outside_enumeration` |
| LLR-M07-003 | `is_fault()` classe comme panne : `ChecksumError`, `HardwareFault`, `Timeout`, `OutOfRange` — et **pas** `NotReady`. | `Status.fault_classification` |
| LLR-M07-010 | Un `Result<T>` construit par défaut est en erreur, statut `NotReady`. | `Result.default_is_an_error` |
| LLR-M07-011..012 | `ok()` / `error()` produisent l'état attendu ; `value_or()` renvoie le repli en cas d'erreur. | `Result.success`, `Result.error` |
| LLR-M07-013 | `Result::error(Status::Ok)` est neutralisé en `InvalidArgument`. | `Result.error_ok_is_neutralized` |
| LLR-M07-014 | `value()` appelé sur un résultat en erreur notifie le gestionnaire d'anomalie et renvoie une valeur neutre. | `Result.access_value_on_error_is_reported` |
| LLR-M07-020..022 | `StatusCounters` compte par statut, totalise les pannes, désigne la dominante, et ignore les statuts hors énumération. | `Counters.*` |
| LLR-M07-030 | `has_odd_parity()` renvoie vrai si et seulement si le nombre de bits à 1 est impair. | `Arinc.odd_parity` |
| LLR-M07-031..032 | `encode()` puis `decode()` restituent label, SDI, charge utile et SSM ; la charge utile est interprétée en complément à deux sur 19 bits. | `Arinc.encoding_then_decoding`, `Arinc.negative_value_twos_complement` |
| LLR-M07-033 | Une parité paire produit `ChecksumError`. | `Arinc.robustness_altered_parity` |
| LLR-M07-034 | Un label hors de la liste acceptée produit `InvalidArgument`. | `Arinc.robustness_unhandled_label` |
| LLR-M07-035 | `SSM = FailureWarning` produit `HardwareFault`. | `Arinc.ssm_source_fault` |
| LLR-M07-036 | `SSM = NoComputedData` ou `FunctionalTest` produit `NotReady`. | `Arinc.ssm_data_unavailable` |
| LLR-M07-040..042 | `extract_altitude_feet()` renvoie l'altitude en pieds, et propage intact le statut de l'étape en erreur. | `Chaining.*` |
| LLR-M07-043..044 | Toute altitude hors de [−2000 ; +60000] ft produit `OutOfRange`, bornes incluses acceptées. | `Chaining.value_out_of_flight_domain`, `Chaining.flight_domain_bounds` |

---

## 6. Exercices

**6.1 — `[[nodiscard]]`**
Ajoutez `[[nodiscard]]` à `Result<T>`, puis écrivez volontairement un appel qui
ignore le résultat. Que dit le compilateur ? Pourquoi est-ce important en
DO-178C ? (Indice : un statut d'erreur ignoré est une exigence de robustesse
non satisfaite, invisible à la relecture.)

**6.2 — Décoder la vitesse air**
Ajoutez `Result<f32> extract_airspeed_knots(u32)` pour le label 206 (octal),
résolution 1/8 nœud par bit, domaine [0 ; 700] nœuds. Exigences d'abord, puis
tests (nominal, bornes, robustesse), puis code.

**6.3 — Le compteur qui déborde**
`StatusCounters` utilise des `u32`. À 100 Hz, en combien de temps un compteur
déborde-t-il si chaque cycle produit une erreur ? Est-ce un problème ? Quelle
stratégie retiendriez-vous (saturation, remise à zéro périodique, `u64`) et
comment l'écririez-vous en exigence ?

**6.4 — Chasse au code mort**
Ajoutez volontairement une branche inatteignable dans `decode()` (par exemple
`if (label > 255U)` — impossible, `label` est un `u8`). Compilez, exécutez les
tests, puis expliquez : quel objectif DO-178C cette branche met-elle en échec ?
Comment un rapport de couverture la ferait-il apparaître ?

**6.5 — Le SSM oublié**
Ajoutez une valeur à `SignStatus` sans traiter le nouveau cas dans le `switch`
de `decode()`. Que dit le compilateur avec `/W4` ? Et si vous ajoutez un
`default:` ? Concluez sur l'intérêt des `switch` exhaustifs sans `default`.

---

## 7. Pour aller plus loin

* ARINC Specification 429 Part 1 — Digital Information Transfer System.
* `std::expected` (C++23) : la version normalisée de `Result`.
* JSF++ (Joint Strike Fighter Air Vehicle C++ Coding Standards), règles sur les
  exceptions — un des rares standards publics et gratuits.
* CAST-6 — *Dead Code vs. Deactivated Code* (position officielle des autorités,
  document public).

---

⬅️ [06 — Templates](../06-templates-constexpr/README.md) |
➡️ [08 — Mémoire statique](../08-memoire-statique/README.md)
