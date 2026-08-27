// =============================================================================
//  Module 04 -- classes, invariants et types forts.
//
//  LE PROBLEME DES UNITES
//  ----------------------
//  1983, vol Air Canada 143 ("Gimli Glider") : un Boeing 767 tombe en panne
//  seche a 12 500 m. Cause : la quantite de carburant a ete calculee avec le
//  facteur livres/litre au lieu de kilogrammes/litre. L'avion embarquait moins
//  de la moitie du carburant necessaire.
//
//  1999, Mars Climate Orbiter : sonde perdue a l'insertion orbitale. Un
//  logiciel produisait des impulsions en livres-force-seconde, l'autre les
//  lisait en newton-secondes.
//
//  Dans les deux cas, TOUS les nombres etaient corrects. C'est leur UNITE qui
//  ne l'etait pas. Un `double` ne porte pas son unite ; un TYPE FORT, si.
//
//  Cout : une classe de trente lignes, zero octet et zero cycle a l'execution
//  (le compilateur genere le meme code qu'avec un float nu).
//  Benefice : la confusion d'unites devient une ERREUR DE COMPILATION.
//
//  LES INVARIANTS
//  --------------
//  Un invariant est une propriete vraie a tout instant ou l'objet est
//  observable de l'exterieur. Exemple : "la quantite de carburant est comprise
//  entre 0 et la capacite du reservoir".
//
//  On l'obtient par construction :
//    * donnees membres PRIVEES ;
//    * tout constructeur etablit l'invariant ;
//    * toute methode publique le preserve.
//  En DO-178C, ces invariants sont exactement ce que l'on ecrit dans les
//  exigences de bas niveau (LLR) et dans le document de conception (SDD).
// =============================================================================
#ifndef MOD04_UNITS_HPP
#define MOD04_UNITS_HPP

#include <avio/types.hpp>

namespace mod04 {

// -----------------------------------------------------------------------------
//  1. Masse : type fort a representation ENTIERE
// -----------------------------------------------------------------------------
//  Stockage en grammes sur 32 bits signes : plage +/- 2 147 tonnes, resolution
//  1 g. Le choix d'un entier plutot que d'un flottant est delibere :
//    * l'egalite exacte a un sens ;
//    * l'accumulation n'introduit aucune derive ;
//    * le comportement est identique sur toute cible (module 15).
// -----------------------------------------------------------------------------
class Mass {
public:
    /// Constructeur par defaut : masse nulle. Un objet doit TOUJOURS etre dans
    /// un etat valide, meme sans argument.
    constexpr Mass() noexcept : grams_(0) {}

    /// Fabriques nommees. On ne peut pas surcharger un constructeur sur
    /// l'unite (deux `f32` sont indistinguables) : le nom porte l'information.
    /// @return false si la valeur est hors du domaine representable
    static bool from_grams(avio::i32 grams, Mass& out) noexcept;
    static bool from_kilograms(avio::f32 kilograms, Mass& out) noexcept;
    static bool from_pounds(avio::f32 pounds, Mass& out) noexcept;

    constexpr avio::i32 grams() const noexcept { return grams_; }
    avio::f32 kilograms() const noexcept;
    avio::f32 pounds() const noexcept;

    /// L'egalite exacte est LEGITIME ici : la representation est entiere.
    friend constexpr bool operator==(Mass lhs, Mass rhs) noexcept {
        return lhs.grams_ == rhs.grams_;
    }
    friend constexpr bool operator!=(Mass lhs, Mass rhs) noexcept { return !(lhs == rhs); }
    friend constexpr bool operator<(Mass lhs, Mass rhs) noexcept { return lhs.grams_ < rhs.grams_; }
    friend constexpr bool operator>(Mass lhs, Mass rhs) noexcept { return rhs < lhs; }
    friend constexpr bool operator<=(Mass lhs, Mass rhs) noexcept { return !(rhs < lhs); }
    friend constexpr bool operator>=(Mass lhs, Mass rhs) noexcept { return !(lhs < rhs); }

    /// Addition SATURANTE : une masse ne deborde jamais silencieusement.
    Mass operator+(Mass other) const noexcept;
    /// Soustraction saturante, bornee a zero (une masse reste positive).
    Mass operator-(Mass other) const noexcept;

    /// 200 tonnes : au-dela du domaine de tout aeronef civil. Borner le
    /// domaine est une decision de CONCEPTION, a ecrire dans le SDD.
    static constexpr avio::i32 kMaxGrams = 200000000;

private:
    /// Constructeur prive : impossible de creer une Mass sans passer par une
    /// fabrique qui verifie le domaine. L'invariant est donc garanti pour
    /// TOUTE instance existante.
    explicit constexpr Mass(avio::i32 grams) noexcept : grams_(grams) {}

