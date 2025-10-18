#include "pch.hpp"
#include "grammar_yg.hpp"
#include "parser.hpp"
#include "logger.hpp"
#include "lexer_builder.hpp"
#include "parser_builder.hpp"
#include "cpp_generator.hpp"
#include "tx_table.hpp"
#include "grammar_printer.hpp"
#include "config.hpp"

/// @brief filename where the AST is logged in markdown format
static std::string gfilename;

bool amalgamatedFile = false;
static bool verbose = false;

extern bool genLines;
bool genLines = true;

namespace {
using LexerTable = Table<const yglx::State*, std::string, std::string>;
inline void generateLexerTable(std::ostream& os, const yg::Grammar& g) {
    LexerTable pt;
    for(auto& t : g.transitions) {
        pt.addHeader(std::format("{}", t->str()));
    }

    for(auto& s : g.states) {
        auto& row = pt.addRow(s.get(), std::format("{:>3}", s->id));
        for(auto& t : s->transitions) {
            row.addCell(pt, std::format("{}", t->str()), std::format("{}", zid(t->next)));
        }
    }
    auto s = pt.genMD();
    std::println(os, "{}", s);
}

using ParserTable = Table<const ygp::ItemSet*, std::string, std::string>;
inline void generateParserTable(std::ostream& os, const yg::Grammar& g) {
    ParserTable pt;
    for(auto& rx : g.regexSets) {
        if(rx->usageCount() == 0) {
            continue;
        }
        pt.addHeader(std::format("{}", rx->name));
    }
    for(auto& r : g.ruleSets) {
        pt.addHeader(std::format("{}", r->name));
    }

    for(auto& is : g.itemSets) {
        auto& row = pt.addRow(is.get(), std::format("{:>3}", is->id));
        for(auto& t : is->shifts) {
            row.addCell(pt, std::format("{}", t.first->name), std::format("S{}", t.second.next->id));
        }
        for(auto& t : is->gotos) {
            row.addCell(pt, std::format("{}", t.first->name), std::format("G{}", t.second->id));
        }
        for(auto& t : is->reduces) {
            row.addCell(pt, std::format("{}", t.first->name), std::format("R:{}", t.second.next->rule.ruleSetName()));
        }
    }
    auto s = pt.genMD();
    std::println(os, "{}", s);
}

inline void generateRuleSetAST(
    std::ostream& os,
    const yg::Grammar& g,
    const ygp::RuleSet& rs,
    const std::string& indent,
    std::unordered_set<std::string>& seen,
    const std::string& path,
    const std::string& nodeName
) {
    for(auto& r : rs.rules) {
        std::println(os, "{}R:{}", indent, r->str(false));
        auto xindent = indent + "|---";
        for(auto& n : r->nodes) {
            if(n->isRule() == true) {
                if(n->varName.size() == 0) {
                    continue;
                }

                auto& crs = g.getRuleSetByName(n->pos, n->name);
                if(seen.contains(crs.name) == false) {
                    seen.insert(crs.name);
                    std::string part;
                    if(nodeName != "/") {
                        part = std::format("{}/{}({})", path, nodeName, r->ruleName);
                    }else{
                        part = nodeName;
                    }
                    std::println(os, "{}RS:{}: {}", xindent, crs.name, part);
                    generateRuleSetAST(os, g, crs, indent + "|    ", seen, part, n->varName);
                }
            }else{
                assert(n->isRegex() == true);
                std::string part;
                if (n->varName.size() > 0) {
                    part = std::format("({}): {}/{}", n->varName, path, n->varName);
                }
                std::println(os, "{}T:{}{}", xindent, n->name, part);
            }
        }
    }
}

inline void generateAbSynTree(std::ostream& os, const yg::Grammar& g) {
    std::unordered_set<std::string> seen;
    auto& rs = g.getRuleSetByName(g.pos(), g.start);
    std::println(os, "AST_TREE");
    generateRuleSetAST(os, g, rs, "", seen, "", "/");
}

inline void processInputEx(
    std::istream& is,
    const std::string& filename,
    const std::string& charset,
    const std::filesystem::path& odir,
    const std::string& oname
) {
    Stream stream(is, filename);

    yg::Grammar g;

    if(charset == "utf8") {
        g.unicodeEnabled = true;
    }else if(charset == "ascii") {
        g.unicodeEnabled = false;
    }

    // parse input file
    if(verbose == true) {
        std::println("Parsing");
    }
    parseInput(g, stream);

    if(verbose == true) {
        std::println("Processing");
    }
    buildLexer(g);
    buildParser(g);

    generateLexerTable(Logger::olog(), g);
    generateParserTable(Logger::olog(), g);
    generateAbSynTree(Logger::olog(), g);

    std::filesystem::path d(odir);
    auto f = d / oname;

    printGrammar(g, gfilename);

    if(verbose == true) {
        std::println("Generating: {}", f.string());
    }
    generateGrammar(g, f);
}

inline int
processInput(std::istream& is, const std::string& filename, const std::string& charset, const std::filesystem::path& odir, const std::string& oname) {
    try {
        processInputEx(is, filename, charset, odir, oname);
        return 0;
    }catch(const GeneratorError& e) {
        std::println("{}:{}:{}: error: {} ({}:{})", e.pos.file, e.pos.row, e.pos.col, e.msg, e.file, e.line);
    }catch(const std::exception& e) {
        std::println("{}: error: {}", filename, e.what());
    }catch(...) {
        std::println("{}: unknown error", filename);
    }
    return 1;
}

inline int help(const std::string& xname, const std::string& msg) {
    auto xxname = std::filesystem::path(xname).filename();
    std::println("== {} ==", msg);
    std::println("{} -c <utf8|ascii> -f <filename> -s <string> -o <odir> -n <oname> -a -g <gfilename>", xxname.string());
    std::println("    -c <utf8|ascii> : select character set. utf8 implies unicode (default)");
    std::println("    -f <filename>   : read grammar from file <filename>");
    std::println("    -s <string>     : read grammar from <string> passed on commandline");
    std::println("    -d <dir>        : output directory");
    std::println("    -n <oname>      : output basename (oname.cpp and oname.hpp will be generated in dir)");
    std::println("    -a              : generate amalgamated file, including main(), which can be compiled into an executable");
    std::println("    -m              : print console messages");
    std::println("    -v (--version)  : print Yantra version");
    std::println("    -r              : don't generate #line messages");
    std::println("    -l <logname>    : generate log file to <logname>, use - for console");
    std::println("    -g <gfilename>  : generate grammar file to <gfilename>");
    return 1;
}

std::ofstream logfile;
std::ostream* logc = nullptr;
}

