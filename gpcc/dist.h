#pragma once
#include <random>
#include <memory>

using namespace std;

class distribution {
public:
    virtual double operator()(minstd_rand&) { return 42; }
};
class exponential_distribution_wrapper: public exponential_distribution<>, public distribution {
public:
    using exponential_distribution::exponential_distribution; // expose needed constructors
    virtual double operator()(minstd_rand& engine) override { return exponential_distribution::operator()(engine); }
};
// class normal_distribution_wrapper: public normal_distribution<>, distribution {};
// and so on

class RandomGenerator {
private:
    minstd_rand engine;
    shared_ptr<distribution> dist; // smart ptr to avoid slicing
public:
    RandomGenerator(const minstd_rand& engine, shared_ptr<distribution> dist): engine(engine), dist(dist) {}
    double operator()() { return (*dist)(engine); }
};