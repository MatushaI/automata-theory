#ifndef LAB2_DFA_H
#define LAB2_DFA_H

#include "NFA.h"
#include <vector>
#include <set>
#include <map>
#include <compare>

namespace state_type {
    typedef char state_type_;
    inline constexpr state_type_ finish = 1;
    inline constexpr state_type_ ordinary  = 2;
}

class StatesGroup {
    std::set<State *> states_;
public:
    StatesGroup() = default;
    explicit StatesGroup(std::set<State *>);
    [[nodiscard]] bool hasEndState(State * endState) const;
    [[nodiscard]] std::set<State *> const& getStates() const;
    bool operator==(StatesGroup const& state_group) const;
    bool operator<(StatesGroup const& state_group) const;
    bool operator>(StatesGroup const& state_group) const;

    ~StatesGroup() = default;
};

class StatesGroupCollector {
    std::map<StatesGroup, State *> collector_;
public:
    StatesGroupCollector() = default;
    std::pair<State *, bool> findState(StatesGroup const& states_group);
    std::pair<StatesGroup const&, bool> findStatesGroup(State * state);
    std::pair<State *, bool> insert(StatesGroup const& states_group, State * state);
};

struct StateCaptureGroupInfo {
    std::string group_name_;
    bool isStart_;
    bool isFinish_;
};

class DFA_Automata {
protected:
    State * start_ = nullptr;
    State * actualState_ = nullptr;

    [[nodiscard]] static StatesGroup order_for_epsilon(State * state);
    [[nodiscard]] static StatesGroup order_for_epsilon(std::vector<State*> const& state);
    [[nodiscard]] std::map<char, std::vector<State*>> single_order_for_symbol(State * state, StatesGroupCollector & collector);
    [[nodiscard]] static State * addTransitionNewState(State *to_state, State * out_state, char transitionSymbol);
    [[nodiscard]] static State* createState(StatesGroup const& states_group, State * endState);
    static void addTransitionState(State * input_state, State * to_state, char transitionSymbol);
public:
    DFA_Automata() = default;
    explicit DFA_Automata(State * start);
    void synthesisFromNFA(const NFA_Automata * nfa_auto);
    void printDOT(std::string const& file_name);
    //bool checkStr(std::string const&); // TEST
    void optimize();

    [[nodiscard]] std::pair<bool, std::vector<StateCaptureGroupInfo>> getCaptureGroupInfo();
    [[nodiscard]] bool next_state(char sym);
    [[nodiscard]] bool isAccept() noexcept;
    [[nodiscard]] State * getStart() const noexcept;
    [[nodiscard]] State * getActualState() const noexcept;
    void start() noexcept;
    ~DFA_Automata() noexcept = default;
};

class DevideStates {
    bool isFinish_ = false;
    std::set<State*> states_;
public:
    DevideStates() = default;
    bool operator<=>(DevideStates const&) const;
    explicit DevideStates(bool isFinish);
    bool addState(State * state);
    State * deleteState(State * state);
    [[nodiscard]] std::set<State*> const& getStates() const noexcept;
    [[nodiscard]] bool isFinished() const noexcept;
};

class DevideStatesCollector {
    std::set<DevideStates *> devides_;
    [[nodiscard]] DevideStates * findDevide(State * state) const;
    std::pair<bool, DevideStates *> checkDevide(std::set<DevideStates *> const& devides);
    void splitDevideStates(DevideStates * devide, std::vector<std::vector<State*>> const& newGroups);
public:
    DevideStatesCollector(DevideStates * ordinary, DevideStates * finished);
    State * devideAutomata(State * old_start);
};


#endif //LAB2_DFA_H
