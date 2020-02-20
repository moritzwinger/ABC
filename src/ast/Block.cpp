#include "Block.h"
#include <iostream>
#include <exception>
#include "VarDecl.h"
#include "AbstractNode.h"

json Block::toJson() const {
  std::vector<AbstractStatement *> stmts;
  stmts.reserve(children.size());
  for (auto c : children) {
    stmts.push_back(dynamic_cast<AbstractStatement *>(c));
  }
  json j = {{"type", getNodeName()},
            {"statements", stmts}};
  return j;
}

Block::Block(AbstractStatement *stat) {
  children.emplace_back(stat);
}

Block::Block(std::vector<AbstractStatement *> *statements) {
  if (statements->empty()) {
    std::string errorMsg = "Block statement vector is empty!"
                           "If this is intended, use the parameter-less constructor instead.";
    throw std::logic_error(errorMsg);
  }
  children.reserve(statements->size());
  for (auto s : *statements) {
    children.push_back(s);
  }
}

void Block::accept(Visitor &v) {
  v.visit(*this);
}

std::string Block::getNodeName() const {
  return "Block";
}

std::vector<AbstractStatement *> *Block::getStatements() const {
  auto stmts = new std::vector<AbstractStatement *>;
  stmts->reserve(children.size());
  for (auto c : children) {
    stmts->emplace_back(dynamic_cast<AbstractStatement *>(c));
  }
  return stmts;
}

Block *Block::clone(bool keepOriginalUniqueNodeId) {
  auto clonedStatements = new std::vector<AbstractStatement *>();
  for (auto &statement : *this->getStatements()) {
    clonedStatements->push_back(statement->clone(keepOriginalUniqueNodeId)->castTo<AbstractStatement>());
  }
  auto clonedNode = clonedStatements->empty() ? new Block() :  new Block(clonedStatements);
  if (keepOriginalUniqueNodeId) clonedNode->setUniqueNodeId(this->getUniqueNodeId());
  if (this->isReversed) clonedNode->swapChildrenParents();
  return clonedNode;
}
AbstractNode *Block::cloneFlat() {
  //TODO(vianda): Implement cloneFlat in Block
  throw std::runtime_error("Not implemented");
}
int Block::getMaxNumberChildren() {
  return -1;
}
bool Block::supportsCircuitMode() {
  return true;
}
std::string Block::toString() const {
  // return an empty string if there are no children
  if (children.empty()) return "";
  // otherwise return the concatenated string representation for each of the children
  std::stringstream ss;
  for (auto &child : getChildrenNonNull()) {
    ss << child->toString() << ", ";
  }
  ss << "\b\b" << std::endl;
  return ss.str();
}
