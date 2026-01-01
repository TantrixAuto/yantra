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
#include "options.hpp"

/// @brief This is the UNICODE encoding file, embedded as a raw C string
extern const char* const cb_encoding_utf8;

/// @brief This is the ASCII encoding file, embedded as a raw C string
extern const char* const cb_encoding_ascii;

/// @brief This is the prototype.cpp file, embedded as a raw C string
extern const char* const cb_prototype;

/// @brief This is the stream.hpp file, embedded as a raw C string
extern const char* const cb_stream;

/// @brief This is the text_writer.hpp file, embedded as a raw C string
extern const char* const cb_text_writer;

/// @brief This is the print.hpp file, embedded as a raw C string
extern const char* const cb_print;

/// @brief This is the nsutil.hpp file, embedded as a raw C string
extern const char* const cb_nsutil;

/// @brief This is the filepos.hpp file, embedded as a raw C string
extern const char* const cb_filepos;

namespace {

/// @brief This class generates the cpp parser
struct Generator {
    const yg::Grammar& grammar;
    std::string qidNamespace;
    std::string qidClassName;
    std::string qidNameAST;
    CodeBlock cb_astNodeDecls;
    CodeBlock throwError;

    inline explicit Generator(const yg::Grammar& g) : grammar(g) {}

    /// @brief expands all variables in a codeblock and normalizes the indentation
    static inline void
    _expand(
        StringStreamWriter& sw,
        const std::string_view& codeblock,
        const bool& autoIndent,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        enum class State : uint8_t {
            InitSpace0,
            InitSpace1,
            Code,
            Tag0,
            Tag1,
            Tag2,
            Tag3,
            Tag4,
            Tag5,
            Tag6,
        };
        static std::unordered_map<State, const char*> sname = {
            {State::InitSpace0, "InitSpace0"},
            {State::InitSpace1, "InitSpace1"},
            {State::Code, "Code"},
            {State::Tag0, "Tag0"},
            {State::Tag1, "Tag1"},
            {State::Tag2, "Tag2"},
            {State::Tag3, "Tag3"},
            {State::Tag4, "Tag4"},
            {State::Tag5, "Tag5"},
            {State::Tag6, "Tag6"},
        };

        State state = State::InitSpace0;
        size_t space0 = 0;
        size_t space1 = 0;
        std::string key;
        std::string key2;
        for(const auto& ch : codeblock) {
            //std::println("state={}, ch={}", sname.at(state), ch);
            switch(state) {
            case State::InitSpace0:
                if((ch == '\r') || (ch == '\n')) {
                    space0 = 0;
                    continue;
                }
                if(autoIndent && (std::isspace(ch) != 0)) {
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
                if(autoIndent && (std::isspace(ch) != 0) && (ch != '\n')) {
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
                if (ch == '2') {
                    state = State::Tag4;
                    continue;
                }
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
                }else if ((isalpha(ch) != 0) || (ch == '_')) {
                    key += ch;
                }else{
                    sw.write("TAG({}", key);
                    state = State::Code;
                }
                break;
            case State::Tag4:
                if (ch == '(') {
                    state = State::Tag5;
                    continue;
                }
                sw.write("TAG2{}", ch);
                state = State::Code;
                break;
            case State::Tag5:
                if (ch == ',') {
                    key2 = "";
                    state = State::Tag6;
                    continue;
                }else if ((isalpha(ch) != 0) || (ch == '_')) {
                    key += ch;
                }else{
                    sw.write("TAG2({}", key);
                    state = State::Code;
                }
                break;
            case State::Tag6:
                if (ch == ')') {
                    if (auto it = vars.find(key); it != vars.end()) {
                        sw.write("{}{}", it->second, key2);
                    }else{
                        sw.write("TAG2({},{})", key, key2);
                    }
                    state = State::Code;
                }else if ((isalpha(ch) != 0) || (ch == '_')) {
                    key2 += ch;
                }else{
                    sw.write("TAG2({},{}", key, key2);
                    state = State::Code;
                }
                break;
            }
        }
    }

    /// @brief optionally appends a #line to an expanded codeblock
    static inline void
    generateCodeBlock(
        TextFileWriter& tw,
        const CodeBlock& codeblock,
        const std::string_view& indent,
        const bool& autoIndent,
        const std::unordered_map<std::string, std::string>& vars
    ) {
        StringStreamWriter sw;
        _expand(sw, codeblock.code, autoIndent, vars, tw.indent);
        if (codeblock.hasPos == false) {
            tw.swrite(sw);
            return;
        }

        std::string pline = (opts().genLines == false) ? "//" : "";
        unused(indent);

        tw.writeln("{}#line {} \"{}\" //t={},s={}", pline, codeblock.pos.row, codeblock.pos.file, tw.row, sw.row);
        tw.swrite(sw);
        tw.writeln("{}#line {} \"{}\" //t={},s={}", pline, tw.row + 1, tw.file.string(), tw.row, sw.row);
    }

    /// @brief writes expanded codeblock to output stream if the codeblock is not empty
    static inline void
    generatePrototypeLine(
        TextFileWriter& tw,
        const std::string_view& line,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        if(line.empty() == true) {
            // print empty line as-is
            tw.writeln();
        }else{
            StringStreamWriter sw;
            _expand(sw, line, false, vars, indent);
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
    ) const {
        auto xvars = vars;
        xvars["ROW"] = line;
        xvars["COL"] = col;
        xvars["SRC"] = file;
        xvars["MSG"] = msg;
        StringStreamWriter sw;
        _expand(sw, throwError.code, true, xvars, indent);
        tw.swriteln(sw);
    }

    /// @brief Construct a full function name given a rule and a short function name
    static inline auto
    getFunctionName(const ygp::Rule& r, const std::string& fname) -> std::string {
        return std::format("{}_{}", r.ruleName, fname);
    }

    /// @brief Get the type for any node in a production.
    /// If the node is a terminal, returns the default token class name
    /// If non-terminal, return the AST node name for the rule
    /// (which is the same as the rule name)
    inline auto getNodeType(const ygp::Node& n) const -> std::string {
        std::string nativeType;
        if(n.isRegex() == true) {
            nativeType = grammar.tokenClass;
        }else{
            assert(n.isRule() == true);
            nativeType = std::format("{}", n.name);
        }
        return nativeType;
    }

    /// @brief generates a list of args for a semantic action function for a rule in a single line
    inline auto
    getArgs(
        const ygp::Rule& r,
        const yg::Walker::FunctionSig& fsig,
        const yg::Walker::CodeInfo* ci
    ) -> std::string {
        std::stringstream ss;
        std::string sep;

        std::string isUnused;
        if(ci == nullptr) {
            isUnused = "[[maybe_unused]]";
        }

        ss << std::format("{}[[maybe_unused]]const {}::{}& {}", sep, r.ruleSetName(), r.ruleName, r.ruleName);
        sep = ", ";

        for(const auto& n : r.nodes) {
            auto varName = n->varName;
            if(varName.size() == 0) {
                continue;
            }

            std::string nativeTypeNode;
            if(n->isRegex() == true) {
                nativeTypeNode = std::format("const {}", grammar.tokenClass);
            }else{
                nativeTypeNode = std::format("NodeRef<{}>", n->name);
            }

            ss << std::format("{}{}{}& {}", sep, isUnused, nativeTypeNode, varName);
            sep = ", ";
        }
        if(fsig.args.size() > 0) {
            ss << std::format("{}{}", sep, fsig.args);
        }
        return ss.str();
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
    static inline auto
    extractParams(const std::string& args) -> std::string {
        enum class State : uint8_t {
            Init,
            Var,
            Type,
        };

        std::vector<std::string> vars;
        State state = State::Init;
        std::string var;
        for(const auto& ch : std::ranges::reverse_view(args)) {
            switch(state) {
            case State::Init:
                // print("Init: ch={}", ch);
                if(std::isspace(ch) != 0) {
                    break;
                }
                var = ch;
                state = State::Var;
                break;
            case State::Var:
                // print("Var: ch={}", ch);
                if(std::isspace(ch) != 0) {
                    std::ranges::reverse(var);
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
        for (const auto& h : grammar.hdrHeaders) {
            tw.writeln("{}#include \"{}\"", indent, h);
        }

        for (const auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            if(walker.interfaceName.size() > 0) {
                tw.writeln(R"({}#include "{}.hpp")", indent, walker.interfaceName);
            }
        }
    }

    /// @brief generates code to include header files in the parser's source file
    inline void generateSrcHeaders(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& h : grammar.srcHeaders) {
            tw.writeln("{}#include \"{}\"", indent, h);
        }
    }

    /// @brief generates additional class members for the parser, if any are specified in the .y file
    inline void generateClassMembers(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& m : grammar.classMembers) {
            tw.writeln("{}{};", indent, m);
        }
    }

    /// @brief generates function sig for walker
    inline auto
    generateWalkerSig(const yg::Walker& w) -> std::string {
        std::stringstream ss;
        std::string sep;
        ss << std::format("walk_{}(", w.name);
        if(w.interfaceName.size() > 0) {
            ss << std::format("{}& walker", w.interfaceName);
            sep = ", ";
        }else if(w.xctor_args.size() > 0) {
            ss << std::format("{}", w.xctor_args);
            sep = ", ";
        }
        if(w.outputType == yg::Walker::OutputType::TextFile) {
            ss << sep << "const std::filesystem::path& odir, const std::string_view& filename";
            sep = ", ";
        }
        ss << ")";
        return ss.str();
    }

    /// @brief generates calls to each walker
    static inline auto
    generateWalkerCall(const yg::Walker& w) -> std::string {
        std::stringstream ss;
        std::string sep;
        if(w.interfaceName.size() > 0) {
            ss << std::format("walker");
            sep = ", ";
        }else if(w.xctor_args.size() > 0) {
            auto xparams = extractParams(w.xctor_args);
            ss << std::format("{}", xparams);
            sep = ", ";
        }
        if(w.outputType == yg::Walker::OutputType::TextFile) {
            ss << sep << "odir, filename";
        }
        return ss.str();
    }

    /// @brief generates calls to each walker
    inline void generateWalkerCallDecls(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& pw : grammar.walkers) {
            auto& w = *pw;
            auto wsig = generateWalkerSig(w);
            tw.writeln("{}void {};", indent, wsig);
        }
    }

    /// @brief generates calls to each walker
    inline void generateWalkerCallDefns(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& pw : grammar.walkers) {
            auto& w = *pw;
            auto wsig = generateWalkerSig(w);
            auto wcall = generateWalkerCall(w);
            tw.writeln("{}void {}::{} {{", indent, qidClassName, wsig);
            tw.writeln("{}    return _impl->walk_{}({});", indent, w.name, wcall);
            tw.writeln("{}}}", indent);
            tw.writeln();

            tw.writeln("{}void {}::walk({}& m) {{", indent, w.name, grammar.className);
            tw.writeln("{}    auto& start = m._impl->ast._root();", indent);
            tw.writeln("{}    Walker_{}::NodeRef<{}_AST::{}> s(start);", indent, w.name, grammar.className, grammar.start);
            tw.writeln("{}    go(s);", indent);
            tw.writeln("{}}}", indent);
        }
    }

    /// @brief generates calls to each walker
    inline void generateWalkerCallImpls(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& pw : grammar.walkers) {
            auto& w = *pw;
            auto wsig = generateWalkerSig(w);
            tw.writeln("{}inline void {} {{", indent, wsig);
            auto wname = w.name;
            if(w.interfaceName.size() > 0) {
                wname = w.interfaceName;
            }else if(w.xctor_args.size() > 0) {
                auto xparams = extractParams(w.xctor_args);
                tw.writeln("{}    Walker_{} walker(ymodule, {});", indent, w.name, xparams);
                wname = std::format("Walker_{}", w.name);
            }else{
                tw.writeln("{}    Walker_{} walker(ymodule);", indent, w.name);
                wname = std::format("Walker_{}", w.name);
            }
            if(w.outputType == yg::Walker::OutputType::TextFile) {
                tw.writeln("{}    walker.open(odir, filename);", indent);
            }
            tw.writeln("{}    auto& start = ast._root();", indent);
            tw.writeln("{}    {}::NodeRef<{}_AST::{}> s(start);", indent, wname, grammar.className, grammar.start);
            tw.writeln("{}    walker.go(s);", indent);
            tw.writeln("{}}}", indent);
            tw.writeln();
        }
    }

    /// @brief generates declarations for all AST nodes
    inline void generateAstNodeDecls(
        TextFileWriter& tw,
        const std::string_view& indent
    ) {
        for (const auto& rs : grammar.ruleSets) {
            tw.writeln("{}struct {};", indent, rs->name);
        }
    }

    /// @brief generates definitions for all AST nodes
    inline void generateAstNodeDefns(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& rs : grammar.ruleSets) {
            tw.writeln("{}struct {} : public NonCopyable {{", indent, rs->name);
            tw.writeln("{}    const FilePos pos;", indent);

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
                tw.writeln();
                tw.writeln("{}        void dump(std::ostream& ss, const size_t& lvl, const FilePos& p, const std::string& indent, const size_t& depth) const;", indent);
                tw.writeln("{}    }};", indent);
                tw.writeln();
            }

            std::string sep;
            tw.writeln("{}    using Rule = std::variant<", indent);
            tw.writeln("{}        {}_astEmpty", indent, sep);
            sep = ",";
            for (auto& r : rs->rules) {
                tw.writeln("{}        {}{}", indent, sep, r->ruleName);
                sep = ",";
            }
            tw.writeln("{}    >;", indent);
            tw.writeln();

            tw.writeln("{}    const {}& anchor;", indent, grammar.tokenClass);
            tw.writeln("{}    Rule rule;", indent);
            tw.writeln();

            tw.writeln("{}    inline void dump(std::ostream& ss, const size_t& lvl, const std::string& indent, const size_t& depth) const {{", indent);
            tw.writeln("{}        std::visit([this, &ss, &lvl, &indent, &depth](const auto& r){{", indent);
            tw.writeln("{}            r.dump(ss, lvl, pos, indent, depth);", indent);
            tw.writeln("{}        }}, rule);", indent);
            tw.writeln("{}    }}", indent);
            tw.writeln();

            tw.writeln("{}    explicit inline {}(const FilePos& p, const {}& a) : pos(p), anchor(a) {{}}", indent, rs->name, grammar.tokenClass);
            tw.writeln("{}}}; // struct {}", indent, rs->name);
            tw.writeln();
        }

