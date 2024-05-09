#include "LangOperations.h"
#include <fstream>

void bracketsString(std::string & str) {
    str.insert(str.begin(), '(');
    str.insert(str.end(), ')');
}

std::string Expr::getExpression() const noexcept { return expr_; }

Expr::Expr(char sym) {
    expr_ = sym;
    bracketsString(expr_);
}

void Expr::addAND(const Expr &expr) {
    expr_ += expr.getExpression();
    bracketsString(expr_);
}

void Expr::addOR(const Expr &expr) {
    expr_.insert(expr_.end(), '|');
    expr_ += expr.getExpression();
    bracketsString(expr_);
}

void Expr::addOptional() {
    bracketsString(expr_);
    expr_.insert(expr_.end(), '?');
}

void Expr::addKleeny() {
    bracketsString(expr_);
    expr_ += "...";
}

Expr ExprTransition::getExpression() const noexcept { return expression_; }

ExprTransition::ExprTransition(ExprState * next_state, ExprState * prev_state, char sym) : expression_(sym), next_state_(next_state), prev_state_(prev_state) {}

ExprTransition::ExprTransition(ExprState *next_state, ExprState *prev_state, Expr expr) : expression_(expr), next_state_(next_state), prev_state_(prev_state) {}

ExprState *ExprTransition::getNextState() const noexcept { return next_state_; }

Expr &ExprTransition::expr() noexcept { return expression_; }

ExprState::ExprState(bool isFinishState) : isFinishState_(isFinishState) {}

bool ExprState::addTransition(ExprTransition *transition) {
    if(!transition) return false;
    transitions_.push_back(transition);
    return true;
}

void ExprState::finishState() { isFinishState_ = true; }

bool ExprState::isFinishState() const noexcept { return isFinishState_; }

bool ExprState::deleteTransition(ExprTransition *transition) {
    for (auto i = transitions_.begin(); i != transitions_.end() ; ++i) {
        if(*i == transition) { transitions_.erase(i); delete transition; return true;}
    }
    return false;
}

std::vector<ExprTransition *> &ExprState::getTransitions() noexcept { return transitions_; }

const std::vector<ExprTransition *> &ExprState::getTransitions() const noexcept { return transitions_; }

AutomataConverter::AutomataConverter(const DFA_Automata *automata) {
    std::map<State*, ExprState*> conformity;
    std::stack<State *> stack;
    State * start = automata->getStart();
    stack.push(start);

    while (!stack.empty()) {
        State * working = stack.top();
        stack.pop();
        if(!conformity.count(working)) {
            for (auto &i : working->getTransitions()) {
                stack.push(i->getNextState());
            }
            conformity[working] = new ExprState(working->isFinishState());
        }
    }

    std::set<State*> visited;
    stack.push(start);
    while (!stack.empty()) {
        State * working = stack.top();
        stack.pop();
        if(!visited.count(working)) {
            std::map<State *, std::vector<char>> groups;
            for (auto &i : working->getTransitions()) {
                auto symTransition = dynamic_cast<SymbolTransition*>(i);
                stack.push(i->getNextState());
                groups[i->getNextState()].push_back(symTransition->getSymbol());
            }
            for ( auto &i : groups) {
                Expr expr = Expr(i.second.front());
                i.second.erase(i.second.begin());
                for (auto &sym : i.second) { expr.addOR(Expr(sym)); }
                auto transition = new ExprTransition(conformity[i.first], conformity[working], expr);
                conformity[working]->addTransition(transition);
            }
        }
        visited.insert(working);
    }

    automata_start_ = conformity[start];
}

std::vector<std::pair<ExprState *, ExprTransition*>> AutomataConverter::findInputStates(ExprState *victim) {
    std::stack<ExprState *> stack;
    std::set<ExprState*> visited;
    std::vector<std::pair<ExprState *, ExprTransition*>> result;
    stack.push(automata_start_);

    while (!stack.empty()) {
        ExprState * working = stack.top();
        stack.pop();
        if(!visited.count(working)) {
            for (auto &i : working->getTransitions()) {
                ExprState * next_state = i->getNextState();
                stack.push(i->getNextState());
                if(next_state == victim && working != victim) {
                    result.push_back({ working, i });
                }
            }
        }
        visited.insert(working);
    }
    return result;
}

