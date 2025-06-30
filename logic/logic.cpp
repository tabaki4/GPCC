#include <concepts>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include "logic.h"

using namespace std;

template<integral T>
T ceil_exp2(T x) {
    --x; // to prevent changing already power of 2
    uint_fast8_t p = 1;
    for (; p < sizeof(x)*8; p*=2) x |= x >> p; // 101001 -> 111111
    ++x; // 1111 -> 10000
    return x;
}
    

template<typename T> void exp_reserve(vector<T> &vec, size_t size) {
    if (size <= vec.capacity());
    // size_t exp_size = ceil(log2(size)) // bit manipulation is faster
    vec.reserve(ceil_exp2(size));
}

LogicNode::LogicNode(logic_op op, LogicNode rhs) { // NOT
    data = make_unique<LogicNodeData>();
    if (op != NOT) cerr << "logic_op ins not NOT\n";
    if (rhs.data->type == VAL) { // !1 = 0; !0 = 1
        data->type = VAL;
        data->content = !get<bool>(rhs.data->content);
    }
    else if (rhs.data->type == NOT) { // !(!x) = x
        data = move(get<LogicNode>(rhs.data->content).data);
    }
    else data->content = move(rhs);
}

LogicNode::LogicNode(bool val) { 
    data = make_unique<LogicNodeData>();
    data->type = VAL; 
    data->content = val; 
} // VAL

LogicNode::LogicNode(func_t func) {
    data = make_unique<LogicNodeData>();
    data->type = EVAL;
    data->content = func; 
} // EVAL

LogicNode::LogicNode() {
    data = make_unique<LogicNodeData>();
    data->type = VAL;
    data->content = false;
} // default

LogicNode::LogicNode(LogicNode&& rhs) { // move unique_ptr
    data = move(rhs.data);
}

LogicNode& LogicNode::operator=(LogicNode&& rhs) { // move unique_ptr
    data = move(rhs.data);
    return *this;
}

bool LogicNode::eval() {
    switch(data->type) {
        case PAR: return get<LogicNode>(data->content).eval();
        case NOT: return !get<LogicNode>(data->content).eval();
        case AND: {
            for (int i = 0; i < get<vec_t>(data->content).size(); ++i)
                if (!get<vec_t>(data->content)[i].eval()) return false; // x & 0 & z & ... = 0
            return true;
        };
        case OR: {
            for (int i = 0; i < get<vec_t>(data->content).size(); ++i)
                if (get<vec_t>(data->content)[i].eval()) return true; // x | 1 | z & ... = 1
            return false;
        };
        case VAL: return get<bool>(data->content);
        case EVAL: return get<func_t>(data->content)();
        default: cerr << "bad logic_op value\n"; return false; // must be unreachable
    }
}

#ifndef NDEBUG
LogicNode::logic_op LogicNode::get_type() const { return data->type; }
const LogicNode::content_t& LogicNode::get_content() const { return data->content; }
#endif

LogicNode LogicNode::make_par(LogicNode rhs) {
    LogicNode res;

    if (rhs.data->type == PAR) res = move(rhs); // ((x)) = (x)
    else { res.data->type = PAR; res.data->content = move(rhs); }

    return res;
}

LogicNode LogicNode::make_not(LogicNode rhs) {
    LogicNode res;

    if (rhs.data->type == VAL) { // !1 = 0; !0 = 1
        res.data->type = VAL;
        res.data->content = !get<bool>(rhs.data->content);
    }

    else if (rhs.data->type == NOT) res = move(get<LogicNode>(rhs.data->content)); // !(!x) = x
    else { res.data->content = move(rhs); res.data->type = NOT; }

    return res;
}

LogicNode LogicNode::make_and(LogicNode l, LogicNode r) {
    LogicNode res;

    // check if we can simplify it into 1 operand [1 & x = x; 0 & x = 0]
    res.data->type = AND;
    if (l.data->type == VAL) {
        if (get<bool>(l.data->content) == false) { // 0 & x = 0
            res.data->type = VAL;
            res.data->content = false;
        }
        else res.data = move(r.data); // 1 & x = x
        return res;
    }
    if (r.data->type == VAL) {
        if (get<bool>(r.data->content) == false) { // 0 & x = 0
            res.data->type = VAL;
            res.data->content = false;
        }
        else res.data = move(l.data); // 1 & x = x
        return res;
    }

    // check if it is the same type as this node [(a & b) & c = a & b & c] // here () does not mean logic_op PAR
    if (l.data->type == AND) res = move(l);
    else { // short-circuitng is not applicable
        res.data->content = vec_t();
        get<vec_t>(res.data->content).push_back(move(l));
    }

    // by now it is guaranteed that content is vector
    if (r.data->type == AND) {
        exp_reserve(
            get<vec_t>(res.data->content),
            get<vec_t>(res.data->content).size() + get<vec_t>(r.data->content).size()
        );
        move(
            get<vec_t>(r.data->content).begin(),
            get<vec_t>(r.data->content).end(),
            back_inserter(get<vec_t>(res.data->content))
        ); // content += r->content
    }
    else get<vec_t>(res.data->content).push_back(move(r)); // short-circuitng is not applicable

    return res;
}

LogicNode LogicNode::make_or(LogicNode l, LogicNode r) {
    // check if we can simplify it into 1 operand [0 | x = x; 1 | x = 0]
    LogicNode res;
    res.data->type = OR;
    if (l.data->type == VAL) {
        if (get<bool>(l.data->content) == true) { // 1 | x = 0
            res.data->type = VAL;
            res.data->content = true;
        }
        else res.data = move(r.data); // 0 | x = x
        return res;
    }
    if (r.data->type == VAL) {
        if (get<bool>(r.data->content) == true) { // 1 | x = 0
            res.data->type = VAL;
            res.data->content = true;
        }
        else res.data = move(l.data); // 0 | x = x
        return res;
    }

    // check if it is the same type as this node [(a | b) | c = a | b | c] // here () does not mean logic_op PAR
    if (l.data->type == OR) res = move(l);
    else { // short-circuitng is not applicable
        res.data->content = vec_t();
        get<vec_t>(res.data->content).push_back(move(l));
    }

    // by now it is guaranteed that content is vector
    if (r.data->type == OR) {
        exp_reserve(
            get<vec_t>(res.data->content),
            get<vec_t>(res.data->content).size() + get<vec_t>(r.data->content).size()
        );
        move(
            get<vec_t>(r.data->content).begin(),
            get<vec_t>(r.data->content).end(),
            back_inserter(get<vec_t>(res.data->content))
        ); // content += r->content
    }
    else get<vec_t>(res.data->content).push_back(move(r)); // short-circuitng is not applicable

    return res;
}

LogicNode::~LogicNode() {}