#include "HiearchyOperations.h"

#include <iostream>
#include <list>
#include <utility>
#include <charconv>

#define UNCORRECTED false
#define CORRECT true

const char *Errors::what() const noexcept { return message_.c_str(); }

Errors::Errors(std::string message): message_(std::move(message)) {}

// SETTINGS

bool isAlpha(char sym) {
    if( (sym >= 'a' && sym <= 'z') || (sym >= 'A' && sym <= 'Z')) return true;
    return false;
}

bool isAlnum(char sym){
    if(isAlpha(sym) || (sym >= '0' && sym <= '9')) return true;
    return false;
}

bool isDigit(char sym){
    if(sym >= '0' && sym <= '9') return true;
    return false;
}

bool isAlphabet(char sym) {
    if (    sym != '%' &&
            sym != '|' &&
            sym != '.' &&
            sym != '?' &&
            sym != '{' &&
            sym != '}' &&
            sym != '<' &&
            sym != '>' &&
            sym != '(' &&
            sym != ')'
            ) return true;
    return false;
}

// Operations with container

str_container::iterator str_container::replace(str_container::container & list,
                                               str_container::iterator &begin,
                                               str_container::iterator &end,
                                               Node *newNode) {
    str_container::iterator res_;
    for (auto e_i = begin;  e_i != end; ++e_i) {
        delete *e_i;
    }
    res_ = list.erase(begin, end);
    res_ = list.insert(res_, newNode);
    return res_;
}

str_container::iterator str_container::merge(str_container::container & list,
                                               str_container::iterator &begin,
                                               str_container::iterator &end,
                                               Node *newNode) {
    str_container::iterator res_;
    res_ = list.erase(begin, end);
    res_ = list.insert(res_, newNode);
    return res_;
}

// Operations

bool SymbolOP::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    for (auto &i = begin; i != end; ++i) {
        if(isAlphabet((*i)->getSymbol())) {
            auto newNode = new SymbolNode((*i)->getSymbol());
            delete *i;
            *i = newNode;
        }
    }
    return CORRECT;
}

bool ShieldOP::compaire(str_container::container& list, //!!!
                        str_container::iterator begin,
                        str_container::iterator end) {
    auto iterFirstShield = end;
    int lenghtAfterShield = 0;
    for (auto &i = begin; i != end ; ++i) {
        if((*i)->getSymbol() == '%' && lenghtAfterShield == 0) {
            lenghtAfterShield = 1;
            iterFirstShield = i;
        } else
        if(lenghtAfterShield == 1) {
            lenghtAfterShield = 2;
        } else
        if((*i)->getSymbol() == '%' && lenghtAfterShield == 2) {
            auto mid = iterFirstShield;
            ++mid;
            ++i;
            char data = (*mid)->getSymbol();
            auto newNode = new SymbolNode(data);
            i = str_container::replace(list, iterFirstShield, i, newNode);
            iterFirstShield = end;
            lenghtAfterShield = 0;
        } else if(lenghtAfterShield != 0) {
            throw std::logic_error("Wrong '%'");
        }
    }
    if(lenghtAfterShield) throw std::logic_error("Wrong '%'");
    return UNCORRECTED;
}

bool ExpressionPart::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    if((*begin)->getSymbol() != '(' || (*end)->getSymbol() != ')') throw std::logic_error("Uncorrect expression");
    size_t distance = std::distance(begin, end);
    auto next_end_iter = end;
    ++next_end_iter;
    if(distance < 2) {
        auto newNode = new EmptyNode;
        auto newExpr = new Expression(newNode);
        newExpr->getNode()->addParent(newExpr);
        str_container::replace(list, begin, next_end_iter, newExpr);
    } else if(distance == 2) {
        auto next_iter = begin;
        ++next_iter;
        auto node = *next_iter;
        *next_iter = nullptr;
        auto newExpr = new Expression(node);
        newExpr->getNode()->addParent(newExpr);
        str_container::replace(list, begin, next_end_iter, newExpr);
    } else if(distance == 3) {
        auto capt_group_iter = begin;
        ++capt_group_iter;
        auto expession_iter = capt_group_iter;
        ++expession_iter;

        auto capt_group = dynamic_cast<CaptureGroupNode*>(*capt_group_iter);
        if(!capt_group->addNode(*expession_iter)) return UNCORRECTED;
        (*expession_iter)->addParent(capt_group);
        *capt_group_iter = nullptr;
        *expession_iter = nullptr;

        auto newExpr = new Expression(capt_group);
        newExpr->getNode()->addParent(newExpr);
        str_container::replace(list, begin, next_end_iter, newExpr);
    } else throw std::logic_error("ExpressionPart Feilure");
    return CORRECT;
}

