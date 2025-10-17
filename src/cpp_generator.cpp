/// @file cpp_generator.cpp
/// @brief This file contains all the code for generating the parser file
///
/// # EMBEDDED PROTOTYPE
/// The prototype for the generated file is stored in prototype.cpp
/// The cmake script converts prototype.cpp to cb_prototype.cpp, which
/// contains the text of prototype.cpp as a raw C string.
/// The text of the prototype.cpp file is thus embedded in the Yantra executable
/// and used to create the generated parser file
/// The embedded files have metacommands prefixed by ///PROTOTYPE indicating
/// the location where generated code must be inserted
/// The Generator class in this file is a set of functions named generateXXX()
/// which are handlers for the metacommands.
/// This technique is used for a couple of other files, such as stream and encodings.
///
/// # RULESET VISITORS
/// A grammar can contain more that one rule reducing to the same rule name. e.g:
/// ```
/// stmts := stmts(l) stmt(s);
/// stmts := stmt(s);
/// ```
/// All rules with the same name form a @ref RuleSet
/// Each RuleSet defines a std::variant that holds all rules in the set
/// and uses a std::visit to invoke the appropriate semantic actions for the rules

#include "pch.hpp"
#include "cpp_generator.hpp"
#include "logger.hpp"

/// @brief This is the UNICODE encoding file, embedded as a raw C string
extern const char* utf8Encoding;

/// @brief This is the ASCII encoding file, embedded as a raw C string
extern const char* asciiEncoding;

/// @brief This is the prototype.cpp file, embedded as a raw C string
extern const char* cbPrototype;

/// @brief This is the stream.hpp file, embedded as a raw C string
extern const char* cbStream;

/// @brief This is a flag that indicates whether to insert #line statements for codeblocks in the generated file
extern bool genLines;

namespace {

/// @brief This class generates the cpp parser
struct Generator {
    const yg::Grammar& grammar;
    std::string qid;
    CodeBlock throwError;

    inline Generator(const yg::Grammar& g) : grammar{g} {}

    /// @brief expands all variables in a codeblock and normalizes the indentation
    inline std::string expand(const std::string_view& codeblock, const std::string_view& indent, const bool& autoIndent, const std::unordered_map<std::string, std::string>& vars) {
        enum class State {
            InitSpace0,
            InitSpace1,
            Code,
            Tag0,
            Tag1,
            Tag2,
            Tag3,
        };
        static std::unordered_map<State, const char*> sname = {
            {State::InitSpace0, "InitSpace0"},
            {State::InitSpace1, "InitSpace1"},
            {State::Code, "Code"},
            {State::Tag0, "Tag0"},
            {State::Tag1, "Tag1"},
            {State::Tag2, "Tag2"},
            {State::Tag3, "Tag3"},
        };

        State state = State::InitSpace0;
        size_t space0 = 0;
        size_t space1 = 0;
        std::string rv;
        std::string key;
        for(auto& ch : codeblock) {
            switch(state) {
            case State::InitSpace0:
                if((ch == '\r') || (ch == '\n')) {
                    space0 = 0;
                    continue;
                }
                if(autoIndent && (std::isspace(ch))) {
                    ++space0;
                    continue;
                }
                rv += indent;
                if (ch == 'T') {
                    state = State::Tag0;
                    key = "";
                    continue;
                }
                rv += ch;
                space1 = 0;
                state = State::Code;
                break;
            case State::InitSpace1:
                if(ch == '\r') {
                    continue;
                }
                if(ch == '\n') {
                    rv += ch;
                    continue;
                }
                if(autoIndent && (std::isspace(ch)) && (ch != '\n')) {
                    if(space1 < space0) {
                        ++space1;
                        continue;
                    }
                }
                rv += indent;
                if (ch == 'T') {
                    state = State::Tag0;
                    key = "";
                    continue;
                }
                rv += ch;
                state = State::Code;
                break;
            case State::Code:
                if(ch == '\r') {
                    continue;
                }
                if(ch == '\n') {
                    rv += ch;
                    space1 = 0;
                    state = State::InitSpace1;
                    continue;
                }
                if(ch == 'T') {
                    state = State::Tag0;
                    key = "";
                    continue;
                }
                rv += ch;
                state = State::Code;
                break;
            case State::Tag0:
                if (ch == 'A') {
                    state = State::Tag1;
                    continue;
                }
                rv += 'T';
                rv += ch;
                state = State::Code;
                break;
            case State::Tag1:
                if (ch == 'G') {
                    state = State::Tag2;
                    continue;
                }
                rv += "TA";
                rv += ch;
                state = State::Code;
                break;
            case State::Tag2:
                if (ch == '(') {
                    state = State::Tag3;
                    continue;
                }
                rv += "TAG";
                rv += ch;
                state = State::Code;
                break;
            case State::Tag3:
                if (ch == ')') {
                    if (auto it = vars.find(key); it != vars.end()) {
                        rv += it->second;
                    }else{
                        rv += ("TAG(" + key + ")");
                    }
                    state = State::Code;
                }
                else if ((isalpha(ch)) || (ch == '_')) {
                    key += ch;
                }else{
                    rv += ("TAG(" + key);
                    state = State::Code;
                }
                break;
            }
        }

        return rv;
    }

    /// @brief optionally appends a #line to an expanded codeblock
    inline std::string expand(const CodeBlock& codeblock, const std::string_view& indent, const bool& autoIndent, const std::unordered_map<std::string, std::string>& vars) {
        auto cb = expand(codeblock.code, indent, autoIndent, vars);
        if (codeblock.hasPos == false) {
            return cb;
        }

        std::string pline = (genLines == false) ? "//" : "";
        auto pos = std::format("{}{}#line {} \"{}\"", indent, pline, codeblock.pos.row, codeblock.pos.file);
        return std::format("\n{}\n{}", pos, cb);
    }

    /// @brief writes expanded codeblock to output stream if the codeblock is not empty
    inline void expand(std::ostream& os, const std::string_view& codeblock, const std::string_view& indent, const bool& autoIndent, const std::unordered_map<std::string, std::string>& vars) {
        if(codeblock.empty() == true) {
            return;
        }
        print(os, "{}", expand(codeblock, indent, autoIndent, vars));
    }

    /// @brief writes expanded codeblock to output stream if the codeblock is not empty
    inline void expand(std::ostream& os, const CodeBlock& codeblock, const std::string_view& indent, const bool& autoIndent, const std::unordered_map<std::string, std::string>& vars) {
        auto cb = expand(codeblock, indent, autoIndent, vars);
        if (cb.empty() == true) {
            return;
        }
        print(os, "{}", cb);
    }

    /// @brief Checks if the \ref yg::Grammar::RuleSet contains a empty prooduction
    inline bool hasEpsilon(const yg::Grammar::RuleSet& rs) const {
        return (rs.firstIncludes(grammar.empty) == true);
    }

    /// @brief Construct a full function name given a rule and a short function name
    inline std::string getFunctionName(const yg::Grammar::Rule& r, const std::string& fname) const {
        return std::format("{}_{}", r.ruleName, fname);
    }

