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
    iwrite(const std::format_string<ArgsT...>& msg, ArgsT... args) {
        auto rv = std::format(msg, std::forward<ArgsT>(args)...);
        ss << indent << rv;
        ss.flush();
        wrote = true;
    }

    template <typename ...ArgsT>
    inline void
    write(const std::format_string<ArgsT...>& msg, ArgsT... args) {
        auto rv = std::format(msg, std::forward<ArgsT>(args)...);
        ss << rv;
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

    template <typename ...ArgsT>
    inline void
    xwriteln(const std::format_string<ArgsT...>& msg, ArgsT... args) {
        auto rv = std::format(msg, std::forward<ArgsT>(args)...);
        ss << rv << "\n";
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

    inline auto
    buildOutputPath(
        std::filesystem::path inf,
        const std::filesystem::path& odir,
        const std::string& ext
    ) -> std::filesystem::path {
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
        if(ss.is_open() == true) {
            ss.close();
        }
        ss.open(fname);
        if(ss.is_open() == false) {
            throw std::runtime_error("unable to open output file:" + fname.string());
        }
        row = 1;
        file = fname;
    }

    inline void
    open(const std::filesystem::path& fname) {
        _open(fname);
    }

    inline void
    open(const std::filesystem::path& odir, const std::string_view& filename, const std::string& ext) {
        auto ofilename = buildOutputPath(filename, odir, ext);
        _open(ofilename);
    }

    inline auto isOpen() const -> bool {
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
struct Indenter : public NonCopyable {
    TW& writer;
    std::string indent;

    explicit inline Indenter(TW& w) : writer(w) {
        indent = writer.indent; // NOLINT(cppcoreguidelines-prefer-member-initializer)
        writer.indent += "    ";
    }
    inline ~Indenter() {
        writer.indent = indent;
    }
    explicit inline operator bool() const {
        return true;
    }
};

using StringStreamIndenter = Indenter<StringStreamWriter>;
using TextFileIndenter = Indenter<TextFileWriter>;
