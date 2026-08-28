// =============================================================================
//  Module 08 -- StaticVector : la capacite est dans le TYPE.
//
//  POURQUOI L'ALLOCATION DYNAMIQUE EST INTERDITE APRES INITIALISATION
//  ------------------------------------------------------------------
//  1. NON-DETERMINISME TEMPOREL
//     `new` et `malloc` parcourent des listes de blocs libres. Leur duree
//     depend de l'historique complet des allocations : elle n'est PAS bornable
//     de facon exploitable. Or le WCET doit etre demontre (module 15).
//
//  2. FRAGMENTATION
//     Apres des heures de vol, la memoire libre peut etre suffisante EN TOTAL
//     mais morcelee : l'allocation echoue alors qu'il "reste de la place".
//     Ce defaut apparait apres des dizaines d'heures, donc jamais en test.
//
//  3. ECHEC INGERABLE
//     Que faire si `new` echoue a 10 000 m ? Il n'y a pas de reponse
//     satisfaisante. La seule strategie tenable est de rendre l'echec
//     IMPOSSIBLE, en reservant tout a la compilation.
//
//  4. DO-332, OO.6.8.2
//     Le supplement OO impose de demontrer l'absence de fuite, de
//     fragmentation, d'epuisement et de reference pendante. Avec de
//     l'allocation statique, ces objectifs sont satisfaits PAR CONSTRUCTION.
//
//  LA REGLE DU DOMAINE : toute la memoire est reservee A LA COMPILATION.
//  L'occupation RAM est alors une constante, calculable, verifiable par
//  `sizeof`, et inscrite au budget memoire du calculateur.
//
//  StaticVector est l'equivalent de `std::vector` sans le tas : la TAILLE
//  varie a l'execution, la CAPACITE est fixee a la compilation.
// =============================================================================
#ifndef MOD08_STATIC_VECTOR_HPP
#define MOD08_STATIC_VECTOR_HPP

#include <avio/assert.hpp>
#include <avio/span.hpp>
#include <avio/types.hpp>
#include <type_traits>

namespace mod08 {

template <typename T, avio::usize Capacity>
class StaticVector {
public:
    static_assert(Capacity > 0U, "StaticVector : la capacite doit etre strictement positive");
    static_assert(std::is_trivially_copyable_v<T>,
                  "StaticVector : T doit etre trivialement copiable (aucune ressource a gerer)");
    static_assert(std::is_default_constructible_v<T>,
                  "StaticVector : T doit etre constructible par defaut");

    using value_type = T;
    static constexpr avio::usize kCapacity = Capacity;

    constexpr StaticVector() noexcept : storage_{}, size_(0U), rejected_(0U) {}

    // --- Capacite ------------------------------------------------------------
    constexpr avio::usize size() const noexcept { return size_; }
    constexpr avio::usize capacity() const noexcept { return Capacity; }
    constexpr bool empty() const noexcept { return size_ == 0U; }
    constexpr bool full() const noexcept { return size_ == Capacity; }

    /// Nombre d'ajouts REFUSES depuis la construction ou le dernier clear().
    /// Un conteneur plein est un evenement a SURVEILLER, pas a ignorer : c'est
    /// le signe que le dimensionnement est peut-etre faux.
    constexpr avio::u32 rejected_count() const noexcept { return rejected_; }

    // --- Modification --------------------------------------------------------

    /// Ajoute un element a la fin.
    /// @return false si le conteneur est plein (l'element n'est PAS ajoute)
    ///
    /// Comparez avec std::vector::push_back, qui reallouerait silencieusement.
    /// Ici, le depassement de capacite est un evenement OBSERVABLE, tracable
    /// a une exigence de robustesse.
    bool push_back(const T& value) noexcept {
        if (size_ >= Capacity) {
            rejected_ += 1U;
            return false;
        }
        storage_[size_] = value;
        size_ += 1U;
        return true;
    }

    /// Retire le dernier element.
    /// @return false si le conteneur est vide (out_value non modifie)
    bool pop_back(T& out_value) noexcept {
        if (size_ == 0U) {
            return false;
        }
        size_ -= 1U;
        out_value = storage_[size_];
        return true;
    }

    /// Retire l'element d'indice `index` en decalant les suivants.
    /// @return false si l'indice est hors domaine
    bool erase(avio::usize index) noexcept {
        if (index >= size_) {
            return false;
        }
        // BORNE DEFENSIVE -- voir README du module, section "quand le
        // compilateur ne peut pas vous croire".
        //
        // `size_ <= Capacity` est un invariant de la classe : aucune methode ne
        // permet de le violer. Mais cet invariant n'est visible NULLE PART dans
        // ce corps de fonction, et le compilateur ne raisonne que sur ce qu'il
        // voit. GCC -O2, en inlinant l'appel de robustesse `erase(100)`, doit
        // donc envisager un `size_` de 200 -- d'ou un avertissement d'ecriture
        // hors bornes (-Warray-bounds) parfaitement logique de son point de vue.
        //
        // Cette borne rend l'ecriture PROUVABLE et non plus seulement vraie.
        // Son prix est assume : la branche `Capacity` est inatteignable, donc
        // NON COUVRABLE. C'est le conflit classique entre programmation
        // defensive et couverture structurelle (paragraphe 6.4.4.3, CAST-17) :
        // il se traite par une justification d'analyse, pas en supprimant la
        // protection.
        const avio::usize last = (size_ < Capacity) ? size_ : Capacity;
        for (avio::usize k = index; (k + 1U) < last; ++k) {
            storage_[k] = storage_[k + 1U];
        }
        size_ -= 1U;
        return true;
    }

    void clear() noexcept {
        size_ = 0U;
        rejected_ = 0U;
    }

    // --- Acces ---------------------------------------------------------------

    /// Acces VERIFIE. La violation notifie le gestionnaire d'anomalie et
    /// renvoie une valeur deterministe : jamais de comportement indefini.
    bool at(avio::usize index, T& out_value) const noexcept {
        if (index >= size_) {
            return false;
        }
        out_value = storage_[index];
        return true;
    }

    /// Acces NON verifie. L'appelant garantit index < size().
    /// L'assertion documente ce contrat et l'observe en test.
    const T& operator[](avio::usize index) const noexcept {
        AVIO_ASSERT(index < size_);
        return storage_[(index < size_) ? index : 0U];
    }

    T& operator[](avio::usize index) noexcept {
        AVIO_ASSERT(index < size_);
        return storage_[(index < size_) ? index : 0U];
    }

    /// Vue sur les elements REELLEMENT presents (pas sur toute la capacite).
    avio::Span<const T> view() const noexcept { return avio::Span<const T>(storage_, size_); }

private:
    T storage_[Capacity];
    avio::usize size_;
    avio::u32 rejected_;
};

}  // namespace mod08

#endif  // MOD08_STATIC_VECTOR_HPP
