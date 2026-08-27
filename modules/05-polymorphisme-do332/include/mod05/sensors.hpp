// =============================================================================
//  Module 05 -- polymorphisme dynamique et supplement DO-332.
//
//  La DO-332 ("Object-Oriented Technology and Related Techniques Supplement")
//  ne condamne pas l'orientation objet : elle en identifie les VULNERABILITES
//  et ajoute des objectifs pour les couvrir. Les six sujets traites :
//
//    1. heritage et polymorphisme  -> COHERENCE LOCALE DE TYPE (objectif OO.6.7)
//    2. polymorphisme parametrique (templates) -> module 06
//    3. surcharge (overloading)    -> risque d'ambiguite de resolution
//    4. conversions de type        -> upcast/downcast, RTTI
//    5. gestion dynamique memoire  -> module 08
//    6. gestion des exceptions     -> module 07
//
//  CE MODULE TRAITE LE POINT 1, le plus structurant.
//
//  COHERENCE LOCALE DE TYPE (Local Type Consistency)
//  -------------------------------------------------
//  Formulation DO-332 : partout ou une reference vers un type T est utilisee,
//  toute instance d'un sous-type de T doit se comporter de facon conforme au
//  CONTRAT de T. C'est le principe de substitution de Liskov (LSP), transforme
//  en objectif de certification.
//
//  La norme propose deux moyens de le demontrer :
//    a) VERIFICATION FORMELLE du LSP (pre-conditions non renforcees,
//       post-conditions non affaiblies, invariants preserves) ;
//    b) TEST PESSIMISTE : rejouer, pour CHAQUE sous-type, la totalite des cas
//       de test ecrits pour le type de base.
//
//  Le moyen (b) est le plus courant, et c'est celui que ce module outille :
//  `verify_contract()` est un harnais de test qui applique le contrat de
//  `Sensor` a n'importe quel sous-type. Les tests l'appliquent a chaque
//  capteur concret -- et a un capteur volontairement fautif, pour prouver que
//  le harnais detecte reellement les violations.
// =============================================================================
#ifndef MOD05_SENSORS_HPP
#define MOD05_SENSORS_HPP

#include <avio/types.hpp>
#include <type_traits>

namespace mod05 {

// -----------------------------------------------------------------------------
//  1. Le contrat de la classe de base
// -----------------------------------------------------------------------------
//  CONTRAT DE Sensor (a recopier tel quel dans le SDD) :
//
//    C1  name() renvoie une chaine non nulle, non vide, constante dans le temps.
//    C2  raw_min() < raw_max().
//    C3  value_min() < value_max().
//    C4  to_engineering(raw) est definie pour tout raw dans [raw_min, raw_max]
//        et son resultat appartient a [value_min, value_max].
//    C5  to_engineering est MONOTONE CROISSANTE sur [raw_min, raw_max].
//    C6  to_engineering(raw_min) == value_min et
//        to_engineering(raw_max) == value_max, a la tolerance pres.
//    C7  Pour raw hors de [raw_min, raw_max], le resultat est ECRETE aux bornes
//        (robustesse : aucune valeur aberrante ne sort du capteur).
//
//  TOUT sous-type doit respecter ces sept clauses. C'est cela, la coherence
//  locale de type.
// -----------------------------------------------------------------------------
class Sensor {
public:
    virtual ~Sensor() noexcept = default;

    // Une classe de base polymorphe ne doit pas etre copiable : la copie d'un
    // objet derive a travers une reference de base produirait un DECOUPAGE
    // (slicing). On interdit donc les quatre operations.
    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    Sensor(Sensor&&) = delete;
    Sensor& operator=(Sensor&&) = delete;

    virtual const char* name() const noexcept = 0;
    virtual avio::i32 raw_min() const noexcept = 0;
    virtual avio::i32 raw_max() const noexcept = 0;
    virtual avio::f32 value_min() const noexcept = 0;
    virtual avio::f32 value_max() const noexcept = 0;

    /// Conversion brut -> grandeur physique. Voir clauses C4 a C7.
    virtual avio::f32 to_engineering(avio::i32 raw) const noexcept = 0;

    /// Methode NON virtuelle : comportement commun, identique pour tous les
    /// sous-types. La rendre virtuelle serait ouvrir une porte inutile.
    bool is_in_range(avio::i32 raw) const noexcept;

protected:
    Sensor() noexcept = default;
};

// Contrat verifie A LA COMPILATION : sans destructeur virtuel, toute
// destruction polymorphe serait un comportement indefini.
static_assert(std::has_virtual_destructor_v<Sensor>,
              "Sensor est une classe de base polymorphe : destructeur virtuel obligatoire");

// -----------------------------------------------------------------------------
//  2. Sous-types conformes
// -----------------------------------------------------------------------------

/// Capteur de pression 12 bits : 0..4095 -> 0..1200 hPa (lineaire).
class PressureSensor final : public Sensor {
public:
    PressureSensor() noexcept = default;

