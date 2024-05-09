#include "NFA.h"
#include "syntaxTree.h"
#include <map>
#include <stack>
#include <set>
#include <fstream>


// State

bool State::addTransition(Transition *transition) {
    if(!transition) return false;
    transitions_.push_back(transition);
    return true;
}

State::State(bool isFinishState) { isFinishState_ = isFinishState; }

State::State(std::initializer_list<Transition *> const& transitions) {
    for (auto &i : transitions) { transitions_.push_back(i); }
}

void State::finishState() { isFinishState_ = true; }

bool State::isFinishState() noexcept { return isFinishState_; }

bool State::isCycle() const noexcept { return can_cycle_; }

void State::cycle() { can_cycle_ = true; }

std::vector<Transition*> &State::getTransitions() noexcept { return transitions_; }

State::~State() { for (auto &i : transitions_)  delete i; }

CaptureGroupState::CaptureGroupState(Transition * transition, std::string group_name) : State({transition}) {
    group_name_ = std::move(group_name);
}

CaptureGroupState::CaptureGroupState(std::string group_name) : State() {
    group_name_ = std::move(group_name);
}

bool CaptureGroupState::isStart() const { return isStart_; }

bool CaptureGroupState::isFinish() const { return isFinish_; }

void CaptureGroupState::startCaptureGroup() { isStart_ = true; }

void CaptureGroupState::finishCaptureGroup() { isFinish_ = true; }

void CaptureGroupState::addInfo(std::string const& group_name, bool is_Start, bool isFinish) {
    if(captureInfo.count(group_name)) {
        auto prev_info = captureInfo[group_name];
        captureInfo[group_name] = {is_Start || prev_info.first, isFinish || prev_info.second};
    } else {
        captureInfo[group_name] = {is_Start, isFinish};
    }
}

std::string CaptureGroupState::getCaptureGroupName() { return group_name_; }

// Transition

char Transition::getPriority() const { return priority_; }

Transition::Transition(State *next_state, char priority) {
    if(next_state) { next_state_ = next_state; }
    priority_ = priority;
}

const std::vector<Transition *> &State::getTransitions() const noexcept { return transitions_; }

Transition::Transition(State *next_state) {
    if(next_state) { next_state_ = next_state; }
}

State *Transition::getNextState() noexcept { return next_state_; }

Transition::~Transition() noexcept { }

SymbolTransition::SymbolTransition(State *next_state_, char sym) : Transition(next_state_) {
    sym_ = sym;
}

char SymbolTransition::getSymbol() const noexcept { return sym_; }

EpsilonTransition::EpsilonTransition(State *next_state_) : Transition(next_state_) {}

EpsilonTransition::EpsilonTransition(State *next_state_, char priority) : Transition(next_state_, priority) {}

// Automata

NFA_Automata::NFA_Automata(State *begin, State *end) {
    if(!begin || !end) { return; }
    begin_connector_ = begin;
    end_connector_ = end;
}

State *NFA_Automata::getBeginConnector() const noexcept { return begin_connector_; }

State *NFA_Automata::getEndConnector() const noexcept { return end_connector_; }

bool NFA_Automata::addTransition(Transition *transition) {
    if(!transition) return false;
    return end_connector_->addTransition(transition);
}

void NFA_Automata::dismissConnectors() {
    begin_connector_ = nullptr;
    end_connector_ = nullptr;
}

// Automata

NFA_Automata *Node::createAutomata() { throw std::logic_error("Node don't have automata"); }

NFA_Automata *SymbolNode::createAutomata() {
    auto beginState = new State();
    auto endState = new State();
    auto transition = new SymbolTransition(endState, s_);
    beginState->addTransition(transition);
    auto automata = new NFA_Automata(beginState, endState);
    return automata;
}

NFA_Automata *CaptureGroupNode::createAutomata() {
    auto prev_automata= getNode()->createAutomata();

    auto beginState = new CaptureGroupState(name_);
    beginState->startCaptureGroup();
    auto beginTransition = new EpsilonTransition(prev_automata->getBeginConnector());
    beginState->addTransition(beginTransition);

    auto endState = new CaptureGroupState(name_);
    endState->finishCaptureGroup();
    auto endTransition = new EpsilonTransition(endState);
    prev_automata->addTransition(endTransition);

    auto automata = new NFA_Automata(beginState, endState);
    prev_automata->dismissConnectors();
    delete prev_automata;
    return automata;
}

NFA_Automata *EmptyNode::createAutomata() {
    auto beginState = new State();
    auto endState = new State();
    auto transition = new Transition(endState);
    beginState->addTransition(transition);

    auto automata = new NFA_Automata(beginState, endState);
    return automata;
}

