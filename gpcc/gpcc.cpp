#include <queue>
#include <functional> 
#include <memory>
#include <iostream>
#include "gpcc.h"

using namespace std;

uniform_real_distribution<> Simulation::TransferBlock_prob::dist = uniform_real_distribution();

Simulation::Block::Block(Simulation& s, shared_ptr<Simulation::Block> next): sim(s), next(next) {}
Simulation::QueueBlock::QueueBlock(Simulation& s, shared_ptr<Block> next, size_t q_index): Block(s, next), q_index(q_index) {}
Simulation::DepartBlock::DepartBlock(Simulation& s, shared_ptr<Block> next, size_t q_index): Block(s, next), q_index(q_index) {}
Simulation::EnterBlock::EnterBlock(Simulation& s, shared_ptr<Block> next, size_t storage_index): Block(s, next), storage_index(storage_index) {}
Simulation::LeaveBlock::LeaveBlock(Simulation& s, shared_ptr<Block> next, size_t storage_index): Block(s, next), storage_index(storage_index) {}
Simulation::GenBlock::GenBlock(Simulation& s, shared_ptr<Block> next, size_t priority): Block(s, next) {}
Simulation::AdvanceBlock::AdvanceBlock(Simulation& s, shared_ptr<Block> next, RandomGenerator gen): Block(s, next), gen(gen) {}
Simulation::GateBlock::GateBlock(Simulation& s, shared_ptr<Block> next, LogicNode expr): Block(s, next), expr(move(expr)) {}
Simulation::TransferBlock_imm::TransferBlock_imm(Simulation& s, shared_ptr<Block> next, size_t index): Block(s, next), index(index) {}
Simulation::TransferBlock_expr::TransferBlock_expr(Simulation& s, shared_ptr<Block> next, size_t alt_index, LogicNode expr): Block(s, next), alt_index(alt_index), expr(move(expr)) {}
Simulation::TransferBlock_prob::TransferBlock_prob(Simulation& s, shared_ptr<Block> next, size_t alt_index, double prob, int seed): Block(s, next), alt_index(alt_index), prob(prob) { if (seed) gen.seed(seed); }
Simulation::TerminateBlock::TerminateBlock(Simulation& sim): Block(sim, nullptr) {}

/*Simulation::Block::~Block() {}
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
string Simulation::TerminateBlock::name() { return "terminate"; }
#endif

shared_ptr<Simulation::Block> Simulation::QueueBlock::advance(priority_t) {
    ++sim.queues[q_index].data;
    return next;
}

shared_ptr<Simulation::Block> Simulation::DepartBlock::advance(priority_t) {
    if (sim.queues[q_index].data == 0) throw SimulationException("Attempted to leave empty queue");
    --sim.queues[q_index].data;
    return next;
}

shared_ptr<Simulation::Block> Simulation::EnterBlock::advance(priority_t pr) {
    if ( sim.storages[storage_index].data->enter(pr, this->shared_from_this())) return next;
    return nullptr; 
}

shared_ptr<Simulation::Block> Simulation::LeaveBlock::advance(priority_t) {
    sim.storages[storage_index].data->leave();
    return next;
}

shared_ptr<Simulation::Block> Simulation::GenBlock::advance(priority_t) {
    return next;
}

shared_ptr<Simulation::Block> Simulation::AdvanceBlock::advance(priority_t pr) {
    sim.spawn_schedule.emplace(pr, next, sim.g_time + gen());
    return nullptr;
}

bool Simulation::GateBlock::refresh() {
    if (q.empty()) return false;
    if (expr.eval()) { sim.priority_spawn_schedule.emplace(q.top(), next); q.pop(); return true; }
    return false;
}

shared_ptr<Simulation::Block> Simulation::GateBlock::advance(priority_t pr) {
    if ((q.empty() || (pr > q.top())) && expr.eval()) return next;
    q.push(pr); return nullptr; // check will be conducted in the end of tick
}

shared_ptr<Simulation::Block> Simulation::TransferBlock_imm::advance(priority_t) {
    return sim.labels[index].data;
}

shared_ptr<Simulation::Block> Simulation::TransferBlock_expr::advance(priority_t) {
    if (!expr.eval()) return next;
    return sim.labels[alt_index].data;
}

shared_ptr<Simulation::Block> Simulation::TransferBlock_prob::advance(priority_t) {
    if (dist(gen) < prob) return sim.labels[alt_index].data;
    return next;
}

shared_ptr<Simulation::Block> Simulation::TerminateBlock::advance(priority_t) { return nullptr; }

Simulation::Storage::Storage(Simulation& s, size_t capacity): sim(s), capacity(capacity) {}
bool Simulation::Storage::empty() { return current == 0; }
bool Simulation::Storage::available() { return current < capacity; }
bool Simulation::Storage::full() { return current == capacity; }
size_t Simulation::Storage::get_current() { return current; }
size_t Simulation::Storage::get_capacity() { return capacity; }

bool Simulation::Storage::enter(priority_t pr, shared_ptr<EnterBlock> ret) {
    if (available()) { ++current; return true; }
    q.emplace(pr, ret);
    return false;
}

void Simulation::Storage::leave() {
    if (current == 0) throw SimulationException("Attempted to leave empty storage");
    if (!q.empty()) {
        SpawnData spawn_data = q.top();
        q.pop();

        sim.priority_spawn_schedule.emplace(spawn_data.pr, spawn_data.block);
    }
    else --current;
}

Simulation::Simulation() {}
Simulation::~Simulation() {}