    const char* name() const noexcept override;
    avio::i32 raw_min() const noexcept override;
    avio::i32 raw_max() const noexcept override;
    avio::f32 value_min() const noexcept override;
    avio::f32 value_max() const noexcept override;
    avio::f32 to_engineering(avio::i32 raw) const noexcept override;
};

/// Capteur de temperature 12 bits : 0..4095 -> -60..+80 degres Celsius.
class TemperatureSensor final : public Sensor {
public:
    TemperatureSensor() noexcept = default;

    const char* name() const noexcept override;
    avio::i32 raw_min() const noexcept override;
    avio::i32 raw_max() const noexcept override;
    avio::f32 value_min() const noexcept override;
    avio::f32 value_max() const noexcept override;
    avio::f32 to_engineering(avio::i32 raw) const noexcept override;
};

// -----------------------------------------------------------------------------
//  3. Sous-type VOLONTAIREMENT NON CONFORME
// -----------------------------------------------------------------------------
//  Ce capteur compile, s'utilise, et passe tous les tests "nominaux" que l'on
//  ecrirait spontanement. Il viole pourtant deux clauses du contrat :
//    * C5 : sa conversion n'est pas monotone au-dela d'un seuil ;
//    * C7 : il ne borne pas son resultat pour les entrees hors domaine.
//
//  C'est exactement le type de defaut que la DO-332 vise : le compilateur ne
//  peut rien voir, seule une verification de la coherence locale de type le
//  detecte. Il n'existe ici QUE pour prouver que notre harnais fonctionne.
// -----------------------------------------------------------------------------
class BrokenSensor final : public Sensor {
public:
    BrokenSensor() noexcept = default;

    const char* name() const noexcept override;
    avio::i32 raw_min() const noexcept override;
    avio::i32 raw_max() const noexcept override;
    avio::f32 value_min() const noexcept override;
    avio::f32 value_max() const noexcept override;
    avio::f32 to_engineering(avio::i32 raw) const noexcept override;
};

// -----------------------------------------------------------------------------
//  4. Harnais de coherence locale de type
// -----------------------------------------------------------------------------

/// Resultat detaille de la verification du contrat.
struct ContractReport {
    bool c1_name_valid = false;
    bool c2_raw_domain_valid = false;
    bool c3_value_domain_valid = false;
    bool c4_result_in_domain = false;
    bool c5_monotonic = false;
    bool c6_endpoints_match = false;
    bool c7_clamped_outside = false;

    /// Vrai si les sept clauses sont satisfaites.
    bool all_satisfied() const noexcept;

    /// Identifiant de la premiere clause violee ("C1".."C7"), ou nullptr.
    const char* first_violation() const noexcept;
};

/// Applique le contrat de `Sensor` a N'IMPORTE QUEL sous-type.
///
/// C'est la mise en oeuvre concrete du "test pessimiste" de la DO-332 :
/// une seule campagne de test, rejouee sur chaque sous-type. Ajouter un
/// nouveau capteur au systeme coute alors UNE ligne de test.
///
/// @param sensor      instance a verifier (reference vers la BASE : c'est le
///                    point ou la substitution a lieu)
/// @param sample_count nombre de points d'echantillonnage du domaine (>= 2)
ContractReport verify_contract(const Sensor& sensor, avio::u32 sample_count) noexcept;

// -----------------------------------------------------------------------------
//  5. Decoupage (slicing) : le piege de la copie polymorphe
// -----------------------------------------------------------------------------
//  Ces deux types sont COPIABLES, contrairement a Sensor : ils servent
//  uniquement a montrer ce qui se passe quand on l'autorise.
// -----------------------------------------------------------------------------

class Message {
public:
    explicit Message(avio::u16 identifier) noexcept : identifier_(identifier) {}
    virtual ~Message() noexcept = default;
    Message(const Message&) noexcept = default;
    Message& operator=(const Message&) noexcept = default;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;

    avio::u16 identifier() const noexcept { return identifier_; }

    /// Longueur de la trame en octets. Redefinie par les sous-types.
    virtual avio::u16 length() const noexcept { return 4U; }

private:
    avio::u16 identifier_;
};

class ExtendedMessage final : public Message {
public:
    ExtendedMessage(avio::u16 identifier, avio::u16 payload_length) noexcept
        : Message(identifier), payload_length_(payload_length) {}

    avio::u16 length() const noexcept override {
        return static_cast<avio::u16>(4U + payload_length_);
    }

private:
    avio::u16 payload_length_;
};

/// Passage PAR VALEUR : l'objet derive est DECOUPE, seule la partie `Message`
/// survit. La resolution dynamique disparait avec elle.
// NOLINTNEXTLINE(performance-unnecessary-value-param) -- decoupage volontaire
avio::u16 length_by_value(Message message) noexcept;

/// Passage PAR REFERENCE : le type dynamique est preserve, `length()` appelle
/// bien la version du sous-type.
avio::u16 length_by_reference(const Message& message) noexcept;

}  // namespace mod05

#endif  // MOD05_SENSORS_HPP
