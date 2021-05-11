
#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_RUNTIME_ABSTRACTNOISEMEASURINGCIPHERTEXT_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_RUNTIME_ABSTRACTNOISEMEASURINGCIPHERTEXT_H_

#include "AbstractCiphertext.h"

class AbstractNoiseMeasuringCiphertext : public AbstractCiphertext {
 protected:
  /// Protected constructor: makes sure that class is abstract, i.e., cannot be instantiated.
  explicit AbstractNoiseMeasuringCiphertext(AbstractCiphertextFactory  &acf) :  AbstractCiphertext(acf) {};

 public:

  /// Return the current noise budget in the ciphertext, measured in bits
  /// \return A double measuring the number of bits in the noise budget.
  virtual void noiseBits() = 0;

  /// Noise of a freshly encrypted ciphertext
  /// \return the noise budget in bits
  [[nodiscard]] virtual int64_t initialNoise() = 0;

};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_RUNTIME_ABSTRACTNOISEMEASURINGCIPHERTEXT_H_