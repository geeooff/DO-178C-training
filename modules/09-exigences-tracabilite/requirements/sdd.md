# SDD — Software Design Description (extrait)
## Composant : ADC-ALT — Calcul d'altitude barométrique

> **Document** : SDD (Design Description, DO-178C §11.10)
> **Composant** : ADC-ALT
> **Niveau** : DAL C
> **Version** : 1.0
> **Statut de configuration** : CC1

Ce document contient l'**architecture logicielle** et les **exigences de bas
niveau** (LLR). Les LLR décrivent **COMMENT** le logiciel satisfait les HLR :
elles sont suffisamment détaillées pour que le code puisse être écrit
directement à partir d'elles, sans autre décision de conception.

---

## 1. Architecture

```
                 pression statique (f32, hPa)
                            │
                            ▼
              ┌──────────────────────────────┐
              │  validate_static_pressure()  │  ← HLR-ADCALT-002/003
              └──────────────┬───────────────┘
                             │ Result<f32>
                             ▼
              ┌──────────────────────────────┐
              │  pressure_altitude_feet()    │  ← HLR-ADCALT-001/004
              └──────────────┬───────────────┘
                             │ Result<f32> (ft)
                             ▼
   QNH (hPa) ──►┌──────────────────────────────┐
                │  corrected_altitude_feet()   │  ← HLR-ADCALT-005/006
                └──────────────┬───────────────┘
                               │ Result<f32> (ft)
                               ▼
                      altitude indiquée
```

**Décision d'architecture DA-01** : le composant est *sans état* (aucune donnée
membre persistante). Conséquence : pas de couplage de données par variable
partagée, réentrance triviale, tests indépendants de l'ordre d'exécution.

**Décision d'architecture DA-02** : la validation des entrées est faite dans une
fonction **dédiée**, appelée par toutes les entrées publiques. Une seule
implémentation du domaine, donc un seul point à modifier et à vérifier.

**Décision d'architecture DA-03** : les erreurs sont remontées par
`mod07::Result<T>` (module 07). Aucune exception, aucun code d'erreur global.

---

## 2. Exigences de bas niveau

Format d'un identifiant : `LLR-ADCALT-nnn`.

### LLR-ADCALT-010

- **Type** : LLR
- **Parent** : HLR-ADCALT-002, HLR-ADCALT-003, HLR-ADCALT-007
- **Énoncé** : `validate_static_pressure(p)` doit renvoyer `Status::OutOfRange`
  si `p < 100,0` ou `p > 1100,0`, `Status::InvalidArgument` si `p` n'est pas un
  nombre fini, et le succès sinon.
- **Vérification** : `Validation.*`

### LLR-ADCALT-020

- **Type** : LLR
- **Parent** : HLR-ADCALT-001
- **Énoncé** : `pressure_altitude_feet(p)` doit calculer
  `h = 145366,45 × (1 − (p / 1013,25)^0,190284)`, où `h` est exprimée en pieds
  et `p` en hectopascals.
- **Justification** : Forme fermée du modèle ISA pour la troposphère, avec les
  constantes de l'OACI Doc 7488 : `T₀ = 288,15 K`, `L = 0,0065 K/m`,
  `P₀ = 1013,25 hPa`, exposant `= L·R/(g·M) = 0,190284`.
- **Vérification** : `Altitude.*`

### LLR-ADCALT-021

- **Type** : LLR
- **Parent** : HLR-ADCALT-002, HLR-ADCALT-003
- **Énoncé** : `pressure_altitude_feet(p)` doit appeler
  `validate_static_pressure(p)` et propager son statut sans le modifier avant
  tout calcul.
- **Vérification** : `Altitude.robustesse_*`

### LLR-ADCALT-022

- **Type** : LLR
- **Parent** : HLR-ADCALT-004
- **Énoncé** : Pour les pressions de référence 1013,25 / 1000 / 950 / 850 /
  700 / 500 / 300 / 200 hPa, `pressure_altitude_feet` doit produire
  respectivement 0 / 363,6 / 1772,0 / 4779,2 / 9878,4 / 18281,2 / 30052,7 /
  38615,1 ft, à ±20 ft près.
- **Vérification** : `Altitude.table_de_reference`

### LLR-ADCALT-023

- **Type** : LLR
- **Parent** : HLR-ADCALT-001
- **Énoncé** : `pressure_altitude_feet` doit être strictement décroissante sur
  le domaine de validité : une pression plus élevée produit une altitude plus
  basse.
- **Vérification** : `Altitude.monotonie`

### LLR-ADCALT-030

- **Type** : LLR
- **Parent** : HLR-ADCALT-005, HLR-ADCALT-006, HLR-ADCALT-007
- **Énoncé** : `validate_qnh(q)` doit renvoyer `Status::OutOfRange` si
  `q < 948,0` ou `q > 1084,0`, `Status::InvalidArgument` si `q` n'est pas un
  nombre fini, et le succès sinon.
- **Vérification** : `Validation.*`

### LLR-ADCALT-031

- **Type** : LLR
- **Parent** : HLR-ADCALT-005
- **Énoncé** : `corrected_altitude_feet(p, q)` doit calculer
  `h_ind = pressure_altitude_feet(p) + (q − 1013,25) × 27,0`, la constante
  27,0 exprimant le nombre de pieds par hectopascal au voisinage du niveau
  de la mer.
- **Vérification** : `Correction.*`

### LLR-ADCALT-032

- **Type** : LLR
- **Parent** : HLR-ADCALT-003, HLR-ADCALT-006
- **Énoncé** : `corrected_altitude_feet(p, q)` doit valider la pression avant
  le calage, et propager le premier statut en erreur rencontré.
- **Justification** : L'ordre de validation est **spécifié** : deux entrées
  invalides simultanément doivent produire un résultat déterministe et
  reproductible.
- **Vérification** : `Correction.priorite_des_erreurs`

### LLR-ADCALT-033

- **Type** : LLR
- **Parent** : *(aucun — exigence DÉRIVÉE)*
- **Énoncé** : `feet_per_hpa()` doit exposer la constante 27,0 ft/hPa utilisée
  par LLR-ADCALT-031.
- **Justification** : **Exigence dérivée**. Exposer la constante permet aux
  tests de vérifier la valeur sans la dupliquer, et à la maintenance de la
  retrouver. Aucun impact sur la sécurité : la fonction est en lecture seule.
- **Vérification** : `Correction.constante_exposee`

---

## 3. Matrice de traçabilité HLR → LLR

| HLR | LLR couvrantes |
|-----|----------------|
| HLR-ADCALT-001 | LLR-ADCALT-020, LLR-ADCALT-023 |
| HLR-ADCALT-002 | LLR-ADCALT-010, LLR-ADCALT-021 |
| HLR-ADCALT-003 | LLR-ADCALT-010, LLR-ADCALT-021, LLR-ADCALT-032 |
| HLR-ADCALT-004 | LLR-ADCALT-022 |
| HLR-ADCALT-005 | LLR-ADCALT-030, LLR-ADCALT-031 |
| HLR-ADCALT-006 | LLR-ADCALT-030, LLR-ADCALT-032 |
| HLR-ADCALT-007 | LLR-ADCALT-010, LLR-ADCALT-030 |

Cette matrice est **maintenue à la main** dans ce document, mais
`tools/trace_check.py` la **reconstruit** à partir des champs `Parent` et
compare : toute divergence est un défaut de configuration.
