#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
config_index.py -- generation du SCI et du SECI a partir du depot Git.

CONTEXTE DO-178C
----------------
La section 11 de la DO-178C decrit deux documents que tout dossier de
certification doit contenir :

  * SCI  -- Software Configuration Index (11.16)
            La liste EXACTE de ce qui constitue le logiciel : chaque fichier,
            sa version, son empreinte. C'est ce qui permet d'affirmer "ce
            binaire est bien celui qui a ete verifie", et de le reconstruire
            a l'identique dans quinze ans.

  * SECI -- Software Life Cycle Environment Configuration Index (11.15)
            La liste EXACTE de l'environnement de production : compilateur et
            sa version, options, editeur de liens, systeme hote, outils de
            test et d'analyse.

Ces deux documents sont classes CC1 quel que soit le niveau DAL : sans eux,
le produit n'est ni identifiable ni reproductible.

CE QUE FAIT CET OUTIL
---------------------
Il produit les deux documents au format Markdown a partir de l'etat reel du
depot : commit, etiquette, empreintes SHA-256 de chaque fichier suivi,
versions des outils detectes.

STATUT DE QUALIFICATION (DO-330)
--------------------------------
Cet outil produit une DONNEE DE VIE DU LOGICIEL. Il n'elimine, ne reduit ni
n'automatise aucune activite de VERIFICATION : le SCI qu'il genere est relu et
approuve. Il ne requiert donc PAS de qualification.
S'il servait a demontrer l'integrite du chargement sans autre verification, il
deviendrait un outil de verification (TQL-5).

USAGE
-----
    python tools/config_index.py
    python tools/config_index.py --output reports/SCI.md
    python tools/config_index.py --part-number PN-7654321-002 --version 1.0.0
"""

from __future__ import annotations

import argparse
import hashlib
import platform
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

# Fichiers exclus du perimetre de configuration du LOGICIEL : ils appartiennent
# au projet, pas au produit charge dans l'equipement.
EXCLUDED_PREFIXES = ("build/", "reports/", ".vs/")


def run_git(root: Path, *args: str) -> str:
    """Execute une commande git et renvoie sa sortie, ou une chaine vide."""
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=str(root),
            capture_output=True,
            text=True,
            check=False,
            encoding="utf-8",
            errors="replace",
        )
    except OSError:
        return ""
    return result.stdout.strip() if result.returncode == 0 else ""


def tool_version(command: list[str], pattern: str = "") -> str:
    """Recupere la version d'un outil, ou 'non detecte'."""
    try:
        result = subprocess.run(
            command, capture_output=True, text=True, check=False, encoding="utf-8",
            errors="replace"
        )
    except OSError:
        return "non detecte"
    if result.returncode != 0 and not result.stdout and not result.stderr:
        return "non detecte"

    sortie = (result.stdout + result.stderr).strip()
    if pattern:
        found = re.search(pattern, sortie)
        if found:
            return found.group(0)
    return sortie.splitlines()[0].strip() if sortie else "non detecte"


def tracked_files(root: Path) -> list[str]:
    listing = run_git(root, "ls-files")
    if not listing:
        return []
    fichiers = [line for line in listing.splitlines() if line]
    return [f for f in fichiers if not f.startswith(EXCLUDED_PREFIXES)]


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for bloc in iter(lambda: handle.read(65536), b""):
            digest.update(bloc)
    return digest.hexdigest()


