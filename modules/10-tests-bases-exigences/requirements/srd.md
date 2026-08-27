# SRD — Software Requirements Data (extrait)
## Composant : ALERT-MON — Surveillance de dépassement de seuil

> **Composant** : ALERT-MON (générique, instancié pour survitesse, bas niveau
> carburant, survitesse volets…)
> **Niveau** : DAL B
> **Version** : 1.0

Ce composant surveille une grandeur cyclique et lève une alerte lorsqu'elle
dépasse un seuil. Deux mécanismes le rendent utilisable en vol :

* la **confirmation** (anti-rebond) : l'alerte ne se lève qu'après N cycles
  consécutifs au-delà du seuil, ce qui filtre le bruit du capteur ;
* l'**hystérésis** : le seuil de retombée est inférieur au seuil de montée, ce
  qui évite le battement quand la valeur oscille autour du seuil.

Sans ces deux mécanismes, une alerte cabine clignoterait à chaque rafale.

---

### HLR-ALERT-001

- **Type** : HLR
- **Parent** : SYS-WARN-101
- **Énoncé** : L'alerte doit passer à l'état actif lorsque la grandeur
  surveillée est strictement supérieure au seuil de montée pendant
  `confirm_cycles` cycles consécutifs.
- **Vérification** : test unitaire (séquences)

### HLR-ALERT-002

- **Type** : HLR
- **Parent** : SYS-WARN-101
- **Énoncé** : L'alerte doit repasser à l'état inactif lorsque la grandeur
  surveillée est strictement inférieure au seuil de retombée pendant
  `clear_cycles` cycles consécutifs.
- **Vérification** : test unitaire (séquences)

### HLR-ALERT-003

- **Type** : HLR
- **Parent** : SYS-WARN-102
- **Énoncé** : Tout cycle ne satisfaisant pas la condition en cours de
  confirmation doit annuler cette confirmation et restaurer l'état précédent.
- **Justification** : Le comptage doit porter sur des cycles **consécutifs** ;
  un compteur qui ne se réinitialise pas déclencherait l'alerte sur des
  dépassements isolés cumulés.
- **Vérification** : test unitaire (séquences)

### HLR-ALERT-004

- **Type** : HLR
- **Parent** : SYS-WARN-102
- **Énoncé** : La configuration doit être rejetée si le seuil de retombée est
  supérieur ou égal au seuil de montée, si l'un des compteurs de confirmation
  est nul, ou si l'un des seuils n'est pas un nombre fini.
- **Justification** : Une configuration sans hystérésis produirait un battement
  d'alerte ; un compteur nul supprimerait l'anti-rebond.
- **Vérification** : test unitaire (robustesse)

### HLR-ALERT-005

- **Type** : HLR
- **Parent** : SYS-WARN-101
- **Énoncé** : À l'initialisation, l'alerte doit être inactive, quelle que soit
  la valeur du premier échantillon.
- **Justification** : Une alerte active au démarrage du calculateur serait
  interprétée comme une panne réelle par l'équipage.
- **Vérification** : test unitaire

### HLR-ALERT-006

- **Type** : HLR
- **Parent** : SYS-WARN-103
- **Énoncé** : Le composant doit compter le nombre total d'activations depuis
  l'initialisation.
- **Justification** : Donnée de maintenance (BITE) : une alerte qui se lève
  vingt fois par vol signale un capteur défaillant.
- **Vérification** : test unitaire

### HLR-ALERT-007

- **Type** : HLR
- **Parent** : *(aucun — exigence DÉRIVÉE)*
- **Énoncé** : Un échantillon non fini (NaN ou infini) doit être traité comme
  ne satisfaisant aucune condition de seuil, sans modifier l'état de l'alerte
  ni les compteurs de confirmation.
- **Justification** : **Exigence dérivée**. Aucune exigence système ne traite
  ce cas. Le choix retenu — « ignorer l'échantillon » — évite qu'un capteur en
  panne lève ou efface une alerte. À remonter au processus de sécurité
  système : il faut vérifier qu'une panne capteur prolongée est bien couverte
  par une autre surveillance.
- **Vérification** : test unitaire (robustesse)
