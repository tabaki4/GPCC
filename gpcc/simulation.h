#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <stdexcept>

using namespace std;

class SimulationException: public runtime_error {
public:
    SimulationException(const char* msg): runtime_error(msg) {}
    SimulationException(const string& msg): runtime_error(msg) {}
};

class Simulation {
public:
private:
    typedef unsigned long priority_t;

    uint64_t g_transaction_id = 0;

    class Block;

    struct  Stat {
        size_t max = 0;
        double m = 0;
        double empty = 0;
        double full = 0;
    };

    template <typename T>
    struct NamedVar {
        string name;
        T data;

        template <typename U>
        NamedVar(const string& name, U&& data): name(name), data(forward<U>(data)) {}
    };
    // small deduction guide hack
    template <typename U>
    NamedVar(const string&, U&&) -> NamedVar<decay_t<U>>;

    struct Transaction {
        priority_t priority;
        uint64_t id;
        bool just_generated;

        Transaction(priority_t priority, uint64_t id, bool just_generateed = false);
        bool operator<(const Transaction& rhs) const; // higher prioriry -> better
    };

    struct SpawnData {
        Block* block;
        Transaction transaction;

        SpawnData(const Transaction& transaction, Block* block);
        bool operator<(const SpawnData& rhs) const; // higher priority -> better
    };

    struct TimedSpawn {
        SpawnData spawn_data;
        double time;

        TimedSpawn(const SpawnData& data, double time);
        bool operator<(const TimedSpawn& rhs) const; // 1. lower time -> better; 2. higher priority -> better
    };

    class FallThroughBlock;
    class QueueBlock;
    class DepartBlock;
    class EnterBlock;
    class LeaveBlock;
    class GenBlock;
    class AdvanceBlock;
    class GateBlock;
    class TransferBlock_imm;
    class TransferBlock_expr;
    class TransferBlock_prob;
    class DebugBlock;
    class TerminateBlock;
    class Storage;

    double g_time = 0;
    double end_time;
    vector<unique_ptr<Block>> blocks;
    vector<NamedVar<Block*>> labels;
    vector<NamedVar<size_t>> queues;
    vector<NamedVar<unique_ptr<Storage>>> storages;
    vector<GateBlock*> gates;
    priority_queue<TimedSpawn> spawn_schedule; // spawn_shedule is the main schedule with time as priority parameter
    queue<SpawnData> priority_spawn_schedule;  // priority_spawn_schedule is a special queue that holds tranasctions that just became able to move after being suspended (e.g. gate, enter etc.)

    void serve(SpawnData& data); // serves a transaction until it dies
    bool refresh_gates();
    void report();

    void save_stat(double delta);
    void finalize_stat();

    struct Stat;

    vector<Stat> q_stat, storage_stat;

    friend class SimBuilder;

public:

    bool is_q_empty(size_t index);
    bool is_storage_empty(size_t index);
    bool is_storage_avail(size_t index);
    bool is_storage_full(size_t index);

    void launch();

    Simulation(Simulation&) = delete; // TODO: implement deep copy and move semantics for sim
    Simulation();
    ~Simulation();
};