        for (const auto& rs : grammar.ruleSets) {
            for (auto& r : rs->rules) {
                tw.writeln("{}void {}::{}::dump(std::ostream& ss, const size_t& lvl, const FilePos& p, const std::string& indent, const size_t& depth) const {{", indent, rs->name, r->ruleName);
                tw.writeln("{}    if(lvl >= 2) {{", indent);
                tw.writeln(R"({}        ss << std::format("{{}}: {{}}+--{}\n", p.str(), indent);)", indent, r->str(false));
                for (size_t idx = 0; idx < r->nodes.size(); ++idx) {
                    auto& n = r->nodes.at(idx);
                    auto varName = n->varName;
                    if (n->varName.size() == 0) {
                        varName = std::format("{}{}", n->name, idx);
                    }
                    if(n->isRule() == true) {
                        tw.writeln(R"({}        {}.dump(ss, lvl, indent + "|  ", depth + 1);)", indent, varName);
                    }else{
                        assert(n->isRegex() == true);
                        tw.writeln(R"({}        {}.dump(ss, lvl, "{}", indent + "|  ", depth + 1);)", indent, varName, n->name);
                    }
                }
                tw.writeln("{}    }}else{{", indent);
                tw.writeln("{}        assert(lvl == 1);", indent);
                tw.writeln(R"({}        ss << std::format("{{}}{{}}:{}(", indent, depth);)", indent, r->ruleName);
                auto ind = std::format("\"\"");
                for (size_t idx = 0; idx < r->nodes.size(); ++idx) {
                    auto& n = r->nodes.at(idx);
                    auto varName = n->varName;
                    if (n->varName.size() == 0) {
                        varName = std::format("{}{}", n->name, idx);
                    }
                    if(n->isRule() == true) {
                        tw.writeln("{}        {}.dump(ss, lvl, {}, depth + 1);", indent, varName, ind);
                    }else{
                        assert(n->isRegex() == true);
                        tw.writeln("{}        {}.dump(ss, lvl, \"{}\", {}, depth + 1);", indent, varName, n->name, ind);
                    }
                    ind = std::format("\" \"");
                }
                tw.writeln("{}        ss << std::format(\")\");", indent);
                tw.writeln("{}    }}", indent);
                tw.writeln("{}}}", indent);
                tw.writeln();
            }
        }

    }

