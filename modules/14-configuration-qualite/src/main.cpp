// =============================================================================
//  Module 14 -- demonstration : configuration, qualite, qualification d'outils.
// =============================================================================
#include <avio/span.hpp>
#include <avio/types.hpp>
#include <cstdio>

#include "mod14/identity.hpp"

using avio::u32;
using avio::u8;
using avio::usize;
using mod14::SoftwareIdentity;

namespace {

void title(const char* text) {
    std::printf("\n=== %s ===\n", text);
}

// -----------------------------------------------------------------------------
void life_cycle_data() {
    title("Les donnees de vie du logiciel (DO-178C section 11)");

    const avio::Span<const mod14::LifeCycleData> table = mod14::life_cycle_data();

    std::printf("  %-6s %-52s %-8s %-8s\n", "acro.", "intitule", "DAL A/B", "DAL C/D");
    std::printf("  ------------------------------------------------------------------------\n");
    for (usize index = 0U; index < table.size(); ++index) {
        std::printf("  %-6s %-52s %-8s %-8s\n", table[index].acronym, table[index].name,
                    mod14::category_name(table[index].dal_ab),
                    mod14::category_name(table[index].dal_cd));
    }

    std::printf("\n  CC1 et CC2 ne classent pas l'IMPORTANCE d'un document : ils\n");
    std::printf("  classent le NIVEAU DE RIGUEUR de son controle.\n\n");
    std::printf("    CC1 -- identification, tracabilite des changements, REVUE des\n");
    std::printf("           changements, controle des baselines, ARCHIVAGE,\n");
    std::printf("           chargement controle, protection.\n");
    std::printf("    CC2 -- identification, tracabilite des changements, protection.\n\n");
    std::printf("  Remarquez que la categorie DEPEND DU NIVEAU DAL : le SDD est CC1\n");
    std::printf("  en DAL A/B et CC2 en DAL C/D. Passer de DAL C a DAL B, ce n'est\n");
    std::printf("  pas seulement plus de tests : c'est plus de RIGUEUR sur les memes\n");
    std::printf("  documents.\n\n");
    std::printf("  Six donnees restent CC1 a TOUS les niveaux : PSAC, SRD, SRC, EOC,\n");
    std::printf("  SECI, SCI. Ce sont celles sans lesquelles on ne peut ni identifier\n");
    std::printf("  ni reconstruire le produit.\n");
}

// -----------------------------------------------------------------------------
void loading_integrity() {
    title("Verification d'integrite du logiciel charge");

    SoftwareIdentity identity;
    identity.part_number = "PN-7654321-002";
    identity.version_major = 1U;
    identity.version_minor = 2U;
    identity.version_patch = 3U;

    const u8 valid_image[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U};
    identity.expected_crc = mod14::crc32(avio::make_const_span(valid_image));

    std::printf(
        "  part number      : %s (%s)\n", identity.part_number,
        mod14::is_valid_part_number(identity.part_number) ? "format valide" : "FORMAT INVALIDE");
    std::printf("  version          : %u.%u.%u\n", identity.version_major, identity.version_minor,
                identity.version_patch);
    std::printf("  CRC-32 attendu   : 0x%08X\n", identity.expected_crc);
    std::printf("\n");

    std::printf(
        "  image conforme                 -> %s\n",
        mod14::verify_load(identity, avio::make_const_span(valid_image)) ? "ACCEPTEE" : "refusee");

    u8 altered_image[9] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x38U};
    std::printf("  image alteree (1 octet)        -> %s\n",
                mod14::verify_load(identity, avio::make_const_span(altered_image)) ? "acceptee"
                                                                                   : "REFUSEE");

    const avio::Span<const u8> empty;
    std::printf("  image vide                     -> %s\n",
                mod14::verify_load(identity, empty) ? "acceptee" : "REFUSEE");

    SoftwareIdentity bad_pn = identity;
    bad_pn.part_number = "SW-V1.2";
    std::printf(
        "  part number hors format        -> %s\n",
        mod14::verify_load(bad_pn, avio::make_const_span(valid_image)) ? "acceptee" : "REFUSEE");

    std::printf("\n  Le chargement logiciel d'un equipement en atelier peut echouer\n");
    std::printf("  partiellement, la Flash peut se degrader, et un technicien peut\n");
    std::printf("  charger la mauvaise version. La verification d'integrite au\n");
    std::printf("  demarrage est la derniere barriere.\n");
}