    /// @brief Get the type for any node in a production.
    /// If the node is a terminal, returns the default token class name
    /// If non-terminal, return the AST node name for the rule
    /// (which is the same as the rule name)
    inline std::string getNodeType(const yg::Grammar::Node& n) const {
        std::string nativeType;
        if(n.isRegex() == true) {
            nativeType = grammar.tokenClass;
        }else{
            assert(n.isRule() == true);
            nativeType = std::format("{}", n.name);
        }
        return nativeType;
    }

    /// @brief generates a list of args for a semantic action function for a rule
    inline std::string getArgs(
        const yg::Grammar::Rule& r,
        const yg::Grammar::Walker::FunctionSig& fsig,
        const yg::Grammar::Walker::CodeInfo* ci,
        const std::string& indent
        ) {
        std::stringstream args;
        std::string sep;

        std::string isUnused;
        if(ci == nullptr) {
            isUnused = "[[maybe_unused]]";
        }

        for(auto& n : r.nodes) {
            auto varName = n->varName;
            if(varName.size() == 0) {
                continue;
            }

            std::string nativeTypeNode;
            if(n->isRegex() == true) {
                nativeTypeNode = std::format("const {}", grammar.tokenClass);
            }else{
                nativeTypeNode = std::format("WalkerNodeRef<{}>", n->name);
            }

            args << std::format("{}{}{}& {}", sep, isUnused, nativeTypeNode, varName);
            sep = "," + indent;
        }
        if(fsig.args.size() > 0) {
            args << std::format("{}{}", sep, fsig.args);
        }
        return args.str();
    }

    /// @brief extract argument names into a comma-separated list
    /// a function can be defined as
    /// ```%function rTypeRef CppServer::str(int x, const std::string& y) -> std::string;```
    /// this function parses the source string `int x, const std::string& y` into final string `x, y`
    /// It does this by
    /// - iterating the source string in reverse order,
    /// - extracting the variable name
    /// - pushing it into a stack.
    /// - skipping the rest of the type
    /// This logic assumes that the variable name is always after the type.
    /// It will fail in cases where the variable name is inside the type-spec
    /// such as in case of function pointers
    inline std::string extractParams(const std::string& args) {
        enum class State {
            Init,
            Var,
            Type,
        };

        std::vector<std::string> vars;
        State state = State::Init;
        std::string var;
        for(auto it = args.rbegin(), ite = args.rend(); it != ite; ++it) {
            auto& ch = *it;
            switch(state) {
            case State::Init:
                // print("Init: ch={}", ch);
                if(std::isspace(ch)) {
                    break;
                }
                var = ch;
                state = State::Var;
                break;
            case State::Var:
                // print("Var: ch={}", ch);
                if(std::isspace(ch)) {
                    std::reverse(var.begin(), var.end());
                    vars.push_back(var);
                    state = State::Type;
                    break;
                }
                var += ch;
                break;
            case State::Type:
                // print("Type: ch={}", ch);
                if(ch == ',') {
                    state = State::Init;
                    break;
                }
                break;
            }
        }

        std::stringstream ss;
        std::string sep;
        while(vars.size() > 0) {
            ss << sep << vars.back();
            sep = ", ";
            vars.pop_back();
        }
        return ss.str();
    }

    /// @brief writes expanded codeblock to throw an exception
    /// This is called at various points in the generation code
    inline std::string generateError(const std::string& line, const std::string& col, const std::string& file, const std::string& msg, const std::string_view& indent, const std::unordered_map<std::string, std::string>& vars) {
        auto xvars = vars;
        xvars["ROW"] = line;
        xvars["COL"] = col;
        xvars["SRC"] = file;
        xvars["MSG"] = msg;
        return expand(throwError.code, indent, true, xvars);
    }

    /// @brief generates code to include PCH
    inline void generatePchHeader(std::ostream& os, const std::string_view& indent) {
        print(os, "{}#include \"{}\"", indent, grammar.pchHeader);
    }

    /// @brief generates code to include header files in the parser's header file
    inline void generateHdrHeaders(std::ostream& os, const std::string_view& indent) {
        for (auto& h : grammar.hdrHeaders) {
            print(os, "{}#include \"{}\"", indent, h);
        }
    }

    /// @brief generates code to include header files in the parser's source file
    inline void generateSrcHeaders(std::ostream& os, const std::string_view& indent) {
        for (auto& h : grammar.srcHeaders) {
            print(os, "{}#include \"{}\"", indent, h);
        }
    }

    /// @brief generates additional class members for the parser, if any are specified in the .y file
    inline void generateClassMembers(std::ostream& os, const std::string_view& indent) {
        for (auto& m : grammar.classMembers) {
            print(os, "{}{};", indent, m);
        }
    }

    /// @brief generates declarations for all AST nodes
    inline void generateAstNodeDecls(std::ostream& os, const std::string_view& indent) {
        for (auto& rs : grammar.ruleSets) {
            print(os, "{}struct {};", indent, rs->name);
        }
    }

    /// @brief generates definitions for all AST nodes
    inline void generateAstNodeDefns(std::ostream& os, const std::string_view& indent) {
        for (auto& rs : grammar.ruleSets) {
            print(os, "{}struct {} : public NonCopyable {{", indent, rs->name);
            print(os, "{}    const FilePos pos;", indent);

            if(hasEpsilon(*rs) == true) {
                print(os, "{}    struct {}_r0 {{", indent, rs->name);
                print(os, "{}    }};\n", indent);
            }

            for (auto& r : rs->rules) {
                print(os, "{}    struct {} : public NonCopyable {{", indent, r->ruleName);
                std::ostringstream pos;
                std::ostringstream ios;
                std::string sep;
                std::string coln;
                for (size_t idx = 0; idx < r->nodes.size(); ++idx) {
                    auto& n = r->nodes.at(idx);
                    auto varName = n->varName;
                    if (n->varName.size() == 0) {
                        varName = std::format("{}{}", n->name, idx);
                    }
                    auto rt = getNodeType(*n);
                    print(os, "{}        {}& {};", indent, rt, varName);
                    pos << std::format("{}{}& p{}", sep, rt, varName);
                    ios << std::format("{}{}(p{})", sep, varName, varName);
                    sep = ", ";
                    coln = " : ";
                }
                print(os, "{}        explicit inline {}({}){}{} {{}}", indent, r->ruleName, pos.str(), coln, ios.str());
                print(os, "{}    }};\n", indent);
            }

            std::string sep;
            print(os, "{}    using Rule = std::variant<", indent);
            print(os, "{}        {}std::monostate", indent, sep);
            sep = ",";
            if(hasEpsilon(*rs) == true) {
                print(os, "{}        {}{}_r0", indent, sep, rs->name);
                sep = ",";
            }
            for (auto& r : rs->rules) {
                print(os, "{}        {}{}", indent, sep, r->ruleName);
                sep = ",";
            }
            print(os, "{}    >;\n", indent);

            print(os, "{}    const {}& anchor;", indent, grammar.tokenClass);
            print(os, "{}    Rule rule;\n", indent);

            auto anchorArg = std::format(", const {}& a", grammar.tokenClass);
            auto anchorCtor = ", anchor(a)";
            print(os, "{}    explicit inline {}(const FilePos& p{}) : pos(p){} {{}}", indent, rs->name, anchorArg, anchorCtor);
            print(os, "{}}}; // struct {}\n", indent, rs->name);
        }
    }

