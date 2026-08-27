# Fiche de déviation au standard de codage

> Une déviation n'est pas une faute : c'est une décision d'ingénierie qui doit
> être **localisée**, **justifiée**, **analysée**, **approuvée** et **tracée**.
> Une déviation *non documentée*, en revanche, est un constat de
> non-conformité.

| Champ | Valeur |
|---|---|
| Numéro | DEV-____ |
| Date | |
| Demandeur | |
| Composant | |
| Fichier(s) et ligne(s) | |
| Règle concernée | ex. R-03 : pas de macro de type fonction |
| Catégorie de la règle | ☐ mandatory ☐ required ☐ advisory |

---

## 1. Description de la déviation

Ce qui est fait, et en quoi cela s'écarte de la règle.

## 2. Portée

- Nombre d'occurrences :
- Périmètre : ☐ ligne ☐ fonction ☐ fichier ☐ cible de compilation
- La déviation est-elle isolée dans sa propre unité de compilation ? ☐ oui ☐ non

> Une déviation appliquée à **tout le projet** est presque toujours refusable.

## 3. Justification

Pourquoi la règle ne peut pas être respectée **ici**.

Motifs **recevables** :

* contrainte matérielle ou de performance démontrée, chiffres à l'appui ;
* interface imposée par un composant tiers non modifiable ;
* le respect de la règle dégraderait la vérifiabilité (cas rare, à étayer) ;
* code de test dont la déviation **est** l'objet du test.

Motifs **non recevables** : « c'est plus rapide à écrire », « c'est
l'habitude », « le code existant fait déjà comme ça », « l'outil se trompe ».

## 4. Analyse de risque

| Question | Réponse |
|---|---|
| Quel défaut la règle prévient-elle ? | |
| Ce défaut est-il possible ici ? | |
| Quel serait son effet au niveau système ? | |
| Un objectif DO-178C est-il affecté ? | |

## 5. Mesures compensatoires

Ce qui est mis en place à la place de la règle : revue renforcée, cas de test
supplémentaire, assertion, analyse statique ciblée, commentaire de mise en
garde.

## 6. Marquage dans le code

La déviation doit être visible **à l'endroit exact** où elle s'applique :

```cpp
// DEVIATION DEV-____ (règle R-__) : <justification en une phrase>
// NOLINTNEXTLINE(<vérificateur>)
```

## 7. Approbation

| Rôle | Nom | Date | Visa |
|---|---|---|---|
| Demandeur | | | |
| Responsable technique | | | |
| Assurance qualité logicielle | | | |

## 8. Revue périodique

| Date | Toujours nécessaire ? | Commentaire |
|---|---|---|
| | ☐ oui ☐ non | |
