/// @file parser.cpp
/// @brief This file contains all the code for parsing a grammar file
/// The parser is implemented as a recursive descent parser
/// Both Lexer and Parser have a lookahead of 1
/// the terms token and rgex are used interchangably in this file

#include "pch.hpp"
#include "parser.hpp"
#include "logger.hpp"

namespace {
/// @brief Encapsulates all tokens identified by the lexer
struct Token {
    enum class ID {
        END,
        ID,
        TYPE,
        ARGS,

        COLONEQ,
        COLONEQGT,
        COLONEQEQ,
        DBLCOLON,
        CARET,
        POINTER,
        DBLPOINTER,
        BANG,
        SEMI,
        AT,
        PERCENT,
        LBRACKET,
        RBRACKET,
        LSQUARE,
        RSQUARE,
        CODEBLOCK,
        RX_DBLQUOTE,
        RX_DISJUNCT,
        RX_WILDCARD,

        RX_GROUP_ENTER,
        RX_GROUP_LEAVE,
        RX_GROUP_DONT,

        RX_ESC_CLASS_DIGIT,
        RX_ESC_CLASS_NOT_DIGIT,
        RX_ESC_CLASS_LETTER,
        RX_ESC_CLASS_NOT_LETTER,
        RX_ESC_CLASS_WORD,
        RX_ESC_CLASS_NOT_WORD,
        RX_ESC_CLASS_SPACE,
        RX_ESC_CLASS_NOT_SPACE,

        RX_ESC_CLASS_WBOUNDARY,
        RX_ESC_CLASS_NOT_WBOUNDARY,
        RX_ESC_CLASS_HEX,

        RX_CLOSURE_STAR,
        RX_CLOSURE_PLUS,
        RX_CLOSURE_QUESTION,
        RX_CLOSURE_ENTER,
        RX_CLOSURE_VALUE,
        RX_CLOSURE_LEAVE,

        RX_CLASS_ENTER,
        RX_CLASS_LEAVE,
        RX_CLASS_CARET,

        RX_CLASS_RANGE,
    };

    /// @brief maps the string name for a token
    [[maybe_unused]]
    static inline const char* sname(const Token::ID& s) {
        static const std::unordered_map<Token::ID, const char*> map = {
            {Token::ID::END, "END"},
            {Token::ID::ID, "ID"},
            {Token::ID::TYPE, "TYPE"},
            {Token::ID::ARGS, "ARGS"},

            {Token::ID::COLONEQ, "COLONEQ"},
            {Token::ID::COLONEQGT, "COLONEQGT"},
            {Token::ID::COLONEQEQ, "COLONEQEQ"},
            {Token::ID::DBLCOLON, "DBLCOLON"},
            {Token::ID::CARET, "CARET"},
            {Token::ID::POINTER, "POINTER"},
            {Token::ID::DBLPOINTER, "DBLPOINTER"},
            {Token::ID::BANG, "BANG"},
            {Token::ID::AT, "AT"},
            {Token::ID::PERCENT, "PERCENT"},
            {Token::ID::SEMI, "SEMI"},
            {Token::ID::LBRACKET, "LBRACKET"},
            {Token::ID::RBRACKET, "RBRACKET"},
            {Token::ID::LSQUARE, "LSQUARE"},
            {Token::ID::RSQUARE, "RSQUARE"},
            {Token::ID::CODEBLOCK, "CODEBLOCK"},
            {Token::ID::RX_DBLQUOTE, "RX_DBLQUOTE"},
            {Token::ID::RX_DISJUNCT, "RX_DISJUNCT"},
            {Token::ID::RX_WILDCARD, "RX_WILDCARD"},

            {Token::ID::RX_GROUP_ENTER, "RX_GROUP_ENTER"},
            {Token::ID::RX_GROUP_LEAVE, "RX_GROUP_LEAVE"},
            {Token::ID::RX_GROUP_DONT, "RX_GROUP_DONT"},
            {Token::ID::RX_ESC_CLASS_DIGIT, "RX_ESC_CLASS_DIGIT"},
            {Token::ID::RX_ESC_CLASS_NOT_DIGIT, "RX_ESC_CLASS_NOT_DIGIT"},
            {Token::ID::RX_ESC_CLASS_LETTER, "RX_ESC_CLASS_LETTER"},
            {Token::ID::RX_ESC_CLASS_NOT_LETTER, "RX_ESC_CLASS_NOT_LETTER"},
            {Token::ID::RX_ESC_CLASS_WORD, "RX_ESC_CLASS_WORD"},
            {Token::ID::RX_ESC_CLASS_NOT_WORD, "RX_ESC_CLASS_NOT_WORD"},
            {Token::ID::RX_ESC_CLASS_SPACE, "RX_ESC_CLASS_SPACE"},
            {Token::ID::RX_ESC_CLASS_NOT_SPACE, "RX_ESC_CLASS_NOT_SPACE"},
            {Token::ID::RX_ESC_CLASS_WBOUNDARY, "RX_ESC_CLASS_WBOUNDARY"},
            {Token::ID::RX_ESC_CLASS_NOT_WBOUNDARY, "RX_ESC_CLASS_NOT_WBOUNDARY"},
            {Token::ID::RX_ESC_CLASS_HEX, "RX_ESC_CLASS_HEX"},

            {Token::ID::RX_CLOSURE_STAR, "RX_CLOSURE_STAR"},
            {Token::ID::RX_CLOSURE_PLUS, "RX_CLOSURE_PLUS"},
            {Token::ID::RX_CLOSURE_QUESTION, "RX_CLOSURE_QUESTION"},

            {Token::ID::RX_CLOSURE_ENTER, "RX_CLOSURE_ENTER"},
            {Token::ID::RX_CLOSURE_VALUE, "RX_CLOSURE_VALUE"},
            {Token::ID::RX_CLOSURE_LEAVE, "RX_CLOSURE_LEAVE"},

            {Token::ID::RX_CLASS_ENTER, "RX_CLASS_ENTER"},
            {Token::ID::RX_CLASS_LEAVE, "RX_CLASS_LEAVE"},
            {Token::ID::RX_CLASS_CARET, "RX_CLASS_CARET"},
            {Token::ID::RX_CLASS_RANGE, "RX_CLASS_RANGE"},
        };
        if(auto it = map.find(s); it != map.end()) {
            return it->second;
        }
        return "";
    }

    /// @brief returns the string name for this token
    static inline const char* sname(const Token& k) {
        return sname(k.id);
    }

    /// @brief position in the input stream where the token was scanned
    FilePos pos;

    /// @brief text of the token
    std::string text;

    /// @brief the token ID
    ID id = ID::END;
};

#define DO_TRACER 0

/// @brief Helper class to trace the execution of the parser
/// Set DO_TRACER to 1 to enable tracing
struct Tracer {
    const size_t lvl;
    std::string name;

    [[maybe_unused]]
    inline Tracer(const size_t& l, const std::string& n) : lvl{l}, name{n} {
#if DO_TRACER
        log("{}:{}: enter", lvl, name);
#endif
    }

    [[maybe_unused]]
    inline ~Tracer() {
#if DO_TRACER
        log("{}:{}: leave", lvl, name);
#endif
    }
};

/// @brief The Lexer class
/// The next() function scans the input stream repeatedly until
/// it rcognizes a token. The token is stored and can be fetched repeatedly
/// by the parser, until the parser calls next() again.
/// The parser can give contextual hints to the lexer on what to expect next
/// e.g: setMode_Type() tells the Lexer to expect a type, in which case the
/// Lexer will keep reading all iput untilit receives a semi-colon.
struct Lexer {
    /// @brief The current Lexer state
    enum class State {
        Init,
        Colon,
        ColonEq,
        Identifier,

        SlComment0,
        SlComment1,
        MlComment0,
        MlComment1,
        MlComment2,

        EnterPragma,
        PragmaHeader0,
        PragmaHeader1,
        PragmaHeader2,
        PragmaType0,
        PragmaType1,
        PragmaArgs0,
        PragmaArgs1,
        CodeBlock0,
        CodeBlock1,
        LeaveCodeBlock0,
        Pointer0,
        Pointer1,

        EnterRegex,
        Regex,
        RegexEsc,

        RegexEscHex2,
        RegexEscHex21,
        RegexEscHex4,
        RegexEscHex41,
        RegexEscHex42,
        RegexEscHex43,
        RegexEscHexInitN,
        RegexEscHexN,

        RegexClassInit,
        RegexClassChar,
        RegexClassRange,

        RegexQuantifer0,
        RegexQuantifer1,
        RegexQuantifer2,

        RegexGroupInit,
    };

