/// @file grammar.hpp
/// @brief This file contains all the classes required for constructing the grammar's AST
/// This file contains the following major classes
/// - Atoms: A basic component of a Regex
/// - Transitions: State transitions in the regex FSM
/// - State: Regex FSM state
/// - Walkers: class for traversing AST

#pragma once
#include "util.hpp"
#include "error.hpp"
#include "codeblock.hpp"

/// @brief This class contains the grammar's AST
struct Grammar {
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
        const Grammar& grammar;
        std::string checker;
        inline LargeEscClass(const Grammar& g, const std::string& c) : grammar(g), checker{c} {}
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
        const Grammar& grammar;
        const FilePos pos;
        std::unique_ptr<const Atom> atom;
        size_t min = 0;
        size_t max = 0;
        inline Closure(const Grammar& g, const FilePos& p, std::unique_ptr<const Atom> n, const size_t& mn, const size_t& mx)
            : grammar(g), pos(p), atom{std::move(n)}, min{mn}, max{mx} {}

        inline std::string str(const bool& md = false) const {
            if(max == grammar.maxRepCount) {
                if(min == 0) {
                    if(md) {
                        return std::format("\\*");
                    }
                    return std::format("*");
                }
                if(min == 1) {
                    if(md) {
                        return std::format("\\+");
                    }
                    return std::format("+");
                }
                return std::format("{{{},}}", min);
            }
            return std::format("{{{},{}}}", min, max);
        }
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
    static inline int32_t compare(const Grammar::WildCard&, const Grammar::LargeEscClass&) {
        return -1;
    }

    /// @brief compare(Wildcard, RangeClass) function
    /// a Wildcard is always less than a RangeClass
    static inline int32_t compare(const Grammar::WildCard&, const Grammar::RangeClass&) {
        return -1;
    }

    /// @brief compare(LargeEscClass, RangeClass) function
    /// a LargeEscClass is always less than a RangeClass
    static inline int32_t compare(const Grammar::LargeEscClass&, const Grammar::RangeClass&) {
        return -1;
    }

    /// @brief compare(LargeEscClass, WildCard) function
    /// a LargeEscClass is always less than a WildCard
    static inline int32_t compare(const Grammar::LargeEscClass&, const Grammar::WildCard&) {
        return +1;
    }

    /// @brief compare(RangeClass, WildCard) function
    /// a RangeClass is always less than a WildCard
    static inline int32_t compare(const Grammar::RangeClass&, const Grammar::WildCard&) {
        return +1;
    }

    /// @brief compare(RangeClass, LargeEscClass) function
    /// a RangeClass is always less than a LargeEscClass
    static inline int32_t compare(const Grammar::RangeClass&, const Grammar::LargeEscClass&) {
        return +1;
    }

    /// @brief compare(WildCard, WildCard) function
    /// a Wildcard is always same as a WildCard
    static inline int32_t compare(const Grammar::WildCard&, const Grammar::WildCard&) {
        return 0;
    }

    /// @brief compare(RangeClass, RangeClass) function
    /// compare width of both RangeClass'es
    static inline int32_t compare(const Grammar::RangeClass& lhs, const Grammar::RangeClass& rhs) {
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
        print("compare:l={}, r={}", lhs.str(), rhs.str());
        assert(false);
        return -1;
    }

