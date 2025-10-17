#include "pch.hpp"
#include "lexer_builder.hpp"
#include "logger.hpp"

namespace {
struct LexerStateMachineBuilder {
    yg::Grammar& grammar;
    std::string indent = "-";
    const yg::Grammar::Atom* _atom = nullptr;

    yg::Grammar::State* currentState = nullptr;
    yg::Grammar::State* closureState = nullptr;
    yg::Grammar::Transition* startClosureTransition = nullptr;
    bool inCapture = true;

    inline LexerStateMachineBuilder(yg::Grammar& g, yg::Grammar::State* s)
        : grammar(g), currentState(s) {}

    inline yg::Grammar::State* getCurrentState() const {
        assert(currentState != nullptr);
        return currentState;
    }

    inline void setCurrentState(yg::Grammar::State* state) {
        currentState = state;
    }

    inline void setStartClosureTransition(yg::Grammar::Transition* t) {
        assert(t != nullptr);
        if(startClosureTransition == nullptr) {
            startClosureTransition = t;
        }
    }

    inline const yg::Grammar::Atom& atom() const {
        assert(_atom);
        return *_atom;
    }

    struct Tracer {
        const LexerStateMachineBuilder& lxb;
        const std::string fn;

        template <typename ...ArgsT>
        inline void iprint(const std::format_string<ArgsT...>& msg, ArgsT... args) const {
            auto rv = fmt(msg, args...);
            print(Logger::log(), "{}{}({}): {}", lxb.indent, fn, lxb.atom().pos().str(), rv);
        }

        inline void printCS(const std::string& msg) const {
            iprint("{}:currentStates={}", msg, zid(lxb.getCurrentState()));
        }

        inline Tracer(const LexerStateMachineBuilder& l, const std::string& f) : lxb(l), fn(f) {
            printCS("ENTER");
        }

        inline ~Tracer() {
            printCS("LEAVE");
        }
    };

    template<typename AtomT>
    inline yg::Grammar::State* updateStateX(const yg::Grammar::Primitive& pa, const AtomT& a, yg::Grammar::State* state, yg::Grammar::State* nextState) {
        yg::Grammar::Transition* t = state->getTransition(a);
        if((t != nullptr) && (t->next != nullptr)) {
            setStartClosureTransition(t);
            return t->next;
        }

        auto s = nextState;
        if(s == nullptr) {
            s = grammar.createNewState(pa.pos);
        }

        assert((t == nullptr) || (t->next == nullptr));
        if(t == nullptr) {
            t = grammar.addTransition(pa, state, s, inCapture);
            setStartClosureTransition(t);
            return s;
        }

        assert(t != nullptr);
        assert(t->next == nullptr);
        t->next = s;
        setStartClosureTransition(t);
        return s;
    }

    template<typename AtomT>
    inline void updateState(const yg::Grammar::Primitive& pa, const AtomT& a) {
        auto state = updateStateX(pa, a, getCurrentState(), nullptr);
        setCurrentState(state);

        if(closureState != nullptr) {
            updateStateX(pa, a, closureState, getCurrentState());
            closureState = nullptr;
        }
    }

    inline void operator()(const yg::Grammar::Primitive& a) {
        Tracer _t(*this, std::format("Primitive:{}", a.str()));

        std::visit([this, &a](const auto& ax){
            updateState(a, ax);
        }, a.atom);
    }

    inline void operator()(const yg::Grammar::Class& a) {
        Tracer _t(*this, std::format("Class:{}", a.str()));

        yg::Grammar::Transition* t = getCurrentState()->getTransition(a);
        if(t != nullptr) {
            setStartClosureTransition(t);
            setCurrentState(t->next);
            return;
        }

        auto xstate = grammar.createNewState(a.pos);
        t = grammar.addClassTransition(a, getCurrentState(), xstate, inCapture);
        setStartClosureTransition(t);
        setCurrentState(xstate);
    }

    inline void operator()(const yg::Grammar::Sequence& a) {
        Tracer _t(*this, std::format("Sequence:{}", a.str()));

        process(*(a.lhs));
        process(*(a.rhs));
    }

    inline void operator()(const yg::Grammar::Disjunct& a) {
        Tracer _t(*this, std::format("Disjunct:{}", a.str()));

        auto s0 = getCurrentState();
        process(*(a.lhs));
        auto s1 = getCurrentState();

        setCurrentState(s0);
        process(*(a.rhs));
        grammar.redirectState(s1, getCurrentState());

        setCurrentState(s1);
    }

    inline void operator()(const yg::Grammar::Group& a) {
        Tracer _t(*this, std::format("Group:{}", a.str()));

        if(a.capture == false) {
            inCapture = false;
        }
        process(*(a.atom));
        inCapture = true;
    }

