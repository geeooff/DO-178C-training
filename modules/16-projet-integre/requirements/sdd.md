# SDD — Software Design Description
## FQMS — Fuel Quantity Management System

> **Document** : Design Description (DO-178C §11.10)
> **Catégorie de contrôle** : CC1 (DAL B)
> **Version** : 1.0

---

## 1. Architecture

```
        raw[3] (points convertisseur)
              │
              ▼
   ┌──────────────────────────────────────────────┐
   │              FuelSystem::update()            │
   │                                              │
   │  1 ── TankGauge::read()  x3 ────────────────►│  mod16
   │  2 ── total = somme des valides              │
   │  3 ── ecart = |gauche − droite| si mesurable │
   │  4 ── low_fuel_monitor_.update(deficit)  ───►│  mod10
   │  5 ── imbalance_monitor_.update(ecart)   ───►│  mod10
   └──────────────────┬───────────────────────────┘
                      ▼
                 CycleReport
```

### 1.1 Dépendances (réutilisation inter-modules)

| Composant réutilisé | Module | Usage |
|---|---|---|
| `mod04::Mass` | 04 | type fort de masse, arithmétique saturante, invariant ≥ 0 |
| `mod07::Result<T>`, `Status` | 07 | remontée d'erreur sans exception |
| `mod10::AlertMonitor` | 10 | hystérésis + anti-rebond des deux alertes |

> **Décision d'architecture DA-01** — La temporisation et l'hystérésis des
> alertes sont **déléguées** à `mod10::AlertMonitor`, composant déjà vérifié.
> Bénéfice : un seul mécanisme d'alerte à vérifier pour tout le programme, et
> les campagnes de test de `AlertMonitor` restent valides.
> Contrepartie : ce composant devient une dépendance CC1, et toute évolution
> de son comportement impacte le FQMS (analyse de changement, §7.2.4).

### 1.2 Adaptation des grandeurs aux moniteurs

`AlertMonitor` lève une alerte quand la grandeur surveillée **dépasse** un
seuil. Les deux alertes du FQMS sont donc exprimées comme des **grandeurs
dérivées** :

| Alerte | Grandeur transmise | Seuil de montée | Seuil de retombée |
|---|---|---|---|
| Bas niveau | `déficit = 1500 − total` (kg) | `0` | `−200` |
| Déséquilibre | `\|gauche − droite\|` (kg) | `500` | `400` |

**Décision d'architecture DA-02** — Lorsqu'une grandeur n'est pas mesurable
(jauge en panne), l'échantillon transmis au moniteur est **NaN**.
`mod10::AlertMonitor` ignore les échantillons non finis sans modifier son état
(LLR-ALERT-050) : l'alerte est donc **gelée**, ce qui est exactement le
comportement exigé par HLR-FQMS-022 et HLR-FQMS-032.

> C'est un cas où une **exigence dérivée d'un composant** (le rejet silencieux
> des NaN, module 10) devient une **brique fonctionnelle** d'un autre. Ce lien
> doit être documenté ici, faute de quoi une évolution de `AlertMonitor`
> casserait le FQMS sans que personne ne le voie.

### 1.3 Matrice de couplage de CONTRÔLE (module 12)

| # | Appelant | Appelé | Condition | Exercé par |
|---|---|---|---|---|
| I1 | `FuelSystem::update` | `TankGauge::read()` ×3 | inconditionnel | tous les tests de cycle |
| I2 | `FuelSystem::update` | `system_status()` | inconditionnel | `Totalisation.*` |
| I3 | `FuelSystem::update` | `imbalance_is_measurable()` | inconditionnel | `Desequilibre.*` |
| I4 | `FuelSystem::update` | `low_fuel_is_measurable()` | inconditionnel | `BasNiveau.*` |
| I5 | `FuelSystem::update` | `AlertMonitor::update()` ×2 | inconditionnel | tous |
| I6 | `FuelSystem::create` | `TankGauge::create()` ×3 | inconditionnel | `Configuration.*` |
| I7 | `FuelSystem::create` | `AlertMonitor::create()` ×2 | après validation des seuils | `Configuration.*` |

> Aucun appel conditionnel dans `update()` : **c'est délibéré**. Le chemin
> d'exécution est identique à chaque cycle, ce qui rend le WCET constant et
> satisfait HLR-FQMS-041.