bool CaptureGroupOP::compaire(str_container::container &list,
                         str_container::iterator begin,
                         str_container::iterator end) {
    bool hasAlpha = false;
    bool hasFirstSymbolName = false;
    std::string buffer;
    auto start = begin;
    ++start;

    if((*begin)->getSymbol() != '<') return UNCORRECTED;
    for (auto i = start; i != end; ++i) {
        char sym = (*i)->getSymbol();
        if(isAlpha(sym)) {
            hasAlpha = true;
            hasFirstSymbolName = true;
            buffer.push_back(sym);
        } else if(hasFirstSymbolName && sym == '>') {
            auto newNode = new CaptureGroup(buffer);
            str_container::replace(list, begin, ++i, newNode);
            return CORRECT;
        } else if (hasAlpha && isAlnum(sym)) {
            buffer.push_back(sym);
        } else break;
    }
    throw std::logic_error("Uncorrect capture group construction");
}

bool RepeatsOP::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    if(compaireNode<CaptureGroup>(*begin)) ++begin;
    auto symbolNodeIter = begin;
    auto prev_iter = begin;
    bool hasDigit = false;
    bool hasLeftBracket = false;
    std::string buffer;

    if((*begin)->getSymbol() == '{') throw std::logic_error("Uncorrect repeat conctruction");
    for (auto &i = begin; i != end ; ++i) {
        char sym = (*i)->getSymbol();
        if(sym == '}' && !hasLeftBracket) {
            *i = dynamic_cast<SymbolNode*>(*i);
            prev_iter = i;
            continue;
        }
        if(sym == '}' && !hasDigit) throw std::logic_error("Uncorrect repeat conctruction");
        if(sym == '{' && !hasLeftBracket && (compaireNode<SymbolNode>(*prev_iter) || (compaireNode<Expression>(*prev_iter)))) {
            hasLeftBracket = true;
            symbolNodeIter = prev_iter;
        } else if(sym == '{') throw std::logic_error("Uncorrect repeat conctruction");
        else if(isDigit(sym) && hasLeftBracket) {
            hasDigit = true;
            buffer.push_back(sym);
        } else if(sym == '}' && hasDigit && hasLeftBracket) {
            unsigned int count = 0;
            auto res_ = std::from_chars(buffer.data(), buffer.data() + buffer.size(), count);
            if(res_.ec == std::errc::result_out_of_range) throw std::logic_error("Number in repeat construction is so much");
            auto newNode = new MatchTimes(*symbolNodeIter, count);
            auto symbolNode = *symbolNodeIter;
            *symbolNodeIter = nullptr;
            symbolNode->addParent(newNode);
            auto next_iter_ = i; ++next_iter_;
            i = str_container::replace(list, symbolNodeIter, next_iter_, newNode);
            hasLeftBracket = false;
            hasDigit = false;
            buffer.clear();
        } else if(!isDigit(sym) && (hasLeftBracket || hasDigit)) throw std::logic_error("Uncorrect repeat conctruction");
        prev_iter = i;
    }
    if(hasLeftBracket) throw std::logic_error("Uncorrect repeat conctruction");
    return CORRECT;
}

bool OptionalOP::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    if(compaireNode<CaptureGroup>(*begin)) ++begin;
    auto prev_iter = begin;

    if((*begin)->getSymbol() == '?') throw std::logic_error("Uncorrect optional conctruction");
    for (auto i = begin;  i != end ; ++i) {
        if((*i)->getSymbol() == '?'){
            if(     !compaireNode<SymbolNode>(*i)               &&
                    (compaireNode<SymbolNode>(*prev_iter)       ||
                    compaireNode<Expression>(*prev_iter)        ||
                    compaireNode<KleenyStar>(*prev_iter))) {
                auto symNode = *prev_iter;
                *prev_iter = nullptr;
                auto newNode = new Optional(symNode);
                symNode->addParent(newNode);
                auto next_iter = i;
                ++next_iter;
                i = str_container::replace(list, prev_iter, next_iter, newNode);
            } else if(compaireNode<SymbolNode>(*i)) { prev_iter = i; continue;}
            else throw std::logic_error("Uncorrect optional conctruction");
        }
        prev_iter = i;
    }
    return CORRECT;
}