    /// @brief compare(LargeEscClass, LargeEscClass) function
    /// compare checker of both LargeEscClass'es
    static inline int32_t compare(const Grammar::LargeEscClass& lhs, const Grammar::LargeEscClass& rhs) {
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
    static inline int32_t compare(const Grammar::Class& lhs, const Grammar::Class& rhs) {
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
                        return Grammar::compare(la, ra);
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
        const Grammar::Primitive& atom;
        inline PrimitiveTransition(const Grammar::Primitive& a) : atom(a) {}
        inline std::string str(const bool& md = false) const {
            return atom.str(md);
        }
    };

    /// @brief this class represents a lexer state transition on a class atom
    struct ClassTransition{
        const Grammar::Class& atom;
        inline ClassTransition(const Grammar::Class& a) : atom(a) {}
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

        const Grammar& grammar;
        const Grammar::Closure& atom;
        const Type type = Type::InLoop;
        size_t initialCount = 0;

        inline ClosureTransition(const Grammar& g, const Grammar::Closure& a, const Type& t, const size_t& ic = 0) : grammar(g), atom(a), type(t), initialCount(ic) {}

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

        /// @brief returns the Wildcard atom, if this is a Wildcard transition
        template<>
        inline const WildCard* get<WildCard>() {
            if(auto tx = std::get_if<PrimitiveTransition>(&t)) {
                if(auto a = std::get_if<WildCard>(&(tx->atom.atom))) {
                    return a;
                }
            }
            return nullptr;
        }

        /// @brief returns the LargeEscClass atom, if this is a LargeEscClass transition
        template<>
        inline const LargeEscClass* get<LargeEscClass>() {
            if(auto tx = std::get_if<PrimitiveTransition>(&t)) {
                if(auto a = std::get_if<LargeEscClass>(&(tx->atom.atom))) {
                    return a;
                }
            }
            return nullptr;
        }

        /// @brief returns the RangeClass atom, if this is a RangeClass transition
        template<>
        inline const RangeClass* get<RangeClass>() {
            if(auto tx = std::get_if<PrimitiveTransition>(&t)) {
                if(auto a = std::get_if<RangeClass>(&(tx->atom.atom))) {
                    return a;
                }
            }
            return nullptr;
        }

        /// @brief returns the Class atom, if this is a Class transition
        template<>
        inline const Class* get<Class>() {
            if(auto tx = std::get_if<ClassTransition>(&t)) {
                return &(tx->atom);
            }
            return nullptr;
        }

        /// @brief returns the ClosureTransition class, if this is a Closure transition
        template<>
        inline const ClosureTransition* get<ClosureTransition>() {
            if(auto tx = std::get_if<ClosureTransition>(&t)) {
                return tx;
            }
            return nullptr;
        }

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
                    if(Grammar::compare(*a, pa) == 0) {
                        return tx;
                    }
                }
            }
            return nullptr;
        }

        /// @brief get a closure transition from this state, if exists
        inline const ClosureTransition* getClosureTransition(const ClosureTransition::Type& type) {
            for(auto& tx : transitions) {
                if(auto t = tx->get<ClosureTransition>()) {
                    if(t->type == type) {
                        return t;
                    }
                }
            }
            return nullptr;
        }
    };

    struct RegexSet;
    struct Rule;
    struct RuleSet;

    /// @brief this class represents a Walker, used to traverse an AST
    struct Walker : public NonCopyable {
        /// @brief the traversal mode
        enum class TraversalMode {
            /// @brief the parent nodes must explicitly call the child nodes
            Manual,

            /// @brief the parent nodes automatically call the child nodes,
            /// unless explicitly called by the semantic action
            TopDown,
        };

        /// @brief output type for this walker
        enum class OutputType {
            /// @brief no output
            None,

            /// @brief this walker generates a text file
            TextFile,

            /// @brief this walker generates a binary file
            BinaryFile,
        };

        /// @brief function signature for semantic actions in this Walker
        struct FunctionSig {
            /// @brief name of the function
            std::string func;

            /// @brief extra args for this function
            std::string args;

            /// @brief return type of this function
            std::string type;

            /// @brief whether this function is a UDF
            bool isUDF = false;

            /// @brief whether this function should be autowalked
            /// Can be used to override the Walker-level autowalk setting
            bool autowalk = false;
        };

        /// @brief code blocks for semantic actions in this Walker
        struct CodeInfo {
            /// @brief name of the function
            std::string func;

            /// @brief code block of this function
            CodeBlock codeblock;
        };

        /// @brief name of the walker
        std::string name;

        /// @brief base walker of the walker
        const Walker* base = nullptr;

        /// @brief addition user-define members in this walker
        CodeBlock xmembers;

        /// @brief name of the void type in this walker
        /// This is used as the type for functions that don't have a return type
        std::string voidClass = "void";

        /// @brief the default function name used to traverse the walker
        std::string defaultFunctionName = "go";

        /// @brief default traversal mode of the walker
        TraversalMode traversalMode = TraversalMode::TopDown;

        /// @brief default output type of the walker
        OutputType outputType = OutputType::None;

        /// @brief instance name for the writer object in the walker, if nahy
        std::string writerName = "out";

        /// @brief file extension for files genrated by this walker, if any
        std::string ext;

        /// @brief function sig for the default 'go' function of this walker
        std::unique_ptr<FunctionSig> defaultFunctionSig;

        /// @brief all function sigs defined in this walker, mapped by RuleSet
        std::unordered_map<const RuleSet*, std::vector<FunctionSig>> functionSigs;

        /// @brief all codeblocks defined in this walker, mapped by Rule
        std::unordered_map<const Rule*, std::vector<CodeInfo>> codeblocks;

        inline Walker(const std::string& n, const Walker* b) : name(n), base(b) {}

        /// @brief set traversal mode for this walker
        inline void setTraversalMode(const TraversalMode& m) {
            traversalMode = m;
        }

        /// @brief set output mode to text file, for this walker
        inline void setOutputTextFile(const std::string& e) {
            outputType = OutputType::TextFile;
            ext = e;
        }

        /// @brief set default function sig for this walker
        inline void init() {
            assert(!defaultFunctionSig);
            if(!defaultFunctionSig) {
                defaultFunctionSig = std::make_unique<FunctionSig>();
                defaultFunctionSig->func = defaultFunctionName;
                defaultFunctionSig->type = voidClass;
            }
        }

        /// @brief check if this walker, or any of it base walkers, have a function sig
        inline const FunctionSig* hasFunctionSig(const RuleSet& rs, const std::string& func) const {
            if(auto it = functionSigs.find(&rs); it != functionSigs.end()) {
                auto& tlist = it->second;
                for(auto& t : tlist) {
                    if(t.func == func) {
                        return &t;
                    }
                }
            }
            if(base != nullptr) {
                return base->hasFunctionSig(rs, func);
            }
            return nullptr;
        }

        /// @brief get list of all function sigs defined for specified RuleSet, in this walker,
        inline std::vector<const FunctionSig*> getFunctions(const RuleSet& rs) const {
            std::vector<const FunctionSig*> l;

            bool defaultFunctionDefined = false;
            for(auto& fsig : functionSigs) {
                if(fsig.first->name == rs.name) {
                    for(auto& i : fsig.second) {
                        if(i.func.size() > 0) {
                            l.push_back(&i);
                            if(i.func == defaultFunctionName) {
                                defaultFunctionDefined = true;
                            }
                        }
                    }
                }
            }

            if(defaultFunctionDefined == false) {
                assert(defaultFunctionSig);
                l.push_back(defaultFunctionSig.get());
            }

            return l;
        }

        /// @brief add function sig in this walker,
        inline void addFunctionSig(
            const FilePos& npos,
            const RuleSet& rs,
            const bool& isUDF,
            const std::string& func,
            const std::string& args,
            const std::string& type,
            const bool& autowalk
        ) {
            if(hasFunctionSig(rs, func) != nullptr) {
                throw GeneratorError(__LINE__, __FILE__, npos, "DUPLICATE_FUNCTION:{}/{}::{}", rs.name, name, func);
            }
            auto& tlist = functionSigs[&rs];
            tlist.push_back(FunctionSig());
            auto& ti = tlist.back();
            ti.isUDF = isUDF;
            ti.func = func;
            ti.args = args;
            ti.type = type;
            ti.autowalk = autowalk;
        }

        /// @brief check if a codeblock is defined for specified function in this walker,
        inline const CodeInfo* hasCodeblock(const Rule& r, const std::string& func) const {
            if(auto it = codeblocks.find(&r); it != codeblocks.end()) {
                auto& tlist = it->second;
                for(auto& t : tlist) {
                    if(t.func == func) {
                        return &t;
                    }
                }
            }
            return nullptr;
        }

        /// @brief add codeblock for a Rule, in this walker
        inline void addCodeblock(
            const FilePos& npos,
            const Rule& r,
            const std::string& func,
            const std::string& codeblock
        ) {
            if((hasFunctionSig(*(r.ruleSet), func) == nullptr) && (func.size() > 0) && (func != defaultFunctionName)) {
                throw GeneratorError(__LINE__, __FILE__, npos, "UNKNOWN_FUNCTION:{}", func);
            }
            if(hasCodeblock(r, func) != nullptr) {
                throw GeneratorError(__LINE__, __FILE__, npos, "DUPLICATE_CODEBLOCK:{}::{}::{}", r.ruleSetName(), name, func);
            }
            auto& tlist = codeblocks[&r];
            tlist.push_back(CodeInfo());
            auto& ci = tlist.back();
            ci.func = func;
            ci.codeblock.setCode(npos, codeblock);
        }
    };

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
        std::unique_ptr<const Atom> atom;

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

    /// @brief represents a node in a production
    struct Node : public NonCopyable {
        /// @brief whether this node refers to a ruleset or a tokenset
        enum class NodeType {
            /// @brief this node refers to a ruleset
            RuleRef,

            /// @brief this node refers to a tokenset
            RegexRef,
        };

        /// @brief this node type
        NodeType type = NodeType::RegexRef;

        /// @brief position of this node in grammar file
        FilePos pos;

        /// @brief name of this node
        std::string name;

        /// @brief variable name of this node
        std::string varName;

        /// @brief autogenerated variable name of this node
        std::string idxName;

        /// @brief return true if this is a ruleset reference
        inline bool isRule() const {
            return type == NodeType::RuleRef;
        }

        /// @brief return true if this is a tokenset reference
        inline bool isRegex() const {
            return type == NodeType::RegexRef;
        }

        /// @brief return string representation of this node
        /// useful for debugging
        inline std::string str() const {
            if(varName.size() > 0) {
                return std::format("{}({})", name, varName);
            }
            return std::format("{}", name);
        }
    };

    /// @brief this class represents a rule in the grammar
    struct Rule : public NonCopyable {
        /// @brief unique id for this rule
        size_t id = 0;

        /// @brief position in grammar file where this rule is defined
        FilePos pos;

        /// @brief name of this rule
        std::string ruleName;

        /// @brief the ruleset this rule belongs to
        RuleSet* ruleSet = nullptr;

        /// @brief list of nodes in this rule
        std::vector<std::unique_ptr<Node>> nodes;

        /// @brief index of anchor node in this rule
        size_t anchor = 0;

        /// @brief this rule takes on the precedence of this token
        const RegexSet* precedence = nullptr;

        /// @brief name of this rule's ruleset
        inline const std::string& ruleSetName() const {
            assert(ruleSet != nullptr);
            return ruleSet->name;
        }

        /// @brief get node at index specified by @arg idx
        inline Node& getNode(const size_t& idx) const {
            assert(idx < nodes.size());
            auto& node = nodes.at(idx);
            return *node;
        }

        /// @brief get node at index specified by @arg idx, or null if idx is out of range
        inline Node* getNodeAt(const size_t& idx) const {
            if(idx >= nodes.size()) {
                assert(idx == nodes.size());
                return nullptr;
            }
            assert(idx < nodes.size());
            auto& node = nodes.at(idx);
            return node.get();
        }

        /// @brief add node to rule
        inline Node& addNode(const FilePos& npos, const std::string& name, const Grammar::Node::NodeType& type) {
            auto pnode = std::make_unique<Node>();
            nodes.push_back(std::move(pnode));
            auto& node = *(nodes.back());
            node.type = type;
            node.pos = npos;
            node.name = name;
            node.idxName = std::format("{}{}", name, nodes.size() - 1);
            return node;
        }

        /// @brief add regex node to rule
        inline Node& addRegexNode(const FilePos& npos, const std::string& name) {
            return addNode(npos, name, Node::NodeType::RegexRef);
        }

        /// @brief return string representation of this rule
        /// useful for debugging
        inline std::string str(const size_t& cpos, const bool& full = true) const {
            std::stringstream s;
            for(size_t idx = 0; idx <= nodes.size(); ++idx) {
                s << " ";
                if(idx == cpos) {
                    s << ".";
                }
                if(idx == anchor) {
                    s << "^";
                }
                if(idx < nodes.size()) {
                    auto& node = getNode(idx);
                    s << node.str();
                }else{
                    s << "@";
                }
            }

            if(full == true) {
                assert(ruleSet != nullptr);
                s << ", {";
                std::string sep;
                for(auto& t : ruleSet->firsts) {
                    s << sep << t->name;
                    sep = " ";
                }
                s << "}, {";
                sep = "";
                for(auto& t : ruleSet->follows) {
                    s << sep << t->name;
                    sep = " ";
                }
                s << "}";

                s << " [";
                if(precedence != nullptr) {
                    s << precedence->name << "/" << precedence->id;
                }else{
                    s << "-";
                }
                s << "]";
            }
            return std::format("{}({}) :={};", ruleSetName(), ruleName, s.str());
        }

        /// @brief return string representation of this rule
        /// useful for debugging
        inline std::string str(const bool& full = true) const {
            return str(nodes.size() + 1, full);
        }
    };

    /// @brief represents a set of rules that reduce to the same rule name
    /// e.g:
    /// stmts := stmts stmt;
    /// stmts := stmt;
    struct RuleSet : public NonCopyable {
        /// @brief unique id for this ruleset
        size_t id = 0;

        /// @brief name of this ruleset
        std::string name;

        /// @brief all the rules in this ruleset
        std::vector<Rule*> rules;

        /// @brief all the tokens in the FIRST-SET of this ruleset
        /// used for constructing the LALR state machine
        std::vector<RegexSet*> firsts;

        /// @brief all the tokens in the FOLLOW-SET of this ruleset
        /// used for constructing the LALR state machine
        std::vector<RegexSet*> follows;

        /// @brief true if @arg list contains @arg name
        static inline bool hasToken(const std::vector<RegexSet*>& list, const std::string& name) {
            for(auto& rx : list) {
                if(rx->name == name) {
                    return true;
                }
            }
            return false;
        }

        /// @brief true if FIRST-SET contains @arg t
        inline bool firstIncludes(const std::string& t) const {
            return hasToken(firsts, t);
        }
    };

    /// @brief represents a config for a rule
    /// this is used for constructing the LALR state machine
    /// it stores the @ref Rule and the position of the dot in the @ref Rule
    struct Config : public NonCopyable {
        /// @brief the rule wrapped by this config
        const Rule& rule;

        /// @brief the position of the dot in this config
        size_t cpos = 0;

        inline Config(const Rule& r, const size_t& p) : rule(r), cpos(p) {}

        /// @brief return the next node from the position of the dot in this config
        inline Node& getNextNode() const {
            return rule.getNode(cpos);
        }

        /// @brief return string representation of this config
        /// used for debugging and logging
        inline std::string str(const bool& full = true) const {
            return rule.str(cpos, full);
        }
    };

    /// @brief represents a set of configs
    /// This is an intermediate data structure used during the
    /// construction of ItemSets in the LALR state machine
    /// TODO: This class is almost identical to ItemSet, chek if opssible to merge.
    struct ConfigSet : public NonCopyable {
        /// @brief represents all SHIFT actions from this config set
        struct Shift {
            /// @brief the next config to shift to
            std::vector<const Config*> next;

            /// @brief epsilon transitions from this ConfigSet
            std::vector<const RuleSet*> epsilons;
        };

        /// @brief represents all REDUCE actions from this config set
        struct Reduce {
            /// @brief the next config to shift when this REDUCE action is executed
            /// a ConfigSet can have multiple reduces for the same REGEX.
            /// this is resolved when converting to ItemSet.
            /// hence, a vector here.
            std::vector<const Config*> next;

            /// @brief length of the Rule to REDUCE
            size_t len = 0;
        };

        /// @brief list of SHIFT actions from this config set
        std::unordered_map<const RegexSet*, Shift> shifts;

        /// @brief list of REDUCE actions from this config set
        std::unordered_map<const RegexSet*, Reduce> reduces;

        /// @brief list of GOTO actions from this config set
        std::unordered_map<const RuleSet*, std::vector<const Config*>> gotos;

        /// @brief check if there is a SHIFT action for the given token @arg rx
        inline const Shift* hasShift(const RegexSet& rx) const {
            if(auto it = shifts.find(&rx); it != shifts.end()) {
                return &(it->second);
            }
            return nullptr;
        }

        /// @brief add a SHIFT action for the given token @arg rx, from current Config to @arg next Config
        inline void addShift(const RegexSet& rx, const Config& next) {
            if(hasShift(rx) == nullptr) {
                shifts[&rx] = Shift();
            }
            shifts[&rx].next.push_back(&next);
        }

        /// @brief add a SHIFT action for the given token @arg rx, from current Config to @arg next Config
        /// along with @arg epsilon tranitions
        inline void addShift(const RegexSet& rx, const Config& next, const std::vector<const RuleSet*>& epsilons) {
            if(hasShift(rx) == nullptr) {
                shifts[&rx] = Shift();
                shifts[&rx].epsilons = epsilons;
            }
            shifts[&rx].next.push_back(&next);
        }

        /// @brief move SHIFTs from another Config to here
        /// This is used during LALR construction, to collapse similar Configs into one
        inline void moveShifts(const RegexSet& rx, std::vector<const Config*>& nexts, const std::vector<const RuleSet*>& epsilons) {
            if(hasShift(rx) == nullptr) {
                shifts[&rx] = Shift();
                shifts[&rx].epsilons = epsilons;
            }
            shifts[&rx].next = std::move(nexts);
        }

        /// @brief check if there is a REDUCE action for the given token @arg rx
        inline const Reduce* hasReduce(const RegexSet& rx) const {
            if(auto it = reduces.find(&rx); it != reduces.end()) {
                return &(it->second);
            }
            return nullptr;
        }

        /// @brief add a REDUCE action for the given token @arg rx
        inline void addReduce(const RegexSet& rx, const Config& next, const size_t& len) {
            if(hasReduce(rx) == nullptr) {
                Reduce r;
                r.len = len;
                reduces[&rx] = r;
            }
            reduces[&rx].next.push_back(&next);
        }

        /// @brief check if there is a GOTO action for the given RuleSet @arg rs
        inline bool hasGoto(const RuleSet& rs, const Config& cfg) const {
            if(auto it = gotos.find(&rs); it != gotos.end()) {
                for(auto& c : it->second) {
                    if(c == &cfg) {
                        return true;
                    }
                }
            }
            return false;
        }
    };

    /// @brief This class reprsents a state in the LALR state machine
    struct ItemSet : public NonCopyable {
        /// @brief represents all SHIFT actions from this ItemSet
        struct Shift {
            /// @brief the next ItemSet to shift to
            ItemSet* next = nullptr;

            /// @brief epsilon transitions from this ItemSet
            std::vector<const RuleSet*> epsilons;
        };

        /// @brief represents all REDUCE actions from this config set
        struct Reduce {
            /// @brief the next config to shift when this REDUCE action is executed
            const Config* next = nullptr;

            /// @brief length of the Rule to REDUCE
            size_t len = 0;
        };

        size_t id = 0;

        /// @brief this is the list of configs for this item
        std::vector<const Config*> configs;

        /// @brief this is the list of processed config-sets
        Grammar::ConfigSet configSet;

        /// @brief list of SHIFT actions from this ItemSet
        std::unordered_map<const RegexSet*, Shift> shifts;

        /// @brief list of REDUCE actions from this ItemSet
        std::unordered_map<const RegexSet*, Reduce> reduces;

        /// @brief list of GOTO actions from this ItemSet
        std::unordered_map<const RuleSet*, ItemSet*> gotos;

        /// @brief check if there is a GOTO action for the given RuleSet @arg rs
        inline ItemSet* hasGoto(const RuleSet* rs) const {
            if(auto it = gotos.find(rs); it != gotos.end()) {
                return it->second;
            }
            return nullptr;
        }

        /// @brief set GOTO action for the given RuleSet @arg rs
        inline void setGoto(const Node& node, const RuleSet* rs, ItemSet* is) {
            if(hasGoto(rs)) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "GOTO_CONFLICT:{}", rs->name);
            }
            gotos[rs] = is;
        }

        /// @brief check if there is a SHIFT action for the given token @arg rx
        inline ItemSet* hasShift(const RegexSet& rx) const {
            if(auto it = shifts.find(&rx); it != shifts.end()) {
                return it->second.next;
            }
            return nullptr;
        }

        /// @brief check if there is a SHIFT action for the given token @arg rx, leading to given ItemSet @arg is
        inline ItemSet* hasShift(const RegexSet& rx, ItemSet& is) const {
            auto is2 = hasShift(rx);
            if((is2 == nullptr) || (is2 == &is)) {
                return nullptr;
            }
            return is2;
        }

        /// @brief delete SHIFT action for the given token @arg rx
        /// This is used when resolving a SHIFT-REDUCE conflict in
        /// favour of a REDUCE
        inline void delShift(const Node& node, const RegexSet& rx) {
            if(hasShift(rx) == nullptr) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "UNKNOWN_SHIFT:{}", rx.name);
            }
            shifts.erase(&rx);
        }

        /// @brief check if there is a REDUCE action for the given token @arg rx
        inline const Config* hasReduce(const RegexSet& rx) const {
            if(auto it = reduces.find(&rx); it != reduces.end()) {
                return it->second.next;
            }
            return nullptr;
        }

        /// @brief check if there is a REDUCE action for the given token @arg rx, leading to given Config @arg c
        inline const Config* hasReduce(const RegexSet& rx, const Config& c) const {
            auto c2 = hasReduce(rx);
            if((c2 == nullptr) || (c2 == &c)) {
                return nullptr;
            }
            return c2;
        }

        /// @brief delete REDUCE action for the given token @arg rx
        /// This is used when resolving a SHIFT-REDUCE conflict in
        /// favour of a SHIFT
        inline void delReduce(const Node& node, const RegexSet& rx) {
            if(hasReduce(rx) == nullptr) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "UNKNOWN_REDUCE:{}", rx.name);
            }
            reduces.erase(&rx);
        }

        /// @brief add a SHIFT action for the given token @arg rx
        /// from current Config to @arg is ItemSet
        /// along with @arg epsilons transitions
        inline void setShift(const Node& node, const RegexSet& rx, ItemSet& is, const std::vector<const Grammar::RuleSet*>& epsilons) {
            if(hasShift(rx, is) != nullptr) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "SHIFT_SHIFT_CONFLICT:{}", rx.name);
            }
            if(hasReduce(rx) != nullptr) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "SHIFT_REDUCE_CONFLICT:{}", rx.name);
            }
            auto& s = shifts[&rx];
            s.next = &is;
            s.epsilons = epsilons;
        }

        /// @brief add a REDUCE action for the given token @arg rx
        /// from current Config to @arg is ItemSet
        /// along with @arg len length of the Rule to reduce
        inline void setReduce(const Node& node, const RegexSet& rx, const Config& c, const size_t& len) {
            if(hasShift(rx)) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "REDUCE_SHIFT_CONFLICT:{}", rx.name);
            }
            if(hasReduce(rx, c)) {
                throw GeneratorError(__LINE__, __FILE__, node.pos, "REDUCE_REDUCE_CONFLICT:{}", rx.name);
            }
            Reduce rd;
            rd.next = &c;
            rd.len = len;
            reduces[&rx] = rd;
        }

        /// @brief return string representation of this ItemSet
        /// used for debugging and logging
        inline std::string str(const std::string& indent = "", const std::string& nl = "\n", const bool& full = true) const {
            std::stringstream ss;
            ss << indent << std::format("ItemSet:{}", id);
            for(auto& c : configs) {
                ss << nl << indent << c->str(full);
            }
            ss << nl;
            std::string sep;
            for(auto& s : shifts) {
                ss << sep << s.first->name <<  " -> S" << s.second.next->id;
                sep = ", ";
            }
            for(auto& s : reduces) {
                ss << sep << s.first->name <<  " -> R" << s.second.next->rule.id;
                sep = ", ";
            }
            for(auto& s : gotos) {
                ss << sep << s.first->name <<  " -> G" << s.second->id;
                sep = ", ";
            }
            return ss.str();
        }
    };

    std::string ns = "";
    std::string className = "YantraModule";
    std::vector<std::string> classMembers;

    std::string defaultWalkerClassName = "Walker";
    Walker* defaultWalkerClass = nullptr;

    std::string tokenClass = "Token";
    std::string astClass = "AbSynTree";
    std::string defaultMode = "";
    std::string start = "start";
    std::string end = "_tEND";
    std::string empty = "_tEMPTY";
    std::string tokenType = "std::string";
    std::string listType = "std::vector";
    bool hasREPL = true;

    CodeBlock prologue;
    CodeBlock epilogue;
    CodeBlock throwError;
    bool checkUnusedTokens = true;
    bool autoResolve = true;
    bool warnResolve = true;
    bool unicodeEnabled = true;
    size_t smallRangeSize = 16;
    size_t maxRepCount = 65535;

    bool stdHeadersEnabled = true;
    std::string pchHeader;
    std::vector<std::string> hdrHeaders;
    std::vector<std::string> srcHeaders;

    std::vector<std::unique_ptr<Walker>> walkers;
    std::vector<std::unique_ptr<Regex>> regexes;
    std::vector<std::unique_ptr<RegexSet>> regexSets;

    std::unordered_map<std::string, std::unique_ptr<LexerMode>> lexerModes;
    std::vector<std::unique_ptr<State>> states;
    std::vector<std::unique_ptr<Transition>> transitions;

    std::vector<std::unique_ptr<Rule>> rules;
    std::vector<std::unique_ptr<RuleSet>> ruleSets;
    std::vector<std::unique_ptr<Config>> configs;
    std::vector<std::unique_ptr<ItemSet>> itemSets;

    ItemSet* initialState = nullptr;

    inline const Walker* hasDefaultWalker() const {
        return defaultWalkerClass;
    }

    inline Walker* hasDefaultWalker() {
        return defaultWalkerClass;
    }

    inline const Walker& getDefaultWalker() const {
        assert(defaultWalkerClass != nullptr);
        return *defaultWalkerClass;
    }

    inline Walker& getDefaultWalker() {
        assert(defaultWalkerClass != nullptr);
        return *defaultWalkerClass;
    }

    inline bool isWalker(const Walker& walker) const {
        for(auto& w : walkers) {
            if(w.get() == &walker) {
                return true;
            }
        }
        return false;
    }

    // not derived from any walker
    inline bool isRootWalker(const Walker& walker) const {
         return walker.base == nullptr;
    }

    // is derived from another walker
    inline bool isDerivedWalker(const Walker& walker) const {
        return walker.base != nullptr;
    }

    // has 1 or more derived walkers
    inline bool isBaseWalker(const Walker& walker) const { //TODO: can this be replaced with isRootWalker?
        for(auto& w : walkers) {
            if(w->base == &walker) {
                return true;
            }
        }
        return false;
    }

    inline const Walker* getWalker(const std::string& name) const {
        for(auto& w : walkers) {
            if(w->name == name) {
                return w.get();
            }
        }
        return nullptr;
    }

    inline Walker* getWalker(const std::string& name) {
        auto& This = static_cast<const Grammar&>(*this);
        auto walker = This.getWalker(name);
        return const_cast<Walker*>(walker);
    }

    inline void resetWalkers() {
        walkers.clear();
        defaultWalkerClassName = "";
        defaultWalkerClass = nullptr;
    }

    inline Walker* addWalker(const std::string& name, const Walker* base) {
        assert(getWalker(name) == nullptr);
        walkers.push_back(std::make_unique<Walker>(name, base));
        auto& lw = walkers.back();
        if(defaultWalkerClass == nullptr) {
            defaultWalkerClassName = name;
            defaultWalkerClass = lw.get();
        }
        return lw.get();
    }

    inline void setDefaultWalker(const FilePos& npos, const std::string& name) {
        if(auto w = getWalker(name)) {
            defaultWalkerClassName = name;
            defaultWalkerClass = w;
        }else{
            throw GeneratorError(__LINE__, __FILE__, npos, "UNKNOWN_WALKER:{}", name);
        }
    }

    inline State* createNewState(const FilePos& p) {
        states.push_back(std::make_unique<State>());
        auto& s = *(states.back());
        s.id = states.size();
        s.pos = p;
        return &s;
    }

    inline void releaseState(const State* s) {
        assert(s != nullptr);
        assert(s->id == states.size());
        assert(s->id == states.back()->id);
        assert(s->transitions.size() == 0);
        states.pop_back();
    }

    inline void errorizeState(const State* target) {
        assert(target != nullptr);
        assert(target->transitions.size() == 0);

        // verify target state is the last state
        assert(target->id == states.size());
        assert(target->id == states.back()->id);

        // redirect all transitions
        for(auto& t : transitions) {
            if(t->next == target) {
                t->next = nullptr;
            }
        }

        // release source state.
        states.pop_back();
    }

    inline void redirectState(State* target, const State* source) {
        assert(target != nullptr);
        assert(source != nullptr);
        assert(source != target);
        assert(source->transitions.size() == 0);
        // assert(target->transitions.size() == 0);

        // verify source state is the last state
        assert(source->id == states.size());
        assert(source->id == states.back()->id);

        // redirect all transitions
        for(auto& t : transitions) {
            if(t->next == source) {
                t->next = target;
            }
        }

        // release source state.
        states.pop_back();
    }

    inline Transition* _addTransition(std::unique_ptr<Transition>& tx) {
        Transition* t = tx.get();
        auto it = transitions.begin();
        auto ite = transitions.end();
        bool found = false;
        while(it != ite) {
            if((*it)->compare(*tx) >= 0) {
                transitions.insert(it, std::move(tx));
                found = true;
                break;
            }
            ++it;
        }
        if(!found) {
            transitions.push_back(std::move(tx));
        }
        return t;
    }

    inline Transition* _addTransitionToState(State* s, std::unique_ptr<Transition>& tx) {
        auto t = _addTransition(tx);
        s->addTransition(t);
        return t;
    }

    inline Transition* cloneTransition(const Transition& tx, State* s, State* n) {
        auto nt = std::make_unique<Transition>(tx.t, s, n, tx.capture);
        auto t = _addTransition(nt);
        return t;
    }

    inline Transition* addClassTransition(const Grammar::Class& a, State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(ClassTransition(a), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline Transition* addEnterClosureTransition(const Grammar::Closure& a, State* s, State* n, const bool& c, const size_t& ic) {
        auto t = std::make_unique<Transition>(ClosureTransition(*this, a, ClosureTransition::Type::Enter, ic), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline Transition* addPreLoopTransition(const Grammar::Closure& a, State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(ClosureTransition(*this, a, ClosureTransition::Type::PreLoop), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline Transition* addInLoopTransition(const Grammar::Closure& a, State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(ClosureTransition(*this, a, ClosureTransition::Type::InLoop), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline Transition* addPostLoopTransition(const Grammar::Closure& a, State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(ClosureTransition(*this, a, ClosureTransition::Type::PostLoop), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline Transition* addLeaveClosureTransition(const Grammar::Closure& a, State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(ClosureTransition(*this, a, ClosureTransition::Type::Leave), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline Transition* addSlideTransition(State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(SlideTransition(), s, n, c);
        auto tx = _addTransitionToState(s, t);
        return tx;
    }

    inline Transition* addTransition(const Grammar::Primitive& a, State* s, State* n, const bool& c) {
        auto t = std::make_unique<Transition>(PrimitiveTransition(a), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline RegexSet* hasRegexSet(const std::string& name) {
        for(auto& rs : regexSets) {
            if(rs->name == name) {
                return rs.get();
            }
        }
        return nullptr;
    }

    inline RegexSet* addRegexSet(const std::string& name, const RegexSet::Assoc& assoc) {
        regexSets.push_back(std::make_unique<RegexSet>());
        auto& regexSet = regexSets.back();
        regexSet->id = regexSets.size();
        regexSet->name = name;
        regexSet->precedence = regexSets.size();
        regexSet->assoc = assoc;
        return regexSet.get();
    }

    inline void addRegex(std::unique_ptr<Regex>& pregex, const RegexSet::Assoc& assoc) {
        regexes.push_back(std::move(pregex));
        auto& regex = *(regexes.back());
        regex.id = regexes.size();

        auto regexSet = hasRegexSet(regex.regexName);
        if(regexSet == nullptr) {
            regexSet = addRegexSet(regex.regexName, assoc);
        }

        regex.regexSet = regexSet;
        regexSet->regexes.push_back(&regex);
    }

    inline void addRegexByName(const std::string& name, const RegexSet::Assoc& assoc) {
        auto regex = std::make_unique<Grammar::Regex>();
        regex->regexName = name;
        addRegex(regex, assoc);
    }

    inline RegexSet* hasRegexSet(const std::string& name) const {
        for(auto& t : regexSets) {
            if(t->name == name) {
                return t.get();
            }
        }
        return nullptr;
    }

    inline RegexSet& getRegexSet(const Node& node) const {
        auto p = hasRegexSet(node.name);
        if(p == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, node.pos, "INVALID_TOKEN:{}", node.name);
        }
        return *p;
    }

    inline RegexSet& getRegexSetByName(const FilePos& npos, const std::string& name) const {
        auto p = hasRegexSet(name);
        if(p == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, npos, "INVALID_TOKEN:{}", name);
        }
        return *p;
    }

    inline void addLexerMode(const FilePos& npos, const std::string& name) {
        if (lexerModes.find(name) != lexerModes.end()) {
            throw GeneratorError(__LINE__, __FILE__, npos, "DUPLICATE_MODE:{}", name);
        }
        lexerModes[name] = std::make_unique<LexerMode>();
        auto& mode = *(lexerModes[name]);
        mode.root = createNewState(npos);
        mode.root->checkEOF = true;
        mode.root->isRoot = true;
    }

    inline LexerMode& getLexerMode(const Regex& regex) const {
        if (lexerModes.find(regex.mode) == lexerModes.end()) {
            throw GeneratorError(__LINE__, __FILE__, regex.pos, "UNKNOWN_MODE:{}", regex.mode);
        }
        auto& mode = lexerModes.at(regex.mode);
        assert(mode);
        return *mode;
    }

    inline LexerMode& getRegexNextMode(const Regex& regex) const {
        if (lexerModes.find(regex.nextMode) == lexerModes.end()) {
            throw GeneratorError(__LINE__, __FILE__, regex.pos, "UNKNOWN_MODE:{}", regex.nextMode);
        }
        auto& mode = lexerModes.at(regex.nextMode);
        assert(mode);
        return *mode;
    }

    inline Rule& addRule(const std::string& name, std::unique_ptr<Rule>& prule, const bool& anchorSet) {
        RuleSet* ruleSet = nullptr;
        for(auto& rs : ruleSets) {
            if(rs->name == name) {
                ruleSet = rs.get();
                break;
            }
        }
        if(ruleSet == nullptr) {
            ruleSets.push_back(std::make_unique<RuleSet>());
            ruleSet = ruleSets.back().get();
            ruleSet->id = ruleSets.size();
            ruleSet->name = name;
        }

        rules.push_back(std::move(prule));
        auto& rule = *(rules.back());
        rule.ruleSet = ruleSet;
        ruleSet->rules.push_back(&rule);
        rule.id = ruleSet->rules.size();

        // if anchor is not explicitly set, then set it to first regex node
        // if there are no regexes, leave it at first node.
        if(anchorSet == false) {
            size_t idx = 0;
            for(auto& n : rule.nodes) {
                if(n->isRegex()) {
                    rule.anchor = idx;
                    break;
                }
                ++idx;
            }
        }
        return rule;
    }

    inline RuleSet& getRuleSetByName(const FilePos& p, const std::string& name) const {
        for(auto& r : ruleSets) {
            if(r->name == name) {
                return *r;
            }
        }
        throw GeneratorError(__LINE__, __FILE__, p, "UNKNOWN_RULESET:{}", name);
    }

    inline const Config& createConfig(const Rule& r, const size_t& p) {
        assert(p <= r.nodes.size());
        for(auto& c : configs) {
            if((&(c->rule) == &r) && (c->cpos == p)) {
                return *c;
            }
        }
        configs.push_back(std::make_unique<Config>(r, p));
        auto& cfg = *(configs.back());
        return cfg;
    }

    inline ItemSet& createItemSet(std::vector<const Config*>& cfgs) {
        itemSets.push_back(std::make_unique<ItemSet>());
        auto& is = *(itemSets.back());
        is.id = itemSets.size();
        is.configs = std::move(cfgs);
        return is;
    }

    inline ItemSet* hasItemSet(const std::vector<const Config*>& cfgs) const {
        for(auto& is : itemSets) {
            if(is->configs.size() != cfgs.size()) {
                continue;
            }

            auto iit = is->configs.begin();
            auto iite = is->configs.end();
            auto cit = cfgs.begin();
            auto cite = cfgs.end();
            bool mismatch = false;
            while((iit != iite) && (cit != cite)) {
                auto& icfg = *(*iit);
                auto& ccfg = *(*cit);

                if(&(icfg.rule) != &(ccfg.rule)) {
                    mismatch = true;
                    break;
                }
                if(icfg.cpos != ccfg.cpos) {
                    mismatch = true;
                    break;
                }
                ++iit;
                ++cit;
            }
            if(mismatch) {
                continue;
            }
            return is.get();
        }
        return nullptr;
    }

    inline ItemSet& getItemSet(const FilePos& npos, const std::vector<const Config*>& cfgs) const {
        auto is = hasItemSet(cfgs);
        if(is == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, npos, "INVALID_ITEMSET");
        }
        return *is;
    }

    inline FilePos pos() const {
        FilePos npos;
        if(rules.size() > 0) {
            npos = rules.at(0)->pos;
        }
        return npos;
    }

    inline ~Grammar() {
        for(auto& gs : states) {
            gs->transitions.clear();
        }
    }
};
