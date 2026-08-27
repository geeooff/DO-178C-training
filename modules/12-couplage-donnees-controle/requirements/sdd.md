# SDD — Software Design Description (extrait)
## Sous-système : CHAIN — Chaîne acquisition → filtrage → supervision

> **Niveau** : DAL B
> **Version** : 1.0

---

## 1. Architecture et couplages

```
   ┌───────────────┐   read()      ┌───────────────┐
   │  SUPERVISOR   │ ────────────► │  ACQUISITION  │
   │               │ ◄──────────── │               │
   │               │  Result<f32>  └───────────────┘
   │               │
   │               │   push(f32)   ┌───────────────┐
   │               │ ────────────► │    FILTER     │
   │               │   average()   │               │
   │               │ ────────────► │               │
   │               │ ◄──────────── │               │
   │               │   reset()     │               │
   │               │ ┄┄┄┄┄┄┄┄┄┄┄►  │               │
   └───────────────┘  (conditionnel)└──────────────┘
```

### 1.1 Matrice de couplage de CONTRÔLE

Qui appelle qui, et sous quelle condition. Cette matrice est le **résultat de
l'analyse**, à confronter aux tests d'intégration.

| # | Appelant | Appelé | Condition d'appel | Exercé par |
|---|---|---|---|---|
| I1 | Supervisor | `Acquisition::read()` | **inconditionnel**, à chaque cycle | tous les cycles |
| I2 | Supervisor | `Filter::push()` | lecture réussie | `Coupling.cycle_nominal` |
| I3 | Supervisor | `Filter::average()` | lecture réussie | `Coupling.cycle_nominal` |
| I4 | Supervisor | `Filter::reset()` | **3 rejets consécutifs** | `Coupling.purge_after_rejections` |

> **I4 est le cas critique.** C'est un couplage **conditionnel**, déclenché par
> une séquence. Aucun test unitaire de `Filter` ni de `Acquisition` ne peut
> l'exercer : il n'existe qu'à l'assemblage. C'est exactement la classe de
> couplage que l'objectif A-7.8 vise.

### 1.2 Matrice de couplage de DONNÉES

Quelle donnée transite, avec quelle unité, quel domaine, quelle validité.

| # | Producteur | Consommateur | Donnée | Unité | Domaine | Validité |
|---|---|---|---|---|---|---|
| D1 | Acquisition | Supervisor | mesure convertie | unité capteur | [0 ; 1023,75] | `Status::Ok` seulement |
| D2 | Supervisor | Filter | échantillon | unité capteur | idem D1 | non finie rejetée par `push()` |
| D3 | Filter | Supervisor | moyenne | unité capteur | [0 ; 1023,75] | `NotReady` si fenêtre incomplète |
| D4 | Supervisor | appelant | `CycleOutcome` | — | — | `status` porte la validité |

**La colonne « unité » n'est pas décorative.** C'est à la frontière D1 que
l'échelle est fixée (`kScaleUnitsPerCount = 0,25`). Une erreur d'échelle à cet
endroit se propage silencieusement dans toute la chaîne aval — scénario Mars
Climate Orbiter (module 04).

### 1.3 Décision d'architecture DA-01 : dépendances par paramètre de template

```cpp
template <typename AcquisitionT, typename FilterT>
class Supervisor { … };

using FlightSupervisor = Supervisor<Acquisition, Filter>;
```

Justification, à conserver au dossier :

1. le couplage est **explicite dans le type** : il ne peut pas être introduit
   en catimini par une variable globale ;
2. les tests peuvent substituer des composants **instrumentés** sans modifier
   le code de production — le binaire vérifié reste **rigoureusement** le
   binaire embarqué, ce qui n'est pas le cas d'une instrumentation par
   `#ifdef` ;
3. aucune vtable, aucun coût à l'exécution (module 06).

### 1.4 Décision d'architecture DA-02 : aucune variable globale mutable

Aucun composant ne communique par variable globale. Toute donnée transite par
paramètre ou valeur de retour. Conséquence : la matrice §1.2 est **complète et
vérifiable par lecture des signatures**.

---

## 2. Exigences de bas niveau

### LLR-CHAIN-010

- **Type** : LLR
- **Parent** : HLR-CHAIN-001
- **Énoncé** : `Acquisition::read()` doit renvoyer `raw × 0,25` en cas de
  succès, et `Status::OutOfRange` si `raw` sort de [0 ; 4095]. Elle incrémente
  `read_count()` à chaque appel et `reject_count()` à chaque rejet.
