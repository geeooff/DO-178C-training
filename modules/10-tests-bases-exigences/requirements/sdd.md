# SDD — Software Design Description (extrait)
## Composant : ALERT-MON

> **Niveau** : DAL B
> **Version** : 1.0

---

## 1. Machine à états

```
                 valeur > seuil_montee
        ┌──────────────────────────────────►┐
        │                                    │
  ┌─────┴──────┐                      ┌──────▼──────┐
  │  INACTIVE  │◄─────────────────────┤   PENDING   │
  └─────▲──────┘   valeur <= seuil    └──────┬──────┘
        │          (annulation)              │ N cycles consécutifs
        │                                    ▼
        │                             ┌─────────────┐
        │   M cycles consécutifs      │   ACTIVE    │
        └─────────────────────────────┤             │
                                      └──────┬──────┘
                              ┌──────────────┤ valeur < seuil_retombee
                              │              ▼
                              │       ┌─────────────┐
                              └───────┤  CLEARING   │
                valeur >= seuil       └─────────────┘
                 (annulation)
```

Quatre états, six transitions. **Chaque transition est un cas de test.**

**Décision d'architecture DA-01** : les états transitoires `PENDING` et
`CLEARING` sont **observables** de l'extérieur (`state()`). Ils auraient pu
rester internes ; les exposer permet de tester la machine à états sans
inspecter ses membres privés, et facilite le diagnostic en essais.

**Décision d'architecture DA-02** : la configuration est validée **une fois**, à
la construction. Une instance existante est donc toujours correctement
configurée — l'invariant est établi par construction (module 04).

---

## 2. Exigences de bas niveau

### LLR-ALERT-010

- **Type** : LLR
- **Parent** : HLR-ALERT-004
- **Énoncé** : `AlertMonitor::create(config, out)` doit renvoyer faux si
  `config.clear_threshold >= config.raise_threshold`, si
  `config.confirm_cycles == 0`, si `config.clear_cycles == 0`, ou si l'un des
  deux seuils n'est pas un nombre fini.
- **Vérification** : `Configuration.*`

### LLR-ALERT-011

- **Type** : LLR
- **Parent** : HLR-ALERT-005
- **Énoncé** : Après `create()`, `state()` doit valoir `Inactive`,
  `activation_count()` doit valoir 0 et `confirm_progress()` doit valoir 0.
- **Vérification** : `Initialization.*`

### LLR-ALERT-020

- **Type** : LLR
- **Parent** : HLR-ALERT-001
- **Énoncé** : Dans l'état `Inactive`, un échantillon strictement supérieur au
  seuil de montée doit faire passer l'état à `Pending` avec une progression de
  1, ou directement à `Active` si `confirm_cycles == 1`.
- **Vérification** : `Transitions.inactive_to_pending`, `Equivalence.classifies_above_threshold`, `Limits.exact_raise_threshold_does_not_trigger`

### LLR-ALERT-021

- **Type** : LLR
- **Parent** : HLR-ALERT-001
- **Énoncé** : Dans l'état `Pending`, un échantillon strictement supérieur au
  seuil de montée doit incrémenter la progression ; l'état passe à `Active`
  lorsque la progression atteint `confirm_cycles`.
- **Vérification** : `Transitions.pending_to_pending`, `Transitions.pending_to_active`

### LLR-ALERT-022

- **Type** : LLR
- **Parent** : HLR-ALERT-003
- **Énoncé** : Dans l'état `Pending`, un échantillon inférieur ou égal au seuil
  de montée doit ramener l'état à `Inactive` et la progression à 0.
- **Vérification** : `Transitions.pending_to_inactive_cancellation`

### LLR-ALERT-030

- **Type** : LLR
- **Parent** : HLR-ALERT-002
- **Énoncé** : Dans l'état `Active`, un échantillon strictement inférieur au
  seuil de retombée doit faire passer l'état à `Clearing` avec une progression
  de 1, ou directement à `Inactive` si `clear_cycles == 1`.
