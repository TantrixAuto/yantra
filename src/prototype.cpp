/// @file prototype.cpp
/// @brief The prototype file for the generated parser
/// This file is stringified and embedded into the Yantra executable
/// The structure of this file (and implicitly the generated parser file) is
/// - AST classes
/// - Walker class(es)
/// - Token class
/// - Parser class
/// - Lexer class
/// - main() function
///
/// The main() function reads the input and feeds it into the Lexer
///
/// The Lexer scans the input and as soon as it recognises a token,
/// it sends the token to the Parser
///
/// The Parser sees the token and decides on the next State-Machine action (SHIFT, REDUCE, GOTO)
///
/// In case of REDUCE, it adds the approrpiate nodes to the AST
///
/// Once the Parser is done, the correct Walker is called to walk the AST and invoke the semantic action blocks.

///PROTOTYPE_ENTER:SKIP
#include <stdint.h>
#include <iostream>
#include <format>
#include "filepos.hpp"
#include "nsutil.hpp"
#include "print.hpp"
#define TAG(X) X

#define CLSNAME CLASSQID

constexpr const char* START_RULE_NAME = "";
constexpr unsigned long MAX_REPEAT_COUNT = 100;
constexpr unsigned long ROW = 1;
constexpr unsigned long COL = 1;
constexpr const char* SRC = "";
constexpr const char* MSG = "";

///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_ENTER:stdHeaders
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <variant>
#include <ranges>
#include <format>
#include <filesystem>
#include <unordered_map>
#include <assert.h>
///PROTOTYPE_LEAVE:stdHeaders

///PROTOTYPE_SEGMENT:hdrHeaders

struct TAG(CLASSQID) {
    struct Error : public std::runtime_error {
        template <typename ...ArgsT>
        static inline std::string
        fmtT(const std::format_string<ArgsT...>& msg, ArgsT... args) {
            auto xmsg = std::format(msg, std::forward<ArgsT>(args)...);
            return xmsg;
        }
        static inline std::string
        fmt(const size_t& r, const size_t& c, const std::string_view& f, const std::string& m) {
            auto ymsg = std::format("{}({:03d},{:03d}):{}", f, r, c, m);
            return ymsg;
        }
        template <typename ...ArgsT>
        static inline std::string
        fmt(const size_t& r, const size_t& c, const std::string_view& f, const std::format_string<ArgsT...>& msg, ArgsT... args) {
            auto xmsg = fmtT(msg, args...);
            auto ymsg = fmt(r, c, f, xmsg);
            return ymsg;
        }
    public:
        const size_t row = 0;
        const size_t col = 0;
        const std::string file;
        const std::string msg;
        template <typename ...ArgsT>
        inline Error(const size_t& r, const size_t& c, const std::string_view& f, const std::format_string<ArgsT...>& m, ArgsT... args)
            : std::runtime_error{fmt(r, c, f, m, args...)}, row{r}, col{c}, file{f}, msg{fmtT(m, args...)}
        {}
        inline Error(const size_t& r, const size_t& c, const std::string_view& f, const std::string& m)
            : std::runtime_error{fmt(r, c, f, m)}, row{r}, col{c}, file{f}, msg{m}
        {}
    };

    struct Impl;
    std::unique_ptr<Impl> _impl;
    std::string name;

    ///PROTOTYPE_SEGMENT:classMembers

    TAG(CLASSQID)(const std::string& name, const std::string& logger = "");
    ~TAG(CLASSQID)();

    void beginStream();
    void readStream(std::istream& is, const std::string_view& filename);
    void endStream();

    // read file into AST
    void readFile(const std::string& filename);

    // read string into AST
    void readString(const std::string& s, const std::string_view& filename);

    // walk AST, and write to output file
    void walk(const std::vector<std::string>& walkers, const std::filesystem::path& odir, const std::string_view& filename);

    // walk AST, no output file
    void walk(const std::vector<std::string>& walkers);

    // print AST
    void printAST(std::ostream& ss, const size_t& lvl, const std::string& indent) const;
};

