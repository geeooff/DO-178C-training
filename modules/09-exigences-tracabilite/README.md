# Module 09 — Exigences et traçabilité

> **Durée estimée** : 1 à 2 journées
> **Prérequis** : modules 00 à 08

C'est ici que commence la **partie processus** de la formation. Le C++ passe au
second plan : ce module porte sur ce qui distingue réellement un projet
DO-178C d'un projet ordinaire.

---

## Objectifs pédagogiques

1. Situer HLR, LLR, architecture, code et tests dans la hiérarchie DO-178C.
2. **Rédiger** une exigence vérifiable, et reconnaître une exigence qui ne l'est
   pas.
3. Identifier et traiter une **exigence dérivée**.
4. Mettre en place une **traçabilité bidirectionnelle** et l'outiller.
5. Lire un rapport de traçabilité comme le ferait un auditeur.

---

## 1. Le cours

### 1.1 La hiérarchie

```
   Exigences SYSTÈME                        (hors périmètre logiciel)
          │
          ▼
   HLR — Exigences de HAUT niveau           → document SRD
         CE QUE le logiciel doit faire. Jamais COMMENT.
          │
          ▼
   LLR — Exigences de BAS niveau            → document SDD
   + ARCHITECTURE logicielle
         COMMENT. Assez détaillées pour coder directement.
          │
          ▼
   CODE SOURCE                              annoté @satisfies LLR-…
          │
          ▼
   CAS DE TEST                              TEST_REQ(…, "LLR-…")
```

**Le test qui distingue une HLR d'une LLR** : si vous pouvez l'écrire sans
connaître le langage ni l'architecture, c'est une HLR.

| Exemple | Niveau | Pourquoi |
|---|---|---|
| *« Le domaine de pression accepté est [100 ; 1100] hPa »* | **HLR** | Décrit un comportement observable, sans dire comment. |
| *« `validate_static_pressure(p)` renvoie `Status::OutOfRange` si `p < 100,0` »* | **LLR** | Nomme une fonction, un type de retour : c'est de la conception. |
| *« Le logiciel doit être fiable »* | **ni l'un ni l'autre** | Non vérifiable. À rejeter en revue. |

### 1.2 Qu'est-ce qu'une bonne exigence ?

| Critère | Question à se poser |
|---|---|
| **Atomique** | Contient-elle un seul « doit » ? Si elle contient « et », coupez-la. |
| **Vérifiable** | Puis-je écrire un test qui répond oui ou non, sans jugement ? |
| **Non ambiguë** | « rapidement », « suffisant », « approprié » → à bannir. |
| **Bornée** | Le domaine d'entrée et de sortie est-il chiffré ? |
| **Traçable** | À quelle exigence de niveau supérieur se rattache-t-elle ? |
| **Sans conception** (HLR) | Nomme-t-elle une fonction, une structure de données, un algorithme ? Alors c'est une LLR. |

**Exercice mental** — reformulez : *« Le système doit gérer correctement les
grandes valeurs d'altitude. »*
Questions manquantes : quel domaine ? quelle résolution ? que veut dire
« gérer » aux bornes ? et hors bornes ? qui produit la donnée ? avec quelle
tolérance ?

Comparez avec `HLR-ADCALT-004` dans [`requirements/srd.md`](requirements/srd.md) :

> *L'erreur de calcul de l'altitude-pression ne doit pas excéder ±20 ft sur
> l'ensemble du domaine défini par HLR-ADCALT-002.*

Un chiffre, un domaine, une méthode de vérification évidente.

### 1.3 Les exigences dérivées

Une **exigence dérivée** n'est traçable vers **aucune** exigence de niveau
supérieur : elle naît du processus de développement lui-même (choix
d'architecture, contrainte d'implémentation, besoin de surveillance).

La DO-178C (§5.1.2.h et §5.2.2.d) impose **deux** choses :

1. les **identifier explicitement** comme dérivées ;
2. les **transmettre au processus de sécurité système**, qui vérifie qu'elles
   n'introduisent pas un mode de panne non analysé.

> C'est le premier point que regarde un auditeur. Une exigence dérivée non
> identifiée est un contournement — volontaire ou non — de l'analyse de
> sécurité. Dans ce module, `HLR-ADCALT-007` et `LLR-ADCALT-033` sont dérivées,
> et l'outil les signale explicitement.

### 1.4 Traçabilité bidirectionnelle

