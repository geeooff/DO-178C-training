# Plan de formation

> 17 modules, environ **20 journées** de travail effectif.
> Chaque module est autonome mais s'appuie sur les précédents.

---

## Rythme conseillé

| Formule | Durée | Commentaire |
|---|---|---|
| **Intensif** | 4 semaines à plein temps | rythme d'une formation professionnelle |
| **Soutenu** | 10 semaines à mi-temps | un module tous les 2-3 jours |
| **En parallèle d'un emploi** | 4 à 5 mois | 1 module par semaine, le week-end |

> **Ne sautez pas les exercices.** Les modules se lisent en une heure ; ce sont
> les exercices qui font la différence entre « avoir lu » et « savoir faire ».
> C'est aussi d'eux que viendront vos réponses en entretien.

---

## Progression

### Partie 0 — Mise en route (1 jour)

| # | Module | Contenu | DO-178C |
|:-:|---|---|---|
| 00 | [Environnement](../modules/00-environnement/) | CMake, MSVC, modèle de compilation C++ vs C# | vue d'ensemble, DAL, tables A-1 à A-10 |

### Partie 1 — Le langage C++ vu par un développeur C# (8 jours)

| # | Module | Contenu | DO-178C |
|:-:|---|---|---|
| 01 | [Types et mémoire](../modules/01-types-et-memoire/) | largeurs fixes, promotions, débordement, disposition mémoire | A-5.6, Ariane 5 |
| 02 | [Pointeurs et `const`](../modules/02-pointeurs-references-const/) | référence vs pointeur, `const`, `Span`, durée de vie | A-5.6, code défensif |
| 03 | [RAII](../modules/03-raii-cycle-de-vie/) | constructeurs/destructeurs, règle de 0/3/5, déplacement | déterminisme, DO-332 OO.6.8.2 |
| 04 | [Classes et invariants](../modules/04-classes-invariants/) | invariants, fabriques validantes, **types forts** | A-4.1, Air Canada 143 |
| 05 | [Polymorphisme et DO-332](../modules/05-polymorphisme-do332/) | `virtual`, découpage, **cohérence locale de type** | **DO-332 OO.6.7** |
| 06 | [Templates et `constexpr`](../modules/06-templates-constexpr/) | templates, CRTP, calcul à la compilation | **couverture par instanciation** |
| 07 | [Erreurs sans exceptions](../modules/07-erreurs-sans-exceptions/) | `Result<T>`, ARINC 429, code mort/désactivé | A-4.11, §6.4.4.3 |
| 08 | [Mémoire statique](../modules/08-memoire-statique/) | `StaticVector`, réserve de blocs, analyse de pile | DO-332 OO.6.8.2, A-5.6 |

### Partie 2 — Les processus DO-178C (7 jours)

| # | Module | Contenu | DO-178C |
|:-:|---|---|---|
| 09 | [Exigences et traçabilité](../modules/09-exigences-tracabilite/) | HLR/LLR, exigences dérivées, **outil de traçabilité** | A-3.x, A-4.x, A-5.5 |
| 10 | [Tests basés sur les exigences](../modules/10-tests-bases-exigences/) | classes d'équivalence, valeurs limites, **mutation** | A-6.x, §6.4.2 |
| 11 | [Couverture structurelle](../modules/11-couverture-structurelle/) | statement, decision, **MC/DC** + analyseur | **A-7.5 à A-7.9** |
| 12 | [Couplage données/contrôle](../modules/12-couplage-donnees-controle/) | matrices de couplage, tests d'intégration | **A-7.8** |
| 13 | [Standards de codage](../modules/13-standards-codage/) | MISRA, clang-tidy, **déviations** | A-5.4, §11.8 |
| 14 | [Configuration et qualité](../modules/14-configuration-qualite/) | baselines, CC1/CC2, **DO-330** | §7, §8, §11, §12.2 |

### Partie 3 — Contraintes de l'embarqué (2 jours)

| # | Module | Contenu | DO-178C |
|:-:|---|---|---|
| 15 | [Déterminisme et temps réel](../modules/15-determinisme-temps-reel/) | IEEE-754, virgule fixe, ARINC 653, WCET, `volatile` | A-5.6, CAST-32A |

### Partie 4 — Synthèse (2 à 3 jours)

| # | Module | Contenu |
|:-:|---|---|
| 16 | [Projet intégré — FQMS](../modules/16-projet-integre/) | système complet + dossier de certification miniature |

---

## Comment travailler un module

1. **Lire le README** en entier. Comptez 45 à 90 minutes.
2. **Lancer la démonstration** :
   `.\build\debug\bin\demo_<module>.exe`
3. **Lire le code source**, dans l'ordre : `include/`, puis `src/`, puis
   `tests/`. Les commentaires font partie du cours.
4. **Lancer les tests** avec `--verbose --req` pour voir la matrice de
   traçabilité.
5. **Faire les exercices.** Tous. Ce sont eux qui construisent la compétence.
6. **Committer votre travail** — vous constituez votre propre historique.

---

## Ce que vous saurez faire à la fin

**En C++**

- lire et écrire du C++ moderne restreint (C++17, sans exceptions ni
  allocation dynamique) ;
- concevoir des classes à invariant, des types forts, des conteneurs à
  capacité fixe ;
- choisir entre polymorphisme dynamique et statique, et le justifier ;
- écrire du code dont le comportement temporel et numérique est **prévisible**.

**En DO-178C**

- situer un objectif dans les tables A-1 à A-10 ;
- rédiger des HLR et des LLR vérifiables, et repérer une exigence dérivée ;
- concevoir une campagne de test raisonnée, avec robustesse ;
- expliquer MC/DC, construire un jeu minimal et le vérifier ;
- conduire une analyse de couplage données/contrôle ;
- appliquer un standard de codage et instruire une déviation ;
- dire si un outil doit être qualifié, sous quel critère et à quel TQL.

**En entretien**

- expliquer *pourquoi* chaque interdiction existe, pas seulement *qu'elle*
  existe ;
- montrer un projet complet et cohérent, avec son dossier ;
- **connaître les limites de votre propre travail** — c'est ce qui distingue
  un candidat.

---

## Ce que cette formation ne couvre pas

Soyons honnêtes sur le périmètre :

| Sujet | Pourquoi c'est absent |
|---|---|
| Tests **sur cible réelle** (A-6.5) | il faudrait un calculateur et une chaîne croisée |
| Couverture du **code objet** (A-7.9) | nécessite un désassembleur et un outil qualifié |
| Analyse **WCET** réelle | outils commerciaux (aiT, RapiTime) |
| **Multicœur** et CAST-32A | sujet à part entière |
| **DO-331** (Simulink/SCADE) | autre chaîne d'outils |
| **DO-333** (méthodes formelles) | autre discipline |
| **Rédaction complète** des plans (PSAC, SDP…) | quelques centaines de pages sur un vrai programme |
| Relation avec l'**autorité** de certification | s'apprend en poste |

Ces sujets sont **cités** là où ils s'insèrent, avec des références pour aller
plus loin. Savoir qu'ils existent et où ils s'appliquent est déjà beaucoup.
