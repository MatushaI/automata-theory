#include "DFA.h"
#include <set>
#include <stack>
#include <fstream>

bool StatesGroup::operator==(StatesGroup const& state_group) const {
    return states_ == state_group.states_;
}

bool StatesGroup::operator<(StatesGroup const& state_group) const {
    return states_ < state_group.states_;
}

bool StatesGroup::operator>(StatesGroup const& state_group) const {
    return states_ > state_group.states_;
}

bool StatesGroup::hasEndState(State *endState) const {
    return states_.find(endState) != states_.end();
}

const std::set<State *> &StatesGroup::getStates() const { return states_; }

StatesGroup::StatesGroup(std::set<State *> states) : states_(std::move(states)) {}

std::pair<State *, bool> StatesGroupCollector::findState(const StatesGroup &states_group) {
    auto res = collector_.find(states_group);
    if(res != collector_.end()) return { res->second, true };
    return { nullptr, false };
}

std::pair<const StatesGroup &, bool> StatesGroupCollector::findStatesGroup(State *state) {
    for (auto &i : collector_) {
        if(i.second == state) return {i.first, true};
    }
    return {StatesGroup(), false};
}

std::pair<State *, bool> StatesGroupCollector::insert(const StatesGroup &states_group, State *state) {
    auto res = collector_.insert({states_group, state});
    return {state, res.second};
}

DFA_Automata::DFA_Automata(State *start) { start_ = start; }

StatesGroup DFA_Automata::order_for_epsilon(State *state) {
    std::set<State *> visited;
    std::stack<State *> states_stack;

    states_stack.push(state);
    while (!states_stack.empty()) {
        State * working = states_stack.top();
        states_stack.pop();
        if(!visited.count(working)) {
            for ( auto &i : working->getTransitions()) {
                if(compaireTransition<EpsilonTransition>(i)) {
                    states_stack.push(i->getNextState());
                }
            }
            visited.insert(working);
        }
    }
    return StatesGroup(visited);
}

StatesGroup DFA_Automata::order_for_epsilon(std::vector<State*> const& states) {
    std::set<State *> visited;
    std::stack<State *> states_stack;

    for (auto &i : states) states_stack.push(i);

    while (!states_stack.empty()) {
        State * working = states_stack.top();
        states_stack.pop();
        if(!visited.count(working)) {
            for ( auto &i : working->getTransitions()) {
                if(compaireTransition<EpsilonTransition>(i)) {
                    states_stack.push(i->getNextState());
                }
            }
            visited.insert(working);
        }
    }
    return StatesGroup(visited);
}

[[nodiscard]] State* DFA_Automata::createState(StatesGroup const& states_group, State * endState) {
    bool hasCaptureGroups = false;
    for (auto &i : states_group.getStates()) {
        if(dynamic_cast<CaptureGroupState*>(i)) {
            hasCaptureGroups = true;
            break;
        }
    }

    if(!hasCaptureGroups) { return new State(states_group.hasEndState(endState)); }

    auto state = new CaptureGroupState();
    for (auto &i : states_group.getStates()) {
        if(i == endState) state->finishState();
        auto capt_state =  dynamic_cast<CaptureGroupState*>(i);
        if(capt_state) {
            state->addInfo(capt_state->getCaptureGroupName(), capt_state->isStart(), capt_state->isFinish());
        }
    }

    return state;
}

std::map<char, std::vector<State*>> DFA_Automata::single_order_for_symbol(State *determenistic_state, StatesGroupCollector & collector) {
    std::map<char, std::vector<State*>> visited;

    auto group = collector.findStatesGroup(determenistic_state);
    if(group.second) {
        for (auto &i : group.first.getStates()) {
            for (auto &b : i->getTransitions()) {
                if(compaireTransition<SymbolTransition>(b)) {
                    auto sym_transition = dynamic_cast<SymbolTransition*>(b);
                    visited[sym_transition->getSymbol()].push_back(sym_transition->getNextState());
                }
            }
        }
        return visited;
    }

    throw std::logic_error("DFA Error");
}

State * DFA_Automata::addTransitionNewState(State *to_state, State *out_state, char transitionSymbol) {
    auto transition = new SymbolTransition(to_state, transitionSymbol);
    out_state->addTransition(transition);
    return to_state;
}

void DFA_Automata::addTransitionState(State *input_state, State *to_state, char transitionSymbol) {
    auto transition = new SymbolTransition(to_state, transitionSymbol);
    input_state->addTransition(transition);
}

void DFA_Automata::start() noexcept { actualState_ = start_; }

