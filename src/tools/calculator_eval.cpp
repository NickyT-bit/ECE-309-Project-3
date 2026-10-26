
#include "tools/calculator.h"

#include <stack>
#include <string>
#include <vector>

using Kind = CalcToken::Kind;

// Determine operator precedence.
static int precedence(Kind op) {
    if (op == Kind::Times || op == Kind::Divide) {
        return 2;
    }

    if (op == Kind::Plus || op == Kind::Minus) {
        return 1;
    }

    return 0;
}

// Apply the operator at the top of the stack.
static bool apply_operator(std::stack<double>& values,
                           std::stack<Kind>& ops,
                           std::string* error) {

    if (ops.empty() || values.size() < 2) {
        *error = "invalid expression";
        return false;
    }

    Kind op = ops.top();
    ops.pop();

    double right = values.top();
    values.pop();

    double left = values.top();
    values.pop();

    double result = 0.0;

    switch (op) {
        case Kind::Plus:
            result = left + right;
            break;

        case Kind::Minus:
            result = left - right;
            break;

        case Kind::Times:
            result = left * right;
            break;

        case Kind::Divide:
            if (right == 0.0) {
                *error = "division by zero";
                return false;
            }
            result = left / right;
            break;

        default:
            *error = "invalid operator";
            return false;
    }

    values.push(result);
    return true;
}

// Evaluate the tokenized expression.
bool calc_evaluate(const std::vector<CalcToken>& tokens,
                   double* out,
                   std::string* error) {

    std::stack<double> values;
    std::stack<Kind> ops;

    for (const CalcToken& token : tokens) {
        Kind kind = token.kind;

        if (kind == Kind::Number) {
            values.push(token.value);
        }
        else if (kind == Kind::LParen) {
            ops.push(kind);
        }
        else if (kind == Kind::RParen) {
            while (!ops.empty() && ops.top() != Kind::LParen) {
                if (!apply_operator(values, ops, error)) {
                    return false;
                }
            }

            if (ops.empty()) {
                *error = "mismatched parentheses";
                return false;
            }

            ops.pop();
        }
        else {
            while (!ops.empty() &&
                   ops.top() != Kind::LParen &&
                   precedence(ops.top()) >= precedence(kind)) {

                if (!apply_operator(values, ops, error)) {
                    return false;
                }
            }

            ops.push(kind);
        }
    }

    // Apply remaining operators.
    while (!ops.empty()) {
        if (ops.top() == Kind::LParen) {
            *error = "mismatched parentheses";
            return false;
        }

        if (!apply_operator(values, ops, error)) {
            return false;
        }
    }

    if (values.size() != 1) {
        *error = "invalid expression";
        return false;
    }

    *out = values.top();
    return true;
}
