/// @file grammar_ygp.cpp
/// This file contains implementation of certain functions in the ygp namespace
#include "pch.hpp"
#include "grammar_ygp.hpp"
#include "encodings.hpp"

std::string ygp::Rule::str(const size_t& cpos, const bool& full) const {
    std::stringstream s;
    for(size_t idx = 0; idx <= nodes.size(); ++idx) {
        s << " ";
        if(idx == cpos) {
            s << ".";
        }
        if(idx == anchor) {
            s << "^";
        }
        if(idx < nodes.size()) {
            auto& node = getNode(idx);
            s << node.str();
        }else{
            s << "@";
        }
    }

    if(full == true) {
        assert(ruleSet != nullptr);
        s << ", {";
        std::string sep;
        for(auto& t : ruleSet->firsts) {
            s << sep << t->name;
            sep = " ";
        }
        s << "}, {";
        sep = "";
        for(auto& t : ruleSet->follows) {
            s << sep << t->name;
            sep = " ";
        }
        s << "}";

        s << " [";
        if(precedence != nullptr) {
            s << precedence->name << "/" << precedence->id;
        }else{
            s << "-";
        }
        s << "]";
    }
    return std::format("{}({}) :={};", ruleSetName(), ruleName, s.str());
}