### 1.4 Matrice de couplage de DONNÉES

| # | Producteur | Consommateur | Donnée | Unité | Domaine |
|---|---|---|---|---|---|
| D1 | matériel | `TankGauge` | mesure brute | point convertisseur | [0 ; 4095] |
| D2 | `TankGauge` | `FuelSystem` | quantité réservoir | **gramme** | [0 ; capacité] |
| D3 | `FuelSystem` | `AlertMonitor` (bas niveau) | déficit | **kilogramme** | [−16500 ; +1500] ou NaN |
| D4 | `FuelSystem` | `AlertMonitor` (déséquilibre) | écart d'aile | **kilogramme** | [0 ; 5000] ou NaN |
| D5 | `FuelSystem` | appelant | `CycleReport` | — | — |

> ⚠️ **Changement d'unité entre D2 et D3/D4** : gramme → kilogramme. C'est le
> point le plus dangereux de toute la chaîne (module 04). Il est isolé dans
> `Mass::kilograms()`, une seule fonction, testée. Le test
> `Integration.coherence_des_unites` le vérifie de bout en bout.

### 1.5 Décisions d'architecture complémentaires

| Id | Décision | Justification |
|---|---|---|
| DA-03 | Aucune allocation dynamique ; tout est membre de `FuelSystem` | module 08 : occupation RAM constante et calculable |
| DA-04 | Aucune fonction virtuelle | module 05/15 : WCET exact, pas d'objectif DO-332 sur l'héritage |
| DA-05 | Décisions extraites en fonctions pures de booléens | module 11 : couverture praticable et lisible |
| DA-06 | Arithmétique **entière** pour les masses | module 15 : résultat identique sur toute cible, égalité exacte utile |
| DA-07 | Composant **sans état persistant** hors moniteurs et compteurs | tests indépendants de l'ordre d'exécution |

---

## 2. Exigences de bas niveau

### LLR-FQMS-001

- **Type** : LLR
- **Parent** : HLR-FQMS-001, HLR-FQMS-020, HLR-FQMS-030
- **Énoncé** : `default_config()` doit fournir : capacités 5000 / 8000 /
  5000 kg, seuil bas niveau 1500 kg, hystérésis bas niveau 200 kg, seuil de
  déséquilibre 500 kg, hystérésis de déséquilibre 100 kg, 5 cycles de
  confirmation et 5 cycles de retombée.
- **Vérification** : `Configuration.valeurs_de_reference`

### LLR-FQMS-010

- **Type** : LLR
- **Parent** : HLR-FQMS-001
- **Énoncé** : `TankGauge::create()` doit refuser une capacité nulle et
  initialiser la mesure brute à 0.
- **Vérification** : `Jauge.creation`

### LLR-FQMS-011

- **Type** : LLR
- **Parent** : HLR-FQMS-001
- **Énoncé** : `TankGauge::read()` doit renvoyer
  `capacité × (raw − 0) / (4095 − 0)`, calculée en arithmétique entière
  64 bits.
- **Vérification** : `Jauge.conversion_lineaire`

### LLR-FQMS-012

- **Type** : LLR
- **Parent** : HLR-FQMS-002
- **Énoncé** : `TankGauge::read()` doit renvoyer `Status::OutOfRange` pour
  toute mesure brute strictement inférieure à 0 ou strictement supérieure à
  4095.
- **Vérification** : `Jauge.robustesse_hors_domaine`

### LLR-FQMS-020

- **Type** : LLR
- **Parent** : HLR-FQMS-042
- **Énoncé** : `FuelSystem::create()` doit refuser toute capacité nulle, tout
  seuil d'alerte nul, et toute hystérésis supérieure ou égale à son seuil.
  Après création, aucune alerte n'est active et tous les compteurs de panne
  valent 0.
- **Vérification** : `Configuration.*`

### LLR-FQMS-021

- **Type** : LLR
- **Parent** : HLR-FQMS-041
- **Énoncé** : `FuelSystem::update()` doit exécuter exactement cinq étapes,
  dans cet ordre, sans branchement conditionnel sur le nombre d'itérations :
  lecture des trois jauges, totalisation, calcul d'écart, moniteur bas niveau,
  moniteur de déséquilibre.