def build_sci(root: Path, part_number: str, version: str) -> str:
    commit = run_git(root, "rev-parse", "HEAD") or "(hors depot Git)"
    commit_court = commit[:12] if commit != "(hors depot Git)" else commit
    branche = run_git(root, "rev-parse", "--abbrev-ref", "HEAD") or "(inconnue)"
    etiquette = run_git(root, "describe", "--tags", "--always", "--dirty") or "(aucune)"
    date_commit = run_git(root, "log", "-1", "--format=%cI") or "(inconnue)"
    modifie = bool(run_git(root, "status", "--porcelain"))
    horodatage = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")

    fichiers = tracked_files(root)
    lignes: list[str] = []
    empreinte_globale = hashlib.sha256()

    for relatif in sorted(fichiers):
        chemin = root / relatif
        if not chemin.is_file():
            continue
        empreinte = sha256_of(chemin)
        taille = chemin.stat().st_size
        empreinte_globale.update(relatif.encode("utf-8"))
        empreinte_globale.update(empreinte.encode("ascii"))
        lignes.append(f"| `{relatif}` | {taille} | `{empreinte[:16]}…` |")

    sortie: list[str] = []
    sortie.append("# SCI — Software Configuration Index")
    sortie.append("")
    sortie.append("> Document DO-178C §11.16. Catégorie de contrôle : **CC1** à tous les niveaux.")
    sortie.append("> Généré automatiquement par `tools/config_index.py`.")
    sortie.append("> **Ce document doit être relu et approuvé avant baseline.**")
    sortie.append("")
    sortie.append("## 1. Identification du produit")
    sortie.append("")
    sortie.append("| Élément | Valeur |")
    sortie.append("|---|---|")
    sortie.append(f"| Part number | `{part_number}` |")
    sortie.append(f"| Version | `{version}` |")
    sortie.append(f"| Commit Git | `{commit}` |")
    sortie.append(f"| Commit (court) | `{commit_court}` |")
    sortie.append(f"| Branche | `{branche}` |")
    sortie.append(f"| Étiquette | `{etiquette}` |")
    sortie.append(f"| Date du commit | {date_commit} |")
    sortie.append(f"| Arbre de travail modifié | **{'OUI — NON BASELINABLE' if modifie else 'non'}** |")
    sortie.append(f"| Empreinte globale (SHA-256) | `{empreinte_globale.hexdigest()}` |")
    sortie.append(f"| Date de génération | {horodatage} |")
    sortie.append("")

    if modifie:
        sortie.append("> ⚠️ **L'arbre de travail contient des modifications non validées.**")
        sortie.append("> Un SCI ne peut être établi que sur un état figé. Validez ou")
        sortie.append("> annulez les modifications, puis régénérez.")
        sortie.append("")

    sortie.append("## 2. Éléments de configuration")
    sortie.append("")
    sortie.append(f"{len(lignes)} fichier(s) sous contrôle de configuration.")
    sortie.append("")
    sortie.append("| Fichier | Taille (o) | SHA-256 (tronqué) |")
    sortie.append("|---|---:|---|")
    sortie.extend(lignes)
    sortie.append("")
    sortie.append("## 3. Procédure de reconstruction")
    sortie.append("")
    sortie.append("```bash")
    sortie.append(f"git clone <url-du-depot> && git checkout {commit_court}")
    sortie.append("```")
    sortie.append("")
    sortie.append("```bash")
    sortie.append(".\\scripts\\build.ps1 -Preset strict -Test")
    sortie.append("```")
    sortie.append("")
    return "\n".join(sortie)


def distribution_hote() -> str:
    """Identifie precisement le systeme hote, quelle que soit la plateforme."""
    base = f"{platform.system()} {platform.release()} ({platform.machine()})"

    # Sous Linux, la version du noyau ne suffit pas : c est la DISTRIBUTION qui
    # determine les versions de la chaine d outils.
    os_release = Path("/etc/os-release")
    if os_release.is_file():
        try:
            for ligne in os_release.read_text(encoding="utf-8").splitlines():
                if ligne.startswith("PRETTY_NAME="):
                    nom = ligne.split("=", 1)[1].strip().strip('"')
                    return f"{nom} ({platform.machine()}), noyau {platform.release()}"
        except OSError:
            pass

    # Sous WSL, le noyau porte la marque "microsoft". C est une information qui
    # compte : l environnement n est pas un Linux natif.
    if "microsoft" in platform.release().lower():
        base += " [WSL]"
    return base