std::vector<std::pair<ExprState *, ExprTransition*>> AutomataConverter::findOutStates(ExprState *victim) {
    std::vector<std::pair<ExprState *, ExprTransition*>> result;
    for (auto &i : victim->getTransitions()) {
        if(i->getNextState() != victim) { result.push_back({ i->getNextState(), i}); }
    }
    return result;
}

ExprTransition *AutomataConverter::findTransition(ExprState *start, ExprState *finish) {
    for ( auto &i : start->getTransitions() ) {
        if(i->getNextState() == finish) { return i; }
    }
    return nullptr;
}

bool AutomataConverter::isOrdinaryFinish(ExprState *state) {
    for (auto &i : state->getTransitions()) {
        if(i->getNextState() != automata_start_ && i->getNextState() != state) return true;
    }
    return false;
}

bool AutomataConverter::hasReverseStartTransition(ExprState *state) {
    for (auto &i : state->getTransitions()) {
        if(i->getNextState() == automata_start_) return true;
    }
    return false;
}

std::set<ExprState *> AutomataConverter::findStates(bool isOrdinaryStates) {
    std::stack<ExprState *> stack;
    std::set<ExprState*> visited;
    std::set<ExprState*> states;
    stack.push(automata_start_);

    while (!stack.empty()) {
        ExprState * working = stack.top();
        stack.pop();
        if(!visited.count(working)) {
            for (auto &i : working->getTransitions()) {
                ExprState * next_state = i->getNextState();
                stack.push(next_state);
                if(next_state != automata_start_ && !states.count(next_state)) {
                    if(isOrdinaryStates) {
                        if(!next_state->isFinishState()) states.insert(next_state);
                    }
                    else {
                        if(isOrdinaryFinish(next_state) && !hasReverseStartTransition(next_state)) states.insert(next_state);
                    }
                }
            }
        }
        visited.insert(working);
    }
    return states;
}

ExprState* AutomataConverter::findOrdinaryFinish() {
    std::stack<ExprState *> stack;
    std::set<ExprState*> visited;
    stack.push(automata_start_);

    while (!stack.empty()) {
        ExprState * working = stack.top();
        stack.pop();
        if(!visited.count(working)) {
            for (auto &i : working->getTransitions()) {
                ExprState * next_state = i->getNextState();
                stack.push(next_state);
                if(next_state != automata_start_) {
                    if(isOrdinaryFinish(next_state)) return next_state;
                }
            }
        }
        visited.insert(working);
    }
    return nullptr;
}

