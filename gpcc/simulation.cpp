#include "gpcc.h"
#include <iostream>
#include <iomanip>

Simulation::SpawnData::SpawnData(priority_t pr, Block* block): pr(pr), block(block) {}
bool Simulation::SpawnData::operator<(const SpawnData& rhs) const { return pr < rhs.pr; }

Simulation::TimedSpawn::TimedSpawn(SpawnData& data, double time): data(data), time(time) {}
Simulation::TimedSpawn::TimedSpawn(priority_t pr, Block* block, double time): data(pr, block), time(time) {}
bool Simulation::TimedSpawn::operator<(const TimedSpawn& rhs) const { return time > rhs.time; }


bool Simulation::is_q_empty(size_t index) { return queues[index].data == 0; }
bool Simulation::is_storage_empty(size_t index) { return storages[index].data->empty(); }
bool Simulation::is_storage_avail(size_t index) { return storages[index].data->available(); }
bool Simulation::is_storage_full(size_t index) { return storages[index].data->full(); }

void Simulation::serve(SpawnData& data) {
    Block* current = data.block;
    while (current != nullptr) { 
            
        #ifndef NDEBUG
        cout << "advancing from " << current->name() << '\n';
        #endif
        current = current->advance(data.pr);
    }

    #ifndef NDEBUG
    cout << "died\n";
    #endif
}

bool Simulation::refresh_gates() {
    #ifndef NDEBUG
    cout << "refreshing gates\n";
    #endif
    for (auto& gate : gates) if (gate->refresh()) return true;
    return false;
}

void Simulation::report() {
    cout << fixed << showpoint;
    cout << setprecision(4);

    cout << "QUEUES:\n";
    cout << "\tqueue\t\tCurrent\t\tMax\t\tM\t\tP(0)\n";
    for (size_t i = 0; i < queues.size(); ++i) cout << "\t" 
    << queues[i].name << "\t\t"
    << queues[i].data << "\t\t"
    << q_stat[i].max << "\t\t"
    << q_stat[i].m << "\t\t"
    << q_stat[i].empty << "\n";

    cout << "STORAGES:\n";
    cout << "\tstorage\t\tCap\t\tCurrent\t\tMax\t\tM\t\tK\t\tP(0)\t\tP(full)\n";
    for (size_t i = 0; i < storages.size(); ++i) cout << "\t" 
    << storages[i].name << "\t\t"
    << storages[i].data->get_capacity() << "\t\t"
    << storages[i].data->get_current() << "\t\t"
    << storage_stat[i].max << "\t\t"
    << storage_stat[i].m << "\t\t"
    << storage_stat[i].m / storages[i].data->get_capacity() << "\t\t"
    << storage_stat[i].empty << "\t\t"
    << storage_stat[i].full << '\n';
}

void Simulation::save_stat(double delta) {
    for (size_t i = 0; i < queues.size(); ++i) { // queues
        q_stat[i].max = max(q_stat[i].max, queues[i].data);
        q_stat[i].m += queues[i].data * delta;
        if (queues[i].data == 0) q_stat[i].empty += delta;
    }

    for (size_t i = 0; i < storages.size(); ++i) { // storages
        storage_stat[i].max = max(storage_stat[i].max, storages[i].data->get_current());
        storage_stat[i].m += storages[i].data->get_current() * delta;
        if (storages[i].data->empty()) storage_stat[i].empty += delta;
        else if (storages[i].data->full()) storage_stat[i].full += delta;
    }
}

void Simulation::finalize_stat() {
    for (size_t i = 0; i < queues.size(); ++i) { q_stat[i].m /= g_time; q_stat[i].empty /= g_time; }
    for (size_t i = 0; i < storages.size(); ++i) {
        storage_stat[i].m /= g_time;
        storage_stat[i].empty /= g_time;
        storage_stat[i].full /= g_time;
    }
}

void Simulation::launch() {
    while (g_time < end_time && !spawn_schedule.empty()) {
        #ifndef NDEBUG
        cout << "entering main section\n";
        #endif
        TimedSpawn spawn = spawn_schedule.top();
        spawn_schedule.pop();
        
        #ifndef NDEBUG
        cout << "advancing " << spawn.time - g_time << '\n';
        #endif

        save_stat(spawn.time - g_time);

        g_time = spawn.time;
        serve(spawn.data);

        #ifndef NDEBUG
        cout << "entering refresh section\n";
        #endif
        while (!priority_spawn_schedule.empty() || refresh_gates()) {
            SpawnData data = priority_spawn_schedule.front();
            priority_spawn_schedule.pop();
            serve(data);
        }

    }
    finalize_stat();
    report();
} 