    /// @brief generates a variant that contains all AST nodes
    inline void generateAstNodeItems(std::ostream& os, const std::string_view& indent) {
        print(os, "{}using AstNode = std::variant<", indent);
        print(os, "{}    {}", indent, grammar.tokenClass);
        for (auto& rs : grammar.ruleSets) {
            print(os, "{}    ,{}", indent, rs->name);
        }
        print(os, "{}>;", indent);
    }

    /// @brief generates member function corresponding to the semantic action for each rule
    inline void generateRuleHandler(
        std::ostream& os,
        const yg::Grammar::Walker& walker,
        const yg::Grammar::Rule& r,
        const yg::Grammar::Walker::FunctionSig& fsig,
        const std::string& isVirtual,
        const std::string& isOverride,
        const yg::Grammar::Walker::CodeInfo* ci,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        auto fname = getFunctionName(r, fsig.func);
        std::string returnType = fsig.type;

        // infer indent for function args
        print(os, "{}//RULE_HANDLER({}):{}", indent, walker.name, r.str(false));
        auto xindent = std::format("\n    {}", indent);

        // get args list as indented string
        auto args = getArgs(r, fsig, ci, xindent);

        // open function
        if(args.size() > 0) {
            print(os, "{}{} {}", indent, isVirtual, returnType);
            print(os, "{}{}(", indent, fname);
            print(os, "{}    {}", indent, args);
            print(os, "{}){} {{", indent, isOverride);
        }else{
            print(os, "{}{} {} {}(){} {{", indent, isVirtual, returnType, fname, isOverride);
        }

        // generate function body, if any
        if(ci != nullptr) {
            expand(os, ci->codeblock, std::string(indent) + "    ", true, vars);
        }

        // close function
        print(os, "{}}}\n", indent);
    }

    /// @brief generates visitor overload to invoke the corresponding member function
    inline void generateRuleVisitorBody(
        std::ostream& os,
        const yg::Grammar::Walker& walker,
        const yg::Grammar::Walker::FunctionSig& fsig,
        const yg::Grammar::RuleSet& rs,
        const yg::Grammar::Rule& r,
        const std::string& called,
        const std::string& xparams,
        const std::string_view& indent
    ) {
        std::stringstream wnodes;
        std::stringstream wcalls;
        std::stringstream params;
        std::stringstream ss;
        bool rnode_used = false;

        // generate invocation code
        std::string sep;
        std::string nsep;
        for (auto& n : r.nodes) {
            auto nrt = grammar.tokenClass;
            if(n->isRule() == true) {
                nrt = n->name;
            }

            if (n->varName.size() > 0) {
                if(fsig.isUDF == false) {
                    if(n->isRule() == true) {
                        wnodes << std::format("{}            WalkerNodeRef<{}> {}(_n.{}, {});/*wvar1u*/\n", indent, n->name, n->varName, n->varName, called);
                        wcalls << std::format("{}            if({}.called == false) {}({});/*wvar1*/\n", indent, n->varName, fsig.func, n->varName);
                    }else{
                        wnodes << std::format("{}            const {}& {} = _n.{};/*wvar1x*/\n", indent, grammar.tokenClass, n->varName, n->varName);
                    }
                    params << std::format("{}{}/*wvar1*/", sep, n->varName);
                    rnode_used = true;
                }else {
                    if(n->isRule() == true) {
                        wnodes << std::format("{}            WalkerNodeRef<{}> {}(_n.{}, {});/*gvar2u*/\n", indent, n->name, n->varName, n->varName, called);
                    }else{
                        wnodes << std::format("{}            const {}& {} = _n.{};/*gvar2x*/\n", indent, grammar.tokenClass, n->varName, n->varName);
                    }
                    params << std::format("{}{}/*gvar2*/", sep, n->varName);
                    rnode_used = true;
                }
                sep = ", ";

                ss << std::format("{}{}", nsep, n->varName);
                nsep = ", ";
            }else{
                if(fsig.isUDF == false) {
                    if(n->name != grammar.end) {
                        if(n->isRule() == true) {
                            wnodes << std::format("{}            WalkerNodeRef<{}> {}(_n.{}, {});/*nwvar1u*/\n", indent, n->name, n->idxName, n->idxName, called);
                            wcalls << std::format("{}            {}({});/*nwvar1*/\n", indent, fsig.func, n->idxName);
                            rnode_used = true;
                        }
                        ss << std::format("{}{}", nsep, n->idxName);
                        nsep = ", ";
                    }
                }
            }
        }

        if(xparams.size() > 0) {
            params << std::format("{}{}", sep, xparams);
        }

        std::string node_unused = (rnode_used == false)?"[[maybe_unused]]":"";
        auto iname = getFunctionName(r, fsig.func);

        print(os, "{}        [&]({}const {}::{}& _n) -> {} {{", indent, node_unused, rs.name, r.ruleName, fsig.type);

        print(os, "{}", wnodes.str());

        // call the handler
        if(fsig.isUDF == false) {
            print(os, "{}            {}({});/*call3*/", indent, iname, params.str());
        }else{
            print(os, "{}            return {}({});/*call4*/", indent, iname, params.str());
        }

        if(walker.traversalMode == yg::Grammar::Walker::TraversalMode::TopDown) {
            print(os, "{}", wcalls.str());
        }
        print(os, "{}        }},\n", indent);
    }

    /// @brief generates ruleset handler
    /// This function implements a visitor that calls the
    /// specific rule handler
    inline void generateRuleSetVisitor(
        std::ostream& os,
        const yg::Grammar::Walker& walker,
        const yg::Grammar::RuleSet& rs,
        const yg::Grammar::Walker::FunctionSig& fsig,
        const std::string_view& indent
    ) {
        print(os, "{}//RULESET_VISITOR({}):{}:{}", indent, walker.name, rs.name, fsig.type);

        std::string args;
        if(fsig.args.size() > 0) {
            args = std::format(", {}", fsig.args);
        }
        print(os, "{}inline {}", indent, fsig.type);
        print(os, "{}{}(WalkerNodeRef<{}>& node{}) {{", indent, fsig.func, rs.name, args);
        if(fsig.isUDF == false) {
            print(os, "{}    WalkerNodeCommit<{}> _wc(node);", indent, rs.name);
        }

        auto xparams = extractParams(fsig.args);

        std::string called;
        if(walker.traversalMode == yg::Grammar::Walker::TraversalMode::Manual) {
            called = "true";
        }else{
            called = "false";
        }

        // generate Visitor
        print(os, "{}    return std::visit(overload{{", indent);

        if(true) {
            print(os, "{}        [&](const std::monostate&) -> {} {{", indent, fsig.type);
            print(os, "{}            throw std::runtime_error(\"internal_error\"); //should never reach here", indent);
            print(os, "{}        }},\n", indent);
        }

        if(hasEpsilon(rs) == true) {
            print(os, "{}        [&](const {}::{}_r0&) -> {} {{", indent, rs.name, rs.name, fsig.type);
            print(os, "{}            //throw std::runtime_error(\"internal_error(epsilon)\"); //should never reach here", indent);
            print(os, "{}        }},\n", indent);
        }

        // generate handlers for all rules in ruleset
        for (auto& r : rs.rules) {
            generateRuleVisitorBody(os, walker, fsig, rs, *r, called, xparams, indent);
        }
        print(os, "{}    }}, node.node.rule);", indent);

        print(os, "{}}}\n", indent);
    }

