# SRD — Software Requirements Data (extrait)
## Composant : ADC-ALT — Calcul d'altitude barométrique

> **Document** : SRD (Software Requirements Data, DO-178C §11.9)
> **Composant** : ADC-ALT, sous-ensemble de l'Air Data Computer
> **Niveau** : DAL C
> **Version** : 1.0
> **Statut de configuration** : CC1

Ce document contient les **exigences de haut niveau** (HLR). Elles décrivent
**CE QUE** le logiciel doit faire, jamais **COMMENT**. Elles sont dérivées des
exigences système et se tracent vers elles.

Format d'un identifiant : `HLR-ADCALT-nnn`.

---

### HLR-ADCALT-001

- **Type** : HLR
- **Parent** : SYS-ADC-014
- **Énoncé** : Le logiciel doit calculer l'altitude-pression à partir de la
  pression statique mesurée, conformément au modèle d'atmosphère standard
  internationale (ISA, OACI Doc 7488).
- **Justification** : L'altitude-pression est la référence commune à tous les
  aéronefs pour le respect des niveaux de vol.
- **Vérification** : test unitaire + analyse de l'algorithme

### HLR-ADCALT-002

- **Type** : HLR
- **Parent** : SYS-ADC-014
- **Énoncé** : Le domaine de pression statique accepté par le logiciel doit
  être [100,0 ; 1100,0] hPa inclus.
- **Justification** : 1100 hPa correspond au record de pression au niveau de la
  mer ; 100 hPa correspond à environ 53 000 ft, au-delà du plafond de tout
  aéronef civil.
- **Vérification** : test unitaire (bornes)

### HLR-ADCALT-003

- **Type** : HLR
- **Parent** : SYS-ADC-021
- **Énoncé** : Toute pression statique hors du domaine défini par
  HLR-ADCALT-002, ainsi que toute valeur non finie, doit être rejetée. Le
  logiciel ne doit alors produire aucune altitude.
- **Justification** : Une altitude calculée à partir d'une mesure aberrante est
  plus dangereuse que l'absence d'altitude, car elle est indiscernable d'une
  valeur correcte par les systèmes aval.
- **Vérification** : test unitaire (robustesse)

### HLR-ADCALT-004

- **Type** : HLR
- **Parent** : SYS-ADC-014
- **Énoncé** : L'erreur de calcul de l'altitude-pression ne doit pas excéder
  ±20 ft sur l'ensemble du domaine défini par HLR-ADCALT-002.
- **Justification** : La séparation verticale réglementaire RVSM est de
  1000 ft ; le budget d'erreur alloué au calcul logiciel est de 20 ft.
- **Vérification** : test unitaire (comparaison à une table de référence)

### HLR-ADCALT-005

- **Type** : HLR
- **Parent** : SYS-ADC-018
- **Énoncé** : Le logiciel doit fournir une altitude corrigée du calage
  altimétrique (QNH) fourni par l'équipage, dans le domaine
  [948,0 ; 1084,0] hPa inclus.
- **Justification** : Domaine de réglage des altimètres selon la
  réglementation OACI.
- **Vérification** : test unitaire

### HLR-ADCALT-006

- **Type** : HLR
- **Parent** : SYS-ADC-021
- **Énoncé** : Tout calage altimétrique hors du domaine défini par
  HLR-ADCALT-005 doit être rejeté.
- **Justification** : Une saisie erronée de l'équipage ne doit pas produire une
  altitude fausse mais plausible.
- **Vérification** : test unitaire (robustesse)

### HLR-ADCALT-007

- **Type** : HLR
- **Parent** : *(aucun — exigence DÉRIVÉE)*
- **Énoncé** : Le logiciel doit signaler distinctement les causes de rejet
  suivantes : mesure hors domaine, mesure non finie, calage hors domaine.
- **Justification** : **Exigence dérivée**. Elle ne découle d'aucune exigence
  système : elle naît d'une décision de conception (distinguer les causes de
  rejet pour la maintenance). À ce titre, elle doit être **remontée au
  processus de sécurité système** (DO-178C §5.1.2.h), qui vérifie qu'elle
  n'introduit pas de mode de panne non analysé.
- **Vérification** : test unitaire

---

## Note sur les exigences dérivées

Une **exigence dérivée** est une exigence qui n'est pas traçable vers une
exigence de niveau supérieur : elle est introduite par le processus de
développement lui-même (choix d'architecture, contrainte d'implémentation,
besoin de surveillance).

La DO-178C impose deux choses à leur sujet :

1. les **identifier explicitement** comme telles ;
2. les **transmettre au processus de sécurité système**, qui doit vérifier
   qu'elles n'invalident pas l'analyse de sécurité.

C'est le point que les auditeurs regardent en premier : une exigence dérivée
non identifiée est un contournement (volontaire ou non) de l'analyse de
sécurité.