- **Vérification** : revue de code + `Integration.*`

### LLR-FQMS-030

- **Type** : LLR
- **Parent** : HLR-FQMS-011
- **Énoncé** : `system_status(n)` doit renvoyer `HardwareFault` si `n == 0`,
  `NotReady` si `1 ≤ n ≤ 2`, `Ok` si `n == 3`.
- **Vérification** : `Statut.*`

### LLR-FQMS-031

- **Type** : LLR
- **Parent** : HLR-FQMS-010
- **Énoncé** : La quantité totale doit être la somme des quantités des
  réservoirs valides uniquement ; un réservoir en panne contribue pour 0 et
  voit son champ `sensor_fault` positionné.
- **Vérification** : `Totalisation.*`

### LLR-FQMS-040

- **Type** : LLR
- **Parent** : HLR-FQMS-020
- **Énoncé** : L'écart d'aile doit valoir la valeur absolue de la différence
  entre les quantités des réservoirs gauche et droit.
- **Vérification** : `Desequilibre.*`

### LLR-FQMS-041

- **Type** : LLR
- **Parent** : HLR-FQMS-022
- **Énoncé** : `imbalance_is_measurable(g, d)` doit renvoyer vrai si et
  seulement si les deux jauges d'aile sont valides.
- **Vérification** : `Decisions.desequilibre_mesurable`

### LLR-FQMS-042

- **Type** : LLR
- **Parent** : HLR-FQMS-021, HLR-FQMS-022
- **Énoncé** : L'écart d'aile doit être transmis au moniteur de déséquilibre
  en kilogrammes s'il est mesurable, et sous forme d'échantillon non fini
  sinon.
- **Vérification** : `Desequilibre.*`

### LLR-FQMS-050

- **Type** : LLR
- **Parent** : HLR-FQMS-030, HLR-FQMS-031
- **Énoncé** : Le déficit transmis au moniteur bas niveau doit valoir
  `seuil_bas_niveau − quantité_totale`, exprimé en kilogrammes.
- **Vérification** : `BasNiveau.*`

### LLR-FQMS-051

- **Type** : LLR
- **Parent** : HLR-FQMS-032
- **Énoncé** : `low_fuel_is_measurable(g, c, d)` doit renvoyer vrai si et
  seulement si les **trois** jauges sont valides.
- **Vérification** : `Decisions.bas_niveau_mesurable`

### LLR-FQMS-052

- **Type** : LLR
- **Parent** : HLR-FQMS-032
- **Énoncé** : Si le bas niveau n'est pas mesurable, un échantillon non fini
  doit être transmis au moniteur, dont l'état est alors conservé.
- **Vérification** : `BasNiveau.alerte_gelee_sur_panne`

### LLR-FQMS-060

- **Type** : LLR
- **Parent** : HLR-FQMS-040
- **Énoncé** : `fault_count(tank)` doit renvoyer le nombre de mesures rejetées
  pour ce réservoir depuis la création, et 0 pour tout identifiant hors
  domaine.
- **Vérification** : `Maintenance.*`

---

## 3. Matrice HLR → LLR

| HLR | LLR couvrantes |
|-----|----------------|
| HLR-FQMS-001 | LLR-FQMS-001, LLR-FQMS-010, LLR-FQMS-011 |
| HLR-FQMS-002 | LLR-FQMS-012 |
| HLR-FQMS-010 | LLR-FQMS-031 |
| HLR-FQMS-011 | LLR-FQMS-030 |
| HLR-FQMS-020 | LLR-FQMS-001, LLR-FQMS-040 |
| HLR-FQMS-021 | LLR-FQMS-042 |
| HLR-FQMS-022 | LLR-FQMS-041, LLR-FQMS-042 |
| HLR-FQMS-030 | LLR-FQMS-001, LLR-FQMS-050 |
| HLR-FQMS-031 | LLR-FQMS-050 |
| HLR-FQMS-032 | LLR-FQMS-051, LLR-FQMS-052 |
| HLR-FQMS-040 | LLR-FQMS-060 |
| HLR-FQMS-041 | LLR-FQMS-021 |
| HLR-FQMS-042 | LLR-FQMS-020 |
