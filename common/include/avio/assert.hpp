// =============================================================================
//  avio/assert.hpp -- mecanisme d'assertion maitrise.
//
//  Debat classique en avionique : faut-il laisser les assertions dans le code
//  embarque ?
//
//  * Une assertion qui DISPARAIT en Release cree du "code different de celui
//    qui a ete verifie". La DO-178C exige que le code verifie soit le code
//    embarque (Executable Object Code). Compiler deux variantes oblige a
//    verifier les deux, ou a justifier tres precisement la difference.
//  * Une assertion qui RESTE devient du code executable comme un autre : il
//    doit alors etre tracable a une exigence et couvert structurellement.
//    Une branche "cette condition ne peut pas arriver" n'est jamais couverte
//    par un test... donc elle apparait comme du code mort (dead code), ce qui
//    est un constat inacceptable en DAL A/B/C.
//
//  Solution retenue ici, courante dans l'industrie : l'assertion appelle un
//  GESTIONNAIRE D'ANOMALIE remplacable. En test on l'observe, en vol il declenche
//  la strategie de securite prevue (passivation, bascule sur le calculateur
//  redondant, journalisation en memoire non volatile...). La branche d'erreur
//  est ainsi tracable a une exigence de robustesse, donc testable et couverte.
// =============================================================================
#ifndef AVIO_ASSERT_HPP
#define AVIO_ASSERT_HPP

#include "avio/types.hpp"

namespace avio {

/// Signature du gestionnaire d'anomalie.
using FaultHandler = void (*)(const char* condition, const char* file, i32 line);

/// Installe un gestionnaire et renvoie le precedent (permet de le restaurer).
FaultHandler set_fault_handler(FaultHandler handler) noexcept;

/// Nombre d'anomalies detectees depuis le dernier reset (utile en test).
u32 fault_count() noexcept;

/// Remet le compteur a zero.
void reset_fault_count() noexcept;

namespace detail {
/// Point d'entree unique : facilite l'instrumentation et la couverture.
void on_assert_failed(const char* condition, const char* file, i32 line) noexcept;
}  // namespace detail

}  // namespace avio

/// Assertion TOUJOURS active (Debug comme Release) : le code embarque est
/// identique au code verifie.
#define AVIO_ASSERT(condition)                                                    \
    do {                                                                          \
        if (!(condition)) {                                                       \
            ::avio::detail::on_assert_failed(#condition, __FILE__,                \
                                             static_cast<::avio::i32>(__LINE__)); \
        }                                                                         \
    } while (false)

#endif  // AVIO_ASSERT_HPP