void AutomataConverter::convert() {
    std::set<ExprState*> states = findStates(true);

    for (auto &i : states) {
        auto inputStates = findInputStates(i);
        auto outStates = findOutStates(i);

        ExprTransition *cycleTransition = findTransition(i, i);
        for (auto &inp: inputStates) {
            for (auto &out: outStates) {
                ExprTransition *inp_to_out = findTransition(inp.first, out.first);
                if (inp_to_out) {
                    if (cycleTransition) {
                        Expr expr = inp.second->getExpression();
                        Expr kleeny = cycleTransition->getExpression();
                        kleeny.addKleeny();
                        expr.addAND(kleeny);
                        expr.addAND(out.second->getExpression());
                        inp_to_out->expr().addOR(expr);
                    } else {
                        Expr expr = inp.second->getExpression();
                        expr.addAND(out.second->getExpression());
                        inp_to_out->expr().addOR(expr);
                    }
                } else {
                    if (cycleTransition) {
                        Expr expr = inp.second->getExpression();
                        //if(inp.first->isFinishState()) expr.addOptional();
                        Expr kleeny = cycleTransition->getExpression();
                        kleeny.addKleeny();
                        expr.addAND(kleeny);
                        expr.addAND(out.second->getExpression());
                        auto transition = new ExprTransition(out.first, inp.first, expr);
                        inp.first->addTransition(transition);
                    } else {
                        Expr expr = inp.second->getExpression();
                        //if(inp.first->isFinishState()) expr.addOptional();
                        expr.addAND(out.second->getExpression());
                        auto transition = new ExprTransition(out.first, inp.first, expr);
                        inp.first->addTransition(transition);
                    }
                }
            }
        }

        for (auto &st: inputStates) {
            st.first->deleteTransition(st.second);
        }
        for (auto &st: outStates) {
            i->deleteTransition(st.second);
        }
        delete i;
    }

    ExprState * find = findOrdinaryFinish();

    while(find) {
        auto inputStates = findInputStates(find);
        auto outStates = findOutStates(find);
        ExprTransition *cycleTransition = findTransition(find, find);
        for (auto &inp: inputStates) {
            for (auto &out: outStates) {
                ExprTransition *inp_to_out = findTransition(inp.first, out.first);
                if (inp_to_out) {
                    if (cycleTransition) {
                        Expr expr = inp.second->getExpression();
                        Expr kleeny = cycleTransition->getExpression();
                        kleeny.addKleeny();
                        expr.addAND(kleeny);
                        Expr optional = out.second->getExpression();
                        optional.addOptional();
                        expr.addAND(optional);
                        inp_to_out->expr().addOR(expr);
                        //if(inp.first->isFinishState()) inp_to_out->expr().addOptional();
                    } else {
                        Expr expr = inp.second->getExpression();
                        Expr optional = out.second->getExpression();
                        optional.addOptional();
                        expr.addAND(optional);
                        inp_to_out->expr().addOR(expr);
                        //if(inp.first->isFinishState()) inp_to_out->expr().addOptional();
                    }
                } else {
                    if (cycleTransition) {
                        Expr expr = inp.second->getExpression();
                        Expr kleeny = cycleTransition->getExpression();
                        kleeny.addKleeny();
                        expr.addAND(kleeny);
                        Expr optional = out.second->getExpression();
                        optional.addOptional();
                        expr.addAND(optional);
                        //if(inp.first->isFinishState()) expr.addOptional();
                        auto transition = new ExprTransition(out.first, inp.first, expr);
                        inp.first->addTransition(transition);
                    } else {
                        Expr expr = inp.second->getExpression();
                        Expr optional = out.second->getExpression();
                        optional.addOptional();
                        expr.addAND(optional);
                        //if(inp.first->isFinishState()) expr.addOptional();
                        auto transition = new ExprTransition(out.first, inp.first, expr);
                        inp.first->addTransition(transition);
                    }
                }
            }
        }

        for (auto &st: inputStates) {
            st.first->deleteTransition(st.second);
        }
        for (auto &st: outStates) {
            find->deleteTransition(st.second);
        }
        delete find;
        find = findOrdinaryFinish();
    }

    for (auto &i : automata_start_->getTransitions()) {
        ExprState * next_state = i->getNextState();
        if(hasReverseStartTransition(next_state) && next_state != automata_start_) {
            ExprTransition * startCycle = findTransition(automata_start_, automata_start_);
            if(startCycle) {
                ExprTransition * ordCycle = findTransition(next_state, next_state);
                if(ordCycle) {
                    ExprTransition * reverseTransition = findTransition(next_state, automata_start_);
                    Expr first_s = i->getExpression();
                    Expr cycle = ordCycle->getExpression();
                    cycle.addKleeny();
                    first_s.addAND(cycle);
                    first_s.addAND(reverseTransition->getExpression());
                    startCycle->expr().addOR(first_s);
                    next_state->deleteTransition(reverseTransition);
                } else {
                    ExprTransition * reverseTransition = findTransition(next_state, automata_start_);
                    Expr first_s = i->getExpression();
                    first_s.addAND(reverseTransition->getExpression());
                    startCycle->expr().addOR(first_s);
                    next_state->deleteTransition(reverseTransition);
                }
            } else {
                ExprTransition * ordCycle = findTransition(next_state, next_state);
                if(ordCycle) {
                    ExprTransition * reverseTransition = findTransition(next_state, automata_start_);
                    Expr first_s = i->getExpression();
                    Expr cycle = ordCycle->getExpression();
                    cycle.addKleeny();
                    first_s.addAND(cycle);
                    first_s.addAND(reverseTransition->getExpression());
                    auto transition = new ExprTransition(automata_start_, automata_start_, first_s);
                    automata_start_->addTransition(transition);
                    next_state->deleteTransition(reverseTransition);
                } else {
                    ExprTransition * reverseTransition = findTransition(next_state, automata_start_);
                    Expr first_s = i->getExpression();
                    first_s.addAND(reverseTransition->getExpression());
                    auto transition = new ExprTransition(automata_start_, automata_start_, first_s);
                    automata_start_->addTransition(transition);
                    next_state->deleteTransition(reverseTransition);
                }
            }
        }
    }

    Expr result;
    bool hasStartCycle = false;

    if(findTransition(automata_start_, automata_start_)) {
        result = findTransition(automata_start_, automata_start_)->getExpression();
        result.addKleeny();
        hasStartCycle = true;
    }

    Expr alternatives;
    bool fisrt_alternative = true;

    for (auto &i : automata_start_->getTransitions()) {
        ExprState * next_state = i->getNextState();
        if(next_state == automata_start_) continue;
        ExprTransition * ordCycle = findTransition(next_state, next_state);
        if(ordCycle) {
            Expr first_f = i->getExpression();
            Expr addFirstF = ordCycle->getExpression();
            addFirstF.addKleeny();
            first_f.addAND(addFirstF);
            if(automata_start_->isFinishState()) first_f.addOptional();
            if(fisrt_alternative) { alternatives = first_f; fisrt_alternative = false; }
            else { alternatives.addOR(first_f); }
        } else {
            Expr first_f = i->getExpression();
            if(automata_start_->isFinishState()) first_f.addOptional();
            if(fisrt_alternative) { alternatives = first_f; fisrt_alternative = false; }
            else { alternatives.addOR(first_f); }
        }
    }

    if(hasStartCycle) { result.addAND(alternatives); }
    else result = alternatives;

    expr_ = result;
    //std::cout << expr_.getExpression();
}

