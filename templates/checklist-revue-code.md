# Checklist — Revue de code source

> **Objectifs DO-178C couverts** : A-5.1 à A-5.6
> **Indépendance requise** : DAL A et B — le relecteur ne peut pas être l'auteur.
> **Durée conseillée** : 300 à 500 lignes par séance, pas plus.

| Champ | Valeur |
|---|---|
| Composant | |
| Fichiers revus | |
| Version / commit | |
| Auteur | |
| Relecteur(s) | |
| Date | |
| Verdict | ☐ accepté ☐ accepté sous réserve ☐ refusé |

---

## 1. Conformité aux exigences de bas niveau — A-5.1

- [ ] Chaque fonction non triviale porte une annotation `@satisfies LLR-…`.
- [ ] Le code fait **exactement** ce que dit l'exigence — ni moins, ni **plus**.
- [ ] Les valeurs numériques du code correspondent à celles de l'exigence
      (seuils, domaines, tolérances, unités).
- [ ] Le comportement **aux bornes** est conforme (`<` contre `<=`).
- [ ] Le comportement **hors domaine** est conforme à l'exigence de robustesse.
- [ ] Aucun code n'est présent sans exigence correspondante.

## 2. Conformité à l'architecture — A-5.2

- [ ] Le composant n'appelle que les interfaces prévues par le SDD.
- [ ] Aucune dépendance nouvelle non documentée (voir matrice de couplage).
- [ ] Aucune variable globale mutable partagée.
- [ ] Le sens des flux de données correspond à la matrice de couplage.

## 3. Vérifiabilité — A-5.3

- [ ] Chaque décision est atteignable par un jeu d'entrées réaliste.
- [ ] Aucune condition constante ni branche inatteignable.
- [ ] Les décisions complexes sont extraites en fonctions pures de booléens
      (couverture MC/DC praticable — module 11).
- [ ] Les conditions sont évaluées avant la décision (pas de masquage par
      court-circuit).

## 4. Conformité au standard de codage — A-5.4

- [ ] Pas de `using namespace` en portée de fichier.
- [ ] Aucun nombre magique ; toute constante est nommée et tracée à sa source.
- [ ] Pas de macro de type fonction.
- [ ] Toute variable est initialisée à sa déclaration.
- [ ] Types de largeur explicite (`avio::u16`), jamais `int` ni `long`.
- [ ] Conversions par `static_cast`, jamais à la manière du C.
- [ ] Pas de récursion.
- [ ] Aucune allocation dynamique après initialisation.
- [ ] `noexcept` sur toute fonction ; aucune exception.
- [ ] Boucles à bornes connues et constantes.
- [ ] Pas de `goto` ; `break`/`continue` justifiés.
- [ ] Formatage conforme (`clang-format` ne produit aucune modification).
- [ ] Toute déviation porte un commentaire de justification (`NOLINTNEXTLINE`
      nu = refus).

## 5. Traçabilité — A-5.5

- [ ] `trace_check.py` ne signale aucun défaut sur ce composant.
- [ ] Aucune exigence référencée n'est inconnue.
- [ ] Aucun test orphelin.

## 6. Exactitude et cohérence — A-5.6

- [ ] **Débordement** : toute opération arithmétique est bornée ou vérifiée.
- [ ] **Division** : le diviseur nul est traité ; `INT_MIN / -1` est traité.
- [ ] **Conversions** : aucune conversion rétrécissante non vérifiée.
- [ ] **Flottants** : pas d'égalité exacte ; NaN et ±∞ filtrés aux frontières.
- [ ] **Pointeurs** : pas d'arithmétique ; durée de vie démontrable.
- [ ] **Tableaux** : tout accès est borné ; la taille voyage avec la donnée.
- [ ] **Pile** : profondeur bornée ; pas de VLA, pas d'`alloca`.
- [ ] **Ressources** : acquises et libérées par RAII, sur tous les chemins.
- [ ] **Initialisation** : aucun membre laissé indéterminé.
- [ ] **Ordre d'évaluation** : au plus un effet de bord par expression.
- [ ] **Concurrence** : les données partagées sont protégées ; les sections
      critiques sont courtes et bornées.

## 7. Lisibilité et maintenance

- [ ] Les noms disent l'intention, pas l'implémentation.
- [ ] Les commentaires expliquent le **pourquoi**, pas le **quoi**.
- [ ] Aucun code commenté laissé en place.
- [ ] Aucun `TODO` / `FIXME` sans référence à une anomalie ouverte.

---

## Constats

| # | Fichier:ligne | Catégorie | Constat | Sévérité | Action | Statut |
|---|---|---|---|---|---|---|
| 1 | | | | maj / min / obs | | ouvert |
| 2 | | | | | | |

**Sévérité** — *majeur* : non-conformité à un objectif, bloque l'acceptation ;
*mineur* : à corriger avant baseline ; *observation* : amélioration suggérée.

---

## Signatures

| Rôle | Nom | Date | Visa |
|---|---|---|---|
| Auteur | | | |
| Relecteur (indépendant) | | | |
| Assurance qualité | | | |
