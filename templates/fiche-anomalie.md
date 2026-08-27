# Fiche d'anomalie (*Problem Report* / SCR)

> Donnée de vie du logiciel DO-178C §11.17. Catégorie de contrôle : **CC2**.
> Le suivi des anomalies est le point que Git ne couvre **pas** : c'est un
> processus à part entière, avec classification, analyse d'impact et
> approbation de clôture.

| Champ | Valeur |
|---|---|
| Numéro | SCR-____ |
| Date d'ouverture | |
| Émetteur | |
| État | ☐ ouvert ☐ analysé ☐ corrigé ☐ vérifié ☐ clos ☐ refusé |
| Baseline concernée | étiquette ou commit |
| Composant | |

---

## 1. Description

Ce qui a été observé, **factuellement**.

## 2. Reproduction

Étapes exactes, environnement (SECI), données d'entrée.

> Une anomalie non reproductible se documente **comme telle**. Elle n'est pas
> classée sans suite.

## 3. Classification

| Champ | Valeur |
|---|---|
| Type | ☐ exigence ☐ conception ☐ code ☐ test ☐ documentation ☐ outil |
| Sévérité | ☐ 1 sécurité ☐ 2 fonction majeure ☐ 3 mineure ☐ 4 cosmétique |
| Effet système potentiel | |
| Niveau DAL du composant | |

> La sévérité se juge par l'**effet système**, jamais par la difficulté de
> correction.

## 4. Analyse de cause

Cause racine. **Remonter jusqu'au processus** : une erreur de code trouve
souvent sa cause dans une exigence ambiguë ou une revue insuffisante.

## 5. Analyse d'impact (DO-178C §7.2.4)

Ce qui doit être modifié **et re-vérifié** :

- [ ] Exigences de haut niveau — lesquelles ?
- [ ] Exigences de bas niveau — lesquelles ?
- [ ] Architecture / matrices de couplage
- [ ] Code source — quels fichiers ?
- [ ] Cas de test — lesquels ajouter ou modifier ?
- [ ] Couverture structurelle — à refaire sur quel périmètre ?
- [ ] Documents — SRD, SDD, SCI, SAS
- [ ] Autres composants affectés (analyse de propagation)

## 6. Correction

Description de la correction, et **commit** qui la porte.

## 7. Test de non-régression

> **Règle** : pas de correction sans test qui aurait détecté l'anomalie.

| Cas de test ajouté | Exigence tracée | Échoue avant correction ? |
|---|---|---|
| | | ☐ oui ☐ non |

## 8. Vérification et clôture

| Rôle | Nom | Date | Visa |
|---|---|---|---|
| Correcteur | | | |
| Vérificateur (indépendant) | | | |
| Assurance qualité | | | |
