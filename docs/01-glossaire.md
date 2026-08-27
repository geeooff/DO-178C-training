# Glossaire DO-178C et avionique

> Les acronymes de ce métier sont nombreux et non négociables : ils
> structurent les conversations, les documents et les entretiens. Cette page
> est faite pour être relue.

---

## Documents normatifs

| Sigle | Nom | Ce que c'est |
|---|---|---|
| **DO-178C** | *Software Considerations in Airborne Systems and Equipment Certification* | La norme logicielle. ED-12C en Europe (texte identique). |
| **DO-278A** | idem, pour les systèmes **sol** (contrôle aérien) | Même logique, monde CNS/ATM. |
| **DO-254** | *Design Assurance Guidance for Airborne Electronic Hardware* | L'équivalent pour le **matériel** programmable (FPGA, ASIC). |
| **DO-330** | *Software Tool Qualification Considerations* | Qualification des **outils**. |
| **DO-331** | Supplément *Model-Based Development* | Simulink, SCADE. |
| **DO-332** | Supplément *Object-Oriented Technology* | **C++**, Java, Ada 2012. |
| **DO-333** | Supplément *Formal Methods* | Preuve formelle, interprétation abstraite. |
| **DO-297** | *Integrated Modular Avionics Development Guidance* | IMA, partitionnement. |
| **ARP4754A** | *Guidelines for Development of Civil Aircraft and Systems* | Le **système**, en amont du logiciel. |
| **ARP4761** | *Guidelines and Methods for Conducting the Safety Assessment Process* | L'analyse de sécurité qui **alloue les DAL**. |
| **ARINC 429** | *Digital Information Transfer System* | Le bus de données historique (module 07). |
| **ARINC 653** | *Avionics Application Software Standard Interface* | Interface du noyau temps réel partitionné (module 15). |
| **MISRA C++:2023** | — | Standard de codage C++ (module 13). |
| **CAST** | *Certification Authorities Software Team* | Notes de position **publiques et gratuites** des autorités. |

---

## Niveaux et sécurité

| Sigle | Signification |
|---|---|
| **DAL** | *Design Assurance Level* — niveau A à E, alloué par l'analyse de sécurité système |
| **A** | Condition de panne **catastrophique** — perte de l'appareil |
| **B** | **Dangereuse** — blessés graves, forte réduction des marges |
| **C** | **Majeure** — inconfort, charge de travail accrue |
| **D** | **Mineure** — conséquences négligeables |
| **E** | **Sans effet** sur la sécurité |
| **FHA** | *Functional Hazard Assessment* — identifie les conditions de panne |
| **PSSA / SSA** | *Preliminary / System Safety Assessment* |
| **FDAL / IDAL** | *Function / Item Development Assurance Level* (vocabulaire ARP4754A) |

---

## Données de vie du logiciel (§11)

| Sigle | Nom | § |
|---|---|:--:|
| **PSAC** | Plan for Software Aspects of Certification | 11.1 |
| **SDP** | Software Development Plan | 11.2 |
| **SVP** | Software Verification Plan | 11.3 |
| **SCMP** | Software Configuration Management Plan | 11.4 |
| **SQAP** | Software Quality Assurance Plan | 11.5 |
| **SRS / SDS / SCS** | Software Requirements / Design / Code **Standards** | 11.6–11.8 |
| **SRD** | Software Requirements Data — les **HLR** | 11.9 |
| **SDD** | Design Description — **architecture + LLR** | 11.10 |
| **SVCP** | Software Verification Cases and Procedures | 11.13 |
| **SVR** | Software Verification Results | 11.14 |
| **SECI** | Software Life Cycle **Environment** Configuration Index | 11.15 |
| **SCI** | Software Configuration Index | 11.16 |
| **SCR / PR** | Software Change Request / Problem Report | 11.17 |
| **SAS** | Software Accomplishment Summary — le document **final** | 11.20 |

---

## Exigences et vérification

