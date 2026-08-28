#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
trace_check.py -- verificateur de tracabilite exigences <-> code <-> tests.

CONTEXTE DO-178C
----------------
La norme exige une tracabilite BIDIRECTIONNELLE entre les exigences de haut
niveau, les exigences de bas niveau, le code source et les cas de test
(objectifs A-3.6, A-4.6, A-5.5 et tables A-6/A-7). Le sens "descendant"
(exigence -> code) montre que tout ce qui etait demande est fait. Le sens
"remontant" (code -> exigence) montre qu'il n'y a rien DE PLUS que ce qui etait
demande : c'est celui qui revele le code non justifie, donc le code mort et les
fonctions ajoutees "au cas ou".

Cet outil reconstruit les deux sens et signale quatre defauts :

    1. exigence SANS code   -> exigence non implementee
    2. exigence SANS test   -> exigence non verifiee
    3. code SANS exigence   -> code non justifie (code mort ? exigence oubliee ?)
    4. test SANS exigence   -> test orphelin
    5. documentation citant un cas de test INEXISTANT -> reference pourrie

Le defaut 5 merite un mot. Les documents d'exigences citent leurs cas de test
dans le champ "Verification", et les README de module dans leur colonne
"Verifiee par". Ces references croisees sont ecrites A LA MAIN. Une reference
croisee que personne ne verifie POURRIT : il suffit de renommer un cas de test
pour que le document continue a citer un nom qui n'existe plus, sans que rien
ne le signale. La matrice a alors l'air complete, mais elle designe du vide.

