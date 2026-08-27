# Ressources

> Priorité aux documents **gratuits et publics** : il y en a plus qu'on ne
> croit, et ce sont souvent les meilleurs.

---

## 1. Gratuit et indispensable

### Les papiers CAST

Les *Certification Authorities Software Team position papers* sont publics,
courts (5 à 20 pages), et écrits par les autorités elles-mêmes. **C'est le
meilleur point d'entrée dans la culture du domaine.**

| Papier | Sujet | Module |
|---|---|---|
| **CAST-6** | *Rationale for Accepting Masking MC/DC* | 11 |
| **CAST-10** | *What is a "Decision" in Application of MC/DC* | 11 |
| **CAST-12** | *Guidelines for Approving Source Code to Object Code Traceability* | 11 |
| **CAST-13** | *Automatic Code Generation Tools Development Assurance* | 14 |
| **CAST-17** | *Structural Coverage of Object Code* | 11 |
| **CAST-19** | *Clarification of Structural Coverage Analyses of Data Coupling and Control Coupling* | **12** |
| **CAST-32A** | *Multi-core Processors* | 15 |

> **CAST-19 est à lire avant tout entretien** portant sur le couplage.

### NASA

| Document | Pourquoi |
|---|---|
| **NASA/TM-2001-210876** — *A Practical Tutorial on Modified Condition/Decision Coverage* (Hayhurst et al.) | **85 pages, gratuit, et la meilleure introduction au MC/DC qui existe.** Si vous ne lisez qu'un document, c'est celui-là. |
| *NASA Software Safety Guidebook* (NASA-GB-8719.13) | vision complète de la sûreté logicielle |
| *Mars Climate Orbiter Mishap Investigation Board Phase I Report* | l'accident du module 04 |

### Autorités

| Document | Source |
|---|---|
| **FAA AC 20-115D** | reconnaissance de la DO-178C par la FAA |
| **EASA CM-SWCEH-002** — *Software Aspects of Certification* | position détaillée de l'autorité européenne |
| **FAA Order 8110.49** | *Software Approval Guidelines* |

### Standards de codage

| Document | Note |
|---|---|
| **JSF++ Air Vehicle C++ Coding Standards** (Lockheed Martin, 2005) | **public et gratuit** — la seule référence libre du domaine. Datée (C++03) mais les sections sur les exceptions et l'héritage restent excellentes. |
| **C++ Core Guidelines** — <https://isocpp.github.io/CppCoreGuidelines/> | gratuit, moderne, base de la moitié des règles clang-tidy |

### Numérique

| Document | Note |
|---|---|
| David Goldberg, *What Every Computer Scientist Should Know About Floating-Point Arithmetic* (1991) | gratuit, **la** référence sur IEEE-754 |
| *Rapport de la commission d'enquête sur le vol 501 d'Ariane 5* (J.-L. Lions, 1996) | dix pages, à lire une fois dans sa carrière |

---

## 2. Payant — à demander à votre employeur

Tous ces documents s'achètent auprès de la **RTCA** (États-Unis) ou de
l'**EUROCAE** (Europe). Comptez quelques centaines d'euros pièce. **Aucun
employeur du secteur ne s'attend à ce que vous les ayez achetés vous-même.**

| Document | Priorité |
|---|---|
| **DO-178C / ED-12C** | ★★★ |
| **DO-332** (orienté objet) | ★★★ si vous faites du C++ |
| **DO-330** (qualification des outils) | ★★ |
| **ARP4754A** et **ARP4761** (SAE) | ★★ pour comprendre d'où viennent les DAL |
| **DO-297** (IMA) | ★ |
| **MISRA C++:2023** | ★★★ |
| **ARINC 429**, **ARINC 653** | ★★ |

---

## 3. Livres C++

| Livre | Pour quoi |
|---|---|
| Scott Meyers, **Effective Modern C++** | les 42 items qui font la différence en C++11/14. Le meilleur investissement. |
| Bjarne Stroustrup, **A Tour of C++** (3ᵉ éd.) | tour d'horizon rapide et fiable, idéal en venant d'un autre langage |
| Nicolai Josuttis, **C++17 — The Complete Guide** | référence sur les nouveautés de notre norme cible |
| Jason Turner, **C++ Best Practices** | court, pratique, orienté outillage |
| Christopher Kormanyos, **Real-Time C++** | **le** livre C++ embarqué : registres, `constexpr`, pas d'allocation |

---

## 4. Livres et articles domaine

| Ouvrage | Note |
|---|---|
| Leanna Rierson, **Developing Safety-Critical Software** (CRC Press) | **la** référence pratique sur la DO-178. Écrit par une ancienne de la FAA. Si vous n'achetez qu'un livre du domaine, c'est celui-là. |
| Vance Hilderman, **Avionics Certification** | plus court, orienté gestion de projet |
| Nancy Leveson, **Engineering a Safer World** | prend de la hauteur sur la sécurité des systèmes ; change la façon de penser |

---

## 5. En ligne

| Ressource | Note |
|---|---|
| <https://en.cppreference.com/> | **la** référence C++. Précise, à jour, gratuite. |
| <https://compiler-explorer.com/> (Godbolt) | voir l'assembleur généré. Irremplaçable pour comprendre le coût réel d'une abstraction. |
| <https://clang.llvm.org/extra/clang-tidy/checks/> | documentation de chaque vérificateur |
| <https://github.com/OpenCppCoverage/OpenCppCoverage> | l'outil de couverture du module 11 |
| Chaîne YouTube **CppCon** | conférences ; cherchez « embedded », « safety critical », « constexpr » |

---

## 6. Un parcours de lecture

Si vous voulez approfondir après cette formation, dans cet ordre :

1. **NASA/TM-2001-210876** (MC/DC) — gratuit, 2 heures ;
2. **CAST-19** (couplage) — gratuit, 30 minutes ;
3. **JSF++** sections exceptions et héritage — gratuit, 1 heure ;
4. **Goldberg** (flottants) — gratuit, 2 heures ;
5. **Rierson**, *Developing Safety-Critical Software* — quelques semaines ;
6. **Meyers**, *Effective Modern C++* — en parallèle, un item par jour ;
7. **DO-178C** elle-même, quand votre employeur vous la fournira.

---

## 8. Trouver un poste

**Les employeurs en France** — équipementiers et avionneurs : Airbus, Thales,
Safran, Dassault Aviation, Liebherr Aerospace, Collins Aerospace, Honeywell,
Latécoère.
**Les sociétés de service** spécialisées recrutent beaucoup et forment :
Sopra Steria, Capgemini Engineering, Alten, Akka/Modis, Expleo, Sogeti,
Segula, ainsi que des structures plus petites très spécialisées.

**Les bassins d'emploi** : Toulouse d'abord, puis Bordeaux, Paris–Saclay,
Marseille, Nantes, Grenoble.

**Ce qui est valorisé chez un profil venant du C#** :
- la rigueur et le goût de la traçabilité — vous en avez plus que vous ne le
  croyez ;
- l'expérience du test automatisé ;
- la capacité à lire et écrire de la documentation ;
- et, précisément, le fait d'avoir fait la démarche de vous former au domaine.

> Les sociétés de service sont souvent la meilleure porte d'entrée : elles
> forment, et l'on y voit plusieurs programmes en peu d'années.