    /// @brief generates all ruleset handlers
    inline void generateRuleSetVisitors(
        std::ostream& os,
        const yg::Grammar::Walker& walker,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        if(grammar.isRootWalker(walker) == true) {
            print(os, "{}inline {}& {}({}& node) const {{", indent, grammar.tokenClass, walker.defaultFunctionName, grammar.tokenClass);
            print(os, "{}    return node;", indent);
            print(os, "{}}}\n", indent);
        }

        std::string isVirtual = (grammar.isBaseWalker(walker) == true)?"virtual":"inline";
        std::string isOverride;
        if(grammar.isDerivedWalker(walker) == true) {
            isOverride = " override";
            isVirtual = "virtual";
        }

        for(auto& prs : grammar.ruleSets) {
            auto& rs = *prs;
            auto funcl = walker.getFunctions(rs);
            for(auto& pfi : funcl) {
                auto& fi = *pfi;
                auto isv = isVirtual;
                auto iso = isOverride;
                if(fi.isUDF == true) {
                    isv = "inline";
                    iso = "";
                }

                for(auto& pr : rs.rules) {
                    auto& r = *pr;
                    auto ci = walker.hasCodeblock(r, fi.func);
                    if((grammar.isDerivedWalker(walker) == true) && (ci == nullptr)) {
                        continue;
                    }
                    generateRuleHandler(os, walker, r, fi, isv, iso, ci, vars, indent);
                }

                if((grammar.isRootWalker(walker) == true) || (fi.isUDF == true)) {
                    print(os, "{}//RULE_VISITOR({}):{}", indent, walker.name, rs.name);
                    generateRuleSetVisitor(os, walker, rs, fi, indent);
                }
            }
        }
    }

    /// @brief generates writer for Walker
    inline void generateWriter(
        std::ostream& os,
        const yg::Grammar::Walker& walker,
        const std::string_view& indent
    ) {
        if(walker.outputType == yg::Grammar::Walker::OutputType::TextFile) {
            print(os, "{}//GEN_FILE", indent);
            print(os, "{}TextFileWriter {};\n", indent, walker.writerName);

            print(os, "{}inline void open(const std::filesystem::path& odir, const std::string_view& filename) {{", indent);
            print(os, "{}    {}.open(odir, filename, \"{}\");", indent, walker.writerName, walker.ext);
            print(os, "{}}}", indent);
        }else{
            print(os, "{}inline void open(const std::filesystem::path& odir, const std::string_view& filename) {{", indent);
            print(os, "{}    unused(odir, filename);", indent);
            print(os, "{}}}", indent);
        }
    }

    /// @brief generates all Walker's
    inline void generateWalkers(
        std::ostream& os,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        auto xindent = std::string(indent) + "    ";

        // generate Walkers
        std::string first;
        for (auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            auto wname = std::format("Walker_{}", walker.name);
            print(os, "\n{}//walker:{}", indent, walker.name);
            if(grammar.isRootWalker(walker) == true) {
                first = wname;
                print(os, "{}struct {} : public NonCopyable {{", indent, wname);
                print(os, "{}{}& mod;", xindent, grammar.className);
                print(os, "{}explicit inline {}({}& m) : mod(m) {{}}", xindent, wname, grammar.className);
                if(grammar.walkers.size() > 1) {
                    print(os, "{}virtual ~{}() {{}}", xindent, wname);
                }

                generateWriter(os, walker, xindent);
                expand(os, walker.xmembers, xindent, true, vars);
                generateRuleSetVisitors(os, walker, vars, xindent);
                print(os, "{}}}; //walker:{}", indent, walker.name);
            }else{
                assert(walker.base != nullptr);
                print(os, "{}struct {} : public {} {{", indent, wname, first);
                print(os, "{}explicit inline {}({}& m) : {}(m) {{}}", xindent, wname, grammar.className, first);
                generateWriter(os, walker, xindent);
                expand(os, walker.xmembers, xindent, true, vars);
                generateRuleSetVisitors(os, walker, vars, xindent);
                print(os, "{}}}; //walker:{}", indent, walker.name);
            }
        }
    }

    /// @brief generates code to add default Walker, if none specified
    inline void generateInitWalkers(std::ostream& os, const std::string_view& indent) {
        for (auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            if(grammar.isBaseWalker(walker) == true) {
                continue;
            }
            print(os, "{}walkers.push_back(\"{}\");", indent, walker.name);
            break;
        }
    }

    /// @brief generates code to invoke specified Walker
    inline void generateWalkerCalls(std::ostream& os, const std::string_view& indent) {
        if(grammar.walkers.size() == 0) {
            return;
        }

        for (auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            if(grammar.isBaseWalker(walker) == true) {
                continue;
            }
            auto wname = std::format("Walker_{}", walker.name);
            print(os, "{}else if(w == \"{}\") {{", indent, walker.name);
            print(os, "{}    {}::{} walker(module);", indent, grammar.astClass, wname);
            print(os, "{}    walker.open(odir, filename);", indent);
            print(os, "{}    {}::WalkerNodeRef<{}::{}> s(start);", indent, grammar.astClass, grammar.astClass, grammar.start);
            print(os, "{}    walker.go(s);", indent);
            print(os, "{}}}", indent);
        }
        print(os, "{}else {{", indent);
        print(os, "{}    throw std::runtime_error(\"unknown walker: \" + w);", indent);
        print(os, "{}}}", indent);
    }

    /// @brief generates all Token IDs inside an enum in the prototype file
    inline void generateTokenIDs(std::ostream& os, const std::unordered_set<std::string>& tnames) {
        for (auto& t : tnames) {
            print(os, "        {},", t);
        }
    }

    /// @brief generates the string names for all Token IDs inside a map in the prototype file
    inline void generateTokenIDNames(std::ostream& os, const std::unordered_set<std::string>& tnames) {
        for (auto& t : tnames) {
            print(os, "        {{ID::{}, \"{}\"}},", t, t);
        }
    }

