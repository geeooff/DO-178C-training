# Module 16 — Projet intégré : FQMS

> **Durée estimée** : 2 à 3 journées
> **Prérequis** : modules 00 à 15

Le module de synthèse. Un composant avionique complet, avec son **dossier de
certification miniature** : exigences, conception, code, deux campagnes de
test, traçabilité, matrices de couplage.

---

## Objectifs pédagogiques

1. Assembler tout ce que la formation a couvert sur un composant réaliste.
2. Produire un **dossier** cohérent, pas seulement du code.
3. Comprendre ce que coûte — et ce que rapporte — la **réutilisation** de
   composants déjà vérifiés.
4. Savoir présenter ce travail en entretien.

---

## 1. Le système

**FQMS** — *Fuel Quantity Management System*. Gestion de la quantité de
carburant d'un biréacteur court/moyen courrier.

| Réservoir | Capacité |
|---|---:|
| Aile gauche | 5 000 kg |
| Caisson central | 8 000 kg |
| Aile droite | 5 000 kg |
| **Total** | **18 000 kg** |

### 1.1 Niveau : DAL B

**Justification** (issue de l'analyse de sécurité système, ARP4761) : une
indication de quantité **erronée par excès** peut conduire l'équipage à
décoller avec un carburant insuffisant, donc à une **panne sèche en vol** —
condition de panne **dangereuse**.

> Le vol **Air Canada 143** (1983) en est l'illustration : un Boeing 767 tombé
> en panne sèche à 12 500 m à cause d'une confusion livres/kilogrammes, posé en
> vol plané sur un aérodrome désaffecté à Gimli. C'est le même accident qui a
> ouvert le module 04.

### 1.2 Les deux alertes

| Alerte | Montée | Retombée | Confirmation |
|---|---|---|---|
| **Bas niveau** | total < 1500 kg | total > 1700 kg | 5 cycles |
| **Déséquilibre** | \|gauche − droite\| > 500 kg | écart < 400 kg | 5 cycles |

---

## 2. Le dossier

| Livrable | Fichier | Document DO-178C |
|---|---|---|
| Exigences de haut niveau | [`requirements/srd.md`](requirements/srd.md) | SRD (§11.9) |
| Conception et exigences de bas niveau | [`requirements/sdd.md`](requirements/sdd.md) | SDD (§11.10) |
| Code source | [`include/`](include/), [`src/`](src/) | Source Code (§11.11) |
| Tests fondés sur les **HLR** | [`tests/test_fqms_hlr.cpp`](tests/test_fqms_hlr.cpp) | SVCP (§11.13), table A-6.1/A-6.2 |
| Tests fondés sur les **LLR** | [`tests/test_fqms_llr.cpp`](tests/test_fqms_llr.cpp) | SVCP (§11.13), table A-6.3/A-6.4 |
| Traçabilité | sortie de `trace_check.py` | SVR (§11.14) |
| Identification | sortie de `config_index.py` | SCI / SECI (§11.15, §11.16) |

**Deux campagnes distinctes, et c'est délibéré.** La table A-6 sépare les tests
fondés sur les HLR de ceux fondés sur les LLR. Les premiers raisonnent en
comportement observable par l'équipage ; les seconds vérifient chaque décision
de conception. En projet réel, les tests HLR seraient écrits par une personne
**différente** de l'auteur du code (indépendance exigée en DAL B).

---

## 3. Ce que le module réutilise

| Composant | Module | Ce qu'il apporte |
|---|---|---|
| `mod04::Mass` | 04 | type fort, arithmétique saturante, invariant « masse ≥ 0 » |
| `mod07::Result<T>`, `Status` | 07 | remontée d'erreur sans exception |
| `mod10::AlertMonitor` | 10 | hystérésis et anti-rebond des deux alertes |

**La réutilisation n'est pas gratuite.** Ces trois composants deviennent des
dépendances **CC1** du FQMS. Toute évolution de leur comportement impose une
**analyse de changement** (§7.2.4) sur le FQMS. C'est explicitement documenté
dans le SDD §1.1, et c'est le genre de lien qu'un auditeur cherche.

### 3.1 Le plus bel exemple de réutilisation du dépôt

`mod10::AlertMonitor` **ignore les échantillons non finis** sans modifier son
état. Cette exigence (`LLR-ALERT-050`) était, au module 10, une exigence
**dérivée** de robustesse : *« un capteur en panne ne doit ni lever ni effacer
une alerte »*.

Le FQMS s'en sert comme d'une **brique fonctionnelle** : lorsqu'une grandeur
n'est pas mesurable (jauge en panne), il transmet **NaN** au moniteur, dont
l'état est ainsi **gelé**. C'est exactement le comportement exigé par
`HLR-FQMS-022` et `HLR-FQMS-032`.

> Une exigence dérivée d'un composant devient une brique fonctionnelle d'un
> autre. **Ce lien doit être documenté**, faute de quoi une évolution
> d'`AlertMonitor` casserait le FQMS sans que personne ne le voie. C'est
> précisément ce que le SDD §1.2 (décision DA-02) enregistre.

---

## 4. Le cas qui justifie tout le dossier

Lancez la démonstration : la section « panne de jauge » montre le scénario
suivant.

| Situation | Total visible | Alerte bas niveau |
|---|---:|---|
| Nominal | 2 200 kg | — |
| Jauge centrale en panne | **600 kg** | **aucune** |

La quantité visible est tombée **bien sous** le seuil de 1500 kg, et pourtant
aucune alerte.

**Pourquoi c'est correct** : la quantité n'est plus *mesurable*
(`HLR-FQMS-032`). Sans ce gel, l'équipage verrait une alerte BAS NIVEAU alors
que l'avion a 2 200 kg à bord, et **se dérouterait sans raison**.

> Une fausse alerte à effet opérationnel majeur est un défaut de sécurité, **au
> même titre qu'une alerte manquante**. C'est un point que beaucoup de
> développeurs découvrent tard : en avionique, le système doit être juste dans
> les deux sens.

Et le statut passe à *NonDisponible* : l'équipage **sait** que la quantité
affichée est incomplète (`HLR-FQMS-011`).

---

## 5. Ce que le module démontre, module par module

| Module | Manifestation concrète dans le FQMS |
|---|---|
| 01 | types de largeur fixe, conversion brut→masse en 64 bits pour éviter tout débordement |
| 02 | `const`-correctness, aucune arithmétique de pointeur |
| 03 | **règle de 0** : aucune ressource à libérer, aucun destructeur écrit |
| 04 | `Mass` : type fort, fabrique validante, invariant établi par construction |
| 05 | hiérarchie **plate** : aucune fonction virtuelle, aucun objectif DO-332 sur l'héritage |
| 06 | constantes `constexpr`, aucune instanciation de template superflue |
| 07 | `Result<T>` et `Status` : aucune exception, propagation explicite |
| 08 | **aucune allocation dynamique** ; `sizeof(FuelSystem)` est une constante |
| 09 | SRD et SDD complets, annotations `@satisfies`, exigences dérivées identifiées |
| 10 | `AlertMonitor` réutilisé pour l'hystérésis et l'anti-rebond |
| 11 | décisions extraites en fonctions pures de booléens (`system_status`, `*_is_measurable`) |
| 12 | matrices de couplage données/contrôle dans le SDD, dépendances explicites |
| 13 | standard de codage appliqué, **zéro déviation** |
| 14 | identité logicielle, part number, intégrité |
| 15 | arithmétique entière, cycle à WCET constant, aucune boucle non bornée |

---

## 6. Manipulation

```bash
.\build\debug\bin\demo_16-projet-integre.exe
```

```bash
.\build\debug\bin\tests_16-projet-integre.exe --verbose --req
```

```bash
.\build\debug\bin\tests_16-projet-integre-hlr.exe --verbose --req
```

Vérifier le dossier complet :

```bash
python tools/trace_check.py
```

```bash
python tools/config_index.py
```

---

## 7. Ce qui reste à faire — et c'est volontaire

`trace_check.py` signale, après ce module, **zéro défaut** mais une
**observation** : plusieurs HLR des modules 10, 11 et 12 ne sont vérifiées
qu'**indirectement**, via leurs LLR. La table A-6 demande aussi des tests
fondés sur les exigences de haut niveau.

C'est un dossier **réaliste** : il n'est jamais complet du premier coup, et ce
qui compte est de **savoir ce qui manque**. Un dossier dont on connaît
exactement les trous vaut infiniment mieux qu'un dossier qu'on croit complet.

Combler ces observations est l'exercice 8.4.

---

## 8. Exercices

**8.1 — Une exigence de plus**
Le système doit désormais estimer l'**autonomie restante** en minutes, à partir
de la quantité totale et d'un débit carburant fourni en entrée (kg/h).
Parcourez **tout** le cycle : HLR, LLR, code annoté, tests LLR, tests HLR,
mise à jour des matrices de couplage, vérification par `trace_check.py`.
Chronométrez-vous : c'est le coût réel d'une exigence en DAL B.

**8.2 — Analyse d'impact**
Le seuil bas niveau passe de 1500 à 1800 kg. Remplissez
[`fiche-anomalie.md`](../../templates/fiche-anomalie.md) section 5 : quels
artefacts sont touchés ? Combien de tests à rejouer ? Combien à réécrire ?

**8.3 — Revue de code complète**
Appliquez [`checklist-revue-code.md`](../../templates/checklist-revue-code.md)
à `src/fqms.cpp`, ligne à ligne. Notez chaque constat. Combien en trouvez-vous ?
Combien de temps pour 200 lignes ? Extrapolez à 50 000.

**8.4 — Combler les observations de traçabilité**
Écrivez les campagnes de tests fondés sur les HLR manquantes pour les modules
10, 11 et 12. Vérifiez que `trace_check.py` ne signale plus aucune observation.

**8.5 — Couverture structurelle**
Lancez `.\scripts\coverage.ps1 -Module 16-projet-integre`. Quel taux de
couverture d'instructions ? Quelles lignes ne sont pas couvertes ? Pour
chacune, appliquez §6.4.4.3 : cas de test manquant, exigence manquante, code
mort, ou code désactivé ?

**8.6 — Défendre votre travail**
Préparez une présentation de 10 minutes du FQMS, comme si vous la faisiez à un
recruteur. Structure suggérée :
1. le besoin système et l'allocation du DAL, avec sa justification ;
2. l'architecture et les décisions structurantes (DA-01 à DA-07) ;
3. la stratégie de vérification : deux campagnes, robustesse, traçabilité ;
4. **un défaut que vos tests ont trouvé**, et comment ;
5. ce qui manque au dossier, et pourquoi vous le savez.

> Le point 5 est celui qui impressionne le plus. Un candidat qui connaît les
> limites de son propre travail est un candidat qui a compris le métier.

---

## 9. Pour aller plus loin

- Rapport du Bureau de la sécurité des transports du Canada sur le vol
  Air Canada 143.
- ARP4754A et ARP4761 : d'où viennent les exigences système et l'allocation
  des DAL.
- Rejouez la formation en changeant le niveau : que faudrait-il **ajouter**
  pour passer le FQMS en DAL A ? (Réponse courte : MC/DC sur toutes les
  décisions, couverture du code objet, et davantage d'indépendance.)

---

⬅️ [15 — Déterminisme et temps réel](../15-determinisme-temps-reel/README.md) |
🏠 [Retour au sommaire](../../README.md)