bool KleenyStarOP::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    if(compaireNode<CaptureGroup>(*begin)) ++begin;
    auto real_end = end;
    --real_end;
    auto real_begin = begin;
    --real_begin;
    size_t count_dot = 0;
    auto end_kleeny_contruction = real_end;
    for (auto &i = real_end; i != real_begin ; --i) {
        if(count_dot == 3) {
            if( compaireNode<SymbolNode>(*i) ||
                compaireNode<Expression>(*i))
            {
                auto newNode = new KleenyStar(*i);
                newNode->getNode()->addParent(newNode);
                *i = nullptr;
                i = str_container::replace(list, i, end_kleeny_contruction, newNode);
            } else if((*i)->getSymbol() == '.'){
                auto newSym = new SymbolNode('.');
                auto newNode = new KleenyStar(newSym);
                newNode->getNode()->addParent(newNode);
                delete *i; *i = nullptr;
                i = str_container::replace(list, i, end_kleeny_contruction, newNode);
            } else throw std::logic_error("Uncorrect Kleeny contruction");
            count_dot = 0;
        } else if ((*i)->getSymbol() == '.' && !compaireNode<SymbolNode>(*i)) {
            if(!count_dot) { end_kleeny_contruction = i; end_kleeny_contruction++; }
            count_dot++;
        } else if (count_dot && ((*i)->getSymbol() != '.' || compaireNode<SymbolNode>(*i))) {
            auto next_iter = i;
            ++next_iter;
            for (auto &b = next_iter; b != end_kleeny_contruction; ++b) {
                auto newNode = new SymbolNode('.');
                delete *b;
                *b = newNode;
            }
            count_dot = 0;
        }
    }
    if(count_dot == 3) throw std::logic_error("Uncorrect Kleeny contruction");
    if(count_dot > 0 && count_dot < 3) {
        for (auto &i = begin; i != end_kleeny_contruction; ++i) {
            auto newNode = new SymbolNode('.');
            delete *i;
            *i = newNode;
        }
    }
    return CORRECT;
}

bool ConcatenationOP::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    if(compaireNode<CaptureGroup>(*begin)) ++begin;
    if(std::distance(begin, end) == 0) return CORRECT;
    auto prev_iter = begin;
    ++begin;
    for (auto &i = begin; i != end ; ++i) {
        if(compaireNode<DefineNode>(*i) && compaireNode<DefineNode>(*prev_iter)) {
            auto newNode = new AndNode(*prev_iter, *i);
            newNode->getRight()->addParent(newNode);
            newNode->getLeft()->addParent(newNode);
            *i = nullptr;
            *prev_iter = nullptr;
            auto next_iter = i;
            ++next_iter;
            i = str_container::replace(list, prev_iter, next_iter, newNode);
        } else if((*i)->getSymbol() == '|' && compaireNode<DefineNode>(*prev_iter)) {
            if(std::distance(i, end) == 1) { continue; }
            ++i;
            prev_iter = i;
            continue;
        } else if((*prev_iter)->getSymbol() == '|' && !compaireNode<SymbolNode>(*prev_iter)) {
            prev_iter = i;
            continue;
        }
        else throw std::logic_error("Concatenation failure");
        prev_iter = i;
    }
    return CORRECT;
}

bool OrOP::compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) {
    if(compaireNode<CaptureGroup>(*begin)) ++begin;
    auto prev_iter = begin;
    auto next_iter = begin;
    auto start = begin;

    for (auto &i = begin; i != end ; ++i) {
        next_iter = i;
        ++next_iter;
        if((*i)->getSymbol() == '|' && !compaireNode<SymbolNode>(*i)) {
            if((*next_iter)->getSymbol() == '|' &&  !compaireNode<SymbolNode>(*next_iter)) {
                Node * firstNode;

                if(i == start) firstNode = new EmptyNode;
                else firstNode = *prev_iter;

                auto secondNode = new EmptyNode;
                auto newNode = new OrNode(firstNode, secondNode);
                newNode->getLeft()->addParent(newNode);
                newNode->getRight()->addParent(newNode);
                *prev_iter = nullptr;
                str_container::replace(list, prev_iter, next_iter, newNode);
            } else if(compaireNode<DefineNode>(*next_iter)) {
                Node * firstNode;
                if(i == start) { firstNode = new EmptyNode; }
                else { firstNode = *prev_iter; }

                auto newNode = new OrNode(firstNode, *next_iter);
                newNode->getLeft()->addParent(newNode);
                newNode->getRight()->addParent(newNode);
                *prev_iter = nullptr;
                *next_iter = nullptr;
                auto next_next_iter = next_iter;
                ++next_next_iter;
                i = str_container::replace(list, prev_iter, next_next_iter, newNode);
            } else if (next_iter == end) {
                Node * firstNode;

                if(i == start) firstNode = new EmptyNode;
                else firstNode = *prev_iter;

                auto secondNode = new EmptyNode;
                auto newNode = new OrNode(firstNode, secondNode);
                newNode->getLeft()->addParent(newNode);
                newNode->getRight()->addParent(newNode);
                *prev_iter = nullptr;
                str_container::replace(list, prev_iter, next_iter, newNode);
            }
        }
        prev_iter = i;
    }
    return CORRECT;
}

