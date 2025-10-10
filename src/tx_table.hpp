#pragma once

template<typename RowT, typename ColT, typename CellT>
struct Table {
    struct Row {
        std::string name;
        std::vector<std::pair<ColT, CellT>> cols;
        inline void addCell(Table& t, const ColT& col, const CellT& val) {
            for(auto& c : cols) {
                if(c.first == col) {
                    assert(c.second == val);
                    return;
                }
            }
            cols.push_back(std::make_pair(col, val));
            for(auto& h : t.headers) {
                if(h.first == col) {
                    if(val.size() > h.second) {
                        h.second = val.size();
                    }
                    return;
                }
            }
            assert(false);
        }
    };
    std::vector<std::pair<ColT, size_t>> headers;
    std::vector<std::pair<RowT, Row>> rows;

    inline void addHeader(const std::string& name) {
        for(auto& h : headers) {
            if(h.first == name) {
                return;
            }
        }
        headers.push_back(std::make_pair(name, name.size()));
    }

    inline Row& addRow(const RowT& row, const std::string& name) {
        for(auto& r : rows) {
            if(r.first == row) {
                return r.second;
            }
        }
        rows.push_back(std::make_pair(row, Row()));
        auto& nrow = rows.back();
        nrow.second.name = name;
        return nrow.second;
    }

    inline std::string genMD() const {
        std::stringstream hs;
        hs << std::format("|   |");
        for(auto& h : headers) {
            hs << std::format("{}|", centre(h.first, h.second));
        }
        hs << std::endl;

        hs << std::format("|---|");
        for(auto& h : headers) {
            hs << std::string(h.second, '-') << "|";
        }
        hs << std::endl;

        for(auto& r : rows) {
            hs << std::format("|{}|", r.second.name);
            for(auto& h : headers) {
                std::string cell;
                for(auto& c : r.second.cols) {
                    if(h.first == c.first) {
                        cell = centre(c.second, h.second);
                        break;
                    }
                }
                if(cell.empty()) {
                    cell = centre("", h.second);
                }
                hs << std::format("{}|", cell);
            }
            hs << std::endl;
        }
        return hs.str();
    }

    static inline std::string centre(const std::string& string, const size_t& width, char fillchar = ' ') {
        if (width <= string.size()) {
            return string;
        }
        std::string fmt_str = std::format("{{:{}^{}}}", fillchar, width);
        return std::vformat(fmt_str, std::make_format_args(string));
    }
};
