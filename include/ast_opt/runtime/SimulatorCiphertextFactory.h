#ifndef GRAPHNODE_H_INCLUDE_AST_OPT_VISITOR_RUNTIME_SIMULATORCIPHERTEXTFACTORY_H_
#define GRAPHNODE_H_INCLUDE_AST_OPT_VISITOR_RUNTIME_SIMULATORCIPHERTEXTFACTORY_H_

#include <memory>
#include "AbstractCiphertextFactory.h"

#ifdef HAVE_SEAL_BFV
#include <seal/seal.h>

class SimulatorCiphertextFactory : public AbstractCiphertextFactory {
 private:
  /// The number of slots (i.e., maximum no. of elements) in a ciphertext.
  const unsigned int ciphertextSlotSize = 8192;

  seal::EncryptionParameters _params;

  /// The SEAL context.
  std::shared_ptr<seal::SEALContext> context;

  /// The encoder helper object.
  std::unique_ptr<seal::BatchEncoder> encoder;

  /// Sets up the FHE scheme by creating a new context, setting required scheme parameters, generating keys, and
  /// instantiating the helper objects.
  void setupSealContext();

  /// Fills a given vector with its last element until it has ciphertextSlotSize elements.
  /// \tparam T The type of the elements that should be encoded in the ciphertext.
  /// \param values The vector that should be expanded by its last element.
  /// \throws std::runtime_error if the number of elements in values is larger than the size defined in
  /// ciphertextSlotSize.
  template<typename T>
  std::vector<T> expandVector(const std::vector<T> &values) const;

 public:
  SimulatorCiphertextFactory() = default;

  /// Creates a new SealCiphertextFactory whereat each ciphertext created by the factory can hold
  /// numElementsPerCiphertextSlot elements.
  /// \param numElementsPerCiphertextSlot The number of ciphertext slots of created ciphertexts. Must be a power of
  /// two, e.g., 4'096 or 8'192.
  explicit SimulatorCiphertextFactory(unsigned int numElementsPerCiphertextSlot);

  SimulatorCiphertextFactory(const SimulatorCiphertextFactory &other); // copy constructor

  SimulatorCiphertextFactory(SimulatorCiphertextFactory &&other) noexcept;  // copy assignment

  SimulatorCiphertextFactory &operator=(const SimulatorCiphertextFactory &other);  // move constructor

  SimulatorCiphertextFactory &operator=(SimulatorCiphertextFactory &&other) noexcept;  // move assignment

  /// Gets the context to retrieve parameters
  /// \return (A const reference to) the seal::SEALcontext object
  [[nodiscard]] const seal::SEALContext &getContext() const;

  /// Gets the relinearization keys, required for relinearizing a ciphertext after a multiplication, that were
  /// precomputed in setupSealContext().
  /// \return (A const reference to) the seal::RelinKeys object.
  [[nodiscard]] const seal::RelinKeys &getRelinKeys() const;

  /// Gets the Galois keys, required for rotating a ciphertext, that were precomputed in setupSealContext().
  /// \return (A const reference to) the seal::GaloisKeys object.
  [[nodiscard]] const seal::GaloisKeys &getGaloisKeys() const;

  /// Gets the seal::Evaluator instance.
  /// \return (A reference to) the seal::Evaluator instance.
  [[nodiscard]] seal::Evaluator &getEvaluator() const;

  /// Gets the number of ciphertext slots that each ciphertext generated by this factory consists of.
  /// \return The number of total slots in each ciphertext.
  [[nodiscard]] unsigned int getCiphertextSlotSize() const;

  /// Creates a new seal::Plaintext that encodes the given data (vector value) using the defined encoder.
  /// \param value The values to be encoded into the new plaintext.
  /// \return (A std::unique_ptr) to the newly created seal::Plaintext.
  std::unique_ptr<seal::Plaintext> createPlaintext(const std::vector<int> &value) const;

  /// Creates a new seal::Plaintext object that encodes the given value. Remaining slots in the plaintext are filled up
  /// with the last given value.
  /// \param value The values to be encoded in the plaintext.
  /// \return (A std::unique_ptr to) the seal::Plaintext that encodes the given values.
  std::unique_ptr<seal::Plaintext> createPlaintext(const std::vector<int64_t> &value) const;

  /// Creates a new seal::Plaintext object that encodes the given value. Remaining slots in the plaintext are filled up
  /// with the same value.
  /// \param value The value to be encoded in the plaintext.
  /// \return (A std::unique_ptr to) the seal::Plaintext that encodes the given value.
  std::unique_ptr<seal::Plaintext> createPlaintext(int64_t value) const;

  std::unique_ptr<AbstractCiphertext> createCiphertext(const std::vector<int64_t> &data)  const override;

  std::unique_ptr<AbstractCiphertext> createCiphertext(int64_t data)  const override;

  void decryptCiphertext(AbstractCiphertext &abstractCiphertext, std::vector<int64_t> &ciphertextData)  const override;

  std::string getString(AbstractCiphertext &abstractCiphertext)  const override;

  std::unique_ptr<AbstractCiphertext> createCiphertext(std::unique_ptr<AbstractValue> &&abstractValue)  const override;

  std::unique_ptr<AbstractCiphertext> createCiphertext(const std::vector<int> &data)  const override;
};

#endif

#endif //GRAPHNODE_H_INCLUDE_AST_OPT_VISITOR_RUNTIME_SEALCIPHERTEXTFACTORY_H_
