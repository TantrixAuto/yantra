/// @file grammar_yglx.hpp
/// @brief This file contains all the lexer-related classes required for constructing the grammar's AST
/// This file contains the following major classes
/// - Atoms: A basic component of a Regex
/// - Transitions: State transitions in the regex FSM
/// - State: Regex FSM state

#pragma once

#include "util.hpp"
#include "filepos.hpp"

namespace yg {
struct Grammar;
}

namespace yglx {
struct Atom;

/// @brief This class represents a regex's wildcard atom primitive
/// .
struct WildCard{
    inline WildCard() {}
    inline std::string str(const bool& = false) const {
        return std::format(".");
    }
};

/// @brief This class represents a regex's large esc-class atom primitive
/// \\w, \\b
/// its called a large escape class because it could cover a large range
/// e.g: \\w could cover the entire unicode charset
struct LargeEscClass{
    const yg::Grammar& grammar;
    std::string checker;
    inline LargeEscClass(const yg::Grammar& g, const std::string& c) : grammar(g), checker{c} {}
    inline std::string str(const bool& = false) const {
        return std::format("{}", checker);
    }
};

/// @brief This class represents a regex's range atom primitive
/// A, A-Z
struct RangeClass{
    uint32_t ch1;
    uint32_t ch2;
    inline RangeClass(const uint32_t& c1, const uint32_t& c2) : ch1{c1}, ch2{c2} {}

    inline std::string str(const bool& md = false) const {
        return std::format("{}", getChString(ch1, ch2, md));
    }
};

/// @brief This class represents a regex's primitive atom
/// ., A, A-Z, \\w, \\b
struct Primitive {
    using Atom_t = std::variant<
        WildCard,
        LargeEscClass,
        RangeClass
        >;
    const FilePos pos;
    const Atom_t atom;
    inline Primitive(const FilePos& p, const Atom_t& a) : pos(p), atom(a) {}

    inline std::string str(const bool& md = false) const {
        return std::visit([&md](const auto& a){
            auto s = a.str(md);
            return std::format("${}", s);
        }, atom);
    }
};

/// @brief This class represents a regex's class atom
/// [A-Za-z], [^A-Za-z]
struct Class{
    const FilePos pos;
    bool negate = false;
    std::vector<Primitive::Atom_t> atoms;
    inline Class(const FilePos& p, const bool& n, std::vector<Primitive::Atom_t>&& a)
        : pos(p), negate{n}, atoms(std::move(a)) {}

    inline std::string str(const bool& md = false) const {
        std::stringstream ss;
        ss << "[";
        if(negate == true) {
            ss << "^";
        }
        for(auto& ax : atoms) {
            std::visit([&ss, &md](const auto& a){ss << a.str(md);}, ax);
        }
        ss << "]";

        return ss.str();
    }
};

/// @brief This class represents a regex's sequence atom (`and`)
/// AB
struct Sequence{
    const FilePos pos;
    std::unique_ptr<const Atom> lhs;
    std::unique_ptr<const Atom> rhs;
    inline Sequence(const FilePos& p, std::unique_ptr<const Atom> l, std::unique_ptr<const Atom> r)
        : pos(p), lhs{std::move(l)}, rhs{std::move(r)} {}
    inline std::string str(const bool& = false) const {
        return std::format("&");
    }
};

/// @brief This class represents a regex's disjunct atom (`or`)
/// A|B
struct Disjunct{
    const FilePos pos;
    std::unique_ptr<const Atom> lhs;
    std::unique_ptr<const Atom> rhs;
    inline Disjunct(const FilePos& p, std::unique_ptr<const Atom> l, std::unique_ptr<const Atom> r)
        : pos(p), lhs{std::move(l)}, rhs{std::move(r)} {}

    inline std::string str(const bool& = false) const {
        return std::format("|");
    }
};

/// @brief This class represents a regex's group
/// (ABC), (!ABC)
struct Group{
    const FilePos pos;
    bool capture = true;
    std::unique_ptr<const Atom> atom;
    inline Group(const FilePos& p, const bool& c, std::unique_ptr<const Atom>&& a)
        : pos(p), capture{c}, atom{std::move(a)} {}
    inline std::string str(const bool& = false) const {
        return std::format("()");
    }
};

/// @brief This class represents a regex's closure operator
/// A*, A+, A{2,8}
struct Closure{
    const yg::Grammar& grammar;
    const FilePos pos;
    std::unique_ptr<const Atom> atom;
    size_t min = 0;
    size_t max = 0;
    inline Closure(const yg::Grammar& g, const FilePos& p, std::unique_ptr<const Atom> n, const size_t& mn, const size_t& mx)
        : grammar(g), pos(p), atom{std::move(n)}, min{mn}, max{mx} {}

