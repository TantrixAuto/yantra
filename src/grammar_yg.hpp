/// @file grammar.hpp
/// @brief This file contains all the classes required for constructing the grammar's AST
/// This file contains the following major classes
/// - Atoms: A basic component of a Regex
/// - Transitions: State transitions in the regex FSM
/// - State: Regex FSM state
/// - Walkers: class for traversing AST

#pragma once
#include "grammar_yglx.hpp"
#include "grammar_ygp.hpp"
#include "codeblock.hpp"

/// @brief yg is short for Yantra Grammar
/// The classes in this namespace are the core of Yantra
/// The .y file is parsed into these classes, and
/// the output files are generated from these classes.
namespace yg {

/// @brief this class represents a Walker, used to traverse an AST
struct Walker : public NonCopyable {
    /// @brief the traversal mode
    enum class TraversalMode : uint8_t {
        /// @brief the parent nodes must explicitly call the child nodes
        Manual,

        /// @brief the parent nodes automatically call the child nodes,
        /// unless explicitly called by the semantic action
        TopDown,
    };

    /// @brief output type for this walker
    enum class OutputType : uint8_t {
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

    /// @brief additional ctor statements in this walker
    CodeBlock xctor;

    /// @brief additional ctor args in this walker
    std::string xctor_args;

    /// @brief additional user-define members in this walker
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

    /// @brief the external inteface for the walker, if any
    std::string interfaceName;

    /// @brief instance name for the writer object in the walker, if nahy
    std::string writerName = "out";

    /// @brief file extension for files genrated by this walker, if any
    std::string ext;

    /// @brief function sig for the default 'go' function of this walker
    std::unique_ptr<FunctionSig> defaultFunctionSig;

    /// @brief all function sigs defined in this walker, mapped by RuleSet
    std::unordered_map<const ygp::RuleSet*, std::vector<FunctionSig>> functionSigs;

    /// @brief all codeblocks defined in this walker, mapped by Rule
    std::unordered_map<const ygp::Rule*, std::vector<CodeInfo>> codeblocks;

    inline Walker(std::string n, const Walker* b) : name(std::move(n)), base(b) {}

    /// @brief set traversal mode for this walker
    inline void setTraversalMode(const TraversalMode& m) {
        traversalMode = m;
    }

    /// @brief set output mode to text file, for this walker
    inline void setOutputTextFile(const std::string& e) {
        outputType = OutputType::TextFile;
        ext = e;
    }

