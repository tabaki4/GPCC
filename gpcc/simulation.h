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

        NamedVar(const string& name, T& data): name(name), data(data) {}
        NamedVar(const string& name, T&& data): name(name), data(data) {}
    };

    struct SpawnData {
        priority_t pr;
        shared_ptr<Block> block;

        SpawnData(priority_t pr, shared_ptr<Block> block);
        bool operator<(const SpawnData& rhs) const;
    };

    struct TimedSpawn {
        SpawnData data;
        double time;

        TimedSpawn(SpawnData& data, double time);
        TimedSpawn(priority_t pr, shared_ptr<Block> block, double time);
        bool operator<(const TimedSpawn& rhs) const;
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
    class TerminateBlock;
    class Storage;

    double g_time = 0;
    double end_time;
    vector<NamedVar<shared_ptr<Block>>> labels;
    vector<NamedVar<size_t>> queues;
    vector<NamedVar<shared_ptr<Storage>>> storages;
    vector<shared_ptr<GateBlock>> gates;
    priority_queue<TimedSpawn> spawn_schedule;
    queue<SpawnData> priority_spawn_schedule;

    void serve(SpawnData& data);
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

    Simulation(Simulation&) = delete;
    Simulation();
    ~Simulation();
};