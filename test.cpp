#include "gpcc/gpcc.h"
#include "sim_builder/builder.h"

int main() {
    auto builder = SimBuilder(2000);
    auto s = builder
    .add_storage("rab1", 5)
    .add_storage("rab2", 5)

    .add_generate(RandomGenerator(minstd_rand(1), make_unique<exponential_distribution_wrapper>(5)), 1)
    .add_queue("qrab1")
    .add_enter("rab1")
    .add_depart("qrab1")
    .add_advance(RandomGenerator(minstd_rand(2), make_unique<exponential_distribution_wrapper>(22)))
    .add_leave("rab1")
    .add_terminate()

    .add_generate(RandomGenerator(minstd_rand(3), make_unique<exponential_distribution_wrapper>(9)), 1)
    .add_queue("qrab2")
    .add_enter("rab2")
    .add_depart("qrab2")
    .add_advance(RandomGenerator(minstd_rand(4), make_unique<exponential_distribution_wrapper>(19)))
    .add_leave("rab2")
    .add_terminate()

    .add_generate(RandomGenerator(minstd_rand(5), make_unique<exponential_distribution_wrapper>(9)), 1)
    .add_queue("qrab3")
    .add_gate(LogicNode(builder.is_storage_avail("rab1")) | builder.is_storage_avail("rab2"))
    .add_transfer_expr("both_avail", LogicNode(builder.is_storage_avail("rab1")) & builder.is_storage_avail("rab2"))
    .add_transfer_expr("enter_r1", builder.is_storage_avail("rab1"))
    .add_transfer_imm("enter_r2")

    .add_transfer_prob("enter_r1", 0.5, 8)
    .add_label("both_avail")
    .add_transfer_imm("enter_r2")

    .add_enter("rab1")
    .add_label("enter_r1")
    .add_depart("qrab3")
    .add_advance(RandomGenerator(minstd_rand(6), make_unique<exponential_distribution_wrapper>(36)))
    .add_leave("rab1")
    .add_terminate()

    .add_enter("rab2")
    .add_label("enter_r2")
    .add_depart("qrab3")
    .add_advance(RandomGenerator(minstd_rand(7), make_unique<exponential_distribution_wrapper>(35)))
    .add_leave("rab2")
    .add_terminate().build();
    s->launch();
}