// -----------------------------------------------------------------------------
void tool_qualification() {
    title("DO-330 : quand faut-il qualifier un outil ?");
    std::printf("  LA question n'est jamais \"l'outil est-il bon ?\" mais :\n");
    std::printf("  \"SON RESULTAT REMPLACE-T-IL UNE ACTIVITE QUE LA NORME EXIGE ?\"\n\n");

    std::printf("  TROIS CRITERES (DO-178C 12.2.1) :\n");
    std::printf("    Critere 1 : l'outil produit du code embarque SANS que sa sortie\n");
    std::printf("                soit verifiee -> outil de DEVELOPPEMENT\n");
    std::printf("                (compilateur qualifie, generateur de code)\n");
    std::printf("    Critere 2 : l'outil automatise une verification ET pourrait ne\n");
    std::printf("                pas detecter une erreur\n");
    std::printf("    Critere 3 : l'outil, par son resultat, permet de REDUIRE une\n");
    std::printf("                autre activite que la selection des cas de test\n\n");

    std::printf("  CINQ NIVEAUX (TQL) :\n");
    std::printf("    TQL-1 a 3 : outils de DEVELOPPEMENT (critere 1), selon le DAL.\n");
    std::printf("                Tres couteux : un compilateur qualifie TQL-1 se\n");
    std::printf("                compte en millions d'euros.\n");
    std::printf("    TQL-4     : criteres 2 ou 3, DAL A ou B\n");
    std::printf("    TQL-5     : criteres 2 ou 3, DAL C ou D\n\n");

    std::printf("  APPLICATION AUX OUTILS DE CE DEPOT :\n");
    std::printf("    %-24s %s\n", "microtest", "non qualifie : complete la revue");
    std::printf("    %-24s %s\n", "clang-tidy", "non qualifie : n'elimine rien");
    std::printf("    %-24s %s\n", "trace_check.py", "non qualifie : complete la revue");
    std::printf("    %-24s %s\n", "config_index.py", "non qualifie : produit une donnee relue");
    std::printf("    %-24s %s\n", "OpenCppCoverage", "TQL-5 SI son resultat remplace");
    std::printf("    %-24s %s\n", "", "une analyse manuelle de couverture");
    std::printf("\n  Le compilateur MSVC n'est PAS qualifie. C'est la situation\n");
    std::printf("  normale : on ne qualifie pas le compilateur, on VERIFIE SA SORTIE\n");
    std::printf("  (tests sur cible, couverture du code objet en DAL A). C'est\n");
    std::printf("  precisement pourquoi la DO-178C exige des tests sur le CODE\n");
    std::printf("  EXECUTABLE, et non sur le code source.\n");
}

// -----------------------------------------------------------------------------
void configuration_with_git() {
    title("Gestion de configuration : Git au service de la DO-178C");
    std::printf("  La DO-178C section 7 demande six choses. Git en couvre cinq :\n\n");
    std::printf("  %-42s %s\n", "EXIGENCE DO-178C", "MOYEN");
    std::printf("  ------------------------------------------------------------------\n");
    std::printf("  %-42s %s\n", "Identification de configuration", "commit SHA-1 / SHA-256");
    std::printf("  %-42s %s\n", "Etablissement de baselines", "git tag signe");
    std::printf("  %-42s %s\n", "Tracabilite des changements", "historique + message");
    std::printf("  %-42s %s\n", "Controle des changements", "revue de fusion obligatoire");
    std::printf("  %-42s %s\n", "Archivage et restitution", "depot miroir hors ligne");
    std::printf("  %-42s %s\n", "Rapports de probleme (SCR)", "systeme de tickets SEPARE");
    std::printf("\n  Le sixieme point est celui que Git ne couvre PAS : le suivi des\n");
    std::printf("  anomalies est un processus a part entiere, avec classification,\n");
    std::printf("  analyse d'impact et approbation de cloture.\n\n");
    std::printf("  BONNES PRATIQUES QUI PAIENT EN AUDIT :\n");
    std::printf("    * un commit = un changement ATOMIQUE, avec son motif ;\n");
    std::printf("    * le message reference l'exigence ou l'anomalie traitee ;\n");
    std::printf("    * les baselines sont des ETIQUETTES SIGNEES, jamais des branches ;\n");
    std::printf("    * aucune reecriture d'historique apres baseline (pas de rebase,\n");
    std::printf("      pas de force-push) : la piste d'audit doit rester intacte ;\n");
    std::printf("    * les binaires produits ne sont PAS dans le depot : ils sont\n");
    std::printf("      reconstruits a partir du SCI.\n\n");
    std::printf("  Generer le SCI et le SECI de ce depot :\n\n");
    std::printf("      python tools/config_index.py\n\n");
    std::printf("  Les deux documents sont ecrits dans reports/.\n");
}

// -----------------------------------------------------------------------------
void quality_assurance() {
    title("Assurance qualite logicielle (section 8)");
    std::printf("  La SQA ne verifie PAS le produit : elle verifie que le PROCESSUS\n");
    std::printf("  a ete suivi. C'est une distinction que beaucoup de candidats\n");
    std::printf("  manquent en entretien.\n\n");
    std::printf("    Verification (section 6) : \"le logiciel est-il correct ?\"\n");
    std::printf("    Assurance qualite (sect. 8) : \"avons-nous suivi nos plans ?\"\n\n");
    std::printf("  Concretement, la SQA :\n");
    std::printf("    * audite la conformite aux plans (PSAC, SDP, SVP, SCMP) ;\n");
    std::printf("    * verifie que les revues ont eu lieu, avec les bonnes personnes\n");
    std::printf("      et l'INDEPENDANCE requise ;\n");
    std::printf("    * verifie que les anomalies sont tracees et cloturees ;\n");
    std::printf("    * conduit la CONFORMITY REVIEW avant livraison ;\n");
    std::printf("    * dispose d'une AUTORITE et d'une INDEPENDANCE organisationnelles.\n\n");
    std::printf("  Livrables : SQAR (enregistrements) et contribution au SAS\n");
    std::printf("  (Software Accomplishment Summary), le document final qui declare\n");
    std::printf("  au regulateur que tous les objectifs sont satisfaits.\n");
}

}  // namespace

int main() {
    std::printf("#############################################################\n");
    std::printf("#  Module 14 : configuration, qualite, qualification d'outils#\n");
    std::printf("#############################################################\n");

    life_cycle_data();
    loading_integrity();
    configuration_with_git();
    tool_qualification();
    quality_assurance();

    std::printf("\nModule 14 termine.\n");
    return 0;
}
