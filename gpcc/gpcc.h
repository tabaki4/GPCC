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
    shared_ptr<Block> next;

    friend class SimBuilder;
public:
    virtual shared_ptr<Block> advance(priority_t) = 0;
    Block(Simulation& s, shared_ptr<Block> next);

    #ifndef NDEBUG
    virtual string name() = 0;
    #endif
};

class Simulation::QueueBlock: public Block {
private:
    size_t q_index;
public:
    QueueBlock(Simulation& s, shared_ptr<Block> next, size_t q_index);
    virtual shared_ptr<Block> advance(priority_t = 0) override;
    virtual ~QueueBlock() {};

    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::DepartBlock: public Block {
private:
    size_t q_index;
public:
    DepartBlock(Simulation& s, shared_ptr<Block> next, size_t q_index);
    virtual shared_ptr<Block> advance(priority_t = 0) override;
    virtual ~DepartBlock() {};

    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::EnterBlock: public Block, public enable_shared_from_this<EnterBlock> {
private:
    size_t storage_index;
public:
    EnterBlock(Simulation& s, shared_ptr<Block> next, size_t storage_index);
    virtual shared_ptr<Block> advance(priority_t = 0) override;
    virtual ~EnterBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::LeaveBlock: public Block {
private:
    size_t storage_index;
public:
    LeaveBlock(Simulation& s, shared_ptr<Block> next, size_t storage_index);
    virtual shared_ptr<Block> advance(priority_t = 0) override;
    virtual ~LeaveBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::GenBlock: public Block {
public:
    GenBlock(Simulation& s, shared_ptr<Block> next, size_t priority);
    virtual shared_ptr<Block> advance(priority_t = 0) override;
    virtual ~GenBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::AdvanceBlock: public Block {
private:
    RandomGenerator gen;
public:
    AdvanceBlock(Simulation& s, shared_ptr<Block> next, RandomGenerator gen);
    virtual shared_ptr<Block> advance(priority_t pr) override;
    virtual ~AdvanceBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::GateBlock: public Block {
private:
    priority_queue<priority_t> q;
    LogicNode expr;
public:
    GateBlock(Simulation& s, shared_ptr<Block> next, LogicNode expr);
    bool refresh();
    virtual shared_ptr<Block> advance(priority_t pr) override;
    virtual ~GateBlock() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::TransferBlock_imm: public Block {
private:
    size_t index;
public:
    TransferBlock_imm(Simulation& s, shared_ptr<Block> next, size_t index);
    shared_ptr<Block> advance(priority_t = 0) override;
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
    TransferBlock_expr(Simulation& s, shared_ptr<Block> next, size_t alt_index, LogicNode expr);
    shared_ptr<Block> advance(priority_t = 0) override;
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
    TransferBlock_prob(Simulation& s, shared_ptr<Block> next, size_t alt_index, double prob, int seed);
    shared_ptr<Block> advance(priority_t = 0) override;
    virtual ~TransferBlock_prob() {};
        
    #ifndef NDEBUG
    virtual string name() override;
    #endif
};

class Simulation::TerminateBlock: public Block {
public:
    virtual shared_ptr<Block> advance(priority_t = 0) override;
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

    bool enter(priority_t pr, shared_ptr<EnterBlock> ptr); // true: some op accepted priority_t; false: there are no free ops
    void leave();
};