    std::string str(const bool& md = false) const;
};

/// @brief variant that holds all regex atoms
using Atom_t = std::variant<
    Primitive,
    Sequence,
    Disjunct,
    Class,
    Group,
    Closure
    >;

/// @brief This class holds a single atom of a regex
struct Atom : public NonCopyable {
    Atom_t atom;
    inline Atom(Atom_t&& a) : atom{std::move(a)} {}
    inline const FilePos& pos() const {
        return std::visit([](const auto& a) -> const FilePos& {return a.pos;}, atom);
    }
};

/// @brief compare(Wildcard, LargeEscClass) function
/// a Wildcard is always less than a LargeEscClass
static inline int32_t compare(const yglx::WildCard&, const yglx::LargeEscClass&) {
    return -1;
}

/// @brief compare(Wildcard, RangeClass) function
/// a Wildcard is always less than a RangeClass
static inline int32_t compare(const yglx::WildCard&, const yglx::RangeClass&) {
    return -1;
}

/// @brief compare(LargeEscClass, RangeClass) function
/// a LargeEscClass is always less than a RangeClass
static inline int32_t compare(const yglx::LargeEscClass&, const yglx::RangeClass&) {
    return -1;
}

/// @brief compare(LargeEscClass, WildCard) function
/// a LargeEscClass is always less than a WildCard
static inline int32_t compare(const yglx::LargeEscClass&, const yglx::WildCard&) {
    return +1;
}

/// @brief compare(RangeClass, WildCard) function
/// a RangeClass is always less than a WildCard
static inline int32_t compare(const yglx::RangeClass&, const yglx::WildCard&) {
    return +1;
}

/// @brief compare(RangeClass, LargeEscClass) function
/// a RangeClass is always less than a LargeEscClass
static inline int32_t compare(const yglx::RangeClass&, const yglx::LargeEscClass&) {
    return +1;
}

/// @brief compare(WildCard, WildCard) function
/// a Wildcard is always same as a WildCard
static inline int32_t compare(const yglx::WildCard&, const yglx::WildCard&) {
    return 0;
}

/// @brief compare(RangeClass, RangeClass) function
/// compare width of both RangeClass'es
static inline int32_t compare(const yglx::RangeClass& lhs, const yglx::RangeClass& rhs) {
    if((lhs.ch1 == rhs.ch1) && (lhs.ch2 == rhs.ch2)) {
        return 0;
    }
    if((lhs.ch1 >= rhs.ch1) && (lhs.ch2 <= rhs.ch2)) {
        return -1;
    }
    if((lhs.ch1 < rhs.ch1) && (lhs.ch2 > rhs.ch2)) {
        return 1;
    }
    if((lhs.ch2 - lhs.ch1) < (rhs.ch2 - rhs.ch1)) {
        return -1;
    }
    if((lhs.ch2 - lhs.ch1) > (rhs.ch2 - rhs.ch1)) {
        return 1;
    }
    if(lhs.ch1 < rhs.ch1) {
        return -1;
    }
    if(lhs.ch1 > rhs.ch1) {
        return -1;
    }
    std::println("compare:l={}, r={}", lhs.str(), rhs.str());
    assert(false);
    return -1;
}

/// @brief compare(LargeEscClass, LargeEscClass) function
/// compare checker of both LargeEscClass'es
static inline int32_t compare(const yglx::LargeEscClass& lhs, const yglx::LargeEscClass& rhs) {
    if(lhs.checker < rhs.checker) {
        return -1;
    }
    if(lhs.checker > rhs.checker) {
        return +1;
    }
    return 0;
}

/// @brief compare(Class, Class) function
/// compare checker of both Class'es
static inline int32_t compare(const yglx::Class& lhs, const yglx::Class& rhs) {
    if(lhs.atoms.size() < rhs.atoms.size()) {
        return -1;
    }
    if(lhs.atoms.size() > rhs.atoms.size()) {
        return +1;
    }
    for(auto& lax : lhs.atoms) {
        auto c0 = std::visit([&rhs](const auto& la){
            for(auto& rax : rhs.atoms) {
                auto c1 = std::visit([&la](const auto& ra){
                    return yglx::compare(la, ra);
                }, rax);
                if(c1 != 0) {
                    return c1;
                }
            }
            return 0;
        }, lax);
        if(c0 != 0) {
            return c0;
        }
    }
    return 0;
}

/// @brief this class represents a lexer state transition on a primitive atom
struct PrimitiveTransition{
    const yglx::Primitive& atom;
    inline PrimitiveTransition(const yglx::Primitive& a) : atom(a) {}
    inline std::string str(const bool& md = false) const {
        return atom.str(md);
    }
};

/// @brief this class represents a lexer state transition on a class atom
struct ClassTransition{
    const yglx::Class& atom;
    inline ClassTransition(const yglx::Class& a) : atom(a) {}
    inline std::string str(const bool& md = false) const {
        return atom.str(md);
    }
};

/// @brief this class represents a lexer state transition on a closure atom
struct ClosureTransition{
    enum class Type {
        Enter,
        PreLoop,
        InLoop,
        PostLoop,
        Leave
    };