    /// @brief Convert the Lexer state to a string for logging purposes
    [[maybe_unused]]
    static inline const char* sname(const State& s) {
        static std::unordered_map<Lexer::State, const char*> map = {
            {Lexer::State::Init, "Init"},
            {Lexer::State::Colon, "Colon"},
            {Lexer::State::ColonEq, "ColonEq"},
            {Lexer::State::Identifier, "Identifier"},

            {Lexer::State::SlComment0, "SlComment0"},
            {Lexer::State::SlComment1, "SlComment1"},
            {Lexer::State::MlComment0, "MlComment0"},
            {Lexer::State::MlComment1, "MlComment1"},
            {Lexer::State::MlComment2, "MlComment2"},
            {Lexer::State::EnterPragma, "EnterPragma"},
            {Lexer::State::PragmaType0, "PragmaType0"},
            {Lexer::State::PragmaType1, "PragmaType1"},
            {Lexer::State::PragmaArgs0, "PragmaArgs0"},
            {Lexer::State::PragmaArgs1, "PragmaArgs1"},
            {Lexer::State::PragmaHeader0, "PragmaHeader0"},
            {Lexer::State::PragmaHeader1, "PragmaHeader1"},
            {Lexer::State::PragmaHeader2, "PragmaHeader2"},
            {Lexer::State::CodeBlock0, "CodeBlock0"},
            {Lexer::State::CodeBlock1, "CodeBlock1"},
            {Lexer::State::LeaveCodeBlock0, "LeaveCodeBlock0"},
            {Lexer::State::Pointer0, "Pointer0"},
            {Lexer::State::Pointer1, "Pointer1"},

            {Lexer::State::EnterRegex, "EnterRegex"},
            {Lexer::State::Regex, "Regex"},

            {Lexer::State::RegexEsc, "RegexEsc"},
            {Lexer::State::RegexEscHex2, "RegexEscHex2"},
            {Lexer::State::RegexEscHex21, "RegexEscHex21"},
            {Lexer::State::RegexEscHex4, "RegexEscHex4"},
            {Lexer::State::RegexEscHex41, "RegexEscHex41"},
            {Lexer::State::RegexEscHex42, "RegexEscHex42"},
            {Lexer::State::RegexEscHex43, "RegexEscHex43"},
            {Lexer::State::RegexEscHexInitN, "RegexEscHexInitN"},
            {Lexer::State::RegexEscHexN, "RegexEscHexN"},

            {Lexer::State::RegexClassInit, "RegexClassInit"},
            {Lexer::State::RegexClassChar, "RegexClassChar"},
            {Lexer::State::RegexClassRange, "RegexClassRange"},

            {Lexer::State::RegexQuantifer0, "RegexQuantifer0"},
            {Lexer::State::RegexQuantifer1, "RegexQuantifer1"},
            {Lexer::State::RegexQuantifer2, "RegexQuantifer2"},

            {Lexer::State::RegexGroupInit, "RegexGroupInit"},
        };
        if(auto it = map.find(s); it != map.end()) {
            return it->second;
        }
        return "";
    }

    /// @brief the input stream to read from
    Stream& stream;

    /// @brief the current lexer state
    State state = State::Init;

    /// @brief the current depth of nested regex classes
    size_t classDepth = 0;

    /// @brief the current depth of nested regex groups
    size_t groupDepth = 0;

    /// @brief the current depth of nested multi-line comments
    size_t mlcomment = 0;

#ifndef NDEBUG
    /// @brief debugging aid to prevent Parser from looping infinitely
    /// An assert is triggered if peek() is called more than 50 times
    /// Typically it should be called more than 2 times, 3 at max.
    mutable size_t pcount = 0;
#endif

    /// @brief the currently recognized token
    Token t;

    inline Lexer(Stream& s) : stream{s} {}

    /// @brief when called, the next token will be a header
    /// ie, read all text between "" or <>
    inline void setMode_Header() {
        resetTokenString();
        state = State::PragmaHeader0;
    }

    /// @brief when called, the next token will be a type
    /// ie, read all text till the next ';'
    inline void setMode_Type() {
        state = State::PragmaType0;
    }

    /// @brief when called, the next token will be an arglist
    /// ie, read all text between ( and )
    inline void setMode_Args() {
        resetTokenString();
        state = State::PragmaArgs0;
    }

    /// @brief used by the lexer to initialize the token text to @arg ch
    inline void setTokenString(const int& ch) {
        t.text = static_cast<char>(ch);
    }

    /// @brief used by the lexer to initialize the token text to empty and set the token pos
    inline void resetTokenString() {
        t.pos = stream.pos;
        t.text.clear();
    }

    /// @brief used by the lexer to initialize the token text to @arg ch and set the token pos
    inline void resetTokenString(const int& ch) {
        t.pos = stream.pos;
        t.text = static_cast<char>(ch);
    }

    /// @brief used by the lexer to append @arg ch to the token text
    inline void appendTokenString(const int& ch) {
        t.text += static_cast<char>(ch);
    }

    /// @brief used by the lexer to advance the stream and set the current Lexer state to @arg s
    inline void consume(const State& s) {
        stream.consume();
        state = s;
    }

    /// @brief used by the lexer to advance the stream, set the current Lexer state to @arg s, and append @arg ch to the token text
    inline void consume(const State& s, const int& ch) {
        appendTokenString(ch);
        consume(s);
    }

    /// @brief used by the lexer to set the current Lexer state to @arg s, and the current token ID to @arg id
    /// When a token has been recognised, the lexer calls this and returns.
    /// The Parser then calls peek() to retreieve the recognised token.
    inline void match(const Token::ID& id, const State& s = State::Init) {
        state = s;
        t.id = id;
    }

    /// @brief used by the lexer to set the current Lexer state to @arg s, append @arg ch to token text, and the current token ID to @arg id
    /// When a token has been recognised, the lexer calls this and returns.
    /// The Parser then calls peek() to retrieve the recognised token.
    inline void match(const Token::ID& id, const int& ch, const State& s = State::Init) {
        appendTokenString(ch);
        stream.consume();
        match(id, s);
    }

    /// @brief used by the lexer to set the current Lexer state to @arg s, append @arg ch to token text, and the current token ID to @arg id
    /// this function is called when in the middle of a regex.
    inline void rxmatch(const Token::ID& id, const int& ch, const State& s = State::Regex) {
        match(id, ch, s);
    }

    /// @brief returns true if @arg ch is a initial ID character
    inline bool isID0(const int& ch) const {
        if(
            ((ch >= 'a') && (ch <= 'z'))
            || ((ch >= 'A') && (ch <= 'Z'))
        ) {
            return true;
        }
        return false;
    }

    /// @brief returns true if @arg ch is a subsequent ID character
    inline bool isID1(const int& ch) const {
        if(
            (((ch >= 'a') && (ch <= 'z'))
             || ((ch >= 'A') && (ch <= 'Z'))
             || ((ch >= '0') && (ch <= '9'))
             || (ch == '_')
             )
        ) {
            return true;
        }
        return false;
    }

    /// @brief returns the currently recognized token
    inline const Token& peek() const {
#ifndef NDEBUG
        assert(++pcount < 50);
#endif
        return t;
    }