void DFA_Automata::synthesisFromNFA(const NFA_Automata *nfa_auto) {
    StatesGroupCollector collector;

    std::stack<State *> determenisticStates;
    auto endState = nfa_auto->getEndConnector();
    auto startStates = order_for_epsilon(nfa_auto->getBeginConnector());

    State * startState = createState(startStates, endState);
    start_ = startState;

    collector.insert(startStates,  startState);
    determenisticStates.push(startState);

    while (!determenisticStates.empty()) {
        State * working = determenisticStates.top();
        determenisticStates.pop();
        auto symbolStates = single_order_for_symbol(working, collector);

        for (auto &i : symbolStates) {
            StatesGroup epsGroups = order_for_epsilon(i.second);
            auto state_find = collector.findState(epsGroups);
            if(state_find.second) {
                addTransitionState(working, state_find.first, i.first);
                if(working == state_find.first) {
                    auto fin = collector.findStatesGroup(working);
                    if(fin.second) {
                        for (auto &b: fin.first.getStates()) {
                            for (auto &c : b->getTransitions()) {
                                auto sym_trans = dynamic_cast<SymbolTransition*>(c);
                                if(sym_trans) {
                                    if(sym_trans->getSymbol() == i.first) {
                                        for (auto &d : sym_trans->getNextState()->getTransitions()) {
                                            if(d->getNextState() == b) {
                                                working->cycle();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                State * state_to = createState(epsGroups, endState);
                state_to = addTransitionNewState(state_to, working, i.first);
                collector.insert(epsGroups, state_to);
                determenisticStates.push(state_to);
            }
        }
    }
}

void DFA_Automata::printDOT(const std::string &file_name) {
    std::ofstream file_(file_name + ".txt");
    file_ << "digraph G {" << std::endl << "rankdir=LR" << std::endl;

    unsigned int count = 0;
    std::map<State*, unsigned int> count_state_;
    std::set<State*> visited_;
    std::stack<State*> stack_;
    stack_.push(start_);
    count_state_[start_] = count;

    if(dynamic_cast<CaptureGroupState*>(start_)) {
        auto capt = dynamic_cast<CaptureGroupState*>(start_);
        for (auto &i : capt->getInfo()) {
            std::cout << count << " " << i.first << " start: " << i.second.first << ", end: " << i.second.second << " " << start_->isCycle() << std::endl;
        }
        if(start_->isFinishState()) {
            file_ << count << " [shape = doubleoctagon, label =\"" << count << "\"]" << std::endl;
        } else {
            file_ << count << " [shape = octagon, label =\"" << count << "\"]" << std::endl;
        }
    }
    else if(start_->isFinishState()) {
        file_ << count << " [shape = doublecircle, label =\"" << count << "\"]" << std::endl;
    } else {
        file_ << count << " [shape = circle, label =\"" << count << "\"]" << std::endl;
    }
    ++count;

    while (!stack_.empty()) {
        State *processing = stack_.top();
        stack_.pop();
        if (!visited_.count(processing)) {
            for (auto &i: processing->getTransitions()) {
                if (!count_state_.count(i->getNextState())) {
                    count_state_[i->getNextState()] = count;
                    if(dynamic_cast<CaptureGroupState*>(i->getNextState())) {
                        auto capt = dynamic_cast<CaptureGroupState*>(i->getNextState());
                        for (auto &d : capt->getInfo()) {
                            std::cout << count << " " << d.first << " start: " << d.second.first << ", end: " << d.second.second << " " << i->getNextState()->isCycle() << std::endl;
                        }
                        if(i->getNextState()->isFinishState()) {
                            file_ << count << " [shape = doubleoctagon, label =\"" << count << "\"]" << std::endl;
                        } else {
                            file_ << count << " [shape = octagon, label =\"" << count << "\"]" << std::endl;
                        }
                    }
                    else if(i->getNextState()->isFinishState()) {
                        file_ << count << " [shape = doublecircle, label =\"" << count << "\"]" << std::endl;
                    } else {
                        file_ << count << " [shape = circle, label =\"" << count << "\"]" << std::endl;
                    }
                    ++count;
                }
                if(compaireTransition<EpsilonTransition>(i)) {
                    file_ << count_state_[processing] << "->" << count_state_[i->getNextState()] << "[label = \"eps\"]" << std::endl;
                } else {
                    auto transition = dynamic_cast<SymbolTransition*>(i);
                    file_ << count_state_[processing] << "->" << count_state_[i->getNextState()] << "[label = \"" << transition->getSymbol() << "\"]" << std::endl;
                }
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

bool DFA_Automata::next_state(char sym) {
    for (auto &i : actualState_->getTransitions()) {
        auto symTransition = dynamic_cast<SymbolTransition*>(i);
        if(symTransition->getSymbol() == sym) { actualState_ = symTransition->getNextState(); return true; }
    }
    return false;
}

State *DFA_Automata::getActualState() const noexcept { return actualState_; }

bool DFA_Automata::isAccept() noexcept { return actualState_->isFinishState();}

std::pair<bool, std::vector<StateCaptureGroupInfo>> DFA_Automata::getCaptureGroupInfo() {
    auto captureGroupState = dynamic_cast<CaptureGroupState*>(actualState_);
    if(!captureGroupState) return {false, {}};
    std::vector<StateCaptureGroupInfo> resInfo;
    for (auto &i : captureGroupState->getInfo()) {
        resInfo.push_back({i.first, i.second.first, i.second.second});
    }
    return { true, resInfo };
}

State *DFA_Automata::getStart() const noexcept { return start_; }

bool DevideStates::addState(State *state) {
    if(states_.count(state)) return false;
    states_.insert(state);
    return true;
}

State *DevideStates::deleteState(State *state) {
    if(states_.count(state)) { states_.erase(state); return state; }
    return nullptr;
}

DevideStates::DevideStates(bool isFinish) { isFinish_ = isFinish; }

bool DevideStates::operator<=>(const DevideStates & other) const {
    return states_ == other.states_;
}

const std::set<State *> &DevideStates::getStates() const noexcept { return states_; }

bool DevideStates::isFinished() const noexcept { return isFinish_; }

DevideStatesCollector::DevideStatesCollector(DevideStates * ordinary, DevideStates * finished) {
    devides_.insert(finished);
    devides_.insert(ordinary);
}

DevideStates * DevideStatesCollector::findDevide(State *state) const {
    for (auto &i : devides_) {
        for (auto &b : i->getStates()) {
            if(b == state) return i;
        }
    }
    throw std::logic_error("Devide find error");
}

void DevideStatesCollector::splitDevideStates(DevideStates * devide,
                                              const std::vector<std::vector<State *>> &newGroups) {
    bool isFinished = devide->isFinished();
    devides_.erase(devide);
    for (auto &i : newGroups) {
        auto newGroup = new DevideStates(isFinished);
        for (auto &b : i) {
            newGroup->addState(b);
        }
        devides_.insert(newGroup);
    }
}

State *DevideStatesCollector::devideAutomata(State * old_start) {
    bool lever = true;

    while (lever) {
        lever = false;
        for (int c = 0; c < 1; ++c) {
            for (auto &dev_state: devides_) {
                auto &states = dev_state;
                std::set<State *> visited;
                std::vector<std::vector<State *>> newGroups;
                for (auto &i: states->getStates()) {
                    if (visited.count(i)) continue;
                    std::vector<State *> add;
                    add.push_back(i);
                    for (auto &b: states->getStates()) {
                        if (visited.count(b)) continue;
                        if (i->getTransitions().size() == b->getTransitions().size()) {
                            std::map<char, DevideStates *> compaireAll;
                            for (auto &t: i->getTransitions()) {
                                auto work = dynamic_cast<SymbolTransition *>(t);
                                compaireAll[work->getSymbol()] = findDevide(t->getNextState());
                            }
                            bool checkCompaire = true;
                            for (auto &t: b->getTransitions()) {
                                auto work = dynamic_cast<SymbolTransition *>(t);
                                if (!compaireAll.count(work->getSymbol())) {
                                    checkCompaire = false;
                                    lever = true;
                                    break;
                                }
                                if (compaireAll[work->getSymbol()] != findDevide(work->getNextState())) {
                                    checkCompaire = false;
                                    lever = true;
                                    break;
                                }
                            }
                            if (checkCompaire) {
                                add.push_back(b);
                                visited.insert(b);
                            }
                        } else {
                            lever = true;
                        }
                    }
                    newGroups.push_back(add);
                    visited.insert(i);
                }
                if (lever) {
                    splitDevideStates(dev_state, newGroups);
                    break;
                }
            }
        }
    }

    std::map<DevideStates const*, State *> groups;
    for (auto &i : devides_) {
        State * newState;
        if(i->isFinished()) { newState = new State(true); }
        else { newState = new State(false); }
        groups[i] = newState;
    }

    for (auto &i : devides_) {
        std::set<char> visitedTransitions;
        for (auto &b : i->getStates()) {
            for (auto &t : b->getTransitions()) {
                auto work = dynamic_cast<SymbolTransition*>(t);
                if(!visitedTransitions.count(work->getSymbol())) {
                    State * to_state = groups[findDevide(work->getNextState())];
                    auto to_trans = new SymbolTransition(to_state, work->getSymbol());
                    groups[i]->addTransition(to_trans);
                    visitedTransitions.insert(work->getSymbol());
                }
            }
        }
    }

    for (auto &i : devides_) {
        std::set<char> visitedTransitions;
        for (auto &b : i->getStates()) {
            if(b == old_start) return groups[i];
        }
    }
    throw std::logic_error("optimize DFA error");
}

void DFA_Automata::optimize() {
    if(!start_) return;

    auto finishedStates = new DevideStates(true);
    auto ordinaryStates = new DevideStates(false);
    std::set<State *> visited;
    std::stack<State*> stack;
    stack.push(start_);
    while (!stack.empty()) {
        State * working = stack.top();
        stack.pop();
        if(working->isFinishState()) finishedStates->addState(working);
        else ordinaryStates->addState(working);

        for (auto &i : working->getTransitions()) {
            if(!visited.count(i->getNextState())) stack.push(i->getNextState());
        }

        visited.insert(working);
    }

    DevideStatesCollector devideStatesCollector(ordinaryStates, finishedStates);
    start_ =  devideStatesCollector.devideAutomata(start_);
}
/*
bool DFA_Automata::checkStr(const std::string & string) {
    State * actualState = start_;
    bool isAccept = actualState->isFinishState();

    for (auto &i : string) {
        auto next = next_state(i, actualState);
        if(!next.second) return false;
        actualState = next.first;
        isAccept = actualState->isFinishState();
    }

    return  isAccept;
}
 */