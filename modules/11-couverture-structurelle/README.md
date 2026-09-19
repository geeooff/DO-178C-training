# Module 11 — Couverture structurelle et MC/DC

> **Durée estimée** : 1 à 2 journées
> **Prérequis** : modules 00 à 10

Le MC/DC est le critère de couverture le plus cité — et le plus mal compris —
du domaine. Ce module le rend **tangible** : un analyseur MC/DC opérationnel
est fourni, et les tests l'utilisent pour *prouver* que les jeux d'essai
atteignent l'objectif.

---

## Objectifs pédagogiques

1. Définir précisément *statement*, *decision* et **MC/DC**.
2. Savoir quel critère s'applique à quel niveau DAL.
3. **Construire** un jeu de tests MC/DC minimal, et le **vérifier**.
4. Comprendre l'effet du **court-circuit** et la parade de conception.
5. Distinguer ce qu'un outil de couverture mesure de ce qu'il ne mesure pas.

---

## 1. Le cours

### 1.1 Les trois critères

| Critère | Définition | Objectif |
|---|---|---|
| **Statement** (instruction) | chaque instruction exécutable a été exécutée au moins une fois | A-7.7 |
| **Decision** (branche) | chaque point de décision a pris **toutes** ses issues | A-7.6 |
| **MC/DC** | chaque **condition** d'une décision a démontré qu'elle affecte **seule** l'issue | A-7.5 |

Vocabulaire, à ne pas confondre :

* une **condition** est une expression booléenne **élémentaire** :
  `altitude < 500` ;
* une **décision** est l'expression booléenne **complète** qui contrôle le
  flot : `altitude < 500 && vitesse < 190 && train_rentre && en_vol`.

### 1.2 Quel critère pour quel niveau ?

| DAL | statement | decision | MC/DC | couplage données/contrôle |
|:---:|:---:|:---:|:---:|:---:|
| **A** | oui | oui | **OUI** | oui |
| **B** | oui | oui | non | oui |
| **C** | oui | non | non | oui |
| **D** | non | non | non | non |
| **E** | non | non | non | non |

> Passer de DAL B à DAL A, c'est essentiellement **ajouter MC/DC**. C'est aussi
> ce qui explique l'essentiel de l'écart de coût entre les deux niveaux.

### 1.3 La définition exacte de MC/DC

La couverture MC/DC est atteinte lorsque :

1. chaque **point d'entrée et de sortie** du programme a été invoqué ;
2. chaque **condition** a pris **toutes** les valeurs possibles ;
3. chaque **décision** a pris **toutes** ses issues ;
4. chaque **condition** a démontré qu'elle affecte **seule** l'issue de la
   décision.

C'est le point (4) qui distingue MC/DC de la simple couverture des conditions.
Il demande, pour chaque condition, d'exhiber une **paire d'indépendance** :
deux jeux d'entrées où **seule** cette condition change, et où l'issue de la
décision change aussi.

### 1.4 Le piège : 100 % de décision, 0 % de MC/DC

Deux tests suffisent à couvrir la décision `C1 && C2 && C3 && C4` :

| # | C1 | C2 | C3 | C4 | issue |
|---|----|----|----|----|-------|
| 0 | V | V | V | V | **VRAI** |
| 1 | F | F | F | F | faux |

Couverture de décision : **100 %**. Chaque condition a même pris ses deux
valeurs. Et pourtant, MC/DC : **0 %**.

Pourquoi ? Les deux évaluations diffèrent sur **les quatre conditions à la
fois** : impossible d'attribuer le changement d'issue à l'une d'elles.
Concrètement, une condition pourrait être **inversée dans le code** sans
qu'aucun de ces deux tests ne le détecte.

C'est exactement ce que démontre le test
`Mcdc.decision_coverage_is_not_enough`.

### 1.5 Le jeu MC/DC minimal

Pour `C1 && C2 && C3 && C4` :

| # | C1 | C2 | C3 | C4 | issue | rôle |
|---|----|----|----|----|-------|------|
| 0 | V | V | V | V | **VRAI** | référence |
| 1 | **F** | V | V | V | faux | paire d'indépendance de C1 avec #0 |
| 2 | V | **F** | V | V | faux | paire de C2 |
| 3 | V | V | **F** | V | faux | paire de C3 |
| 4 | V | V | V | **F** | faux | paire de C4 |

