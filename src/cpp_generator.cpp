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
#include "text_writer.hpp"
#include "logger.hpp"

/// @brief This is the UNICODE encoding file, embedded as a raw C string
extern const char* cb_encoding_utf8;

/// @brief This is the ASCII encoding file, embedded as a raw C string
extern const char* cb_encoding_ascii;

/// @brief This is the prototype.cpp file, embedded as a raw C string
extern const char* cb_prototype;

/// @brief This is the stream.hpp file, embedded as a raw C string
extern const char* cb_stream;

/// @brief This is the text_writer.hpp file, embedded as a raw C string
extern const char* cb_text_writer;

/// @brief This is the print.hpp file, embedded as a raw C string
extern const char* cb_print;

/// @brief This is the nsutil.hpp file, embedded as a raw C string
extern const char* cb_nsutil;

/// @brief This is the filepos.hpp file, embedded as a raw C string
extern const char* cb_filepos;

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
    inline void
    _expand(
        StringStreamWriter& sw,
        const std::string_view& codeblock,
        const std::string_view& indent,
        const bool& autoIndent,
        const std::unordered_map<std::string, std::string>& vars
    ) {
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
                sw.write("{}", indent);
                if (ch == 'T') {
                    state = State::Tag0;
                    key = "";
                    continue;
                }
                sw.write(ch);
                space1 = 0;
                state = State::Code;
                break;
            case State::InitSpace1:
                if(ch == '\r') {
                    continue;
                }
                if(ch == '\n') {
                    sw.write(ch);
                    ++sw.row;
                    continue;
                }
                if(autoIndent && (std::isspace(ch)) && (ch != '\n')) {
                    if(space1 < space0) {
                        ++space1;
                        continue;
                    }
                }
                sw.write("{}", indent);
                if (ch == 'T') {
                    state = State::Tag0;
                    key = "";
                    continue;
                }
                sw.write(ch);
                state = State::Code;
                break;
            case State::Code:
                if(ch == '\r') {
                    continue;
                }
                if(ch == '\n') {
                    sw.write(ch);
                    ++sw.row;
                    space1 = 0;
                    state = State::InitSpace1;
                    continue;
                }
                if(ch == 'T') {
                    state = State::Tag0;
                    key = "";
                    continue;
                }
                sw.write(ch);
                state = State::Code;
                break;
            case State::Tag0:
                if (ch == 'A') {
                    state = State::Tag1;
                    continue;
                }
                sw.write("T{}", ch);
                state = State::Code;
                break;
            case State::Tag1:
                if (ch == 'G') {
                    state = State::Tag2;
                    continue;
                }
                sw.write("TA{}", ch);
                state = State::Code;
                break;
            case State::Tag2:
                if (ch == '(') {
                    state = State::Tag3;
                    continue;
                }
                sw.write("TAG{}", ch);
                state = State::Code;
                break;
            case State::Tag3:
                if (ch == ')') {
                    if (auto it = vars.find(key); it != vars.end()) {
                        sw.write("{}", it->second);
                    }else{
                        sw.write("TAG({})", key);
                    }
                    state = State::Code;
                }
                else if ((isalpha(ch)) || (ch == '_')) {
                    key += ch;
                }else{
                    sw.write("TAG({}", key);
                    state = State::Code;
                }
                break;
            }
        }
    }

    /// @brief optionally appends a #line to an expanded codeblock
    inline void
    generateCodeBlock(
        TextFileWriter& tw,
        const CodeBlock& codeblock,
        const std::string_view& indent,
        const bool& autoIndent,
        const std::unordered_map<std::string, std::string>& vars
    ) {
        StringStreamWriter sw;
        _expand(sw, codeblock.code, indent, autoIndent, vars);
        if (codeblock.hasPos == false) {
            tw.swrite(sw);
            return;
        }

        std::string pline = (genLines == false) ? "//" : "";

        tw.writeln();
        tw.writeln("{}{}#line {} \"{}\" //t={},s={}", indent, pline, codeblock.pos.row, codeblock.pos.file, tw.row, sw.row);
        tw.swrite(sw);
        tw.writeln("{}{}#line {} \"{}\" //t={},s={}", indent, pline, tw.row, tw.file.string(), tw.row, sw.row);
    }

    /// @brief writes expanded codeblock to output stream if the codeblock is not empty
    inline void
    generatePrototypeLine(
        TextFileWriter& tw,
        const std::string_view& line,
        const std::unordered_map<std::string, std::string>& vars
    ) {
        if(line.empty() == true) {
            // print empty line as-is
            tw.writeln();
        }else{
            StringStreamWriter sw;
            _expand(sw, line, "", false, vars);
            tw.swriteln(sw);
        }
    }

    /// @brief writes expanded codeblock to throw an exception
    /// This is called at various points in the generation code
    inline void
    generateError(
        TextFileWriter& tw,
        const std::string& line,
        const std::string& col,
        const std::string& file,
        const std::string& msg,
        const std::string_view& indent,
        const std::unordered_map<std::string, std::string>& vars
    ) {
        auto xvars = vars;
        xvars["ROW"] = line;
        xvars["COL"] = col;
        xvars["SRC"] = file;
        xvars["MSG"] = msg;
        StringStreamWriter sw;
        _expand(sw, throwError.code, indent, true, xvars);
        // tw.writeln("{}", s);
        tw.swrite(sw);
    }

    /// @brief Checks if the \ref yg::Grammar::RuleSet contains a empty prooduction
    inline bool hasEpsilon(const ygp::RuleSet& rs) const {
        return (rs.firstIncludes(grammar.empty) == true);
    }

    /// @brief Construct a full function name given a rule and a short function name
    inline std::string getFunctionName(const ygp::Rule& r, const std::string& fname) const {
        return std::format("{}_{}", r.ruleName, fname);
    }

    /// @brief Get the type for any node in a production.
    /// If the node is a terminal, returns the default token class name
    /// If non-terminal, return the AST node name for the rule
    /// (which is the same as the rule name)
    inline std::string getNodeType(const ygp::Node& n) const {
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
    inline void
    getArgs(
        StringStreamWriter& sw,
        const ygp::Rule& r,
        const yg::Walker::FunctionSig& fsig,
        const yg::Walker::CodeInfo* ci,
        const std::string_view& indent
    ) {
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

            sw.writeln("{}    {}{}{}& {}", indent, sep, isUnused, nativeTypeNode, varName);
            sep = ",";
        }
        if(fsig.args.size() > 0) {
            sw.writeln("{}    {}{}", indent, sep, fsig.args);
        }
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

    /// @brief generates code to include PCH
    inline void generatePchHeader(TextFileWriter& tw, const std::string_view& indent) {
        tw.writeln("{}#include \"{}\"", indent, grammar.pchHeader);
    }

    /// @brief generates code to include header files in the parser's header file
    inline void generateHdrHeaders(TextFileWriter& tw, const std::string_view& indent) {
        for (auto& h : grammar.hdrHeaders) {
            tw.writeln("{}#include \"{}\"", indent, h);
        }
    }

    /// @brief generates code to include header files in the parser's source file
    inline void generateSrcHeaders(TextFileWriter& tw, const std::string_view& indent) {
        for (auto& h : grammar.srcHeaders) {
            tw.writeln("{}#include \"{}\"", indent, h);
        }
    }

    /// @brief generates additional class members for the parser, if any are specified in the .y file
    inline void generateClassMembers(TextFileWriter& tw, const std::string_view& indent) {
        for (auto& m : grammar.classMembers) {
            tw.writeln("{}{};", indent, m);
        }
    }

    /// @brief generates declarations for all AST nodes
    inline void generateAstNodeDecls(TextFileWriter& tw, const std::string_view& indent) {
        for (auto& rs : grammar.ruleSets) {
            tw.writeln("{}struct {};", indent, rs->name);
        }
    }

    /// @brief generates definitions for all AST nodes
    inline void generateAstNodeDefns(TextFileWriter& tw, const std::string_view& indent) {
        for (auto& rs : grammar.ruleSets) {
            tw.writeln("{}struct {} : public NonCopyable {{", indent, rs->name);
            tw.writeln("{}    const FilePos pos;", indent);

            if(hasEpsilon(*rs) == true) {
                tw.writeln("{}    struct {}_r0 {{", indent, rs->name);
                tw.writeln("{}    }};", indent);
                tw.writeln();
            }

            for (auto& r : rs->rules) {
                tw.writeln("{}    struct {} : public NonCopyable {{", indent, r->ruleName);
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
                    tw.writeln("{}        {}& {};", indent, rt, varName);
                    pos << std::format("{}{}& p{}", sep, rt, varName);
                    ios << std::format("{}{}(p{})", sep, varName, varName);
                    sep = ", ";
                    coln = " : ";
                }
                tw.writeln("{}        explicit inline {}({}){}{} {{}}", indent, r->ruleName, pos.str(), coln, ios.str());
                tw.writeln("{}    }};", indent);
                tw.writeln();
            }

            std::string sep;
            tw.writeln("{}    using Rule = std::variant<", indent);
            tw.writeln("{}        {}std::monostate", indent, sep);
            sep = ",";
            if(hasEpsilon(*rs) == true) {
                tw.writeln("{}        {}{}_r0", indent, sep, rs->name);
                sep = ",";
            }
            for (auto& r : rs->rules) {
                tw.writeln("{}        {}{}", indent, sep, r->ruleName);
                sep = ",";
            }
            tw.writeln("{}    >;", indent);
            tw.writeln();

            tw.writeln("{}    const {}& anchor;", indent, grammar.tokenClass);
            tw.writeln("{}    Rule rule;", indent);
            tw.writeln();

            auto anchorArg = std::format(", const {}& a", grammar.tokenClass);
            auto anchorCtor = ", anchor(a)";
            tw.writeln("{}    explicit inline {}(const FilePos& p{}) : pos(p){} {{}}", indent, rs->name, anchorArg, anchorCtor);
            tw.writeln("{}}}; // struct {}", indent, rs->name);
            tw.writeln();
        }
    }

    /// @brief generates a variant that contains all AST nodes
    inline void generateAstNodeItems(TextFileWriter& tw, const std::string_view& indent) {
        tw.writeln("{}using AstNode = std::variant<", indent);
        tw.writeln("{}    {}", indent, grammar.tokenClass);
        for (auto& rs : grammar.ruleSets) {
            tw.writeln("{}    ,{}", indent, rs->name);
        }
        tw.writeln("{}>;", indent);
    }

    /// @brief generates member function corresponding to the semantic action for each rule
    inline void generateRuleHandler(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const ygp::Rule& r,
        const yg::Walker::FunctionSig& fsig,
        const std::string& isVirtual,
        const std::string& isOverride,
        const yg::Walker::CodeInfo* ci,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        auto fname = getFunctionName(r, fsig.func);
        std::string returnType = fsig.type;

        // infer indent for function args
        tw.writeln("{}//RULE_HANDLER({}):{}", indent, walker.name, r.str(false));

        // get args list as indented string
        StringStreamWriter sw;
        getArgs(sw, r, fsig, ci, indent);

        // open function
        if(sw.wrote == true) {
            tw.writeln("{}{} {}", indent, isVirtual, returnType);
            tw.writeln("{}{}(", indent, fname);
            tw.swrite(sw);
            tw.writeln("{}){} {{", indent, isOverride);
        }else{
            tw.writeln("{}{} {} {}(){} {{", indent, isVirtual, returnType, fname, isOverride);
        }

        // generate function body, if any
        if(ci != nullptr) {
            generateCodeBlock(tw, ci->codeblock, std::string(indent) + "    ", true, vars);
        }

        // close function
        tw.writeln("{}}}", indent);
        tw.writeln();
    }

    /// @brief generates visitor overload to invoke the corresponding member function
    inline void generateRuleVisitorBody(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const yg::Walker::FunctionSig& fsig,
        const ygp::RuleSet& rs,
        const ygp::Rule& r,
        const std::string& called,
        const std::string& xparams,
        const std::string_view& indent
    ) {
        StringStreamWriter wnodes;
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
                        wnodes.writeln("{}            WalkerNodeRef<{}> {}(_n.{}, {});/*wvar1u*/", indent, n->name, n->varName, n->varName, called);
                        wcalls << std::format("{}            if({}.called == false) {}({});/*wvar1*/\n", indent, n->varName, fsig.func, n->varName);
                    }else{
                        wnodes.writeln("{}            const {}& {} = _n.{};/*wvar1x*/", indent, grammar.tokenClass, n->varName, n->varName);
                    }
                    params << std::format("{}{}/*wvar1*/", sep, n->varName);
                    rnode_used = true;
                }else {
                    if(n->isRule() == true) {
                        wnodes.writeln("{}            WalkerNodeRef<{}> {}(_n.{}, {});/*gvar2u*/", indent, n->name, n->varName, n->varName, called);
                    }else{
                        wnodes.writeln("{}            const {}& {} = _n.{};/*gvar2x*/", indent, grammar.tokenClass, n->varName, n->varName);
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
                            wnodes.writeln("{}            WalkerNodeRef<{}> {}(_n.{}, {});/*nwvar1u*/\n", indent, n->name, n->idxName, n->idxName, called);
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

        tw.writeln("{}        [&]({}const {}::{}& _n) -> {} {{", indent, node_unused, rs.name, r.ruleName, fsig.type);

        tw.swrite(wnodes);

        // call the handler
        if(fsig.isUDF == false) {
            tw.writeln("{}            {}({});/*call3*/", indent, iname, params.str());
        }else{
            tw.writeln("{}            return {}({});/*call4*/", indent, iname, params.str());
        }

        if(walker.traversalMode == yg::Walker::TraversalMode::TopDown) {
            tw.writeln("{}", wcalls.str());
        }
        tw.writeln("{}        }},", indent);
        tw.writeln();
    }

    /// @brief generates ruleset handler
    /// This function implements a visitor that calls the
    /// specific rule handler
    inline void generateRuleSetVisitor(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const ygp::RuleSet& rs,
        const yg::Walker::FunctionSig& fsig,
        const std::string_view& indent
    ) {
        tw.writeln("{}//RULESET_VISITOR({}):{}:{}", indent, walker.name, rs.name, fsig.type);

        std::string args;
        if(fsig.args.size() > 0) {
            args = std::format(", {}", fsig.args);
        }
        tw.writeln("{}inline {}", indent, fsig.type);
        tw.writeln("{}{}(WalkerNodeRef<{}>& node{}) {{", indent, fsig.func, rs.name, args);
        if(fsig.isUDF == false) {
            tw.writeln("{}    WalkerNodeCommit<{}> _wc(node);", indent, rs.name);
        }

        auto xparams = extractParams(fsig.args);

        std::string called;
        if(walker.traversalMode == yg::Walker::TraversalMode::Manual) {
            called = "true";
        }else{
            called = "false";
        }

        // generate Visitor
        tw.writeln("{}    return std::visit(overload{{", indent);

        if(true) {
            tw.writeln("{}        [&](const std::monostate&) -> {} {{", indent, fsig.type);
            tw.writeln("{}            throw std::runtime_error(\"internal_error\"); //should never reach here", indent);
            tw.writeln("{}        }},", indent);
            tw.writeln();
        }

        if(hasEpsilon(rs) == true) {
            tw.writeln("{}        [&](const {}::{}_r0&) -> {} {{", indent, rs.name, rs.name, fsig.type);
            tw.writeln("{}            //throw std::runtime_error(\"internal_error(epsilon)\"); //should never reach here", indent);
            tw.writeln("{}        }},", indent);
            tw.writeln();
        }

        // generate handlers for all rules in ruleset
        for (auto& r : rs.rules) {
            generateRuleVisitorBody(tw, walker, fsig, rs, *r, called, xparams, indent);
        }
        tw.writeln("{}    }}, node.node.rule);", indent);

        tw.writeln("{}}}", indent);
        tw.writeln();
    }

    /// @brief generates all ruleset handlers
    inline void generateRuleSetVisitors(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        if(grammar.isRootWalker(walker) == true) {
            tw.writeln("{}inline {}& {}({}& node) const {{", indent, grammar.tokenClass, walker.defaultFunctionName, grammar.tokenClass);
            tw.writeln("{}    return node;", indent);
            tw.writeln("{}}}", indent);
            tw.writeln();
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
                    generateRuleHandler(tw, walker, r, fi, isv, iso, ci, vars, indent);
                }

                if((grammar.isRootWalker(walker) == true) || (fi.isUDF == true)) {
                    tw.writeln("{}//RULE_VISITOR({}):{}", indent, walker.name, rs.name);
                    generateRuleSetVisitor(tw, walker, rs, fi, indent);
                }
            }
        }
    }

    /// @brief generates writer for Walker
    inline void generateWriter(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const std::string_view& indent
    ) {
        if(walker.outputType == yg::Walker::OutputType::TextFile) {
            tw.writeln("{}//GEN_FILE", indent);
            tw.writeln("{}TextFileWriter {};", indent, walker.writerName);
            tw.writeln();

            tw.writeln("{}inline void open(const std::filesystem::path& odir, const std::string_view& filename) {{", indent);
            tw.writeln("{}    {}.open(odir, filename, \"{}\");", indent, walker.writerName, walker.ext);
            tw.writeln("{}}}", indent);
        }else{
            tw.writeln("{}inline void open(const std::filesystem::path& odir, const std::string_view& filename) {{", indent);
            tw.writeln("{}    unused(odir, filename);", indent);
            tw.writeln("{}}}", indent);
        }
    }

    /// @brief generates all Walker's
    inline void generateWalkers(
        TextFileWriter& tw,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        auto xindent = std::string(indent) + "    ";

        // generate Walkers
        std::string first;
        for (auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            auto wname = std::format("Walker_{}", walker.name);
            tw.writeln();
            tw.writeln("{}//walker:{}", indent, walker.name);
            if(grammar.isRootWalker(walker) == true) {
                first = wname;
                tw.writeln("{}struct {} : public NonCopyable {{", indent, wname);
                tw.writeln("{}{}& mod;", xindent, grammar.className);
                tw.writeln("{}explicit inline {}({}& m) : mod(m) {{}}", xindent, wname, grammar.className);
                if(grammar.walkers.size() > 1) {
                    tw.writeln("{}virtual ~{}() {{}}", xindent, wname);
                }

                generateWriter(tw, walker, xindent);
                generateCodeBlock(tw, walker.xmembers, xindent, true, vars);
                generateRuleSetVisitors(tw, walker, vars, xindent);
                tw.writeln("{}}}; //walker:{}", indent, walker.name);
            }else{
                assert(walker.base != nullptr);
                tw.writeln("{}struct {} : public {} {{", indent, wname, first);
                tw.writeln("{}explicit inline {}({}& m) : {}(m) {{}}", xindent, wname, grammar.className, first);
                generateWriter(tw, walker, xindent);
                generateCodeBlock(tw, walker.xmembers, xindent, true, vars);
                generateRuleSetVisitors(tw, walker, vars, xindent);
                tw.writeln("{}}}; //walker:{}", indent, walker.name);
            }
        }
    }

    /// @brief generates code to add default Walker, if none specified
    inline void generateInitWalkers(TextFileWriter& tw, const std::string_view& indent) {
        for (auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            if(grammar.isBaseWalker(walker) == true) {
                continue;
            }
            tw.writeln("{}walkers.push_back(\"{}\");", indent, walker.name);
            break;
        }
    }

    /// @brief generates code to invoke specified Walker
    inline void generateWalkerCalls(TextFileWriter& tw, const std::string_view& indent) {
        if(grammar.walkers.size() == 0) {
            return;
        }

        for (auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            if(grammar.isBaseWalker(walker) == true) {
                continue;
            }
            auto wname = std::format("Walker_{}", walker.name);
            tw.writeln("{}else if(w == \"{}\") {{", indent, walker.name);
            tw.writeln("{}    {}::{} walker(module);", indent, grammar.astClass, wname);
            tw.writeln("{}    walker.open(odir, filename);", indent);
            tw.writeln("{}    {}::WalkerNodeRef<{}::{}> s(start);", indent, grammar.astClass, grammar.astClass, grammar.start);
            tw.writeln("{}    walker.go(s);", indent);
            tw.writeln("{}}}", indent);
        }
        tw.writeln("{}else {{", indent);
        tw.writeln("{}    throw std::runtime_error(\"unknown walker: \" + w);", indent);
        tw.writeln("{}}}", indent);
    }

    /// @brief generates all Token IDs inside an enum in the prototype file
    inline void generateTokenIDs(TextFileWriter& tw, const std::unordered_set<std::string>& tnames) {
        for (auto& t : tnames) {
            tw.writeln("        {},", t);
        }
    }

    /// @brief generates the string names for all Token IDs inside a map in the prototype file
    inline void generateTokenIDNames(TextFileWriter& tw, const std::unordered_set<std::string>& tnames) {
        for (auto& t : tnames) {
            tw.writeln("            {{ID::{}, \"{}\"}},", t, t);
        }
    }

    /// @brief generates function declarations to create each AST node
    inline void generateCreateASTNodesDecls(TextFileWriter& tw) {
        for (auto& rs : grammar.ruleSets) {
            tw.writeln("template<>");
            tw.writeln("inline {}::{}& Parser::create<{}::{}>(const ValueItem& vi);", grammar.astClass, rs->name, grammar.astClass, rs->name);
            tw.writeln();
        }
    }

    /// @brief generates functions definitions to create each AST node
    /// These functions are called by the Parser on REDUCE actions
    inline void generateCreateASTNodesDefns(TextFileWriter& tw, const std::unordered_map<std::string, std::string>& vars) {
        for (auto& rs : grammar.ruleSets) {
            tw.writeln("template<>");
            tw.writeln("inline {}::{}& Parser::create<{}::{}>(const ValueItem& vi) {{", grammar.astClass, rs->name, grammar.astClass, rs->name);
            tw.writeln("    switch(vi.ruleID) {{");
            if(hasEpsilon(*rs) == true) {
                tw.writeln("    case 0: {{");
                tw.writeln("        // EPSILON-C");
                tw.writeln("        auto& _cv_anchor = vi;");
                tw.writeln("        auto& anchor = create<{}::{}>(_cv_anchor);", grammar.astClass, grammar.tokenClass);
                tw.writeln("        auto& cel = ast.createAstNode<{}::{}>(vi.token.pos, anchor);", grammar.astClass, rs->name);
                tw.writeln("        cel.rule.emplace<{}::{}::{}_r0>();", grammar.astClass, rs->name, rs->name);
                tw.writeln("        return cel;");
                tw.writeln("    }} // case");
            }
            for (auto& r : rs->rules) {
                tw.writeln("    case {}: {{", r->id);
                tw.writeln("        //{}", r->str(false));
                tw.writeln("        assert(vi.childs.size() == {});", r->nodes.size());

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
                    tw.writeln("        auto& _cv_{} = *(vi.childs.at({}));", varName, idx);
                    tw.writeln("        auto& p{} = create<{}::{}>(_cv_{});", varName, grammar.astClass, rt, varName);
                    if (idx == r->anchor) {
                        tw.writeln("        auto& anchor = create<{}::{}>(_cv_{});", grammar.astClass, grammar.tokenClass, varName);
                        hasAnchor = ", anchor";
                    }
                    ss << std::format("{}p{}", sep, varName);
                    sep = ", ";
                }

                tw.writeln("        auto& cel = ast.createAstNode<{}::{}>(vi.token.pos{});", grammar.astClass, rs->name, hasAnchor);
                tw.writeln("        cel.rule.emplace<{}::{}::{}>({});", grammar.astClass, rs->name, r->ruleName, ss.str());
                tw.writeln("        return cel;");
                tw.writeln("    }} // case");
            }
            tw.writeln("    }} // switch");
            generateError(tw, "vi.token.pos.row", "vi.token.pos.col", "vi.token.pos.file", "std::format(\"ASTGEN_ERROR:{}\", vi.ruleID)", "    ", vars);
            tw.writeln("}}");
            tw.writeln();
        }
    }

    /// @brief generates case statements for the Parser
    /// Each case block checks the next Token received from the Lexer
    /// and decides whether to SHIFT, REDUCE or GOTO to the next state
    inline void generateParserTransitions(TextFileWriter& tw, const std::unordered_map<std::string, std::string>& vars) {
        for (auto& ps : grammar.itemSets) {
            auto& itemSet = *ps;
            assert((itemSet.shifts.size() > 0) || (itemSet.reduces.size() > 0) || (itemSet.gotos.size() > 0));
            bool breaked = false;

            tw.writeln("            case {}:", itemSet.id);
            tw.writeln("                std::print(log(), \"{{}}\", \"{}\\n\");", itemSet.str("", "\\n", true));
            tw.writeln("                switch(k.id) {{");
            for (auto& c : itemSet.shifts) {
                for(auto& fb : c.first->fallbacks) {
                    if ((itemSet.hasShift(*fb)) || (itemSet.hasReduce(*fb))) {
                        continue;
                    }
                    tw.writeln("                case Tolkien::ID::{}: // SHIFT(fallback)", fb->name);
                }

                tw.writeln("                case Tolkien::ID::{}: // SHIFT", c.first->name);
                for(auto& e : c.second.epsilons) {
                    tw.writeln("                    shift(k.pos, Tolkien::ID::{}); //EPSILON-S", e->name);
                    tw.writeln("                    stateStack.push_back(0);");
                    tw.writeln("                    reduce(0, 1, 0, Tolkien::ID::{}, \"{}\");", e->name, e->name);
                    tw.writeln("                    stateStack.push_back(0);");
                }
                tw.writeln("                    std::print(log(), \"SHIFT {}: t={}\\n\");", c.second.next->id, c.first->name);
                tw.writeln("                    shift(k);");
                tw.writeln("                    stateStack.push_back({});", c.second.next->id);
                tw.writeln("                    return accepted;");
            }
            for (auto& rd : itemSet.reduces) {
                auto& c = *(rd.second.next);
                auto& r = c.rule;
                tw.writeln("                case Tolkien::ID::{}: // REDUCE", rd.first->name);
                tw.writeln("                    std::print(log(), \"REDUCE:{}:{{}}/{}\\n\", \"{}\");", rd.first->name, r.nodes.size(), c.str());
                auto len = rd.second.len;
                while(len < r.nodes.size()) {
                    tw.writeln("                    //shift-epsilon: len={}", len);
                    tw.writeln("                    shift(k.pos, Tolkien::ID::{}); //EPSILON-R", grammar.empty);
                    tw.writeln("                    stateStack.push_back(0);");
                    ++len;
                }

                if ((r.ruleSetName() == grammar.start) && (rd.first->name == grammar.end)) {
                    tw.writeln("                    shift(k.pos, Tolkien::ID::{}); //END", grammar.end);
                    tw.writeln("                    stateStack.push_back(0);");
                }
                tw.writeln("                    reduce({}, {}, {}, Tolkien::ID::{}, \"{}\");", r.id, len, r.anchor, r.ruleSetName(), r.ruleSetName());
                tw.writeln("                    k.id = Tolkien::ID::{};", r.ruleSetName());
                if (r.ruleSetName() == grammar.start) {
                    tw.writeln("                    accepted = true;");
                    tw.writeln("                    return accepted;");
                }else{
                    tw.writeln("                    break;");
                    breaked = true;
                }
            }
            for (auto& c : itemSet.gotos) {
                tw.writeln("                case Tolkien::ID::{}: // GOTO", c.first->name);
                tw.writeln("                    std::print(log(), \"GOTO {}:id={}, rule={}\\n\");", c.second->id, c.first->id, c.first->name);
                tw.writeln("                    stateStack.push_back({});", c.second->id);
                tw.writeln("                    k = k0;");
                tw.writeln("                    break;");
                breaked = true;
            }
            tw.writeln("                default:");
            generateError(tw, "k.pos.row", "k.pos.col", "k.pos.file", "\"SYNTAX_ERROR\"", "                    ", vars);
            tw.writeln("                }} // switch(k.id)");
            if(breaked == true) {
                tw.writeln("                break;");
            }
        }
    }

    /// @brief class to calculate transitions from one Lexer state to the next
    struct TransitionSet {
        const yglx::Transition* wildcard = nullptr;

        std::vector<std::pair<const yglx::Transition*, const yglx::RangeClass*>> smallRanges;
        std::vector<std::pair<const yglx::Transition*, const yglx::RangeClass*>> largeRanges;
        std::vector<std::pair<const yglx::Transition*, const yglx::LargeEscClass*>> largeEscClasses;

        std::vector<std::pair<const yglx::Transition*, const yglx::ClassTransition*>> classes;

        std::pair<const yglx::Transition*, const yglx::ClosureTransition*> enterClosure = {nullptr, nullptr};
        std::pair<const yglx::Transition*, const yglx::ClosureTransition*> preLoop = {nullptr, nullptr};
        std::pair<const yglx::Transition*, const yglx::ClosureTransition*> inLoop = {nullptr, nullptr};
        std::pair<const yglx::Transition*, const yglx::ClosureTransition*> postLoop = {nullptr, nullptr};
        std::pair<const yglx::Transition*, const yglx::ClosureTransition*> leaveClosure = {nullptr, nullptr};

        std::pair<const yglx::Transition*, const yglx::SlideTransition*> slide = {nullptr, nullptr};

        struct Visitor {
            const yg::Grammar& grammar;
            TransitionSet& tset;
            const yglx::Transition& tx;

            inline Visitor(const yg::Grammar& g, TransitionSet& s, const yglx::Transition& x)
                : grammar(g), tset(s), tx(x) {}

            inline void operator()(const yglx::WildCard&) {
                assert(tset.wildcard == nullptr);
                tset.wildcard = &tx;
            }

            inline void operator()(const yglx::LargeEscClass& t) {
                tset.largeEscClasses.push_back(std::make_pair(&tx, &t));
            }

            inline void operator()(const yglx::RangeClass& t) {
                bool isSmallRange = ((t.ch2 - t.ch1) <= grammar.smallRangeSize);
                if (isSmallRange) {
                    tset.smallRanges.push_back(std::make_pair(&tx, &t));
                }else{
                    tset.largeRanges.push_back(std::make_pair(&tx, &t));
                }
            }

            inline void operator()(const yglx::PrimitiveTransition& t) {
                std::visit(*this, t.atom.atom);
            }

            inline void operator()(const yglx::ClassTransition& t) {
                tset.classes.push_back(std::make_pair(&tx, &t));
            }

            inline void operator()(const yglx::ClosureTransition& t) {
                switch(t.type) {
                case yglx::ClosureTransition::Type::Enter:
                    assert(tset.enterClosure.first == nullptr);
                    tset.enterClosure = std::make_pair(&tx, &t);
                    break;
                case yglx::ClosureTransition::Type::PreLoop:
                    assert(tset.preLoop.first == nullptr);
                    tset.preLoop = std::make_pair(&tx, &t);
                    break;
                case yglx::ClosureTransition::Type::InLoop:
                    assert(tset.inLoop.first == nullptr);
                    tset.inLoop = std::make_pair(&tx, &t);
                    break;
                case yglx::ClosureTransition::Type::PostLoop:
                    assert(tset.postLoop.first == nullptr);
                    tset.postLoop = std::make_pair(&tx, &t);
                    break;
                case yglx::ClosureTransition::Type::Leave:
                    assert(tset.leaveClosure.first == nullptr);
                    tset.leaveClosure = std::make_pair(&tx, &t);
                    break;
                }
            }

            inline void operator()(const yglx::SlideTransition& t) {
                assert(tset.slide.first == nullptr);
                tset.slide = std::make_pair(&tx, &t);
            }
        };

        inline void process(const yg::Grammar& g, const std::vector<yglx::Transition*>& txs) {
            for (auto& tx : txs) {
                Visitor v(g, *this, *tx);
                std::visit(v, tx->t);
            }
        }
    };

    /// @brief generate code to transition from one Lexer state to another
    inline void generateStateChange(TextFileWriter& tw, const yglx::Transition& t, const yglx::State* nextState, const std::string& indent) {
        if (t.capture) {
            tw.writeln("                {}token.addText(ch);", indent);
        }
        tw.writeln("                {}stream.consume();", indent);
        tw.writeln("                {}state = {};", indent, nextState->id);
    }

    /// @brief generate Lexer states
    inline void generateLexerStates(TextFileWriter& tw, const std::unordered_map<std::string, std::string>& vars) {
        tw.writeln("            case 0:");
        generateError(tw, "stream.pos.row", "stream.pos.col", "stream.pos.file", "\"SYNTAX_ERROR\"", "                ", vars);

        for (auto& ps : grammar.states) {
            auto& state = *ps;

            TransitionSet tset;
            tset.process(grammar, state.transitions);
            tset.process(grammar, state.superTransitions);
            tset.process(grammar, state.shadowTransitions);

            tw.writeln("            case {}:", state.id);
            if(state.isRoot == true) {
                tw.writeln("                token = Tolkien(stream.pos);");
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
                tw.writeln("                assert(counts.size() > 0);");
                if (tset.preLoop.first != nullptr) {
                    tw.writeln("                if(count() < {}) {{", tx.atom.min);
                    tw.writeln("                    ++counts.back();");
                    tw.writeln("                    state = {};", tset.preLoop.first->next->id);
                    tw.writeln("                    continue; //precount");
                    tw.writeln("                }}");
                }

                std::string chkx;
                auto mrc = (tx.atom.max == grammar.maxRepCount ? "MaxRepeatCount" : std::to_string(tx.atom.max));
                if(tx.atom.min > 0) {
                    chkx = std::format("(count() >= {}) && (count() < {})", tx.atom.min, mrc);
                }else{
                    chkx = std::format("count() < {}", mrc);
                }

                tw.writeln("                if({}) {{", chkx);
                tw.writeln("                    ++counts.back();");
                tw.writeln("                    state = {};", tset.inLoop.first->next->id);
                tw.writeln("                    continue; //inLoop");
                tw.writeln("                }}");

                assert(tset.postLoop.first != nullptr);
                tw.writeln("                assert(count() == {});", mrc);
                tw.writeln("                counts.pop_back();");
                tw.writeln("                state = {};", tset.postLoop.first->next->id);
                tw.writeln("                continue; //postLoop");

                continue;
            }

            // END check must always be the second one
            if (state.checkEOF) {
                tw.writeln("                if(ch == static_cast<char_t>(EOF)) {{");
                tw.writeln("                    token.id = Tolkien::ID::{};", grammar.end);
                tw.writeln("                    parser.parse(token);");
                tw.writeln();
                tw.writeln("                    // at EOF, call parse() repeatedly until all final reductions are complete");
                tw.writeln("                    while(parser.isClean() == false) {{");
                tw.writeln("                        parser.parse(token);");
                tw.writeln("                    }}");
                tw.writeln("                    state = 0;");
                tw.writeln("                    stream.consume();");
                tw.writeln("                    continue; //EOF");
                tw.writeln("                }}");
            }

            // generate switch cases for small ranges
            if (tset.smallRanges.size() > 0) {
                tw.writeln("                switch(ch) {{");

                for (auto& t : tset.smallRanges) {
                    assert(t.second->ch2 >= t.second->ch1);
                    for (auto c = t.second->ch1; c <= t.second->ch2; ++c) {
                        tw.writeln("                case {}:", getChString(c));
                    }
                    generateStateChange(tw, *(t.first), t.first->next, "    ");
                    tw.writeln("                    continue; //smallRange");
                }

                tw.writeln("                }}");
            }

            // generate checks for large escape classes
            for (auto& t : tset.largeEscClasses) {
                tw.writeln("                if({}(ch)) {{", t.second->checker);
                generateStateChange(tw, *(t.first), t.first->next, "    ");
                tw.writeln("                    continue; //largeEsc");
                tw.writeln("                }}");
            }

            // generate checks for large ranges
            for (auto& t : tset.largeRanges) {
                tw.writeln("                // id={}, large", state.id);
                tw.writeln("                if(contains(ch, {}, {})) {{", getChString(t.second->ch1), getChString(t.second->ch2));
                generateStateChange(tw, *(t.first), t.first->next, "    ");
                tw.writeln("                    continue; //largeRange");
                tw.writeln("                }}");
            }

            // generate checks for classes
            for (auto& t : tset.classes) {
                std::stringstream ss;
                std::string sep;
                std::string sepx = (t.second->atom.negate?" && ":" || ");
                std::string negate = (t.second->atom.negate?"!":"");
                for(auto& ax : t.second->atom.atoms) {
                    std::visit(overload{
                        [&negate, &ss, &sep](const yglx::WildCard&) {
                            ss << std::format("{}({}true)", sep, negate);
                        },
                        [&negate, &ss, &sep](const yglx::LargeEscClass& a) {
                            ss << std::format("{}({}{}(ch))", sep, negate, a.checker);
                        },
                        [&negate, &ss, &sep](const yglx::RangeClass& a) {
                            ss << std::format("{}({}contains(ch, {}, {}))", sep, negate, getChString(a.ch1), getChString(a.ch2));
                        },
                    }, ax);
                    sep = sepx;
                }
                tw.writeln("                if((ch != static_cast<char_t>(EOF)) && ({})) {{", ss.str());
                generateStateChange(tw, *(t.first), t.first->next, "    ");
                tw.writeln("                    continue; //Class");
                tw.writeln("                }}");
            }

            // the sequence of checks in this if-ladder is significant
            if (tset.wildcard != nullptr) {
                generateStateChange(tw, *(tset.wildcard), tset.wildcard->next, "");
                tw.writeln("                continue; //wildcard");
            }else if (tset.slide.first != nullptr) {
                assert(tset.wildcard == nullptr);
                tw.writeln("                state = {};", tset.slide.first->next->id);
                tw.writeln("                continue; //slide");
            }else if (tset.enterClosure.first != nullptr) {
                tw.writeln("                counts.push_back({});", tset.enterClosure.second->initialCount);
                tw.writeln("                state = {};", tset.enterClosure.first->next->id);
                tw.writeln("                continue; //enterClosure");
            }else if (state.matchedRegex) {
                size_t nextStateID = 0;
                switch(state.matchedRegex->modeChange) {
                case yglx::Regex::ModeChange::None:
                    break;
                case yglx::Regex::ModeChange::Next: {
                    auto& mode = grammar.getRegexNextMode(*(state.matchedRegex));
                    assert(mode.root);
                    nextStateID = mode.root->id;
                    tw.writeln("                modes.push_back({}); // MATCH, -> {}", nextStateID, state.matchedRegex->nextMode);
                    break;
                }
                case yglx::Regex::ModeChange::Back:
                    tw.writeln("                assert(modes.size() > 0);");
                    tw.writeln("                modes.pop_back();");
                    break;
                case yglx::Regex::ModeChange::Init:
                    tw.writeln("                assert(modes.size() > 0);");
                    tw.writeln("                modes.clear();");
                    tw.writeln("                modes.push_back(1);");
                    break;
                }
                tw.writeln("                state = modeRoot();");

                if (state.matchedRegex->usageCount > 0) {
                    tw.writeln("                token.id = Tolkien::ID::{};", state.matchedRegex->regexName);
                    tw.writeln("                parser.parse(token);");
                }else{
                    tw.writeln("                token = Tolkien(stream.pos);");
                }
                tw.writeln("                continue;");
            }else if (tset.leaveClosure.first != nullptr) {
                assert(tset.wildcard == nullptr);
                assert(tset.slide.first == nullptr);
                tw.writeln("                state = {};", tset.leaveClosure.first->next->id);
                tw.writeln("                continue; //leaveClosure");
            }else{
                generateError(tw, "stream.pos.row", "stream.pos.col", "stream.pos.file", "\"SYNTAX_ERROR\"", "                ", vars);
            }
        }
    }

    /// @brief main parser generator
    /// This function reads the prototype.cpp file (embedded as a raw string)
    /// and acts on each meta command found in the file.
    inline void includeCodeBlock(
        const char* codeBlock,
        TextFileWriter& tw,
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

//#define PRINT(...) std::println(__VA_ARGS__)
#define PRINT(...)

            // static int x = 0;
            // assert(++x < 10);

            switch (token) {
            case Token::EnterBlock: {
                PRINT("Init::EB:{}", line);
                if (eblockName != "SKIP") {
                    tw.writeln("{}", line);
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
                        tw.writeln("#define HAS_REPL {}", (grammar.hasREPL == true)?1:0);
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
                    tw.writeln("{}", line);
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
                tw.writeln("{}:BEGIN //{}", line, tw.row);
                if (segmentName == "pchHeader") {
                    generatePchHeader(tw, indent);
                }else if (segmentName == "hdrHeaders") {
                    generateHdrHeaders(tw, indent);
                }else if (segmentName == "srcHeaders") {
                    generateSrcHeaders(tw, indent);
                }else if (segmentName == "classMembers") {
                    generateClassMembers(tw, indent);
                }else if (segmentName == "astNodeDecls") {
                    generateAstNodeDecls(tw, indent);
                }else if (segmentName == "astNodeDefns") {
                    generateAstNodeDefns(tw, indent);
                }else if (segmentName == "astNodeItems") {
                    generateAstNodeItems(tw, indent);
                }else if (segmentName == "walkers") {
                    generateWalkers(tw, vars, indent);
                }else if (segmentName == "prologue") {
                    tw.writeln("{}:XX-1 //{}", line, tw.row);
                    generateCodeBlock(tw, grammar.prologue, indent, true, vars);
                    tw.writeln("{}:XX-2 //{}", line, tw.row);
                }else if (segmentName == "initWalkers") {
                    if(amalgamatedFile) {
                        generateInitWalkers(tw, indent);
                    }
                }else if (segmentName == "walkerCalls") {
                    generateWalkerCalls(tw, indent);
                }else if (segmentName == "epilogue") {
                    generateCodeBlock(tw, grammar.epilogue, indent, true, vars);
                }else if (segmentName == "tokenIDs") {
                    generateTokenIDs(tw, tnames);
                }else if (segmentName == "tokenIDNames") {
                    generateTokenIDNames(tw, tnames);
                }else if (segmentName == "createASTNodesDecls") {
                    generateCreateASTNodesDecls(tw);
                }else if (segmentName == "createASTNodesDefns") {
                    generateCreateASTNodesDefns(tw, vars);
                }else if (segmentName == "parserTransitions") {
                    generateParserTransitions(tw, vars);
                }else if (segmentName == "lexerStates") {
                    generateLexerStates(tw, vars);
                }else{
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_SEGMENT:{}", segmentName);
                }
                tw.writeln("{}:END //{}", line, tw.row);
                break;
            }
            case Token::Include: {
                PRINT("Line::INCLUDE:{}", line);
                tw.writeln("{}:BEGIN", line);
                if (includeName == "utf8Encoding") {
                    if (grammar.unicodeEnabled == true) {
                        includeCodeBlock(cb_encoding_utf8, tw, vars, tnames, filebase, srcName);
                    }
                }else if (includeName == "asciiEncoding") {
                    if (grammar.unicodeEnabled == false) {
                        includeCodeBlock(cb_encoding_ascii, tw, vars, tnames, filebase, srcName);
                    }
                }else if (includeName == "stream") {
                    includeCodeBlock(cb_stream, tw, vars, tnames, filebase, srcName);
                }else if (includeName == "textWriter") {
                    includeCodeBlock(cb_text_writer, tw, vars, tnames, filebase, srcName);
                }else if (includeName == "print") {
                    includeCodeBlock(cb_print, tw, vars, tnames, filebase, srcName);
                }else if (includeName == "nsutil") {
                    includeCodeBlock(cb_nsutil, tw, vars, tnames, filebase, srcName);
                }else if (includeName == "filepos") {
                    includeCodeBlock(cb_filepos, tw, vars, tnames, filebase, srcName);
                }else{
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_INCLUDE:{}", includeName);
                }
                tw.writeln("{}:END", line);
                break;
            }
            case Token::Target: {
                PRINT("Line::TARGET:{}", line);
                if ((!amalgamatedFile) && (targetName == "SOURCE")) {
                    auto hdrName = srcName;
                    auto nsrcName = filebase.string() + ".cpp";
                    PRINT("reopening:{}", nsrcName);
                    tw.open(nsrcName);
                    if (tw.isOpen() == false) {
                        throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_SRC:{}", nsrcName);
                    }
                    tw.writeln("#include \"{}\"", std::filesystem::path(hdrName).filename().string());
                }
                break;
            }
            case Token::Line: {
                PRINT("Init::LINE:{}, eblockName=[{}]", line, eblockName);
                if (skip == false) {
                    generatePrototypeLine(tw, line, vars);
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
        TextFileWriter tw;
        tw.open(srcName);
        if (tw.isOpen() == false) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_SRC:{}", srcName);
        }
        if(amalgamatedFile == false) {
            tw.writeln("#pragma once");
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

        includeCodeBlock(cb_prototype, tw, vars, tnames, filebase, srcName);
    }
};
}

void generateGrammar(const yg::Grammar& g, const std::filesystem::path& of) {
    Generator gen(g);
    gen.generate(of);
}
