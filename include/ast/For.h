#ifndef AST_OPTIMIZER_INCLUDE_AST_FOR_H_
#define AST_OPTIMIZER_INCLUDE_AST_FOR_H_

#include "AbstractStatement.h"
#include <string>

class For : public AbstractStatement {
 public:
  For(AbstractStatement *initializer,
      LogicalExpr *condition,
      AbstractStatement *update,
      AbstractStatement *statementToBeExecuted);

  [[nodiscard]] std::string getNodeType() const override;

  void accept(Visitor &v) override;

  AbstractNode *clone(bool keepOriginalUniqueNodeId) override;

  void setAttributes(AbstractStatement *initializer,
                     LogicalExpr *condition,
                     AbstractStatement *update,
                     AbstractStatement *statementToBeExecuted);

  [[nodiscard]] AbstractExpr *getInitializer() const;

  [[nodiscard]] LogicalExpr *getCondition() const;

  [[nodiscard]] AbstractExpr *getUpdateStatement() const;

  [[nodiscard]] AbstractStatement *getStatementToBeExecuted() const;

  int getMaxNumberChildren() override;

  [[nodiscard]] std::string toString(bool printChildren) const override;

  bool supportsCircuitMode() override;

  [[nodiscard]] json toJson() const override;

  bool isEqual(AbstractStatement *other) override;
};

#endif //AST_OPTIMIZER_INCLUDE_AST_FOR_H_