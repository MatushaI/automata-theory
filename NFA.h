#ifndef LAB2_NFA_H
#define LAB2_NFA_H

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <stack>

class Transition;
class NFA_Automata;

template <class T, class Transition>
bool compaireTransition(Transition* t) { return dynamic_cast<T*> (t); }

class State {
protected:
    std::vector<Transition*> transitions_;
    bool isFinishState_ = false;
    bool can_cycle_ = false;
public:
    State() = default;
    State(bool isFinishState_);
    State(std::initializer_list<Transition*> const& transitions);
    bool addTransition(Transition * transition);
    void finishState();
    void cycle();
    [[nodiscard]] bool isCycle() const noexcept;
    [[nodiscard]] bool isFinishState() noexcept;
    [[nodiscard]] std::vector<Transition*> const& getTransitions() const noexcept;
    [[nodiscard]] std::vector<Transition*> & getTransitions() noexcept;
    virtual ~State();
};

class CaptureGroupState : public State {
    std::string group_name_;
    bool isStart_ = false;
    bool isFinish_ = false;
    std::map<std::string, std::pair<bool, bool>> captureInfo;
public:
    CaptureGroupState() = default;
    CaptureGroupState(Transition * transition, std::string group_name);
    explicit CaptureGroupState(std::string group_name);
    void startCaptureGroup();
    void finishCaptureGroup();
    void addInfo(std::string const& group_name, bool is_Start, bool isFinish);
    std::map<std::string, std::pair<bool, bool>> const& getInfo() const noexcept;
    std::string getCaptureGroupName();
    [[nodiscard]] bool isStart() const;
    [[nodiscard]] bool isFinish() const;
    ~CaptureGroupState() override = default;
};

class Transition {
protected:
    char priority_ = 0;
    State * next_state_ = nullptr;
public:
    [[nodiscard]] char getPriority() const;
    explicit Transition(State * next_state);
    Transition(State * next_state, char priority);
    [[nodiscard]] State * getNextState() noexcept;
    virtual ~Transition() noexcept;
};

class SymbolTransition : public Transition {
    char sym_;
public:
    SymbolTransition(State * next_state_, char sym);
    [[nodiscard]] char getSymbol() const noexcept;
    ~SymbolTransition() override = default;
};

class EpsilonTransition : public Transition {
public:
    explicit EpsilonTransition(State * next_state_);
    explicit EpsilonTransition(State * next_state_, char priority);
};

class NFA_Automata {
    State * begin_connector_ = nullptr;
    State * end_connector_ = nullptr;
    std::vector<State*> findAvailableStates(std::vector<State*> const& states, std::stack<std::pair<State*, std::string::const_iterator>> & stack, std::string::const_iterator const& iter);
    std::vector<State*> step(std::vector<State*> & states, char a);
    bool checkFinalStateAll(std::vector<State*> const& states);
    bool checkFinalStateNext(std::vector<State*> const& states);
public:
    NFA_Automata(State * begin, State * end);
    [[nodiscard]] State * getBeginConnector() const noexcept;
    [[nodiscard]] State * getEndConnector() const noexcept;
    void print();
    void printDOT(std::string const& file_name);
    bool addTransition(Transition * transition);
    void dismissConnectors();
};

#endif //LAB2_NFA_H