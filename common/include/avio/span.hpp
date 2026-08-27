// =============================================================================
//  avio/span.hpp -- vue non proprietaire sur une sequence contigue.
//
//  Equivalent conceptuel de `Span<T>` / `ReadOnlySpan<T>` en C#, ou de
//  `std::span` (C++20). On le reimplemente ici pour trois raisons :
//    1. la formation cible C++17 (comme MISRA C++:2023) ;
//    2. cela montre qu'une abstraction "zero cout" tient en 60 lignes ;
//    3. dans un contexte certifie, une abstraction qu'on maitrise entierement
//       est preferable a une dependance externe a re-verifier.
//
//  Une Span NE POSSEDE PAS les donnees : elle ne libere rien. C'est un couple
//  (pointeur, taille) qui remplace avantageusement le duo `T*` + `size_t`
//  passe separement, source classique de desynchronisation.
//
//  DANGER a connaitre : une Span peut survivre au tableau qu'elle designe
//  (dangling). En C#, le GC et les regles de `ref struct` empechent cela.
//  En C++, c'est a VOUS de garantir la duree de vie. C'est typiquement le
//  genre de propriete que l'on demontre par revue ou par analyse statique.
// =============================================================================
#ifndef AVIO_SPAN_HPP
#define AVIO_SPAN_HPP

#include "avio/assert.hpp"
#include "avio/types.hpp"

namespace avio {

template <typename T>
class Span {
public:
    constexpr Span() noexcept : data_(nullptr), size_(0U) {}
    constexpr Span(T* data, usize size) noexcept : data_(data), size_(size) {}

    /// Construction depuis un tableau C de taille connue a la compilation :
    /// la taille ne peut pas etre fausse.
    template <usize N>
    constexpr explicit Span(T (&array)[N]) noexcept : data_(array), size_(N) {}

    constexpr T* data() const noexcept { return data_; }
    constexpr usize size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0U; }

    constexpr T* begin() const noexcept { return data_; }
    constexpr T* end() const noexcept { return data_ + size_; }

    /// Acces NON verifie : rapide, mais l'appelant doit garantir index < size().
    constexpr T& operator[](usize index) const noexcept { return data_[index]; }

    /// Acces verifie : la violation declenche le gestionnaire d'anomalie.
    /// On renvoie tout de meme un element valide pour rester deterministe.
    T& at(usize index) const noexcept {
        AVIO_ASSERT(index < size_);
        const usize safe = (index < size_) ? index : 0U;
        return data_[safe];
    }

    constexpr Span<T> subspan(usize offset, usize count) const noexcept {
        return ((offset <= size_) && (count <= (size_ - offset))) ? Span<T>(data_ + offset, count)
                                                                  : Span<T>();
    }

private:
    T* data_;
    usize size_;
};

template <typename T, usize N>
constexpr Span<T> make_span(T (&array)[N]) noexcept {
    return Span<T>(array, N);
}

template <typename T, usize N>
constexpr Span<const T> make_const_span(const T (&array)[N]) noexcept {
    return Span<const T>(array, N);
}

}  // namespace avio

#endif  // AVIO_SPAN_HPP
