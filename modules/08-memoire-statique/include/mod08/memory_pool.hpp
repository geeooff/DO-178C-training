// =============================================================================
//  Module 08 -- reserve de blocs (memory pool) a temps constant.
//
//  Certains systemes ont BESOIN d'allouer et de liberer a l'execution : file de
//  messages, contextes de communication, tampons de reception. La reponse
//  avionique n'est pas `new` : c'est une RESERVE DE BLOCS DE TAILLE FIXE,
//  reservee statiquement.
//
//  Proprietes obtenues, toutes verifiables :
//    * allocation et liberation en TEMPS CONSTANT (liste chainee de blocs
//      libres) -> WCET borne et connu ;
//    * AUCUNE FRAGMENTATION possible : tous les blocs ont la meme taille ;
//    * epuisement DETECTABLE et BORNE : la capacite est connue a la compilation ;
//    * double liberation DETECTEE (bitmap d'occupation) ;
//    * pointeur etranger REFUSE (verification d'appartenance).
//
//  C'est ainsi que fonctionnent les "buffer pools" d'un noyau ARINC 653.
//
//  DETAIL D'IMPLEMENTATION : la liste des blocs libres est stockee DANS les
//  blocs eux-memes (un bloc libre contient l'adresse du suivant). Cout memoire
//  supplementaire : zero. C'est pourquoi BlockSize doit valoir au moins la
//  taille d'un pointeur.
// =============================================================================
#ifndef MOD08_MEMORY_POOL_HPP
#define MOD08_MEMORY_POOL_HPP

#include <avio/types.hpp>

#include <cstddef>
#include <cstring>

namespace mod08 {

template <avio::usize BlockSize, avio::usize BlockCount>
class MemoryPool {
public:
    static_assert(BlockSize >= sizeof(void*),
                  "MemoryPool : un bloc doit pouvoir contenir un pointeur (liste des libres)");
    static_assert(BlockCount > 0U, "MemoryPool : au moins un bloc");
    static_assert((BlockSize % alignof(std::max_align_t)) == 0U,
                  "MemoryPool : BlockSize doit etre un multiple de l'alignement maximal");

    static constexpr avio::usize kBlockSize = BlockSize;
    static constexpr avio::usize kBlockCount = BlockCount;

    MemoryPool() noexcept : storage_{}, used_{}, free_list_(nullptr), in_use_(0U), high_water_(0U) {
        reset();
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;
    ~MemoryPool() noexcept = default;

    /// Remet la reserve dans son etat initial : tous les blocs libres.
    /// A n'appeler qu'a l'INITIALISATION du systeme, jamais en vol : les
    /// pointeurs deja distribues deviendraient pendants.
    void reset() noexcept {
        for (avio::usize index = 0U; index < kBitmapBytes; ++index) {
            used_[index] = 0U;
        }
        free_list_ = nullptr;
        // On chaine a l'envers pour que allocate() renvoie les blocs dans
        // l'ordre croissant : plus lisible dans les journaux de test.
        for (avio::usize index = BlockCount; index > 0U; --index) {
            avio::u8* block = block_at(index - 1U);
            store_next(block, free_list_);
            free_list_ = block;
        }
        in_use_ = 0U;
        high_water_ = 0U;
    }

    /// Reserve un bloc.
    /// @return nullptr si la reserve est epuisee (JAMAIS une exception)
    void* allocate() noexcept {
        if (free_list_ == nullptr) {
            return nullptr;
        }
        avio::u8* block = static_cast<avio::u8*>(free_list_);
        free_list_ = load_next(block);

        set_used(index_of(block), true);
        in_use_ += 1U;
        if (in_use_ > high_water_) {
            high_water_ = in_use_;
        }
        return block;
    }

    /// Rend un bloc a la reserve.
    /// @return false si le pointeur n'appartient pas a la reserve, n'est pas
    ///         aligne sur un debut de bloc, ou correspond a un bloc deja libre
    ///         (double liberation).
    bool deallocate(void* pointer) noexcept {
        if (pointer == nullptr) {
            return false;
        }
        avio::u8* block = static_cast<avio::u8*>(pointer);
        if (!owns(block)) {
            return false;
        }
        const avio::usize offset = static_cast<avio::usize>(block - storage_);
        if ((offset % BlockSize) != 0U) {
            return false;  // pointeur au milieu d'un bloc
        }
        const avio::usize index = offset / BlockSize;
        if (!is_used(index)) {
            return false;  // DOUBLE LIBERATION detectee
        }

        set_used(index, false);
        store_next(block, free_list_);
        free_list_ = block;
        in_use_ -= 1U;
        return true;
    }

    bool owns(const void* pointer) const noexcept {
        const avio::u8* candidate = static_cast<const avio::u8*>(pointer);
        return (candidate >= storage_) && (candidate < (storage_ + (BlockSize * BlockCount)));
    }

    avio::usize in_use() const noexcept { return in_use_; }
    avio::usize available() const noexcept { return BlockCount - in_use_; }

    /// Occupation MAXIMALE atteinte depuis reset(). C'est LA metrique a
    /// relever en essais : elle valide (ou invalide) le dimensionnement.
    /// Une reserve dont le pic atteint 100 % est sous-dimensionnee.
    avio::usize high_water_mark() const noexcept { return high_water_; }

private:
    static constexpr avio::usize kBitmapBytes = (BlockCount + 7U) / 8U;

    avio::u8* block_at(avio::usize index) noexcept { return storage_ + (index * BlockSize); }

    avio::usize index_of(const avio::u8* block) const noexcept {
        return static_cast<avio::usize>(block - storage_) / BlockSize;
    }

    // Le pointeur "bloc libre suivant" est ecrit dans le bloc via memcpy :
    // aucune violation des regles d'aliasing, aucun comportement indefini.
    static void store_next(avio::u8* block, void* next) noexcept {
        std::memcpy(block, &next, sizeof(void*));
    }

    static avio::u8* load_next(const avio::u8* block) noexcept {
        void* next = nullptr;
        std::memcpy(&next, block, sizeof(void*));
        return static_cast<avio::u8*>(next);
    }

    bool is_used(avio::usize index) const noexcept {
        const avio::u8 mask = static_cast<avio::u8>(1U << (index % 8U));
        return (used_[index / 8U] & mask) != 0U;
    }

    void set_used(avio::usize index, bool value) noexcept {
        const avio::u8 mask = static_cast<avio::u8>(1U << (index % 8U));
        if (value) {
            used_[index / 8U] = static_cast<avio::u8>(used_[index / 8U] | mask);
        } else {
            used_[index / 8U] = static_cast<avio::u8>(used_[index / 8U] & static_cast<avio::u8>(~mask));
        }
    }

    alignas(std::max_align_t) avio::u8 storage_[BlockSize * BlockCount];
    avio::u8 used_[kBitmapBytes];
    void* free_list_;
    avio::usize in_use_;
    avio::usize high_water_;
};

}  // namespace mod08

#endif  // MOD08_MEMORY_POOL_HPP
