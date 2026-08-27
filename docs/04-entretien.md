# Préparation aux entretiens

> Ce que l'on vous demandera réellement, et comment y répondre en montrant que
> vous avez compris le métier — pas seulement appris des règles.

---

## 1. Le principe qui gouverne tout

> **Un code excellent sans preuve tracée ne passe pas. Un code moyen
> intégralement tracé, revu, testé et couvert passe.**

C'est le renversement culturel principal quand on vient du développement
classique. Si vous ne deviez retenir qu'une phrase, c'est celle-là — et savoir
l'illustrer.

Corollaire, à placer quand on vous parle d'une contrainte qui semble absurde :
> *« Ce n'est pas une question de qualité du code, c'est une question de ce
> qu'on peut en **démontrer**. »*

---

## 2. Questions techniques C++

### « Pourquoi les exceptions sont-elles interdites en avionique ? »

Ne répondez pas « c'est la règle ». Donnez les cinq raisons **techniques**
(module 07) :

1. **temps d'exécution non borné** — le déroulement de pile parcourt des
   tables ; aucun outil WCET ne le borne utilement, or le WCET doit être
   *démontré* ;
2. **flot de contrôle implicite** — `f(); g();` : si `f` peut lancer, `g` peut
   ne jamais s'exécuter, sans que rien ne l'indique. L'analyse de couplage de
   contrôle (A-7.8) devient très difficile ;
3. **allocation dynamique** — l'objet exception est alloué sur un tas dédié ;
4. **taille du code** — 10 à 30 % du binaire en tables de déroulement ;
5. **DO-332 vulnérabilité 6** — objectifs supplémentaires à satisfaire.

Puis montrez l'alternative : `Result<T>`, propagation explicite, tout le flot
visible.

> **Bonus** : « `noexcept` n'*empêche* pas de lancer, il *promet* de ne pas le
> faire. Si une exception s'en échappe, c'est `std::terminate` — donc un
> redémarrage du calculateur en vol. »

### « Pourquoi pas d'allocation dynamique ? »

