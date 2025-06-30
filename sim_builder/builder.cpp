#include "builder.h"
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <format>

using namespace std;

class SimBuilderException: public runtime_error {
public:
    SimBuilderException(const char* msg): runtime_error(msg) {}
    SimBuilderException(const string& msg): runtime_error(msg) {}
};

SimBuilder& SimBuilder::add_label(const string& label) {
    if (label.empty()) throw SimBuilderException("empty string is not a valid label");
    if (label_map.contains(label) && sim->labels[label_map[label]].data != nullptr) throw SimBuilderException(format("redeclaration of label \"{}\"", label));
    if (hold == nullptr) throw SimBuilderException("label must be added after block. Labels to TERMINATE and TRANSFER(imm) are not allowed\n");
    
    if (!label_map.contains(label)) {
        label_map[label] = sim->labels.size();
        sim->labels.emplace_back(label, hold);
    }
    else sim->labels[label_map[label]].data = hold;

    return *this;
}

SimBuilder& SimBuilder::add_storage(const string& label, size_t capacity) {

    if (storage_map.contains(label)) throw SimBuilderException(format("redeclaration of storage \"{}\"", label));
    storage_map[label] = sim->storages.size();
    sim->storages.emplace_back(label, make_shared<Simulation::Storage>(*sim, capacity));
    return *this;
}

SimBuilder& SimBuilder::add_queue(const string& label) {
    if (!q_map.contains(label)) {
        q_map[label] = sim->queues.size();
        sim->queues.emplace_back(label, 0);
    }

    auto res = make_shared<Simulation::QueueBlock>(*sim, nullptr, q_map[label]);
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_depart(const string& label) {
    if (!q_map.contains(label)) throw SimBuilderException(format("depart from undeclared queue \"{}\"", label));
    
    auto res = make_shared<Simulation::DepartBlock>(*sim, nullptr, q_map[label]);
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_enter(const string& label) {
    if (!storage_map.contains(label)) throw SimBuilderException(format("enter to undeclared storage \"{}\"", label));

    auto res = make_shared<Simulation::EnterBlock>(*sim, nullptr, storage_map[label]);
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_leave(const string& label) {
    if (!storage_map.contains(label)) throw SimBuilderException(format("leave from undeclared storage \"{}\"", label));

    auto res = make_shared<Simulation::LeaveBlock>(*sim, nullptr, storage_map[label]);
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_generate(RandomGenerator gen, priority_t priority) {
    auto res = make_shared<Simulation::GenBlock>(*sim, nullptr, priority);
    if (hold != nullptr) hold->next = res;
    hold = res;

    double time = 0;
    while ( time += gen(), time < sim->end_time ) { // populate spawn schedule with generated transactions
        sim->spawn_schedule.emplace(priority, res, time);
    }

    return *this;
}

SimBuilder& SimBuilder::add_advance(RandomGenerator gen) {
    auto res = make_shared<Simulation::AdvanceBlock>(*sim, nullptr, gen);
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_gate(LogicNode expr) {
    auto res = make_shared<Simulation::GateBlock>(*sim, nullptr, move(expr));
    if (hold != nullptr) hold->next = res;
    hold = res;

    sim->gates.push_back(res);

    return *this;
}

SimBuilder& SimBuilder::add_transfer_expr(const string& alt_label, LogicNode expr) {
    if (!label_map.contains(alt_label)) {
        label_map[alt_label] = sim->labels.size();
        sim->labels.emplace_back(alt_label, nullptr);
    }

    auto res = make_shared<Simulation::TransferBlock_expr>(*sim, nullptr, label_map[alt_label], move(expr));
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_transfer_prob(const string& alt_label, double prob, int seed) {
    if (!label_map.contains(alt_label)) {
        label_map[alt_label] = sim->labels.size();
        sim->labels.emplace_back(alt_label, nullptr);
    }
    
    auto res = make_shared<Simulation::TransferBlock_prob>(*sim, nullptr, label_map[alt_label], prob, seed);
    if (hold != nullptr) hold->next = res;
    hold = res;

    return *this;
}

SimBuilder& SimBuilder::add_transfer_imm(const string& label) {
    if (!label_map.contains(label)) {
        label_map[label] = sim->labels.size();
        sim->labels.emplace_back(label, nullptr);
    }
    
    if (hold != nullptr) hold->next = make_shared<Simulation::TransferBlock_imm>(*sim, nullptr, label_map[label]);
    hold = nullptr;

    return *this;
}

SimBuilder& SimBuilder::add_terminate() {
    if (hold != nullptr) hold->next = make_shared<Simulation::TerminateBlock>(*sim);
    hold = nullptr;

    return *this;
}

LogicNode::func_t SimBuilder::is_q_empty(const string& label) {
    size_t index = q_map[label];
    return [this, index](){ return sim->is_q_empty(index); };
}

LogicNode::func_t SimBuilder::is_storage_empty(const string& label) {
    size_t index = storage_map[label];
    return [this, index](){ return sim->is_storage_empty(index); };
}

LogicNode::func_t SimBuilder::is_storage_avail(const string& label) {
    size_t index = storage_map[label];
    return [this, index](){ return sim->is_storage_avail(index); };
}
LogicNode::func_t SimBuilder::is_storage_full(const string& label) {
    size_t index = storage_map[label];
    return [this, index](){ return sim->is_storage_full(index); };
}

shared_ptr<Simulation> SimBuilder::build() {
    if (hold != nullptr) cerr << "Warning: transactions may fall out of bounds\n";
    for (auto& el : sim->labels) if (el.data == nullptr) throw SimBuilderException(format("Usage of undefined label \"{}\"", el.name));
    sim->q_stat.resize(sim->queues.size());
    sim->storage_stat.resize(sim->queues.size());

    return sim;
}