///PROTOTYPE_ENTER:SKIP
[[noreturn]]
inline void dummy() {
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_TARGET:SOURCE

///PROTOTYPE_SEGMENT:srcHeaders

///PROTOTYPE_ENTER:throwError
throw TAG(CLASSQID)::Error(TAG(ROW), TAG(COL), TAG(SRC), TAG(MSG));
///PROTOTYPE_LEAVE:throwError

///PROTOTYPE_ENTER:SKIP
}
///PROTOTYPE_LEAVE:SKIP

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-member-function"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wtautological-unsigned-zero-compare"
#elif defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

///PROTOTYPE_INCLUDE:print

namespace {
    ///PROTOTYPE_INCLUDE:nsutil
    ///PROTOTYPE_INCLUDE:filepos
    ///PROTOTYPE_INCLUDE:textWriter

    static std::ostream* _log = nullptr;

    inline std::ostream& log() {
        assert(_log != nullptr);
        return *_log;
    }

    struct TAG(AST);

    struct TAG(AST) : public NonCopyable {
        using Error = TAG(CLASSQID)::Error;
        TAG(CLASSQID)& pub;

        inline TAG(AST)(TAG(CLASSQID)& p) : pub(p) {}

        inline TAG(AST)(const TAG(AST)&) = delete;
        inline TAG(AST)(TAG(AST)&&) = delete;
        inline TAG(AST)& operator=(const TAG(AST)&) = delete;
        inline TAG(AST)& operator=(TAG(AST)&&) = delete;

        struct _astEmpty {
            inline void dump(std::ostream&, const size_t&, const FilePos&, const std::string&) const {
            }
        };

        struct TAG(TOKEN) {
            FilePos pos;
            std::string text;
            inline TAG(TOKEN)() {}
            inline TAG(TOKEN)(const FilePos& p, const std::string& t) : pos(p), text(t) {}
            inline TAG(TOKEN)(const TAG(TOKEN)& src) : pos(src.pos), text(src.text) {}

            inline void dump(std::ostream& ss, const size_t&, const std::string& name, const std::string& indent) const {
                ss << std::format("{}: {}+--{}({})\n", pos.str(), indent, name, text);
            }

            inline const TAG(TOKEN)* get() const {
                return this;
            }

            inline TAG(TOKEN)& go(TAG(AST)&) {
                return *this;
            }

            inline TAG(TOKEN)& operator=(const TAG(TOKEN)& src) {
                pos = src.pos;
                text = src.text;
                return *this;
            }
        };

        ///PROTOTYPE_ENTER:SKIP
        struct START_RULE{
            inline START_RULE& go(TAG(AST)&) {
                return *this;
            }
            inline void dump(std::ostream&, const size_t&, const std::string&) const {
            }
        };

        using AstNode = std::variant <
            TAG(TOKEN)
        >;
        ///PROTOTYPE_LEAVE:SKIP

        ///PROTOTYPE_SEGMENT:astNodeDecls

        ///PROTOTYPE_SEGMENT:astNodeDefns

        ///PROTOTYPE_SEGMENT:astNodeItems

        std::vector<std::unique_ptr<AstNode>> astNodes;

        inline TAG(TOKEN)& createToken(const FilePos& p, const std::string& text) {
            astNodes.push_back(std::make_unique<AstNode>(TAG(TOKEN)(p, text)));
            AstNode& ri = *(astNodes.back());
            return std::get<TAG(TOKEN)>(ri);
        }

        template<typename AstNodeT>
        ///PROTOTYPE_ENTER:SKIP
        [[maybe_unused]]
        ///PROTOTYPE_LEAVE:SKIP
        inline AstNodeT& createAstNode(const FilePos& p, const TAG(TOKEN)& anchor) {
            auto w = std::make_unique<AstNode>();
            astNodes.push_back(std::move(w));
            AstNode& astNode = *(astNodes.back());
            astNode.emplace<AstNodeT>(p, anchor);
            return std::get<AstNodeT>(astNode);
        }