**5 tests au lieu de 16.** Le gain croît vite :

| N conditions | MC/DC | exhaustif |
|---|---|---|
| 4 | 5 | 16 |
| 8 | 9 | 256 |
| 16 | 17 | 65 536 |

C'est ce compromis — une confiance proche de l'exhaustif pour un coût linéaire
— qui a fait retenir MC/DC pour le DAL A.

### 1.6 Une décision mixte

`inhibition = A || (B && C)` — trois conditions, mais le jeu minimal n'a plus
rien d'évident :

| # | A | B | C | issue | rôle |
|---|---|---|---|-------|------|
| 0 | F | F | V | faux | référence |
| 1 | **V** | F | V | VRAI | paire de A avec #0 |
| 2 | F | **V** | V | VRAI | paire de B avec #0 |
| 3 | F | V | **F** | faux | paire de C avec #2 |

4 tests pour 3 conditions : l'optimum théorique N+1. Remarquez que
l'évaluation #0 sert de référence pour **deux** paires, et #2 pour la
troisième. Trouver un jeu minimal sur une décision mixte est un vrai travail
d'ingénierie de test — souvent outillé.

### 1.7 Le court-circuit

En C++, `&&` et `||` sont à **court-circuit** : dans `a && b`, `b` n'est pas
évalué si `a` est faux. Une condition non évaluée n'a donc **pris aucune
valeur**, ce qui complique l'analyse.

Deux variantes de MC/DC sont acceptées par les autorités :

| Variante | Règle | Usage |
|---|---|---|
| **Unique cause** | la paire d'indépendance ne diffère **que** par la condition étudiée | définition d'origine, la plus stricte ; implémentée dans `mcdc.cpp` |
| **Masking** | d'autres conditions peuvent changer si l'on démontre qu'elles sont **masquées** (sans effet sur l'issue) | nécessaire dès qu'une décision contient des conditions couplées |

**La parade de conception**, appliquée dans ce module :

```cpp
const bool alt = radio_altitude_ft < 500.0F;
const bool vit = airspeed_kt < 190.0F;
const bool trn = !gear_down_locked;
const bool vol = !on_ground;
return alt && vit && trn && vol;   // toutes déjà évaluées
```

Les conditions sont évaluées **avant** la décision : le court-circuit ne masque
plus rien, l'analyse devient exacte, et le code est plus lisible.

> C'est **la** décision de conception à savoir défendre en revue : *« j'ai
> extrait la décision dans une fonction pure de booléens pour la rendre
> couvrable »*. Elle est documentée comme telle dans
> [`requirements/sdd.md`](requirements/sdd.md), décision DA-01.

### 1.8 Mesurer pour de vrai

L'analyseur de ce module raisonne sur les **décisions**. Pour la couverture
d'**instructions** et de **branches**, il faut instrumenter le binaire.

```powershell
.\scripts\coverage.ps1
```

Installation d'OpenCppCoverage (gratuit, open source) :

```bash
winget install OpenCppCoverage.OpenCppCoverage
```

> ⚠️ **OpenCppCoverage mesure la couverture d'instructions. Il ne mesure ni la
> couverture de décision, ni le MC/DC.** Pour ces deux critères, il faut un
> outil commercial qualifié (VectorCAST, LDRA Testbed, Rational Test RealTime,
> Cantata) ou une analyse manuelle appuyée sur des tables de décision — comme
> celle que produit `mod11::analyze_mcdc`.
>
> Beaucoup de projets croient couvrir le MC/DC avec un outil qui ne le mesure
> pas. Savoir faire cette distinction en entretien fait une excellente
> impression.

### 1.9 Couverture du code objet (A-7.9)

Piège majeur du DAL A, et poste de coût le plus sous-estimé :

> Si le compilateur génère du **code objet non traçable au code source**, la
> couverture doit être démontrée **au niveau du code objet**.

Exemples de code objet sans équivalent source :
* vérifications implicites insérées par le compilateur ;
* déroulage de boucle, vectorisation ;
* tables de saut générées pour un `switch` ;
* code d'initialisation de tableaux.

C'est l'une des raisons pour lesquelles les programmes DAL A figent un
compilateur, un jeu d'options, et évitent les optimisations agressives.

