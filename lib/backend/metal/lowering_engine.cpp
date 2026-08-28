#include "tame/backend/metal/lowering_engine.h"

using namespace tame::ast;
using namespace tame::frontend;
using namespace tame::backend;

void LoweringEngine::process(std::vector<std::unique_ptr<Stmt>> &statements) {
    for (auto &statement : statements) {
        lower_statement(statement);
    }
}

void LoweringEngine::lower_statement(const std::unique_ptr<Stmt> &statement) {
    if (auto *variable_statement = dynamic_cast<VarStmt *>(statement.get())) {
        if (variable_statement->initializer) {
            variable_statement->initializer = lower_expression(std::move(variable_statement->initializer));
        }
    } else if (auto *expression_statement = dynamic_cast<ExprStmt *>(statement.get())) {
        expression_statement->expression = lower_expression(std::move(expression_statement->expression));
    } else if (auto *print_statement = dynamic_cast<PrintStmt *>(statement.get())) {
        print_statement->expression = lower_expression(std::move(print_statement->expression));
    }
}

std::unique_ptr<Expr> LoweringEngine::lower_expression(std::unique_ptr<Expr> expression) {
    if (!expression) {
        return nullptr;
    }

    if (auto *assign_expression = dynamic_cast<AssignExpr *>(expression.get())) {
        assign_expression->rhs = lower_expression(std::move(assign_expression->rhs));
        return expression;
    }

    if (auto *binary_expression = dynamic_cast<BinaryExpr *>(expression.get())) {
        if (binary_expression->operator_token.type == TokenType::STAR_TOKEN) {
            binary_expression->lhs = lower_expression(std::move(binary_expression->lhs));
            binary_expression->rhs = lower_expression(std::move(binary_expression->rhs));
            return expression;
        }

        MSLCodeGenerator code_generator;
        auto msl_kernel = code_generator.generate(std::move(expression));

        for (auto &value : msl_kernel.values) {
            value = lower_expression(std::move(value));
        }

        return std::make_unique<GPULaunchExpr>(msl_kernel.source, std::move(msl_kernel.values));
    }

    return expression;
}
