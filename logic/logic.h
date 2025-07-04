#pragma once
#include <cstdint>
#include <variant>
#include <memory>
#include <functional>

using namespace std;

class LogicNode {
private:
    struct LogicNodeData;
    unique_ptr<LogicNodeData> data;

public:
    enum logic_op: int {PAR, NOT, AND, OR, VAL, EVAL};
    using vec_t = vector<LogicNode>;
    using func_t = function<bool()>;
    using content_t = variant<
        vec_t, 
        LogicNode, 
        func_t,
        bool
    >;
    LogicNode(bool val); // VAL
    LogicNode(func_t func); // EVAL
    LogicNode(); // deafult
    LogicNode(LogicNode&& rhs); // move unique_ptr
    LogicNode(const LogicNode& rhs) = delete; // TODO: implement deep copy;
    
    LogicNode& operator=(LogicNode&& rhs); // move unique_ptr

    LogicNode operator!() { return make_not(move(*this)); }
    LogicNode operator&(LogicNode rhs) { return make_and(move(*this), move(rhs)); }
    LogicNode operator|(LogicNode rhs) { return make_or(move(*this), move(rhs)); }

    void operator&=(LogicNode rhs) { *this = *this & move(rhs); }
    void operator|=(LogicNode rhs) { *this = *this | move(rhs); }

    bool eval();

    #ifndef NDEBUG
    logic_op get_type() const;
    const content_t& get_content() const;
    #endif

    LogicNode(logic_op op, LogicNode rhs);  // PAR NOT

    static LogicNode make_par(LogicNode rhs);
    static LogicNode make_not(LogicNode rhs);
    static LogicNode make_and(LogicNode l, LogicNode r);
    static LogicNode make_or(LogicNode l, LogicNode r);

    ~LogicNode();
};

struct LogicNode::LogicNodeData { 
    using logic_op = LogicNode::logic_op;
    using vec_t = LogicNode::vec_t;
    using func_t = LogicNode::func_t;
    using content_t = LogicNode::content_t;

    content_t content;
    logic_op type;
};