    /// @brief scan the input stream for the next token
    inline void next() {
#ifndef NDEBUG
        pcount = 0;
#endif
        while(!stream.eof()) {
            int ch = stream.peek();
            log("{:>3}:  lexer: s={}, ch={}, ch={}, text={}, tpos={}, classDepth={}, groupDepth={}"
                  , stream.pos.str()
                  , sname(state)
                  , isprint(ch)?static_cast<char>(ch):' '
                  , static_cast<int>(ch)
                  , t.text
                  , t.pos.str()
                  , classDepth
                  , groupDepth
            );

            switch(state) {
            case State::Init:
                resetTokenString();
                switch(ch) {
                case EOF:
                    t.id = Token::ID::END;
                    return;
                case ' ':
                case '\t':
                case '\r':
                case '\n':
                    stream.consume();
                    continue;
                case ';':
                    return match(Token::ID::SEMI, ch);
                case '(':
                    return match(Token::ID::LBRACKET, ch);
                case ')':
                    return match(Token::ID::RBRACKET, ch);
                case '[':
                    return match(Token::ID::LSQUARE, ch);
                case ']':
                    return match(Token::ID::RSQUARE, ch);
                case '^':
                    return match(Token::ID::CARET, ch);
                case '!':
                    return match(Token::ID::BANG, ch);
                case '@':
                    return match(Token::ID::AT, ch);
                case ':':
                    consume(State::Colon, ch);
                    break;
                case '-':
                    consume(State::Pointer0, ch);
                    break;
                case '=':
                    consume(State::Pointer1, ch);
                    break;
                case '/':
                    consume(State::SlComment0, ch);
                    break;
                case '%':
                    consume(State::EnterPragma, ch);
                    break;
                case '"':
                    return match(Token::ID::RX_DBLQUOTE, ch, State::EnterRegex);
                default:
                    if(isID0(ch) == true) {
                        consume(State::Identifier, ch);
                        break;
                    }
                    throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");
                }
                break;
            case State::Colon:
                if(ch == '=') {
                    consume(State::ColonEq, ch);
                    break;
                }
                if(ch == ':') {
                    return match(Token::ID::DBLCOLON, ch);
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");
            case State::ColonEq:
                if(ch == '>') {
                    return match(Token::ID::COLONEQGT, ch);
                }
                if(ch == '=') {
                    return match(Token::ID::COLONEQEQ, ch);
                }
                return match(Token::ID::COLONEQ, ch);
            case State::Pointer0:
                if(ch == '>') {
                    return match(Token::ID::POINTER, ch);
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");
            case State::Pointer1:
                if(ch == '>') {
                    return match(Token::ID::DBLPOINTER, ch);
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");
            case State::Identifier:
                if(isID1(ch) == true) {
                    appendTokenString(ch);
                    stream.consume();
                    break;
                }

                return match(Token::ID::ID);
            case State::PragmaType0:
                if(std::isspace(ch)) {
                    stream.consume();
                    break;
                }
                resetTokenString(ch);
                state = State::PragmaType1;
                stream.consume();
                break;
            case State::PragmaType1:
                if(ch ==';') {
                    state = State::Init;
                    t.id = Token::ID::TYPE;
                    return;
                }
                appendTokenString(ch);
                stream.consume();
                break;
            case State::PragmaArgs0:
                if(std::isspace(ch)) {
                    stream.consume();
                    break;
                }
                if(ch ==')') {
                    state = State::Init;
                    t.id = Token::ID::ARGS;
                    return;
                }
                resetTokenString(ch);
                state = State::PragmaArgs1;
                stream.consume();
                break;
            case State::PragmaArgs1:
                if(ch ==')') {
                    state = State::Init;
                    t.id = Token::ID::ARGS;
                    return;
                }
                appendTokenString(ch);
                stream.consume();
                break;
            case State::PragmaHeader0:
                if(std::isspace(ch)) {
                    stream.consume();
                    break;
                }
                if(ch == '"') {
                    state = State::PragmaHeader1;
                    stream.consume();
                    break;
                }
                if(ch == '<') {
                    state = State::PragmaHeader2;
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");
            case State::PragmaHeader1:
                if(ch == '"') {
                    stream.consume();
                    state = State::Init;
                    t.id = Token::ID::ID;
                    return;
                }
                appendTokenString(ch);
                stream.consume();
                break;
            case State::PragmaHeader2:
                if(ch == '>') {
                    stream.consume();
                    state = State::Init;
                    t.id = Token::ID::ID;
                    return;
                }
                appendTokenString(ch);
                stream.consume();
                break;
            case State::SlComment0:
                if(ch == '/') {
                    stream.consume();
                    state = State::SlComment1;
                    break;
                }
                if(ch == '*') {
                    mlcomment = 1;
                    stream.consume();
                    state = State::MlComment0;
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");
            case State::SlComment1:
                if(ch == '\n') {
                    state = State::Init;
                }
                stream.consume();
                break;
            case State::MlComment0:
                if(ch == '*') {
                    stream.consume();
                    state = State::MlComment1;
                    break;
                }
                if(ch == '/') {
                    stream.consume();
                    state = State::MlComment2;
                    break;
                }
                stream.consume();
                break;
            case State::MlComment1:
                if(ch == '/') {
                    --mlcomment;
                    if(mlcomment == 0) {
                        state = State::Init;
                    }else{
                        state = State::MlComment0;
                    }
                }else{
                    state = State::MlComment0;
                }
                stream.consume();
                break;
            case State::MlComment2:
                if(ch == '*') {
                    ++mlcomment;
                }
                state = State::MlComment0;
                stream.consume();
                break;
            case State::EnterPragma:
                if(ch == '{') {
                    resetTokenString();
                    state = State::CodeBlock0;
                    stream.consume();
                    break;
                }
                setTokenString(ch);
                return match(Token::ID::PERCENT);
            case State::CodeBlock0:
                if((ch == '\r') || (ch == '\n')) {
                    stream.consume();
                    break;
                }
                if(ch == '%') {
                    resetTokenString();
                    state = State::LeaveCodeBlock0;
                    stream.consume();
                    break;
                }
                resetTokenString(ch);
                state = State::CodeBlock1;
                stream.consume();
                break;
            case State::CodeBlock1:
                if(ch == '%') {
                    state = State::LeaveCodeBlock0;
                }else {
                    appendTokenString(ch);
                }
                stream.consume();
                break;
            case State::LeaveCodeBlock0:
                if(ch == '}') {
                    stream.consume();
                    return match(Token::ID::CODEBLOCK);
                }
                appendTokenString('%');
                appendTokenString(ch);
                state = State::CodeBlock1;
                stream.consume();
                break;
            case State::EnterRegex:
                if(ch == '"') {
                    throw GeneratorError(__LINE__, __FILE__, stream.pos, "EMPTY_REGEX");
                }
                state = State::Regex;
                break;
            case State::Regex:
                switch(ch) {
                case '"':
                    if((groupDepth > 0) || (classDepth > 0)) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    return match(Token::ID::RX_DBLQUOTE, ch);
                case '\\':
                    state = State::RegexEsc;
                    break;
                case '|':
                    if(classDepth > 0) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    return rxmatch(Token::ID::RX_DISJUNCT, ch);
                case '.':
                    return rxmatch(Token::ID::RX_WILDCARD, ch);
                case '[':
                    if(classDepth > 0) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    setTokenString(ch);
                    t.id = Token::ID::RX_CLASS_ENTER;
                    state = State::RegexClassInit;
                    ++classDepth;
                    stream.consume();
                    return;
                case ']':
                    if(classDepth == 0) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    setTokenString(ch);
                    t.id = Token::ID::RX_CLASS_LEAVE;
                    state = State::Regex;
                    --classDepth;
                    stream.consume();
                    return;
                case '*':
                    return rxmatch(Token::ID::RX_CLOSURE_STAR, ch);
                case '+':
                    return rxmatch(Token::ID::RX_CLOSURE_PLUS, ch);
                case '?':
                    return rxmatch(Token::ID::RX_CLOSURE_QUESTION, ch);
                case '(':
                    if(classDepth > 0) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    ++groupDepth;
                    return rxmatch(Token::ID::RX_GROUP_ENTER, ch, State::RegexGroupInit);
                case ')':
                    if(classDepth > 0) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    if(groupDepth == 0) {
                        throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_CHAR");
                    }
                    setTokenString(ch);
                    t.id = Token::ID::RX_GROUP_LEAVE;
                    --groupDepth;
                    stream.consume();
                    return;
                case '{':
                    resetTokenString();
                    t.id = Token::ID::RX_CLOSURE_ENTER;
                    state = State::RegexQuantifer0;
                    stream.consume();
                    return;
                default:
                    setTokenString(ch);
                    if(classDepth > 0) {
                        stream.consume();
                        state = State::RegexClassChar;
                        continue;
                    }
                    appendTokenString(ch);
                    t.id = Token::ID::RX_CLASS_RANGE;
                    stream.consume();
                    return;
                }

                stream.consume();
                break;

            case State::RegexQuantifer0:
                if(isDEC(ch) == true) {
                    appendTokenString(ch);
                    stream.consume();
                    break;
                }
                if(ch == ',') {
                    t.id = Token::ID::RX_CLOSURE_VALUE;
                    state = State::RegexQuantifer1;
                    stream.consume();
                    return;
                }

                if(ch == '}') {
                    return rxmatch(Token::ID::RX_CLOSURE_LEAVE, ch);
                }

                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");

            case State::RegexQuantifer1:
                if(isDEC(ch) == true) {
                    resetTokenString(ch);
                    state = State::RegexQuantifer2;
                    stream.consume();
                    break;
                }

                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");

            case State::RegexQuantifer2:
                if(isDEC(ch) == true) {
                    appendTokenString(ch);
                    stream.consume();
                    break;
                }

                if(ch == '}') {
                    return rxmatch(Token::ID::RX_CLOSURE_LEAVE, ch);
                }

                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_INPUT");

            case State::RegexGroupInit:
                setTokenString(ch);
                switch(ch) {
                case '!':
                    return rxmatch(Token::ID::RX_GROUP_DONT, ch);
                }
                state = State::Regex;
                break;
            case State::RegexClassInit:
                setTokenString(ch);
                switch(ch) {
                case '^':
                    return rxmatch(Token::ID::RX_CLASS_CARET, ch);
                }
                state = State::Regex;
                break;
            case State::RegexClassChar:
                switch(ch) {
                case '-':
                    state = State::RegexClassRange;
                    stream.consume();
                    break;
                default:
                    assert(t.text.size() == 1);
                    appendTokenString(t.text.at(0));
                    t.id = Token::ID::RX_CLASS_RANGE;
                    state = State::Regex;
                    return;
                }
                break;
            case State::RegexClassRange:
                assert(t.text.size() == 1);
                if(ch < t.text.at(0)) {
                    throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_RANGE");
                }
                return rxmatch(Token::ID::RX_CLASS_RANGE, ch);

            case State::RegexEsc:
                setTokenString(ch);
                switch(ch) {
                case 'x':
                    state = State::RegexEscHex2;
                    stream.consume();
                    continue;
                case 'u':
                    state = State::RegexEscHex4;
                    stream.consume();
                    continue;
                }

                switch(ch) {
                case 'd':
                    t.id = Token::ID::RX_ESC_CLASS_DIGIT;
                    break;
                case 'D':
                    t.id = Token::ID::RX_ESC_CLASS_NOT_DIGIT;
                    break;
                case 'l':
                    t.id = Token::ID::RX_ESC_CLASS_LETTER;
                    break;
                case 'L':
                    t.id = Token::ID::RX_ESC_CLASS_NOT_LETTER;
                    break;
                case 'w':
                    t.id = Token::ID::RX_ESC_CLASS_WORD;
                    break;
                case 'W':
                    t.id = Token::ID::RX_ESC_CLASS_NOT_WORD;
                    break;
                case 's':
                    t.id = Token::ID::RX_ESC_CLASS_SPACE;
                    break;
                case 'S':
                    t.id = Token::ID::RX_ESC_CLASS_NOT_SPACE;
                    break;
                case 'b':
                    t.id = Token::ID::RX_ESC_CLASS_WBOUNDARY;
                    break;
                case 'B':
                    t.id = Token::ID::RX_ESC_CLASS_NOT_WBOUNDARY;
                    break;

                case 'f':
                    setTokenString('\f');
                    appendTokenString('\f');
                    t.id = Token::ID::RX_CLASS_RANGE;
                    break;
                case 'n':
                    setTokenString('\n');
                    appendTokenString('\n');
                    t.id = Token::ID::RX_CLASS_RANGE;
                    break;
                case 'r':
                    setTokenString('\r');
                    appendTokenString('\r');
                    t.id = Token::ID::RX_CLASS_RANGE;
                    break;
                case 't':
                    setTokenString('\t');
                    appendTokenString('\t');
                    t.id = Token::ID::RX_CLASS_RANGE;
                    break;
                case 'v':
                    setTokenString('\v');
                    appendTokenString('\v');
                    t.id = Token::ID::RX_CLASS_RANGE;
                    break;

                case '0':
                    t.text.clear();
                    [[fallthrough]];
                case '^':
                case '$':
                case '\\':
                case '.':
                case '*':
                case '+':
                case '?':
                case '(':
                case ')':
                case '[':
                case ']':
                case '{':
                case '}':
                case '|':
                case '"':
                    appendTokenString(ch);
                    t.id = Token::ID::RX_CLASS_RANGE;
                    break;
                default:
                    throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_ESC_CHAR");
                }

                state = State::Regex;
                stream.consume();
                return;
            case State::RegexEscHex2:
                setTokenString(ch);
                if(isHEX(ch)) {
                    state = State::RegexEscHex21;
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHex21:
                appendTokenString(ch);
                if(isHEX(ch)) {
                    return rxmatch(Token::ID::RX_ESC_CLASS_HEX, ch);
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHex4:
                if(ch == '{') {
                    resetTokenString();
                    state = State::RegexEscHexInitN;
                    stream.consume();
                    break;
                }
                setTokenString(ch);
                if(isHEX(ch)) {
                    state = State::RegexEscHex41;
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHex41:
                appendTokenString(ch);
                if(isHEX(ch)) {
                    state = State::RegexEscHex42;
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHex42:
                appendTokenString(ch);
                if(isHEX(ch)) {
                    state = State::RegexEscHex43;
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHex43:
                appendTokenString(ch);
                if(isHEX(ch)) {
                    return rxmatch(Token::ID::RX_ESC_CLASS_HEX, ch);
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHexInitN:
                appendTokenString(ch);
                if(isHEX(ch)) {
                    state = State::RegexEscHexN;
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");

            case State::RegexEscHexN:
                if(ch == '}') {
                    return rxmatch(Token::ID::RX_ESC_CLASS_HEX, ch);
                }

                appendTokenString(ch);
                if(isHEX(ch)) {
                    stream.consume();
                    break;
                }
                throw GeneratorError(__LINE__, __FILE__, stream.pos, "INVALID_REGEX_HEX_CHAR");
            }
        }

        t.id = Token::ID::END;
        return;
    }
};

/// @brief The Parser class
struct Parser {
    /// @brief list of fallback Tokens for a Token
    struct Fallback {
        /// @brief The token
        Token token;

        /// @brief The associated fallback tokens
        std::vector<Token> tokens;
    };

    /// @brief transient structure to store the function type for semantic actions for a ruleset
    /// This same struct is used for function sigs, as well as for codeblocks
    struct RuleSetType {
        /// @brief The ruleset name
        Token rs;

        /// @brief The rule, if this is a codeblock
        const ygp::Rule* rule = nullptr;

        /// @brief The associated Walker
        yg::Walker* walker = nullptr;

        /// @brief whether this is a user-defined function
        bool isUDF = false;

        /// @brief whether this function is autowalk-enabled
        bool autowalk = false;

        /// @brief The function name
        std::string func;

        /// @brief Additional function args
        std::string args;

        /// @brief The type, in case of function sigs, or codeblock in case of codeblocks
        Token data;
    };

    /// @brief The yg::Grammar class that is updated by this parser
    yg::Grammar& grammar;

    /// @brief The Lexer class used by this Parser
    Lexer& lexer;

    /// @brief List of explicitly specified rule precedences
    std::unordered_map<std::string, std::string> rulePrecedence;

    /// @brief List of fallbacks for a token
    std::unordered_map<std::string, Fallback> fallbacks;

    /// @brief The current lexer mode
    std::string lexerMode;

    /// @brief The current regex class depth
    size_t classDepth = 0;

    /// @brief The current regex level
    /// eg: `a` is at level 1
    /// in `a|b`, the a and b are at level 2
    /// used only for debugging
    size_t lvl = 0;

    /// @brief list of function sigs associated with a ruleset
    std::vector<RuleSetType> rsWalkerTypes;

    /// @brief list of codeblocks with a rule
    std::vector<RuleSetType> rsWalkerCodeblocks;

    inline Parser(yg::Grammar& g, Lexer& l) : grammar{g}, lexer{l} {}

    /// @brief get fallback entry for a token.
    /// create a new entry if not found.
    inline Fallback& getFallback(const Token& name) {
        if(auto it = fallbacks.find(name.text); it != fallbacks.end()) {
            return it->second;
        }
        auto& fb = fallbacks[name.text];
        fb.token = name;
        return fb;
    }

    /// @brief Search for a function sig (or codeblock) in the list specified by @arg list
    static inline RuleSetType*
    hasWalkerData(
        std::vector<RuleSetType>& list,
        const Token& rs,
        const ygp::Rule* rule,
        yg::Walker& w,
        const std::string& f
    ) {
        for(auto& rt : list) {
            if((rt.rs.text == rs.text) && (rt.walker == &w) &&(rt.rule == rule) && (rt.func == f)) {
                return &rt;
            }
        }
        return nullptr;
    }

    /// @brief Add function sig (or codeblock) to the list specified by @arg list
    static inline void
    _addRsWalkerData(
        std::vector<RuleSetType>& list,
        const Token& rs,
        const ygp::Rule* rule,
        yg::Walker& w,
        const bool& u,
        const std::string& f,
        const std::string& a,
        const bool& autowalk,
        const Token& d
    ) {
        list.push_back(RuleSetType());
        auto& rt = list.back();
        rt.rs = rs;
        rt.rule = rule;
        rt.walker = &w;
        rt.isUDF = u;
        rt.func = f;
        rt.args = a;
        rt.autowalk = autowalk;
        rt.data = d;
    }

    /// @brief Add function sig to the ruleset list
    inline void addRsWalkerType(
        const Token& rs,
        yg::Walker& w,
        const std::string& f,
        const std::string& a,
        const bool& autowalk,
        const Token& t
    ) {
        if(hasWalkerData(rsWalkerTypes, rs, nullptr, w, f) != nullptr) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "DUPLICATE_TYPE: {}/{}::{}", rs.text, w.name, f);
        }
        return _addRsWalkerData(rsWalkerTypes, rs, nullptr, w, true, f, a, autowalk, t);
    }

    /// @brief Add codeblock to the rule list
    inline void addRsWalkerCode(
        const Token& rs,
        ygp::Rule* rule,
        yg::Walker& w,
        const bool& n,
        const std::string& f,
        const Token& t
    ) {
        if(hasWalkerData(rsWalkerCodeblocks, rs, rule, w, f) != nullptr) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "DUPLICATE_CODEBLOCK: {}/{}::{}", rs.text, w.name, f);
        }
        if(hasWalkerData(rsWalkerTypes, rs, nullptr, w, f) == nullptr) {
            if((f.size() > 0) && (f != w.defaultFunctionName)) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_FUNCTION: {}/{}::{}", rs.text, w.name, f);
            }
        }
        return _addRsWalkerData(rsWalkerCodeblocks, rs, rule, w, n, f, "", false, t);
    }

    /// @brief Check if @arg name is a valid rule name
    /// The first character of the name must be lowercase
    inline bool isRuleName(const std::string& name) {
        if(std::isupper(name.at(0)) == 0) {
            return true;
        }
        return false;
    }

    /// @brief Check if @arg name is a valid token name
    /// The name must be all uppercase
    inline bool isRegexName(const std::string& name) {
        for(auto& ch : name) {
            if(std::islower(ch) != 0) {
                return false;
            }
        }
        return true;
    }

    /// @brief add a rule to the grammar
    inline void addRule(const Token& t, const Token& name, std::unique_ptr<ygp::Rule>& rule, const bool& anchorSet) {
        assert(rule);

        // there are no nodes in the rule, add an empty node
        if(rule->nodes.size() == 0) {
            rule->addRegexNode(t.pos, grammar.empty);
        }
        grammar.addRule(name.text, rule, anchorSet);
    }

    /// @brief peek current token in the lexer.
    /// this is a wrapper around lexer.peek(), but additionally logs the peeked token for debugging purposes
    inline const Token& peek(const Tracer& tr) {
        auto& t = lexer.peek();
        unused(tr);
        log("{:>3}:>parser: lvl={}, s={}, tok={}, text=[{}], pos={}", lexer.stream.pos.str(), lvl, tr.name, Token::sname(t), t.text, t.pos.str());
        return t;
    }

    /// @brief create a regex atom instance
    inline std::unique_ptr<yglx::Atom> make_atom(yglx::Atom_t&& a) {
        return std::make_unique<yglx::Atom>(std::move(a));
    }

    /// @brief create a regex atom instance and advance the lexer to next token
    inline std::unique_ptr<yglx::Atom> make_atom_leaf(yglx::Atom_t&& a) {
        lexer.next();
        return make_atom(std::move(a));
    }

    /// @brief read a regex primitive atom
    /// includes single chars, char ranges, esc classes, hex digits, etc
    inline yglx::Primitive::Atom_t primitive_atomx(const Token& t) {
        Tracer tr{lvl, "primitive_atomx"};

        switch(t.id) {
        case Token::ID::RX_ESC_CLASS_HEX: {
            uint32_t ch = 0;
            for(auto c : t.text) {
                if((c >= '0') && (c <= '9')) {
                    c = c - '0';
                }else if((c >= 'a') && (c <= 'f')) {
                    c = c - 'a' + 10;
                }else if((c >= 'A') && (c <= 'F')) {
                    c = c - 'A' + 10;
                }else {
                    throw GeneratorError(__LINE__, __FILE__, lexer.stream.pos, "INVALID_HEX");
                }
                ch = (ch * 16) + static_cast<uint32_t>(c);
            }
            return yglx::RangeClass(ch, ch);
        }
        case Token::ID::RX_WILDCARD: {
            return yglx::WildCard();
        }
        case Token::ID::RX_CLASS_RANGE: {
            uint32_t ch1 = static_cast<uint32_t>(t.text.at(0));
            uint32_t ch2 = static_cast<uint32_t>(t.text.at(1));
            return yglx::RangeClass(ch1, ch2);
        }
        case Token::ID::RX_ESC_CLASS_DIGIT: {
            return yglx::LargeEscClass(grammar, "isDigit");
        }
        case Token::ID::RX_ESC_CLASS_NOT_DIGIT: {
            return yglx::LargeEscClass(grammar, "!isDigit");
        }
        case Token::ID::RX_ESC_CLASS_LETTER: {
            return yglx::LargeEscClass(grammar, "isLetter");
        }
        case Token::ID::RX_ESC_CLASS_NOT_LETTER: {
            return yglx::LargeEscClass(grammar, "!isLetter");
        }
        case Token::ID::RX_ESC_CLASS_WORD: {
            return yglx::LargeEscClass(grammar, "isWord");
        }
        case Token::ID::RX_ESC_CLASS_NOT_WORD: {
            return yglx::LargeEscClass(grammar, "!isWord");
        }
        case Token::ID::RX_ESC_CLASS_SPACE: {
            return yglx::LargeEscClass(grammar, "isSpace");
        }
        case Token::ID::RX_ESC_CLASS_NOT_SPACE: {
            return yglx::LargeEscClass(grammar, "!isSpace");
        }
        case Token::ID::RX_ESC_CLASS_WBOUNDARY: {
            return yglx::LargeEscClass(grammar, "isWBoundary");
        }
        case Token::ID::RX_ESC_CLASS_NOT_WBOUNDARY: {
            return yglx::LargeEscClass(grammar, "!isWBoundary");
        }
        default:
            throw GeneratorError(__LINE__, __FILE__, lexer.stream.pos, "INVALID_INPUT");
        }
    }

    /// @brief read a regex primitive and create a leaf atom
    inline std::unique_ptr<yglx::Atom> primitive_atom() {
        Tracer tr{lvl, "primitive_atom"};

        Token t = peek(tr);
        if(t.id == Token::ID::RX_DBLQUOTE) {
            throw GeneratorError(__LINE__, __FILE__, lexer.stream.pos, "INVALID_INPUT");
        }
        auto ax = primitive_atomx(t);
        return make_atom_leaf(yglx::Primitive(t.pos, std::move(ax)));
    }

    /// @brief read a regex class
    /// [a-z], [^a-z], etc
    inline std::unique_ptr<yglx::Atom> class_atoms() {
        Tracer tr{lvl, "class_atoms"};

        Token t;
        if((t = peek(tr)).id == Token::ID::RX_CLASS_ENTER) {
            assert(classDepth == 0);
            lexer.next();
            bool negate = false;
            if((t = peek(tr)).id == Token::ID::RX_CLASS_CARET) {
                lexer.next();
                negate = true;
            }

            std::vector<yglx::Primitive::Atom_t> atoms;
            while((t = peek(tr)).id != Token::ID::RX_CLASS_LEAVE) {
                ++classDepth;
                yglx::Primitive::Atom_t rhs = primitive_atomx(t);
                --classDepth;
                atoms.push_back(rhs);
                lexer.next();
            }
            return make_atom_leaf(yglx::Class{t.pos, negate, std::move(atoms)});
        }
        return primitive_atom();
    }

    /// @brief read a regex class
    /// (abc), (!abc), etc
    inline std::unique_ptr<yglx::Atom> group_atoms() {
        Tracer tr{lvl, "group_atoms"};

        Token t;
        if((t = peek(tr)).id == Token::ID::RX_GROUP_ENTER) {
            lexer.next();
            bool capture = true;
            if((t = peek(tr)).id == Token::ID::RX_GROUP_DONT) {
                lexer.next();
                capture = false;
            }

            auto atom = atomx();

            if((t = peek(tr)).id != Token::ID::RX_GROUP_LEAVE) {
                throw GeneratorError(__LINE__, __FILE__, lexer.stream.pos, "INVALID_INPUT");
            }

            return make_atom_leaf(yglx::Group{t.pos, capture, std::move(atom)});
        }
        return class_atoms();
    }

    /// @brief read a regex closure
    /// a*, a+, a?, etc
    inline std::unique_ptr<yglx::Atom> atom_closure() {
        Tracer tr{lvl, "atom_closure"};

        auto lhs = group_atoms();
        Token t;
        if((t = peek(tr)).id != Token::ID::RX_DBLQUOTE) {
            switch(t.id) {
            case Token::ID::RX_CLOSURE_STAR: {
                lhs = make_atom_leaf(yglx::Closure{grammar, t.pos, std::move(lhs), 0, grammar.maxRepCount});
                break;
            }
            case Token::ID::RX_CLOSURE_PLUS: {
                lhs = make_atom_leaf(yglx::Closure{grammar, t.pos, std::move(lhs), 1, grammar.maxRepCount});
                break;
            }
            case Token::ID::RX_CLOSURE_QUESTION: {
                lhs = make_atom_leaf(yglx::Closure{grammar, t.pos, std::move(lhs), 0, 1});
                break;
            }
            case Token::ID::RX_CLOSURE_ENTER: {
                lexer.next();
                size_t min = 0;
                size_t max = 0;
                bool hasMin = false;
                if((t = peek(tr)).id == Token::ID::RX_CLOSURE_VALUE) {
                    min = std::stoul(t.text);
                    hasMin = true;
                    lexer.next();
                }

                if((t = peek(tr)).id != Token::ID::RX_CLOSURE_LEAVE) {
                    throw GeneratorError(__LINE__, __FILE__, lexer.stream.pos, "INVALID_INPUT");
                }

                if(hasMin) {
                    max = std::stoul(t.text);
                }else{
                    max = std::stoul(t.text);
                    min = 0;
                }
                lhs = make_atom_leaf(yglx::Closure{grammar, t.pos, std::move(lhs), min, max});
                break;
            }
            default:
                return lhs;
            }
        }
        return lhs;
    }

    /// @brief read two consequtive atoms
    /// ab, (abc)(def), etc
    inline std::unique_ptr<yglx::Atom> atom_and_atom() {
        Tracer tr{lvl, "atom_and_atom"};

        auto lhs = atom_closure();
        if(classDepth > 0) {
            return lhs;
        }

        Token t;
        while((t = peek(tr)).id != Token::ID::RX_DBLQUOTE) {
            switch(t.id) {
            case Token::ID::RX_WILDCARD:
            case Token::ID::RX_GROUP_ENTER:
            case Token::ID::RX_CLASS_ENTER:
            case Token::ID::RX_CLASS_RANGE:
            case Token::ID::RX_ESC_CLASS_DIGIT:
            case Token::ID::RX_ESC_CLASS_NOT_DIGIT:
            case Token::ID::RX_ESC_CLASS_LETTER:
            case Token::ID::RX_ESC_CLASS_NOT_LETTER:
            case Token::ID::RX_ESC_CLASS_WORD:
            case Token::ID::RX_ESC_CLASS_NOT_WORD:
            case Token::ID::RX_ESC_CLASS_SPACE:
            case Token::ID::RX_ESC_CLASS_NOT_SPACE:
            case Token::ID::RX_ESC_CLASS_WBOUNDARY:
            case Token::ID::RX_ESC_CLASS_NOT_WBOUNDARY:
            case Token::ID::RX_ESC_CLASS_HEX: {
                auto rhs = atomx();
                lhs = make_atom(yglx::Sequence{t.pos, std::move(lhs), std::move(rhs)});
                break;
            }
            default:
                return lhs;
            }
        }
        return lhs;
    }

    /// @brief read two alternate atoms
    /// a|b, (abc)|(def), etc
    inline std::unique_ptr<yglx::Atom> atom_or_atom() {
        Tracer tr{lvl, "atom_or_atom"};

        auto lhs = atom_and_atom();
        Token t;
        while((t = peek(tr)).id == Token::ID::RX_DISJUNCT) {
            lexer.next();
            auto rhs = atomx();
            lhs = make_atom(yglx::Disjunct{t.pos, std::move(lhs), std::move(rhs)});
        }
        return lhs;
    }

    /// @brief read a regex atom recursively, incrementing the current lvl
    inline std::unique_ptr<yglx::Atom> atomx() {
        Tracer tr{lvl, "atomx"};
        ++lvl;
        auto lhs = atom_or_atom();
        --lvl;
        return lhs;
    }

    /// @brief read and consume a semi-colon at the end of a grammar line
    inline void read_semi(const Tracer& tr) {
        Token t = peek(tr);
        if(t.id != Token::ID::SEMI) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();
    }

    /// @brief set @arg text to next ID read from the lexer
    inline void set_string(std::string& text, const Token& s) {
        Tracer tr{lvl, s.text};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        text = t.text;
        lexer.next();
        read_semi(tr);
    }

    /// @brief set boolean value @arg b to next ID read from the lexer
    /// set it to true if ID is 'on', false if ID is 'off'.
    inline void set_bool(bool& b, const Token& s) {
        Tracer tr{lvl, s.text};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        if(t.text == "on") {
            b = true;
        }else if (t.text == "off") {
            b = false;
        } else {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_PRAGMA_VALUE:{}, should be 'on' or 'off'", t.text);
        }
        lexer.next();
        read_semi(tr);
    }

    /// @brief set codeblock @arg cb to next CODEBLOCK read by the Lexer
    inline void set_codeblock(CodeBlock& cb, const Token& s) {
        Tracer tr{lvl, s.text};

        if(cb.hasCode()) {
            throw GeneratorError(__LINE__, __FILE__, s.pos, "INVALID_PRAGMA:{}, already defined", s.text);
        }
        Token t = peek(tr);
        if(t.id != Token::ID::CODEBLOCK) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        cb.setCode(t.pos, t.text);
        lexer.next();
    }

    /// @brief read module class member and set it in the grammar
    inline void class_member() {
        Tracer tr{lvl, "class_member"};

        lexer.setMode_Type();
        lexer.next();

        Token t = peek(tr);
        if(t.id != Token::ID::TYPE) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        grammar.classMembers.push_back(t.text);
        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::SEMI) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();
    }

    /// @brief read pragma value for `default_walker` and set it in the grammar
    /// this pragma can be set after only list of walkers are defined
    inline void default_walker() {
        Tracer tr{lvl, "default_walker"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        grammar.setDefaultWalker(t.pos, t.text);
        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::SEMI) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();
    }

    /// @brief read list of walkers and set them in the grammar
    inline void walkers() {
        Tracer tr{lvl, "walkers"};
        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        grammar.resetWalkers();
        while((t = peek(tr)).id == Token::ID::ID) {
            // cppServer(X)
            // ^
            auto walkerID = t;
            const yg::Walker* base = nullptr;

            lexer.next();
            t = peek(tr);
            if(t.id == Token::ID::LBRACKET) {
                // cppServer(X)
                //          ^
                lexer.next();
                t = peek(tr);
                if(t.id != Token::ID::ID) {
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
                }
                if(t.id == Token::ID::ID) {
                    // cppServer(X)
                    //           ^
                    base = grammar.getWalker(t.text);
                    if(base == nullptr) {
                        throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_WALKER_BASE:{}", t.text);
                    }
                    lexer.next();
                    t = peek(tr);
                }else{
                    assert(t.id == Token::ID::RBRACKET);
                    // cppServer()
                    //           ^
                    // if empty brackets, use default walker as base
                    base = grammar.hasDefaultWalker(); //may or may not be nullptr
                }
                if(t.id != Token::ID::RBRACKET) {
                    // cppServer(X)
                    //            ^
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
                }
                lexer.next();
            }
            grammar.addWalker(walkerID.text, base);
        }
        if(t.id != Token::ID::SEMI) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();
    }

    /// @brief read walker_output pragma and set it in the specified walker
    inline void walker_output() {
        Tracer tr{lvl, "walker_output"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        auto walker = grammar.getWalker(t.text);
        if(walker == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_WALKER:{}", t.text);
        }

        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        if(t.text != "text_file") {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_OUTPUT_TYPE:[{}]", t.text);
        }

        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        walker->setOutputTextFile(t.text);
        lexer.next();
        read_semi(tr);
    }

    /// @brief read walker_traversal mode pragma and set it in the specified walker
    /// the mode can be manual or top_down
    inline void walker_traversal() {
        Tracer tr{lvl, "walker_traversal"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        auto walker = grammar.getWalker(t.text);
        if(walker == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_WALKER:{}", t.text);
        }

        if(grammar.isRootWalker(*walker) == false) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "WALKER_NOT_ROOT:{}", t.text);
        }

        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        if(t.text == "manual") {
            walker->setTraversalMode(yg::Walker::TraversalMode::Manual);
        }else if(t.text == "top_down") {
            walker->setTraversalMode(yg::Walker::TraversalMode::TopDown);
        }else{
            throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_TRAVERSAL_MODE:{}", t.text);
        }

        lexer.next();
        read_semi(tr);
    }

    /// @brief read additional class members for the specified walker
    inline void walker_members() {
        Tracer tr{lvl, "walker_members"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        auto walker = grammar.getWalker(t.text);
        if(walker == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_WALKER:{}", t.text);
        }

        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::CODEBLOCK) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        walker->xmembers.setCode(t.pos, t.text);
        lexer.next();
    }

    /// @brief read explicitly specified token precdence values
    inline void set_precedence(const yglx::RegexSet::Assoc& assoc, const Token& s) {
        Tracer tr{lvl, s.text};

        Token t;

        auto precedence = grammar.getNextPrecedence();
        while((t = peek(tr)).id == Token::ID::ID) {
            if(isRegexName(t.text) == false) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_PRAGMA_VALUE:{}, should be TOKEN name", t.text);
            }
            grammar.addRegexSet(t.text, assoc, precedence);
            lexer.next();
        }

        read_semi(tr);
    }

    /// @brief read single header value and set @arg text
    inline void set_header(std::string& text, const Token& s) {
        Tracer tr{lvl, s.text};

        lexer.setMode_Header();
        lexer.next();
        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        text = t.text;
        lexer.next();
        read_semi(tr);
    }

    /// @brief read single header value and add it to @arg hdr
    inline void add_header(std::vector<std::string>& hdr, const Token& s) {
        Tracer tr{lvl, s.text};

        lexer.setMode_Header();
        lexer.next();
        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        hdr.push_back(t.text);
        lexer.next();
        read_semi(tr);
    }

    /// @brief read fallback list for a token
    inline void set_fallback() {
        Tracer tr{lvl, "fallback"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        auto& fb = getFallback(t);
        lexer.next();
        while((t = peek(tr)).id == Token::ID::ID) {
            fb.tokens.push_back(t);
            lexer.next();
        }

        if(t.id != Token::ID::SEMI) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();
    }

    /// @brief read encoding
    inline void set_encoding() {
        Tracer tr{lvl, "encoding"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        if(t.text == "utf8") {
            grammar.unicodeEnabled = true;
        }else if(t.text == "ascii") {
            grammar.unicodeEnabled = false;
        }else{
            throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_ENCODING:{}", t.text);
        }

        lexer.next();
        read_semi(tr);
    }

    /// @brief read pragma to change lexer mode
    inline void set_lexermode() {
        Tracer tr{lvl, "lexermode"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexerMode = t.text;
        grammar.addLexerMode(t.pos, lexerMode);
        lexer.next();
        read_semi(tr);
    }

    /// @brief read pragma to set ruleset function sig
    inline void set_function() {
        Tracer tr{lvl, "function"};

        //%function expr Walker::eval() -> int;
        //          ^
        Token rname = peek(tr);
        if(rname.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, rname.pos, "INVALID_INPUT");
        }

        lexer.next();

        auto walker = grammar.hasDefaultWalker();
        if(walker == nullptr) {
            throw GeneratorError(__LINE__, __FILE__, rname.pos, "NO_DEFAULT_WALKER");
        }
        std::string func;

        Token t = peek(tr);
        if(t.id == Token::ID::ID) {
            //%function expr Walker::eval() -> int;
            //               ^
            Token wname = peek(tr);

            lexer.next();

            t = peek(tr);
            if(t.id == Token::ID::DBLCOLON) {
                //%function expr Walker::eval() -> int;
                //                     ^
                walker = grammar.getWalker(wname.text);
                if(walker == nullptr) {
                    throw GeneratorError(__LINE__, __FILE__, wname.pos, "UNKNOWN_WALKER:{}", wname.text);
                }

                lexer.next();

                //%function expr Walker::eval() -> int;
                //                       ^
                t = peek(tr);
                if(t.id != Token::ID::ID) {
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
                }
                func = t.text;
                lexer.next();

            }else{
                assert(t.id == Token::ID::LBRACKET);
                //%function expr eval() -> int;
                //                   ^
                func = wname.text;
            }

            //%function expr Walker::eval() -> int;
            //                           ^
            t = peek(tr);
            if(t.id != Token::ID::LBRACKET) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }

            lexer.setMode_Args();
            lexer.next();

            //%function expr Walker::eval(int x) -> int;
            //                            ^
            Token args = peek(tr);
            if(args.id != Token::ID::ARGS) {
                throw GeneratorError(__LINE__, __FILE__, args.pos, "INVALID_INPUT");
            }

            lexer.next();

            //%function expr Walker::eval(int x) -> int;
            //                                 ^
            t = peek(tr);
            if(t.id != Token::ID::RBRACKET) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }

            lexer.next();

            bool autowalk = false;
            //%function expr Walker::eval() -> int;
            //                              ^
            t = peek(tr);
            if(t.id == Token::ID::POINTER) {
            }else if(t.id == Token::ID::DBLPOINTER) {
                autowalk = true;
            }else{
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }

            lexer.setMode_Type();
            lexer.next();

            //%function expr Walker::eval() -> int;
            //                                 ^
            Token rtype = peek(tr);
            if(rtype.id != Token::ID::TYPE) {
                throw GeneratorError(__LINE__, __FILE__, rtype.pos, "INVALID_INPUT");
            }

            addRsWalkerType(rname, *walker, func, args.text, autowalk, rtype);
        }else{
            bool autowalk = false;

            //%function expr -> int;
            //               ^
            if(t.id == Token::ID::POINTER) {
            }else if(t.id == Token::ID::DBLPOINTER) {
                autowalk = true;
            }else{
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }

            lexer.setMode_Type();
            lexer.next();

            //%function expr -> int;
            //                  ^
            Token rtype = peek(tr);
            if(rtype.id != Token::ID::TYPE) {
                throw GeneratorError(__LINE__, __FILE__, rtype.pos, "INVALID_INPUT");
            }

            addRsWalkerType(rname, *walker, walker->defaultFunctionName, "", autowalk, rtype);
        }

        lexer.next();
        read_semi(tr);
    }

    /// @brief read pragmas
    inline void begin_pragma() {
        Tracer tr{lvl, "pragma"};

        Token t = peek(tr);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        if(t.text == "pch_header") {
            return set_header(grammar.pchHeader, t);
        }

        if(t.text == "hdr_header") {
            return add_header(grammar.hdrHeaders, t);
        }

        if(t.text == "src_header") {
            return add_header(grammar.srcHeaders, t);
        }

        if(t.text == "class_member") {
            return class_member();
        }

        lexer.next();
        if(t.text == "namespace") {
            return set_string(grammar.ns, t);
        }

        if(t.text == "class") {
            return set_string(grammar.className, t);
        }

        if(t.text == "encoding") {
            return set_encoding();
        }

        if(t.text == "check_unused_tokens") {
            return set_bool(grammar.checkUnusedTokens, t);
        }

        if(t.text == "auto_resolve") {
            return set_bool(grammar.autoResolve, t);
        }

        if(t.text == "warn_resolve") {
            return set_bool(grammar.warnResolve, t);
        }

        if(t.text == "std_header") {
            return set_bool(grammar.stdHeadersEnabled, t);
        }

        if(t.text == "default_walker") {
            return default_walker();
        }
        if(t.text == "walkers") {
            return walkers();
        }

        if(t.text == "walker_output") {
            return walker_output();
        }

        if(t.text == "walker_traversal") {
            return walker_traversal();
        }

        if(t.text == "members") {
            return walker_members();
        }

        if(t.text == "public") {
            //TODO: code that goes into Module class
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        if(t.text == "prologue") {
            return set_codeblock(grammar.prologue, t);
        }

        if(t.text == "epilogue") {
            return set_codeblock(grammar.epilogue, t);
        }

        if(t.text == "error") {
            return set_codeblock(grammar.throwError, t);
        }

        if(t.text == "start") {
            return set_string(grammar.start, t);
        }

        if(t.text == "left") {
            return set_precedence(yglx::RegexSet::Assoc::Left, t);
        }

        if(t.text == "right") {
            return set_precedence(yglx::RegexSet::Assoc::Right, t);
        }

        if(t.text == "token") {
            return set_precedence(yglx::RegexSet::Assoc::None, t);
        }

        if(t.text == "fallback") {
            return set_fallback();
        }

        if(t.text == "function") {
            return set_function();
        }

        if(t.text == "lexer_mode") {
            return set_lexermode();
        }

        throw GeneratorError(__LINE__, __FILE__, t.pos, "UNKNOWN_PRAGMA:{}", t.text);
    }

    /// @brief read a single node of a rule
    inline void parse_node(ygp::Rule& rule) {
        Tracer tr{lvl, "parse_node"};

        //rule(myrule) := ^node1;
        //                 ^
        Token t = peek(tr);
        unused(rule);
        if(t.id != Token::ID::ID) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        auto& node = rule.addNode(t.pos, t.text, ygp::Node::NodeType::RegexRef);
        if(isRegexName(t.text) == true) {
            node.type = ygp::Node::NodeType::RegexRef;
        }else if(isRuleName(t.text) == true) {
            node.type = ygp::Node::NodeType::RuleRef;
        }else{
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_RULE_NAME");
        }
        lexer.next();

        //rule(myrule) := ^node1(ID);
        //                      ^
        t = peek(tr);
        if(t.id == Token::ID::LBRACKET) {
            lexer.next();
            t = peek(tr);

            //rule(myrule) := ^node1(ID);
            //                       ^
            if(t.id != Token::ID::ID) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }

            if(node.isRule() == true) {
                if(isRuleName(t.text) == false) {
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_RULE_VARNAME:{}", t.text);
                }
            }else{
                assert(node.isRegex() == true);
                if(isRegexName(t.text) == false) {
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_REGEX_VARNAME:{}", t.text);
                }
            }

            node.varName = t.text;
            lexer.next();
            t = peek(tr);

            //rule(myrule) := ^node1(ID);
            //                         ^
            if(t.id != Token::ID::RBRACKET) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
            lexer.next();
        }
    }

    /// @brief read a rule
    inline void parse_rule(const Token& ruleName) {
        Tracer tr{lvl, "parse_rule"};

        std::string nativeName;
        Token t = peek(tr);
        if(t.id == Token::ID::LBRACKET) {
            //rule(myrule) := node1;
            //    ^
            lexer.next();

            //rule(myrule) := node1;
            //     ^
            t = peek(tr);
            if(t.id != Token::ID::ID) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
            nativeName = t.text;
            lexer.next();

            //rule(myrule) := node1;
            //           ^
            t = peek(tr);
            if(t.id != Token::ID::RBRACKET) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
            lexer.next();
        }

        //rule(myrule) := node1;
        //             ^
        t = peek(tr);
        if(t.id != Token::ID::COLONEQ) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();
        auto rule = std::make_unique<ygp::Rule>();
        rule->pos = ruleName.pos;
        rule->ruleName = nativeName;
        bool anchorSet = false;

        while(true) {
            //rule(myrule) := ^node1;
            //                ^
            t = peek(tr);
            if(t.id == Token::ID::CARET) {
                anchorSet = true;
                lexer.next();
                t = peek(tr);
            }
            if(t.id != Token::ID::ID) {
                break;
            }
            parse_node(*rule);
        }

        //rule(myrule) := ^node1 [PLUS];
        //                       ^
        if(t.id == Token::ID::LSQUARE) {
            lexer.next();
            t = peek(tr);

            //rule(myrule) := ^node1 [PLUS];
            //                        ^
            if(t.id != Token::ID::ID) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
            if(isRegexName(t.text) == false) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_TOKEN_REF");
            }
            if(rulePrecedence.contains(rule->ruleSetName())) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "DUPLICATE_RULE_PRECEDENCE");
            }
            rulePrecedence[rule->ruleSetName()] = t.text;

            lexer.next();
            t = peek(tr);

            //rule(myrule) := ^node1 [PLUS];
            //                            ^
            if(t.id != Token::ID::RSQUARE) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }

            lexer.next();
            t = peek(tr);
        }

        bool hasCodeBlocks = false;
        while((t.id == Token::ID::CODEBLOCK) || (t.id == Token::ID::AT)) {
            auto w = grammar.hasDefaultWalker();
            if(w == nullptr) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "NO_DEFAULT_WALKER");
            }
            auto func = w->defaultFunctionName;
            bool isUDF = false;

            if(t.id == Token::ID::AT) {
                lexer.next();
                t = peek(tr);

                // @CppServer
                //  ^
                if(t.id != Token::ID::ID) {
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
                }
                auto walkerID = t;
                lexer.next();
                t = peek(tr);

                // @CppServer::
                //           ^
                if(t.id == Token::ID::DBLCOLON) {
                    w = grammar.getWalker(walkerID.text);
                    if(w == nullptr) {
                        throw GeneratorError(__LINE__, __FILE__, walkerID.pos, "UNKNOWN_GENERATOR:{}", walkerID.text);
                    }

                    lexer.next();
                    t = peek(tr);

                    // @CppServer::str
                    //             ^
                    if(t.id != Token::ID::ID) {
                        throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
                    }

                    func = t.text;
                    lexer.next();
                    t = peek(tr);
                }else{
                    auto w1 = grammar.getWalker(walkerID.text);
                    if(w1 != nullptr) {
                        w = w1;
                        func = w1->defaultFunctionName;
                    }else{
                        func = walkerID.text;
                    }

                    assert(t.id == Token::ID::CODEBLOCK);
                }

                // @CppServer %{ %}
                //            ^
                if(t.id != Token::ID::CODEBLOCK) {
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
                }
                isUDF = true;
            }

            assert(t.id == Token::ID::CODEBLOCK);
            assert(w != nullptr);
            addRsWalkerCode(ruleName, rule.get(), *w, isUDF, func, t);
            hasCodeBlocks = true;
            lexer.next();
            t = peek(tr);
        }