        TAG(START_RULE)* R_start = nullptr;

        ///PROTOTYPE_ENTER:SKIP
        struct TAG(WALKER) {
            inline TAG(WALKER)(TAG(CLASSQID)&) {}

            inline std::unique_ptr<TAG(TOKEN)> go(TAG(START_RULE)&) {
                return nullptr;
            }
            inline void layout() {
            }
        };
        ///PROTOTYPE_LEAVE:SKIP

        ///PROTOTYPE_SEGMENT:prologue

        template<typename T>
        struct WalkerNodeRef : public NonCopyable {
            const T& node;
            bool called = false;
            inline WalkerNodeRef(const T& n, const bool& c = false) : node(n), called(c) {}
        };

        template<typename T>
        struct WalkerNodeCommit : public NonCopyable {
            WalkerNodeRef<T>& node;
            inline WalkerNodeCommit(WalkerNodeRef<T>& n) : node(n) {}
            inline ~WalkerNodeCommit() {
                node.called = true;
            }
        };

        ///PROTOTYPE_SEGMENT:walkers

        ///PROTOTYPE_SEGMENT:epilogue

        inline TAG(START_RULE)& _root() const {
            assert(R_start);
            return *(R_start);
        }
    }; // TAG(AST)
} // namespace


namespace {
[[maybe_unused]]
constexpr size_t MaxRepeatCount = TAG(MAX_REPEAT_COUNT);

///PROTOTYPE_INCLUDE:utf8Encoding

///PROTOTYPE_INCLUDE:asciiEncoding

///PROTOTYPE_ENTER:SKIP
using char_t = uint32_t;
inline char_t read(std::istream&) {
    return 0;
}
///PROTOTYPE_LEAVE:SKIP

static inline bool contains(const char_t& ch, const char_t& from, const char_t& to, const std::initializer_list<std::pair<char_t, char_t>>& except) {
    if((ch >= from) && (ch <= to)) {
        return true;
    }
    for(auto& p : except) {
        if((ch >= p.first) && (ch <= p.second)) {
            return false;
        }
    }
    return false;
}

static inline bool contains(const char_t& ch, const char_t& from, const char_t& to) {
    if((ch >= from) && (ch <= to)) {
        return true;
    }
    return false;
}

////PROTOTYPE_ENTER:error
// include class Error
////PROTOTYPE_LEAVE:error

///PROTOTYPE_INCLUDE:stream

///PROTOTYPE_ENTER:SKIP
struct Stream {
    FilePos pos;
    inline Stream(std::istream&, const std::string_view&){}

    inline const bool& eof() const {
        static bool v = false;
        return v;
    }

    inline const char_t& peek() const {
        static char_t ch = ' ';
        return ch;
    }
};
///PROTOTYPE_LEAVE:SKIP

struct Tolkien {
    enum class ID {
        _null = 0,
        ///PROTOTYPE_ENTER:SKIP
        _tEND,
        ///PROTOTYPE_LEAVE:SKIP
        ///PROTOTYPE_SEGMENT:tokenIDs
    };

    FilePos pos;
    std::string text;
    ID id = ID::_null;

    inline Tolkien() {}
    inline Tolkien(const FilePos& p) : pos(p) {}

    static inline std::string str(const ID& i) {
        static const std::unordered_map<ID, std::string> tnames = {
            {ID::_null, "_null"},
            ///PROTOTYPE_SEGMENT:tokenIDNames
        };

        if (auto it = tnames.find(i); it != tnames.end()) {
            return it->second;
        }
        return "";
    }

    inline void addText(const char_t& ch) {
        text += static_cast<char>(ch);
    }

    inline std::string str() const {
        return std::format("{}({})", str(id), text);
    }
};

struct Parser {
    struct ValueItem {
        Tolkien token;
        size_t ruleID = 0;
        std::vector<ValueItem*> childs;
        inline ValueItem(const Tolkien& t) : token(t) {}
        inline ValueItem(const ValueItem&) = delete;
        inline ValueItem(ValueItem&&) = delete;
        inline ValueItem& operator=(const ValueItem&) = delete;
        inline ValueItem& operator=(ValueItem&&) = delete;
    };

