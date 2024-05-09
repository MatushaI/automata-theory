#include <string>
#include "syntaxTree.h"
#include "DFA.h"
#include "LangOperations.h"

#ifndef LAB2_MYREGEX_H
#define LAB2_MYREGEX_H

namespace syntax_option_type {
    typedef unsigned char syntax_option;
    inline constexpr syntax_option optimize = 1;
}

struct CaptureGroupStr {
    std::string name_;
    std::string str_;
    std::string str_freeze_;
    bool active_;
};

class CaptureGroupCollector {
    std::map<std::string, CaptureGroupStr> collector;
};

class mySmatch {
    CaptureGroupCollector collector_;
public:
    mySmatch() = default;
    ~mySmatch() = default;
};

class myRegex {
    DFA_Automata automata_;
    std::vector<State*> findAllStates(State * start);
public:
    explicit myRegex(std::string const& str, syntax_option_type::syntax_option type);
    explicit myRegex(std::string const& str);
    myRegex & inverse();
    myRegex & substract(myRegex const& other_regex);
    //bool match(std::string const& str_, mySmatch & smatch);
    bool match(std::string const& str_);
};


#endif //LAB2_MYREGEX_H