    /// @brief generates functions to create each AST node
    /// These functions are called by the Parser on REDUCE actions
    inline void generateCreateASTNodes(std::ostream& os, const std::unordered_map<std::string, std::string>& vars) {
        for (auto& rs : grammar.ruleSets) {
            print(os, "    template<>");
            print(os, "    inline {}::{}& create<{}::{}>(const ValueItem& vi) {{", grammar.astClass, rs->name, grammar.astClass, rs->name);
            print(os, "        switch(vi.ruleID) {{");
            if(hasEpsilon(*rs) == true) {
                print(os, "        case 0: {{");
                print(os, "            // EPSILON-C");
                print(os, "            auto& _cv_anchor = vi;");
                print(os, "            auto& anchor = create<{}::{}>(_cv_anchor);", grammar.astClass, grammar.tokenClass);
                print(os, "            auto& cel = ast.createAstNode<{}::{}>(vi.token.pos, anchor);", grammar.astClass, rs->name);
                print(os, "            cel.rule.emplace<{}::{}::{}_r0>();", grammar.astClass, rs->name, rs->name);
                print(os, "            return cel;");
                print(os, "        }} // case");
            }
            for (auto& r : rs->rules) {
                print(os, "        case {}: {{", r->id);
                print(os, "            //{}", r->str(false));
                print(os, "            assert(vi.childs.size() == {});", r->nodes.size());

                // generate parameter variables
                std::ostringstream ss;
                std::string sep;
                std::string hasAnchor;
                for (size_t idx = 0; idx < r->nodes.size(); ++idx) {
                    auto& n = r->nodes.at(idx);
                    auto varName = n->varName;
                    if (n->varName.size() == 0) {
                        varName = std::format("{}{}", n->name, idx);
                    }

                    auto rt = getNodeType(*n);
                    print(os, "            auto& _cv_{} = *(vi.childs.at({}));", varName, idx);
                    print(os, "            auto& p{} = create<{}::{}>(_cv_{});", varName, grammar.astClass, rt, varName);
                    if (idx == r->anchor) {
                        print(os, "            auto& anchor = create<{}::{}>(_cv_{});", grammar.astClass, grammar.tokenClass, varName);
                        hasAnchor = ", anchor";
                    }
                    ss << std::format("{}p{}", sep, varName);
                    sep = ", ";
                }

                print(os, "            auto& cel = ast.createAstNode<{}::{}>(vi.token.pos{});", grammar.astClass, rs->name, hasAnchor);
                print(os, "            cel.rule.emplace<{}::{}::{}>({});", grammar.astClass, rs->name, r->ruleName, ss.str());
                print(os, "            return cel;");
                print(os, "        }} // case");
            }
            print(os, "        }} // switch");
            print(os, "{}", generateError("vi.token.pos.row", "vi.token.pos.col", "vi.token.pos.file", "std::format(\"ASTGEN_ERROR:{}\", vi.ruleID)", "        ", vars));
            print(os, "    }}\n");
        }
    }

    /// @brief generates case statements for the Parser
    /// Each case block checks the next Token received from the Lexer
    /// and decides whether to SHIFT, REDUCE or GOTO to the next state
    inline void generateParserTransitions(std::ostream& os, const std::unordered_map<std::string, std::string>& vars) {
        for (auto& ps : grammar.itemSets) {
            auto& itemSet = *ps;
            assert((itemSet.shifts.size() > 0) || (itemSet.reduces.size() > 0) || (itemSet.gotos.size() > 0));
            bool breaked = false;

            print(os, "            case {}:", itemSet.id);
            print(os, "                std::print(log(), \"{{}}\", \"{}\\n\");", itemSet.str("", "\\n", true));
            print(os, "                switch(k.id) {{");
            for (auto& c : itemSet.shifts) {
                for(auto& fb : c.first->fallbacks) {
                    if ((itemSet.hasShift(*fb)) || (itemSet.hasReduce(*fb))) {
                        continue;
                    }
                    print(os, "                case Tolkien::ID::{}: // SHIFT(fallback)", fb->name);
                }

                print(os, "                case Tolkien::ID::{}: // SHIFT", c.first->name);
                for(auto& e : c.second.epsilons) {
                    print(os, "                    shift(k.pos, Tolkien::ID::{}); //EPSILON-S", e->name);
                    print(os, "                    stateStack.push_back(0);");
                    print(os, "                    reduce(0, 1, 0, Tolkien::ID::{}, \"{}\");", e->name, e->name);
                    print(os, "                    stateStack.push_back(0);");
                }
                print(os, "                    std::print(log(), \"SHIFT {}: t={}\\n\");", c.second.next->id, c.first->name);
                print(os, "                    shift(k);");
                print(os, "                    stateStack.push_back({});", c.second.next->id);
                print(os, "                    return accepted;");
            }
            for (auto& rd : itemSet.reduces) {
                auto& c = *(rd.second.next);
                auto& r = c.rule;
                print(os, "                case Tolkien::ID::{}: // REDUCE", rd.first->name);
                print(os, "                    std::print(log(), \"REDUCE:{}:{{}}/{}\\n\", \"{}\");", rd.first->name, r.nodes.size(), c.str());
                auto len = rd.second.len;
                while(len < r.nodes.size()) {
                    print(os, "                    //shift-epsilon: len={}", len);
                    print(os, "                    shift(k.pos, Tolkien::ID::{}); //EPSILON-R", grammar.empty);
                    print(os, "                    stateStack.push_back(0);");
                    ++len;
                }

                if ((r.ruleSetName() == grammar.start) && (rd.first->name == grammar.end)) {
                    print(os, "                    shift(k.pos, Tolkien::ID::{}); //END", grammar.end);
                    print(os, "                    stateStack.push_back(0);");
                }
                print(os, "                    reduce({}, {}, {}, Tolkien::ID::{}, \"{}\");", r.id, len, r.anchor, r.ruleSetName(), r.ruleSetName());
                print(os, "                    k.id = Tolkien::ID::{};", r.ruleSetName());
                if (r.ruleSetName() == grammar.start) {
                    print(os, "                    accepted = true;");
                    print(os, "                    return accepted;");
                }else{
                    print(os, "                    break;");
                    breaked = true;
                }
            }
            for (auto& c : itemSet.gotos) {
                print(os, "                case Tolkien::ID::{}: // GOTO", c.first->name);
                print(os, "                    std::print(log(), \"GOTO {}:id={}, rule={}\\n\");", c.second->id, c.first->id, c.first->name);
                print(os, "                    stateStack.push_back({});", c.second->id);
                print(os, "                    k = k0;");
                print(os, "                    break;");
                breaked = true;
            }
            print(os, "                default:");
            print(os, "{}", generateError("k.pos.row", "k.pos.col", "k.pos.file", "\"SYNTAX_ERROR\"", "                    ", vars));
            print(os, "                }} // switch(k.id)");
            if(breaked == true) {
                print(os, "                break;");
            }
        }
    }

    /// @brief class to calculate transitions from one Lexer state to the next
    struct TransitionSet {
        const yg::Grammar::Transition* wildcard = nullptr;

        std::vector<std::pair<const yg::Grammar::Transition*, const yg::Grammar::RangeClass*>> smallRanges;
        std::vector<std::pair<const yg::Grammar::Transition*, const yg::Grammar::RangeClass*>> largeRanges;
        std::vector<std::pair<const yg::Grammar::Transition*, const yg::Grammar::LargeEscClass*>> largeEscClasses;

        std::vector<std::pair<const yg::Grammar::Transition*, const yg::Grammar::ClassTransition*>> classes;

        std::pair<const yg::Grammar::Transition*, const yg::Grammar::ClosureTransition*> enterClosure = {nullptr, nullptr};
        std::pair<const yg::Grammar::Transition*, const yg::Grammar::ClosureTransition*> preLoop = {nullptr, nullptr};
        std::pair<const yg::Grammar::Transition*, const yg::Grammar::ClosureTransition*> inLoop = {nullptr, nullptr};
        std::pair<const yg::Grammar::Transition*, const yg::Grammar::ClosureTransition*> postLoop = {nullptr, nullptr};
        std::pair<const yg::Grammar::Transition*, const yg::Grammar::ClosureTransition*> leaveClosure = {nullptr, nullptr};