Quatre raisons (module 08) : non-déterminisme temporel, **fragmentation**
(défaut qui apparaît après des dizaines d'heures, donc jamais en test), échec
ingérable à 10 000 m, et DO-332 OO.6.8.2.

Puis : *« quand on a vraiment besoin d'allouer, on utilise une réserve de blocs
de taille fixe — allocation en O(1), aucune fragmentation possible,
épuisement détectable. C'est ce que fait un noyau ARINC 653. »*

### « Quelle est la différence entre `volatile` et `std::atomic` ? »

Réponse courte et nette :
> *« `volatile` s'adresse au **matériel**, `std::atomic` s'adresse aux
> **autres fils d'exécution**. `volatile` garantit qu'un accès mémoire a
> réellement lieu ; il ne garantit ni l'atomicité, ni l'ordre, ni les
> barrières mémoire. »*

C'est une confusion très répandue : la trancher proprement marque des points.

### « Expliquez RAII. »

*« La durée de vie d'une ressource est celle d'un objet. »* Puis l'argument de
certification : **la libération est garantie sur tous les chemins de sortie**,
y compris les `return` anticipés. Sans RAII, un oubli sur une seule branche
d'erreur ne se voit ni en relecture rapide, ni en test nominal.

Comparez avec C# : `using` doit être **écrit** ; un destructeur, non.

### « Faut-il utiliser le polymorphisme dynamique ? »

Montrez que vous savez **arbitrer**, pas réciter :
> *« Si l'ensemble des types est connu à la compilation — ce qui est le cas de
> presque tout système embarqué certifié — le CRTP donne la même factorisation
> pour zéro coût et une vérification plus simple. Le dynamique se justifie
> surtout aux frontières matérielles, où la substitution sert vraiment. »*

Et ajoutez le coût : vptr, pas d'inlining, WCET = celui de la redéfinition la
plus lente.

---

## 3. Questions DO-178C

### « Qu'est-ce que le MC/DC, et pourquoi ? »

La définition, en quatre points (module 11), puis **l'exemple qui prouve que
vous avez compris** :

> *« Deux tests suffisent à couvrir la décision `A && B && C && D` : tout vrai,
> tout faux. Couverture de décision : 100 %. Couverture MC/DC : 0 %. Les deux
> évaluations diffèrent sur les quatre conditions à la fois — une condition
> pourrait être inversée dans le code sans qu'aucun test ne le détecte. »*

Puis le jeu minimal (N+1 tests) et le gain : 17 tests au lieu de 65 536 pour
16 conditions.

### « Que fait-on d'une exigence dérivée ? »

Deux choses, **et la seconde est celle qu'on oublie** :
1. l'identifier explicitement comme dérivée ;
2. la **transmettre au processus de sécurité système** (§5.1.2.h), qui vérifie
   qu'elle n'introduit pas un mode de panne non analysé.

> *« C'est le premier point que regarde un auditeur : une exigence dérivée non
> identifiée est un contournement de l'analyse de sécurité. »*

### « Différence entre code mort et code désactivé ? »

| | Code mort | Code désactivé |
|---|---|---|
| Définition | ne peut **jamais** s'exécuter | intentionnellement non exécutable **dans cette configuration** |
| Statut | **défaut** | acceptable |
| Action | supprimer, et analyser pourquoi il existait | identifier, démontrer qu'il ne peut pas être activé, justifier |

Ajoutez le piège : *« un `if (p == nullptr)` alors que `p` ne peut pas être nul
est une excellente pratique ailleurs, et un constat de revue ici : branche non
tracée, jamais couverte. La bonne démarche est d'écrire l'exigence de
robustesse d'abord. »*

### « Différence entre vérification et assurance qualité ? »

> *« La vérification demande : le logiciel est-il correct ? L'assurance qualité
> demande : avons-nous suivi nos plans ? La première porte sur le produit, la
> seconde sur le processus. »*

### « Quand faut-il qualifier un outil ? »

> *« La question n'est jamais “l'outil est-il bon ?” mais “son résultat
> remplace-t-il une activité que la norme exige ?”. »*

Puis les trois critères et les cinq TQL (module 14). Et l'exemple qui montre
la maîtrise :
> *« Le compilateur n'est pas qualifié, et c'est normal : on ne le qualifie
> pas, on **vérifie sa sortie**. C'est pour cela que la DO-178C exige des
> tests sur le code exécutable, pas sur le code source. »*

### « Qu'est-ce que le couplage de données et de contrôle ? »

Les deux définitions du glossaire, puis **pourquoi c'est un objectif séparé** :
> *« On peut atteindre 100 % de MC/DC sur chaque composant isolément et n'avoir
> jamais testé leur assemblage. Ordre d'appel inversé, donnée non initialisée
> au premier cycle, unité non convertie à la frontière : ces défauts n'existent
> qu'à l'intégration. »*

---

## 4. Questions de conception

### « Comment vous y prendriez-vous pour développer ce composant ? »

L'ordre compte. Montrez que vous le connaissez :

1. lire les exigences système et le niveau DAL alloué ;
2. écrire les **HLR**, les faire relire ;
3. concevoir l'architecture, écrire les **LLR** et les matrices de couplage ;
4. écrire les **cas de test** — avant ou en parallèle du code, jamais après ;
5. coder, en annotant la traçabilité ;
6. exécuter, mesurer la couverture, **résoudre** chaque non-couvert ;
7. revues, avec les checklists et l'indépendance requise ;
8. baseline, SCI, SECI.

> Le point 4 surprend souvent : *« les tests sont dérivés des exigences, pas du
> code. Un test dérivé du code reproduit les erreurs du code. »*

### « Comment testez-vous un composant à état ? »

Les quatre techniques (module 10) : classes d'équivalence, valeurs limites,
couverture des **états et des transitions**, séquences réalistes. Puis :

> *« Atteindre les quatre états ne suffit pas. Les défauts se cachent dans les
> transitions d'**annulation** — celles qui garantissent que le comptage porte
> sur des cycles consécutifs — et ce sont les plus souvent oubliées. »*

Et enfin l'analyse de mutation : *« un test qui passe ne prouve rien s'il
passerait aussi avec un code faux. »*

---

## 5. Ce que vous pouvez montrer

Ce dépôt est un **portfolio**. Quelques éléments qui parlent d'eux-mêmes :

| Élément | Ce qu'il démontre |
|---|---|
| [`tools/trace_check.py`](../tools/trace_check.py) | vous savez ce qu'est la traçabilité bidirectionnelle **et** l'outiller |
| [`modules/11/src/mcdc.cpp`](../modules/11-couverture-structurelle/src/mcdc.cpp) | vous savez ce qu'est une paire d'indépendance |
| [`modules/12/include/mod12/coupling_trace.hpp`](../modules/12-couplage-donnees-controle/include/mod12/coupling_trace.hpp) | vous savez démontrer A-7.8 sans modifier le code de production |
| [`modules/16/requirements/`](../modules/16-projet-integre/requirements/) | vous savez rédiger un SRD et un SDD |
| [`templates/`](../templates/) | vous savez à quoi ressemble une revue |
| Les `NOLINT` justifiés | vous savez instruire une déviation |

**Le plus impressionnant reste le module 16 §7** : la liste de ce qui **manque**
au dossier, et pourquoi vous le savez. Un candidat qui connaît les limites de
son propre travail est un candidat qui a compris le métier.

---

## 6. Les questions à leur poser

Elles montrent que vous connaissez le terrain :

- *« Quel DAL pour le produit sur lequel je travaillerais ? »*
- *« Quelle chaîne d'outils de couverture — VectorCAST, LDRA, autre ? Est-elle
  qualifiée ? »*
- *« Comment gérez-vous la traçabilité : DOORS, Polarion, autre ? »*
- *« Quel standard de codage — MISRA C++:2023, AUTOSAR, un standard maison ? »*
- *« Le projet est-il en IMA / ARINC 653, ou sur calculateur dédié ? »*
- *« Comment sont réparties les responsabilités entre développement et
  vérification ? Quelle indépendance ? »*
- *« Quelle est la part du temps passée à écrire du code, par rapport aux
  documents et aux revues ? »*

> Cette dernière question est excellente : la réponse honnête est souvent
> « 20 à 30 % de code ». La poser montre que vous ne vous faites pas
> d'illusions sur le métier.

---

## 7. Attentes réalistes

**Ce qu'on n'attendra pas d'un profil junior en avionique** :
- connaître par cœur les tables A-1 à A-10 ;
- avoir déjà utilisé VectorCAST ou LDRA ;
- avoir rédigé un PSAC ;
- avoir travaillé sur cible réelle.

**Ce qu'on attendra** :
- du C++ solide, et la conscience de ses pièges ;
- comprendre **pourquoi** les contraintes existent ;
- de la rigueur, et le goût de la preuve ;
- savoir écrire une exigence vérifiable ;
- de la curiosité pour le domaine.

**Ce qui vous distinguera** :
- avoir fait la démarche complète sur un projet, même petit ;
- savoir parler couplage, MC/DC et exigences dérivées sans réciter ;
- connaître les limites de ce que vous avez fait.
