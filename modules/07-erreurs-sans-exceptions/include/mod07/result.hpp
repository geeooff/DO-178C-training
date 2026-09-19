// =============================================================================
//  Module 07 -- gestion d'erreurs SANS exceptions.
//
//  POURQUOI LES EXCEPTIONS SONT (PRESQUE TOUJOURS) INTERDITES EN AVIONIQUE
//  ----------------------------------------------------------------------
//  Ce n'est pas du conservatisme : chaque raison est technique et mesurable.
//
//  1. TEMPS D'EXECUTION NON BORNE
//     Le deroulement de pile (stack unwinding) parcourt des tables generees
//     par le compilateur, dont le cout depend de la profondeur d'appel et du
//     nombre d'objets a detruire. Aucun outil d'analyse WCET du marche ne sait
//     borner cela de facon exploitable. Or le WCET doit etre DEMONTRE.
//
//  2. FLOT DE CONTROLE IMPLICITE
//     `f(); g();` : si `f` peut lancer, `g` peut ne jamais s'executer, sans
//     que rien ne l'indique dans le code. La DO-178C exige une architecture
//     de flot de controle VERIFIABLE (objectif A-4.11) ; l'analyse de couplage
//     de controle (module 12) devient tres difficile.
//
//  3. ALLOCATION DYNAMIQUE
//     Sur la plupart des ABI, l'objet exception est alloue sur un tas dedie.
//     Allocation dynamique = interdite apres l'initialisation (module 08).
//
//  4. TAILLE DU CODE
//     Les tables de deroulement pesent typiquement 10 a 30 % du binaire. Sur
//     une cible avec 512 ko de Flash, cela se discute.
//
//  5. DO-332, VULNERABILITE 6
//     Le supplement OO ajoute des objectifs specifiques a la gestion des
//     exceptions. Les eviter, c'est eviter ces objectifs.
//
//  Les projets certifies compilent donc avec les exceptions DESACTIVEES :
//      MSVC       : /EHs-c-   (avec /D_HAS_EXCEPTIONS=0 pour la STL)
//      GCC/Clang  : -fno-exceptions
//
//  Ce depot les laisse actives (la formation tourne sur PC), mais tout le code
//  est ecrit COMME SI elles etaient interdites : `noexcept` partout, et le
//  motif `Result<T>` ci-dessous.
//
//  ATTENTION : `noexcept` n'empeche PAS de lancer. Il PROMET de ne pas le
//  faire. Si une exception s'echappe malgre tout d'une fonction `noexcept`,
//  `std::terminate()` est appele -- ce qui, en vol, signifie un redemarrage du
//  calculateur.
// =============================================================================
#ifndef MOD07_RESULT_HPP
#define MOD07_RESULT_HPP

#include <avio/assert.hpp>
#include <avio/types.hpp>
#include <type_traits>

namespace mod07 {

// -----------------------------------------------------------------------------
//  1. Le catalogue d'erreurs
// -----------------------------------------------------------------------------
//  Un `enum class` unique pour tout le composant. Avantages :
//    * la liste des erreurs possibles est EXHAUSTIVE et revisable ;
//    * chaque valeur peut etre tracee a une exigence de robustesse ;
//    * un `switch` sans `default` force le compilateur a signaler tout oubli
//      lorsqu'une nouvelle erreur est ajoutee.
// -----------------------------------------------------------------------------
enum class Status : avio::u8 {
    Ok = 0U,
    InvalidArgument = 1U,  ///< l'appelant a fourni une entree hors contrat
    OutOfRange = 2U,       ///< resultat non representable
    ChecksumError = 3U,    ///< integrite du message compromise
    NotReady = 4U,         ///< donnee pas encore disponible
    HardwareFault = 5U,    ///< l'equipement signale une panne
    Timeout = 6U           ///< echeance depassee
};

/// Libelle lisible. Ne renvoie jamais nullptr (robustesse).
const char* status_name(Status status) noexcept;

/// Vrai si le statut designe une anomalie devant etre remontee au systeme.
bool is_fault(Status status) noexcept;

// -----------------------------------------------------------------------------
//  2. Result<T> : une valeur OU une erreur
// -----------------------------------------------------------------------------
//  Equivalent de `std::expected<T, Status>` (C++23) ou du `Result<T, E>` de
//  Rust, reecrit ici pour C++17 et pour rester entierement maitrise.
//
//  CHOIX D'IMPLEMENTATION : la valeur et le statut coexistent, sans union ni
//  placement new. C'est quelques octets de plus, mais :
//    * aucune construction/destruction conditionnelle ;
//    * disposition memoire triviale, donc analysable ;
//    * pas de comportement indefini possible en cas de mauvais usage.
//  En avionique, la simplicite d'analyse prime sur l'economie d'octets.
// -----------------------------------------------------------------------------
template <typename T>
class Result {
public:
    static_assert(std::is_trivially_copyable_v<T>, "Result<T> : T doit etre trivialement copiable");
    static_assert(std::is_default_constructible_v<T>,
                  "Result<T> : T doit etre constructible par defaut");

    using value_type = T;

    /// Construit un resultat en erreur. C'est le DEFAUT : un Result non
    /// initialise explicitement ne doit jamais passer pour un succes.
    constexpr Result() noexcept : value_{}, status_(Status::NotReady) {}

    static constexpr Result ok(const T& value) noexcept { return Result(value, Status::Ok); }

    static constexpr Result error(Status status) noexcept {
        // Un Result::error(Status::Ok) serait un contresens : on le neutralise.
        return Result(T{}, (status == Status::Ok) ? Status::InvalidArgument : status);
    }

    constexpr bool is_ok() const noexcept { return status_ == Status::Ok; }
    constexpr bool is_error() const noexcept { return status_ != Status::Ok; }
    constexpr Status status() const noexcept { return status_; }

    /// Acces a la valeur. L'appelant DOIT avoir verifie is_ok().
    /// En cas de violation, le gestionnaire d'anomalie est notifie et une
    /// valeur neutre est renvoyee : deterministe, jamais indefini.
    const T& value() const noexcept {
        AVIO_ASSERT(is_ok());
        return value_;
    }

    /// Acces avec valeur de repli : la forme la plus sure, et la plus courante
    /// dans une boucle temps reel ou l'on ne peut pas s'arreter.
    constexpr T value_or(const T& fallback) const noexcept { return is_ok() ? value_ : fallback; }

private:
    constexpr Result(const T& value, Status status) noexcept : value_(value), status_(status) {}

    T value_;
    Status status_;
};

// -----------------------------------------------------------------------------
//  3. Compteur d'anomalies (surveillance)
// -----------------------------------------------------------------------------
//  Une erreur qui n'est ni traitee ni comptee est une erreur invisible. Tout
//  calculateur certifie tient des compteurs par type d'anomalie, relus par la
//  maintenance (BITE : Built-In Test Equipment).
// -----------------------------------------------------------------------------
class StatusCounters {
public:
    static constexpr avio::usize kStatusCount = 7U;

    void reset() noexcept;
    void record(Status status) noexcept;

    avio::u32 count(Status status) const noexcept;
    avio::u32 total_faults() const noexcept;

    /// Statut le plus frequent parmi les anomalies, Ok si aucune.
    Status dominant_fault() const noexcept;

private:
    avio::u32 counters_[kStatusCount] = {};
};

}  // namespace mod07

#endif  // MOD07_RESULT_HPP
