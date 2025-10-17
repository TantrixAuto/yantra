/// @file grammar.cpp
/// This file contains implementation of certain functions in the yg::Grammar class
/// specifically, the Atom compare() functions

#include "pch.hpp"
#include "grammar.hpp"
#include "encodings.hpp"

namespace {

template<typename T>
concept HasVisitMode = requires { typename T::VisitClassMode;};

template<typename T, typename LT, typename RT>
concept HasMethod = requires(T obj, const LT& l, const RT& r) {
    obj(l, r);
};

template<typename VT, typename LT, typename RT>
inline auto _visitTransition(VT& v, const LT& lhs, const RT& rhs) {
    return v(lhs, rhs);
}

template<typename VT, typename LT>
inline auto _visitTransition(VT& v, const LT& lhs, const yg::Grammar::ClassTransition& rhs) requires (HasVisitMode<VT>) {
    v.classDone = false;
    for(auto& ax : rhs.atom.atoms) {
        std::visit([&v, &lhs](const auto& a){
            return v(lhs, a);
        }, ax);
        if(v.classDone == true) {
            break;
        }
    }

    if constexpr(!std::same_as<typename VT::VisitClassMode, void>) {
        return v.classRet;
    }
}

template<typename VT, typename RT>
inline auto _visitTransition(VT& v, const yg::Grammar::ClassTransition& lhs, const RT& rhs) requires (HasVisitMode<VT>) {
    v.classDone = false;
    for(auto& ax : lhs.atom.atoms) {
        std::visit([&v, &rhs](const auto& a){
            return v(a, rhs);
        }, ax);
        if(v.classDone == true) {
            break;
        }
    }

    if constexpr(!std::same_as<typename VT::VisitClassMode, void>) {
        return v.classRet;
    }
}

template<typename VT>
[[maybe_unused]]
inline auto _visitTransition(VT& v, const yg::Grammar::ClassTransition& lhs, const yg::Grammar::ClassTransition& rhs)
requires (HasVisitMode<VT> && !HasMethod<VT, yg::Grammar::ClassTransition, yg::Grammar::ClassTransition>) {
    v.classDone = false;
    for(auto& lax : lhs.atom.atoms) {
        std::visit([&v, &rhs](const auto& la){
            for(auto& rax : rhs.atom.atoms) {
                std::visit([&v, &la](const auto& ra){
                    return v(la, ra);
                }, rax);
                if(v.classDone == true) {
                    break;
                }
            }
        }, lax);
        if(v.classDone == true) {
            break;
        }
    }

    if constexpr(!std::same_as<typename VT::VisitClassMode, void>) {
        return v.classRet;
    }
}

template<typename VT>
inline auto _visitTransition(VT& v, const yg::Grammar::ClassTransition& lhs, const yg::Grammar::ClassTransition& rhs)
requires (HasVisitMode<VT> && HasMethod<VT, yg::Grammar::ClassTransition, yg::Grammar::ClassTransition>) {
    return v(lhs, rhs);
}

template<typename VT, typename RT>
inline auto _visitTransition(VT& v, const yg::Grammar::PrimitiveTransition& lhs, const RT& rhs) requires (HasVisitMode<VT>) {
    return std::visit([&v, &rhs](const auto& a){
        return v(a, rhs);
    }, lhs.atom.atom);
}

template<typename VT, typename LT>
inline auto _visitTransition(VT& v, const LT& lhs, const yg::Grammar::PrimitiveTransition& rhs) requires (HasVisitMode<VT>) {
    return std::visit([&v, &lhs](const auto& a){
        return v(lhs, a);
    }, rhs.atom.atom);
}

template<typename VT>
inline auto _visitTransition(VT& v, const yg::Grammar::PrimitiveTransition& lhs, const yg::Grammar::PrimitiveTransition& rhs) requires (HasVisitMode<VT>) {
    return std::visit([&rhs, &v](const auto& l){
        return std::visit([&l, &v](const auto& r){
            return v(l, r);
        }, rhs.atom.atom);
    }, lhs.atom.atom);

    if constexpr(!std::same_as<typename VT::VisitClassMode, void>) {
        return v.classRet;
    }
}

template<typename VT>
inline auto _visitTransition(VT& v, const yg::Grammar::PrimitiveTransition& lhs, const yg::Grammar::ClassTransition& rhs) requires (HasVisitMode<VT>) {
    return std::visit([&v, &rhs](const auto& la){
        for(auto& rax : rhs.atom.atoms) {
            std::visit([&v, &la](const auto& ra){
                return v(la, ra);
            }, rax);
            if(v.classDone == true) {
                break;
            }
        }
        if constexpr(!std::same_as<typename VT::VisitClassMode, void>) {
            return v.classRet;
        }
    }, lhs.atom.atom);
}

template<typename VT>
inline auto _visitTransition(VT& v, const yg::Grammar::ClassTransition& lhs, const yg::Grammar::PrimitiveTransition& rhs) requires (HasVisitMode<VT>) {
    v.classDone = false;
    for(auto& ax : lhs.atom.atoms) {
        std::visit([&v, &rhs](const auto& la){
            return std::visit([&v, &la](const auto& ra){
                return v(la, ra);
            }, rhs.atom.atom);
        }, ax);
        if(v.classDone == true) {
            break;
        }
    }

    if constexpr(!std::same_as<typename VT::VisitClassMode, void>) {
        return v.classRet;
    }
}

template<typename VT>
inline auto visitTransition(VT& v, const yg::Grammar::Transition& lhs, const yg::Grammar::Transition& rhs) {
    return std::visit([&rhs, &v](const auto& l){
        return std::visit([&l, &v](const auto& r){
            return _visitTransition(v, l, r);
        }, rhs.t);
    }, lhs.t);
}

struct SubsetChecker {
    using VisitClassMode = bool;