NFA_Automata *KleenyStar::createAutomata() {
    auto prev_automata= getNode()->createAutomata();
    auto reverse_prev_automata_transit = new EpsilonTransition(prev_automata->getBeginConnector());
    prev_automata->getEndConnector()->addTransition(reverse_prev_automata_transit);

    auto beginState = new State();
    auto begin_to_prev_transit = new EpsilonTransition(prev_automata->getBeginConnector(), 1);
    beginState->addTransition(begin_to_prev_transit);

    auto endState = new State();
    auto prev_to_end_transit = new EpsilonTransition(endState);
    prev_automata->getEndConnector()->addTransition(prev_to_end_transit);

    auto begin_to_end_transit = new EpsilonTransition(endState);
    beginState->addTransition(begin_to_end_transit);

    auto automata = new NFA_Automata(beginState, endState);
    prev_automata->dismissConnectors();
    delete prev_automata;
    return automata;
}

NFA_Automata *Optional::createAutomata() {
    auto prev_automata= getNode()->createAutomata();

    auto beginState = new State();
    auto begin_to_prev_transit = new EpsilonTransition(prev_automata->getBeginConnector(), 1);
    beginState->addTransition(begin_to_prev_transit);

    auto endState = new State();
    auto prev_to_end_transit = new EpsilonTransition(endState);
    prev_automata->getEndConnector()->addTransition(prev_to_end_transit);

    auto begin_to_end_transit = new EpsilonTransition(endState);
    beginState->addTransition(begin_to_end_transit);

    auto automata = new NFA_Automata(beginState, endState);
    prev_automata->dismissConnectors();
    delete prev_automata;
    return automata;
}

NFA_Automata *MatchTimes::createAutomata() {
    unsigned long count = getCount();
    auto beginState = new State();
    State * prev_new_state = beginState;

    while (count) {
        auto prev_automata = getNode()->createAutomata();
        auto new_prev_to_prev_transition = new EpsilonTransition(prev_automata->getBeginConnector());
        prev_new_state->addTransition(new_prev_to_prev_transition);
        prev_new_state = prev_automata->getEndConnector();
        prev_automata->dismissConnectors();
        delete prev_automata;
        --count;
    }

    auto endState = new State();
    auto prev_to_end_transition = new EpsilonTransition(endState);
    prev_new_state->addTransition(prev_to_end_transition);

    auto automata = new NFA_Automata(beginState, endState);
    return automata;
}

NFA_Automata *OrNode::createAutomata() {
    auto left_automata= getLeft()->createAutomata();
    auto right_automata= getRight()->createAutomata();

    auto beginState = new State();
    auto begin_to_left_transit = new EpsilonTransition(left_automata->getBeginConnector(), 1);
    auto begin_to_right_transit = new EpsilonTransition(right_automata->getBeginConnector());
    beginState->addTransition(begin_to_left_transit);
    beginState->addTransition(begin_to_right_transit);

    auto endState = new State();
    auto left_to_end_transit = new EpsilonTransition(endState);
    auto right_to_end_transit = new EpsilonTransition(endState);
    left_automata->getEndConnector()->addTransition(left_to_end_transit);
    right_automata->getEndConnector()->addTransition(right_to_end_transit);

    left_automata->dismissConnectors();
    delete left_automata;
    right_automata->dismissConnectors();
    delete right_automata;

    auto automata = new NFA_Automata(beginState, endState);
    return automata;
}

NFA_Automata *AndNode::createAutomata() {
    auto left_automata= getLeft()->createAutomata();
    auto right_automata= getRight()->createAutomata();
/*
    for (auto &i : right_automata->getBeginConnector()->getTransitions()) {
        left_automata->getEndConnector()->addTransition(i);
        i = nullptr;
    }

    auto beginState = left_automata->getBeginConnector();
    auto endState = right_automata->getEndConnector();

    delete right_automata->getBeginConnector();
*/

    auto beginState = left_automata->getBeginConnector();
    auto endState = right_automata->getEndConnector();

    auto trans = new EpsilonTransition(right_automata->getBeginConnector());
    left_automata->getEndConnector()->addTransition(trans);

    left_automata->dismissConnectors();
    delete left_automata;
    right_automata->dismissConnectors();
    delete right_automata;

    auto automata = new NFA_Automata(beginState, endState);
    return automata;
}

NFA_Automata *Expression::createAutomata() {
    return getNode()->createAutomata();
}

void NFA_Automata::print() {
    unsigned int count = 0;
    std::map<State*, unsigned int> count_state_;
    std::set<State*> visited_;
    std::stack<State*> stack_;
    stack_.push(begin_connector_);
    count_state_[begin_connector_] = count;
    ++count;

    while (!stack_.empty()) {
        State *processing = stack_.top();
        stack_.pop();
        if (!visited_.count(processing)) {
            for (auto &i: processing->getTransitions()) {
                if (!count_state_.count(i->getNextState())) {
                    count_state_[i->getNextState()] = count;
                    ++count;
                }
                if(compaireTransition<EpsilonTransition>(i)) {
                    std::cout << count_state_[processing] << " - epsilon - " << count_state_[i->getNextState()] << std::endl;
                } else {
                    auto transition = dynamic_cast<SymbolTransition*>(i);
                    std::cout << count_state_[processing] << " - " << transition->getSymbol() << " - " << count_state_[i->getNextState()] << std::endl;
                }
                stack_.push(i->getNextState());
            }
        }
        visited_.insert(processing);
    }
}



