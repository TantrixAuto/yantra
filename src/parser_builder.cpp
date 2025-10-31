#include "pch.hpp"
#include "parser_builder.hpp"
#include "logger.hpp"

namespace {
/// @brief represents a set of configs
/// This is an intermediate data structure used during the
/// construction of ItemSets in the LALR state machine
/// TODO: This class is almost identical to ItemSet, chek if opssible to merge.
struct PreItemSet : public NonCopyable {
    /// @brief represents all SHIFT actions from this config set
    struct Shift {
        /// @brief the next config to shift to
        std::vector<const ygp::Config*> next;

        /// @brief epsilon transitions from this PreItemSet
        std::vector<const ygp::RuleSet*> epsilons;
    };

    /// @brief represents all REDUCE actions from this config set
    struct Reduce {
        /// @brief the next config to shift when this REDUCE action is executed
        /// a PreItemSet can have multiple reduces for the same REGEX.
        /// this is resolved when converting to ItemSet.
        /// hence, a vector here.
        std::vector<const ygp::Config*> next;

        /// @brief length of the Rule to REDUCE
        size_t len = 0;
    };

    /// @brief the underlying ItemSet for this config set
    ygp::ItemSet& is;

    /// @brief list of SHIFT actions from this config set
    std::unordered_map<const yglx::RegexSet*, Shift> shifts;

    /// @brief list of REDUCE actions from this config set
    std::unordered_map<const yglx::RegexSet*, Reduce> reduces;

    /// @brief list of GOTO actions from this config set
    std::unordered_map<const ygp::RuleSet*, std::vector<const ygp::Config*>> gotos;

    /// @brief ctor
    inline PreItemSet(ygp::ItemSet& i) : is(i) {}

