#include "pch.hpp"
#include "parser_builder.hpp"
#include "logger.hpp"

namespace {
struct ParserStateMachineBuilder {
    yg::Grammar& grammar;
    inline ParserStateMachineBuilder(yg::Grammar& g) : grammar{g} {}

    inline void printRules(std::ostream& os, const std::string& msg) const {
        print(os, "--------------------------------");
        print(os, "GRAMMAR({}):{}", grammar.rules.size(), msg);
        for(auto& rs : grammar.regexSets) {
            print(os, "TOKEN: T{}: name={} [{}]", rs->id, rs->name, rs->precedence);
            for(auto& rx : rs->regexes) {
                print(os, "  REGEX: X{}: name={} [{}]", rx->id, rx->regexName, rx->regexSet->precedence);
            }
        }


        for(auto& rule : grammar.rules) {
            print(os, "RULE<{}>: {}", rule->ruleSet->name, rule->str());
        }
    }

    [[maybe_unused]]
    inline void
    printConfigList(const std::string& msg, const std::vector<const yg::Grammar::Config*>& cfgs) {
        print("{}({})", msg, cfgs.size());
        for(auto& cfg : cfgs) {
            print("-cfg:{}", cfg->str());
        }
    }

    inline bool hasRuleInConfigList(const std::vector<const yg::Grammar::Config*>& configs, const yg::Grammar::Rule& r) {
        for(auto& c : configs) {
            if(&(c->rule) == &r) {
                return true;
            }
        }
        return false;
    }

