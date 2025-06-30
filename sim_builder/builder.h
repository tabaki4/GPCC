#pragma once
#include <string>
#include <stdexcept>
#include "gpcc/gpcc.h"

class SimBuilder {
private:
    shared_ptr<Simulation> sim;
    shared_ptr<Simulation::Block> hold;
    unordered_map<string, size_t> q_map;
    unordered_map<string, size_t> storage_map;
    unordered_map<string, size_t> label_map;

public:
    using priority_t = Simulation::priority_t;
    SimBuilder& add_label(const string& label);
    SimBuilder& add_storage(const string& label, size_t capacity);
    SimBuilder& add_queue(const string& label);
    SimBuilder& add_depart(const string& label);
    SimBuilder& add_enter(const string& label);
    SimBuilder& add_leave(const string& label);
    SimBuilder& add_generate(RandomGenerator gen, priority_t priority = 0);
    SimBuilder& add_advance(RandomGenerator gen);
    SimBuilder& add_gate(LogicNode expr);
    SimBuilder& add_transfer_expr(const string& alt_label, LogicNode expr);
    SimBuilder& add_transfer_prob(const string& alt_label, double prob, int seed = 1);
    SimBuilder& add_transfer_imm(const string& alt_label);
    SimBuilder& add_terminate();

    LogicNode::func_t is_q_empty(const string& label);
    LogicNode::func_t is_storage_empty(const string& label);
    LogicNode::func_t is_storage_avail(const string& label);
    LogicNode::func_t is_storage_full(const string& label);

    shared_ptr<Simulation> build();

    SimBuilder(double end_time): sim(make_shared<Simulation>()) { sim->end_time = end_time; }
};

class SimBuilderException;