    inline void operator()(const yg::Grammar::Closure& a) {
        Tracer _t(*this, std::format("Closure:{}", a.str()));

        startClosureTransition = nullptr;
        auto s0 = getCurrentState();
        process(*(a.atom));
        auto s1 = getCurrentState();
        assert(startClosureTransition != nullptr);

        if(s1->closure != nullptr) {
            assert(s1->leaveClosureTransition != nullptr);
            assert(s1->checkClosureTransition != nullptr);
            assert(startClosureTransition->compare(*(s1->checkClosureTransition)) == 0);
            auto sx = s1->leaveClosureTransition->next;
            setCurrentState(sx);
        }else{
            auto s2 = grammar.createNewState(a.pos);
            auto enterClosureTransition = grammar.addEnterClosureTransition(a, getCurrentState(), s2, inCapture, 1);
            setCurrentState(s2);

            auto s3 = grammar.createNewState(a.pos);
            if(a.min > 1) {
                grammar.addPreLoopTransition(a, s2, s3, inCapture);
                setCurrentState(s3);
                process(*(a.atom));
                grammar.redirectState(s2, getCurrentState());
                s3 = grammar.createNewState(a.pos);
            }

            grammar.addInLoopTransition(a, s2, s3, inCapture);
            setCurrentState(s3);
            process(*(a.atom));
            grammar.redirectState(s2, getCurrentState());
            assert(s3->transitions.size() == 1);
            auto checkClosureTransition = s3->transitions.at(0);

            auto sx = grammar.createNewState(a.pos);
            grammar.addPostLoopTransition(a, s2, sx, inCapture);
            auto leaveClosureTransition = grammar.addLeaveClosureTransition(a, s3, sx, inCapture);
            if(a.min == 0) {
                closureState = s0;
            }

            s1->closure = &a;
            s1->closureState = s2;
            s1->enterClosureTransition = enterClosureTransition;
            s1->leaveClosureTransition = leaveClosureTransition;
            s1->checkClosureTransition = checkClosureTransition;
            if(checkClosureTransition->get<yg::Grammar::WildCard>() != nullptr) {
                s3->startClosureTransition = startClosureTransition;
            }
            setCurrentState(sx);
        }
    }

    void process(const yg::Grammar::Atom& a) {
        auto xindent = indent;
        indent = indent + "-";
        auto xatom = _atom;
        _atom = &a;
        std::visit(*this, a.atom);
        _atom = xatom;
        indent = xindent;
    }
};

struct Optimizer {
    yg::Grammar& grammar;
    std::unordered_set<const yg::Grammar::State*> vset;

    inline Optimizer(yg::Grammar& g) : grammar(g) {}

    inline bool isVisited(const yg::Grammar::State* state) {
        if(vset.contains(state) == true) {
            return true;
        }
        vset.insert(state);
        return false;
    }

    inline bool contains(std::vector<yg::Grammar::Transition*>& dst, const yg::Grammar::Transition* tx) {
        for (auto& ptx : dst) {
            if(ptx->compare(*tx) == 0) {
                return true;
            }
        }
        return false;
    }

    inline bool contains(yg::Grammar::State* state, yg::Grammar::Transition* stx) {
        if(contains(state->transitions, stx) || contains(state->superTransitions, stx) || contains(state->shadowTransitions, stx)) {
            return true;
        }
        return false;
    }

    //TODO: find smallest superset
    // right now this function returns the first valid superset
    inline const yg::Grammar::Transition*
    _findSmallestSuperset(const yg::Grammar::Transition* subTx, const yg::Grammar::State* superState, const bool& isClosure) const {
        for(auto& superTx : superState->transitions) {
            if(isClosure == true) {
                if(superTx->next->closure == nullptr) {
                    continue;
                }
            }else{
                if(superTx->next->closure != nullptr) {
                    continue;
                }
            }

            if(superTx->next == subTx->from) {
                continue;
            }

            if(subTx->isSubsetOf(*superTx) == true) {
                return superTx;
            }
        }
        return nullptr;
    }

    inline const yg::Grammar::Transition*
    findSmallestSuperset(const yg::Grammar::Transition* subTx, const yg::Grammar::State* superState) const {
        return _findSmallestSuperset(subTx, superState, false);
    }

    inline const yg::Grammar::Transition*
    findSmallestClosureSuperset(const yg::Grammar::Transition* subTx, const yg::Grammar::State* superState) const {
        return _findSmallestSuperset(subTx, superState, true);
    }

    inline yg::Grammar::Transition*
    cloneTransition(const yg::Grammar::Transition& srcTx, yg::Grammar::State* from, yg::Grammar::State* next) {
        auto ntx = grammar.cloneTransition(srcTx, from, next);
        from->superTransitions.push_back(ntx);
        return ntx;
    }

    inline yg::Grammar::Transition*
    createEnterTransition(
        const yg::Grammar::Transition& checkClosureTx,
        const yg::Grammar::Transition& enterClosureTx,
        yg::Grammar::State* from,
        yg::Grammar::State* next,
        const size_t& initialCount
    ) {
        auto istate = grammar.createNewState(from->pos);
        cloneTransition(checkClosureTx, from, istate);
        auto ntx = cloneTransition(enterClosureTx, istate, next);
        if(auto nctx = ntx->isClosure(yg::Grammar::ClosureTransition::Type::Enter)) {
            nctx->initialCount = initialCount;
        }else{
            assert(false);
        }
        return ntx;
    }