### 1.10 Et si du code n'est pas couvert ?

DO-178C §6.4.4.3 impose de **résoudre** chaque cas :

| Cause | Action |
|---|---|
| Cas de test manquant | **ajouter le test** |
| Exigence manquante | **écrire l'exigence**, puis le test |
| **Code mort** | **supprimer le code**, et analyser pourquoi il existait |
| **Code désactivé** | l'identifier, démontrer qu'il ne peut pas être activé, justifier |

Le non-couvert n'est **jamais** acceptable en l'état : il faut toujours une des
quatre réponses ci-dessus.

---

## 2. Ce que dit la DO-178C

| Objectif | Intitulé | Niveau |
|---|---|---|
| **A-7.5** | Couverture MC/DC atteinte | A |
| **A-7.6** | Couverture de décision atteinte | A, B |
| **A-7.7** | Couverture d'instructions atteinte | A, B, C |
| **A-7.8** | Couplage données et contrôle vérifié | A, B, C (module 12) |
| **A-7.9** | Couverture du code objet non traçable au source | A |
| **§6.4.4.2** | Analyse de couverture structurelle | — |
| **§6.4.4.3** | Résolution du code non couvert | — |

---

## 3. Manipulation

```powershell
.\build\debug\bin\demo_11-couverture-structurelle.exe
```

```powershell
.\build\debug\bin\tests_11-couverture-structurelle.exe --verbose --req
```

```powershell
.\scripts\coverage.ps1 -Module 11-couverture-structurelle
```

---

## 4. Exercices

**4.1 — Construire un jeu MC/DC à la main**
Pour la décision `(A && B) || (C && D)` (4 conditions), construisez le jeu
minimal **sur papier**, puis vérifiez-le avec `analyze_mcdc`. Combien de tests
avez-vous trouvés ? L'optimum théorique est 5 ; y arrivez-vous ?

**4.2 — Une condition inversée**
Dans `mode4a_decision`, remplacez `gear_not_down` par `!gear_not_down`.
Lancez les tests : lesquels échouent ? Puis refaites l'expérience avec
seulement les deux tests du §1.4 (100 % décision, 0 % MC/DC) : le défaut
passe-t-il inaperçu ? Vous venez de démontrer *pourquoi* le DAL A exige MC/DC.

**4.3 — Court-circuit**
Écrivez une variante `mode4a_decision_shortcircuit` qui évalue les conditions
**dans** l'expression (avec des appels de fonction ayant un effet de bord
observable, par exemple un compteur d'appels). Mesurez combien de fois chaque
condition est réellement évaluée pour le jeu MC/DC minimal. Concluez.

**4.4 — Code non couvert**
Ajoutez à `gpws.cpp` une branche inatteignable, par exemple :
```cpp
if (kMode4aAltitudeLimitFt < 0.0F) { return false; }
```
Lancez `coverage.ps1`. La branche apparaît-elle ? Quelle est la bonne action
selon §6.4.4.3 ? Rédigez la justification que vous mettriez au dossier si vous
vouliez la conserver.

**4.5 — Le coût du DAL A**
Comptez les décisions et conditions de `alert_monitor.cpp` (module 10).
Estimez le nombre de cas de test nécessaires pour : la couverture
d'instructions, la couverture de décision, le MC/DC. Quel facteur entre les
trois ? C'est le chiffrage que l'on vous demandera de faire en avant-vente.

---

## 5. Pour aller plus loin

* **CAST-6** — *Rationale for Accepting Masking MC/DC in Certification
  Projects*. Le document de référence sur la variante *masking*.
* **CAST-10** — *What is a "Decision" in Application of MC/DC*.
* **CAST-17** — *Structural Coverage of Object Code*.
* Kelly J. Hayhurst *et al.*, *A Practical Tutorial on Modified
  Condition/Decision Coverage*, NASA/TM-2001-210876 — **gratuit, 85 pages, et
  la meilleure introduction au sujet qui existe**. À lire absolument.
* OpenCppCoverage : <https://github.com/OpenCppCoverage/OpenCppCoverage>

---

⬅️ [10 — Tests basés sur les exigences](../10-tests-bases-exigences/README.md) |
➡️ [12 — Couplage données et contrôle](../12-couplage-donnees-controle/README.md)
