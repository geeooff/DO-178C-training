# Checklist — Revue des exigences (HLR et LLR)

> **Objectifs DO-178C couverts** : A-3.1 à A-3.7 (HLR), A-4.1 à A-4.6 (LLR)
> **Indépendance requise** : DAL A et B.

| Champ | Valeur |
|---|---|
| Document revu | |
| Version | |
| Auteur | |
| Relecteur(s) | |
| Date | |
| Verdict | ☐ accepté ☐ accepté sous réserve ☐ refusé |

---

## 1. Conformité au niveau supérieur

- [ ] Chaque HLR est traçable à une exigence système (champ `Parent`).
- [ ] Chaque LLR est traçable à au moins une HLR.
- [ ] Toute exigence sans parent est explicitement marquée **DÉRIVÉE**.
- [ ] Chaque exigence dérivée a été **transmise au processus de sécurité
      système** (DO-178C §5.1.2.h) et la réponse est enregistrée.
- [ ] Aucune HLR n'est orpheline (sans LLR couvrante).

## 2. Exactitude et cohérence

- [ ] Aucune contradiction entre exigences.
- [ ] Les **unités** sont précisées partout où une grandeur physique apparaît.
- [ ] Les domaines d'entrée et de sortie sont **chiffrés**.
- [ ] Les tolérances sont chiffrées et justifiées (budget d'erreur).
- [ ] Les constantes physiques sont tracées à une **source normative**.
- [ ] Le comportement **aux bornes** est précisé (incluse ou exclue).
- [ ] Le comportement **hors domaine** est précisé.
- [ ] En cas d'anomalies simultanées, l'**ordre de priorité est spécifié**.

## 3. Vérifiabilité

- [ ] Chaque exigence est vérifiable par un test, une analyse ou une revue.
- [ ] La méthode de vérification est indiquée.
- [ ] Aucun terme non mesurable : « rapidement », « suffisant », « approprié »,
      « si possible », « le cas échéant », « robuste », « convivial ».
- [ ] Une exigence = **un seul « doit »**. Pas de « et » masquant deux exigences.

## 4. Niveau d'abstraction

- [ ] Une HLR ne nomme ni fonction, ni structure de données, ni algorithme.
- [ ] Une LLR est assez détaillée pour coder sans autre décision de conception.
- [ ] Aucune LLR ne se contente de paraphraser sa HLR (LLR inutile).

## 5. Forme

- [ ] Identifiant unique, stable, **jamais réutilisé** après suppression.
- [ ] Une justification est fournie pour toute exigence non évidente.
- [ ] Formulation au présent de l'obligation (« doit »).
- [ ] Vocabulaire cohérent avec le glossaire du projet.

---

## Constats

| # | Exigence | Constat | Sévérité | Action | Statut |
|---|---|---|---|---|---|
| 1 | | | maj / min / obs | | ouvert |

---

## Signatures

| Rôle | Nom | Date | Visa |
|---|---|---|---|
| Auteur | | | |
| Relecteur (indépendant) | | | |
| Assurance qualité | | | |
