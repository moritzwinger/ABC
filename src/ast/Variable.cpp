#include <utility>

#include "../../include/ast/Variable.h"
#include "Ast.h"

Variable::Variable(std::string identifier) : identifier(std::move(identifier)) {}

json Variable::toJson() const {
  json j;
  j["type"] = getNodeType();
  j["identifier"] = this->identifier;
  return j;
}

void Variable::accept(Visitor &v) {
  v.visit(*this);
}

std::string Variable::getNodeType() const {
  return "Variable";
}

const std::string &Variable::getIdentifier() const {
  return identifier;
}

bool Variable::contains(Variable *var) {
  return *this==*var;
}

bool Variable::operator==(const Variable &rhs) const {
  return identifier==rhs.identifier;
}

bool Variable::operator!=(const Variable &rhs) const {
  return !(rhs==*this);
}

bool Variable::isEqual(AbstractExpr *other) {
  if (auto otherVar = dynamic_cast<Variable *>(other)) {
    return this->getIdentifier()==otherVar->getIdentifier();
  }
  return false;
}

std::vector<std::string> Variable::getVariableIdentifiers() {
  return {{this->getIdentifier()}};
}

std::string Variable::toString(bool printChildren) const {
  return AbstractNode::generateOutputString(printChildren, {this->getIdentifier()});
}

bool Variable::supportsCircuitMode() {
  return true;
}

Variable::~Variable() = default;

Variable *Variable::clone(bool keepOriginalUniqueNodeId) {
  auto clonedNode = new Variable(this->identifier);
  if (keepOriginalUniqueNodeId) clonedNode->setUniqueNodeId(this->getUniqueNodeId());
  if (this->isReversed) clonedNode->swapChildrenParents();
  return clonedNode;
}