    inline std::vector<const yg::Grammar::Config*>
    expandConfig(const std::vector<const yg::Grammar::Config*>& initConfig) {
        // create all sub-configs
        std::vector<const yg::Grammar::Config*> configs;

        auto nexts = initConfig;

        while(nexts.size() > 0) {
            std::vector<const yg::Grammar::Config*> firsts;
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

    inline char resolveAssoc(const yg::Grammar::RegexSet& rx) const {
        if(rx.assoc == yg::Grammar::RegexSet::Assoc::Left) {
            return 'R';
        }
        return 'S';
    }

    inline char resolveConflict(const yg::Grammar::RegexSet& rx, const yg::Grammar::Config& cfg) const {
        auto& r = cfg.rule;

        // if we are resolving a conflict against the END token,
        // always resolve in favor of a REDUCE.
        if(rx.name == grammar.end) {
            return 'R';
        }
        if(rx.precedence < r.precedence->precedence) {
            return 'S';
        }
        if(rx.precedence > r.precedence->precedence) {
            return 'R';
        }
        return resolveAssoc(rx);
    }

    inline void addReduce(
        yg::Grammar::ConfigSet& cs,
        const yg::Grammar::Config& config,
        const size_t& len,
        const std::string& indent
    ) {
        print(Logger::log(), "{}addReduce:cfg={}, next=null", indent, config.str());
        auto& rs = grammar.getRuleSetByName(config.rule.pos, config.rule.ruleSetName());

        print(Logger::log(), "{}addReduce:rs={}", indent, rs.name);

        for(auto& rx : rs.follows) {
            char p = resolveConflict(*rx, config);
            if(cs.hasShift(*rx) != nullptr) {
                std::stringstream ss;
                print(Logger::log(), "{}addReduce:{}: REDUCE-SHIFT conflict on:{}{}", indent, config.rule.pos.str(), rx->name, ss.str());
                if(p == 'R') {
                    print(Logger::log(), "{}addReduce: rx={}, p={}, R-S conflict, reducing", indent, rx->name, p);
                    Logger::log().flush();
                    cs.shifts.erase(rx);
                    assert(cs.hasReduce(*rx) == nullptr);
                    cs.addReduce(*rx, config, len);

                    print(Logger::log(), "{}addReduce: Resolved in favor of REDUCE", indent);
                }else{
                    print(Logger::log(), "{}addReduce: rx={}, p={}, R-S conflict, shifting", indent, rx->name, p);
                    print(Logger::log(), "{}addReduce: Resolved in favor of SHIFT", indent);
                }
                return;
            }

            if(p == 'R') {
                print(Logger::log(), "{}addReduce: rx={}, p={}, reducing, r={}", indent, rx->name, p, config.str());
                // if(auto c = cs.hasReduce(*rx); c != nullptr) {
                //     print(Logger::log(), "{}addReduce: R-R conflict", indent);
                //     if(c->next != &config) {
                //         print(Logger::log(), "{}- prev-config={}", indent, c->next->str());
                //         print(Logger::log(), "{}- next-config={}", indent, config.str());
                //         // throw GeneratorError(__LINE__, __FILE__, config.rule->pos, "REDUCE_REDUCE_CONFLICT:ON:{}", rx->name);
                //         continue;
                //     }
                // }
                cs.addReduce(*rx, config, len);
                continue;
            }

            // SR conflict resolved in favor of SHIFT
            assert(p == 'S');

            // check if there are any matching configs
            auto& lastNode = *(config.rule.nodes.back());
            print(Logger::log(), "{}addReduce R-S conflict:lastNode={}, rx={}", indent, lastNode.name, rx->name);
            std::vector<const yg::Grammar::Config*> ncfgs;
            for(auto& r : grammar.rules) {
                if(r->nodes.size() < 2) {
                    continue;
                }
                if((r->nodes.at(0)->name == lastNode.name) && (r->nodes.at(1)->name == rx->name)) {
                    auto& ncfg = grammar.createConfig(*r, 2);
                    ncfgs.push_back(&ncfg);
                }
            }

            // if matching configs found, shift to that
            if(ncfgs.size() > 0) {
                std::stringstream ss;
                ss << std::format("\n{}:REDUCE:{}", config.rule.pos.str(), config.str());
                for(auto& ncfg : ncfgs) {
                    ss << std::format("\n{}: SHIFT:{}", ncfg->rule.pos.str(), ncfg->str());
                }
                if(grammar.autoResolve == false) {
                    throw GeneratorError(__LINE__, __FILE__, config.rule.pos, "REDUCE_SHIFT_CONFLICT:ON:{}{}", rx->name, ss.str());
                }else if(grammar.warnResolve == true) {
                    print(Logger::log(), "{}addReduce: {}: REDUCE-SHIFT conflict on:{}{}", indent, config.rule.pos.str(), rx->name, ss.str());
                    print(Logger::log(), "Resolved in favor of SHIFT");
                }
                print(Logger::log(), "{}addReduce: REDUCE-SHIFT:shifting {}", indent, rx->name);
                cs.moveShifts(*rx, ncfgs, {});
                continue;
            }

            // else reduce by default
            print(Logger::log(), "{}addReduce: REDUCE-SHIFT:reducing {}, rx={}", indent, config.str(), rx->name);
            if(auto r = cs.hasReduce(*rx)) {
                unused(r);
                // assert(r->next != nullptr);
                // auto& ocfg = *(r->next);
                // print(Logger::log(), "{}addReduce: REDUCE-SHIFT:ocfg={}", indent, ocfg.str());
                // assert(ocfg == &config);
                continue;
            }
            assert(cs.hasReduce(*rx) == nullptr);
            cs.addReduce(*rx, config, len);
        }
    }

    inline void addShift(
        yg::Grammar::Node& nextNode,
        yg::Grammar::ConfigSet& cs,
        const yg::Grammar::Config& config,
        const size_t& cpos,
        const std::vector<const yg::Grammar::RuleSet*>& epsilons,
        const std::string& indent
    ) {
        print(Logger::log(), "{}addShift:cfg={}, next=regex", indent, config.str());
        if(nextNode.name == grammar.empty) {
            print(Logger::log(), "{}addShift:skip_empty", indent);
            return;
        }
        auto& rx = grammar.getRegexSet(nextNode);
        if(cs.hasReduce(rx) != nullptr) {
            std::stringstream ss;
            char p = resolveConflict(rx, config);
            print(Logger::log(), "{}addShift: {}: SHIFT-REDUCE conflict on:{}{}, p={}", indent, config.rule.pos.str(), rx.name, ss.str(), p);
            if(p == 'R') {
                return;
            }
            cs.reduces.erase(&rx);
        }
        bool hasExisting = false;
        if(cs.hasShift(rx)) {
            for(auto& xcfg : cs.shifts[&rx].next) {
                if(&(xcfg->rule) == &(config.rule)) {
                    hasExisting = true;
                    break;
                }
            }
        }
        if(hasExisting == false) {
            auto& ncfg = grammar.createConfig(config.rule, cpos + 1);
            cs.addShift(rx, ncfg, epsilons);
        }
    }

    inline void addGoto(
        yg::Grammar::Node& nextNode,
        yg::Grammar::ConfigSet& cs,
        const yg::Grammar::Config& config,
        const size_t& cpos,
        const std::string& indent
    ) {
        print(Logger::log(), "{}addGoto:cfg={}, cpos={}, nextNode={}", indent, config.str(), cpos, nextNode.str());

        // add GOTO for rule node
        auto& ncfg = grammar.createConfig(config.rule, cpos + 1);
        auto& rs = grammar.getRuleSetByName(nextNode.pos, nextNode.name);
        assert(cs.hasGoto(rs, ncfg) == false);
        cs.gotos[&rs].push_back(&ncfg);
    }

    inline void getNextConfigSet(yg::Grammar::ItemSet& is, const std::string& indent) {
        print(Logger::log(), "{}getNextConfigSet:is={}", indent, is.id);
        yg::Grammar::ConfigSet cs;

        for(auto& c : is.configs) {
            auto& config = *c;
            const yg::Grammar::Node* epsilonNode = nullptr;
            std::vector<const yg::Grammar::RuleSet*> epsilons;
            auto cpos = config.cpos;
            do {
                print(Logger::log(), "{}getNextConfigSet({}):cfg={}, cpos={}", indent, is.id, config.str(), cpos);
                assert(cpos <= config.rule.nodes.size());
                bool first = (epsilonNode == nullptr);
                epsilonNode = nullptr;
                auto nextNode = config.rule.getNodeAt(cpos);
                if(nextNode == nullptr) {
                    auto len = config.rule.nodes.size() - (cpos - config.cpos);
                    print(Logger::log(), "{}getNextConfigSet({}):is-end:len={}", indent, is.id, len);
                    addReduce(cs, config, len, indent);
                }else if(nextNode->isRegex()) {
                    if(nextNode->name == grammar.empty) {
                        print(Logger::log(), "{}getNextConfigSet({}):is-regex-empty:{}", indent, is.id, nextNode->name);
                    //     epsilonNode = nextNode;
                    }else if(nextNode->name == grammar.end) {
                        auto len = config.rule.nodes.size() - (cpos - config.cpos);
                        print(Logger::log(), "{}getNextConfigSet({}):is-regex-end:{}, len={}, cfg={}", indent, is.id, nextNode->name, len, config.str(false));
                        assert(len > 0);
                        addReduce(cs, config, len, indent);
                    }else{
                        print(Logger::log(), "{}getNextConfigSet({}):is-regex:{}", indent, is.id, nextNode->name);
                        addShift(*nextNode, cs, config, cpos, epsilons, indent);
                    }
                }else if(nextNode->isRule()) {
                    print(Logger::log(), "{}getNextConfigSet({}):is-rule:{}", indent, is.id, nextNode->name);
                    if(first == true) {
                        addGoto(*nextNode, cs, config, cpos, indent);
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

        // set is.shifts
        for(auto& c : cs.shifts) {
            auto& rx = c.first;
            auto& cfgs = c.second;
            auto xcfgs = expandConfig(cfgs.next);
            is.configSet.moveShifts(*rx, xcfgs, cfgs.epsilons);
        }

        // set is.gotos
        for(auto& c : cs.gotos) {
            auto& rs = c.first;
            auto& cfgs = c.second;
            auto xcfgs = expandConfig(cfgs);
            is.configSet.gotos[rs] = xcfgs;
        }

        // set is.reduces
        is.configSet.reduces = cs.reduces;
    }

    inline void linkItemSets() {
        for(auto& pis : grammar.itemSets) {
            auto& is = *pis;
            print(Logger::log(), "linkItemSets:is={}", is.id);

            for(auto& c : is.configSet.gotos) {
                auto& rs = c.first;
                auto& cfgs = c.second;
                print(Logger::log(), "  goto: rs={}, next_sz={}", rs->name, cfgs.size());
                assert(cfgs.size() > 0);
                auto& config = *(cfgs.at(0));
                auto& nextNode = config.rule.getNode(0);
                auto& cis = grammar.getItemSet(config.rule.pos, cfgs);
                is.setGoto(nextNode, rs, &cis);
            }

            for(auto& c : is.configSet.shifts) {
                auto& rx = c.first;
                auto& cfgs = c.second;
                print(Logger::log(), "  shift: rx={}, next_sz={}", rx->name, cfgs.next.size());
                assert(cfgs.next.size() > 0);
                auto& config = *(cfgs.next.at(0));
                auto& nextNode = config.rule.getNode(0);
                auto& cis = grammar.getItemSet(config.rule.pos, cfgs.next);
                assert(is.hasShift(*rx) == nullptr);
                assert(is.hasReduce(*rx) == nullptr);
                is.setShift(nextNode, *rx, cis, cfgs.epsilons);
            }

            for(auto& c : is.configSet.reduces) {
                auto& rx = *(c.first);
                auto& cfgs = c.second;
                print(Logger::log(), "  reduce: rx={}, next_sz={}", rx.name, cfgs.next.size());
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

    inline yg::Grammar::ItemSet& createItemSet(const std::vector<const yg::Grammar::Config*>& initConfig, const std::string& indent) {
        auto configs = expandConfig(initConfig);
        if(auto xis = grammar.hasItemSet(configs)) {
            print(Logger::log(), "{}Found Existing ItemSet:{}", indent, xis->id);
            return *xis;
        }

        auto& is = grammar.createItemSet(configs);
        print(Logger::log(), "{}Created ItemSet:{}", indent, is.id);
        getNextConfigSet(is, indent);

        for(auto& c : is.configSet.shifts) {
            auto& cfgs = c.second;
            assert(cfgs.next.size() > 0);
            createItemSet(cfgs.next, indent + "  ");
        }

        for(auto& c : is.configSet.gotos) {
            auto& cfgs = c.second;
            assert(cfgs.size() > 0);
            createItemSet(cfgs, indent + "  ");
        }

        return is;
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

        inline bool isNullable(const yg::Grammar::Node& node) const {
            if(nullable.contains(node.name) == true) {
                return true;
            }

            if(node.name == grammar.empty) {
                return true;
            }
            return false;
        }

        inline bool isRuleNullable(const yg::Grammar::Rule& rule, const size_t& from, const size_t& to) const {
            assert(to >= from);
            assert((from >= 0) && (from <= rule.nodes.size()));
            assert((to >= 0) && (to <= rule.nodes.size()));
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

        printRules(Logger::log(), "final");

        // create Parser State Machine
        std::vector<const yg::Grammar::Config*> configs;
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
        print(Logger::log(), "linking");
        linkItemSets();

        grammar.initialState = &sis;
    }
};
}

void buildParser(yg::Grammar& g) {
    ParserStateMachineBuilder psmb{g};
    psmb.process();
}