    TAG(AST)& ast;
    std::vector<std::unique_ptr<ValueItem>> values;
    std::vector<ValueItem*> valueStack;
    std::vector<size_t> stateStack;

    inline ValueItem& addValue(const Tolkien& t) {
        values.push_back(std::make_unique<ValueItem>(t));
        return *(values.back());
    }

    inline Parser(TAG(AST)& a) : ast(a) {}

    inline ValueItem& shift(const Tolkien& k) {
        auto& vi = addValue(k);
        valueStack.push_back(&vi);
        return vi;
    }

    inline ValueItem& shift(const FilePos& pos, const Tolkien::ID& id) {
        Tolkien token(pos);
        token.id = id;
        return shift(token);
    }

    inline Tolkien _reduce(const size_t& len, const size_t& anchor, std::vector<ValueItem*>& childs) {
        assert(valueStack.size() >= len);
        assert(stateStack.size() >= len);
        auto ite = valueStack.end();
        auto it = ite - static_cast<long>(len);
        Tolkien anchorToken;
        std::stringstream ss;
        for (auto i = it; i < ite; ++i) {
            auto& v = **i;
            if (static_cast<size_t>(i - it) == anchor) {
                anchorToken = v.token;
            }
            ss << " " << v.token.str();
            ValueItem* cvi = *i;
            assert(cvi != nullptr);
            childs.push_back(cvi);
        }
        std::print(log(), "pop:{}\n", ss.str());
        for (size_t i = 0; i < len; ++i) {
            stateStack.pop_back();
            valueStack.pop_back();
        }
        return anchorToken;
    }

    inline void reduce(const size_t& ruleID, const size_t& len, const size_t& anchor, const Tolkien::ID& k, const std::string& text) {
        std::vector<ValueItem*> childs;
        auto tok = _reduce(len, anchor, childs);
        tok.id = k;
        tok.text = text;
        auto& vi = shift(tok);
        vi.ruleID = ruleID;
        vi.childs = childs;
    }

    inline bool isClean() const {
        if((valueStack.size() > 1) || (stateStack.size() > 1)) {
            return false;
        }
        if(stateStack.at(0) != 1) {
            return false;
        }

        if(valueStack.at(0)->token.text != TAG(START_RULE_NAME)) {
            return false;
        }

        return true;
    }

    inline void printParserState() const {
        std::stringstream vss;
        for (auto& i : valueStack) {
            vss << " " << i->token.str();
        }
        std::stringstream sss;
        for (auto& i : stateStack) {
            sss << " " << i;
        }
        std::print(log(), "\n");
        std::print(log(), "--------------------------\n");
        std::print(log(), "StateStack:{}\n", sss.str());
        std::print(log(), "ValueStack:{}\n", vss.str());
    }

    inline void printParserState(const Tolkien& k) const {
        printParserState();
        std::print(log(), "Token({}): {}\n", k.pos.str(), k.str());
    }

    template<typename NodeT>
    inline NodeT& create(const ValueItem& vi);

    ///PROTOTYPE_ENTER:SKIP
    struct START_RULE{
        inline START_RULE& go() {
            return *this;
        }
    };
    ///PROTOTYPE_LEAVE:SKIP