void AutomataConverter::printDOT(const std::string &file_name) const noexcept {
    std::ofstream file_(file_name + ".txt");
    file_ << "digraph G {" << std::endl << "rankdir=LR" << std::endl;

    unsigned int count = 0;
    std::map<ExprState*, unsigned int> count_state_;
    std::set<ExprState*> visited_;
    std::stack<ExprState*> stack_;
    stack_.push(automata_start_);
    count_state_[automata_start_] = count;

    if(automata_start_->isFinishState()) {
        file_ << count << " [shape = doublecircle, label =\"" << count << "\"]" << std::endl;
    } else {
        file_ << count << " [shape = circle, label =\"" << count << "\"]" << std::endl;
    }
    ++count;

    while (!stack_.empty()) {
        ExprState *processing = stack_.top();
        stack_.pop();
        if (!visited_.count(processing)) {
            for (auto &i: processing->getTransitions()) {
                if (!count_state_.count(i->getNextState())) {
                    count_state_[i->getNextState()] = count;
                    if(i->getNextState()->isFinishState()) {
                        file_ << count << " [shape = doublecircle, label =\"" << count << "\"]" << std::endl;
                    } else {
                        file_ << count << " [shape = circle, label =\"" << count << "\"]" << std::endl;
                    }
                    ++count;
                }

                file_ << count_state_[processing] << "->" << count_state_[i->getNextState()] << "[label = \"" << i->getExpression().getExpression() << "\"]" << std::endl;

                stack_.push(i->getNextState());
            }
        }
        visited_.insert(processing);
    }

    file_ << "}";
    file_.close();

    std::string command_dot = "dot -Tpng " + file_name + ".txt -o" + file_name + ".png";
    system(command_dot.c_str());
}

std::string AutomataConverter::getExpr() const { return expr_.getExpression(); }