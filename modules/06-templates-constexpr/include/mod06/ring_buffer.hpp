// =============================================================================
//  Module 06 -- templates : le polymorphisme PARAMETRIQUE.
//
//  Un template n'est PAS une classe : c'est un patron a partir duquel le
//  compilateur FABRIQUE une classe pour chaque jeu de parametres utilise.
//  `RingBuffer<i32, 4>` et `RingBuffer<f32, 16>` sont deux types distincts,
//  avec deux codes machine distincts.
//
//  DIFFERENCE MAJEURE AVEC LES GENERIQUES C#
//  -----------------------------------------
//  C#  : `List<T>` est compile UNE FOIS. Les contraintes (`where T : IComparable`)
//        sont verifiees a la definition. Le JIT partage le code pour tous les
//        types reference.
//  C++ : le template est re-instancie POUR CHAQUE type. Il n'y a pas de
//        contrainte declaree (avant les `concepts` de C++20) : la validite est
//        verifiee A L'INSTANCIATION, ce qui explique les messages d'erreur
//        interminables. C'est du "duck typing" a la compilation.
//
//  CONSEQUENCE POUR LA DO-178C  (DO-332, vulnerabilite n.2)
//  --------------------------------------------------------
//  Le code source contient UN template ; le code executable contient N copies.
//  La couverture structurelle porte sur le CODE EXECUTABLE : elle doit donc
//  etre obtenue POUR CHAQUE INSTANCIATION effectivement embarquee.
//
//  Avoir couvert `RingBuffer<i32, 4>` ne dit RIEN de `RingBuffer<f32, 16>` :
//  le code genere peut differer (comparaisons flottantes, tailles, deroulage
//  de boucle...). C'est un point d'attention classique en revue.
//
//  Corollaire pratique : on LIMITE volontairement le nombre d'instanciations
//  embarquees, et on les LISTE dans le document de conception.
// =============================================================================
#ifndef MOD06_RING_BUFFER_HPP
#define MOD06_RING_BUFFER_HPP

#include <avio/types.hpp>

#include <type_traits>

namespace mod06 {

/// Tampon circulaire a capacite fixe, connue a la compilation.
///
/// @tparam T type des elements (doit etre trivialement copiable)
/// @tparam N capacite, strictement positive
template <typename T, avio::usize N>
class RingBuffer {
public:
    // CONTRAINTES EXPLICITES.
    // Sans elles, une utilisation invalide produirait vingt lignes d'erreurs
    // incomprehensibles au fond de la bibliotheque standard. Avec elles, le
    // message est celui que NOUS avons ecrit. C'est l'equivalent C++17 des
    // `concepts` de C++20, et cela remplace le `where T : ...` de C#.
    static_assert(N > 0U, "RingBuffer : la capacite doit etre strictement positive");
    static_assert(std::is_trivially_copyable_v<T>,
                  "RingBuffer : T doit etre trivialement copiable (pas de ressource a gerer)");

    using value_type = T;
    static constexpr avio::usize kCapacity = N;

    constexpr RingBuffer() noexcept : storage_{}, head_(0U), count_(0U), overwrites_(0U) {}

    /// Ajoute un element. Ecrase le plus ancien si le tampon est plein.
    /// @return false si un element a ete ecrase (le tampon etait plein)
    bool push(const T& value) noexcept {
        const avio::usize tail = (head_ + count_) % N;
        bool sans_perte = true;

        if (count_ == N) {
            head_ = (head_ + 1U) % N;  // le plus ancien est abandonne
            overwrites_ += 1U;
            sans_perte = false;
        } else {
            count_ += 1U;
        }
        storage_[tail] = value;
        return sans_perte;
    }

    /// Retire le plus ancien element.
    /// @return false si le tampon est vide (out_value non modifie)
    bool pop(T& out_value) noexcept {
        if (count_ == 0U) {
            return false;
        }
        out_value = storage_[head_];
        head_ = (head_ + 1U) % N;
        count_ -= 1U;
        return true;
    }

    /// Lecture sans retrait, 0 = le plus ancien.
    bool peek(avio::usize index, T& out_value) const noexcept {
        if (index >= count_) {
            return false;
        }
        out_value = storage_[(head_ + index) % N];
        return true;
    }

    constexpr avio::usize size() const noexcept { return count_; }
    constexpr bool empty() const noexcept { return count_ == 0U; }
    constexpr bool full() const noexcept { return count_ == N; }
    constexpr avio::u32 overwrite_count() const noexcept { return overwrites_; }

    void clear() noexcept {
        head_ = 0U;
        count_ = 0U;
        overwrites_ = 0U;
    }

private:
    T storage_[N];
    avio::usize head_;
    avio::usize count_;
    avio::u32 overwrites_;
};

// -----------------------------------------------------------------------------
//  Algorithme generique + `if constexpr`
// -----------------------------------------------------------------------------

/// Moyenne des elements d'un tampon circulaire.
///
/// `if constexpr` (C++17) selectionne A LA COMPILATION la branche a garder :
/// la branche non retenue n'est meme pas compilee, et n'apparait donc pas dans
/// le code executable. Consequence pour la couverture : la version entiere et
/// la version flottante sont DEUX codes differents, a couvrir separement.
template <typename T, avio::usize N>
T average(const RingBuffer<T, N>& buffer) noexcept {
    if (buffer.empty()) {
        return T{};
    }

    if constexpr (std::is_integral_v<T>) {
        // Accumulation en 64 bits : aucun debordement possible tant que
        // N * max(T) tient sur 64 bits.
        avio::i64 somme = 0;
        for (avio::usize index = 0U; index < buffer.size(); ++index) {
            T valeur{};
            (void)buffer.peek(index, valeur);
            somme += static_cast<avio::i64>(valeur);
        }
        return static_cast<T>(somme / static_cast<avio::i64>(buffer.size()));
    } else {
        double somme = 0.0;
        for (avio::usize index = 0U; index < buffer.size(); ++index) {
            T valeur{};
            (void)buffer.peek(index, valeur);
            somme += static_cast<double>(valeur);
        }
        return static_cast<T>(somme / static_cast<double>(buffer.size()));
    }
}

}  // namespace mod06

#endif  // MOD06_RING_BUFFER_HPP