def build_seci(root: Path) -> str:
    """Construit le SECI en s adaptant a la plateforme hote.

    L inventaire n est pas le meme sous Windows et sous Linux ou macOS. Le
    document doit refleter l environnement REEL, pas un modele suppose : un
    SECI qui annonce Visual Studio sur une machine Ubuntu est un SECI FAUX, et
    un SECI faux est pire que pas de SECI du tout.
    """
    outils = [("Systeme hote", distribution_hote())]

    if platform.system() == "Windows":
        vswhere = Path(
            r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
        )
        vs_path = ""
        if vswhere.exists():
            vs_path = tool_version([str(vswhere), "-latest", "-property", "installationPath"])
        outils.append(("Visual Studio", vs_path or "non detecte"))
        outils.append(("MSVC (cl.exe)", tool_version(["cl"], r"\d+\.\d+\.\d+")))
    else:
        outils.append(("GCC (g++)", tool_version(["g++", "--version"], r"\d+\.\d+\.\d+")))
        outils.append(
            ("Clang (clang++)", tool_version(["clang++", "--version"], r"\d+\.\d+\.\d+"))
        )
        outils.append(("Editeur de liens (ld)", tool_version(["ld", "--version"], r"\d+\.\d[\d.]*")))

    outils += [
        ("CMake", tool_version(["cmake", "--version"], r"\d+\.\d+\.\d+[\w.-]*")),
        ("Ninja", tool_version(["ninja", "--version"], r"\d+\.\d+\.\d+")),
        ("clang-tidy", tool_version(["clang-tidy", "--version"], r"\d+\.\d+\.\d+")),
        ("clang-format", tool_version(["clang-format", "--version"], r"\d+\.\d+\.\d+")),
        ("Python", platform.python_version()),
        ("Git", tool_version(["git", "--version"], r"\d+\.\d+\.\d+[\w.-]*")),
    ]

    if platform.system() != "Windows":
        outils.append(("gcovr", tool_version(["gcovr", "--version"], r"\d+\.\d+")))

    sortie: list[str] = []
    sortie.append("# SECI — Software Life Cycle Environment Configuration Index")
    sortie.append("")
    sortie.append("> Document DO-178C §11.15. Catégorie de contrôle : **CC1** à tous les niveaux.")
    sortie.append("> Généré automatiquement par `tools/config_index.py`.")
    sortie.append("")
    sortie.append("## 1. Environnement de production et de vérification")
    sortie.append("")
    sortie.append("| Élément | Version détectée |")
    sortie.append("|---|---|")
    for nom, valeur in outils:
        sortie.append(f"| {nom} | `{valeur}` |")
    sortie.append("")
    # Un SECI genere hors de l environnement de production ne DECRIT PAS cet
    # environnement. On le dit, plutot que de laisser croire que la chaine
    # d outils est absente de la machine.
    if any(valeur == "non detecte" for _, valeur in outils):
        sortie.append("> ⚠️ Certains outils n'ont pas été détectés.")
        sortie.append(">")
        sortie.append("> Sous Windows, la chaîne Visual Studio n'est dans le `PATH` qu'à")
        sortie.append("> l'intérieur de l'environnement développeur. Régénérez ce document")
        sortie.append("> depuis une **Developer PowerShell for VS**, ou après avoir lancé")
        sortie.append(r"> `scripts/build.ps1`, faute de quoi le SECI ne décrit pas")
        sortie.append("> l'environnement qui a réellement produit le binaire.")
        sortie.append("")

    sortie.append("## 2. Options de compilation")
    sortie.append("")
    sortie.append("Définies dans `cmake/TrainingHelpers.cmake`, fonction")
    sortie.append("`training_setup_compiler_flags()` :")
    sortie.append("")
    sortie.append("| Option | Rôle |")
    sortie.append("|---|---|")
    sortie.append("| `/W4` | niveau d'avertissement élevé |")
    sortie.append("| `/WX` | avertissements traités comme erreurs (preset `strict`) |")
    sortie.append("| `/permissive-` | conformité stricte au standard |")
    sortie.append("| `/Zc:__cplusplus` | macro `__cplusplus` correcte |")
    sortie.append("| `/utf-8` | sources et exécutables en UTF-8 |")
    sortie.append("| `/EHsc` | modèle d'exceptions (désactivé en production réelle) |")
    sortie.append("| `/fp:precise` | arithmétique flottante déterministe |")
    sortie.append("| `-std=c++17` | norme du langage |")
    sortie.append("")
    sortie.append("## 3. Outils de vérification")
    sortie.append("")
    sortie.append("| Outil | Rôle | Qualification DO-330 |")
    sortie.append("|---|---|---|")
    sortie.append("| `microtest` | harnais de test unitaire | non requise — utilisé en complément de la revue |")
    sortie.append("| `clang-tidy` | analyse statique | non requise — n'élimine aucune activité |")
    sortie.append("| `tools/trace_check.py` | matrice de traçabilité | non requise — complète la revue manuelle |")
    sortie.append("| `tools/config_index.py` | génération SCI/SECI | non requise — produit une donnée, relue |")
    sortie.append("| `OpenCppCoverage` | couverture d'instructions | **TQL-5 si le résultat remplace une revue** |")
    sortie.append("")
    sortie.append("> Cette dernière colonne est le cœur de la DO-330 : la question n'est")
    sortie.append("> jamais « l'outil est-il bon ? » mais « son résultat remplace-t-il une")
    sortie.append("> activité que la norme exige ? ».")
    sortie.append("")
    return "\n".join(sortie)


def main() -> int:
    # La console Windows utilise cp1252 par defaut : on force l'UTF-8 pour que
    # les accents et les symboles des rapports s'affichent correctement.
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, OSError):
        pass

    parser = argparse.ArgumentParser(description="Genere le SCI et le SECI du depot.")
    parser.add_argument("--root", default=".", help="racine du depot")
    parser.add_argument("--output", default="reports", help="repertoire de sortie")
    parser.add_argument("--part-number", default="PN-7654321-001")
    parser.add_argument("--version", default="1.0.0")
    parser.add_argument(
        "--print", action="store_true", help="affiche le SCI sur la sortie standard"
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not (root / "modules").is_dir():
        print(f"ERREUR : {root} ne ressemble pas a la racine du depot.")
        return 2

    sci = build_sci(root, args.part_number, args.version)
    seci = build_seci(root)

    sortie = Path(args.output)
    if not sortie.is_absolute():
        sortie = root / sortie
    sortie.mkdir(parents=True, exist_ok=True)

    (sortie / "SCI.md").write_text(sci, encoding="utf-8")
    (sortie / "SECI.md").write_text(seci, encoding="utf-8")

    if args.print:
        print(sci)
        print()
        print(seci)

    print(f"SCI  ecrit dans {sortie / 'SCI.md'}")
    print(f"SECI ecrit dans {sortie / 'SECI.md'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
