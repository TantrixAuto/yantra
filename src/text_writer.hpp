///PROTOTYPE_ENTER:SKIP
#pragma once
#include "util.hpp"
///PROTOTYPE_LEAVE:SKIP

template<typename SS>
struct TextWriter {
    SS ss;
    std::string indent;
    size_t row = 1;
    bool wrote = false;

    inline void
    write(const char& ch) {
        ss << ch;
        wrote = true;
    }

    template <typename ...ArgsT>
    inline void
    write(const std::format_string<ArgsT...>& msg, ArgsT... args) {
        auto rv = std::format(msg, std::forward<ArgsT>(args)...);
        ss << indent << rv;
        ss.flush();
        wrote = true;
    }

    template <typename ...ArgsT>
    inline void
    writeln(const std::format_string<ArgsT...>& msg, ArgsT... args) {
        auto rv = std::format(msg, std::forward<ArgsT>(args)...);
        ss << indent << rv << "\n";
        ss.flush();
        ++row;
        wrote = true;
    }

    inline void
    writeln() {
        ss << "\n";
        ss.flush();
        ++row;
        wrote = true;
    }
};

struct StringStreamWriter : public TextWriter<std::ostringstream> {
};

struct TextFileWriter : public TextWriter<std::ofstream> {
    std::filesystem::path dir;
    std::filesystem::path file;

    inline std::filesystem::path
    buildOutputPath(
        std::filesystem::path inf,
        const std::filesystem::path& odir,
        const std::string& ext
    ) {
        inf = std::filesystem::absolute(inf);
        inf = inf.lexically_normal();
        dir = inf.parent_path();
        inf = odir / inf.filename();
        inf.replace_extension(ext);
        return inf;
    }

    inline void
    _open(const std::filesystem::path& fname) {
        if(fname.empty()) {
            return;
        }
        ss.open(fname);
        if(ss.is_open() == false) {
            throw std::runtime_error("unable to open output file:" + fname.string());
        }
        file = fname;
    }

    inline void
    open(const std::filesystem::path& fname) {
        return _open(fname);
    }

    inline void
    open(const std::filesystem::path& odir, const std::string_view& filename, const std::string& ext) {
        auto ofilename = buildOutputPath(filename, odir, ext);
        return _open(ofilename);
    }

    inline bool isOpen() const {
        return ss.is_open();
    }

    inline void
    swrite(const StringStreamWriter& sw) {
        if(sw.wrote == false) {
            return;
        }
        TextWriter::write("{}", sw.ss.str());
        row += (sw.row - 1);
    }

    inline void
    swriteln(const StringStreamWriter& sw) {
        if(sw.wrote == false) {
            return;
        }
        TextWriter::writeln("{}", sw.ss.str());
        row += (sw.row - 1);
    }
};

template<typename TW>
struct Indenter {
    TW& writer;
    std::string indent;
    inline Indenter(TW& w) : writer(w) {
        indent = writer.indent;
        writer.indent += "    ";
    }
    inline ~Indenter() {
        writer.indent = indent;
    }
    inline operator bool() const {
        return true;
    }
};

using StringStreamIndenter = Indenter<StringStreamWriter>;
using TextFileIndenter = Indenter<TextFileWriter>;
