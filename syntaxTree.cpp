#include "syntaxTree.h"
#include <iostream>
#include <stack>

#include "HiearchyOperations.h"
#include "DFA.h"
#include "LangOperations.h"

#define UNCORRECTED false
#define CORRECT true

// UndefinedNode

UndefinedNode::UndefinedNode() : s_(0) {}

UndefinedNode::UndefinedNode(char sym) : s_(sym) {}

char UndefinedNode::getSymbol() const { return s_; }

// Node

Node::Node(char sym) { s_ = sym; test_name_ = "Node"; }

bool Node::addParent(Node *node) {
    if (node) { parent_ = node; return CORRECT; }
    return UNCORRECTED;
}

const Node *Node::parent() { return parent_; }

// SymbolNode

SymbolNode::SymbolNode(char sym) { s_ = sym; test_name_ = "SymNode"; }

// CaptureGroup

CaptureGroupNode::CaptureGroupNode(std::string name): name_(std::move(name)) { test_name_ = "CaptGroupNode"; }

std::string CaptureGroupNode::getName() { return name_; }

CaptureGroup::CaptureGroup(std::string name) : CaptureGroupNode(std::move(name)) { test_name_ = "CaptGroup"; }
// UnaryNode

UnaryNode::UnaryNode(Node * node) {
    if(node) { node_ = node; s_ = node->getSymbol(); }
    else throw std::logic_error("Wrong: nullptr node");
}

bool UnaryNode::addNode(Node * node) {
    if(node && !node_) { node_ = node; return CORRECT; }
    if(node) return UNCORRECTED;
    throw std::logic_error("Wrong: nullptr node");
}

UnaryNode::~UnaryNode() noexcept { delete node_; }

Node * UnaryNode::getNode() { return node_; }

// KleenyStar

KleenyStar::KleenyStar(Node *node) : UnaryNode(node) { test_name_ = "Kleeny"; }

// Optional

Optional::Optional(Node *node) : UnaryNode(node) { test_name_ = "Optional"; }

// MatchTimes

MatchTimes::MatchTimes(Node *node, unsigned int count) : UnaryNode(node), count_(count) { test_name_ = "RepeatNode: " + std::to_string(count); }

unsigned int MatchTimes::getCount() const noexcept { return count_; }

// Expression

Expression::Expression(Node *node) : UnaryNode(node) { test_name_ = "Expr"; }

// BinaryNode

BinaryNode::BinaryNode(Node *leftNode, Node *rightNode) {
    if(!rightNode || !leftNode) throw std::logic_error("Wrong: leftNode or rightNode is nullptr");
    rightNode_ = rightNode;
    leftNode_ = leftNode;
}

BinaryNode::~BinaryNode() noexcept { delete leftNode_; delete rightNode_; }

Node * BinaryNode::getLeft() noexcept { return leftNode_; }

Node * BinaryNode::getRight() noexcept { return rightNode_; }

// OrNode

OrNode::OrNode(Node * leftNode, Node * rightNode) : BinaryNode(leftNode, rightNode) { test_name_ = "OR"; }

// AndNode

AndNode::AndNode(Node * leftNode, Node * rightNode) : BinaryNode(leftNode, rightNode) { test_name_ = "AND"; }

// SyntaxTree

SyntaxTree::SyntaxTree(Node *root) {
    if(root) root_ = root;
    else throw std::logic_error("Wrong: nullptr node");
}

bool SyntaxTree::addRoot(Node *root) {
    if(root && !root_) { root_ = root; return CORRECT; }
    if(root) { return UNCORRECTED; }
    throw std::logic_error("Wrong: nullptr root-node");
}

SyntaxTree::~SyntaxTree() noexcept { delete root_; }
/*
//CaptureGroupStorage

bool CaptureGroupStorage::insert(CaptureGroup *node, std::string group_name) {
    if(storage_.count(group_name)) return UNCORRECTED;
    storage_[std::move(group_name)] = node;
    return CORRECT;
}

CaptureGroupStorage::storage_type_ &CaptureGroupStorage::getStorage() noexcept {
    return storage_;
}
*/
// bracketsData