| Terme | Définition |
|---|---|
| **HLR** | *High-Level Requirements* — **ce que** le logiciel doit faire |
| **LLR** | *Low-Level Requirements* — **comment**, assez détaillé pour coder |
| **Exigence dérivée** | Exigence non traçable vers un niveau supérieur. **Doit** être identifiée et remontée au processus de sécurité (§5.1.2.h). |
| **Traçabilité bidirectionnelle** | Exigence → code → test **et** retour |
| **Test normal** | Entrées du domaine valide (§6.4.2.1) |
| **Test de robustesse** | Entrées hors domaine ou anormales (§6.4.2.2) |
| **Indépendance** | L'auteur ne peut pas être son propre vérificateur |
| **Code mort** | Code non exécutable en toute circonstance → **défaut**, à supprimer |
| **Code désactivé** | Code intentionnellement non exécutable dans cette configuration → **acceptable**, à justifier |
| **Analyse de mutation** | Introduire un défaut et vérifier qu'un test échoue |

---

## Couverture structurelle

| Terme | Définition | Requis en |
|---|---|---|
| **Statement coverage** | Chaque instruction exécutée | A, B, C |
| **Decision coverage** | Chaque décision a pris ses deux issues | A, B |
| **MC/DC** | Chaque **condition** affecte **seule** l'issue de sa décision | **A** |
| **Condition** | Expression booléenne **élémentaire** |  |
| **Décision** | Expression booléenne **complète** qui contrôle le flot |  |
| **Paire d'indépendance** | Deux jeux d'entrées où seule la condition étudiée change, et l'issue change |  |
| **Unique cause / Masking** | Les deux variantes de MC/DC acceptées |  |
| **Data coupling** | Dépendance d'un composant à des données qu'il ne contrôle pas | A, B, C |
| **Control coupling** | Manière dont un composant influence l'exécution d'un autre | A, B, C |

---

## Configuration et outils

| Terme | Définition |
|---|---|
| **Baseline** | État figé, identifié et approuvé d'un ensemble de données |
| **CC1 / CC2** | Catégories de contrôle de configuration (table 7-1) |
| **TQL** | *Tool Qualification Level*, 1 à 5 (DO-330) |
| **Critère 1** | L'outil produit du code embarqué sans vérification de sa sortie |
| **Critère 2** | L'outil automatise une vérification et pourrait rater une erreur |
| **Critère 3** | L'outil permet de réduire une autre activité |
| **Conformity review** | Revue finale avant livraison, conduite par la SQA |

---

## Temps réel et embarqué

| Terme | Définition |
|---|---|
| **WCET** | *Worst-Case Execution Time* — temps d'exécution au **pire cas** |
| **Major frame** | Trame majeure : plan d'ordonnancement cyclique complet |
| **Partition** | Unité d'isolation spatiale et temporelle (ARINC 653) |
| **Freedom from interference** | Un composant ne peut ni corrompre ni retarder un autre |
| **IMA** | *Integrated Modular Avionics* — calculateurs partagés, partitionnés |
| **BITE** | *Built-In Test Equipment* — autotests et compteurs de maintenance |
| **Health monitoring** | Surveillance ARINC 653 des dépassements et anomalies |
| **Watchdog** | Chien de garde : redémarre le calculateur s'il ne répond plus |
| **Passivation** | Mise en sécurité d'une fonction défaillante |
| **SSM** | *Sign/Status Matrix* — état d'une donnée ARINC 429 |
| **Label** | Identifiant de donnée ARINC 429, noté en **octal** |

---

## Anglais du métier

Les documents sont en anglais, les réunions souvent aussi. Quelques
correspondances utiles :

| Français | Anglais |
|---|---|
| exigence | requirement |
| exigence dérivée | derived requirement |
| traçabilité | traceability |
| couverture | coverage |
| revue | review |
| constat (de revue) | finding |
| anomalie | problem report, discrepancy |
| déviation | deviation |
| jalon | milestone |
| référentiel figé | baseline |
| chaîne de compilation | toolchain |
| pire cas | worst case |
| aux bornes | at boundary |
| domaine de validité | valid range |
| écrêtage | clamping, saturation |
| dépassement | overflow, overrun |