    /// @brief set interface name for this walker
    inline void setInterfaceName(const std::string& n) {
        interfaceName = n;
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
    inline auto
    hasFunctionSig(
        const ygp::RuleSet& rs,
        const std::string& func
    ) const -> const FunctionSig* {
        if(auto it = functionSigs.find(&rs); it != functionSigs.end()) {
            const auto& tlist = it->second;
            for(const auto& t : tlist) {
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

    /// @brief check if this walker, or any of it base walkers, have a function sig
    inline auto
    getRuleSetNativeType(
        const ygp::RuleSet& rs,
        const std::string& func,
        const std::string& tokenClass
    ) const -> std::string {
        auto fsig = hasFunctionSig(rs, func); // NOLINT(readability-qualified-auto)
        if(fsig != nullptr) {
            return fsig->type;
        }
        return tokenClass;
    }

    /// @brief get list of all function sigs defined for specified RuleSet, in this walker,
    inline auto getFunctions(const ygp::RuleSet& rs) const -> std::vector<const FunctionSig*>{
        std::vector<const FunctionSig*> l;

        bool defaultFunctionDefined = false;
        for(const auto& fsig : functionSigs) {
            if(fsig.first->name == rs.name) {
                for(const auto& i : fsig.second) {
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
        const ygp::RuleSet& rs,
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
        tlist.emplace_back();
        auto& ti = tlist.back();
        ti.isUDF = isUDF;
        ti.func = func;
        ti.args = args;
        ti.type = type;
        ti.autowalk = autowalk;
    }

    /// @brief check if a codeblock is defined for specified function in this walker,
    inline auto
    hasCodeblock(
        const ygp::Rule& r,
        const std::string& func
    ) const -> const CodeInfo* {
        if(auto it = codeblocks.find(&r); it != codeblocks.end()) {
            const auto& tlist = it->second;
            for(const auto& t : tlist) {
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
        const ygp::Rule& r,
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
        tlist.emplace_back();
        auto& ci = tlist.back();
        ci.func = func;
        ci.codeblock.setCode(npos, codeblock);
    }
};

/// @brief This class contains the grammar's AST
struct Grammar : public NonCopyable { // NOLINT(cppcoreguidelines-special-member-functions)
    std::string ns;
    std::string className = "YantraModule";
    std::vector<std::string> classMembers;

    std::string defaultWalkerClassName = "Walker";
    Walker* defaultWalkerClass = nullptr;

    std::string tokenClass = "Token";
    std::string astClass = "AbSynTree";
    std::string defaultMode;
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
    bool checkEmptyCodeblocks = true;
    bool errorUnusedTokens = true;
    bool errorEmptyCodeblocks = false;
    bool autoResolve = true;
    bool warnResolve = true;
    bool unicodeEnabled = true;
    size_t smallRangeSize = 16; // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    size_t maxRepCount = 65535; // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

    bool stdHeadersEnabled = true;
    std::string pchHeader;
    std::vector<std::string> hdrHeaders;
    std::vector<std::string> srcHeaders;

    std::vector<std::unique_ptr<Walker>> walkers;
    std::vector<std::unique_ptr<yglx::Regex>> regexes;
    std::vector<std::unique_ptr<yglx::RegexSet>> regexSets;
    size_t nextPrecedence = 0;

    std::unordered_map<std::string, std::unique_ptr<yglx::LexerMode>> lexerModes;
    std::vector<std::unique_ptr<yglx::State>> states;
    std::vector<std::unique_ptr<yglx::Transition>> transitions;

    std::vector<std::unique_ptr<ygp::Rule>> rules;
    std::vector<std::unique_ptr<ygp::RuleSet>> ruleSets;
    std::vector<std::unique_ptr<ygp::Config>> configs;
    std::vector<std::unique_ptr<ygp::ItemSet>> itemSets;

    ygp::ItemSet* initialState = nullptr;

    inline auto hasDefaultWalker() const -> const Walker* {
        return defaultWalkerClass;
    }

    inline auto hasDefaultWalker() -> Walker* { // NOLINT(readability-make-member-function-const)
        return defaultWalkerClass;
    }

    inline auto getDefaultWalker() const -> const Walker& {
        assert(defaultWalkerClass != nullptr);
        return *defaultWalkerClass;
    }

    inline auto getDefaultWalker() -> Walker& { // NOLINT(readability-make-member-function-const)
        assert(defaultWalkerClass != nullptr);
        return *defaultWalkerClass;
    }

    inline auto isWalker(const Walker& walker) const -> bool {
        for(const auto& w : walkers) {
            if(w.get() == &walker) {
                return true;
            }
        }
        return false;
    }

    // not derived from any walker
    static inline auto
    isRootWalker(const Walker& walker) -> bool {
         return walker.base == nullptr;
    }

    // is derived from another walker
    static inline auto
    isDerivedWalker(const Walker& walker) -> bool {
        return walker.base != nullptr;
    }

    // has 1 or more derived walkers
    inline auto isBaseWalker(const Walker& walker) const -> bool { //TODO: can this be replaced with isRootWalker?
        for(const auto& w : walkers) {
            if(w->base == &walker) {
                return true;
            }
        }
        return false;
    }

    inline auto getWalker(const std::string& name) const -> const Walker* {
        for(const auto& w : walkers) {
            if(w->name == name) {
                return w.get();
            }
        }
        return nullptr;
    }

    inline auto getWalker(const std::string& name) -> Walker*{
        const auto& This = static_cast<const Grammar&>(*this);
        const auto* walker = This.getWalker(name);
        return const_cast<Walker*>(walker); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    inline void resetWalkers() {
        walkers.clear();
        defaultWalkerClassName = "";
        defaultWalkerClass = nullptr;
    }

    inline auto addWalker(const std::string& name, const Walker* base) -> Walker* {
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
        if(auto* w = getWalker(name)) {
            defaultWalkerClassName = name;
            defaultWalkerClass = w;
        }else{
            throw GeneratorError(__LINE__, __FILE__, npos, "UNKNOWN_WALKER:{}", name);
        }
    }

    inline auto createNewState(const FilePos& p) -> yglx::State* {
        states.push_back(std::make_unique<yglx::State>());
        auto& s = *(states.back());
        s.id = states.size();
        s.pos = p;
        return &s;
    }

    inline void releaseState([[maybe_unused]] const yglx::State* s) {
        assert(s != nullptr);
        assert(s->id == states.size());
        assert(s->id == states.back()->id);
        assert(s->transitions.size() == 0);
        states.pop_back();
    }

    inline void errorizeState(const yglx::State* target) {
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

    inline void redirectState(yglx::State* target, const yglx::State* source) {
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

    inline auto _addTransition(std::unique_ptr<yglx::Transition>& tx) -> yglx::Transition* {
        yglx::Transition* t = tx.get();
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

    inline auto _addTransitionToState(yglx::State* s, std::unique_ptr<yglx::Transition>& tx) -> yglx::Transition* {
        auto* t = _addTransition(tx);
        s->addTransition(t);
        return t;
    }

    inline auto cloneTransition(const yglx::Transition& tx, yglx::State* s, yglx::State* n) -> yglx::Transition* {
        auto nt = std::make_unique<yglx::Transition>(tx.t, s, n, tx.capture);
        auto* t = _addTransition(nt);
        return t;
    }

    inline auto addClassTransition(const yglx::Class& a, yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::ClassTransition(a), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto addEnterClosureTransition(const yglx::Closure& a, yglx::State* s, yglx::State* n, const bool& c, const size_t& ic) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::ClosureTransition(*this, a, yglx::ClosureTransition::Type::Enter, ic), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto addPreLoopTransition(const yglx::Closure& a, yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::ClosureTransition(*this, a, yglx::ClosureTransition::Type::PreLoop), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto addInLoopTransition(const yglx::Closure& a, yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::ClosureTransition(*this, a, yglx::ClosureTransition::Type::InLoop), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto addPostLoopTransition(const yglx::Closure& a, yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::ClosureTransition(*this, a, yglx::ClosureTransition::Type::PostLoop), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto addLeaveClosureTransition(const yglx::Closure& a, yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::ClosureTransition(*this, a, yglx::ClosureTransition::Type::Leave), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto addSlideTransition(yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::SlideTransition(), s, n, c);
        auto* tx = _addTransitionToState(s, t);
        return tx;
    }

    inline auto addTransition(const yglx::Primitive& a, yglx::State* s, yglx::State* n, const bool& c) -> yglx::Transition* {
        auto t = std::make_unique<yglx::Transition>(yglx::PrimitiveTransition(a), s, n, c);
        return _addTransitionToState(s, t);
    }

    inline auto hasRegexSet(const std::string& name) -> yglx::RegexSet* {
        for(auto& rs : regexSets) {
            if(rs->name == name) {
                return rs.get();
            }
        }
        return nullptr;
    }

    inline auto getNextPrecedence() -> size_t {
        return ++nextPrecedence;
    }

    inline auto addRegexSet(const std::string& name, const yglx::RegexSet::Assoc& assoc, const size_t& precedence) -> yglx::RegexSet* {
        regexSets.push_back(std::make_unique<yglx::RegexSet>());
        auto& regexSet = regexSets.back();
        regexSet->id = regexSets.size();
        regexSet->name = name;
        regexSet->precedence = precedence;
        regexSet->assoc = assoc;
        return regexSet.get();
    }

    inline void addRegex(std::unique_ptr<yglx::Regex>& pregex, const yglx::RegexSet::Assoc& assoc) {
        regexes.push_back(std::move(pregex));
        auto& regex = *(regexes.back());
        regex.id = regexes.size();

        auto* regexSet = hasRegexSet(regex.regexName);
        if(regexSet == nullptr) {
            auto precedence = getNextPrecedence();
            regexSet = addRegexSet(regex.regexName, assoc, precedence);
        }

        regex.regexSet = regexSet;
        regexSet->regexes.push_back(&regex);
    }

    inline void addSystemRegex(const std::string& name, const yglx::RegexSet::Assoc& assoc) {
        auto regex = std::make_unique<yglx::Regex>();
        regex->regexName = name;
        regex->usageCount = 1;
        addRegex(regex, assoc);
    }

    inline auto hasRegexSet(const std::string& name) const -> yglx::RegexSet* {
        for(const auto& t : regexSets) {
            if(t->name == name) {
                return t.get();
            }
        }
        return nullptr;
    }

    inline auto getRegexSet(const ygp::Node& node) const -> yglx::RegexSet& {
        auto* p = hasRegexSet(node.name);
        if(p == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, node.pos, "INVALID_TOKEN:{}", node.name);
        }
        return *p;
    }

    inline auto getRegexSetByName(const FilePos& npos, const std::string& name) const -> yglx::RegexSet& {
        auto* p = hasRegexSet(name);
        if(p == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, npos, "INVALID_TOKEN:{}", name);
        }
        return *p;
    }

    inline void addLexerMode(const FilePos& npos, const std::string& name) {
        if (lexerModes.contains(name) == true) {
            throw GeneratorError(__LINE__, __FILE__, npos, "DUPLICATE_MODE:{}", name);
        }
        lexerModes[name] = std::make_unique<yglx::LexerMode>();
        auto& mode = *(lexerModes[name]);
        mode.root = createNewState(npos);
        mode.root->checkEOF = true;
        mode.root->isRoot = true;
    }

    inline auto getLexerModeByName(const FilePos& npos, const std::string& name) const -> yglx::LexerMode& {
        if (lexerModes.contains(name) == false) {
            throw GeneratorError(__LINE__, __FILE__, npos, "UNKNOWN_MODE:{}", name);
        }

        auto& mode = lexerModes.at(name);
        assert(mode);
        return *mode;
    }

    inline void _getLexerModes(const std::string& lmName, std::vector<yglx::LexerMode*>& rv) const {
        for(auto& lm : lexerModes) {
            if(lm.second->includes.contains(lmName) == true) {
                rv.push_back(lm.second.get());
                _getLexerModes(lm.first, rv);
            }
        }
    }

    inline auto getLexerModes(const yglx::Regex& regex) const -> std::vector<yglx::LexerMode*> {
        std::vector<yglx::LexerMode*> rv;
        auto& m = getLexerModeByName(regex.pos, regex.mode);
        rv.push_back(&m);
        _getLexerModes(regex.mode, rv);
        return rv;
    }

    inline auto getRegexNextMode(const yglx::Regex& regex) const -> yglx::LexerMode& {
        if (lexerModes.contains(regex.nextMode) == false) {
            throw GeneratorError(__LINE__, __FILE__, regex.pos, "UNKNOWN_MODE:{}", regex.nextMode);
        }

        const auto& mode = lexerModes.at(regex.nextMode);
        assert(mode);
        return *mode;
    }

    inline auto
    addRule(
        const FilePos& npos,
        const std::string& name,
        std::unique_ptr<ygp::Rule>& prule,
        const bool& anchorSet,
        const bool& isEmpty
    ) -> ygp::Rule& {
        ygp::RuleSet* ruleSet = nullptr;
        for(auto& rs : ruleSets) {
            if(rs->name == name) {
                ruleSet = rs.get();
                break;
            }
        }
        if(ruleSet == nullptr) {
            ruleSets.push_back(std::make_unique<ygp::RuleSet>());
            ruleSet = ruleSets.back().get();
            ruleSet->id = ruleSets.size();
            ruleSet->name = name;
        }

        if(isEmpty == true) {
            if(ruleSet->hasEpsilon == true) {
                throw GeneratorError(__LINE__, __FILE__, npos, "MULTIPLE_EMPTY_RULES:{}", ruleSet->name);
            }
            ruleSet->hasEpsilon = true;
        }

        rules.push_back(std::move(prule));
        auto& rule = *(rules.back());
        rule.ruleSet = ruleSet;
        ruleSet->rules.push_back(&rule);
        if(isEmpty == true) {
            rule.id = 0;
        }else{
            rule.id = ruleSet->rules.size();
        }

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

    inline auto getRuleSetByName(const FilePos& p, const std::string& name) const -> ygp::RuleSet& {
        for(const auto& r : ruleSets) {
            if(r->name == name) {
                return *r;
            }
        }
        throw GeneratorError(__LINE__, __FILE__, p, "UNKNOWN_RULESET:{}", name);
    }

    inline auto createConfig(const ygp::Rule& r, const size_t& p) -> ygp::Config& {
        assert(p <= r.nodes.size());
        for(auto& c : configs) {
            if((&(c->rule) == &r) && (c->cpos == p)) {
                return *c;
            }
        }
        configs.push_back(std::make_unique<ygp::Config>(r, p));
        auto& cfg = *(configs.back());
        return cfg;
    }

    inline auto _createItemSet() -> ygp::ItemSet& {
        itemSets.push_back(std::make_unique<ygp::ItemSet>());
        auto& is = *(itemSets.back());
        is.id = itemSets.size();
        return is;
    }

    inline auto createItemSet(std::vector<const ygp::Config*>& cfgs) -> ygp::ItemSet& {
        auto& is = _createItemSet();
        is.configs = std::move(cfgs);
        return is;
    }

    inline auto hasItemSet(const std::vector<const ygp::Config*>& cfgs) const -> ygp::ItemSet* {
        for(const auto& is : itemSets) {
            if(is->configs.size() != cfgs.size()) {
                continue;
            }

            auto iit = is->configs.begin();
            auto iite = is->configs.end();
            auto cit = cfgs.begin();
            auto cite = cfgs.end();
            bool mismatch = false;
            while((iit != iite) && (cit != cite)) {
                const auto& icfg = *(*iit);
                const auto& ccfg = *(*cit);

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

    inline auto getItemSet(const FilePos& npos, const std::vector<const ygp::Config*>& cfgs) const -> ygp::ItemSet& {
        auto* is = hasItemSet(cfgs);
        if(is == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, npos, "INVALID_ITEMSET");
        }
        return *is;
    }

    inline auto pos() const -> FilePos {
        FilePos npos;
        if(rules.size() > 0) {
            npos = rules.at(0)->pos;
        }
        return npos;
    }

    inline Grammar() = default;

    inline ~Grammar() {
        for(auto& gs : states) {
            gs->transitions.clear();
        }
    }
};

} // namespace yg