        if(hasCodeBlocks == false) {
            if(t.id != Token::ID::SEMI) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
            lexer.next();
        }

        addRule(t, ruleName, rule, anchorSet);
    }

    /// @brief read a token
    inline void parse_regex(const Token& ruleName) {
        Tracer tr{lvl, "parse_regex"};

        Token t = peek(tr);
        auto assoc = yglx::RegexSet::Assoc::Right;
        switch(t.id) {
        case Token::ID::COLONEQ:
            assoc = yglx::RegexSet::Assoc::Right;
            break;
        case Token::ID::COLONEQGT:
            assoc = yglx::RegexSet::Assoc::Left;
            break;
        case Token::ID::COLONEQEQ:
            assoc = yglx::RegexSet::Assoc::None;
            break;
        default:
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        auto regex = std::make_unique<yglx::Regex>();
        regex->pos = ruleName.pos;
        regex->regexName = ruleName.text;
        regex->mode = lexerMode;
        regex->nextMode = lexerMode;

        lexer.next();
        t = peek(tr);
        if(t.id != Token::ID::RX_DBLQUOTE) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }
        lexer.next();

        assert(lvl == 0);
        auto atom = atomx();
        assert(lvl == 0);
        regex->atom = std::move(atom);

        lexer.next();
        t = peek(tr);

        if(t.id == Token::ID::BANG) {
            regex->unused = true;
            lexer.next();
            t = peek(tr);
        }

        if(t.id == Token::ID::LSQUARE) {
            lexer.next();
            t = peek(tr);

            regex->modeChange = yglx::Regex::ModeChange::Init;
            regex->nextMode = "";

            if(t.id == Token::ID::ID) {
                regex->modeChange = yglx::Regex::ModeChange::Next;
                regex->nextMode = t.text;
                lexer.next();
                t = peek(tr);
            }else if(t.id == Token::ID::CARET){
                regex->modeChange = yglx::Regex::ModeChange::Back;
                regex->nextMode = "";
                lexer.next();
                t = peek(tr);
            }

            if(t.id != Token::ID::RSQUARE) {
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
            lexer.next();
            t = peek(tr);
        }

        if(t.id != Token::ID::SEMI) {
            throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
        }

        grammar.addRegex(regex, assoc);
        lexer.next();
    }

    /// @brief read input, line by line and parse each line
    /// Every line in a grammar file defines either a pragma, a rule or a token.
    inline void parseInput() {
        Tracer tr{lvl, "parse"};

        // add initial lexer mode to grammar
        grammar.addLexerMode(lexer.stream.pos, lexerMode);

        // read .y file line by line
        Token t;
        while((t = peek(tr)).id != Token::ID::END) {
            switch(t.id) {
            case Token::ID::PERCENT:
                // first char is %, read the rest of the line as a pragma
                lexer.next();
                begin_pragma();
                break;
            case Token::ID::ID:
                lexer.next();
                // use `t` to decide whether to read the rest of the line as a rule or a token
                if(isRuleName(t.text) == true) {
                    parse_rule(t);
                }else if(isRegexName(t.text) == true) {
                    parse_regex(t);
                }else{
                    throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_RULE_NAME:{}", t.text);
                }
                break;
            default:
                throw GeneratorError(__LINE__, __FILE__, t.pos, "INVALID_INPUT");
            }
        }

        // grammar file fully parsed. Now add all function sigs to the walker
        for(auto& rt : rsWalkerTypes) {
            auto& rs = grammar.getRuleSetByName(rt.rs.pos, rt.rs.text);
            assert(rt.walker != nullptr);
            rt.walker->addFunctionSig(rt.data.pos, rs, rt.isUDF, rt.func, rt.args, rt.data.text, rt.autowalk);
        }

        // Add all codeblocks to the walker
        for(auto& rt : rsWalkerCodeblocks) {
            assert(rt.walker != nullptr);
            assert(rt.rule != nullptr);
            rt.walker->addCodeblock(rt.data.pos, *(rt.rule), rt.func, rt.data.text);
        }

        // set precedence for each rule
        for(auto& r : grammar.rules) {
            if(auto it = rulePrecedence.find(r->ruleSetName()); it != rulePrecedence.end()) {
                auto& rx = grammar.getRegexSetByName(r->pos, it->second);
                r->precedence = &rx;
            }
        }

        // set ruleName for each rule where it is not explicitly set
        for(auto& r : grammar.rules) {
            if(r->ruleName.size() == 0) {
                auto rsname = r->ruleSetName();
                if(rsname.ends_with('_') == true) {
                    rsname += "r";
                }
                r->ruleName = std::format("{}_{}", rsname, r->id);
            }
        }

    }
};
}