bool ConcatenationInverseOP::compaire(str_container::container &list, str_container::iterator begin,
                                      str_container::iterator end) {
    if(compaireNode<CaptureGroup>(*begin)) ++begin;
    if(std::distance(begin, end) == 0) return CORRECT;
    auto prev_iter = begin;
    ++begin;
    for (auto &i = begin; i != end ; ++i) {
        if(compaireNode<DefineNode>(*i) && compaireNode<DefineNode>(*prev_iter)) {
            auto newNode = new AndNode(*i, *prev_iter);
            newNode->getRight()->addParent(newNode);
            newNode->getLeft()->addParent(newNode);
            *i = nullptr;
            *prev_iter = nullptr;
            auto next_iter = i;
            ++next_iter;
            i = str_container::replace(list, prev_iter, next_iter, newNode);
        } else if((*i)->getSymbol() == '|' && compaireNode<DefineNode>(*prev_iter)) {
            if(std::distance(i, end) == 1) { continue; }
            ++i;
            prev_iter = i;
            continue;
        } else if((*prev_iter)->getSymbol() == '|' && !compaireNode<SymbolNode>(*prev_iter)) {
            prev_iter = i;
            continue;
        }
        else throw std::logic_error("Concatenation failure");
        prev_iter = i;
    }
    return CORRECT;
}

// opLists

const std::vector<Operation*> &opListEmpty::getList() { return opList_Empty; }

opListEmpty::~opListEmpty() noexcept {
    for (auto &i : opList_Empty) { delete i; }
}

const std::vector<Operation*> &opListPrimary::getList() { return opList_Primary; }

opListPrimary::~opListPrimary() noexcept {
    for (auto &i : opList_Primary) { delete i; }
}

const std::vector<Operation*> &opListOrdinary::getList() { return opList_Ordinary; }

opListOrdinary::~opListOrdinary() noexcept {
    for (auto &i : opList_Ordinary) { delete i; }
}

const std::vector<Operation*> &opListMakeExpression::getList() { return opList_MakeExpression;  }

opListMakeExpression::~opListMakeExpression() noexcept {
    for (auto &i : opList_MakeExpression) { delete i; }
}

const std::vector<Operation*> &opListOrdinaryInverse::getList() { return opList_OrdinaryInverse; }

opListOrdinaryInverse::~opListOrdinaryInverse() noexcept {
    for (auto &i : opList_OrdinaryInverse) { delete i; }
}

const std::vector<Operation*> &opListOrdinaryWithoutGroups::getList() { return opList_Ordinary_WG; }

opListOrdinaryWithoutGroups::~opListOrdinaryWithoutGroups() noexcept {
    for (auto &i : opList_Ordinary_WG) { delete i; }
}

// OperationListCollector

const std::vector<Operation*> &OperationListCollector::getList(stage_options::stage_option_type opt) {
    if(collector_.count(opt)) {
        return collector_.find(opt)->second->getList();
    }
    throw std::logic_error("Value of 'stage_option_type' is not added to 'OperationListCollector'");
}

OperationListCollector::~OperationListCollector() noexcept {
    for (auto &i : collector_) delete i.second;
}

// operationHiearchy

operationHiearchy::operationHiearchy(stage_options::stage_option_type opt) {
    opList_ = OperationListCollector::getList(opt);
}

// DirectStages

DirectStages::DirectStages(str_container::container & list,
                           str_container::iterator str_begin,
                           str_container::iterator str_end,
                           stage_options::stage_option_type opt) : operationHiearchy(opt), str_(list) {
    state_ = opList_.begin();
    end_ = opList_.end();
    str_begin_ = str_begin;
    str_end_ = str_end;
}

DirectStages::DirectStages(const DirectStages &directStages) : operationHiearchy(directStages.opt_), str_(directStages.str_) {
    opList_ = directStages.opList_;
    state_ = directStages.state_;
    end_ = directStages.end_;
    str_begin_ = directStages.str_begin_;
    str_end_ = directStages.str_end_;
    opt_ = directStages.opt_;
}

DirectStages &DirectStages::operator=(const DirectStages &directStages) {
    if (this != &directStages) {
        opList_ = directStages.opList_;
        state_ = opList_.begin();
        end_ = opList_.end();
        str_begin_ = directStages.str_begin_;
        str_end_ = directStages.str_end_;
        str_ = directStages.str_;
        opt_ = directStages.opt_;
    }
    return *this;
}

bool DirectStages::start() {
    state_ = opList_.begin();
    if(state_ == end_) return UNCORRECTED;
    return CORRECT;
}

bool DirectStages::transform() {
    start();

    for (auto i = state_; i != end_; ++i) {
        auto str_begin_1 = str_begin_;
        ++str_begin_1;
        // Processing string from str_begin_ to str_end_ [str_begin_, str_end_)
        (*i)->compaire(str_, str_begin_1, str_end_);
    }
    return CORRECT;
}

bool DirectStages::changeOption(stage_options::stage_option_type opt) {
    *this = DirectStages(str_, str_begin_, str_end_, opt);
    return true;
}