#include "../../include/ast/LiteralBool.h"

LiteralBool::LiteralBool(bool value) : value(value) {}

json LiteralBool::toJson() const {
  json j;
  j["type"] = getNodeName();
  j["value"] = this->value;
  return j;
}

void LiteralBool::accept(Visitor &v) {
  v.visit(*this);
}

bool LiteralBool::getValue() const {
  return value;
}

std::string LiteralBool::getTextValue() const {
  return getValue() ? "true" : "false";
}

std::string LiteralBool::getNodeName() const {
  return "LiteralBool";
}

LiteralBool::~LiteralBool() = default;

Literal* LiteralBool::evaluate(Ast &ast) {
  return this;
}

void LiteralBool::print(std::ostream &str) const {
  str << this->getTextValue();
}

bool LiteralBool::operator==(const LiteralBool &rhs) const {
  return value == rhs.value;
}

bool LiteralBool::operator!=(const LiteralBool &rhs) const {
  return !(rhs == *this);
}

void LiteralBool::storeParameterValue(std::string identifier, std::map<std::string, Literal*> &paramsMap) {
  paramsMap.emplace(identifier, this);
}