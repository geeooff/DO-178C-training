// =============================================================================
//  Module 03 -- RAII, cycle de vie des objets, semantique de deplacement.
//
//  RAII = Resource Acquisition Is Initialization.
//  L'idee tient en une phrase : LA DUREE DE VIE D'UNE RESSOURCE EST CELLE D'UN
//  OBJET. On acquiert dans le constructeur, on libere dans le destructeur, et
//  le langage garantit l'appel du destructeur a la sortie de portee -- par
//  retour normal, par `return` anticipe, ou par exception.
//
//  Pourquoi c'est central en avionique :
//    * la liberation est DETERMINISTE (a l'instruction pres), contrairement a
//      un finaliseur C# dont on ne sait pas quand il s'execute ;
//    * elle est EXHAUSTIVE : impossible d'oublier un chemin de sortie ;
//    * elle est LOCALE : l'acquisition et la liberation sont dans le meme type,
//      donc revisibles ensemble.
//
//  Comparaison C# :
//      using (var l = new Lock()) { ... }   // il faut PENSER a ecrire `using`
//      { ScopedLock l; ... }                // C++ : impossible d'oublier
//  Un `Dispose()` non appele en C# est un bug silencieux. En C++, le
//  destructeur est appele, point.
// =============================================================================
#ifndef MOD03_LIFETIME_HPP
#define MOD03_LIFETIME_HPP

#include <avio/types.hpp>

namespace mod03 {

// -----------------------------------------------------------------------------
//  1. Observation du cycle de vie
// -----------------------------------------------------------------------------

/// Journal statique des evenements de cycle de vie. Permet de PROUVER par
/// test que les constructeurs et destructeurs sont appeles au bon moment et
/// dans le bon ordre -- une exigence de determinisme, pas une curiosite.
class LifetimeLog {
public:
    enum class Event : avio::u8 {
        Construct = 0U,
        Copy = 1U,
        Move = 2U,
        CopyAssign = 3U,
        MoveAssign = 4U,
        Destroy = 5U
    };

    static constexpr avio::usize kCapacity = 64U;

    static void reset() noexcept;
    static void record(Event event, avio::i32 tag) noexcept;

    static avio::usize count() noexcept;
    static bool entry(avio::usize index, Event& out_event, avio::i32& out_tag) noexcept;

    /// Nombre d'occurrences d'un type d'evenement.
    static avio::u32 count_of(Event event) noexcept;

    /// Vrai si le nombre de constructions (toutes formes) egale le nombre de
    /// destructions : aucune fuite, aucune double liberation.
    static bool is_balanced() noexcept;

    static const char* event_name(Event event) noexcept;
};

/// Objet temoin : chaque operation de son cycle de vie est journalisee.
/// Il implemente la "regle de 5" au complet, ce qui permet d'observer
/// precisement quelle operation le compilateur choisit.
class Traced {
public:
    explicit Traced(avio::i32 tag) noexcept;
    Traced(const Traced& other) noexcept;
    Traced(Traced&& other) noexcept;
    Traced& operator=(const Traced& other) noexcept;
    Traced& operator=(Traced&& other) noexcept;
    ~Traced() noexcept;

    avio::i32 tag() const noexcept { return tag_; }

private:
    avio::i32 tag_;
};

// -----------------------------------------------------------------------------
//  2. RAII sur une ressource materielle simulee
// -----------------------------------------------------------------------------

/// Peripherique simule : un banc de N canaux que l'on peut reserver.
/// Represente n'importe quelle ressource exclusive reelle : canal DMA,
/// section critique, verrou de bus, page de memoire non volatile.
class DeviceBank {
public:
    static constexpr avio::u8 kChannelCount = 4U;

    static void reset() noexcept;

    /// Reserve un canal libre. Renvoie 0 si aucun n'est disponible
    /// (0 = identifiant invalide, jamais un canal valide).
    static avio::u8 acquire() noexcept;

    /// Libere un canal precedemment reserve.
    static void release(avio::u8 channel) noexcept;

    static bool is_acquired(avio::u8 channel) noexcept;
    static avio::u8 acquired_count() noexcept;

    /// Compteurs cumules : permettent de verifier l'equilibre acquisitions /
    /// liberations sur toute la duree d'un test.
    static avio::u32 total_acquisitions() noexcept;
    static avio::u32 total_releases() noexcept;
};

/// Poignee RAII sur un canal du DeviceBank.
///
/// TYPE A PROPRIETE UNIQUE (move-only) :
///   * copie INTERDITE  -> deux poignees ne peuvent pas liberer le meme canal ;
///   * deplacement AUTORISE -> la propriete se transfere explicitement.
/// C'est la "regle de 5" appliquee : en declarant un destructeur, on doit
/// statuer sur les quatre autres operations speciales.
class ChannelHandle {
public:
    /// Reserve un canal. Verifier is_valid() apres construction.
    ChannelHandle() noexcept;

    /// Libere le canal s'il est detenu. Ne peut pas echouer, ne lance rien.
    ~ChannelHandle() noexcept;

    ChannelHandle(const ChannelHandle&) = delete;
    ChannelHandle& operator=(const ChannelHandle&) = delete;

    ChannelHandle(ChannelHandle&& other) noexcept;
    ChannelHandle& operator=(ChannelHandle&& other) noexcept;

    bool is_valid() const noexcept { return channel_ != 0U; }
    avio::u8 channel() const noexcept { return channel_; }

    /// Libere explicitement, avant la fin de portee. Idempotent.
    void release() noexcept;

private:
    avio::u8 channel_;
};

// -----------------------------------------------------------------------------
//  3. Section critique
// -----------------------------------------------------------------------------

/// Etat global des interruptions (simule). Dans un vrai calculateur, ce sont
/// des instructions processeur.
class InterruptState {
public:
    static void reset() noexcept;
    static bool enabled() noexcept;
    static void disable() noexcept;
    static void enable() noexcept;

    /// Profondeur maximale d'imbrication atteinte : sert a verifier que les
    /// sections critiques restent courtes et correctement imbriquees.
    static avio::u32 max_nesting() noexcept;

private:
    friend class CriticalSection;
    static void enter() noexcept;
    static void leave() noexcept;
};

/// Garde RAII de section critique. Le motif le plus courant en embarque.
///
/// L'interet decisif : quel que soit le chemin de sortie (return anticipe,
/// break, goto), les interruptions sont reactivees. Un oubli de
/// `enable_interrupts()` sur une branche d'erreur est un defaut classique...
/// et impossible avec ce motif.
class CriticalSection {
public:
    CriticalSection() noexcept;
    ~CriticalSection() noexcept;

    CriticalSection(const CriticalSection&) = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;
    CriticalSection(CriticalSection&&) = delete;
    CriticalSection& operator=(CriticalSection&&) = delete;
};

// -----------------------------------------------------------------------------
//  4. Fonctions de demonstration utilisees par les tests
// -----------------------------------------------------------------------------

/// Effectue un traitement avec plusieurs sorties possibles, sous section
/// critique. Sert a prouver que la ressource est liberee sur TOUS les chemins.
/// @return 0 si `value` est negatif, 1 s'il est nul, 2 s'il est positif.
avio::i32 multi_exit_processing(avio::i32 value) noexcept;

/// Cree deux objets temoins dans une portee : sert a observer que la
/// destruction se fait dans l'ordre INVERSE de la construction.
void demonstrate_destruction_order() noexcept;

}  // namespace mod03

#endif  // MOD03_LIFETIME_HPP
