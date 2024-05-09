#ifndef LAB2_LANGOPERATIONS_H
#define LAB2_LANGOPERATIONS_H

#include "DFA.h"
#include "string"

class Expr {
    std::string expr_;
public:
    Expr() = default;
    explicit Expr(char sym);
    void addAND(Expr const& expr);
    void addOR(Expr const& expr);
    void addOptional();
    void addKleeny();
    [[nodiscard]] std::string getExpression() const noexcept;
};

class ExprState;

class ExprTransition {
    Expr expression_;
    ExprState * prev_state_;
    ExprState * next_state_;
public:
    explicit ExprTransition(ExprState * next_state, ExprState * prev_state, char sym);
    explicit ExprTransition(ExprState * next_state, ExprState * prev_state, Expr expr);
    [[nodiscard]] ExprState * getNextState() const noexcept;
    Expr & expr() noexcept;
    [[nodiscard]] Expr getExpression() const noexcept;
    ~ExprTransition() = default;
};

class ExprState {
    std::vector<ExprTransition*> transitions_;
    bool isFinishState_ = false;
public:
    ExprState() = default;
    explicit ExprState(bool isFinishState);
    bool addTransition(ExprTransition * transition);
    void finishState();

    [[nodiscard]] bool isFinishState() const noexcept;
    [[nodiscard]] std::vector<ExprTransition*> const& getTransitions() const noexcept;
    [[nodiscard]] std::vector<ExprTransition*> & getTransitions() noexcept;
    bool deleteTransition(ExprTransition * transition);
    ~ExprState() = default;
};

class AutomataConverter {
    ExprState * automata_start_ = nullptr;
    Expr expr_;
    std::set<ExprState*> findStates(bool isOrdinaryStates);
    ExprState * findOrdinaryFinish();
    std::vector<std::pair<ExprState *, ExprTransition*>> findInputStates(ExprState * victim);
    std::vector<std::pair<ExprState *, ExprTransition*>> findOutStates(ExprState * victim);
    ExprTransition * findTransition(ExprState * start, ExprState * finish);
    bool isOrdinaryFinish(ExprState * state);
    bool hasReverseStartTransition(ExprState * state);
public:
    explicit AutomataConverter(DFA_Automata const* automata);
    void convert();
    std::string getExpr() const;
    void printDOT(const std::string &file_name) const noexcept;
};

#endif //LAB2_LANGOPERATIONS_H

