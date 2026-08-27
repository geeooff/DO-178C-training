// =============================================================================
//  Module 15 -- `volatile` : ce qu'il garantit, et surtout ce qu'il NE garantit
//  PAS.
//
//  CE QUE `volatile` GARANTIT
//  --------------------------
//  Le compilateur ne doit ni supprimer, ni reordonner, ni fusionner les acces
//  a un objet `volatile`. Chaque lecture produit un acces memoire reel, chaque
//  ecriture aussi.
//
//  C'est indispensable pour :
//    * les REGISTRES MATERIELS, dont la valeur change sans que le programme
//      l'ait ecrite (un registre d'etat de convertisseur, par exemple) ;
//    * les variables modifiees par une INTERRUPTION ;
//    * la memoire partagee avec un peripherique en acces direct (DMA).
//
//  Sans `volatile`, ce code est une boucle infinie apres optimisation :
//
//      while (registre_etat == 0U) { }   // le compilateur lit UNE fois
//
//  CE QUE `volatile` NE GARANTIT PAS
//  ---------------------------------
//    * l'ATOMICITE : une ecriture 32 bits sur un bus 16 bits reste en deux
//      temps ;
//    * l'ORDRE vis-a-vis des acces NON volatiles ;
//    * l'exclusion mutuelle entre taches ou coeurs ;
//    * les barrieres memoire d'un processeur multicoeur.
//
//  En C++11 et au-dela, pour la CONCURRENCE, c'est `std::atomic` qu'il faut,
//  pas `volatile`. La confusion entre les deux est l'une des erreurs les plus
//  repandues du developpement embarque : `volatile` s'adresse au MATERIEL,
//  `std::atomic` s'adresse aux AUTRES FILS D'EXECUTION.
//
//  Sur un calculateur monocoeur avec un ordonnancement a fenetres fixes
//  (voir schedule.hpp), le probleme se pose peu : les partitions ne
//  s'executent jamais simultanement. C'est encore un benefice du
//  partitionnement temporel.
// =============================================================================
#ifndef MOD15_HW_REGISTER_HPP
#define MOD15_HW_REGISTER_HPP

#include <avio/types.hpp>

namespace mod15 {

/// Registre materiel simule : en cible reelle, ce serait une adresse fixe
/// obtenue par `reinterpret_cast<volatile u32*>(0x4002'0010)`.
///
/// La simulation permet de tester le code d'acces sans materiel -- pratique
/// courante, et qui impose que le code de production ne connaisse QUE
/// l'interface, jamais l'adresse (module 12 : dependances explicites).
class SimulatedRegister {
public:
    /// Ecrit la valeur "vue par le materiel" (banc de test uniquement).
    void set_hardware_value(avio::u32 value) noexcept { storage_ = value; }

    /// Lecture, comme le ferait le code embarque.
    avio::u32 read() const noexcept { return storage_; }

    /// Ecriture, comme le ferait le code embarque.
    void write(avio::u32 value) noexcept { storage_ = value; }

    avio::u32 read_count() const noexcept { return read_count_; }

    /// Lecture COMPTEE : demontre que chaque lecture produit bien un acces.
    avio::u32 read_and_count() noexcept {
        read_count_ += 1U;
        return storage_;
    }

private:
    // `volatile` sur le stockage : sur cible reelle, c'est ce qui empeche le
    // compilateur de supprimer les acces.
    volatile avio::u32 storage_ = 0U;
    avio::u32 read_count_ = 0U;
};

/// Attend qu'un bit du registre passe a 1, avec un nombre d'iterations BORNE.
///
/// Point de conception majeur : une attente active NON BORNEE est interdite en
/// avionique. Si le materiel ne repond pas, la boucle doit se terminer et
/// signaler l'anomalie -- sans quoi le chien de garde redemarre le
/// calculateur, ce qui, en vol, est un evenement bien plus grave que la panne
/// du peripherique.
///
/// @satisfies LLR-DET-030
/// @return false si le bit n'est pas passe a 1 dans le budget d'iterations
bool wait_for_bit(SimulatedRegister& reg, avio::u32 bit_mask, avio::u32 max_iterations) noexcept;

}  // namespace mod15

#endif  // MOD15_HW_REGISTER_HPP
