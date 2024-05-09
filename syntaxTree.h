#ifndef SYNTAXTREE_H
#define SYNTAXTREE_H

#include <list>
#include <string>
#include <map>
#include "NFA.h"

class AutomataBuilder {
protected:
    virtual NFA_Automata * createAutomata() = 0;
};

class UndefinedNode {
protected:
    std::string test_name_ = "Undef";
    char s_;
public:
    std::string getTestingName() { return test_name_; }
    UndefinedNode();
    [[nodiscard]] char getSymbol() const;
    explicit UndefinedNode(char sym);
    ~UndefinedNode() = default;
};

class Node : public UndefinedNode, public AutomataBuilder {
    Node * parent_ = nullptr;
public:
    Node() = default;
    explicit Node(char sym);
    bool addParent(Node * node);
    NFA_Automata * createAutomata() override;
    [[nodiscard]] const Node *parent();
    virtual ~Node() = default;
};

class DefineNode {
protected:
    DefineNode() = default;
    virtual ~DefineNode() = default;
};

class SymbolNode : public Node, public DefineNode {
public:
    SymbolNode() = default;
    explicit SymbolNode(char sym);
    NFA_Automata * createAutomata() override;
    ~SymbolNode() override = default;
};

class EmptyNode : public Node {
public:
    EmptyNode() = default;
    NFA_Automata * createAutomata() final;
    ~EmptyNode() override = default;
};

class UnaryNode : public Node, public DefineNode {
    Node * node_ = nullptr;
public:
    UnaryNode() = default;
    explicit UnaryNode(Node * node);
    [[nodiscard]] bool addNode(Node * node);
    [[nodiscard]] Node * getNode();
    ~UnaryNode() noexcept override;
};

class CaptureGroupNode : public UnaryNode {
    std::string name_;
public:
    explicit CaptureGroupNode(std::string name);
    NFA_Automata * createAutomata() override;
    [[nodiscard]] std::string getName();
    ~CaptureGroupNode() override = default;
};

class CaptureGroup : public CaptureGroupNode {
public:
    explicit CaptureGroup(std::string name);
    ~CaptureGroup() override = default;
};

class KleenyStar : public UnaryNode {
public:
    KleenyStar() = default;
    explicit KleenyStar(Node * node);
    NFA_Automata * createAutomata() final;
    ~KleenyStar() override = default;
};

class Optional : public UnaryNode {
public:
    Optional() = default;
    NFA_Automata * createAutomata() final;
    explicit Optional(Node * node);
    ~Optional() override = default;
};

class MatchTimes : public UnaryNode {
    unsigned int count_ = 0;
public:
    MatchTimes() = default;
    explicit MatchTimes(Node * node, unsigned int count);
    [[nodiscard]] unsigned int getCount() const noexcept;
    NFA_Automata * createAutomata() final;
    ~MatchTimes() override = default;
};

class Expression : public UnaryNode {
public:
    Expression() = default;
    explicit Expression(Node * node);
    NFA_Automata * createAutomata() final;
    ~Expression() override = default;
};

class BinaryNode : public Node, public DefineNode {
    Node * leftNode_ = nullptr;
    Node * rightNode_ = nullptr;
protected:
    BinaryNode(Node * leftNode, Node * rightNode);
    ~BinaryNode() noexcept override;
public:
    [[nodiscard]] Node * getLeft() noexcept;
    [[nodiscard]] Node *getRight() noexcept;
};

class OrNode : public BinaryNode {
public:
    OrNode(Node * leftNode, Node * rightNode);
    NFA_Automata * createAutomata() final;
    ~OrNode() override = default;
};

class AndNode : public BinaryNode {
public:
    AndNode(Node * leftNode, Node * rightNode);
    NFA_Automata * createAutomata() final;
    ~AndNode() override = default;
};

class SyntaxTree {
    Node * root_ = nullptr;
    void treewalkInternal(Node * node, int & height);
    //void paintGraph(Node *node, int & height);
public:
    SyntaxTree() = default;
    explicit SyntaxTree(Node * root);
    NFA_Automata * generateNFA();
    bool addRoot(Node * root);
    void treeWalk();
    ~SyntaxTree() noexcept;
};
/*
class CaptureGroupStorage {
    typedef std::map<std::string, CaptureGroup*> storage_type_;
    storage_type_ storage_;
public:
    bool insert(CaptureGroup * node, std::string group_name);
    [[nodiscard]] storage_type_ & getStorage() noexcept;
};
*/
class PatternString {
    std::list<Node*> str_;
public:
    explicit PatternString(std::string const& string);
    SyntaxTree generateSyntaxTree();
    SyntaxTree generateInverseSyntaxTree();
};

#endif //SYNTAXTREE_H