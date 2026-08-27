// =============================================================================
//  Module 02 -- pointeurs, references, const-correctness.
//
//  Le pointeur est LA construction que la DO-178C et MISRA encadrent le plus
//  severement, pour une raison simple : un pointeur invalide ne produit pas
//  une exception propre, il corrompt silencieusement la memoire d'un autre
//  composant. Dans un calculateur qui heberge plusieurs fonctions de niveaux
//  de criticite differents, c'est une perte de "freedom from interference".
//
//  Regles appliquees dans ce module (issues de MISRA C++ / AUTOSAR C++14) :
//    * aucune arithmetique de pointeur en dehors d'un conteneur maitrise ;
//    * un parametre qui n'est pas modifie est `const` ;
//    * une reference plutot qu'un pointeur des que l'absence est impossible ;
//    * couple (pointeur, taille) toujours transporte ensemble -> avio::Span ;
//    * aucune fonction ne renvoie un pointeur vers une variable locale.
// =============================================================================
#ifndef MOD02_BUFFERS_HPP
#define MOD02_BUFFERS_HPP

#include <avio/span.hpp>
#include <avio/types.hpp>

namespace mod02 {

// -----------------------------------------------------------------------------
//  1. Reference contre pointeur
// -----------------------------------------------------------------------------
//  Reference : alias d'un objet EXISTANT. Ne peut pas etre nulle, ne peut pas
//  etre reliee a autre chose apres construction. C'est le choix par defaut.
//
//  Pointeur : peut etre nul, peut etre redirige, supporte l'arithmetique.
//  On ne l'emploie que si l'absence de valeur a un sens.
//
//  Pour un developpeur C# : `ref int` ressemble a `int&`, et `int*` ressemble
//  a une reference d'objet qui peut valoir null -- sauf qu'ici, personne ne
//  verifie a votre place.
// -----------------------------------------------------------------------------

/// Echange deux valeurs. Les references garantissent que les deux objets
/// existent : aucun test de nullite n'est necessaire, donc aucune branche
/// morte a justifier lors de l'analyse de couverture.
void swap_values(avio::i32& lhs, avio::i32& rhs) noexcept;

/// Incremente la valeur pointee SI le pointeur est valide.
/// Renvoie false si le pointeur est nul : c'est une exigence de robustesse,
/// donc une branche testable (et non du code defensif injustifiable).
bool increment_if_valid(avio::i32* value) noexcept;

// -----------------------------------------------------------------------------
//  2. Traitements sur tampons
// -----------------------------------------------------------------------------

/// Cherche la valeur maximale d'un tampon.
/// @param values tampon d'entree, eventuellement vide
/// @param out_max recoit le maximum ; inchange si le tampon est vide
/// @return true si un maximum a ete trouve
///
/// Signature choisie plutot que `i32 find_max(...)` : que renverrait-on pour
/// un tampon vide ? 0 serait une valeur PLAUSIBLE et donc trompeuse. Le
/// couple (bool, out) rend l'echec impossible a ignorer par accident.
bool find_max(avio::Span<const avio::i32> values, avio::i32& out_max) noexcept;

/// Somme de controle 16 bits (complement a un, facon somme de controle IP).
/// Utilisee ici comme exemple de parcours de tampon sans arithmetique de
/// pointeur explicite.
avio::u16 checksum16(avio::Span<const avio::u8> data) noexcept;

/// Copie bornee : ne depasse JAMAIS la capacite de la destination.
/// @return le nombre d'octets reellement copies (min des deux tailles)
///
/// C'est le remplacant de `memcpy` + un commentaire optimiste. La taille de
/// destination fait partie du type, elle ne peut pas etre oubliee.
avio::usize copy_bounded(avio::Span<const avio::u8> source,
                         avio::Span<avio::u8> destination) noexcept;

/// Remplit un tampon avec une valeur constante.
void fill(avio::Span<avio::u8> destination, avio::u8 value) noexcept;

/// Compare deux tampons octet a octet.
bool equals(avio::Span<const avio::u8> lhs, avio::Span<const avio::u8> rhs) noexcept;

// -----------------------------------------------------------------------------
//  3. const-correctness
// -----------------------------------------------------------------------------
//  Les quatre formes a savoir lire (se lit de DROITE a GAUCHE) :
//
//      avio::u8*             p1;  // pointeur modifiable vers octet modifiable
//      const avio::u8*       p2;  // pointeur modifiable vers octet CONSTANT
//      avio::u8* const       p3;  // pointeur CONSTANT vers octet modifiable
//      const avio::u8* const p4;  // tout est constant
//
//  En C#, `readonly` s'applique au champ, pas a ce qu'il designe : un
//  `readonly List<int>` interdit de remplacer la liste, pas de la modifier.
//  `const` en C++ est bien plus expressif -- et le compilateur l'impose.
// -----------------------------------------------------------------------------

/// Journal circulaire de mesures a capacite fixe.
/// Illustration de la const-correctness : les methodes qui ne modifient pas
/// l'objet sont `const`, donc utilisables sur une reference constante.
class MeasurementLog {
public:
    static constexpr avio::usize kCapacity = 8U;

    MeasurementLog() noexcept;

    /// Ajoute une mesure. Ecrase la plus ancienne si le journal est plein.
    void push(avio::i32 measurement) noexcept;

    /// Nombre de mesures actuellement memorisees.
    avio::usize size() const noexcept;

    /// Vrai si le journal a deja ecrase au moins une mesure.
    bool has_overflowed() const noexcept;

    /// Lecture indexee, 0 = la plus ancienne encore presente.
    /// @return false si l'index est hors domaine (robustesse).
    bool at(avio::usize index, avio::i32& out_value) const noexcept;

    /// Vue en LECTURE SEULE sur le stockage interne.
    /// Renvoyer une Span<const T> plutot qu'un pointeur brut documente la
    /// taille et interdit l'ecriture par construction.
    avio::Span<const avio::i32> raw_storage() const noexcept;

    void clear() noexcept;

private:
    avio::i32 storage_[kCapacity];
    avio::usize write_index_;
    avio::usize count_;
    bool overflowed_;
};

}  // namespace mod02

#endif  // MOD02_BUFFERS_HPP