    /// @brief check if there is a SHIFT action for the given token @arg rx
    inline const Shift* hasShift(const yglx::RegexSet& rx) const {
        if(auto it = shifts.find(&rx); it != shifts.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    /// @brief add a SHIFT action for the given token @arg rx, from current Config to @arg next Config
    inline void addShift(const yglx::RegexSet& rx, const ygp::Config& next) {
        if(hasShift(rx) == nullptr) {
            shifts[&rx] = Shift();
        }
        shifts[&rx].next.push_back(&next);
    }

    /// @brief add a SHIFT action for the given token @arg rx, from current Config to @arg next Config
    /// along with @arg epsilon tranitions
    inline void addShift(const yglx::RegexSet& rx, const ygp::Config& next, const std::vector<const ygp::RuleSet*>& epsilons) {
        if(hasShift(rx) == nullptr) {
            shifts[&rx] = Shift();
            shifts[&rx].epsilons = epsilons;
        }
        shifts[&rx].next.push_back(&next);
    }

    /// @brief move SHIFTs from another Config to here
    /// This is used during LALR construction, to collapse similar Configs into one
    inline const std::vector<const ygp::Config*>&
    moveShifts(
        const yglx::RegexSet& rx,
        std::vector<const ygp::Config*>& nexts,
        const std::vector<const ygp::RuleSet*>& epsilons
    ) {
        unused(epsilons);
        if(hasShift(rx) == nullptr) {
            shifts[&rx] = Shift();
            shifts[&rx].epsilons = epsilons;
        }
        auto& shift = shifts[&rx];
        shift.next = std::move(nexts);
        return shift.next;
    }

    /// @brief check if there is a REDUCE action for the given token @arg rx
    inline const Reduce* hasReduce(const yglx::RegexSet& rx) const {
        if(auto it = reduces.find(&rx); it != reduces.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    /// @brief add a REDUCE action for the given token @arg rx
    inline void addReduce(const yglx::RegexSet& rx, const ygp::Config& next, const size_t& len) {
        if(hasReduce(rx) == nullptr) {
            Reduce r;
            r.len = len;
            reduces[&rx] = r;
        }
        reduces[&rx].next.push_back(&next);
    }

    /// @brief move REDUCEs from another Config to here
    inline void
    moveReduces(
        const yglx::RegexSet& rx,
        Reduce& r
    ) {
        reduces[&rx] = std::move(r);
    }

    /// @brief check if there is a GOTO action for the given RuleSet @arg rs
    inline bool hasGoto(const ygp::RuleSet& rs, const ygp::Config& cfg) const {
        if(auto it = gotos.find(&rs); it != gotos.end()) {
            for(auto& c : it->second) {
                if(c == &cfg) {
                    return true;
                }
            }
        }
        return false;
    }

    /// @brief move GOTOs from another Config to here
    inline const std::vector<const ygp::Config*>&
    moveGotos(
        const ygp::RuleSet& rs,
        std::vector<const ygp::Config*>& nexts
    ) {
        auto& g = gotos[&rs];
        g = std::move(nexts);
        return g;
    }
};

struct ParserStateMachineBuilder {
    yg::Grammar& grammar;
    inline ParserStateMachineBuilder(yg::Grammar& g) : grammar{g} {}

    inline void printRules(std::ostream& os, const std::string& msg) const {
        std::println(os, "--------------------------------");
        std::println(os, "GRAMMAR({}):{}", grammar.rules.size(), msg);
        for(auto& rs : grammar.regexSets) {
            std::println(os, "REGEXSET({}): name={}, prec={}, assoc={}", rs->id, rs->name, rs->precedence, yglx::RegexSet::assocName(rs->assoc));
            if(rs->regexes.size() > 1) {
                for(auto& rx : rs->regexes) {
                    std::println(os, "  REGEX({}): name={}, prec={}", rx->id, rx->regexName, rx->regexSet->precedence);
                }
            }
        }

        for(auto& rule : grammar.rules) {
            std::println(os, "RULE<{}>: {}", rule->ruleSet->name, rule->str());
        }
    }

    [[maybe_unused]]
    inline void
    printPreItemSet(
        const PreItemSet& pis,
        const size_t& id,
        const std::string_view& indent
    ) const {
        std::println("{}<<configset:is={}", indent, id);
        for(auto& c : pis.shifts) {
            auto& rx = c.first;
            auto& cfgs = c.second;
            std::println("{}-shift: rx={}, next_sz={}", indent, rx->name, cfgs.next.size());
            for(auto& cfg : cfgs.next) {
                std::println("{}--cfg={}", indent, cfg->str(false));
            }
        }

        for(auto& c : pis.reduces) {
            auto& rx = c.first;
            auto& cfgs = c.second;
            std::println("{}-reduce: rx={}, next_sz={}", indent, rx->name, cfgs.next.size());
            for(auto& cfg : cfgs.next) {
                std::println("{}--cfg={}", indent, cfg->str(false));
            }
        }

        for(auto& c : pis.gotos) {
            auto& rs = c.first;
            auto& cfgs = c.second;
            std::println("{}-goto: rs={}, next_sz={}", indent, rs->name, cfgs.size());
            for(auto& cfg : cfgs) {
                std::println("{}--cfg={}", indent, cfg->str(false));
            }
        }
        std::println("{}>>configset:is={}", indent, id);
    }

    inline void
    _printConfigList(
        std::ostream& os,
        const std::string& msg,
        const size_t& id,
        const std::vector<const ygp::Config*>& configs,
        const std::string_view& indent
    ) const {
        std::println(os, "{}{},id=={}, len={}", indent, msg, id, configs.size());
        for(auto& pc : configs) {
            auto& c = *pc;
            std::println(os, "{}-config:{}", indent, c.str(false));
        }
    }

    [[maybe_unused]]
    inline void
    printConfigList(
        const std::string& msg,
        const size_t& id,
        const std::vector<const ygp::Config*>& configs,
        const std::string_view& indent
    ) const {
        _printConfigList(std::cout, msg, id, configs, indent);
    }

    [[maybe_unused]]
    inline void
    logConfigList(
        const std::string& msg,
        const size_t& id,
        const std::vector<const ygp::Config*>& configs,
        const std::string_view& indent
    ) const {
        _printConfigList(Logger::olog(), msg, id, configs, indent);
    }

    inline bool
    hasRuleInConfigList(
        const std::vector<const ygp::Config*>& configs,
        const ygp::Rule& r
    ) {
        for(auto& c : configs) {
            if(&(c->rule) == &r) {
                return true;
            }
        }
        return false;
    }

    /// @brief expands all configs in the list
    /// for each config, if the next node is a rule-ref,
    /// add all matching rules to the config-list
    inline std::vector<const ygp::Config*>
    expandConfigs(const std::vector<const ygp::Config*>& initConfig) {
        // create all sub-configs
        std::vector<const ygp::Config*> configs;

        auto nexts = initConfig;

        while(nexts.size() > 0) {
            std::vector<const ygp::Config*> firsts;
            for(auto& c : nexts) {
                if(hasRuleInConfigList(configs, c->rule)) {
                    continue;
                }
                configs.push_back(c);

                if(c->cpos >= c->rule.nodes.size()) {
                    assert(c->cpos == c->rule.nodes.size());
                    continue;
                }

                auto& nextNode = c->getNextNode();
                if(nextNode.isRule()) {
                    for(auto& r : grammar.rules) {
                        if((r->ruleSetName() == nextNode.name)) {
                            auto& ccfg = grammar.createConfig(*r, 0);
                            firsts.push_back(&ccfg);
                        }
                    }
                }
            }

            nexts = firsts;
        }

        return configs;
    }

    inline char resolveConflict(const ygp::Config& cfg, const yglx::RegexSet& rx, const std::string& indent) const {
        auto& r = cfg.rule;

        // if we are resolving a conflict against the END token,
        // always resolve in favor of a REDUCE.
        if(rx.name == grammar.end) {
            return 'R';
        }

        if(r.precedence->precedence > rx.precedence) {
            return 'R';
        }

        if(r.precedence->precedence < rx.precedence) {
            return 'S';
        }

        assert(r.precedence->precedence == rx.precedence);
        if(rx.assoc == yglx::RegexSet::Assoc::Left) {
            return 'R';
        }

        if(rx.assoc == yglx::RegexSet::Assoc::Right) {
            return 'S';
        }

        assert(rx.assoc == yglx::RegexSet::Assoc::None);
        if(grammar.autoResolve == false) {
            throw GeneratorError(__LINE__, __FILE__, r.pos, "REDUCE_SHIFT_RESOLVE_ERROR:{}->{}", r.ruleName, rx.name);
        }

        if(grammar.warnResolve == true) {
            log("{}REDUCE-SHIFT-CONFLICT: Resolved in favor of SHIFT: {}->{}", indent, r.ruleName, rx.name);
        }
        return 'S';
    }

    inline void addReduce(
        PreItemSet& pis,
        const ygp::Config& config,
        const size_t& len,
        const std::string& /*indent*/
    ) {
        auto& rs = grammar.getRuleSetByName(config.rule.pos, config.rule.ruleSetName());
        for(auto& prx : rs.follows) {
            auto& rx = *prx;
            pis.addReduce(rx, config, len);
        }
    }

    inline void addShift(
        ygp::Node& nextNode,
        PreItemSet& pis,
        const ygp::Config& config,
        const size_t& cpos,
        const std::vector<const ygp::RuleSet*>& epsilons,
        const std::string& indent
    ) {
        log("{}addShift:cfg={}, next=regex", indent, config.str());
        if(nextNode.name == grammar.empty) {
            log("{}addShift:skip_empty", indent);
            return;
        }

        auto& rx = grammar.getRegexSet(nextNode);
        if(pis.hasReduce(rx) != nullptr) {
            char p = resolveConflict(config, rx, indent);
            if(p == 'R') {
                return;
            }
            pis.reduces.erase(&rx);
        }

        bool hasExisting = false;
        if(pis.hasShift(rx)) {
            for(auto& xcfg : pis.shifts[&rx].next) {
                 if(&(xcfg->rule) == &(config.rule)) {
                    hasExisting = true;
                    break;
                }
            }
        }
        if(hasExisting == false) {
            auto& ncfg = grammar.createConfig(config.rule, cpos + 1);
            pis.addShift(rx, ncfg, epsilons);
        }
    }

    inline void addGoto(
        ygp::Node& nextNode,
        PreItemSet& pis,
        const ygp::Config& config,
        const size_t& cpos,
        const std::string& indent
    ) {
        log("{}addGoto:cfg={}, cpos={}, nextNode={}", indent, config.str(), cpos, nextNode.str());

        // add GOTO for rule node
        auto& ncfg = grammar.createConfig(config.rule, cpos + 1);
        auto& rs = grammar.getRuleSetByName(nextNode.pos, nextNode.name);
        assert(pis.hasGoto(rs, ncfg) == false);
        pis.gotos[&rs].push_back(&ncfg);
    }

    std::unordered_map<const ygp::ItemSet*, std::unique_ptr<PreItemSet>> pisList;

    inline PreItemSet&
    createPreItemSet(ygp::ItemSet& is) {
        if(pisList.find(&is) != pisList.end()) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "DUPLICATE_CONFIGSET:{}", is.id);
        }

        pisList[&is] = std::make_unique<PreItemSet>(is);
        return *(pisList[&is]);
    }

    inline PreItemSet&
    getPreItemSet(ygp::ItemSet& is) {
        if(auto it = pisList.find(&is); it != pisList.end()) {
            return *(it->second);
        }
        throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_CONFIGSET");
    }

    inline void
    getNextPreItemSet(
        const std::vector<const ygp::Config*>& cfgs,
        PreItemSet& pis,
        const std::string& indent
    ) {
        for(auto& c : cfgs) {
            auto& config = *c;
            const ygp::Node* epsilonNode = nullptr;
            std::vector<const ygp::RuleSet*> epsilons;
            auto cpos = config.cpos;
            do {
                log("{}getNextConfigSet:cfg={}, cpos={}", indent, config.str(), cpos);
                assert(cpos <= config.rule.nodes.size());
                bool first = (epsilonNode == nullptr);
                epsilonNode = nullptr;
                auto nextNode = config.rule.getNodeAt(cpos);
                if(nextNode == nullptr) {
                    auto len = config.rule.nodes.size() - (cpos - config.cpos);
                    log("{}getNextConfigSet:is-end:len={}", indent, len);
                    addReduce(pis, config, len, indent);
                }else if(nextNode->isRegex()) {
                    if(nextNode->name == grammar.empty) {
                        log("{}getNextConfigSet:is-regex-empty:{}", indent, nextNode->name);
                    //     epsilonNode = nextNode;
                    }else if(nextNode->name == grammar.end) {
                        auto len = config.rule.nodes.size() - (cpos - config.cpos);
                        log("{}getNextConfigSet:is-regex-end:{}, len={}, cfg={}", indent, nextNode->name, len, config.str(false));
                        assert(len > 0);
                        addReduce(pis, config, len, indent);
                    }else{
                        log("{}getNextConfigSet:is-regex:{}", indent, nextNode->name);
                        addShift(*nextNode, pis, config, cpos, epsilons, indent);
                    }
                }else if(nextNode->isRule()) {
                    log("{}getNextConfigSet:is-rule:{}", indent, nextNode->name);
                    if(first == true) {
                        addGoto(*nextNode, pis, config, cpos, indent);
                        auto& rs = grammar.getRuleSetByName(nextNode->pos, nextNode->name);
                        if(rs.firstIncludes(grammar.empty) == true) {
                            epsilonNode = nextNode;
                            epsilons.push_back(&rs);
                        }
                    }
                }else{
                    assert(false);
                }
                ++cpos;
            }while(epsilonNode != nullptr);
        }
    }

    inline std::tuple<ygp::ItemSet&, bool>
    createNewItemSet(
        std::vector<const ygp::Config*>& configs,
        const std::string& indent
    ) {
        unused(indent);
        if(auto is = grammar.hasItemSet(configs)) {
            log("{}Found Existing ItemSet:{}", indent, is->id);
            return {*is, true};
        }

        auto& is = grammar.createItemSet(configs);
        log("{}Created ItemSet:{}", indent, is.id);
        return {is, false};
    }

    inline ygp::ItemSet&
    createItemSet(
        const std::vector<const ygp::Config*>& initConfig,
        const std::string& indent
    ) {
        auto configs = expandConfigs(initConfig);

        auto [is, found] = createNewItemSet(configs, indent);
        if(found == true) {
            return is;
        }

        PreItemSet pis(is);
        getNextPreItemSet(is.configs, pis, indent);

        auto& npis = createPreItemSet(is);

        // set is.shifts
        for(auto& c : pis.shifts) {
            auto& rx = c.first;
            auto& cfgs = c.second;
            auto xcfgs = expandConfigs(cfgs.next);
            npis.moveShifts(*rx, xcfgs, cfgs.epsilons);
        }

        // set is.reduces
        for(auto& c : pis.reduces) {
            auto& rx = c.first;
            auto& cfgs = c.second;
            npis.moveReduces(*rx, cfgs);
        }

        // set is.gotos
        for(auto& c : pis.gotos) {
            auto& rs = c.first;
            auto& cfgs = c.second;
            auto xcfgs = expandConfigs(cfgs);
            npis.moveGotos(*rs, xcfgs);
        }

        // set is.shifts
        for(auto& c : pis.shifts) {
            auto& cfgs = c.second;
            createItemSet(cfgs.next, indent + "  ");
        }

        // set is.gotos
        for(auto& c : pis.gotos) {
            auto& cfgs = c.second;
            createItemSet(cfgs, indent + "  ");
        }

        return is;
    }

    inline void linkItemSets() {
        for(auto& pis : grammar.itemSets) {
            auto& is = *pis;
            log("linkItemSets:is={}", is.id);

            auto& npis = getPreItemSet(is);
            for(auto& c : npis.gotos) {
                auto& rs = c.first;
                auto& cfgs = c.second;
                log("  goto: rs={}, next_sz={}", rs->name, cfgs.size());
                assert(cfgs.size() > 0);
                auto& config = *(cfgs.at(0));
                auto& nextNode = config.rule.getNode(0);
                auto& cis = grammar.getItemSet(config.rule.pos, cfgs);
                is.setGoto(nextNode, rs, &cis);
            }

            for(auto& c : npis.shifts) {
                auto& rx = c.first;
                auto& cfgs = c.second;
                log("  shift: rx={}, next_sz={}", rx->name, cfgs.next.size());
                assert(cfgs.next.size() > 0);
                // assert(cfgs.next.size() == 1);
                auto& config = *(cfgs.next.at(0));
                auto& nextNode = config.rule.getNode(0);
                auto& cis = grammar.getItemSet(config.rule.pos, cfgs.next);
                assert(is.hasShift(*rx) == nullptr);
                assert(is.hasReduce(*rx) == nullptr);
                is.setShift(nextNode, *rx, cis, cfgs.epsilons);
            }

            for(auto& c : npis.reduces) {
                auto& rx = *(c.first);
                auto& cfgs = c.second;
                log("  reduce: rx={}, next_sz={}", rx.name, cfgs.next.size());
                if(cfgs.next.size() != 1) {
                    // throw GeneratorError(__LINE__, __FILE__, config.rule->pos, "REDUCE_SHIFT_CONFLICT:ON:{}{}", rx->name, ss.str());
                }
                // assert(cfgs.next.size() == 1);
                auto& config = *(cfgs.next.at(0));
                auto& lastNode = *(config.rule.nodes.back());
                assert(is.hasShift(rx) == nullptr);
                assert(is.hasReduce(rx) == nullptr);
                is.setReduce(lastNode, rx, config, cfgs.len);
            }
        }
    }

    struct Links {
        const yg::Grammar& grammar;
        std::unordered_map<std::string, std::set<std::string>> firsts;
        std::unordered_map<std::string, std::set<std::string>> follows;
        std::set<std::string> nullable;

        inline Links(const yg::Grammar& g) : grammar(g) {}

        static inline std::string str(const std::set<std::string>& set) {
            std::stringstream ss;
            std::string sep;
            ss << "[";
            sep = "";
            for(auto& f : set) {
                ss << sep << f;
                sep = ", ";
            }
            ss << "]";
            return ss.str();
        }

        [[maybe_unused]]
        inline std::string str() const {
            std::stringstream ss;
            for(auto& ff : firsts) {
                ss << "  FIRST(" << ff.first << ") <- " << str(ff.second) << "\n";
            }
            for(auto& ff : follows) {
                ss << "  FOLLOW(" << ff.first << ") <- " << str(ff.second) << "\n";
            }

            ss << "  NULLABLE <- " << str(nullable) << "\n";
            return ss.str();
        }

        inline bool isNullable(const ygp::Node& node) const {
            if(nullable.contains(node.name) == true) {
                return true;
            }

            if(node.name == grammar.empty) {
                return true;
            }
            return false;
        }

        inline bool isRuleNullable(const ygp::Rule& rule, const size_t& from, const size_t& to) const {
            assert(to >= from);
            assert(from <= rule.nodes.size());
            assert(to <= rule.nodes.size());
            if(to == from) {
                return false;
            }

            for(size_t i = from; i < to; ++i) {
                auto& n1 = rule.getNode(i);
                if(isNullable(n1) == false) {
                    return false;
                }
            }
            return true;
        }

        static inline size_t _addToSet(std::set<std::string>& set, const std::string& s) {
            if(set.contains(s) == true) {
                return 0;
            }
            set.insert(s);
            return 1;
        }

        inline size_t addNullable(const std::string& s) {
            auto& set = nullable;
            size_t count = _addToSet(set, s);
            return count;
        }

        inline size_t addFirst(const std::string& r, const std::string& s) {
            auto& set = firsts[r];
            size_t count = _addToSet(set, s);
            return count;
        }

        inline size_t addFollow(const std::string& r, const std::string& s) {
            auto& set = follows[r];
            size_t count = _addToSet(set, s);
            return count;
        }

        inline size_t appendFirsts(const std::string& dst, const std::string& src) {
            auto& sset = firsts[src];
            size_t count = 0;
            for(auto& s : sset) {
                count += addFirst(dst, s);
            }
            return count;
        }

        inline size_t appendFirstsToFollows(const std::string& dst, const std::string& src) {
            auto& sset = firsts[src];
            size_t count = 0;
            for(auto& s : sset) {
                count += addFollow(dst, s);
            }
            return count;
        }

        inline size_t appendFollows(const std::string& dst, const std::string& src) {
            auto& sset = follows[src];
            size_t count = 0;
            for(auto& s : sset) {
                count += addFollow(dst, s);
            }
            return count;
        }
    };

    inline void buildLinks() {
        Links links(grammar);
        for(auto& rxs : grammar.regexSets) {
            links.addFirst(rxs->name, rxs->name);
        }

        // RULE 1: follow(a) contains END, if 'a' is a start symbol
        for(auto& r : grammar.rules) {
            auto& rule = *r;
            if(rule.ruleSetName() == grammar.start) {
                links.addFollow(rule.ruleSetName(), grammar.end);
            }
        }

        size_t changes = 0;
        do {
            changes = 0;
            for(auto& r : grammar.rules) {
                auto& rule = *r;

                size_t k = rule.nodes.size();
                assert(k > 0);

                for(auto& r2 : grammar.rules) {
                    for(size_t idx = 0; idx < r2->nodes.size() - 1; ++idx) {
                        auto& n1 = r2->getNode(idx);
                        if(n1.name != rule.ruleSetName()) {
                            continue;
                        }

                        auto& n2 = r2->getNode(idx + 1);
                        if(n2.isRule() == true) {
                            // RULE 2: follow(a) contains first(b), if 'b' is immediately after 'a' in any of the rules
                            changes += links.appendFirstsToFollows(rule.ruleSetName(), n2.name);
                        }else{
                            // RULE 3: follow(a) contains REGEX1, if REGEX1 is immediately after 'a' in any of the rules
                            assert(n2.isRegex() == true);
                            changes += links.addFollow(rule.ruleSetName(), n2.name);
                        }
                    }
                }

                if(links.isRuleNullable(rule, 0, k) == true) {
                    changes += links.addNullable(rule.ruleSetName());
                }

                auto& n0 = rule.getNode(0);
                if(n0.isRule() == true) {
                    changes += links.appendFirsts(rule.ruleSetName(), n0.name);
                }else{
                    assert(n0.isRegex() == true);
                    changes += links.addFirst(rule.ruleSetName(), n0.name);
                }

                // RULE 4: follow(b) contains follow(a), if 'b' is the last node in any of the rules for 'a'
                auto& nx = rule.getNode(k - 1);
                if(nx.isRule() == true) {
                    for(auto& r2 : grammar.rules) {
                        auto& rule2 = *r2;
                        if(rule2.ruleName == nx.name) {
                            changes += links.appendFollows(nx.name, rule2.ruleName);
                        }
                    }
                }

                changes += links.appendFollows(nx.name, rule.ruleSetName());

                for(size_t i = 0; i < k; ++i) {
                    auto& n1 = rule.getNode(i);
                    if(n1.name == rule.ruleSetName()) {
                        continue;
                    }

                    if(i < (k - 1)) {
                        auto& n2 = rule.getNode(i+1);
                        if(links.isNullable(n2) == true) {
                            changes += links.appendFollows(n1.name, n2.name);
                        }
                    }

                    if(links.isRuleNullable(rule, 0, i) == true) {
                        changes += links.appendFirsts(rule.ruleSetName(), n1.name);
                    }

                    if((i < k) && (links.isRuleNullable(rule, i+1, k) == true)) {
                        changes += links.appendFollows(rule.ruleSetName(), n1.name);
                    }
                }
            }
        }while(changes > 0);

        for(auto& rs : grammar.ruleSets) {
            assert(rs->rules.size() > 0);
            auto& r0 = *(rs->rules.at(0));

            assert(rs->firsts.size() == 0);
            for(auto& ss : links.firsts) {
                if(ss.first != rs->name) {
                    continue;
                }
                for(auto& s : ss.second) {
                    auto& rx = grammar.getRegexSetByName(r0.pos, s);
                    rs->firsts.push_back(&rx);
                }
            }

            assert(rs->follows.size() == 0);
            for(auto& ss : links.follows) {
                if(ss.first != rs->name) {
                    continue;
                }
                for(auto& s : ss.second) {
                    auto& rx = grammar.getRegexSetByName(r0.pos, s);
                    rs->follows.push_back(&rx);
                }
            }
        }
    }

    inline void process() {
        buildLinks();

        // calculate precedence of all rules
        for(auto& r : grammar.rules) {
            if(r->precedence != nullptr) {
                continue;
            }
            auto& anchor = r->getNode(r->anchor);
            if(anchor.isRegex()) {
                auto& rx = grammar.getRegexSet(anchor);
                r->precedence = &rx;
            }else if(anchor.isRule()) {
                auto& rs = grammar.getRuleSetByName(anchor.pos, anchor.name);
                assert(rs.firsts.size() > 0);
                auto& rx = grammar.getRegexSetByName(anchor.pos, rs.firsts.at(0)->name);
                r->precedence = &rx;
            }
        }

        printRules(Logger::olog(), "final");

        // create Parser State Machine
        std::vector<const ygp::Config*> configs;
        for(auto& rule : grammar.rules) {
            if(rule->ruleSetName() == grammar.start) {
                if(!hasRuleInConfigList(configs, *rule)) {
                    auto& ccfg = grammar.createConfig(*rule, 0);
                    configs.push_back(&ccfg);
                }
            }
        }

        if(configs.size() == 0) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_START_RULE");
        }

        auto& sis = createItemSet(configs, "");
        log("linking");
        linkItemSets();

        grammar.initialState = &sis;
    }
};
}

void buildParser(yg::Grammar& g) {
    ParserStateMachineBuilder psmb{g};
    psmb.process();
}
