// =============================================================================
//  Module 06 -- polymorphisme STATIQUE (CRTP).
//
//  Le module 05 a montre le polymorphisme dynamique : une vtable, une
//  indirection par appel, un WCET egal a celui de la redefinition la plus
//  lente, et des objectifs DO-332 supplementaires.
//
//  Le CRTP (Curiously Recurring Template Pattern) offre la meme factorisation
//  de code SANS aucun cout a l'execution : la resolution se fait a la
//  compilation.
//
//      template <typename Derived>
//      class Base { ... static_cast<const Derived&>(*this) ... };
//
//      class Concret final : public Base<Concret> { ... };
//
//  La classe de base connait son derive PAR SON PARAMETRE DE TEMPLATE. Elle
//  peut donc l'appeler directement, sans vtable.
//
//  BILAN COMPARATIF
//  ----------------
//                          | dynamique (module 05) | statique (CRTP)
//    ----------------------+-----------------------+-------------------------
//    taille de l'objet     | + 1 pointeur (vptr)   | 0 octet
//    cout d'un appel       | indirection memoire   | inlinable
//    WCET                  | le pire des derives   | exact, par instanciation
//    types connus          | a l'execution         | a la compilation
//    conteneur heterogene  | possible              | IMPOSSIBLE
//    objectifs DO-332      | OO.6.7 (coherence)    | couverture par instanciation
//
//  Regle pratique : si l'ensemble des types est connu a la compilation -- ce
//  qui est le cas de presque tout systeme embarque certifie -- le CRTP est
//  preferable. Le dynamique se justifie surtout aux frontieres materielles
//  (banc de test contre calculateur reel).
// =============================================================================
#ifndef MOD06_STATIC_POLYMORPHISM_HPP
#define MOD06_STATIC_POLYMORPHISM_HPP

#include <avio/types.hpp>

namespace mod06 {

/// Base CRTP : elle fournit le comportement COMMUN, en s'appuyant sur les
/// primitives que chaque derive doit fournir.
///
/// Contrat impose au parametre `Derived` (verifie a l'instanciation) :
///   * avio::i32 raw_min_impl() const noexcept
///   * avio::i32 raw_max_impl() const noexcept
///   * avio::f32 value_min_impl() const noexcept
///   * avio::f32 value_max_impl() const noexcept
///   * avio::f32 to_engineering_impl(avio::i32) const noexcept
///
/// Si l'une manque, l'erreur survient A LA COMPILATION, au point
/// d'instanciation. C'est du "duck typing" statique : pas de declaration de
/// contrainte avant C++20, mais une verification totale a la compilation.
template <typename Derived>
class SensorBase {
public:
    avio::i32 raw_min() const noexcept { return derived().raw_min_impl(); }
    avio::i32 raw_max() const noexcept { return derived().raw_max_impl(); }
    avio::f32 value_min() const noexcept { return derived().value_min_impl(); }
    avio::f32 value_max() const noexcept { return derived().value_max_impl(); }

    /// Comportement commun ecrit UNE SEULE FOIS, comme avec une classe de base
    /// classique -- mais entierement resolu a la compilation.
    avio::f32 to_engineering(avio::i32 raw) const noexcept {
        return derived().to_engineering_impl(raw);
    }

    bool is_in_range(avio::i32 raw) const noexcept {
        return (raw >= raw_min()) && (raw <= raw_max());
    }

    /// Ecretage generique : disponible pour tous les derives, gratuitement.
    avio::f32 to_engineering_clamped(avio::i32 raw) const noexcept {
        const avio::f32 value = to_engineering(raw);
        if (value < value_min()) {
            return value_min();
        }
        if (value > value_max()) {
            return value_max();
        }
        return value;
    }

protected:
    // Constructeur protege : SensorBase n'est jamais instanciee seule.
    // Pas de destructeur virtuel ici -- et c'est CORRECT : aucune destruction
    // polymorphe n'est possible, puisqu'aucun pointeur vers SensorBase ne
    // designe jamais un objet derive de facon anonyme.
    SensorBase() noexcept = default;
    ~SensorBase() noexcept = default;
    // "Regle de 5" (module 03) : des qu'on declare l'une de ces operations,
    // on statue sur les cinq. clang-tidy le verifie
    // (cppcoreguidelines-special-member-functions).
    SensorBase(const SensorBase&) noexcept = default;
    SensorBase& operator=(const SensorBase&) noexcept = default;
    SensorBase(SensorBase&&) noexcept = default;
    SensorBase& operator=(SensorBase&&) noexcept = default;

private:
    const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }
};

/// Capteur de pression, version statique. Comparez avec
/// mod05::PressureSensor : meme comportement, sizeof different.
class StaticPressureSensor final : public SensorBase<StaticPressureSensor> {
public:
    avio::i32 raw_min_impl() const noexcept { return 0; }
    avio::i32 raw_max_impl() const noexcept { return 4095; }
    avio::f32 value_min_impl() const noexcept { return 0.0F; }
    avio::f32 value_max_impl() const noexcept { return 1200.0F; }
    avio::f32 to_engineering_impl(avio::i32 raw) const noexcept;
};

/// Capteur de temperature, version statique.
class StaticTemperatureSensor final : public SensorBase<StaticTemperatureSensor> {
public:
    avio::i32 raw_min_impl() const noexcept { return 0; }
    avio::i32 raw_max_impl() const noexcept { return 4095; }
    avio::f32 value_min_impl() const noexcept { return -60.0F; }
    avio::f32 value_max_impl() const noexcept { return 80.0F; }
    avio::f32 to_engineering_impl(avio::i32 raw) const noexcept;
};

/// Fonction generique : accepte n'importe quel capteur statique.
/// Notez qu'il n'y a AUCUN pointeur, AUCUNE vtable, et que le compilateur
/// peut tout inliner. Une instanciation est produite par type de capteur.
template <typename SensorT>
avio::f32 read_average(const SensorT& sensor, const avio::i32* samples,
                       avio::usize count) noexcept {
    if ((samples == nullptr) || (count == 0U)) {
        return sensor.value_min();
    }
    double sum = 0.0;
    for (avio::usize index = 0U; index < count; ++index) {
        sum += static_cast<double>(sensor.to_engineering_clamped(samples[index]));
    }
    return static_cast<avio::f32>(sum / static_cast<double>(count));
}

}  // namespace mod06

#endif  // MOD06_STATIC_POLYMORPHISM_HPP