    /// @brief generates a variant that contains all AST nodes
    inline void generateAstNodeItems(TextFileWriter& tw, const std::string_view& indent) {
        tw.writeln("{}using AstNode = std::variant<", indent);
        tw.writeln("{}    {}::{}", indent, qidNameAST, grammar.tokenClass);
        for (const auto& rs : grammar.ruleSets) {
            tw.writeln("{}    ,{}::{}", indent, qidNameAST, rs->name);
        }
        tw.writeln("{}>;", indent);
    }

    /// @brief generates visitor overload to invoke the corresponding member function
    inline void generateRuleVisitorBody(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const yg::Walker::FunctionSig& fsig,
        const ygp::RuleSet& rs,
        const ygp::Rule& r,
        const std::string& xparams,
        const std::string_view& indent
    ) {
        // handler function name
        auto hname = getFunctionName(r, fsig.func);

        std::string node_unused = "[[maybe_unused]]";

        // generate param list
        std::stringstream params;
        std::string sep;
        params << std::format("{}_n", sep);
        sep = ", ";
        for (const auto& n : r.nodes) {
            if (n->varName.size() > 0) {
                params << std::format("{}{}", sep, n->varName);
                node_unused.clear();
                sep = ", ";
            }
        }

        if(xparams.size() > 0) {
            params << std::format("{}{}", sep, xparams);
        }

        // open anonymous function
        tw.writeln("{}        [&]({}const {}::{}& _n) -> {} {{", indent, node_unused, rs.name, r.ruleName, fsig.type);
        tw.writeln("{}            NodeRefPostExec _nrpx;", indent);

        if(opts().enableWalkerLogging == true) {
            tw.writeln("{}    std::println(log(), \"{}:{}\");", indent, hname, r.str(false));
        }

        // generate node-refs
        for (const auto& n : r.nodes) {
            if(n->name == grammar.end) {
                continue;
            }

            auto autowalk = false;
            if(
                (n->isRule() == true) &&
                (walker.traversalMode == yg::Walker::TraversalMode::TopDown) &&
                (n->name != grammar.end) &&
                ((fsig.isUDF == false) || (fsig.func == walker.defaultFunctionName))
            ) {
                autowalk = true;
            }

            if (n->varName.size() > 0) {
                if(n->isRule() == true) {
                    tw.writeln("{}            NodeRef<{}> {}(_n.{}); /*var-4x*/", indent, n->name, n->varName, n->varName);
                    if(autowalk == true) {
                        tw.writeln("{}            _nrpx.add({}, [&](){{return {}({});}}); /*var-4x*/", indent, n->varName, fsig.func, n->varName);
                    }
                }else{
                    tw.writeln("{}            const {}& {} = _n.{}; /*var-2*/", indent, grammar.tokenClass, n->varName, n->varName);
                }
            }else{
                if(autowalk == true) {
                    tw.writeln("{}            NodeRef<{}> {}(_n.{}); /*var-4z*/", indent, n->name, n->idxName, n->idxName);
                    tw.writeln("{}            _nrpx.add({}, [&](){{return {}({});}}); /*var-4x*/", indent, n->idxName, fsig.func, n->idxName);
                }
            }
        }
        tw.writeln();

        // call the handler
        tw.writeln("{}            return {}({});/*call-1*/", indent, hname, params.str());


        // close anonymous function
        tw.writeln("{}        }},", indent);
        tw.writeln();
    }

