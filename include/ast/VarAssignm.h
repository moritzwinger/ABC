#ifndef AST_OPTIMIZER_INCLUDE_VARASSIGNM_H
#define AST_OPTIMIZER_INCLUDE_VARASSIGNM_H

#include <string>
#include "AbstractStatement.h"
#include "AbstractExpr.h"

class VarAssignm : public AbstractStatement {
 private:
  std::string identifier;

 public:
  VarAssignm(std::string identifier, AbstractExpr *value);

  ~VarAssignm() override;

  VarAssignm *clone(bool keepOriginalUniqueNodeId) override;

  [[nodiscard]] json toJson() const override;

  void accept(Visitor &v) override;

  [[nodiscard]] const std::string &getIdentifier() const;

  [[nodiscard]] AbstractExpr *getValue() const;

  [[nodiscard]] std::string getNodeName() const override;

  ArithmeticExpr *contains(ArithmeticExpr *aexpTemplate, ArithmeticExpr *excludedSubtree) override;

  std::string getVarTargetIdentifier() override;

  bool isEqual(AbstractStatement *as) override;

  bool supportsCircuitMode() override;

  int getMaxNumberChildren() override;

  void setAttribute(AbstractExpr *assignmentValue);
};

#endif //AST_OPTIMIZER_INCLUDE_VARASSIGNM_H