    inline void begin();
    inline bool parse(const Tolkien& k0);
    inline void leave();
}; // Parser

template<>
inline TAG(AST)::TAG(TOKEN)& Parser::create<TAG(AST)::TAG(TOKEN)>(const ValueItem& vi) {
    auto& cel = ast.createToken(vi.token.pos, vi.token.text);
    return cel;
}

///PROTOTYPE_ENTER:SKIP
template<>
inline TAG(AST)::TAG(START_RULE)& Parser::create<TAG(AST)::TAG(START_RULE)>(const ValueItem&) {
    // control flow won't usually reach here
    throw std::runtime_error("Don't do this please");
}
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_SEGMENT:createASTNodesDecls

///PROTOTYPE_SEGMENT:createASTNodesDefns

inline void Parser::begin() {
    values.clear();
    valueStack.clear();
    stateStack.clear();
    stateStack.push_back(1);
}

inline bool Parser::parse(const Tolkien& k0) {
    bool accepted = false;
    Tolkien k = k0;
    while (!accepted) {
        printParserState(k);
        assert(stateStack.size() > 0);
        switch (stateStack.back()) {
            ///PROTOTYPE_SEGMENT:parserTransitions
        }
    } // while(!accepted)
    return accepted;
} // parse()

inline void Parser::leave() {
    std::print(log(), "parse done\n");
    printParserState();
    if(valueStack.size() != 1) {
        // control flow won't usually reach here
        throw std::runtime_error("parse error");
    }
    auto& vi = *(valueStack.at(0));
    auto& R_start = create<TAG(AST)::TAG(START_RULE)>(vi);
    ast.R_start = &R_start;
}

struct Lexer {
    Parser& parser;
    size_t state = 1;
    Tolkien token;
    std::vector<size_t> counts;
    std::vector<size_t> modes;

    bool _eof = false;
    inline Lexer(Parser& p) : parser(p) {
        modes.push_back(1);
    }

    inline const size_t& modeRoot() const {
        assert(modes.size() > 0);
        return modes.back();
    }

    inline size_t count() const {
        assert(counts.size() > 0);
        return counts.back();
    }

    inline void begin() {
        state = 1;
        token = Tolkien();
        counts.clear();
        modes.clear();
        modes.push_back(1);
        _eof = false;
    }

    inline bool eof() const {
        return _eof;
    }

    inline const Tolkien& peek() const {
        return token;
    }

    inline void next(Stream& stream) {
        if (token.id == Tolkien::ID::_tEND) {
            assert(_eof == false);
            _eof = true;
            return;
        }
        token = Tolkien(stream.pos);
        while (!stream.eof()) {
            auto& ch = stream.peek();
            std::print(log(), "{}: Lexer:state={}, ch={}/{}, count={}\n", stream.pos.str(), state, (static_cast<int>(ch) == EOF ? "EOF" : std::to_string(ch)), std::isprint(static_cast<int>(ch)) ? static_cast<char>(ch) : ' ', counts.size());
            switch (state) {
                ///PROTOTYPE_SEGMENT:lexerStates
            } // switch(state)
        } // while(!eof)
        // parser.parse(token);
        // std::print("leave-lex-2\n");
    } // next()
}; // Lexer
} // namespace

struct TAG(CLASSQID)::Impl {
    TAG(CLSNAME)& module;
    TAG(AST) ast;
    Parser parser;
    Lexer lexer;
    std::ofstream flog;
    bool walking = false;

    struct WalkingGuard {
        Impl& impl;
        inline WalkingGuard(Impl& i) : impl(i) {
            impl.walking = true;
        }

        inline ~WalkingGuard() {
            impl.walking = false;
        }
    };

    inline Impl(TAG(CLSNAME)& m, const std::string& lname)
        : module(m)
        , ast(module)
        , parser(ast)
        , lexer(parser)
    {
        unused(lname);
        if(lname == "-") {
            _log = &std::cout;
        }else{
            if(lname.size() > 0) {
                flog.open(lname);
            }
            _log = &flog;
        }
    }

    inline void beginStream() {
        if(walking == true) {
            throw std::runtime_error("Cannot read stream while walking");
        }
        lexer.begin();
        parser.begin();
    }

    inline void readStream(std::istream& is, const std::string_view& filename) {
        Stream stream(is, filename);
        lexer.next(stream);
    }

    inline void endStream() {
        parser.leave();
    }

    inline void read(std::istream& is, const std::string_view& filename) {
        beginStream();
        readStream(is, filename);
        endStream();
    }

