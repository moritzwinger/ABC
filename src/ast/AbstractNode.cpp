#include <sstream>
#include <queue>
#include <set>
#include "ast_opt/ast/AbstractNode.h"

///////////////////////////// GENERAL ////////////////////////////////
// C++ requires a body for the destructor even if it is declared pure virtual
AbstractNode::~AbstractNode() = default;

void AbstractNode::addChild(AbstractNode *child) {
  children.push_back(child);
}

std::vector<AbstractNode*> AbstractNode::getChildrenList() {
  return children;
}

std::vector<AbstractNode *> AbstractNode::getChildrenNonNull() const {
  std::vector<AbstractNode *> childrenFiltered;
  if (children.empty()) return childrenFiltered;
  std::copy_if(children.begin(), children.end(), std::back_inserter(childrenFiltered),
               [](AbstractNode *n) { return n!=nullptr; });
  return childrenFiltered;
}

void AbstractNode::addParent(AbstractNode *parent) {
  parents.push_back(parent);
}

std::vector<AbstractNode*> AbstractNode::getParentList() {
  return parents;
}

void AbstractNode::swapChildrenParents() {
  std::vector<AbstractNode *> oldParents = this->parents;
  this->parents = this->children;
  this->children = oldParents;
}

void AbstractNode::removeChild(AbstractNode *child) {
  auto it = std::find(children.begin(), children.end(), child);
  if (it!=children.end()) {
      children.erase(it);
  }
}

void AbstractNode::removeParent(AbstractNode *parentToBeRemoved) {
  auto it = std::find(parents.begin(), parents.end(), parentToBeRemoved);
  if (it!=parents.end()) {
    parents.erase(it);
  }
}

std::unique_ptr<AbstractNode> AbstractNode::clone(AbstractNode *parent_) const {
  return std::unique_ptr<AbstractNode>(clone_impl(parent_));
}

bool AbstractNode::operator==(const AbstractNode &other) const noexcept {
  return this==&other;
}

bool AbstractNode::operator!=(const AbstractNode &other) const noexcept {
  return !(*this==other);
}

/////////////////////////////// DAG  /////////////////////////////////

void AbstractNode::setParent(AbstractNode &newParent) {
  //TODO: Why did we want to prevent this again? It's necessary to std::move() children in a std::move() assignment!
//  if (parent) {
//    throw std::logic_error("Cannot overwrite parent.");
//  } else {
  parent = &newParent;
//  }
}

void AbstractNode::setParent(AbstractNode *newParent) {
  parent = newParent;
}

bool AbstractNode::hasParent() const {
  return parent!=nullptr;
}

AbstractNode &AbstractNode::getParent() {
  if (hasParent()) {
    return *parent;
  } else {
    throw std::runtime_error("Node has no parent.");
  }
}

AbstractNode *AbstractNode::getParentPtr() {
  return parent;
}

const AbstractNode &AbstractNode::getParent() const {
  if (hasParent()) {
    return *parent;
  } else {
    throw std::runtime_error("Node has no parent.");
  }
}

////////////////////////////// OUTPUT ///////////////////////////////

std::string AbstractNode::toString(bool printChildren) const {
  return toStringHelper(printChildren, {});
}

std::ostream &operator<<(std::ostream &os, const AbstractNode &node) {
  os << node.toString(true);
  return os;
}

std::string AbstractNode::toStringHelper(bool printChildren, std::vector<std::string> attributes) const {
  std::string indentationCharacter("\t");
  std::stringstream ss;
  // -- example output --
  // Function (computeX):
  //   ParameterList:
  //     FunctionParameter:
  //       Datatype (int, plaintext)
  //       Variable (x)
  ss << getNodeType();
  if (!attributes.empty()) {
    ss << " (";
    for (auto it = attributes.begin(); it!=attributes.end(); ++it) {
      ss << *it;
      if ((it + 1)!=attributes.end()) ss << ", ";
    }
    ss << ")";
  }
  if (printChildren && countChildren() > 0) ss << ":";
  ss << std::endl;
  if (printChildren) {
    for (auto &it : *this) ss << indentationCharacter << it.toString(printChildren);
  }
  return ss.str();
}

////////////////////////////// NODE ID ////////////////////////////////

int AbstractNode::nodeIdCounter = 0;

std::string AbstractNode::generateUniqueNodeId() const {
  if (assignedNodeId==-1) {
    throw std::logic_error("Could not find any reserved ID for node. "
                           "Node constructor needs to reserve ID for node (see empty constructor).");
  }

  // build and return the node ID string
  std::stringstream ss;
  ss << getNodeType() << "_" << assignedNodeId;
  return ss.str();
}

int AbstractNode::getAndIncrementNodeId() {
  return nodeIdCounter++;
}

AbstractNode::AbstractNode() {
  // save the ID reserved for this node but do not build the unique node ID yet as this virtual method must not be
  // called within the constructor
  assignedNodeId = getAndIncrementNodeId();
}

std::string AbstractNode::getUniqueNodeId() const {
  // if there is no ID defined yet, create and assign an ID
  if (uniqueNodeId.empty()) this->uniqueNodeId = this->generateUniqueNodeId();
  // otherwise just return the previously generated ID
  return uniqueNodeId;
}