    avio::i32 grams_;
};

// -----------------------------------------------------------------------------
//  2. Altitude : type fort a representation FLOTTANTE
// -----------------------------------------------------------------------------
class Altitude {
public:
    static constexpr avio::f32 kMinFeet = -2000.0F;  // Mer Morte : -1412 ft
    static constexpr avio::f32 kMaxFeet = 60000.0F;  // au-dela du domaine de vol
    static constexpr avio::f32 kFeetPerMeter = 3.280839895F;

    constexpr Altitude() noexcept : feet_(0.0F) {}

    /// @return false si la valeur est hors domaine ou n'est pas un nombre fini
    static bool from_feet(avio::f32 feet, Altitude& out) noexcept;
    static bool from_meters(avio::f32 meters, Altitude& out) noexcept;

    constexpr avio::f32 feet() const noexcept { return feet_; }
    avio::f32 meters() const noexcept;

    /// PAS d'operator== : sur des flottants, l'egalite exacte est un piege
    /// (voir module 15). On expose une comparaison A TOLERANCE EXPLICITE,
    /// ce qui oblige l'appelant a choisir -- et a documenter -- sa precision.
    bool is_close(Altitude other, avio::f32 tolerance_feet) const noexcept;

    /// L'ordre, lui, est parfaitement defini sur les flottants finis.
    friend bool operator<(Altitude lhs, Altitude rhs) noexcept { return lhs.feet_ < rhs.feet_; }
    friend bool operator>(Altitude lhs, Altitude rhs) noexcept { return rhs < lhs; }
    friend bool operator<=(Altitude lhs, Altitude rhs) noexcept { return !(rhs < lhs); }
    friend bool operator>=(Altitude lhs, Altitude rhs) noexcept { return !(lhs < rhs); }

    /// Ecart signe entre deux altitudes, en pieds.
    avio::f32 difference_feet(Altitude other) const noexcept;

private:
    explicit constexpr Altitude(avio::f32 feet) noexcept : feet_(feet) {}

    avio::f32 feet_;
};

// -----------------------------------------------------------------------------
//  3. Reservoir : un invariant a maintenir dans le temps
// -----------------------------------------------------------------------------
//  INVARIANT : 0 <= quantite <= capacite, a tout instant.
//
//  L'ecrire est facile ; le GARANTIR demande que chaque methode publique le
//  preserve, y compris en presence d'entrees absurdes. C'est ce que verifient
//  les tests de robustesse.
// -----------------------------------------------------------------------------
class FuelTank {
public:
    /// Reservoir "degenere" : capacite nulle, quantite nulle. L'invariant
    /// 0 <= quantite <= capacite est verifie (0 <= 0 <= 0). Sans ce
    /// constructeur, l'appelant ne pourrait pas declarer la variable de sortie
    /// de create().
    constexpr FuelTank() noexcept : capacity_(), quantity_() {}

    /// Construit un reservoir vide de capacite donnee.
    /// @return false si la capacite est nulle (un reservoir de capacite nulle
    ///         n'a pas de sens et rendrait les pourcentages indefinis)
    static bool create(Mass capacity, FuelTank& out) noexcept;

    constexpr Mass capacity() const noexcept { return capacity_; }
    constexpr Mass quantity() const noexcept { return quantity_; }

    /// Pourcentage de remplissage, dans [0,0 ; 100,0].
    avio::f32 fill_ratio_percent() const noexcept;

    /// Ajoute du carburant. La quantite reellement ajoutee peut etre
    /// inferieure a la demande si le reservoir deborde.
    /// @return la masse effectivement ajoutee
    Mass add(Mass amount) noexcept;

    /// Preleve du carburant. La quantite reellement prelevee peut etre
    /// inferieure a la demande si le reservoir se vide.
    /// @return la masse effectivement prelevee
    Mass remove(Mass amount) noexcept;

    bool is_empty() const noexcept;
    bool is_full() const noexcept;

    /// Verifie l'invariant. En production, cette methode alimente un moniteur
    /// de coherence ; en test, elle est appelee apres chaque operation.
    bool invariant_holds() const noexcept;

private:
    constexpr FuelTank(Mass capacity, Mass quantity) noexcept
        : capacity_(capacity), quantity_(quantity) {}

    Mass capacity_;
    Mass quantity_;
};

}  // namespace mod04

#endif  // MOD04_UNITS_HPP
