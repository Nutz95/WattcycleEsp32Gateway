#include "EcoFlow/Crypto/EcoFlowSessionCrypto.h"

#include "EcoFlow/Crypto/EcoFlowLoginKey.h"

#include <Arduino.h>
#include <MD5Builder.h>
#include <esp_system.h>
#include <mbedtls/aes.h>

#include <cstring>

namespace ecoflow::crypto {
namespace {

// SECP160r1 parameters (same curve EcoFlow V3 BLE uses).
constexpr char kSecp160r1P[] = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFF";
constexpr char kSecp160r1A[] = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFC";
constexpr char kSecp160r1B[] = "1C97BEFC54BD7A8B65ACF89F81D4D4ADC565FA45";
constexpr char kSecp160r1Gx[] = "4A96B5688EF573284664698968C38BB913CBFC82";
constexpr char kSecp160r1Gy[] = "23A628553168947D59DCC912042351377AC5FB32";
constexpr char kSecp160r1N[] = "01000000000000000001F4C8F927AED3CA752257";

void md5Bytes(const uint8_t* data, size_t length, uint8_t out[16]) {
  MD5Builder md5;
  md5.begin();
  md5.add(const_cast<uint8_t*>(data), length);
  md5.calculate();
  md5.getBytes(out);
}

}  // namespace

EcoFlowSessionCrypto::EcoFlowSessionCrypto() {
  mbedtls_ecp_group_init(&group_);
  mbedtls_mpi_init(&privateKey_);
  mbedtls_ecp_point_init(&publicPoint_);
  curveReady_ = loadCurve();
}

EcoFlowSessionCrypto::~EcoFlowSessionCrypto() {
  reset();
  mbedtls_ecp_group_free(&group_);
  mbedtls_mpi_free(&privateKey_);
  mbedtls_ecp_point_free(&publicPoint_);
}

void EcoFlowSessionCrypto::reset() {
  hasShared_ = false;
  hasSession_ = false;
  std::memset(publicKey_, 0, sizeof(publicKey_));
  std::memset(sharedAesKey_, 0, sizeof(sharedAesKey_));
  std::memset(sessionKey_, 0, sizeof(sessionKey_));
  std::memset(iv_, 0, sizeof(iv_));
}

bool EcoFlowSessionCrypto::loadCurve() {
  if (mbedtls_mpi_read_string(&group_.P, 16, kSecp160r1P) != 0 ||
      mbedtls_mpi_read_string(&group_.A, 16, kSecp160r1A) != 0 ||
      mbedtls_mpi_read_string(&group_.B, 16, kSecp160r1B) != 0 ||
      mbedtls_mpi_read_string(&group_.N, 16, kSecp160r1N) != 0 ||
      mbedtls_mpi_read_string(&group_.G.X, 16, kSecp160r1Gx) != 0 ||
      mbedtls_mpi_read_string(&group_.G.Y, 16, kSecp160r1Gy) != 0 ||
      mbedtls_mpi_lset(&group_.G.Z, 1) != 0) {
    Serial.println("[ecoflow-crypto] SECP160r1 load failed");
    return false;
  }
  group_.pbits = mbedtls_mpi_bitlen(&group_.P);
  group_.nbits = mbedtls_mpi_bitlen(&group_.N);
  group_.h = 1;
  group_.id = MBEDTLS_ECP_DP_NONE;
  return true;
}

int EcoFlowSessionCrypto::fillRandom(void* /*ctx*/, unsigned char* output, size_t len) {
  esp_fill_random(output, len);
  return 0;
}

bool EcoFlowSessionCrypto::generateKeyPair() {
  if (!curveReady_) {
    return false;
  }
  if (mbedtls_ecp_gen_keypair(&group_, &privateKey_, &publicPoint_, fillRandom, nullptr) != 0) {
    Serial.println("[ecoflow-crypto] ecdh keygen failed");
    return false;
  }

  uint8_t uncompressed[41] = {};
  size_t written = 0;
  if (mbedtls_ecp_point_write_binary(&group_, &publicPoint_, MBEDTLS_ECP_PF_UNCOMPRESSED, &written,
                                    uncompressed, sizeof(uncompressed)) != 0 ||
      written != 41) {
    Serial.println("[ecoflow-crypto] export pubkey failed");
    return false;
  }
  std::memcpy(publicKey_, uncompressed + 1, kPublicKeyBytes);
  return true;
}

bool EcoFlowSessionCrypto::computeSharedSecret(const uint8_t* peerPubKey40, size_t peerPubKeyLen) {
  if (!curveReady_ || peerPubKey40 == nullptr || peerPubKeyLen != kPublicKeyBytes) {
    return false;
  }

  uint8_t uncompressed[41];
  uncompressed[0] = 0x04;
  std::memcpy(uncompressed + 1, peerPubKey40, kPublicKeyBytes);

  mbedtls_ecp_point peerQ;
  mbedtls_ecp_point sharedP;
  mbedtls_ecp_point_init(&peerQ);
  mbedtls_ecp_point_init(&sharedP);
  bool ok = false;

  if (mbedtls_ecp_point_read_binary(&group_, &peerQ, uncompressed, sizeof(uncompressed)) == 0 &&
      mbedtls_ecp_mul(&group_, &sharedP, &privateKey_, &peerQ, fillRandom, nullptr) == 0) {
    uint8_t buf[41] = {};
    size_t len = 0;
    if (mbedtls_ecp_point_write_binary(&group_, &sharedP, MBEDTLS_ECP_PF_UNCOMPRESSED, &len, buf,
                                      sizeof(buf)) == 0 &&
        len == 41) {
      uint8_t fullShared[kSharedSecretBytes];
      std::memcpy(fullShared, buf + 1, kSharedSecretBytes);
      md5Bytes(fullShared, kSharedSecretBytes, iv_);
      std::memcpy(sharedAesKey_, fullShared, kAesKeyBytes);
      hasShared_ = true;
      ok = true;
    }
  }

  mbedtls_ecp_point_free(&peerQ);
  mbedtls_ecp_point_free(&sharedP);
  if (!ok) {
    Serial.println("[ecoflow-crypto] shared secret failed");
  }
  return ok;
}

bool EcoFlowSessionCrypto::deriveSessionKey(const uint8_t seed[kSeedBytes],
                                           const uint8_t srand[kSrandBytes]) {
  if (seed == nullptr || srand == nullptr || kEcoFlowLoginKeySize < 16) {
    return false;
  }
  const size_t pos =
      static_cast<size_t>(seed[0]) * 0x10U +
      (static_cast<size_t>((seed[1] - 1) & 0xFF) * 0x100U);
  if (pos + 16U > kEcoFlowLoginKeySize) {
    Serial.printf("[ecoflow-crypto] login_key pos %u out of range\n",
                  static_cast<unsigned>(pos));
    return false;
  }

  uint8_t material[32];
  std::memcpy(material, &kEcoFlowLoginKey[pos], 16);
  std::memcpy(material + 16, srand, kSrandBytes);
  md5Bytes(material, sizeof(material), sessionKey_);
  hasSession_ = true;
  return true;
}

size_t EcoFlowSessionCrypto::pkcs7Pad(const uint8_t* in, size_t inLen, uint8_t* out,
                                      size_t outCap) {
  const size_t pad = 16U - (inLen % 16U);
  const size_t total = inLen + pad;
  if (out == nullptr || total > outCap) {
    return 0;
  }
  if (inLen > 0 && in != nullptr) {
    std::memcpy(out, in, inLen);
  }
  std::memset(out + inLen, static_cast<int>(pad), pad);
  return total;
}

size_t EcoFlowSessionCrypto::pkcs7Unpad(uint8_t* buf, size_t len) {
  if (buf == nullptr || len == 0 || (len % 16U) != 0) {
    return 0;
  }
  const uint8_t pad = buf[len - 1];
  if (pad == 0 || pad > 16 || pad > len) {
    return 0;
  }
  for (size_t i = 0; i < pad; ++i) {
    if (buf[len - 1 - i] != pad) {
      return 0;
    }
  }
  return len - pad;
}

size_t EcoFlowSessionCrypto::aesCbc(bool encrypt, const uint8_t* key, const uint8_t* input,
                                    size_t inputLen, uint8_t* output, size_t outCapacity) const {
  if (key == nullptr || input == nullptr || output == nullptr || inputLen == 0 ||
      (inputLen % 16U) != 0 || inputLen > outCapacity) {
    return 0;
  }
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  uint8_t ivCopy[kIvBytes];
  std::memcpy(ivCopy, iv_, kIvBytes);
  int rc = -1;
  if (encrypt) {
    rc = mbedtls_aes_setkey_enc(&aes, key, 128);
    if (rc == 0) {
      rc = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, inputLen, ivCopy, input, output);
    }
  } else {
    rc = mbedtls_aes_setkey_dec(&aes, key, 128);
    if (rc == 0) {
      rc = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, inputLen, ivCopy, input, output);
    }
  }
  mbedtls_aes_free(&aes);
  return rc == 0 ? inputLen : 0;
}