    /// @brief generates writer for Walker
    static inline void
    generateWriter(
        TextFileWriter& tw,
        const yg::Walker& walker,
        const std::string_view& indent
    ) {
        if(walker.outputType == yg::Walker::OutputType::TextFile) {
            tw.writeln("{}//GEN_FILE", indent);
            tw.writeln("{}TextFileWriter {};", indent, walker.writerName);
            tw.writeln();

            tw.writeln("{}virtual void open(const std::filesystem::path& odir, const std::string_view& filename) override {{", indent);
            tw.writeln("{}    {}.open(odir, filename, \"{}\");", indent, walker.writerName, walker.ext);
            tw.writeln("{}}}", indent);
        }
    }

    /// @brief generates Walker Handlers
    inline void generateWalkerHandlers(
        const yg::Walker& walker,
        TextFileWriter& tw,
        const std::unordered_map<std::string, std::string>& vars,
        std::unordered_set<std::string>& xfuncs,
        const std::string_view& indent
    ) {
        tw.writeln("{}// handlers for {}", indent, walker.name);

        generateCodeBlock(tw, walker.xmembers, indent, true, vars);

        for(const auto& prs : grammar.ruleSets) {
            auto& rs = *prs;
            auto funcl = walker.getFunctions(rs);

            // TextFileIndenter twi(tw);
            for(auto& pfsig : funcl) {
                const auto& fsig = *pfsig;
                for(auto& pr : rs.rules) {
                    auto& r = *pr;
                    const auto* ci = walker.hasCodeblock(r, fsig.func);
                    if(ci == nullptr) {
                        if((grammar.isDerivedWalker(walker) == true)) {
                            continue;
                        }
                    }

                    auto hname = getFunctionName(r, fsig.func);
                    if(xfuncs.contains(hname) == true) {
                        continue;
                    }
                    xfuncs.insert(hname);

                    auto args = getArgs(r, fsig, ci);

                    if((opts().genLines == true) && (ci != nullptr) && (ci->codeblock.hasPos == true)) {
                        // write an extra #line before the function body
                        // so that 'unused_parameter' errors point to the right location
                        // in the .y file.
                        auto row = ci->codeblock.pos.row;
                        if(row > 2) {
                            row -= 2;
                        }
                        tw.writeln("#line {} \"{}\"", row, ci->codeblock.pos.file);
                    }

                    tw.writeln("{}virtual {}", indent, fsig.type);
                    tw.writeln("{}{} ({}) override {{", indent, hname, args);

                    // generate function body, if any
                    if(ci != nullptr) {
                        TextFileIndenter twi(tw);
                        generateCodeBlock(tw, ci->codeblock, indent, true, vars);
                    }
                    tw.writeln("{}}}", indent);
                    tw.writeln();
                }
            }
        }
        if(walker.base != nullptr) {
            generateWalkerHandlers(*(walker.base), tw, vars, xfuncs, indent);
        }
    }

    /// @brief generates Walker interface
    inline void generateWalkerInterface(
        const yg::Walker& walker,
        TextFileWriter& tw,
        const std::string_view& wname,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        std::string bname;
        if(grammar.isRootWalker(walker) == true) {
            tw.writeln("{}struct {} {{", indent, wname);

            // generate using-decls for AST nodes
            if(auto twi = TextFileIndenter(tw)) {
                tw.writeln("{}using {} = {}::{};", indent, grammar.tokenClass, qidNameAST, grammar.tokenClass);
                for(const auto& prs : grammar.ruleSets) {
                    auto& rs = *prs;
                    tw.writeln("{}using {} = {}::{};", indent, rs.name, qidNameAST, rs.name);
                }
            }
            tw.writeln("{}    {}& ymodule;", indent, grammar.className);
            bname = std::format("ymodule");
        }else{
            assert(walker.base != nullptr);
            bname = std::format("{}", walker.base->name);
            tw.writeln("{}struct {} : public {} {{", indent, wname, bname);
        }

        if(auto twi = TextFileIndenter(tw)) {
            unused(vars);
            tw.writeln("{}inline {}({}& m) : {}(m) {{}}", indent, wname, grammar.className, bname);
            tw.writeln();

            if(grammar.isRootWalker(walker) == true) {
                tw.writeln("{}virtual ~{}() {{}}", indent, wname);
            }else{
                tw.writeln("{}virtual ~{}() override {{}}", indent, wname);
            }
            tw.writeln();

            tw.writeln("{}void walk({}& m);", indent, grammar.className);

            tw.writeln("{}template<typename T>", indent);
            tw.writeln("{}using NodeRef = {}::NodeRef<T>;", indent, qidNameAST);
            tw.writeln();
            tw.writeln("{}using NodeRefPostExec = {}::NodeRefPostExec;", indent, qidNameAST);
            tw.writeln();

            if((walker.interfaceName.size() > 0) && (opts().amalgamatedFile == true)) {
                tw.writeln("{}static std::unique_ptr<{}> create({}& m);", indent, wname, grammar.className);
            }

            if(walker.outputType == yg::Walker::OutputType::TextFile) {
                tw.writeln("{}virtual void open(const std::filesystem::path& odir, const std::string_view& filename) = 0;", indent);
            }

            if(grammar.isRootWalker(walker) == true) {
                tw.writeln("{}template<typename T>", indent);
                tw.writeln("{}void skip(NodeRef<T>& nr);", indent);
                tw.writeln();
            }

            tw.writeln("{}template<typename T>", indent);
            tw.writeln("{}auto pos(NodeRef<T>& nr) -> FilePos {{", indent);
            tw.writeln("{}    return nr.node.anchor.pos;", indent);
            tw.writeln("{}}}", indent);
            tw.writeln();

            for(const auto& prs : grammar.ruleSets) {
                auto& rs = *prs;
                auto funcl = walker.getFunctions(rs);
    
                for(auto& pfsig : funcl) {
                    const auto& fsig = *pfsig;
                    for(auto& pr : rs.rules) {
                        auto& r = *pr;
                        const auto* ci = walker.hasCodeblock(r, fsig.func);
                        if((grammar.isDerivedWalker(walker) == true) && (ci == nullptr)) {
                            continue;
                        }
                        auto hname = getFunctionName(r, fsig.func);
    
                        auto args = getArgs(r, fsig, ci);

                        std::string isOverride;
                        if((grammar.isDerivedWalker(walker) == true)) {
                            if(fsig.func == walker.defaultFunctionName) {
                                isOverride = "override ";
                            }
                        }
                        tw.writeln("{}virtual {}", indent, fsig.type);
                        tw.writeln("{}{}({}) {}= 0;", indent, hname, args, isOverride);
    
                        tw.writeln();
                    }
                }
            }

            for(const auto& prs : grammar.ruleSets) {
                auto& rs = *prs;
                auto funcl = walker.getFunctions(rs);
    
                for(auto& pfsig : funcl) {
                    const auto& fsig = *pfsig;
                    if((grammar.isDerivedWalker(walker) == true) && (fsig.func == walker.defaultFunctionName)) {
                        continue;
                    }
                    std::string args;
                    if(fsig.args.size() > 0) {
                        args = std::format(", {}", fsig.args);
                    }
                    tw.writeln("{}auto {}(NodeRef<{}>& node{}) -> {};", indent, fsig.func, rs.name, args, fsig.type);
                }
            }
        }

        tw.writeln("{}}};", indent);
        tw.writeln();
    }

