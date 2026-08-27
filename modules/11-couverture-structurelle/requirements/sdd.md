# SDD — Software Design Description (extrait)
## Composant : GPWS-M4A — Alerte « TOO LOW GEAR »

> **Niveau** : DAL A (donc **MC/DC obligatoire**, objectif A-7.5)
> **Version** : 1.0

---

## 1. Décision de conception structurante

**DA-01 — Extraction des décisions en fonctions pures de booléens.**

Chaque décision est isolée dans une fonction dont tous les paramètres sont des
booléens déjà évalués :

```cpp
bool mode4a_decision(bool alt_basse, bool vitesse_basse,
                     bool train_rentre, bool en_vol) noexcept;
```

Justification, à conserver telle quelle dans le dossier :

1. la décision devient testable **exhaustivement** (2⁴ = 16 combinaisons) sans
   avoir à fabriquer des valeurs physiques ;
2. les conditions sont évaluées **avant** la décision, donc le **court-circuit**
   de `&&` ne masque plus aucune condition : l'analyse MC/DC devient exacte ;
3. la fonction est sans état, donc réentrante et analysable statiquement.

C'est la conception qu'il faut savoir défendre en revue : *« j'ai extrait la
décision pour la rendre couvrable »*.

---

## 2. Exigences de bas niveau

### LLR-GPWS-010

- **Type** : LLR
- **Parent** : HLR-GPWS-001
- **Énoncé** : `evaluate_conditions()` doit produire :
  `altitude_below_limit = (radio_altitude_ft < 500,0)`,
  `airspeed_below_limit = (airspeed_kt < 190,0)`,
  `gear_not_down = NON gear_down_locked`,
  `airborne = NON on_ground`.
- **Vérification** : `Conditions.*`

### LLR-GPWS-020

- **Type** : LLR
- **Parent** : HLR-GPWS-001
- **Énoncé** : `mode4a_decision()` doit renvoyer vrai si et seulement si les
  quatre conditions sont vraies simultanément.
- **Vérification** : `Mode4a.*`, `Mcdc.mode4a_full_coverage`

### LLR-GPWS-021

- **Type** : LLR
- **Parent** : HLR-GPWS-001
- **Énoncé** : `mode4a_alert()` doit enchaîner `evaluate_conditions()` puis
  `mode4a_decision()` sans autre traitement.
- **Vérification** : `Integration.*`

### LLR-GPWS-030

- **Type** : LLR
- **Parent** : HLR-GPWS-002
- **Énoncé** : `inhibition_decision()` doit renvoyer
  `test_mode OU (approach_config ET glideslope_captured)`.
- **Vérification** : `Mcdc.inhibition_truth_table`, `Mcdc.inhibition_full_coverage`, `Effective.alert_emitted_if_not_inhibited`

### LLR-GPWS-040

- **Type** : LLR
- **Parent** : HLR-GPWS-003
- **Énoncé** : `effective_alert()` doit renvoyer vrai si et seulement si
  `mode4a_alert()` est vrai et `inhibition_decision()` est faux.
- **Vérification** : `Effective.*`

### LLR-GPWS-050

- **Type** : LLR
- **Parent** : *(aucun — exigence DÉRIVÉE)*
- **Énoncé** : L'analyseur `analyze_mcdc()` doit, pour chaque condition d'une
  décision, rechercher une paire d'évaluations différant uniquement par cette
  condition et produisant des issues opposées (MC/DC « unique cause »).
- **Justification** : **Exigence dérivée** liée à l'outillage de vérification,
  pas au produit embarqué. L'outil relève de la DO-330 : voir §1.6 du module 09
  pour la question de sa qualification.
- **Vérification** : `Mcdc.*`

---

## 3. Exigences de haut niveau associées

### HLR-GPWS-001

- **Type** : HLR
- **Parent** : SYS-GPWS-004
- **Énoncé** : Le logiciel doit émettre l'alerte « TOO LOW GEAR » lorsque
  l'aéronef est en vol, à moins de 500 ft sol, à moins de 190 kt, avec le train
  d'atterrissage non sorti et verrouillé.
- **Vérification** : test unitaire + MC/DC

### HLR-GPWS-002

- **Type** : HLR
- **Parent** : SYS-GPWS-007
- **Énoncé** : L'alerte doit être inhibée lorsque le mode test est actif, ou
  lorsque l'aéronef est en configuration d'approche avec le plan de descente
  capturé.
- **Vérification** : test unitaire + MC/DC

### HLR-GPWS-003

- **Type** : HLR
- **Parent** : SYS-GPWS-007
- **Énoncé** : L'alerte effective doit être émise si et seulement si la
  condition d'alerte est satisfaite et l'inhibition n'est pas active.
- **Vérification** : test unitaire
