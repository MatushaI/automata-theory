#ifndef HIEARCHYOPERATIONS_H
#define HIEARCHYOPERATIONS_H

#include "syntaxTree.h"
#include <vector>
#include <map>
#include <string>

template<class T, class Node>
bool compaireNode(Node * node) { return dynamic_cast<T*>(node); }

namespace str_container {
    typedef std::list<Node*> container;
    typedef std::list<Node*>::iterator iterator;

    //Functions
    str_container::iterator replace(str_container::container&, str_container::iterator&, str_container::iterator&, Node*);
    str_container::iterator merge(str_container::container&, str_container::iterator&, str_container::iterator&, Node*);
}

//

class Errors : std::exception {
    std::string message_;
public:
    Errors() = default;
    explicit Errors(std::string  message);
    [[nodiscard]] const char * what() const noexcept override;
    ~Errors() override = default;
};

class Operation {
public:
    Operation() = default;
    virtual bool compaire(str_container::container & list, str_container::iterator begin, str_container::iterator end) = 0;
    virtual ~Operation() = default;
};

class SymbolOP : public Operation {
public:
    SymbolOP() = default;
    bool compaire(str_container::container& list, str_container::iterator begin, str_container::iterator end) final;
    ~SymbolOP() override = default;
};

class ShieldOP : public Operation {
public:
    ShieldOP() = default;
    bool compaire(str_container::container& list, str_container::iterator begin, str_container::iterator end) final;
    ~ShieldOP() override = default;
};

class ExpressionPart : public Operation {
public:
    ExpressionPart() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) override;
    ~ExpressionPart() override = default;
};

class CaptureGroupOP : public ExpressionPart {
public:
    CaptureGroupOP() = default;
    bool compaire(str_container::container& list, str_container::iterator begin, str_container::iterator end) final;
    ~CaptureGroupOP() override = default;
};

class RepeatsOP : public Operation {
public:
    RepeatsOP() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) final;
    ~RepeatsOP() override = default;
};

class OptionalOP : public Operation {
public:
    OptionalOP() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) final;
    ~OptionalOP() override = default;
};

class KleenyStarOP : public Operation {
public:
    KleenyStarOP() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) final;
    ~KleenyStarOP() override = default;
};

class ConcatenationOP : public Operation {
public:
    ConcatenationOP() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) final;
    ~ConcatenationOP() override = default;
};

class OrOP : public Operation {
public:
    OrOP() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) final;
    ~OrOP() override = default;
};

class ConcatenationInverseOP : public Operation {
public:
    ConcatenationInverseOP() = default;
    bool compaire(str_container::container &list, str_container::iterator begin, str_container::iterator end) final;
    ~ConcatenationInverseOP() override = default;
};

namespace stage_options {
    typedef unsigned char stage_option_type;
    inline constexpr stage_option_type empty = 0;
    inline constexpr stage_option_type primary = 1;
    inline constexpr stage_option_type ordinary = 2;
    inline constexpr stage_option_type ordinary_without_captureGroups = 3;
    inline constexpr stage_option_type brackets = 4;
    inline constexpr stage_option_type ordinary_inverse = 5;
}

class IOperationList {
protected:
    typedef std::vector<Operation*> operStorage;
    IOperationList() = default;
public:
    virtual ~IOperationList() noexcept = default;
    [[nodiscard]] virtual operStorage const& getList() = 0;
};

class opListEmpty : public IOperationList{
    static constexpr operStorage const opList_Empty = {};
public:
    opListEmpty() = default;
    operStorage const& getList() final;
    ~opListEmpty() noexcept override;
};

class opListPrimary : public IOperationList {
    static inline operStorage opList_Primary = {
            new ShieldOP,
            new SymbolOP
    };
public:
    opListPrimary() = default;
    operStorage const& getList() final;
    ~opListPrimary() noexcept override;
};

class opListOrdinary : public IOperationList {
    static inline operStorage opList_Ordinary = {
            new CaptureGroupOP,
            new RepeatsOP,
            new KleenyStarOP,
            new OptionalOP,
            new ConcatenationOP,
            new OrOP
    };
public:
    opListOrdinary() = default;
    operStorage const& getList() final;
    ~opListOrdinary() noexcept override;
};

class opListOrdinaryWithoutGroups : public IOperationList {
    static inline operStorage opList_Ordinary_WG = {
            new RepeatsOP,
            new KleenyStarOP,
            new OptionalOP,
            new ConcatenationOP,
            new OrOP
    };
public:
    opListOrdinaryWithoutGroups() = default;
    operStorage const& getList() final;
    ~opListOrdinaryWithoutGroups() noexcept override;
};

class opListOrdinaryInverse : public IOperationList {
    static inline operStorage opList_OrdinaryInverse = {
            new CaptureGroupOP,
            new RepeatsOP,
            new KleenyStarOP,
            new OptionalOP,
            new ConcatenationInverseOP,
            new OrOP
    };
public:
    opListOrdinaryInverse() = default;
    operStorage const& getList() final;
    ~opListOrdinaryInverse() noexcept override;
};

class opListMakeExpression : public IOperationList {
    static inline operStorage opList_MakeExpression = {
            new ExpressionPart
    };
public:
    opListMakeExpression() = default;
    operStorage const& getList() final;
    ~opListMakeExpression() noexcept override;
};

class OperationListCollector {
    typedef std::vector<Operation*> operStorage;
    static inline std::map<stage_options::stage_option_type, IOperationList*> const collector_ {
            {   stage_options::empty,               new opListEmpty           },
            {   stage_options::primary,             new opListPrimary         },
            {   stage_options::ordinary,            new opListOrdinary        },
            {   stage_options::brackets,            new opListMakeExpression  },
            {   stage_options::ordinary_inverse,    new opListOrdinaryInverse },
            {   stage_options::ordinary_without_captureGroups, new opListOrdinaryWithoutGroups }
    };
public:
    [[nodiscard]] static operStorage const& getList(stage_options::stage_option_type opt);
    OperationListCollector() = default;
    ~OperationListCollector() noexcept;
};

class operationHiearchy {
private:
    static inline OperationListCollector const listCollector_ = OperationListCollector();
protected:
    std::vector<Operation*> opList_;
    explicit operationHiearchy(stage_options::stage_option_type opt);
    ~operationHiearchy() = default;
};

class DirectStages : protected operationHiearchy {
    std::vector<Operation*>::iterator state_;
    std::vector<Operation*>::iterator end_;
    str_container::iterator str_begin_;
    str_container::iterator str_end_;
    str_container::container & str_;
    stage_options::stage_option_type opt_;
    bool start();
public:
    DirectStages(str_container::container & list, str_container::iterator str_begin_, str_container::iterator str_end_, stage_options::stage_option_type opt);
    DirectStages(DirectStages const& directStages);
    DirectStages & operator=(DirectStages const& directStages);
    bool transform();
    bool changeOption(stage_options::stage_option_type opt);
    ~DirectStages() = default;
};

#endif //HIEARCHYOPERATIONS_H