        std::pair<const yg::Grammar::Transition*, const yg::Grammar::SlideTransition*> slide = {nullptr, nullptr};

        struct Visitor {
            const yg::Grammar& grammar;
            TransitionSet& tset;
            const yg::Grammar::Transition& tx;

            inline Visitor(const yg::Grammar& g, TransitionSet& s, const yg::Grammar::Transition& x)
                : grammar(g), tset(s), tx(x) {}

            inline void operator()(const yg::Grammar::WildCard&) {
                assert(tset.wildcard == nullptr);
                tset.wildcard = &tx;
            }

            inline void operator()(const yg::Grammar::LargeEscClass& t) {
                tset.largeEscClasses.push_back(std::make_pair(&tx, &t));
            }

            inline void operator()(const yg::Grammar::RangeClass& t) {
                bool isSmallRange = ((t.ch2 - t.ch1) <= grammar.smallRangeSize);
                if (isSmallRange) {
                    tset.smallRanges.push_back(std::make_pair(&tx, &t));
                }else{
                    tset.largeRanges.push_back(std::make_pair(&tx, &t));
                }
            }

            inline void operator()(const yg::Grammar::PrimitiveTransition& t) {
                std::visit(*this, t.atom.atom);
            }

            inline void operator()(const yg::Grammar::ClassTransition& t) {
                tset.classes.push_back(std::make_pair(&tx, &t));
            }

            inline void operator()(const yg::Grammar::ClosureTransition& t) {
                switch(t.type) {
                case yg::Grammar::ClosureTransition::Type::Enter:
                    assert(tset.enterClosure.first == nullptr);
                    tset.enterClosure = std::make_pair(&tx, &t);
                    break;
                case yg::Grammar::ClosureTransition::Type::PreLoop:
                    assert(tset.preLoop.first == nullptr);
                    tset.preLoop = std::make_pair(&tx, &t);
                    break;
                case yg::Grammar::ClosureTransition::Type::InLoop:
                    assert(tset.inLoop.first == nullptr);
                    tset.inLoop = std::make_pair(&tx, &t);
                    break;
                case yg::Grammar::ClosureTransition::Type::PostLoop:
                    assert(tset.postLoop.first == nullptr);
                    tset.postLoop = std::make_pair(&tx, &t);
                    break;
                case yg::Grammar::ClosureTransition::Type::Leave:
                    assert(tset.leaveClosure.first == nullptr);
                    tset.leaveClosure = std::make_pair(&tx, &t);
                    break;
                }
            }

            inline void operator()(const yg::Grammar::SlideTransition& t) {
                assert(tset.slide.first == nullptr);
                tset.slide = std::make_pair(&tx, &t);
            }
        };

        inline void process(const yg::Grammar& g, const std::vector<yg::Grammar::Transition*>& txs) {
            for (auto& tx : txs) {
                Visitor v(g, *this, *tx);
                std::visit(v, tx->t);
            }
        }
    };

    /// @brief generate code to transition from one Lexer state to another
    inline void generateStateChange(std::ostream& os, const yg::Grammar::Transition& t, const yg::Grammar::State* nextState, const std::string& indent) {
        if (t.capture) {
            print(os, "                {}token.addText(ch);", indent);
        }
        print(os, "                {}stream.consume();", indent);
        print(os, "                {}state = {};", indent, nextState->id);
    }