std::vector<State*> NFA_Automata::findAvailableStates(std::vector<State*> const& states, std::stack<std::pair<State*, std::string::const_iterator>> &stack, std::string::const_iterator const& iter) {
    std::vector<State*> res;
    std::stack<State*> wait_for_process;
    std::set<State*> visited;

    for (auto &i : states) {
        wait_for_process.push(i);
    }

    while (!wait_for_process.empty()) {
        State * working = wait_for_process.top();
        wait_for_process.pop();

        bool hasHighPriority = false;
        std::vector<State *> lowPriority;
        for ( auto &i : working->getTransitions()) {
            if(compaireTransition<SymbolTransition>(i)) { res.push_back(working); continue; }
            else {
                if(!visited.count(i->getNextState())) {
                    wait_for_process.push(i->getNextState());
                    visited.insert(i->getNextState());
                } else continue;

            }
            if(i->getNextState() == end_connector_) res.push_back(i->getNextState());
            if(i->getPriority() == 1) { hasHighPriority = true; }
            else lowPriority.push_back(i->getNextState());
        }
        if(hasHighPriority) {
            std::reverse(lowPriority.begin(), lowPriority.end());
            for(auto &i : lowPriority) stack.emplace(i, iter);
        } else {
            for(auto &i : lowPriority) wait_for_process.push(i);
        }
    }
    return res;
}

std::vector<State*> NFA_Automata::step(std::vector<State*> & states, char sym) {
    std::set<State*> visited;
    std::vector<State*> stepStates;
    for (auto &i : states) {
        if(i == end_connector_) continue;
        for (auto &b : i->getTransitions()) {
            if(dynamic_cast<SymbolTransition*>(b)->getSymbol() == sym) {
                if(visited.count(b->getNextState())) { continue; }
                stepStates.push_back(b->getNextState());
                visited.insert(b->getNextState());
            }
        }
    }
    return stepStates;
}

bool NFA_Automata::checkFinalStateAll(const std::vector<State *> &states) {
    std::set<State*> visited;
    std::vector<State*> res;
    std::stack<State *> stack;

    for (auto &i : states) {
        stack.push(i);
    }

    while (!stack.empty()) {
        auto working = stack.top();
        stack.pop();
        for (auto &i : working->getTransitions()) {
            if(compaireTransition<EpsilonTransition>(i)) {
                if (i->getNextState() == end_connector_) return true;
                if (!visited.count(i->getNextState())) { stack.push(i->getNextState()); }
            }
        }
        visited.insert(working);
    }
    return false;
}

bool NFA_Automata::checkFinalStateNext(const std::vector<State *> &states) {
    for (auto &i : states) {
        if(i ==end_connector_) return true;
    }

    return false;
}

bool checkDownStep(std::vector<State *> const& states, std::vector<State *> const& next_states) {
    for (auto &i : states) {
        for (auto &b : next_states) {
            for (auto &c : b->getTransitions()) {
                if(c->getNextState() == i) return true;
            }
        }
    }
    return false;
}

const std::map<std::string, std::pair<bool, bool>> &CaptureGroupState::getInfo() const noexcept {
    return captureInfo;
}

void NFA_Automata::printDOT(const std::string &file_name) {
    std::ofstream file_(file_name);
    file_ << "digraph G {" << std::endl << "rankdir=LR" << std::endl;

    unsigned int count = 0;
    std::map<State*, unsigned int> count_state_;
    std::set<State*> visited_;
    std::stack<State*> stack_;
    stack_.push(begin_connector_);
    count_state_[begin_connector_] = count;
    if(dynamic_cast<CaptureGroupState*>(begin_connector_)) {
        file_ << count << " [shape = Mcircle, label =\"" << count << "\"]" << std::endl;
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
                    if(i->getNextState() == end_connector_) {
                        file_ << count << " [shape = doublecircle, label =\"" << count << "\"]" << std::endl;
                    } else if(dynamic_cast<CaptureGroupState*>(i->getNextState())) {
                        file_ << count << " [shape = Mcircle, label =\"" << count << "\"]" << std::endl;
                    } else {
                        file_ << count << " [shape = circle, label =\"" << count << "\"]" << std::endl;
                    }
                    ++count;
                }
                if(compaireTransition<EpsilonTransition>(i)) {
                    file_ << count_state_[processing] << "->" << count_state_[i->getNextState()] << "[label = \"eps pr:" << int(i->getPriority()) << "\"]" << std::endl;
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

    std::string command_dot = "dot -Tpng " + file_name + " -oNFA.png";
    system(command_dot.c_str());
}