struct bracketsData {
    std::list<Node*>::iterator start_;
    std::list<Node*>::iterator end_;
};

bracketsData minimalBrackets(std::list<Node*>::iterator const& start, std::list<Node*>::iterator const& end) {
    std::stack<char> stack_;
    std::list<Node*>::iterator startBrackets = start;
    bool hasBrackets = false;
    for (auto i = start; i != end; i++) {
        if(dynamic_cast<Node*>(*i)) {
            char sym = (*i)->getSymbol();
            if(!hasBrackets && sym == ')') { throw std::logic_error("First bracket is closing bracket"); }
            if((*i)->getSymbol() == '(') { startBrackets = i; hasBrackets = true; }
            if((*i)->getSymbol() == ')') { return bracketsData { startBrackets, i}; }
        }
    }
    throw std::logic_error("Brackets is not founded");
}

// Processing



PatternString::PatternString(std::string const& string) {
    for (auto & i: string) {
        auto node = new Node(i);
        str_.push_back(node);
    }

    str_.push_front(new Node('('));
    str_.push_back(new Node(')'));
}

SyntaxTree PatternString::generateSyntaxTree() {
    bracketsData brackets;
    DirectStages stage = DirectStages(str_, str_.begin(), str_.end(), stage_options::primary);
    stage.transform();
    brackets = minimalBrackets(str_.begin(), str_.end());
    while (brackets.start_ != str_.begin()) {
        stage = DirectStages(str_, brackets.start_, brackets.end_, stage_options::ordinary);
        stage.transform();

        stage = DirectStages(str_, --brackets.start_, brackets.end_, stage_options::brackets);
        stage.transform();
        brackets = minimalBrackets(str_.begin(), str_.end());
    };

    stage = DirectStages(str_, brackets.start_, brackets.end_, stage_options::ordinary_without_captureGroups);
    stage.transform();

    stage = DirectStages(str_, --brackets.start_, brackets.end_, stage_options::brackets);
    stage.transform();

    return SyntaxTree(*str_.begin());
}

SyntaxTree PatternString::generateInverseSyntaxTree() {
    bracketsData brackets;
    DirectStages stage = DirectStages(str_, str_.begin(), str_.end(), stage_options::primary);
    stage.transform();
    brackets = minimalBrackets(str_.begin(), str_.end());
    while (brackets.start_ != str_.begin()) {
        stage = DirectStages(str_, brackets.start_, brackets.end_, stage_options::ordinary_inverse);
        stage.transform();

        stage = DirectStages(str_, --brackets.start_, brackets.end_, stage_options::brackets);
        stage.transform();
        brackets = minimalBrackets(str_.begin(), str_.end());
    };

    stage = DirectStages(str_, brackets.start_, brackets.end_, stage_options::ordinary_inverse);
    stage.transform();

    stage = DirectStages(str_, --brackets.start_, brackets.end_, stage_options::brackets);
    stage.transform();

    return SyntaxTree(*str_.begin());
}

NFA_Automata * SyntaxTree::generateNFA() {
    return root_->createAutomata();
}

void printSpaces(int count) {
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < 3; ++j) {
            std::cout << " ";
        }
    }
}

void SyntaxTree::treewalkInternal(Node *node, int & height) {
    if(compaireNode<UnaryNode>(node)) {
        auto unary = dynamic_cast<UnaryNode*>(node);
        ++height;
        printSpaces(height);
        std::cout << unary->getTestingName() << "\\" << std::endl;
        treewalkInternal(unary->getNode(), height);
        --height;
    } else if(compaireNode<BinaryNode>(node)) {
        auto binary = dynamic_cast<BinaryNode*>(node);
        ++height;
        treewalkInternal(binary->getLeft(), height);
        printSpaces(height);
        std::cout << binary->getTestingName() << std::endl;
        treewalkInternal(binary->getRight(), height);
        --height;
    } else if(compaireNode<SymbolNode>(node)) {
        ++height;
        printSpaces(height);
        std::cout << "sym: '" << node->getSymbol() << "'" << std::endl;
        --height;
    }
}

void SyntaxTree::treeWalk() {
    int height = 0;
    treewalkInternal(root_, height);
}