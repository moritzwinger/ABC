#ifndef AST_OPTIMIZER_TEST_VISITOR_BINARYTOOPERATOREXPRESSIONVISITOR_H_
#define AST_OPTIMIZER_TEST_VISITOR_BINARYTOOPERATOREXPRESSIONVISITOR_H_

#include "ast_opt/utilities/Visitor.h"
#include "ast_opt/ast/BinaryExpression.h"

class SpecialBinaryToOperatorExpressionVisitor;

typedef Visitor<SpecialBinaryToOperatorExpressionVisitor> BinaryToOperatorExpressionVisitor;

class SpecialBinaryToOperatorExpressionVisitor : public ScopedVisitor {

 private:
  std::unique_ptr<OperatorExpression> replacement_expr = nullptr;

 public:
  void visit(BinaryExpression& elem);
};

#endif //AST_OPTIMIZER_TEST_VISITOR_BINARYTOOPERATOREXPRESSIONVISITOR_H_