    /// @brief generate Lexer states
    inline void generateLexerStates(std::ostream& os, const std::unordered_map<std::string, std::string>& vars) {
        print(os, "            case 0:");
        print(os, "{}", generateError("stream.pos.row", "stream.pos.col", "stream.pos.file", "\"SYNTAX_ERROR\"", "                ", vars));

        for (auto& ps : grammar.states) {
            auto& state = *ps;

            TransitionSet tset;
            tset.process(grammar, state.transitions);
            tset.process(grammar, state.superTransitions);
            tset.process(grammar, state.shadowTransitions);

            print(os, "            case {}:", state.id);
            if(state.isRoot == true) {
                print(os, "                token = Tolkien(stream.pos);");
            }

            if (tset.inLoop.first != nullptr) {
                assert(tset.smallRanges.size() == 0);
                assert(tset.largeRanges.size() == 0);
                assert(tset.largeEscClasses.size() == 0);
                assert(tset.wildcard == nullptr);
                assert(tset.slide.first == nullptr);
                assert(tset.enterClosure.first == nullptr);
                assert(tset.leaveClosure.first == nullptr);

                auto& tx = *(tset.inLoop.second);
                print(os, "                assert(counts.size() > 0);");
                if (tset.preLoop.first != nullptr) {
                    print(os, "                if(count() < {}) {{", tx.atom.min);
                    print(os, "                    ++counts.back();");
                    print(os, "                    state = {};", tset.preLoop.first->next->id);
                    print(os, "                    continue; //precount");
                    print(os, "                }}");
                }

                std::string chkx;
                auto mrc = (tx.atom.max == grammar.maxRepCount ? "MaxRepeatCount" : std::to_string(tx.atom.max));
                if(tx.atom.min > 0) {
                    chkx = std::format("(count() >= {}) && (count() < {})", tx.atom.min, mrc);
                }else{
                    chkx = std::format("count() < {}", mrc);
                }

                print(os, "                if({}) {{", chkx);
                print(os, "                    ++counts.back();");
                print(os, "                    state = {};", tset.inLoop.first->next->id);
                print(os, "                    continue; //inLoop");
                print(os, "                }}");

                assert(tset.postLoop.first != nullptr);
                print(os, "                assert(count() == {});", mrc);
                print(os, "                counts.pop_back();");
                print(os, "                state = {};", tset.postLoop.first->next->id);
                print(os, "                continue; //postLoop");

                continue;
            }

            // END check must always be the second one
            if (state.checkEOF) {
                print(os, "                if(ch == static_cast<char_t>(EOF)) {{");
                print(os, "                    token.id = Tolkien::ID::{};", grammar.end);
                print(os, "                    parser.parse(token);\n");
                print(os, "                    // at EOF, call parse() repeatedly until all final reductions are complete");
                print(os, "                    while(parser.isClean() == false) {{");
                print(os, "                        parser.parse(token);");
                print(os, "                    }}");
                print(os, "                    state = 0;");
                print(os, "                    stream.consume();");
                print(os, "                    continue; //EOF");
                print(os, "                }}");
            }

            // generate switch cases for small ranges
            if (tset.smallRanges.size() > 0) {
                print(os, "                switch(ch) {{");

                for (auto& t : tset.smallRanges) {
                    assert(t.second->ch2 >= t.second->ch1);
                    for (auto c = t.second->ch1; c <= t.second->ch2; ++c) {
                        print(os, "                case {}:", getChString(c));
                    }
                    generateStateChange(os, *(t.first), t.first->next, "    ");
                    print(os, "                    continue; //smallRange");
                }

                print(os, "                }}");
            }

            // generate checks for large escape classes
            for (auto& t : tset.largeEscClasses) {
                print(os, "                if({}(ch)) {{", t.second->checker);
                generateStateChange(os, *(t.first), t.first->next, "    ");
                print(os, "                    continue; //largeEsc");
                print(os, "                }}");
            }

            // generate checks for large ranges
            for (auto& t : tset.largeRanges) {
                print(os, "                // id={}, large", state.id);
                print(os, "                if(contains(ch, {}, {})) {{", getChString(t.second->ch1), getChString(t.second->ch2));
                generateStateChange(os, *(t.first), t.first->next, "    ");
                print(os, "                    continue; //largeRange");
                print(os, "                }}");
            }

            // generate checks for classes
            for (auto& t : tset.classes) {
                std::stringstream ss;
                std::string sep;
                std::string sepx = (t.second->atom.negate?" && ":" || ");
                std::string negate = (t.second->atom.negate?"!":"");
                for(auto& ax : t.second->atom.atoms) {
                    std::visit(overloaded{
                        [&negate, &ss, &sep](const yg::Grammar::WildCard&) {
                            ss << std::format("{}({}true)", sep, negate);
                        },
                        [&negate, &ss, &sep](const yg::Grammar::LargeEscClass& a) {
                            ss << std::format("{}({}{}(ch))", sep, negate, a.checker);
                        },
                        [&negate, &ss, &sep](const yg::Grammar::RangeClass& a) {
                            ss << std::format("{}({}contains(ch, {}, {}))", sep, negate, getChString(a.ch1), getChString(a.ch2));
                        },
                    }, ax);
                    sep = sepx;
                }
                print(os, "                if((ch != static_cast<char_t>(EOF)) && ({})) {{", ss.str());
                generateStateChange(os, *(t.first), t.first->next, "    ");
                print(os, "                    continue; //Class");
                print(os, "                }}");
            }

            // the sequence of checks in this if-ladder is significant
            if (tset.wildcard != nullptr) {
                generateStateChange(os, *(tset.wildcard), tset.wildcard->next, "");
                print(os, "                continue; //wildcard");
            }else if (tset.slide.first != nullptr) {
                assert(tset.wildcard == nullptr);
                print(os, "                state = {};", tset.slide.first->next->id);
                print(os, "                continue; //slide");
            }else if (tset.enterClosure.first != nullptr) {
                print(os, "                counts.push_back({});", tset.enterClosure.second->initialCount);
                print(os, "                state = {};", tset.enterClosure.first->next->id);
                print(os, "                continue; //enterClosure");
            }else if (state.matchedRegex) {
                size_t nextStateID = 0;
                switch(state.matchedRegex->modeChange) {
                case yg::Grammar::Regex::ModeChange::None:
                    break;
                case yg::Grammar::Regex::ModeChange::Next: {
                    auto& mode = grammar.getRegexNextMode(*(state.matchedRegex));
                    assert(mode.root);
                    nextStateID = mode.root->id;
                    print(os, "                modes.push_back({}); // MATCH, -> {}", nextStateID, state.matchedRegex->nextMode);
                    break;
                }
                case yg::Grammar::Regex::ModeChange::Back:
                    print(os, "                assert(modes.size() > 0);");
                    print(os, "                modes.pop_back();");
                    break;
                case yg::Grammar::Regex::ModeChange::Init:
                    print(os, "                assert(modes.size() > 0);");
                    print(os, "                modes.clear();");
                    print(os, "                modes.push_back(1);");
                    break;
                }
                print(os, "                state = modeRoot();");

                if (state.matchedRegex->usageCount > 0) {
                    print(os, "                token.id = Tolkien::ID::{};", state.matchedRegex->regexName);
                    print(os, "                parser.parse(token);");
                }else{
                    print(os, "                token = Tolkien(stream.pos);");
                }
                print(os, "                continue;");
            }else if (tset.leaveClosure.first != nullptr) {
                assert(tset.wildcard == nullptr);
                assert(tset.slide.first == nullptr);
                print(os, "                state = {};", tset.leaveClosure.first->next->id);
                print(os, "                continue; //leaveClosure");
            }else{
                print(os, "{}", generateError("stream.pos.row", "stream.pos.col", "stream.pos.file", "\"SYNTAX_ERROR\"", "                ", vars));
            }
        }
    }