    const yg::Grammar& grammar;
    const yglx::Closure& atom;
    const Type type = Type::InLoop;
    size_t initialCount = 0;

    inline ClosureTransition(const yg::Grammar& g, const yglx::Closure& a, const Type& t, const size_t& ic = 0) : grammar(g), atom(a), type(t), initialCount(ic) {}

    inline std::string str(const bool& md = false) const {
        switch(type) {
        case Type::Enter:
            if(md) {
                return std::format("\\>{}", initialCount);
            }
            return std::format(">{}", initialCount);
        case Type::PreLoop:
            assert(atom.min > 1);
            return std::format("{{{},{}}}", 0, atom.min - 1);
        case Type::InLoop:
            return atom.str(md);
        case Type::PostLoop:
            if(md) {
                return std::format("\\<\\<");
            }
            return std::format("<<");
        case Type::Leave:
            if(md) {
                return std::format("\\<");
            }
            return std::format("<");
        }
        assert(false);
        return std::format("??");
    }
};

/// @brief this class represents a lexer state transition without any trigger
/// if the lexer reaches a state that has an outgoing slide transition, it
/// automatically slides to the destination state of this transition.
struct SlideTransition{
    inline SlideTransition() {}
    inline std::string str(const bool& = false) const {
        return std::format("~");
    }
};

/// @brief variant holds alltransitions
using Transition_t = std::variant<
    PrimitiveTransition,
    ClassTransition,
    ClosureTransition,
    SlideTransition
    >;

struct State;

/// @brief class holds all transitions
struct Transition : public NonCopyable {
    /// @brief holds the transition
    Transition_t t;

    /// @brief holds the source state for this transition
    State* from = nullptr;

    /// @brief holds the target state for this transition
    State* next = nullptr;

    /// @brief whether the char that triggered the transition should be added to the terxt of the token
    bool capture = true;

    inline Transition(const Transition_t& x, State* f, State* n, const bool& c)
        : t(x), from(f), next{n}, capture(c) {}

    template<typename T>
    inline const T* get();

    /// @brief returns the Closure atom, if this is a Closure transition
    inline const Closure* isClosure() {
        if(auto tx = std::get_if<ClosureTransition>(&t)) {
            return &(tx->atom);
        }
        return nullptr;
    }

    /// @brief returns the ClosureTransition class, if this is a Closure transition
    inline ClosureTransition* isClosure(const ClosureTransition::Type& type) {
        if(auto lt = std::get_if<ClosureTransition>(&t)) {
            if(lt->type == type) {
                return lt;
            }
        }
        return nullptr;
    }

    /// @brief compare this transition to @arg rhs
    int32_t compare(const Transition& rhs) const;

    /// @brief return true if this transition is a subset of @arg rhs
    /// 'g' is a subset of 'a'-'z', etc
    bool isSubsetOf(const Transition& rhs) const;

