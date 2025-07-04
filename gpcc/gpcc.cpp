#include <queue>
#include <functional> 
#include <memory>
#include <iostream>
#include "gpcc.h"

using namespace std;

uniform_real_distribution<> Simulation::TransferBlock_prob::dist = uniform_real_distribution();

Simulation::Block::Block(Simulation& s, Simulation::Block* next): sim(s), next(next) {}
Simulation::QueueBlock::QueueBlock(Simulation& s, Block* next, size_t q_index): Block(s, next), q_index(q_index) {}
Simulation::DepartBlock::DepartBlock(Simulation& s, Block* next, size_t q_index): Block(s, next), q_index(q_index) {}
Simulation::EnterBlock::EnterBlock(Simulation& s, Block* next, size_t storage_index): Block(s, next), storage_index(storage_index) {}
Simulation::LeaveBlock::LeaveBlock(Simulation& s, Block* next, size_t storage_index): Block(s, next), storage_index(storage_index) {}
Simulation::GenBlock::GenBlock(Simulation& s, Block* next, priority_t priority, RandomGenerator rng): Block(s, next), rng(rng) {}
Simulation::AdvanceBlock::AdvanceBlock(Simulation& s, Block* next, RandomGenerator rng): Block(s, next), rng(rng) {}
Simulation::GateBlock::GateBlock(Simulation& s, Block* next, LogicNode expr): Block(s, next), expr(move(expr)) {}
Simulation::TransferBlock_imm::TransferBlock_imm(Simulation& s, Block* next, size_t index): Block(s, next), index(index) {}
Simulation::TransferBlock_expr::TransferBlock_expr(Simulation& s, Block* next, size_t alt_index, LogicNode expr): Block(s, next), alt_index(alt_index), expr(move(expr)) {}
Simulation::TransferBlock_prob::TransferBlock_prob(Simulation& s, Block* next, size_t alt_index, double prob, int seed): Block(s, next), alt_index(alt_index), prob(prob) { if (seed) gen.seed(seed); }
Simulation::DebugBlock::DebugBlock(Simulation& s, Block* next, const string& debug_message): Block(s, next), debug_message(debug_message) {}
Simulation::TerminateBlock::TerminateBlock(Simulation& sim): Block(sim, nullptr) {}

/*
Simulation::Block::~Block() {}
Simulation::QueueBlock::~QueueBlock() {}
Simulation::DepartBlock::~DepartBlock() {}
Simulation::EnterBlock::~EnterBlock() {}
Simulation::LeaveBlock::~LeaveBlock() {}
Simulation::GenBlock::~GenBlock() {}
Simulation::AdvanceBlock::~AdvanceBlock() {}
Simulation::GateBlock::~GateBlock() {}
Simulation::TransferBlock_imm::~TransferBlock_imm() {}
Simulation::TransferBlock_expr::~TransferBlock_expr() {}
Simulation::TransferBlock_prob::~TransferBlock_prob() {}
Simulation::TerminateBlock::~TerminateBlock() {}
*/

#ifndef NDEBUG
string Simulation::QueueBlock::name() { return "queue"; }
string Simulation::DepartBlock::name() { return "depart"; }
string Simulation::EnterBlock::name() { return "enter"; }
string Simulation::LeaveBlock::name() { return "leave"; }
string Simulation::GenBlock::name() { return "generate"; }
string Simulation::AdvanceBlock::name() { return "advance"; }
string Simulation::GateBlock::name() { return "gate"; }
string Simulation::TransferBlock_imm::name() { return "transfer_imm"; }
string Simulation::TransferBlock_expr::name() { return "transfer_expr"; }
string Simulation::TransferBlock_prob::name() { return "transfer_prob"; }
string Simulation::DebugBlock::name() { return "debug"; }
string Simulation::TerminateBlock::name() { return "terminate"; }
#endif

Simulation::Block* Simulation::QueueBlock::advance(Transaction&) {
    ++sim.queues[q_index].data;
    return next;
}

Simulation::Block* Simulation::DepartBlock::advance(Transaction&) {
    if (sim.queues[q_index].data == 0) throw SimulationException("Attempted to leave empty queue");
    --sim.queues[q_index].data;
    return next;
}

Simulation::Block* Simulation::EnterBlock::advance(Transaction& transaction) {
    if ( sim.storages[storage_index].data->enter(transaction, this)) return next;
    return nullptr; 
}

Simulation::Block* Simulation::LeaveBlock::advance(Transaction&) {
    sim.storages[storage_index].data->leave();
    return next;
}

Simulation::Block* Simulation::GenBlock::advance(Transaction& transaction) {
    if (transaction.just_generated) {
        Transaction tranasction(priority, sim.g_transaction_id++, true);
        sim.spawn_schedule.emplace(SpawnData(tranasction, this), sim.g_time + rng());
        transaction.just_generated = false;
    }
    return next;
}

Simulation::Block* Simulation::AdvanceBlock::advance(Transaction& transaction) {
    sim.spawn_schedule.emplace(SpawnData(transaction, next), sim.g_time + rng());
    return nullptr;
}

bool Simulation::GateBlock::refresh() {
    if (q.empty()) return false;
    if (expr.eval()) { sim.priority_spawn_schedule.emplace(q.top(), next); q.pop(); return true; }
    return false;
}

Simulation::Block* Simulation::GateBlock::advance(Transaction& transaction) {
    if ((q.empty() || (transaction.priority > q.top().priority)) && expr.eval()) return next; // avoid unnecessary death upon hitting open gate without queue
    q.push(transaction); 
    return nullptr; // check will be conducted in the end of tick
}

Simulation::Block* Simulation::TransferBlock_imm::advance(Transaction&) {
    return sim.labels[index].data;
}

Simulation::Block* Simulation::TransferBlock_expr::advance(Transaction&) {
    if (!expr.eval()) return next;
    return sim.labels[alt_index].data;
}

Simulation::Block* Simulation::TransferBlock_prob::advance(Transaction&) {
    if (dist(gen) < prob) return sim.labels[alt_index].data;
    return next;
}

Simulation::Block* Simulation::DebugBlock::advance(Transaction& transaction) {
    cout << "Transaction[" << transaction.id << "]: " << debug_message << '\n';
    return next;
}

Simulation::Block* Simulation::TerminateBlock::advance(Transaction&) { return nullptr; }

Simulation::Storage::Storage(Simulation& s, size_t capacity): sim(s), capacity(capacity) {}
bool Simulation::Storage::empty() { return current == 0; }
bool Simulation::Storage::available() { return current < capacity; }
bool Simulation::Storage::full() { return current == capacity; }
size_t Simulation::Storage::get_current() { return current; }
size_t Simulation::Storage::get_capacity() { return capacity; }

bool Simulation::Storage::enter(const Transaction& transaction, EnterBlock* ret) {
    if (available()) { ++current; return true; }
    q.emplace(transaction, ret);
    return false;
}

void Simulation::Storage::leave() {
    if (current == 0) throw SimulationException("Attempted to leave empty storage");
    if (!q.empty()) {
        SpawnData spawn_data = q.top();
        q.pop();
        sim.priority_spawn_schedule.emplace(spawn_data.transaction, spawn_data.block);
    }
    else --current;
}

Simulation::Simulation() {}
Simulation::~Simulation() {}