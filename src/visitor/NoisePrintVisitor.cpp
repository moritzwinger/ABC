#include "ast_opt/ast/AbstractNode.h"
#include "ast_opt/ast/Literal.h"
#include "ast_opt/visitor/NoisePrintVisitor.h"

#include <utility>
#include "ast_opt/ast/AbstractExpression.h"

std::string SpecialNoisePrintVisitor::getIndentation() const {
  // Indent with two spaces per level
  return std::string(2*indentation_level, ' ');
}

SpecialNoisePrintVisitor::SpecialNoisePrintVisitor(std::ostream &os, std::unordered_map<std::string, int> noise_map,
                                                   std::unordered_map<std::string, double> rel_noise_map)
    : os(os), noise_map(std::move(noise_map)), rel_noise_map(std::move(rel_noise_map)) {}

void SpecialNoisePrintVisitor::visit(AbstractNode &elem) {

  // Get the current node's toString (without children)
  // This should hopefully be a single line, including end-of-line at the end
  std::string curNodeString = elem.toString(false);

  // Output current node at required indentation
  os << "--------------" << std::endl;
  os << "NODE VISITED: " << getIndentation() << curNodeString;
  auto result = noise_map.find(elem.getUniqueNodeId());
  auto result1 = rel_noise_map.find(elem.getUniqueNodeId());
  if (result!=noise_map.end()) {
    os << "NODE NOISE AT: " << getIndentation() << result->second << std::endl;
  }
  if (result1!=rel_noise_map.end()) {
    os << "REL NOISE BUDGET DECAY AFTER " <<  elem.getUniqueNodeId() << ":" << getIndentation() <<  " "  << result1->second << std::endl;
  }
  // increment indentation level and visit children, decrement afterwards
  ++indentation_level;
  for (AbstractNode &c: elem) {
    c.accept(*this);
  }
  --indentation_level;
}
