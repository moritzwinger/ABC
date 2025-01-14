#ifndef GRAPHNODE_H_INCLUDE_AST_OPT_VISITOR_RUNTIME_ABSTRACTCIPHERTEXT_H_
#define GRAPHNODE_H_INCLUDE_AST_OPT_VISITOR_RUNTIME_ABSTRACTCIPHERTEXT_H_

#include <memory>

#include "ast_opt/ast/Literal.h"
#include "AbstractValue.h"
#include "AbstractCiphertextFactory.h"
// forward declarations
class ICleartext;

class AbstractCiphertext : public AbstractValue {
 protected:
  /// Protected constructor: makes sure that class is abstract, i.e., cannot be instantiated.
  explicit AbstractCiphertext(const std::reference_wrapper<const AbstractCiphertextFactory> acf) : factory(acf) {};

  /// A reference to the factory that created this ciphertext.
  const std::reference_wrapper<const AbstractCiphertextFactory> factory;

 public:
  /// Default destructor.
  ~AbstractCiphertext() override = default;

  /// Multiplies this ciphertext with the given ciphertext and returns a new ciphertext with the result.
  /// \param operand The ciphertext to multiply_inplace this ciphertext with.
  /// \return A std::unique_ptr<AbstractCiphertext> containing the result ciphertext.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> multiply(const AbstractCiphertext &operand) const = 0;

  /// Multiplies this ciphertext with the given ciphertext by overwriting this ciphertext with the result.
  /// \param operand The ciphertext to multiply_inplace this ciphertext with.
  virtual void multiplyInplace(const AbstractCiphertext &operand) = 0;

  /// Multiplies this ciphertext with the given ExpressionList, that is, performs a ciphertext-plaintext multiplication.
  /// \param operand The operand that should be multiplied with this ciphertext.
  /// \return (A unique_ptr to) the resulting product of the multiplication.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> multiplyPlain(const ICleartext &operand) const = 0;

  /// Multiplies this ciphertext with the given ExpressionList by overwriting this ciphertext with the result, that is,
  /// performs an in-place ciphertext-plaintext multiplication.
  /// \param operand The operand that should be multiplied with this ciphertext.
  virtual void multiplyPlainInplace(const ICleartext &operand) = 0;

  /// Adds this ciphertext to the given ciphertext and returns a new ciphertext with the result.
  /// \param operand The ciphertext to add to this ciphertext.
  /// \return A std::unique_ptr<AbstractCiphertext> containing the result ciphertext.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> add(const AbstractCiphertext &operand) const = 0;

  /// Adds this ciphertext to the given ciphertext by overwriting this ciphertext with the result.
  /// \param operand The ciphertext to add to this ciphertext.
  virtual void addInplace(const AbstractCiphertext &operand) = 0;

  /// Adds this ciphertext to the given ExpressionList, that is, performs a ciphertext-plaintext addition.
  /// \param operand The operand that should be added to this ciphertext.
  /// \return (A unique_ptr to) the resulting sum of the addition.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> addPlain(const ICleartext &operand) const = 0;

  /// Adds this ciphertext to the given ExpressionList by overwriting this ciphertext with this result, that is,
  /// performs an in-place ciphertext-plaintext addition.
  /// \param operand The operand that should be added to this ciphertext.
  virtual void addPlainInplace(const ICleartext &operand) = 0;

  /// Subtracts the given ciphertext from this ciphertext and returns a new ciphertext with the result.
  /// \param operand The ciphertext to add to this ciphertext.
  /// \return A std::unique_ptr<AbstractCiphertext> containing the result ciphertext.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> subtract(const AbstractCiphertext &operand) const= 0;

  /// Subtracts the given ciphertext from this given ciphertext by overwriting this ciphertext with the result.
  /// \param operand The ciphertext to subtract_inplace from this ciphertext.
  virtual void subtractInplace(const AbstractCiphertext &operand) = 0;

  /// Subtracts the given ExpressionList from this ciphertext, that is, performs a ciphertext-plaintext subtraction.
  /// \param operand The operand that should be subtracted from this ciphertext.
  /// \return (A unique_ptr to) the result of the subtraction.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> subtractPlain(const ICleartext &operand) const = 0;

  /// Subtracts the given ExpressionList from this ciphertext by overwriting this ciphertext with the result, that is,
  /// performs an in-place ciphertext-plaintext addition.
  /// \param operand The operand that should be added to this ciphertext.
  virtual void subtractPlainInplace(const ICleartext &operand) = 0;

  /// Cyclically rotates a copy of this ciphertext by the given number of steps and returns the rotated ciphertext.
  /// \param steps The number of steps a copy of this ciphertext should be rotated.
  /// \return A std::unique_ptr<AbstractCiphertext> containing the rotated ciphertext.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> rotateRows(int steps) const = 0;

  /// Performs num modswitches on a ciphertext and returns the resulting ciphertext
  /// \param num Number of modswitches to perform
  /// \return A std::unique_ptr<AbstractCiphertext> containing the modswitched ciphertext.
  [[nodiscard]] virtual std::unique_ptr<AbstractCiphertext> modSwitch(int num) = 0;

  /// Cyclically rotates this ciphertext by the given number of steps.
  /// \param steps The number of steps this ciphertext should be rotated.
  virtual void rotateRowsInplace(int steps) = 0;

  /// Creates and returns a clone of this ciphertext.
  /// \return A clone of this ciphertext as std::unique_ptr.
  virtual std::unique_ptr<AbstractCiphertext> clone() const = 0;

  /// Gets the factory that is associated with this AbstractCiphertext, i.e., that created this ciphertext.
  /// \return  (A const reference to) the AbstractCiphertextFactory associated with this ciphertext.
  [[nodiscard]] virtual const AbstractCiphertextFactory &getFactory() const {
    return factory;
  }
};

#endif //GRAPHNODE_H_INCLUDE_AST_OPT_VISITOR_RUNTIME_ABSTRACTCIPHERTEXT_H_