STATUT DE QUALIFICATION (DO-330)
--------------------------------
Cet outil est un OUTIL DE VERIFICATION au sens de la DO-178C 12.2 : son
resultat pourrait servir a eliminer une revue manuelle de la matrice de
tracabilite. Dans ce cas, il releverait du TQL-5 et devrait etre qualifie
(Tool Operational Requirements, tests de l'outil, verification).

Dans le cadre de cette formation, il est utilise en COMPLEMENT de la revue
manuelle, jamais a sa place : la qualification n'est donc pas requise.
Cette distinction est exactement celle que la DO-330 demande d'expliciter.

FORMATS RECONNUS
----------------
Exigences (fichiers modules/*/requirements/*.md) :

    ### LLR-ADCALT-010
    - **Type** : LLR
    - **Parent** : HLR-ADCALT-002, HLR-ADCALT-003
    - **Enonce** : ...

Code source (.hpp / .cpp) :

    /// @satisfies LLR-ADCALT-010

Tests :

    TEST_REQ(Suite, nom_du_cas, "LLR-ADCALT-010,LLR-ADCALT-021")

USAGE
-----
    python tools/trace_check.py                 # rapport console
    python tools/trace_check.py --csv rapport.csv
    python tools/trace_check.py --strict        # code de retour 1 si defaut
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# --- Motifs de reconnaissance -------------------------------------------------

# Un identifiant d'exigence : PREFIXE-COMPOSANT-NUMERO
RE_REQ_ID = re.compile(r"\b((?:HLR|LLR|SYS|DER)-[A-Z0-9]+-\d+)\b")

RE_REQ_HEADER = re.compile(r"^###\s+((?:HLR|LLR|SYS|DER)-[A-Z0-9]+-\d+)\s*$")
RE_REQ_FIELD = re.compile(r"^\s*-\s+\*\*(\w+)\*\*\s*:\s*(.*)$")

RE_SATISFIES = re.compile(r"@satisfies\s+([A-Za-z0-9\-,\s]+)")
RE_TEST_REQ = re.compile(r'TEST_REQ\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*"([^"]*)"\s*\)')
RE_TEST_PLAIN = re.compile(r"^\s*TEST\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)")

SOURCE_SUFFIXES = {".hpp", ".cpp", ".h", ".cc"}


@dataclass
class Requirement:
    identifier: str
    kind: str = ""
    parents: list[str] = field(default_factory=list)
    statement: str = ""
    source_file: str = ""
    line: int = 0
    derived: bool = False

    # Remplis pendant l'analyse
    code_sites: list[str] = field(default_factory=list)
    test_cases: list[str] = field(default_factory=list)


@dataclass
class Findings:
    requirements: dict[str, Requirement] = field(default_factory=dict)
    unknown_in_code: list[tuple[str, str]] = field(default_factory=list)
    unknown_in_tests: list[tuple[str, str]] = field(default_factory=list)
    untraced_tests: list[str] = field(default_factory=list)

    # Noms de cas de test cites dans la documentation mais introuvables dans
    # le code : (fichier markdown, ligne, nom cite).
    stale_doc_refs: list[tuple[str, int, str]] = field(default_factory=list)

    # Ensemble des cas de test reellement declares, sous la forme "Suite.nom",
    # et des suites, pour resoudre les references a motif "Suite.*".
    known_cases: set[str] = field(default_factory=set)
    known_suites: set[str] = field(default_factory=set)


# --- Lecture des exigences ----------------------------------------------------

def parse_requirement_files(root: Path) -> dict[str, Requirement]:
    """Lit tous les fichiers d'exigences du depot."""
    requirements: dict[str, Requirement] = {}

    for path in sorted(root.glob("modules/*/requirements/*.md")):
        current: Requirement | None = None
        relative = path.relative_to(root).as_posix()

        for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            header = RE_REQ_HEADER.match(raw)
            if header:
                identifier = header.group(1)
                current = Requirement(identifier=identifier, source_file=relative, line=number)
                if identifier in requirements:
                    print(f"ATTENTION : exigence en double : {identifier}")
                requirements[identifier] = current
                continue

            if current is None:
                continue

            field_match = RE_REQ_FIELD.match(raw)
            if not field_match:
                continue

            name = field_match.group(1).lower()
            value = field_match.group(2).strip()

            if name == "type":
                current.kind = value
            elif name == "parent":
                parents = RE_REQ_ID.findall(value)
                current.parents = parents
                # Une exigence sans parent est DERIVEE : la DO-178C impose de
                # l'identifier comme telle et de la remonter au processus de
                # securite systeme (5.1.2.h).
                current.derived = len(parents) == 0
            elif name in ("enonce", "énoncé", "enonce"):
                current.statement = value

    return requirements


# --- Lecture du code et des tests ---------------------------------------------

def is_test_file(path: Path) -> bool:
    return "tests" in path.parts or path.name.startswith("test_")


def traced_modules(root: Path) -> list[Path]:
    """Modules situes DANS le perimetre de tracabilite.

    Le perimetre est defini par la presence d un repertoire requirements/ :
    c est une decision de configuration, pas une convention implicite. Les
    modules pedagogiques qui n en ont pas restent hors perimetre, et leurs
    identifiants LLR-Mxx-nnn ne sont donc pas signales comme inconnus.
    """
    return sorted(p.parent for p in root.glob("modules/*/requirements"))


def scan_sources(root: Path, findings: Findings, scope: list[Path] | None) -> None:
    """Parcourt le code source et les tests du perimetre."""
    for path in sorted(root.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES:
            continue
        if "build" in path.parts or ".git" in path.parts:
            continue
        if scope is not None and not any(
            scoped == path or scoped in path.parents for scoped in scope
        ):
            continue

        relative = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        testing = is_test_file(path)

        for number, raw in enumerate(text.splitlines(), start=1):
            site = f"{relative}:{number}"

            # --- annotations @satisfies (code de production) -------------------
            for match in RE_SATISFIES.finditer(raw):
                for identifier in RE_REQ_ID.findall(match.group(1)):
                    requirement = findings.requirements.get(identifier)
                    if requirement is None:
                        findings.unknown_in_code.append((identifier, site))
                    else:
                        requirement.code_sites.append(site)

            # --- cas de test traces --------------------------------------------
            for match in RE_TEST_REQ.finditer(raw):
                suite, name, ids = match.group(1), match.group(2), match.group(3)
                case_name = f"{suite}.{name}"
                findings.known_cases.add(case_name)
                findings.known_suites.add(suite)
                found = RE_REQ_ID.findall(ids)
                if not found:
                    findings.untraced_tests.append(f"{case_name} ({site})")
                for identifier in found:
                    requirement = findings.requirements.get(identifier)
                    if requirement is None:
                        findings.unknown_in_tests.append((identifier, f"{case_name} @ {site}"))
                    else:
                        requirement.test_cases.append(case_name)

            # --- cas de test NON traces -----------------------------------------
            if testing:
                plain = RE_TEST_PLAIN.match(raw)
                if plain:
                    findings.untraced_tests.append(
                        f"{plain.group(1)}.{plain.group(2)} ({site})"
                    )


# --- Verification des references documentaires --------------------------------

# Un nom de cas de test cite dans la documentation : `Suite.nom_du_cas` ou
# `Suite.*` pour designer toute une suite. La majuscule initiale de la suite et
# la minuscule initiale du cas distinguent ces citations d'un nom de fichier
# (`build_info.cpp`) ou d'un identifiant qualifie (`mod07::Result`).
RE_DOC_TESTREF = re.compile(r"`([A-Z][A-Za-z0-9_]*)\.([a-z_][a-z0-9_]*\*?|\*)`")

# `CMakeLists.txt` ou `SCI.md` ont la meme forme qu un nom de cas de test.
# On ecarte les extensions de fichier plutot que de compliquer la regex.
EXTENSIONS_FICHIER = {
    "txt", "md", "cpp", "hpp", "h", "cc", "py", "sh", "ps1", "json", "yml",
    "yaml", "cmake", "exe", "info", "html", "csv", "cov", "obj", "lib", "sln",
}


def check_doc_references(root: Path, findings: Findings, scope: list[Path] | None) -> None:
    """Verifie que les noms de cas de test cites dans la documentation existent.

    POURQUOI CETTE VERIFICATION EXISTE
    ----------------------------------
    Les documents d'exigences citent leurs cas de test dans le champ
    "Verification", et les README de module dans leur colonne "Verifiee par".
    Ces references croisees sont ecrites A LA MAIN.

    Une reference croisee que personne ne verifie est une reference croisee qui
    POURRIT. Il suffit de renommer un cas de test pour que le document continue
    a citer un nom qui n'existe plus -- sans que rien ne le signale, et en
    donnant l'illusion d'une tracabilite intacte.

    C'est exactement le genre de defaut qu'un auditeur cherche : la matrice de
    tracabilite a l'air complete, mais elle designe du vide.
    """
    for path in sorted(root.glob("modules/*/**/*.md")):
        if scope is not None and not any(
            scoped in path.parents for scoped in scope
        ):
            continue

        relative = path.relative_to(root).as_posix()
        for number, raw in enumerate(
            path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1
        ):
            for match in RE_DOC_TESTREF.finditer(raw):
                suite, cas = match.group(1), match.group(2)
                if cas in EXTENSIONS_FICHIER:
                    continue  # nom de fichier, pas un cas de test
                if cas == "*":
                    # `Suite.*` : la suite doit exister.
                    if suite not in findings.known_suites:
                        findings.stale_doc_refs.append((relative, number, f"{suite}.*"))
                elif cas.endswith("*"):
                    # `Suite.prefixe_*` : au moins un cas doit correspondre.
                    prefixe = f"{suite}.{cas[:-1]}"
                    if not any(c.startswith(prefixe) for c in findings.known_cases):
                        findings.stale_doc_refs.append(
                            (relative, number, f"{suite}.{cas}")
                        )
                else:
                    nom = f"{suite}.{cas}"
                    if nom not in findings.known_cases:
                        findings.stale_doc_refs.append((relative, number, nom))



# --- Rapport ------------------------------------------------------------------

def print_report(findings: Findings) -> int:
    requirements = findings.requirements
    defects = 0

    llr = {k: v for k, v in requirements.items() if v.kind.upper() == "LLR"}
    hlr = {k: v for k, v in requirements.items() if v.kind.upper() == "HLR"}

    print("=" * 78)
    print("  RAPPORT DE TRACABILITE")
    print("=" * 78)
    print(f"  exigences de haut niveau (HLR) : {len(hlr)}")
    print(f"  exigences de bas niveau  (LLR) : {len(llr)}")
    print(f"  total                          : {len(requirements)}")

    derived = [r for r in requirements.values() if r.derived]
    print(f"  dont exigences DERIVEES        : {len(derived)}")
    for requirement in derived:
        print(f"      {requirement.identifier}  ({requirement.source_file}:{requirement.line})")
    if derived:
        print("      -> a remonter au processus de securite systeme (DO-178C 5.1.2.h)")

    # --- matrice ---------------------------------------------------------------
    print()
    print("-" * 78)
    print(f"  {'EXIGENCE':<22}{'CODE':>6}{'TESTS':>7}   CAS DE TEST")
    print("-" * 78)
    for identifier in sorted(requirements):
        requirement = requirements[identifier]
        cases = ", ".join(sorted(set(requirement.test_cases)))
        print(
            f"  {identifier:<22}{len(requirement.code_sites):>6}"
            f"{len(set(requirement.test_cases)):>7}   {cases[:38]}"
        )

    # --- defauts ---------------------------------------------------------------
    print()
    print("-" * 78)
    print("  DEFAUTS DETECTES")
    print("-" * 78)

    # 1. LLR sans code
    sans_code = [r for r in llr.values() if not r.code_sites]
    if sans_code:
        defects += len(sans_code)
        print(f"  [1] {len(sans_code)} exigence(s) de bas niveau SANS code (@satisfies) :")
        for requirement in sans_code:
            print(f"      {requirement.identifier}  -- non implementee ?")
    else:
        print("  [1] toutes les LLR sont implementees (annotation @satisfies presente)")

    # 2. exigence sans test
    #
    #    Nuance importante : une HLR peut etre verifiee INDIRECTEMENT si toutes
    #    les LLR qui la couvrent sont elles-memes testees. La DO-178C demande
    #    neanmoins des tests bases sur les HLR (table A-6, objectifs 1 et 2) :
    #    on distingue donc le DEFAUT (aucune verification possible) de
    #    l'OBSERVATION (verification indirecte seulement).
    sans_test_direct = [
        r for r in requirements.values() if not r.test_cases and r.kind.upper() != "SYS"
    ]
    defauts_test = []
    observations_test = []
    for requirement in sans_test_direct:
        if requirement.kind.upper() == "HLR":
            enfants = [v for v in llr.values() if requirement.identifier in v.parents]
            if enfants and all(v.test_cases for v in enfants):
                observations_test.append(requirement)
                continue
        defauts_test.append(requirement)

    if defauts_test:
        defects += len(defauts_test)
        print(f"  [2] {len(defauts_test)} exigence(s) SANS aucune verification :")
        for requirement in defauts_test:
            print(f"      {requirement.identifier}  ({requirement.kind})")
    else:
        print("  [2] toute exigence est verifiee, directement ou via ses LLR")

    if observations_test:
        print(f"      OBSERVATION : {len(observations_test)} HLR verifiee(s) seulement")
        print("      INDIRECTEMENT, via leurs LLR. La table A-6 demande aussi des")
        print("      tests bases sur les exigences de HAUT niveau :")
        for requirement in observations_test:
            print(f"        {requirement.identifier}")

    # 3. code referencant une exigence inconnue
    if findings.unknown_in_code:
        defects += len(findings.unknown_in_code)
        print(f"  [3] {len(findings.unknown_in_code)} reference(s) a une exigence INCONNUE :")
        for identifier, site in findings.unknown_in_code:
            print(f"      {identifier}  a {site}")
    else:
        print("  [3] aucune reference a une exigence inconnue dans le code")

    # 4. tests orphelins
    orphelins = findings.untraced_tests + [
        f"{case} -> {identifier} inconnue" for identifier, case in findings.unknown_in_tests
    ]
    if orphelins:
        defects += len(orphelins)
        print(f"  [4] {len(orphelins)} cas de test SANS exigence valide :")
        for entry in orphelins:
            print(f"      {entry}")
    else:
        print("  [4] tous les cas de test sont traces a une exigence existante")

    # --- coherence HLR -> LLR --------------------------------------------------
    # 5. references documentaires perimees
    if findings.stale_doc_refs:
        defects += len(findings.stale_doc_refs)
        print(
            f"  [5] {len(findings.stale_doc_refs)} reference(s) documentaire(s) vers un "
            "cas de test INEXISTANT :"
        )
        for fichier, ligne, nom in findings.stale_doc_refs:
            print(f"      {fichier}:{ligne}  cite {nom}")
        print("      -> la documentation designe du vide : renommage non repercute ?")
    else:
        print("  [5] toutes les references documentaires designent un cas de test existant")

    print()
    print("-" * 78)
    print("  COUVERTURE DES HLR PAR LES LLR")
    print("-" * 78)
    for identifier in sorted(hlr):
        enfants = sorted(k for k, v in llr.items() if identifier in v.parents)
        if enfants:
            print(f"  {identifier:<22} <- {', '.join(enfants)}")
        else:
            defects += 1
            print(f"  {identifier:<22} <- AUCUNE LLR  *** DEFAUT ***")

    print()
    print("=" * 78)
    if defects == 0:
        print("  RESULTAT : aucun defaut de tracabilite detecte.")
    else:
        print(f"  RESULTAT : {defects} defaut(s) de tracabilite a traiter.")
    print("=" * 78)
    return defects


def write_csv(findings: Findings, path: Path) -> None:
    # Le repertoire de destination n'existe pas forcement : `reports/` est
    # ignore par Git, donc absent de tout depot fraichement clone -- ce qui
    # est exactement le cas d'une machine d'integration continue.
    if path.parent != Path(""):
        path.parent.mkdir(parents=True, exist_ok=True)

    lignes = ["exigence;type;derivee;parents;sites_code;cas_de_test"]
    for identifier in sorted(findings.requirements):
        requirement = findings.requirements[identifier]
        lignes.append(
            ";".join(
                [
                    identifier,
                    requirement.kind,
                    "oui" if requirement.derived else "non",
                    "|".join(requirement.parents),
                    "|".join(requirement.code_sites),
                    "|".join(sorted(set(requirement.test_cases))),
                ]
            )
        )
    path.write_text("\n".join(lignes) + "\n", encoding="utf-8")
    print(f"\nMatrice exportee vers {path}")


def main() -> int:
    # La console Windows utilise cp1252 par defaut : on force l'UTF-8 pour que
    # les accents et les symboles des rapports s'affichent correctement.
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, OSError):
        pass

    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    parser.add_argument("--root", default=".", help="racine du depot")
    parser.add_argument("--csv", help="exporte la matrice au format CSV")
    parser.add_argument(
        "--strict", action="store_true", help="code de retour non nul si un defaut est detecte"
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="scanne TOUT le depot, y compris les modules hors perimetre de tracabilite",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not (root / "modules").is_dir():
        print(f"ERREUR : {root} ne ressemble pas a la racine du depot (pas de modules/).")
        return 2

    findings = Findings(requirements=parse_requirement_files(root))
    if not findings.requirements:
        print("Aucune exigence trouvee dans modules/*/requirements/*.md")
        return 2

    scope = None if args.all else traced_modules(root)
    if scope is not None:
        print("Perimetre de tracabilite :")
        for module in scope:
            print(f"    {module.relative_to(root).as_posix()}")
        print()

    scan_sources(root, findings, scope)
    check_doc_references(root, findings, scope)
    defects = print_report(findings)

    if args.csv:
        write_csv(findings, Path(args.csv))

    return 1 if (args.strict and defects > 0) else 0


if __name__ == "__main__":
    sys.exit(main())
