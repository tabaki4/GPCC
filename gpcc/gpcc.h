#pragma once
#include <queue>    
#include <functional>
#include <random>
#include <memory>
#include <string>
#include <utility>
#include "logic/logic.h"
#include "dist.h"
#include "simulation.h"

using namespace std;

typedef function<void()> action;

class Simulation::Block {
protected:
    Simulation& sim;
    Block* next;

    friend class SimBuilder;
public:
    virtual Block* advance(Transaction&) = 0;
    Block(Simulation& s, Block* next);

    #ifndef NDEBUG
    virtual string name() = 0;
    #endif
};

class Simulation::QueueBlock: public Block {
private:
    size_t q_index;
public:
    QueueBlock(Simulation& s, Block* next, size_t q_index);
    virtual Block* advance(Transaction&) override;
    virtual ~QueueBlock() {};

    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::DepartBlock: public Block {
private:
    size_t q_index;
public:
    DepartBlock(Simulation& s, Block* next, size_t q_index);
    virtual Block* advance(Transaction&) override;
    virtual ~DepartBlock() {};

    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::EnterBlock: public Block {
private:
    size_t storage_index;
public:
    EnterBlock(Simulation& s, Block* next, size_t storage_index);
    virtual Block* advance(Transaction&) override;
    virtual ~EnterBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::LeaveBlock: public Block {
private:
    size_t storage_index;
public:
    LeaveBlock(Simulation& s, Block* next, size_t storage_index);
    virtual Block* advance(Transaction&) override;
    virtual ~LeaveBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::GenBlock: public Block {
private:
    priority_t priority;
    RandomGenerator rng;

public:
    GenBlock(Simulation& s, Block* next, priority_t priority, RandomGenerator rng);
    virtual Block* advance(Transaction&) override;
    virtual ~GenBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::AdvanceBlock: public Block {
private:
    RandomGenerator rng;
public:
    AdvanceBlock(Simulation& s, Block* next, RandomGenerator rng);
    virtual Block* advance(Transaction&) override;
    virtual ~AdvanceBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::GateBlock: public Block {
private:
    priority_queue<Transaction> q;
    LogicNode expr;
public:
    GateBlock(Simulation& s, Block* next, LogicNode expr);
    bool refresh();
    virtual Block* advance(Transaction&) override;
    virtual ~GateBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

// index is used instead of Block* because at the point of usage needed label may be not declared.
// All labels are resolved in the end of build. If used label is not resolved builder will throw an exception. No need to check at runtime
class Simulation::TransferBlock_imm: public Block {
private:
    size_t index;
public:
    TransferBlock_imm(Simulation& s, Block* next, size_t index);
    Block* advance(Transaction&) override;
    virtual ~TransferBlock_imm() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::TransferBlock_expr: public Block {
private:
    size_t alt_index;
    LogicNode expr;
public:
    TransferBlock_expr(Simulation& s, Block* next, size_t alt_index, LogicNode expr);
    Block* advance(Transaction&) override;
    virtual ~TransferBlock_expr() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::TransferBlock_prob: public Block {
private:
    size_t alt_index;
    double prob;
    mt19937 gen;
    static uniform_real_distribution<> dist; // by default returns value in [0; 1)
public:
    TransferBlock_prob(Simulation& s, Block* next, size_t alt_index, double prob, int seed);
    Block* advance(Transaction&) override;
    virtual ~TransferBlock_prob() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::DebugBlock: public Block {
private:
    std::string debug_message;
public:
    virtual Block* advance(Transaction&) override;
    DebugBlock(Simulation& s, Block* next, const string& debug_message);
    virtual ~DebugBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};


class Simulation::TerminateBlock: public Block {
public:
    virtual Block* advance(Transaction&) override;
    TerminateBlock(Simulation& sim);
    virtual ~TerminateBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::Storage {
private:
    Simulation& sim;
    const size_t capacity;
    priority_queue<SpawnData> q;
    size_t current;

public:
    Storage(Simulation& s, size_t capacity = 0);

    bool empty();
    bool available();
    bool full();
    size_t get_current();
    size_t get_capacity();

    bool enter(const Transaction& transaction, EnterBlock* ret); // true: some op accepted priority_t; false: there are no free ops
    void leave();
};