    /// @brief main parser generator
    /// This function reads the prototype.cpp file (embedded as a raw string)
    /// and acts on each meta command found in the file.
    inline void includeCodeBlock(
        const char* codeBlock,
        std::ofstream& srcx,
        const std::unordered_map<std::string, std::string>& vars,
        const std::unordered_set<std::string>& tnames,
        const std::filesystem::path& filebase,
        const std::string& srcName
    ) {
        const std::string_view prefixEnterBlock = "///PROTOTYPE_ENTER:";
        const std::string_view prefixLeaveBlock = "///PROTOTYPE_LEAVE:";
        const std::string_view prefixSegment = "///PROTOTYPE_SEGMENT:";
        const std::string_view includeSegment = "///PROTOTYPE_INCLUDE:";
        const std::string_view prefixTarget = "///PROTOTYPE_TARGET:";

        enum class Token {
            EnterBlock,
            LeaveBlock,
            Line,
            Segment,
            Include,
            Target,
        };

        enum class TokenState {
            InitWS,
            Slash1,
            Slash2,
            Line,
        };

        std::string tblock;

        const std::string_view cb(codeBlock);
        auto it = cb.begin();
        auto ite = cb.end();

        bool skip = false;
        std::vector<std::string_view> eblockNames;

        while (it != ite) {
            auto itb = it;
            auto itl = it;
            bool initWS = true;
            while ((it != ite) && (*it != '\n')) {
                if (initWS) {
                    if (std::isspace(*it) == 0) {
                        initWS = false;
                        itl = it;
                    }
                }

                #if defined(__clang__)
                #pragma clang unsafe_buffer_usage begin
                #endif
                ++it;
                #if defined(__clang__)
                #pragma clang unsafe_buffer_usage end
                #endif
            }

            std::string_view line(itb, it);
            std::string_view indent(itb, itl);
            std::string_view tline(itl, it);

            Token token = Token::Line;
            std::string_view eblockName;
            std::string_view lblockName;
            std::string_view segmentName;
            std::string_view includeName;
            std::string_view targetName;

            if (tline.starts_with(prefixEnterBlock)) {
                token = Token::EnterBlock;
                eblockName = tline.substr(prefixEnterBlock.size());
                eblockNames.push_back(eblockName);
            }else if(tline.starts_with(prefixLeaveBlock)) {
                token = Token::LeaveBlock;
                lblockName = tline.substr(prefixLeaveBlock.size());
                if(eblockNames.size() == 0) {
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "PROTOTYPE_NO_EBLOCK_ERROR:{}", lblockName);
                }
                eblockName = eblockNames.back();
                eblockNames.pop_back();
            }else if(tline.starts_with(prefixSegment)) {
                token = Token::Segment;
                segmentName = tline.substr(prefixSegment.size());
            }else if(tline.starts_with(includeSegment)) {
                token = Token::Include;
                includeName = tline.substr(includeSegment.size());
            }else if(tline.starts_with(prefixTarget)) {
                token = Token::Target;
                targetName = tline.substr(prefixTarget.size());
            }else{
                if(eblockNames.size() > 0) {
                    eblockName = eblockNames.back();
                }
                token = Token::Line;
            }

//#define PRINT(...) print(__VA_ARGS__)
#define PRINT(...)

            switch (token) {
            case Token::EnterBlock: {
                PRINT("Init::EB:{}", line);
                if (eblockName != "SKIP") {
                    print(srcx, "{}", line);
                }
                if (eblockName == "stdHeaders") {
                    if (grammar.stdHeadersEnabled == true) {
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "repl") {
                    if (amalgamatedFile == true) {
                        print(srcx, "#define HAS_REPL {}", (grammar.hasREPL == true)?1:0);
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "fmain") {
                    if (amalgamatedFile == true) {
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "fmain_repl") {
                    break;
                }

                if (eblockName == "SKIP") {
                    skip = true;
                    break;
                }

                if (eblockName == "throwError") {
                    tblock.clear();
                    skip = true;
                    break;
                }

                throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "PROTOTYPE_EBLOCK_ERROR:{}", eblockName);
            }
            case Token::LeaveBlock: {
                PRINT("Line::LB:{}, eb={}, lb={}", line, eblockName, lblockName);
                if (eblockName != "SKIP") {
                    print(srcx, "{}", line);
                }
                assert(lblockName == eblockName);
                skip = false;
                if (eblockName == "throwError") {
                    if (throwError.code.empty()) {
                        throwError.setCode(tblock);
                    }
                    break;
                }
                break;
            }
            case Token::Segment: {
                PRINT("Line::SEGMENT:{}", line);
                print(srcx, "{}:BEGIN", line);
                if (segmentName == "pchHeader") {
                    generatePchHeader(srcx, indent);
                }else if (segmentName == "hdrHeaders") {
                    generateHdrHeaders(srcx, indent);
                }else if (segmentName == "srcHeaders") {
                    generateSrcHeaders(srcx, indent);
                }else if (segmentName == "classMembers") {
                    generateClassMembers(srcx, indent);
                }else if (segmentName == "astNodeDecls") {
                    generateAstNodeDecls(srcx, indent);
                }else if (segmentName == "astNodeDefns") {
                    generateAstNodeDefns(srcx, indent);
                }else if (segmentName == "astNodeItems") {
                    generateAstNodeItems(srcx, indent);
                }else if (segmentName == "walkers") {
                    generateWalkers(srcx, vars, indent);
                }else if (segmentName == "prologue") {
                    expand(srcx, grammar.prologue, indent, true, vars);
                }else if (segmentName == "initWalkers") {
                    if(amalgamatedFile) {
                        generateInitWalkers(srcx, indent);
                    }
                }else if (segmentName == "walkerCalls") {
                    generateWalkerCalls(srcx, indent);
                }else if (segmentName == "epilogue") {
                    expand(srcx, grammar.epilogue, indent, true, vars);
                }else if (segmentName == "tokenIDs") {
                    generateTokenIDs(srcx, tnames);
                }else if (segmentName == "tokenIDNames") {
                    generateTokenIDNames(srcx, tnames);
                }else if (segmentName == "createASTNodes") {
                    generateCreateASTNodes(srcx, vars);
                }else if (segmentName == "parserTransitions") {
                    generateParserTransitions(srcx, vars);
                }else if (segmentName == "lexerStates") {
                    generateLexerStates(srcx, vars);
                }else{
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_SEGMENT:{}", segmentName);
                }
                print(srcx, "{}:END", line);
                break;
            }
            case Token::Include: {
                PRINT("Line::INCLUDE:{}", line);
                print(srcx, "{}:BEGIN", line);
                if (includeName == "utf8Encoding") {
                    if (grammar.unicodeEnabled == true) {
                        includeCodeBlock(utf8Encoding, srcx, vars, tnames, filebase, srcName);
                    }
                }else if (includeName == "asciiEncoding") {
                    if (grammar.unicodeEnabled == false) {
                        includeCodeBlock(asciiEncoding, srcx, vars, tnames, filebase, srcName);
                    }
                }else if (includeName == "stream") {
                    includeCodeBlock(cbStream, srcx, vars, tnames, filebase, srcName);
                }else{
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_INCLUDE:{}", includeName);
                }
                print(srcx, "{}:END", line);
                break;
            }
            case Token::Target: {
                PRINT("Line::TARGET:{}", line);
                if ((!amalgamatedFile) && (targetName == "SOURCE")) {
                    auto hdrName = srcName;
                    auto nsrcName = filebase.string() + ".cpp";
                    PRINT("reopening:{}", nsrcName);
                    srcx = std::ofstream(nsrcName);
                    if (srcx.is_open() == false) {
                        throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_SRC:{}", nsrcName);
                    }
                    print(srcx, "#include \"{}\"", std::filesystem::path(hdrName).filename().string());
                }
                break;
            }
            case Token::Line: {
                PRINT("Init::LINE:{}, eblockName={}", line, eblockName);
                if (skip == false) {
                    if(line.size() == 0) {
                        // print empty line as-is
                        print(srcx, "{}", line);
                    }else{
                        expand(srcx, line, "", false, vars);
                    }
                }else if(eblockName == "throwError") {
                    tblock += line;
                }
                break;
            }
            }

            #if defined(__clang__)
            #pragma clang unsafe_buffer_usage begin
            #endif
            ++it;
            #if defined(__clang__)
            #pragma clang unsafe_buffer_usage end
            #endif
        }
    }

    /// @brief main driver function for the parser generator
    inline void generate(const std::filesystem::path& filebase) {
        if(grammar.hasDefaultWalker() == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "INVALID_WALKERCLASS");
        }

        if (grammar.className.empty()) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "INVALID_CLASSNAME");
        }

        throwError = grammar.throwError;

        auto srcName = filebase.string() + ".hpp";
        if(amalgamatedFile) {
            srcName = filebase.string() + ".cpp";
        }
        auto srcx = std::ofstream(srcName);
        if (srcx.is_open() == false) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_SRC:{}", srcName);
        }
        if(amalgamatedFile == false) {
            print(srcx, "#pragma once");
        }

        // token IDs
        std::unordered_set<std::string> tnames;
        for (auto& t : grammar.regexes) {
            if (tnames.contains(t->regexName)) {
                continue;
            }
            tnames.insert(t->regexName);
        }
        for (auto& t : grammar.ruleSets) {
            if (tnames.contains(t->name)) {
                assert(false);
                continue;
            }
            tnames.insert(t->name);
        }

        if (grammar.ns.empty() == false) {
            qid = grammar.ns;
        }

        if (grammar.className.empty() == false) {
            if (qid.size() > 0) {
                qid += "::";
            }
            qid += grammar.className;
        }

        const std::unordered_map<std::string, std::string> vars = {
            {"CLSNAME", grammar.className},
            {"CLASSQID", qid},
            {"TOKEN", grammar.tokenClass},
            {"WALKER", grammar.getDefaultWalker().name},
            {"START_RULE", std::format("{}", grammar.start)},
            {"START_RULE_NAME", std::format("\"{}\"", grammar.start)},
            {"MAX_REPEAT_COUNT", std::to_string(grammar.maxRepCount)},
            {"AST", grammar.astClass},
        };

        includeCodeBlock(cbPrototype, srcx, vars, tnames, filebase, srcName);
    }
};
}

void generateGrammar(const yg::Grammar& g, const std::filesystem::path& of) {
    Generator gen(g);
    gen.generate(of);
}
