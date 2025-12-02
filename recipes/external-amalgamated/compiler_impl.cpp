#include <iostream>
#include <format>
#include <filesystem>
#include <functional>
#include <assert.h>
#include "CompilerBase.hpp"

struct CompilerImpl : public CompilerBase {
    inline CompilerImpl(YantraModule& m) : CompilerBase(m) {}

    virtual void
    start_1_go(NodeRef<YantraModule_AST::expr>& e) override {
        auto v = eval(e);
        std::cout << "Result(external-amalgamated):" << v << std::endl;
    }

    virtual int
    expr_1_eval(NodeRef<YantraModule_AST::addexpr>& e) override {
        auto v = eval(e);
        return v;
    }

    virtual void
    expr_1_go([[maybe_unused]]NodeRef<YantraModule_AST::addexpr>& e) override {
    }

    virtual int
    addexpr_1_eval (NodeRef<YantraModule_AST::addexpr>& a ,NodeRef<YantraModule_AST::mulexpr>& v) override {
        auto av = eval(a);
        auto vv = eval(v);
        auto rv = av + vv;
        return rv;
    }

    virtual int
    addexpr_2_eval (NodeRef<YantraModule_AST::mulexpr>& e) override {
        auto v = eval(e);
        return v;
    }

    virtual void
    addexpr_1_go ([[maybe_unused]]NodeRef<YantraModule_AST::addexpr>& a, [[maybe_unused]]NodeRef<YantraModule_AST::mulexpr>& v) override {
    }

    virtual void
    addexpr_2_go ([[maybe_unused]]NodeRef<YantraModule_AST::mulexpr>& e) override {
    }

    virtual int
    mulexpr_1_eval (NodeRef<YantraModule_AST::mulexpr>& m, NodeRef<YantraModule_AST::valexpr>& a) override {
        auto mv = eval(m);
        auto av = eval(a);
        auto v = mv * av;
        return v;
    }

    virtual int
    mulexpr_2_eval (NodeRef<YantraModule_AST::valexpr>& e) override {
        auto v = eval(e);
        return v;
    }

    virtual void
    mulexpr_1_go ([[maybe_unused]]NodeRef<YantraModule_AST::mulexpr>& m, [[maybe_unused]]NodeRef<YantraModule_AST::valexpr>& a) override {
    }

    virtual void
    mulexpr_2_go ([[maybe_unused]]NodeRef<YantraModule_AST::valexpr>& e) override {
    }

    virtual int
    valexpr_1_eval (NodeRef<YantraModule_AST::expr>& e) override {
        auto v = eval(e);
        return v;
    }

    virtual int
    valexpr_2_eval (const YantraModule_AST::Token& N) override {
        auto v = std::stoi(N.text);
        return v;
    }

    virtual void
    valexpr_1_go ([[maybe_unused]]NodeRef<YantraModule_AST::expr>& e) override {
    }

    virtual void
    valexpr_2_go ([[maybe_unused]]const YantraModule_AST::Token& N) override {
    }
};

std::unique_ptr<CompilerBase> CompilerBase::create(YantraModule& m) {
    return std::make_unique<CompilerImpl>(m);
}