    inline void setSuperStateClosure(
        const yg::Grammar::Transition* superTx,
        yg::Grammar::State* state,
        const size_t& initialCount,
        const std::string& indent
    ) {
        assert(superTx->next->closure != nullptr);
        assert(superTx->next->closureState != nullptr);

        auto& nextClosure = *(superTx->next->closure);
        auto& nextClosureState = *(superTx->next->closureState);
        auto& enterClosureTx = superTx->next->enterClosureTx();
        auto& leaveClosureTx = superTx->next->leaveClosureTx();
        auto& checkClosureTx = superTx->next->checkClosureTx();

        for(auto& subTx : state->transitions) {
            if(subTx == superTx) {
                continue;
            }

            if(subTx->isSubsetOf(checkClosureTx) == false) {
                continue;
            }

            createEnterTransition(checkClosureTx, enterClosureTx, subTx->next, &nextClosureState, initialCount);
            cloneTransition(leaveClosureTx, subTx->next, leaveClosureTx.next);

            if(isVisited(subTx->next) == true) {
                continue;
            }
            setSuperStateClosure(superTx, subTx->next, initialCount + 1, indent + "  ");
        }

        if((nextClosure.min >= initialCount) && (initialCount <= nextClosure.max)){
            if(state->getClosureTransition(yg::Grammar::ClosureTransition::Type::Leave) == nullptr) {
                cloneTransition(leaveClosureTx, state, leaveClosureTx.next);
            }
        }
    }

    inline void setSuperState(
        yg::Grammar::State* subState,
        yg::Grammar::State* superState,
        const size_t& initialCount,
        const std::string& indent
    ) {
        for(auto& subTx : subState->transitions) {
            auto superTx = findSmallestSuperset(subTx, superState);
            if(superTx == nullptr) {
                continue;
            }

            for(auto& stx : superTx->next->transitions) {
                if(contains(subTx->next, stx) == true) {
                    continue;
                }
                if(stx->next->closure != nullptr) {
                    auto& nextClosure = *(stx->next->closure);
                    if(nextClosure.min == 0) {
                        auto& enterClosureTx = stx->next->enterClosureTx();
                        auto& checkClosureTx = stx->next->checkClosureTx();
                        auto& leaveClosureTx = stx->next->leaveClosureTx();
                        createEnterTransition(checkClosureTx, enterClosureTx, subTx->next, stx->next->closureState, initialCount);
                        cloneTransition(leaveClosureTx, subTx->next, leaveClosureTx.next);
                    }
                    setSuperStateClosure(stx, subTx->next, initialCount, indent);
                    continue;
                }
                cloneTransition(*stx, subTx->next, stx->next);
            }

            if(isVisited(subTx->next) == true) {
                continue;
            }
            setSuperState(subTx->next, superTx->next, initialCount + 1, indent + "  ");
        }
    }

    inline void setSuperStates(yg::Grammar::State* state, const std::string& indent) {
        setSuperState(state, state, 1, indent);

        for(auto& subTx : state->transitions) {
            auto superTx = findSmallestClosureSuperset(subTx, state);
            if(superTx == nullptr) {
                continue;
            }
            setSuperStateClosure(superTx, state, 1, indent);
        }
    }

    inline void setShadowState(yg::Grammar::State* state, const std::string& indent) {
        if(isVisited(state) == true) {
            return;
        }

        if(state->startClosureTransition != nullptr) {
            for(auto& tix : state->startClosureTransition->from->transitions) {
                if(tix == state->startClosureTransition) {
                    continue;
                }
                cloneTransition(*tix, state, tix->next);
            }
        }
        for(auto& tix : state->transitions) {
            setShadowState(tix->next, indent + "  ");
        }
    }
};
}

void buildLexer(yg::Grammar& g) {
    for(auto& regex : g.regexes) {
        if(!regex->atom) {
            continue;
        }

        auto& mode = g.getLexerMode(*regex);

        LexerStateMachineBuilder lsmb(g, mode.root);
        lsmb.process(*(regex->atom));
        if(lsmb.getCurrentState()->id == 1) {
            throw GeneratorError(__LINE__, __FILE__, regex->pos, "EMPTY_TOKEN:{}", regex->regexName);
        }

        lsmb.getCurrentState()->matchedRegex = regex.get();
        if(lsmb.closureState != nullptr) {
            lsmb.closureState->matchedRegex = regex.get();
        }
    }

    for (auto& lxm : g.lexerModes) {
        auto& mode = lxm.second;

        Optimizer optimizer(g);
        optimizer.setSuperStates(mode->root, "");
        optimizer.vset.clear();
        optimizer.setShadowState(mode->root, "");
    }
}