    inline void walk(std::vector<std::string> walkers, const std::filesystem::path& odir, const std::string_view& filename) {
        auto& start = ast._root();
        ///PROTOTYPE_ENTER:SKIP
        unused(start, odir, filename);
        ///PROTOTYPE_LEAVE:SKIP

        WalkingGuard wg(*this);
        for(auto& w : walkers) {
            if(w == "") {
            }
            ///PROTOTYPE_SEGMENT:walkerCalls
        }
    }

    inline void printAST(std::ostream& ss, const size_t& lvl, const std::string& indent) const {
        auto& start = ast._root();
        start.dump(ss, lvl, indent);
    }
};

TAG(CLASSQID)::TAG(CLSNAME)(const std::string& n, const std::string& lname) : name(n) {
    _impl = std::make_unique<Impl>(*this, lname);
}

TAG(CLASSQID)::~TAG(CLSNAME)() {
}

void TAG(CLASSQID)::beginStream() {
    return _impl->beginStream();
}

void TAG(CLASSQID)::readStream(std::istream& is, const std::string_view& filename) {
    return _impl->readStream(is, filename);
}

void TAG(CLASSQID)::endStream() {
    return _impl->endStream();
}

void TAG(CLASSQID)::walk(const std::vector<std::string>& walkers, const std::filesystem::path& odir, const std::string_view& filename) {
    return _impl->walk(walkers, odir, filename);
}

void TAG(CLASSQID)::walk(const std::vector<std::string>& walkers) {
    return _impl->walk(walkers, "", "");
}

void TAG(CLASSQID)::readFile(const std::string& filename) {
    std::ifstream is(filename);
    if(!is) {
        throw std::runtime_error("Cannot open file:" + filename);
    }
    _impl->read(is, filename);
}

void TAG(CLASSQID)::readString(const std::string& s, const std::string_view& filename) {
    std::istringstream is(s);
    _impl->read(is, filename);
}

void TAG(CLASSQID)::printAST(std::ostream& ss, const size_t& lvl, const std::string& indent) const {
    _impl->printAST(ss, lvl, indent);
}

///PROTOTYPE_ENTER:SKIP
#define HAS_REPL 1
///PROTOTYPE_LEAVE:SKIP

///PROTOTYPE_ENTER:repl
#if HAS_REPL
inline bool doREPL(TAG(CLASSQID)& mp, const std::string& input) {
    if(input == "\\q") {
        return false;
    }

    mp.readString(input, "<cmd>");
    return true;
}
#endif
///PROTOTYPE_LEAVE:repl

///PROTOTYPE_ENTER:fmain
inline int help(const std::string& xname, const std::string& msg) {
    auto xxname = std::filesystem::path(xname).filename();
    if(msg.size() > 0) {
        std::print("== {} ==\n", msg);
    }
#if HAS_REPL
    std::print("{} <options>\n", xxname.string());
    std::print("options:\n");
    std::print("    -i              : read input interactively from console\n");
#else
    std::print("{} -f <filename> -s <string> -l <log>\n", xxname.string());
#endif
    std::print("    -f <filename>   : read input from file <filename>\n");
    std::print("    -s <string>     : read input from <string> passed on commandline\n");
    std::print("    -l <log>        : generate debug log to <log> (use - for console)\n");
    std::print("    -t | -t1        : print AST to log\n");
    std::print("    -t2             : print AST to log with rules expanded\n");
    std::print("    -v              : print verbose messages to console\n");
    std::print("    -w <walker>     : use walker\n");
    std::print("    -o <filename>   : write output to file <filename>\n");
    return 1;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        help(argv[0], "no inputs");
        return 0;
    }

    std::vector<std::string> filenames;
    std::vector<std::string> strings;
    std::vector<std::string> walkers;
    std::string odir = ".";
    std::string log;
    bool verbose = false;
    size_t printAstLevel = 0;
#if HAS_REPL
    bool repl = false;
#else
    const bool repl = false;
#endif