| Sens | Ce qu'il démontre | Ce qu'il révèle |
|---|---|---|
| **Descendant** (exigence → code → test) | tout ce qui était demandé est fait et vérifié | exigences non implémentées, non testées |
| **Remontant** (code → exigence) | il n'y a **rien de plus** que ce qui était demandé | **code non justifié** : code mort, fonctions ajoutées « au cas où », reliquats de débogage |

Le sens remontant est celui que les équipes négligent, et celui qui trouve les
vrais problèmes. Un bout de code qui ne se rattache à aucune exigence est soit
du code mort (défaut), soit le signe d'une **exigence manquante** dans la
spécification.

### 1.5 L'outil `trace_check.py`

```bash
python tools/trace_check.py
```

Il lit :
* les exigences déclarées dans `modules/*/requirements/*.md` ;
* les annotations `@satisfies LLR-…` du code de production ;
* les `TEST_REQ(Suite, cas, "LLR-…")` des fichiers de test.

Il reconstruit la matrice et signale **cinq** défauts :

1. exigence **sans code** → non implémentée ;
2. exigence **sans test** → non vérifiée ;
3. code référençant une exigence **inconnue** ;
4. test **sans exigence** → orphelin ;
5. **documentation citant un cas de test inexistant** → référence pourrie.

Le cinquième mérite un mot. Les documents d'exigences citent leurs cas de test
dans le champ *Vérification*, et les README de module dans leur colonne
*Vérifiée par*. Ces références croisées sont écrites **à la main**.

> Une référence croisée que personne ne vérifie **pourrit**. Il suffit de
> renommer un cas de test pour que le document continue à citer un nom qui
> n'existe plus — sans que rien ne le signale. La matrice a alors l'air
> complète, mais elle désigne du vide.
>
> Ce contrôle a trouvé **huit références périmées** dans ce dépôt le jour où il
> a été écrit. Elles y étaient depuis le début.

Plus une **observation** utile : une HLR vérifiée seulement *indirectement*, via
ses LLR. La table A-6 demande aussi des tests fondés sur les HLR.

Autres options :

```bash
python tools/trace_check.py --csv reports/traceabilite.csv
```

```bash
python tools/trace_check.py --strict
```

> **Périmètre** : l'outil ne scanne que les modules possédant un répertoire
> `requirements/`. C'est une décision de configuration explicite, pas une
> convention implicite — les modules pédagogiques 00 à 08 restent hors
> périmètre. `--all` scanne tout le dépôt.

### 1.6 Et la qualification de cet outil ?

`trace_check.py` est un **outil de vérification** au sens de la DO-178C §12.2.
Deux cas :

| Usage | Qualification requise ? |
|---|---|
| Le résultat **remplace** la revue manuelle de la matrice | **Oui**, TQL-5 (DO-330) : besoins opérationnels de l'outil, tests de l'outil, vérification. |
| Le résultat **complète** la revue manuelle, sans la supprimer | **Non**. |

Dans cette formation, on est dans le second cas — et c'est écrit en toutes
lettres dans l'en-tête du script. Expliciter cette distinction est exactement
ce que la DO-330 demande. Voir aussi le module 14.

### 1.7 Le composant support : ADC-ALT

Calcul d'altitude barométrique à partir de la pression statique, selon
l'atmosphère standard internationale (OACI Doc 7488) :

```
h[ft] = 145366,45 × (1 − (P / 1013,25)^0,190284)
```

Plus la correction du calage altimétrique (QNH) : 27 ft par hectopascal
d'écart au standard. C'est ce que fait tout altimètre lorsque le contrôle
aérien annonce « QNH 1008 ».

Deux points de sécurité illustrés :

* **`LLR-ADCALT-032` spécifie l'ordre de validation.** Si la pression *et* le
  calage sont invalides, c'est le statut de la pression qui remonte. Sans cette
  spécification, le comportement dépendrait de l'implémentation, donc ne serait
  pas vérifiable.
* **Toute constante physique est tracée à une source normative.** Un nombre
  magique dans le code est un constat de revue.

---

## 2. Ce que dit la DO-178C