void parseInput(yg::Grammar& g, Stream& is) {
    // create virtual regex for END and EPSILON
    g.addRegexByName(g.end, yglx::RegexSet::Assoc::Right);
    g.addRegexByName(g.empty, yglx::RegexSet::Assoc::Right);

    g.addWalker(g.defaultWalkerClassName, nullptr);

    // parse the input file
    Lexer lexer(is);
    lexer.next();
    Parser p(g, lexer);
    p.parseInput();

    // resolve all fallbacks
    for(auto& f : p.fallbacks) {
        auto& fb = f.second;
        auto& t = g.getRegexSetByName(fb.token.pos, fb.token.text);
        for(auto& i : fb.tokens) {
            auto& ct = g.getRegexSetByName(i.pos, i.text);
            t.fallbacks.push_back(&ct);
        }
    }

    // add END regex at the end of all start rules
    for(auto& rule : g.rules) {
        if(rule->ruleSetName() == g.start) {
            rule->addRegexNode(rule->pos, g.end);
        }
    }

    // increment usageCount for all regexes
    for(auto& rule : g.rules) {
        for(auto& node : rule->nodes) {
            if(node->isRule()) {
                continue;
            }
            assert(node->isRegex());
            auto& t = g.getRegexSet(*node);

            for(auto& regex : t.regexes) {
                ++regex->usageCount;
            }

            // increment usageCount for all fallbacks as well
            for(auto& rs : t.fallbacks) {
                for(auto& regex : rs->regexes) {
                    ++regex->usageCount;
                }
            }
        }
    }

    std::vector<yglx::Regex*> unuseds;
    for(auto& regex : g.regexes) {
        if((regex->unused == false) && (regex->usageCount == 0) && (regex->regexName != g.empty)) {
            unuseds.push_back(regex.get());
        }
    }

    if((g.checkUnusedTokens == true) && (unuseds.size() > 0)) {
        std::string s;
        if(unuseds.size() > 1) {
            s = "S";
        }
        FilePos pos;
        std::string ftok;
        bool pset = false;
        std::stringstream uss;
        for(auto& u : unuseds) {
            uss << std::format("{}: {}\n", u->pos.str(), u->regexName);
            if(pset == false) {
                pos = u->pos;
                ftok = u->regexName;
                pset = true;
            }
        }
        throw GeneratorError(__LINE__, __FILE__, pos, "UNUSED_TOKEN{}:{}\n{}", s, ftok, uss.str());
    }

    for(auto& w : g.walkers) {
        w->init();
    }
}