#if defined(__clang__)
#pragma clang unsafe_buffer_usage begin
#endif
    int i = 1;
    while(i < argc) {
        std::string a = argv[i];
        if(a == "-f") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid filename");
            }
            auto v = argv[i];
            filenames.push_back(v);
        }else if(a == "-s") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid string");
            }
            auto v = argv[i];
            strings.push_back(v);
        }else if(a == "-w") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid string");
            }
            auto v = argv[i];
            walkers.push_back(v);
        }else if(a == "-o") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid output dir");
            }
            odir = argv[i];
        }else if(a == "-l") {
            ++i;
            if(i >= argc) {
                return help(argv[0], "invalid logfile");
            }
            log = argv[i];
        }else if(a == "-v") {
            verbose = true;
#if HAS_REPL
        }else if(a == "-i") {
            repl = true;
#endif
        }else if((a == "-t") || (a == "-t1")) {
            printAstLevel = 1;
        }else if(a == "-t2") {
            printAstLevel = 2;
        }else {
            return help(argv[0], "unknown argument: " + a);
        }
        ++i;
    }
#if defined(__clang__)
#pragma clang unsafe_buffer_usage end
#endif

    if(repl == false) {
        if((filenames.size() + strings.size()) == 0) {
            return help(argv[0], "no input files or strings");
        }
        if((filenames.size() + strings.size()) != 1) {
            return help(argv[0], "exactly 1 input file or string required");
        }
    }

    if(walkers.size() == 0) {
        ///PROTOTYPE_SEGMENT:initWalkers
    }

    // normalized output folder
    std::filesystem::path outf;
    outf = odir;
    outf = std::filesystem::absolute(outf);
    outf = outf.lexically_normal();

    if(repl == false) {
        int errs = 0;

        // read all files
        for(auto& f : filenames) {
            if(verbose) std::print("compiling file: {}\n", f);

            try {
                // instance of the module
                TAG(CLASSQID) mp("main", log);
                mp.readFile(f);
                if(printAstLevel > 0) {
                    mp.printAST(std::cout, printAstLevel, "");
                }
                mp.walk(walkers, outf, f);
            }catch(const std::exception& ex) {
                ++errs;
                std::print("err:{}\n", ex.what());
            }
        }

        // read all strings
        size_t idx = 1;
        for(auto& f : strings) {
            auto inn = std::format("a{}.in", idx);
            ++idx;
            if(verbose) std::print("compiling string: {}\n", inn);

            try {
                // instance of the module
                TAG(CLASSQID) mp("main", log);
                mp.readString(f, inn);
                if(printAstLevel > 0) {
                    mp.printAST(std::cout, printAstLevel, "");
                }
                mp.walk(walkers);
            }catch(const std::exception& ex) {
                std::print("err:{}\n", ex.what());
                ++errs;
            }
        }
        return errs;
    }

#if HAS_REPL
    if(repl == true) {
        // instance of the module
        TAG(CLASSQID) mp("main", log);

        // read all files
        for(auto& f : filenames) {
            if(verbose) std::print("compiling file: {}\n", f);

            try {
                mp.readFile(f);
                if(printAstLevel > 0) {
                    mp.printAST(std::cout, printAstLevel, "");
                }
                mp.walk(walkers);
            }catch(const std::exception& ex) {
                std::print("err:{}\n", ex.what());
            }
        }

        // read all strings
        for(auto& f : strings) {
            //NOTE: not printing the actual string here (there's no {} in fmt-string)
            if(verbose) std::print("compiling string\n", f);

            try {
                mp.readString(f, "<str>");
                if(printAstLevel > 0) {
                    mp.printAST(std::cout, printAstLevel, "");
                }
                mp.walk(walkers);
            }catch(const std::exception& ex) {
                std::print("err:{}\n", ex.what());
            }
        }

        std::string input;
        while(true) {
            std::cout << ">";
            std::getline(std::cin, input);
            try {
                if(doREPL(mp, input) == false) {
                    break;
                }
            }catch(const std::exception& ex) {
                std::print("err:{}\n", ex.what());
            }
        }
    }
#endif

    return EXIT_SUCCESS;
}
///PROTOTYPE_LEAVE:fmain
