#include <gtest/gtest.h>
#include <memory>
#include <functional>
#include "logic/logic.h"

using namespace std;

class LogicTest: public testing::Test {
protected:
    function<bool()> t_eval = [this](){ ++calls_to_eval; return true; };
    function<bool()> f_eval = [this](){ ++calls_to_eval; return false; };
    size_t calls_to_eval = 0;   
};

TEST_F(LogicTest, HandlesPar) {
    LogicNode expr(t_eval);

    // (x)
    expr = LogicNode::make_par(move(expr));
    EXPECT_EQ(expr.get_type(), LogicNode::PAR);
    EXPECT_NE(get_if<LogicNode>(&expr.get_content()), nullptr);

    // ((x)) = (x)
    expr = LogicNode::make_par(move(expr));
    EXPECT_EQ(expr.get_type(), LogicNode::PAR);
    EXPECT_NE(get_if<LogicNode>(&expr.get_content()), nullptr);
    EXPECT_EQ(get<LogicNode>(expr.get_content()).get_type(), LogicNode::EVAL);
    EXPECT_NE(get_if<LogicNode::func_t>(&get<LogicNode>(expr.get_content()).get_content()), nullptr);
};

TEST_F(LogicTest, HandlesNot) {
    LogicNode expr(t_eval);

    // !E
    expr = !expr;
    EXPECT_EQ(expr.get_type(), LogicNode::NOT);
    EXPECT_NE(get_if<LogicNode>(&expr.get_content()), nullptr);

    // !!E = E
    expr = !expr;
    EXPECT_EQ(expr.get_type(), LogicNode::EVAL);
    EXPECT_NE(get_if<LogicNode::func_t>(&expr.get_content()), nullptr);

    // !!!...!!E
    for (int i = 0; i < 101; ++i) expr = !expr;
    EXPECT_EQ(expr.get_type(), LogicNode::NOT);
    EXPECT_NE(get_if<LogicNode>(&expr.get_content()), nullptr);

    // !E(1) = E->0
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 1);

    // !!E(1) = E->1
    expr = !expr;
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 2);
};

TEST_F(LogicTest, HandlesAnd) {
    LogicNode expr;

    // 0 & E = 0
    expr = LogicNode(false) & t_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), false);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 0);

    // E & 0 = 0
    expr = LogicNode(t_eval) & false;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), false);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 0);

    // E(1) & E(1) = 2E->1
    expr = LogicNode(t_eval) & t_eval; 
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 2);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 2);

    // E(0) & E(1) = E->0
    calls_to_eval = 0;
    expr = LogicNode(f_eval) & t_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 2);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 1);

    // E(1) & E(0) = 2E->0
    calls_to_eval = 0;
    expr = LogicNode(t_eval) & f_eval; 
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 2);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 2);

    // E(1) & E(1) & E(1) = 3E->1
    calls_to_eval = 0;
    expr = LogicNode(t_eval) & t_eval & t_eval; 
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 3);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 3);

    // 0 & E(1) & E(1) & E(1) = 0
    calls_to_eval = 0;
    expr = LogicNode(false) & t_eval & t_eval & t_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), false);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 0);

    // E(1) & E(1) & E(1) & 0 = 0
    calls_to_eval = 0;
    expr = LogicNode(t_eval) & t_eval & t_eval & false;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), false);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 0);

    // 1 & E(1) & E(1) & E(1) = E(1) & E(1) & E(1)
    calls_to_eval = 0;
    expr = LogicNode(true) & t_eval & t_eval & t_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 3);

    // (E(1) & E(1) & E(1)) & 1 = E(1) & E(1) & E(1)
    calls_to_eval = 0;
    expr = LogicNode(t_eval) & t_eval & t_eval & true;
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 3);

    // (E(1) & E(1) & E(0)) & (E(1) & E(0) & E(1) & E(1)) = E(1) & E(1) & E(0) & E(1) & E(0) & E(1) & E(1) = 3E->0
    calls_to_eval = 0;
    expr = LogicNode(LogicNode(t_eval) & t_eval & f_eval) & LogicNode(LogicNode(t_eval) & f_eval & t_eval & t_eval);
    EXPECT_EQ(expr.get_type(), LogicNode::AND);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 7);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 3);
}

TEST_F(LogicTest, HandlesOr) {
    LogicNode expr;

    // 1 | E = 1
    expr = LogicNode(true) | f_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), true);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 0);

    // E | 1 = 1
    expr = LogicNode(f_eval) | true;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), true);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 0);

    // E(0) | E(0) = 2E->0
    expr = LogicNode(f_eval) | f_eval; 
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 2);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 2);

    // E(1) | E(0) = E->1
    calls_to_eval = 0;
    expr = LogicNode(t_eval) | f_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 2);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 1);

    // E(0) | E(1) = 2E->1
    calls_to_eval = 0;
    expr = LogicNode(f_eval) | t_eval; 
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 2);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 2);

    // E(0) | E(0) | E(0) = 3E->0
    calls_to_eval = 0;
    expr = LogicNode(f_eval) | f_eval | f_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 3);
    EXPECT_EQ(expr.eval(), false);
    EXPECT_EQ(calls_to_eval, 3);

    // 1 | E(0) | E(0) | E(0) = 1
    calls_to_eval = 0;
    expr = LogicNode(true) ;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), true);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 0);
 
    // E(0) | E(0) | E(0) | 1 = 1
    calls_to_eval = 0;
    expr = LogicNode(f_eval) | f_eval | f_eval | true;
    EXPECT_EQ(expr.get_type(), LogicNode::VAL);
    EXPECT_EQ(get<bool>(expr.get_content()), true);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 0);

    // 0 | E(0) | E(0) | E(0) = E(0) | E(0) | E(0)
    calls_to_eval = 0;
    expr = LogicNode(false) | f_eval | f_eval | f_eval;
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 3);

    // (E(0) | E(0) | E(0)) | 0 = E(0) | E(0) | E(0)
    calls_to_eval = 0;
    expr = LogicNode(f_eval) | f_eval | f_eval | false;
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 3);

    // (E(0) | E(0) | E(1)) | (E(0) | E(1) | E(0) | E(0)) = E(0) | E(0) | E(1) | E(0) | E(1) | E(0) | E(0) = 3E->1
    calls_to_eval = 0;
    expr = (LogicNode(f_eval) | f_eval | t_eval) | (LogicNode(f_eval) | t_eval | f_eval | f_eval);
    EXPECT_EQ(expr.get_type(), LogicNode::OR);
    EXPECT_EQ(get<LogicNode::vec_t>(expr.get_content()).size(), 7);
    EXPECT_EQ(expr.eval(), true);
    EXPECT_EQ(calls_to_eval, 3);
}

TEST_F(LogicTest, HandlesCombinedExpressions) {
    // !(!(...(!(!E(x) & E(1)) | E(0)) = E->x
    LogicNode x(t_eval);
    for (int i = 0; i < 121; ++i) x = !(!x & t_eval) | f_eval;
    EXPECT_EQ(x.eval(), true);
    EXPECT_EQ(calls_to_eval, 1);

    // may be add some more
}