- **Vérification** : `Acquisition.*`

### LLR-CHAIN-020

- **Type** : LLR
- **Parent** : HLR-CHAIN-002
- **Énoncé** : `Filter::push()` doit ajouter l'échantillon à une fenêtre
  glissante de 4 éléments et renvoyer faux sans rien modifier si l'échantillon
  n'est pas fini.
- **Vérification** : `Filter.*`

### LLR-CHAIN-021

- **Type** : LLR
- **Parent** : HLR-CHAIN-002
- **Énoncé** : `Filter::average()` doit renvoyer `Status::NotReady` tant que la
  fenêtre contient moins de 4 échantillons, et la moyenne arithmétique sinon.
- **Justification** : Produire une moyenne sur une fenêtre incomplète serait
  produire une valeur **biaisée mais plausible** — plus dangereux que pas de
  valeur du tout.
- **Vérification** : `Filter.*`

### LLR-CHAIN-022

- **Type** : LLR
- **Parent** : HLR-CHAIN-002
- **Énoncé** : `Filter::reset()` doit vider la fenêtre et remettre le compteur
  d'échantillons à zéro.
- **Vérification** : `Filter.reset_to_zero`

### LLR-CHAIN-030

- **Type** : LLR
- **Parent** : HLR-CHAIN-003
- **Énoncé** : `Supervisor::cycle()` doit appeler `Acquisition::read()` puis,
  en cas de succès seulement, `Filter::push()` et `Filter::average()`.
- **Vérification** : `Coupling.cycle_nominal`, `Coupling.read_in_error`

### LLR-CHAIN-031

- **Type** : LLR
- **Parent** : HLR-CHAIN-004
- **Énoncé** : Après 3 lectures en erreur **consécutives**,
  `Supervisor::cycle()` doit appeler `Filter::reset()` et remettre son compteur
  de rejets consécutifs à zéro.
- **Justification** : Une fenêtre contenant des données périmées produirait une
  moyenne trompeuse au retour du capteur. **Défaut d'intégration classique.**
- **Vérification** : `Coupling.purge_after_rejections`,
  `Coupling.rejection_counter_reset`

### LLR-CHAIN-032

- **Type** : LLR
- **Parent** : HLR-CHAIN-003
- **Énoncé** : `Supervisor::cycle()` doit positionner `alert` à vrai si et
  seulement si la moyenne filtrée dépasse strictement 800,0.
- **Vérification** : `Supervision.*`

### LLR-CHAIN-040

- **Type** : LLR
- **Parent** : *(aucun — exigence DÉRIVÉE)*
- **Énoncé** : L'outillage de vérification doit enregistrer chaque appel
  inter-composants et chaque donnée échangée, et signaler toute interface
  déclarée jamais exercée.
- **Justification** : **Exigence dérivée** portant sur l'outillage, non sur le
  produit embarqué. Elle matérialise la démonstration attendue par
  l'objectif A-7.8.
- **Vérification** : `Coupling.all_interfaces_exercised`

---

## 3. Exigences de haut niveau

### HLR-CHAIN-001

- **Type** : HLR
- **Parent** : SYS-CHAIN-001
- **Énoncé** : Le sous-système doit convertir la mesure brute du capteur en
  grandeur physique, et rejeter toute mesure hors du domaine du convertisseur.
- **Vérification** : test unitaire

### HLR-CHAIN-002

- **Type** : HLR
- **Parent** : SYS-CHAIN-002
- **Énoncé** : Le sous-système doit filtrer la mesure par une moyenne glissante
  sur 4 échantillons, et ne produire aucune valeur tant que la fenêtre est
  incomplète.
- **Vérification** : test unitaire

### HLR-CHAIN-003

- **Type** : HLR
- **Parent** : SYS-CHAIN-003
- **Énoncé** : Le sous-système doit signaler une alerte lorsque la valeur
  filtrée dépasse 800,0 unités.
- **Vérification** : test d'intégration

### HLR-CHAIN-004

- **Type** : HLR
- **Parent** : SYS-CHAIN-004
- **Énoncé** : Le sous-système ne doit pas produire de valeur filtrée à partir
  de données antérieures à une interruption prolongée de l'acquisition.
- **Justification** : Après une panne capteur, la fenêtre contient des données
  périmées ; les moyenner avec les nouvelles produirait une valeur fausse
  pendant plusieurs cycles.
- **Vérification** : test d'intégration