    /// @brief return this transition in string format
    inline std::string str(const bool& md = false) const {
        if(t.valueless_by_exception()) {
            return "??";
        }

        auto st = std::visit([&md](const auto& xt){
            return xt.str(md);
        }, t);

        if(capture == false) {
            st = std::format("{}!", st);
        }
        if(!md) {
            return std::format("{}->{}->{}", zid(from), st, zid(next));
        }
        return st;
    }
};

struct Regex;

/// @brief this class represents a Lexer state
struct State : public NonCopyable {
    /// @brief unique id of this state
    size_t id = 0;

    /// @brief position of the regex of this state
    FilePos pos;

    /// @brief whether this is the root state for a lexer mode
    bool isRoot = false;

    /// @brief list of outgoing transitions from this state
    std::vector<Transition*> transitions;

    /// @brief list of outgoing super transitions from this state
    std::vector<Transition*> superTransitions;

    /// @brief list of outgoing shadow transitions from this state
    std::vector<Transition*> shadowTransitions;

    /// @brief the closure atom, if this state is part of a closure loop
    const Closure* closure = nullptr;

    /// @brief the closure state, if this state is part of a closure loop
    State* closureState = nullptr;

    /// @brief the enter-closure transition, if this state is part of a closure loop
    const Transition* enterClosureTransition = nullptr;

    /// @brief the leave-closure transition, if this state is part of a closure loop
    const Transition* leaveClosureTransition = nullptr;

    /// @brief the check-closure transition, if this state is part of a closure loop
    const Transition* checkClosureTransition = nullptr;

    /// @brief the start-closure transition, if this state is part of a closure loop
    const Transition* startClosureTransition = nullptr;

    /// @brief non-null if this state matches a regex
    Regex* matchedRegex = nullptr;

    /// @brief whether to check for EOF in this state
    bool checkEOF = false;

    inline State() {}

    /// @brief get enter-closure transition
    inline const Transition& enterClosureTx() const {
        assert(enterClosureTransition != nullptr);
        return *enterClosureTransition;
    }

    /// @brief get leave-closure transition
    inline const Transition& leaveClosureTx() const {
        assert(leaveClosureTransition != nullptr);
        return *leaveClosureTransition;
    }

    /// @brief get check-closure transition
    inline const Transition& checkClosureTx() const {
        assert(checkClosureTransition != nullptr);
        return *checkClosureTransition;
    }

    /// @brief get start-closure transition
    inline const Transition& startClosureTx() const {
        assert(startClosureTransition != nullptr);
        return *startClosureTransition;
    }

    /// @brief add a transition to this state
    inline Transition* addTransition(Transition* t) {
        assert(t);
        auto it = transitions.begin();
        auto ite = transitions.end();
        bool found = false;
        while(it != ite) {
            if((*it)->compare(*t) >= 0) {
                transitions.insert(it, t);
                found = true;
                break;
            }
            ++it;
        }
        if(!found) {
            transitions.push_back(t);
        }

        return t;
    }

    /// @brief get the specified transition from this state, if any
    template<typename T>
    inline Transition* getTransition(const T& pa) const {
        for(auto& tx : transitions) {
            if(auto a = tx->get<T>()) {
                if(yglx::compare(*a, pa) == 0) {
                    return tx;
                }
            }
        }
        return nullptr;
    }

    /// @brief get a closure transition from this state, if exists
    inline const ClosureTransition* getClosureTransition(const ClosureTransition::Type& type);
};

struct RegexSet;

/// @brief this class represents a TOKEN in the input grammar
struct Regex : public NonCopyable {
    /// @brief the lexermode change, if any, when this token is matched
    enum class ModeChange {
        /// @brief no LexerMode change
        None,

        /// @brief the next LexerMode to change to.
        /// e.g:
        /// ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
        /// Here, when `/*` is seen, change to ML_COMMENT_MODE lexer mode
        Next,

        /// @brief change to previous LexerMode in the LexerMode stack
        /// e.g:
        /// LEAVE_MLCOMMENT := "\*/"! [^];
        /// Here, when `*/` is seen, change to previous lexer mode
        /// This is useful for nested multi-line comments, etc
        Back,

