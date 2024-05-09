#include "myRegex.h"

myRegex::myRegex(const std::string &str) {
    PatternString pattern(str);
    NFA_Automata * NFA =  pattern.generateSyntaxTree().generateNFA();
    automata_.synthesisFromNFA(NFA);
}

bool myRegex::match(const std::string &str_) {
    automata_.start();
    bool isAccept = automata_.isAccept();

    for (auto &i : str_) {
        bool next = automata_.next_state(i);
        if(!next) return false;
        isAccept = automata_.isAccept();
    }

    return  isAccept;
}

myRegex &myRegex::inverse() {
    AutomataConverter converter(&automata_);
    converter.convert();
    PatternString pattern(converter.getExpr());
    NFA_Automata * NFA =  pattern.generateInverseSyntaxTree().generateNFA();
    NFA->printDOT("nfa");
    automata_.synthesisFromNFA(NFA);
    automata_.optimize();
    automata_.printDOT("dfa");
    return *this;
}

std::vector<State *> myRegex::findAllStates(State * start) {
    std::set<State *> visited;
    std::stack<State *> states_stack;

    states_stack.push(start);

    while (!states_stack.empty()) {
        State * working = states_stack.top();
        states_stack.pop();
        if(!visited.count(working)) {
            for ( auto &i : working->getTransitions()) {
                if(!visited.count(i->getNextState())) {
                    states_stack.push(i->getNextState());
                }
            }
            visited.insert(working);
        }
    }

    std::vector<State*> res;
    res.reserve(visited.size());
    for ( auto &i : visited) {
        res.push_back(i);
    }
    return res;
}

myRegex &myRegex::substract(const myRegex &other_regex) {
    auto main_automata = automata_;
    auto ordinary_automata = other_regex.automata_;

    auto main_states = findAllStates(main_automata.getStart());
    auto ordinary_states = findAllStates(ordinary_automata.getStart());

    auto errState = new State;
    ordinary_states.push_back(errState);

    std::map<std::pair<State*, State*>, State*> newStates;

    for (auto &i : main_states) {
        for (auto &b : ordinary_states) {
            auto new_state = new State;
            newStates[{i, b}] = new_state;
            if(i->isFinishState() && !b->isFinishState()) { new_state->finishState(); }
        }
    }

    for (auto &i : newStates) {
        for (auto &main_transes : i.first.first->getTransitions()) {
            bool hasWord = false;
            auto sym_main_trans = dynamic_cast<SymbolTransition*>(main_transes);
            for (auto &ordinary_transes : i.first.second->getTransitions()) {
                auto sym_ordinary_trans = dynamic_cast<SymbolTransition*>(ordinary_transes);
                if(sym_main_trans->getSymbol() == sym_ordinary_trans->getSymbol()) {
                    State * next_state = newStates[{ main_transes->getNextState(), ordinary_transes->getNextState() }];
                    auto * sym_transition = new SymbolTransition(next_state, sym_main_trans->getSymbol());
                    i.second->addTransition(sym_transition);
                    hasWord = true;
                    break;
                }
            }
            if(!hasWord) {
                State * next_state = newStates[{ main_transes->getNextState(), errState }];
                auto * sym_transition = new SymbolTransition(next_state, sym_main_trans->getSymbol());
                i.second->addTransition(sym_transition);
            }
        }
    }

    State * start = newStates[ { main_automata.getStart(), ordinary_automata.getStart() }];

    auto acceptedStates = findAllStates(start);
    for (auto &i : newStates) {
        bool accept = false;
        for (auto &b : acceptedStates) {
            if(i.second == b) { accept = true; break; }
        }
        if(!accept) {
            delete i.second;
            i.second = nullptr;
        }
    }

    automata_ = DFA_Automata(start);
    automata_.optimize();

    return *this;
}

myRegex::myRegex(const std::string &str, syntax_option_type::syntax_option type) {
    PatternString pattern(str);
    NFA_Automata * NFA =  pattern.generateSyntaxTree().generateNFA();
    automata_.synthesisFromNFA(NFA);
    if(type == syntax_option_type::optimize) { automata_.optimize(); }
}