size_t EcoFlowSessionCrypto::encryptSession(const uint8_t* plaintext, size_t plaintextLen,
                                            uint8_t* out, size_t outCapacity) const {
  if (!hasSession_ || plaintext == nullptr) {
    return 0;
  }
  // Prefer in-place pad into `out` when caller left room; else use a static pad buffer.
  static uint8_t padded[512];
  const size_t paddedLen = pkcs7Pad(plaintext, plaintextLen, padded, sizeof(padded));
  if (paddedLen == 0) {
    return 0;
  }
  return aesCbc(true, sessionKey_, padded, paddedLen, out, outCapacity);
}

size_t EcoFlowSessionCrypto::decryptSession(const uint8_t* ciphertext, size_t ciphertextLen,
                                            uint8_t* out, size_t outCapacity) const {
  if (!hasSession_) {
    return 0;
  }
  const size_t decrypted = aesCbc(false, sessionKey_, ciphertext, ciphertextLen, out, outCapacity);
  if (decrypted == 0) {
    return 0;
  }
  return pkcs7Unpad(out, decrypted);
}

size_t EcoFlowSessionCrypto::decryptShared(const uint8_t* ciphertext, size_t ciphertextLen,
                                           uint8_t* out, size_t outCapacity) const {
  if (!hasShared_) {
    return 0;
  }
  const size_t decrypted =
      aesCbc(false, sharedAesKey_, ciphertext, ciphertextLen, out, outCapacity);
  if (decrypted == 0) {
    return 0;
  }
  return pkcs7Unpad(out, decrypted);
}

}  // namespace ecoflow::crypto