    /// @brief generates Walker interface
    inline void generateWalkerInterfaceExternal(
        const yg::Walker& walker,
        TextFileWriter& tw,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        tw.writeln("{}#pragma once", indent);
        tw.writeln(R"({}#include "{}_astnodes.hpp")", indent, grammar.className);
        tw.writeln();
        generateWalkerInterface(walker, tw, walker.interfaceName, vars, indent);
    }

    /// @brief generates all Walker's
    inline void generateWalkers(
        TextFileWriter& tw,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string_view& indent
    ) {
        // generate Walker
        for (const auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            auto wname = std::format("{}", walker.name);

            // generate Walker interfaces
            if(walker.interfaceName.size() > 0) {
                TextFileWriter twi;
                twi.open(walker.interfaceName + ".hpp");
                if (twi.isOpen() == false) {
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_INTERFACE:{}", walker.interfaceName);
                }
                generateWalkerInterfaceExternal(walker, twi, vars, "");
                wname = walker.interfaceName;
            }else{
                generateWalkerInterface(walker, tw, wname, vars, indent);
            }

            if(grammar.isRootWalker(walker) == true) {
                // generate skip implementation
                tw.writeln("{}template<typename T>", indent);
                tw.writeln("{}void {}::skip(NodeRef<T>& nr) {{", indent, wname);
                tw.writeln("{}    nr.called = true;", indent);
                tw.writeln("{}}}", indent);
            }

            for(const auto& prs : grammar.ruleSets) {
                auto& rs = *prs;
                auto funcl = walker.getFunctions(rs);

                for(auto& pfsig : funcl) {
                    const auto& fsig = *pfsig;
                    if((grammar.isDerivedWalker(walker) == true) && (fsig.func == walker.defaultFunctionName)) {
                        continue;
                    }

                    std::string args;
                    if(fsig.args.size() > 0) {
                        args = std::format(", {}", fsig.args);
                    }

                    tw.writeln("{}auto {}::{}(NodeRef<{}>& node{}) -> {} {{", indent, wname, fsig.func, rs.name, args, fsig.type);
                    if((fsig.isUDF == false) || fsig.func == walker.defaultFunctionName) {
                        tw.writeln("{}    WalkerNodeCommit<{}, {}> _wc(node);", indent, wname, rs.name);
                    }
            
                    auto xparams = extractParams(fsig.args);
                        
                    // generate Visitor
                    tw.writeln("{}    return std::visit(overload{{", indent);
            
                    if(true) {
                        tw.writeln("{}        [&](const _astEmpty&) -> {} {{", indent, fsig.type);
                        tw.writeln("{}            throw std::runtime_error(\"internal_error\"); //should never reach here", indent);
                        tw.writeln("{}        }},", indent);
                        tw.writeln();
                    }
            
                    // generate handlers for all rules in ruleset
                    for (const auto& r : rs.rules) {
                        generateRuleVisitorBody(tw, walker, fsig, rs, *r, xparams, indent);
                    }
                    tw.writeln("{}    }}, node.node.rule);", indent);
                    tw.writeln("{}}}", indent);
                    tw.writeln();
                }
            }


            // generate Walker implementation
            if(walker.interfaceName.size() == 0) {
                std::string xargs;
                if(walker.xctor_args.size() > 0) {
                    xargs = std::format(", {}", walker.xctor_args);;
                }
                tw.writeln("{}struct Walker_{} : public {} {{", indent, walker.name, wname);
                if(walker.xctor.hasCode() == true) {
                    tw.writeln("{}    inline Walker_{}({}& m{}) : {}(m) {{", indent, walker.name, grammar.className, xargs, wname);
                    if(auto twi = TextFileIndenter(tw)) {
                        generateCodeBlock(tw, walker.xctor, indent, true, vars);
                    }
                    tw.writeln("{}}}", indent);
                }else{
                    tw.writeln("{}    inline Walker_{}({}& m{}) : {}(m) {{}}", indent, walker.name, grammar.className, xargs, wname);
                }
                if(auto twi = TextFileIndenter(tw)) {
                    generateWriter(tw, walker, indent);
                }
                tw.writeln();

                std::unordered_set<std::string> xfuncs;
                if(auto twi = TextFileIndenter(tw)) {
                    generateWalkerHandlers(walker, tw, vars, xfuncs, indent);
                }
                tw.writeln("{}}};", indent);
                tw.writeln();
            }
        }
    }