std::ostream& Logger::olog() {
    if(logc != nullptr) {
        return *logc;
    }
    return logfile;
}

int main(int argc, const char* argv[]) {
    if(argc < 2) {
        help(argv[0], "no inputs");
        return 0;
    }

    std::string filename;
    std::string logname;
    std::string string;
    std::string charset = "utf8";
    std::string odir = "./";
    std::string oname;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
    int i = 1;
    while(i < argc) {
        std::string a = argv[i];
        if(a == "-f") {
            if((filename.size() > 0) || (string.size() > 0)) {
                return help(argv[0], "only one filename or string input allowed");
            }
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid filename");
            }
            filename = argv[i];
        }else if(a == "-s") {
            if((filename.size() > 0) || (string.size() > 0)) {
                return help(argv[0], "only one filename or string input allowed");
            }
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid string");
            }
            string = argv[i];
        }else if(a == "-c") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid charset flag");
            }
            charset = argv[i];
        }else if(a == "-d") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid output dir");
            }
            odir = argv[i];
        }else if(a == "-n") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid output name");
            }
            oname = argv[i];
        }else if(a == "-l") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid log name");
            }
            logname = argv[i];
        }else if(a == "-a") {
            amalgamatedFile = true;
        }else if(a == "-m") {
            verbose = true;
        }else if((a == "-v") || (a == "--version")) {
            std::println("{}", YANTRA_VERSION_STRING);
            return 0;
        }else if(a == "-r") {
            genLines = false;
        }else if(a == "-g") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid grammar filename");
            }
            gfilename = argv[i];
        }else {
            return help(argv[0], "unknown option:" + a);
        }
        ++i;
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    if((charset != "utf8") && (charset != "ascii")) {
        return help(argv[0], "charset should be utf8 or ascii");
    }

    if((filename.size() == 0) && (string.size() == 0)) {
        return help(argv[0], "at least one filename or string input required");
    }

    if(oname.size() == 0) {
        if(filename.size() > 0) {
            std::filesystem::path fn(filename);
            oname = fn.stem().string();
        }else{
            oname = "out";
        }
    }


    std::filesystem::path d(odir);
    if(logname.size() == 0) {
        auto lname = d / (oname + ".log");
        logname = lname.string();
    }

    if(logname != "-") {
        logfile = std::ofstream(logname);
    }else{
        logc = &std::cout;
    }

    if(filename.size() > 0) {
        std::ifstream ifs(filename);
        if(!ifs) {
            std::println("cannot open file: {}", filename);
            return 1;
        }

        log("compiling file: {}", filename);
        if(auto r = processInput(ifs, filename, charset, d, oname)) {
            return r;
        }
    }else if(string.size() > 0) {
        std::istringstream iss(string);

        log("compiling string: {}", string);
        if(auto r = processInput(iss, "str", charset, d, oname)) {
            return r;
        }
    }else {
        assert(false);
    }

    return EXIT_SUCCESS;
}