        /// @brief change to root LexerMode
        /// e.g:
        /// END_MODE := "ABC" [];
        /// Here, when `ABC` is seen, change to default lexer mode.
        /// This change is rarely used
        Init,
    };

    /// @brief unique ID for this token
    size_t id = 0;

    /// @brief name of this token
    std::string regexName;

    /// @brief the RegexSet this token belongs to
    RegexSet* regexSet = nullptr;

    /// @brief root regex atom for this token
    std::unique_ptr<const yglx::Atom> atom;

    /// @brief position in grammar file where this token was defined
    FilePos pos;

    /// @brief LexerModein which this token was defined
    std::string mode;

    /// @brief LexerMode change action, if this token was recognised
    ModeChange modeChange = ModeChange::None;

    /// @brief next LexerMode to change to, if @ref modeCnage is @ref ModeChange::Next
    std::string nextMode;

    /// @brief number of times this token was used in the grammar
    size_t usageCount = 0;

    /// @brief whether this token was used in the grammar
    /// e.g:
    /// WS := "\s"!;
    /// If token is suffixed with a !, it is treated as a legitimately unused token, and no warning is issued
    bool unused = false;
};

/// @brief represents a set of tokens
/// e.g:
/// SL_COMMENT := "//.*\n"!;
/// SL_COMMENT := "--.*\n"!;
/// This will match all lines that start with a '//' or a '--', and recognise both as the same token 'SL_COMMENT'
struct RegexSet : public NonCopyable {
    /// @brief association for this token set
    enum class Assoc {
        /// @brief token has left association (default)
        Left,

        /// @brief token has right association
        Right,

        /// @brief token has no association
        None,
    };

    /// @brief unique id for this token set
    size_t id = 0;

    /// @brief name of this token set
    std::string name;

    /// @brief all the tokens in this token set
    std::vector<Regex*> regexes;

    /// @brief all the fallback token sets for this token set
    std::vector<RegexSet*> fallbacks;

    /// @brief precedence for this token set
    size_t precedence = 0;

    /// @brief default association for this token set
    Assoc assoc = Assoc::Left;

    /// @brief return cumulative usage count of all tokens in this set
    inline size_t usageCount() const {
        size_t c = 0;
        for(auto& rx : regexes) {
            c += rx->usageCount;
        }
        return c;
    }
};

/// @brief represents a Lexer mode
struct LexerMode : public NonCopyable {
    /// @brief root state of this lexer mode
    State* root = nullptr;
};

/// @brief returns the Wildcard atom, if this is a Wildcard transition
template<>
inline const WildCard* Transition::get<WildCard>() {
    if(auto tx = std::get_if<PrimitiveTransition>(&t)) {
        if(auto a = std::get_if<WildCard>(&(tx->atom.atom))) {
            return a;
        }
    }
    return nullptr;
}

/// @brief returns the LargeEscClass atom, if this is a LargeEscClass transition
template<>
inline const LargeEscClass* Transition::get<LargeEscClass>() {
    if(auto tx = std::get_if<PrimitiveTransition>(&t)) {
        if(auto a = std::get_if<LargeEscClass>(&(tx->atom.atom))) {
            return a;
        }
    }
    return nullptr;
}

/// @brief returns the RangeClass atom, if this is a RangeClass transition
template<>
inline const RangeClass* Transition::get<RangeClass>() {
    if(auto tx = std::get_if<PrimitiveTransition>(&t)) {
        if(auto a = std::get_if<RangeClass>(&(tx->atom.atom))) {
            return a;
        }
    }
    return nullptr;
}

/// @brief returns the Class atom, if this is a Class transition
template<>
inline const Class* Transition::get<Class>() {
    if(auto tx = std::get_if<ClassTransition>(&t)) {
        return &(tx->atom);
    }
    return nullptr;
}

/// @brief returns the ClosureTransition class, if this is a Closure transition
template<>
inline const ClosureTransition* Transition::get<ClosureTransition>() {
    if(auto tx = std::get_if<ClosureTransition>(&t)) {
        return tx;
    }
    return nullptr;
}

inline const ClosureTransition* State::getClosureTransition(const ClosureTransition::Type& type) {
    for(auto& tx : transitions) {
        if(auto t = tx->get<ClosureTransition>()) {
            if(t->type == type) {
                return t;
            }
        }
    }
    return nullptr;
}
}