    /// @brief generates code to add default Walker, if none specified
    inline void generateInitWalkers(TextFileWriter& tw, const std::string_view& indent) {
        for (const auto& pwalker : grammar.walkers) {
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
        if(opts().amalgamatedFile == false) {
            return;
        }

        for (const auto& pwalker : grammar.walkers) {
            auto& walker = *pwalker;
            if(grammar.isBaseWalker(walker) == true) {
                continue;
            }

            tw.writeln("{}else if(w == \"{}\") {{", indent, walker.name);
            if(walker.interfaceName.size() > 0) {
                tw.writeln("{}    auto pwalker = {}::create(ymodule);", indent, walker.interfaceName);
                tw.writeln("{}    auto& walker = *pwalker;", indent);
                tw.writeln("{}    ymodule.walk_{}(walker);", indent, walker.name);
            }else{
                if(walker.outputType == yg::Walker::OutputType::TextFile) {
                    tw.writeln("{}    ymodule.walk_{}(odir, filename);", indent, walker.name);
                }else{
                    tw.writeln("{}    ymodule.walk_{}();", indent, walker.name);
                }
            }
            tw.writeln("{}}}", indent);
        }
    }

    /// @brief generates all Token IDs inside an enum in the prototype file
    static inline void
    generateTokenIDs(
        TextFileWriter& tw,
        const std::unordered_set<std::string>& tnames
    ) {
        for (const auto& t : tnames) {
            tw.writeln("        {},", t);
        }
    }

    /// @brief generates the string names for all Token IDs inside a map in the prototype file
    static inline void
    generateTokenIDNames(
        TextFileWriter& tw,
        const std::unordered_set<std::string>& tnames
    ) {
        for (const auto& t : tnames) {
            tw.writeln("            {{ID::{}, \"{}\"}},", t, t);
        }
    }

    /// @brief generates function declarations to create each AST node
    inline void generateCreateASTNodesDecls(TextFileWriter& tw) {
        for (const auto& rs : grammar.ruleSets) {
            tw.writeln("template<> inline {}::{}&", qidNameAST, rs->name);
            tw.writeln("Parser::create<{}::{}>(const ValueItem& vi);", qidNameAST, rs->name);
            tw.writeln();
        }
    }

    /// @brief generates functions definitions to create each AST node
    /// These functions are called by the Parser on REDUCE actions
    inline void generateCreateASTNodesDefns(TextFileWriter& tw, const std::unordered_map<std::string, std::string>& vars) {
        for (const auto& rs : grammar.ruleSets) {
            tw.writeln("template<> inline {}::{}&", qidNameAST, rs->name);
            tw.writeln("Parser::create<{}::{}>(const ValueItem& vi) {{", qidNameAST, rs->name);
            tw.writeln("    switch(vi.ruleID) {{");

            for (auto& r : rs->rules) {
                tw.writeln("    case {}: {{", r->id);
                tw.writeln("        //{}", r->str(false));
                if(r->id > 0) {
                    tw.writeln("        assert(vi.childs.size() == {});", r->nodes.size());
                }else{
                    assert(rs->hasEpsilon == true);
                }

                // generate parameter variables
                std::ostringstream ss;
                std::string sep;
                std::string hasAnchor;
                if(r->id > 0) {
                    for (size_t idx = 0; idx < r->nodes.size(); ++idx) {
                        auto& n = r->nodes.at(idx);
                        auto varName = n->varName;
                        if (n->varName.size() == 0) {
                            varName = std::format("{}{}", n->name, idx);
                        }

                        auto rt = getNodeType(*n);
                        tw.writeln("        auto& _cv_{} = *(vi.childs.at({}));", varName, idx);
                        tw.writeln("        auto& p{} = create<{}::{}>(_cv_{});", varName, qidNameAST, rt, varName);
                        if (idx == r->anchor) {
                            tw.writeln("        auto& anchor = create<{}::{}>(_cv_{});", qidNameAST, grammar.tokenClass, varName);
                            hasAnchor = ", anchor";
                        }
                        ss << std::format("{}p{}", sep, varName);
                        sep = ", ";
                    }
                }else{
                    assert(rs->hasEpsilon == true);
                    tw.writeln("        auto& p{}0 = create<{}::{}>(vi);", grammar.empty, qidNameAST, grammar.tokenClass);
                    tw.writeln("        auto& anchor = p{}0;", grammar.empty);
                    hasAnchor = ", anchor";
                    ss << std::format("{}p{}0", sep, grammar.empty);
                    sep = ", ";
                }

                tw.writeln("        auto& cel = ast.createAstNode<{}::{}>(vi.token.pos{});", qidNameAST, rs->name, hasAnchor);
                tw.writeln("        cel.rule.emplace<{}::{}::{}>({});", qidNameAST, rs->name, r->ruleName, ss.str());
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
        for (const auto& ps : grammar.itemSets) {
            auto& itemSet = *ps;
            assert((itemSet.shifts.size() > 0) || (itemSet.reduces.size() > 0) || (itemSet.gotos.size() > 0));
            bool breaked = false;
            std::stringstream xss;
            std::string xsep;

            tw.writeln("            case {}:", itemSet.id);
            if(opts().enableParserLogging == true) {
                tw.writeln(R"(                std::print(log(), "{{}}", "{}\n");)", itemSet.str("", R"(\n)", true));
            }
            tw.writeln("                switch(k.id) {{");
            for (auto& c : itemSet.shifts) {
                for(const auto& fb : c.first->fallbacks) {
                    if ((itemSet.hasShift(*fb) != nullptr) || (itemSet.hasReduce(*fb) != nullptr)) {
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
                if(opts().enableParserLogging == true) {
                    tw.writeln(R"(                    std::print(log(), "SHIFT {}: t={}\n");)", c.second.next->id, c.first->name);
                }
                tw.writeln("                    shift(k);");
                tw.writeln("                    stateStack.push_back({});", c.second.next->id);
                tw.writeln("                    return accepted;");
                xss << xsep << c.first->name;
                xsep = ", ";
            }
            for (auto& rd : itemSet.reduces) {
                const auto& c = *(rd.second.next);
                const auto& r = c.rule;
                tw.writeln("                case Tolkien::ID::{}: // REDUCE", rd.first->name);
                if(opts().enableParserLogging == true) {
                    tw.writeln(R"(                    std::print(log(), "REDUCE:{}:{{}}/{}\n", "{}");)", rd.first->name, r.nodes.size(), c.str());
                }
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
                xss << xsep << rd.first->name;
                xsep = ", ";
            }
            for (auto& c : itemSet.gotos) {
                tw.writeln("                case Tolkien::ID::{}: // GOTO", c.first->name);
                if(opts().enableParserLogging == true) {
                    tw.writeln(R"(                    std::print(log(), "GOTO {}:id={}, rule={}\n");)", c.second->id, c.first->id, c.first->name);
                }
                tw.writeln("                    stateStack.push_back({});", c.second->id);
                tw.writeln("                    k = k0;");
                tw.writeln("                    break;");
                breaked = true;
            }
            tw.writeln("                default:");
            auto msg = std::format(R"("SYNTAX_ERROR:received:" + k.str() + ", expected:{}")", xss.str());
            generateError(tw, "k.pos.row", "k.pos.col", "k.pos.file", msg, "                    ", vars);
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

            inline void operator()([[maybe_unused]] const yglx::WildCard& t) {
                assert(tset.wildcard == nullptr);
                tset.wildcard = &tx;
            }

            inline void operator()(const yglx::LargeEscClass& t) {
                tset.largeEscClasses.emplace_back(&tx, &t);
            }

            inline void operator()(const yglx::RangeClass& t) {
                bool isSmallRange = ((t.ch2 - t.ch1) <= grammar.smallRangeSize);
                if (isSmallRange) {
                    tset.smallRanges.emplace_back(&tx, &t);
                }else{
                    tset.largeRanges.emplace_back(&tx, &t);
                }
            }

            inline void operator()(const yglx::PrimitiveTransition& t) {
                std::visit(*this, t.atom.atom);
            }

            inline void operator()(const yglx::ClassTransition& t) {
                tset.classes.emplace_back(&tx, &t);
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
            for (const auto& tx : txs) {
                Visitor v(g, *this, *tx);
                std::visit(v, tx->t);
            }
        }
    };

    /// @brief generate code to transition from one Lexer state to another
    static inline void
    generateStateChange(
        TextFileWriter& tw,
        const yglx::Transition& t,
        const yglx::State* nextState,
        const std::string& indent
    ) {
        if (t.capture) {
            tw.writeln("                {}token.addText(ch);", indent);
        }
        tw.writeln("                {}stream.consume();", indent);
        tw.writeln("                {}state = {};", indent, nextState->id);
    }

    /// @brief generate Lexer states
    inline void generateLexerStates(TextFileWriter& tw, const std::unordered_map<std::string, std::string>& vars) {
        tw.writeln("            case 0:");
        generateError(tw, "stream.pos.row", "stream.pos.col", "stream.pos.file", "\"LEXER_INTERNAL_ERROR\"", "                ", vars);

        for (const auto& ps : grammar.states) {
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

                const auto& tx = *(tset.inLoop.second);
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
                for(const auto& ax : t.second->atom.atoms) {
                    std::visit(overload{
                        [&negate, &ss, &sep](const yglx::WildCard&) -> void {
                            ss << std::format("{}({}true)", sep, negate);
                        },
                        [&negate, &ss, &sep](const yglx::LargeEscClass& a) -> void {
                            ss << std::format("{}({}{}(ch))", sep, negate, a.checker);
                        },
                        [&negate, &ss, &sep](const yglx::RangeClass& a) -> void {
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
            }else if (state.matchedRegex != nullptr) {
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
                generateError(tw, "stream.pos.row", "stream.pos.col", "stream.pos.file", "std::format(\"TOKEN_ERROR:{}\", token.text)", "                ", vars);
            }
        }
    }

    /// @brief main parser generator
    /// This function reads the prototype.cpp file (embedded as a raw string)
    /// and acts on each meta command found in the file.
    inline void
    includeCodeBlock(
        const char* codeBlock,
        TextFileWriter& tw,
        const std::unordered_map<std::string, std::string>& vars,
        const std::unordered_set<std::string>& tnames,
        const std::filesystem::path& filebase,
        const std::string& srcName,
        const std::string_view& outerIndent
    ) {
        static const std::string_view prefixEnterBlock = "///PROTOTYPE_ENTER:";
        static const std::string_view prefixLeaveBlock = "///PROTOTYPE_LEAVE:";
        static const std::string_view prefixSegment = "///PROTOTYPE_SEGMENT:";
        static const std::string_view includeSegment = "///PROTOTYPE_INCLUDE:";
        static const std::string_view prefixTarget = "///PROTOTYPE_TARGET:";

        static std::unordered_set<std::string_view> dontPrintBlocks = {
            "SKIP",
            "IF_HAS_NS",
            "IF_HAS_LEXER",
            "IF_LOG_PARSER"
        };

        enum class Token : uint8_t {
            EnterBlock,
            LeaveBlock,
            Line,
            Segment,
            Include,
            Target,
        };

        enum class TokenState : uint8_t {
            InitWS,
            Slash1,
            Slash2,
            Line,
        };

        std::string tBlock;

        const std::string_view cb(codeBlock);
        auto it = cb.begin(); // NOLINT(readability-qualified-auto)
        auto ite = cb.end(); // NOLINT(readability-qualified-auto)

        // skip initial empty lines, if any
        while ((it != ite) && ((*it == '\r') || (*it == '\n'))) {
#if defined(__clang__)
#pragma clang unsafe_buffer_usage begin
#endif
            ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if defined(__clang__)
#pragma clang unsafe_buffer_usage end
#endif
        }

        bool skip = false;
        bool capturing = false;
        std::vector<std::string_view> eblockNames;

        while (it != ite) {
            auto itb = it; // NOLINT(readability-qualified-auto)
            auto itl = it; // NOLINT(readability-qualified-auto)
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
                ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                #if defined(__clang__)
                #pragma clang unsafe_buffer_usage end
                #endif
            }

            std::string_view line(itb, it);
            std::string_view innerIndent(itb, itl);
            std::string_view tline(itl, it);

            Token token = Token::Line;
            std::string_view eblockName;
            std::string_view lblockName;
            std::string_view segmentName;
            std::string_view includeName;
            std::string_view targetName;

            if(capturing == false) {
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
            }else{
                // if we are in a capture block,
                // check if we are leaving the capture block
                // if not, add the line to the capture block
                if(tline.starts_with(prefixLeaveBlock)) {
                    lblockName = tline.substr(prefixLeaveBlock.size());
                    if(eblockNames.size() == 0) {
                        throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "PROTOTYPE_NO_EBLOCK_ERROR:{}", lblockName);
                    }
                    eblockName = eblockNames.back();
                    if(eblockName == lblockName) {
                        token = Token::LeaveBlock;
                        eblockNames.pop_back();
                    }else{
                        if(eblockNames.size() > 0) {
                            eblockName = eblockNames.back();
                        }
                        token = Token::Line;
                    }
                }else {
                    if(eblockNames.size() > 0) {
                        eblockName = eblockNames.back();
                    }
                    token = Token::Line;
                }
            }


// #define PRINT(...) std::println(__VA_ARGS__)
#define PRINT(...)

            auto indent = std::string(outerIndent) + std::string(innerIndent);

            switch (token) {
            case Token::EnterBlock: {
                if(opts().enableGeneratorLogging == true) {
                    log("Line::EB:{}, eb={}, lb={}, skip={}", line, eblockName, lblockName, skip);
                }
                if (dontPrintBlocks.contains(eblockName) == false) {
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
                    if (opts().amalgamatedFile == true) {
                        tw.writeln("#define HAS_REPL {}", (grammar.hasREPL == true)?1:0);
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "fmain") {
                    if (opts().amalgamatedFile == true) {
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

                if (eblockName == "IF_HAS_NS") {
                    if(grammar.ns.size() > 0) {
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "IF_LOG_LEXER") {
                    if(opts().enableLexerLogging == true) {
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "IF_LOG_PARSER") {
                    if(opts().enableParserLogging == true) {
                        skip = false;
                    }else{
                        skip = true;
                    }
                    break;
                }

                if (eblockName == "throwError") {
                    tBlock.clear();
                    assert(capturing == false);
                    capturing = true;
                    skip = true;
                    break;
                }

                if (eblockName == "astNodeDeclsBlock") {
                    tBlock.clear();
                    assert(capturing == false);
                    capturing = true;
                    skip = true;
                    break;
                }

                throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "PROTOTYPE_EBLOCK_ERROR:{}", eblockName);
            }
            case Token::LeaveBlock: {
                if(opts().enableGeneratorLogging == true) {
                    log("Line::LB:{}, eb={}, lb={}", line, eblockName, lblockName);
                }
                if (dontPrintBlocks.contains(eblockName) == false) {
                    tw.writeln("{}", line);
                }
                assert(lblockName == eblockName);
                skip = false;
                if (eblockName == "throwError") {
                    assert(capturing == true);
                    if (throwError.code.empty()) {
                        throwError.setCode(tBlock);
                    }
                    capturing = false;
                    break;
                }
                if (eblockName == "astNodeDeclsBlock") {
                    assert(capturing == true);
                    cb_astNodeDecls.setCode(tBlock);
                    capturing = false;
                    break;
                }
                break;
            }
            case Token::Segment: {
                if(opts().enableGeneratorLogging == true) {
                    log("Line::SEGMENT:{}", line);
                }
                tw.writeln("{}:BEGIN //{}", line, tw.row);
                if (segmentName == "pchHeader") {
                    generatePchHeader(tw, indent);
                }else if (segmentName == "hdrHeaders") {
                    generateHdrHeaders(tw, indent);
                }else if (segmentName == "srcHeaders") {
                    generateSrcHeaders(tw, indent);
                }else if (segmentName == "classMembers") {
                    generateClassMembers(tw, indent);
                }else if (segmentName == "walkerCallDecls") {
                    generateWalkerCallDecls(tw, indent);
                }else if (segmentName == "walkerCallDefns") {
                    generateWalkerCallDefns(tw, indent);
                }else if (segmentName == "walkerCallImpls") {
                    generateWalkerCallImpls(tw, indent);
                }else if (segmentName == "astNodeDecls") {
                    generateAstNodeDecls(tw, indent);
                }else if (segmentName == "astNodeDefns") {
                    generateAstNodeDefns(tw, indent);
                }else if (segmentName == "astNodeItems") {
                    generateAstNodeItems(tw, indent);
                }else if (segmentName == "walkers") {
                    generateWalkers(tw, vars, indent);
                }else if (segmentName == "prologue") {
                    generateCodeBlock(tw, grammar.prologue, indent, true, vars);
                }else if (segmentName == "initWalkers") {
                    if(opts().amalgamatedFile == true) {
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
                if(opts().enableGeneratorLogging == true) {
                    log("Line::INCLUDE:{}", line);
                }
                tw.writeln("{}:BEGIN", line);
                if (includeName == "utf8Encoding") {
                    if (grammar.unicodeEnabled == true) {
                        includeCodeBlock(cb_encoding_utf8, tw, vars, tnames, filebase, srcName, indent);
                    }
                }else if (includeName == "asciiEncoding") {
                    if (grammar.unicodeEnabled == false) {
                        includeCodeBlock(cb_encoding_ascii, tw, vars, tnames, filebase, srcName, indent);
                    }
                }else if (includeName == "stream") {
                    includeCodeBlock(cb_stream, tw, vars, tnames, filebase, srcName, indent);
                }else if (includeName == "textWriter") {
                    includeCodeBlock(cb_text_writer, tw, vars, tnames, filebase, srcName, indent);
                }else if (includeName == "print") {
                    includeCodeBlock(cb_print, tw, vars, tnames, filebase, srcName, indent);
                }else if (includeName == "nsutil") {
                    includeCodeBlock(cb_nsutil, tw, vars, tnames, filebase, srcName, indent);
                }else if (includeName == "filepos") {
                    includeCodeBlock(cb_filepos, tw, vars, tnames, filebase, srcName, indent);
                }else if (includeName == "astNodeDeclsBlock") {
                    TextFileWriter* ptw = &tw;
                    TextFileWriter xtw;
                    for(const auto& pw : grammar.walkers) {
                        auto& w = *pw;
                        if(w.interfaceName.size() > 0) {
                            auto astFile = std::format("{}_astnodes.hpp", grammar.className);
                            xtw.open(astFile);
                            if(xtw.isOpen() == false) {
                                throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_AST:{}", astFile);
                            }
                            ptw = &xtw;
                            break;
                        }
                    }
            
                    TextFileWriter& ntw = *ptw;
                    includeCodeBlock(cb_astNodeDecls.code.c_str(), ntw, vars, tnames, filebase, srcName, indent);
                }else{
                    throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "UNKNOWN_INCLUDE:{}", includeName);
                }
                tw.writeln("{}:END", line);
                break;
            }
            case Token::Target: {
                if(opts().enableGeneratorLogging == true) {
                    log("Line::TARGET:{}", line);
                }
                if ((opts().amalgamatedFile == false) && (targetName == "SOURCE")) {
                    auto nsrcName = filebase.string() + ".cpp";
                    tw.open(nsrcName);
                    if (tw.isOpen() == false) {
                        throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_SRC:{}", nsrcName);
                    }
                    tw.writeln("#include \"{}\"", std::filesystem::path(srcName).filename().string());
                }
                break;
            }
            case Token::Line: {
                if(opts().enableGeneratorLogging == true) {
                    log("Init::LINE:{}, eblockName=[{}], skip={}", line, eblockName, skip);
                }
                if (skip == false) {
                    generatePrototypeLine(tw, tline, vars, indent);
                }else if((eblockName == "astNodeDeclsBlock") || (eblockName == "throwError")) {
                    tBlock += "\n";
                    tBlock += line;
                }
                break;
            }
            }

            if(it == ite) {
                continue;
            }

            #if defined(__clang__)
            #pragma clang unsafe_buffer_usage begin
            #endif
            ++it; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
        if(opts().amalgamatedFile == true) {
            srcName = filebase.string() + ".cpp";
        }
        TextFileWriter tw;
        tw.open(srcName);
        if (tw.isOpen() == false) {
            throw GeneratorError(__LINE__, __FILE__, grammar.pos(), "ERROR_OPENING_SRC:{}", srcName);
        }
        if(opts().amalgamatedFile == false) {
            tw.writeln("#pragma once");
        }

        // token IDs
        std::unordered_set<std::string> tnames;
        for (const auto& t : grammar.regexes) {
            if (tnames.contains(t->regexName)) {
                continue;
            }
            tnames.insert(t->regexName);
        }
        for (const auto& t : grammar.ruleSets) {
            if (tnames.contains(t->name)) {
                assert(false);
                continue;
            }
            tnames.insert(t->name);
        }

        if (grammar.ns.empty() == false) {
            qidNamespace = std::format("{}::", grammar.ns);
        }

        assert(grammar.className.empty() == false);
        qidClassName = std::format("{}{}", qidNamespace, grammar.className);
        qidNameAST = std::format("{}_AST", qidClassName);

        const std::unordered_map<std::string, std::string> vars = {
            {"NSNAME", grammar.ns},
            {"Q_NSNAME", qidNamespace},
            {"CLSNAME", grammar.className},
            {"Q_CLSNAME", qidClassName},
            {"Q_ASTNS", qidNameAST},
            {"TOKEN", grammar.tokenClass},
            {"WALKER", grammar.getDefaultWalker().name},
            {"START_RULE", std::format("{}", grammar.start)},
            {"START_RULE_NAME", std::format("\"{}\"", grammar.start)},
            {"MAX_REPEAT_COUNT", std::to_string(grammar.maxRepCount)},
            {"AST", grammar.astClass},
        };

        includeCodeBlock(cb_prototype, tw, vars, tnames, filebase, srcName, "");
    }
};
}

void generateGrammar(const yg::Grammar& g, const std::filesystem::path& of) {
    Generator gen(g);
    gen.generate(of);
}