    bool classDone = false;
    bool classRet = false;

    inline bool found() {
        classRet = true;
        classDone = true;
        return true;
    }

    inline bool operator()(const yg::Grammar::RangeClass& lhs, const yg::Grammar::RangeClass& rhs) {
        // first check for |+++++++|
        if((lhs.ch1 == rhs.ch1) && (lhs.ch2 == rhs.ch2)) {
            return false;
        }

        // next check for these
        // |--+++--|
        // |+++----|
        // |----+++|
        if((lhs.ch1 >= rhs.ch1) && (lhs.ch2 <= rhs.ch2)) {
            return found();
        }

        return false;
    }

    inline bool operator()(const yg::Grammar::RangeClass& lhs, const yg::Grammar::LargeEscClass& rhs) {
        if(rhs.checker == "isLetter") {
            if(rhs.grammar.unicodeEnabled) {
                if(Encodings::isUnicodeLetterSubset(lhs.ch1, lhs.ch2)) {
                    return found();
                }
            }else{
                if(Encodings::isAsciiLetterSubset(lhs.ch1, lhs.ch2)) {
                    return found();
                }
            }
        }
        return false;
    }

    inline bool operator()(const auto& lhs, const auto& rhs) const {
        unused(lhs, rhs);
        return false;
    }
};

struct CompareChecker {
    using VisitClassMode = int32_t;

    bool classDone = false;
    int32_t classRet = -2;

    inline int32_t operator()(const yg::Grammar::WildCard&, const yg::Grammar::WildCard&) {
        return 0;
    }

    inline int32_t operator()(const yg::Grammar::RangeClass& lhs, const yg::Grammar::RangeClass& rhs) {
        return yg::Grammar::compare(lhs, rhs);
    }

    inline int32_t operator()(const yg::Grammar::LargeEscClass& lhs, const yg::Grammar::LargeEscClass& rhs) {
        if(lhs.checker == rhs.checker) {
            return 0;
        }
        return -2;
    }

    inline int32_t operator()(const yg::Grammar::ClassTransition& lhs, const yg::Grammar::ClassTransition& rhs) {
        return yg::Grammar::compare(lhs.atom, rhs.atom);
    }

    inline int32_t operator()(const yg::Grammar::ClosureTransition& lhs, const yg::Grammar::ClosureTransition& rhs) {
        if(lhs.type == rhs.type) {
            switch(lhs.type) {
            case yg::Grammar::ClosureTransition::Type::Enter:
            case yg::Grammar::ClosureTransition::Type::Leave:
                return 0;
            case yg::Grammar::ClosureTransition::Type::PreLoop:
            case yg::Grammar::ClosureTransition::Type::InLoop:
            case yg::Grammar::ClosureTransition::Type::PostLoop:
                break;
            }
        }
        return -2;
    }

    inline int32_t operator()(const yg::Grammar::SlideTransition&, const yg::Grammar::SlideTransition&) {
        return 0;
    }

    inline int32_t operator()(const auto& lhs, const auto& rhs) const {
        unused(lhs, rhs);
        return -2;
    }
};
}

bool yg::Grammar::Transition::isSubsetOf(const Transition& rhs) const {
    SubsetChecker chk;
    return visitTransition(chk, *this, rhs);
}

int32_t yg::Grammar::Transition::compare(const Transition& rhs) const {
    CompareChecker chk;
    return visitTransition(chk, *this, rhs);
}