| Objectif | Intitulé | Application |
|---|---|---|
| **A-3.1 à A-3.7** | Vérification des HLR | conformité, exactitude, vérifiabilité, traçabilité vers les exigences système |
| **A-3.6** | HLR traçables aux exigences système | champ `Parent` de chaque HLR |
| **A-4.1 à A-4.6** | Vérification des LLR et de l'architecture | LLR conformes aux HLR, traçables, architecture cohérente |
| **A-5.5** | Code source traçable aux LLR | annotations `@satisfies` |
| **A-6.1 / A-6.2** | Tests basés sur les HLR | suite `SystemeADCALT` |
| **A-6.3 / A-6.4** | Tests basés sur les LLR | suites `Validation`, `Altitude`, `Correction` |
| **§5.1.2.h** | Exigences dérivées transmises à la sécurité | `HLR-ADCALT-007`, `LLR-ADCALT-033` |

---

## 3. Documents produits (et ce qu'ils valent)

| Fichier | Document DO-178C | §  |
|---|---|---|
| [`requirements/srd.md`](requirements/srd.md) | Software Requirements Data (SRD) | 11.9 |
| [`requirements/sdd.md`](requirements/sdd.md) | Design Description (SDD) | 11.10 |
| `src/`, `include/` | Source Code | 11.11 |
| `tests/` | Software Verification Cases and Procedures | 11.13 |
| sortie de `trace_check.py` | élément des Software Verification Results | 11.14 |

Ce sont de **vrais formats**, simplement raccourcis. Sur un programme réel, le
SRD d'un composant fait entre 30 et 300 pages.

---

## 4. Manipulation

```powershell
.\build\debug\bin\demo_09-exigences-tracabilite.exe
```

```bash
python tools/trace_check.py
```

Puis, pour voir l'outil détecter un vrai défaut :

```bash
python tools/trace_check.py --csv reports/traceabilite.csv
```

---

## 5. Exercices

**5.1 — Casser la traçabilité (quatre fois)**
Provoquez, puis corrigez, chacun des quatre défauts :
a) commentez une annotation `@satisfies` dans `altitude.cpp` ;
b) supprimez un `TEST_REQ` (remplacez-le par `TEST`) ;
c) écrivez `@satisfies LLR-ADCALT-999` ;
d) ajoutez une exigence dans `sdd.md` sans l'implémenter.
Relancez l'outil après chaque étape. Notez le message obtenu : c'est ce que
verra l'auditeur.

**5.2 — Rédiger une HLR**
Le système doit fournir la **vitesse verticale** (taux de montée/descente) à
partir de deux mesures d'altitude successives. Rédigez :
une HLR de comportement, une HLR de domaine, une HLR de robustesse, et
identifiez au moins une **exigence dérivée** (indice : que se passe-t-il au
premier cycle, quand il n'y a qu'une seule mesure ?).

**5.3 — De la HLR aux LLR**
Déclinez vos HLR de l'exercice 5.2 en LLR, en respectant le format de
`sdd.md`. Ajoutez-les au fichier, puis vérifiez que `trace_check.py` les
signale comme non implémentées et non testées. Implémentez ensuite, testez,
et vérifiez que les défauts disparaissent.

**5.4 — Critiquer des exigences**
Pour chacune, dites ce qui ne va pas et réécrivez-la :
* *« Le calcul doit être rapide. »*
* *« Le logiciel doit utiliser une table de linéarisation à 16 points. »*
  (posée comme HLR)
* *« Le module doit gérer les erreurs de capteur et signaler les pannes et
  enregistrer les événements. »*
* *« L'altitude doit être correcte. »*

**5.5 — Analyse d'impact**
`HLR-ADCALT-002` change : le domaine devient [50 ; 1100] hPa. Établissez la
liste **complète** de ce qui doit être modifié et re-vérifié : quelles LLR ?
quel code ? quels tests ? quels documents ? C'est exactement le travail
d'*analyse de changement* imposé par la DO-178C §7.2.4.

---

## 6. Pour aller plus loin

* DO-178C §5 (processus de développement), §6.3 (revues et analyses), §11
  (données de vie du logiciel).
* CAST-10 — *What is a "Decision" in Application of Modified Condition/Decision
  Coverage* — et plus généralement les papiers CAST, gratuits et concrets.
* ARP4754A — développement des systèmes aéronautiques : c'est de là que
  viennent les exigences système et l'allocation des DAL.
* Outils du marché : IBM DOORS, Polarion, Codebeamer, Jama. Tous font ce que
  fait `trace_check.py`, en beaucoup plus cher — et parfois moins clairement.

---

⬅️ [08 — Mémoire statique](../08-memoire-statique/README.md) |
➡️ [10 — Tests basés sur les exigences](../10-tests-bases-exigences/README.md)