- **Vérification** : `Transitions.active_to_clearing`, `Limits.exact_clear_threshold_does_not_clear`

### LLR-ALERT-031

- **Type** : LLR
- **Parent** : HLR-ALERT-002
- **Énoncé** : Dans l'état `Clearing`, un échantillon strictement inférieur au
  seuil de retombée doit incrémenter la progression ; l'état passe à `Inactive`
  lorsque la progression atteint `clear_cycles`.
- **Vérification** : `Transitions.clearing_to_inactive`

### LLR-ALERT-032

- **Type** : LLR
- **Parent** : HLR-ALERT-003
- **Énoncé** : Dans l'état `Clearing`, un échantillon supérieur ou égal au
  seuil de retombée doit ramener l'état à `Active` et la progression à 0.
- **Vérification** : `Transitions.clearing_to_active_cancellation`

### LLR-ALERT-033

- **Type** : LLR
- **Parent** : HLR-ALERT-002
- **Énoncé** : `is_raised()` doit renvoyer vrai dans les états `Active` **et**
  `Clearing`, et faux dans les états `Inactive` et `Pending`.
- **Justification** : L'alerte reste **présentée à l'équipage** tant que la
  retombée n'est pas confirmée. Sans cela, `clear_cycles` ne servirait à rien :
  l'alerte disparaîtrait dès le premier échantillon sous le seuil, et
  l'anti-rebond ne jouerait que dans un sens. C'est exactement le genre de
  détail qu'une exigence explicite empêche d'implémenter de travers.
- **Vérification** : `States.alert_presented_during_decay`

### LLR-ALERT-040

- **Type** : LLR
- **Parent** : HLR-ALERT-006
- **Énoncé** : `activation_count()` doit être incrémenté à chaque transition
  vers l'état `Active` depuis `Inactive` ou `Pending`, et ne doit pas l'être
  lors d'un retour depuis `Clearing`.
- **Justification** : Une oscillation autour du seuil de retombée ne doit pas
  gonfler artificiellement le compteur de maintenance.
- **Vérification** : `Sequences.oscillation_in_dead_band`, `Sequences.full_cycle_then_reactivation`

### LLR-ALERT-050

- **Type** : LLR
- **Parent** : HLR-ALERT-007
- **Énoncé** : `update(v)` avec `v` non fini doit laisser l'état, la
  progression et le compteur d'activations inchangés, et incrémenter
  `rejected_samples()`.
- **Vérification** : `Robustness.*`

### LLR-ALERT-051

- **Type** : LLR
- **Parent** : *(aucun — exigence DÉRIVÉE)*
- **Énoncé** : `rejected_samples()` doit exposer le nombre d'échantillons non
  finis rejetés depuis l'initialisation.
- **Justification** : **Exigence dérivée** de surveillance. Sans compteur, le
  rejet silencieux de LLR-ALERT-050 serait invisible : un capteur qui n'émet
  que des NaN laisserait l'alerte éternellement inactive sans que personne ne
  le sache. Cette exigence transforme un comportement silencieux en un
  comportement observable — c'est une exigence de **sécurité**, à remonter au
  système.
- **Vérification** : `Robustness.rejection_counter`

---

## 3. Matrice HLR → LLR

| HLR | LLR couvrantes |
|-----|----------------|
| HLR-ALERT-001 | LLR-ALERT-020, LLR-ALERT-021 |
| HLR-ALERT-002 | LLR-ALERT-030, LLR-ALERT-031, LLR-ALERT-033 |
| HLR-ALERT-003 | LLR-ALERT-022, LLR-ALERT-032 |
| HLR-ALERT-004 | LLR-ALERT-010 |
| HLR-ALERT-005 | LLR-ALERT-011 |
| HLR-ALERT-006 | LLR-ALERT-040 |
| HLR-ALERT-007 | LLR-ALERT-050 |
