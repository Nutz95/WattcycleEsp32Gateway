#pragma once

#include <mbedtls/bignum.h>
#include <mbedtls/ecp.h>

#include <cstddef>
#include <cstdint>

namespace ecoflow::crypto {

/// V3 session crypto: SECP160r1 ECDH + AES-128-CBC + login_key session derive.
class EcoFlowSessionCrypto {
 public:
  static constexpr size_t kPublicKeyBytes = 40;
  static constexpr size_t kSharedSecretBytes = 20;
  static constexpr size_t kAesKeyBytes = 16;
  static constexpr size_t kIvBytes = 16;
  static constexpr size_t kSeedBytes = 2;
  static constexpr size_t kSrandBytes = 16;

  EcoFlowSessionCrypto();
  ~EcoFlowSessionCrypto();

  EcoFlowSessionCrypto(const EcoFlowSessionCrypto&) = delete;
  EcoFlowSessionCrypto& operator=(const EcoFlowSessionCrypto&) = delete;

  /// Generate SECP160r1 key pair and export 40-byte public key.
  bool generateKeyPair();
  /// ECDH with peer public key; stores shared AES key material.
  bool computeSharedSecret(const uint8_t* peerPubKey40, size_t peerPubKeyLen);
  /// Derive AES session key from KeyInfo seed/srand via login_key.
  bool deriveSessionKey(const uint8_t seed[kSeedBytes], const uint8_t srand[kSrandBytes]);

  /// PKCS7-pad then AES-CBC encrypt with session key. Returns ciphertext length (0 on fail).
  size_t encryptSession(const uint8_t* plaintext, size_t plaintextLen, uint8_t* out,
                        size_t outCapacity) const;

  /// AES-CBC decrypt with session key then PKCS7-unpad. Returns plaintext length (0 on fail).
  size_t decryptSession(const uint8_t* ciphertext, size_t ciphertextLen, uint8_t* out,
                        size_t outCapacity) const;

  /// Decrypt KeyInfo payload with temporary shared AES key (first 16 of ECDH X).
  size_t decryptShared(const uint8_t* ciphertext, size_t ciphertextLen, uint8_t* out,
                       size_t outCapacity) const;

  /// Local uncompressed public key bytes (40).
  const uint8_t* publicKey() const { return publicKey_; }
  /// True after ECDH shared secret / shared AES key is ready.
  bool hasSharedSecret() const { return hasShared_; }
  /// True after session AES key and IV are derived.
  bool hasSessionKey() const { return hasSession_; }
  /// Clear keys and mbedTLS curve state for a new handshake.
  void reset();

 private:
  static int fillRandom(void* /*ctx*/, unsigned char* output, size_t len);
  static size_t pkcs7Pad(const uint8_t* in, size_t inLen, uint8_t* out, size_t outCap);
  static size_t pkcs7Unpad(uint8_t* buf, size_t len);
  size_t aesCbc(bool encrypt, const uint8_t* key, const uint8_t* input, size_t inputLen,
                uint8_t* output, size_t outCapacity) const;
  bool loadCurve();

  mbedtls_ecp_group group_{};
  mbedtls_mpi privateKey_{};
  mbedtls_ecp_point publicPoint_{};
  bool curveReady_ = false;
  bool hasShared_ = false;
  bool hasSession_ = false;

  uint8_t publicKey_[kPublicKeyBytes] = {};
  uint8_t sharedAesKey_[kAesKeyBytes] = {};
  uint8_t sessionKey_[kAesKeyBytes] = {};
  uint8_t iv_[kIvBytes] = {};
};

}  // namespace ecoflow::crypto
