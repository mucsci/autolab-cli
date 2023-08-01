#include "pseudocrypto.h"

#include <cstring>

#include <string>

#include <openssl/err.h>
#include <openssl/evp.h>

#include "logger.h"

#define MAX_CIPHERTEXT_LEN 256

void exit_with_crypto_error() {
  Logger::fatal << "OpenSSL error" << Logger::endl;
  ERR_print_errors_fp(stderr);
  exit(-1);
}

void check_key_and_iv_lengths(unsigned char *key, unsigned char *iv) {
  if (strnlen((char *)key, key_length_in_chars + 1) != key_length_in_chars) {
    Logger::fatal << "[Pseudocrypto] key length error" << Logger::endl;
    exit(-1);
  }
  if (strnlen((char *)iv, iv_length_in_chars + 1) != iv_length_in_chars) {
    Logger::fatal << "[Pseudocrypto] iv length error" << Logger::endl;
    exit(-1);
  }
}

std::string encrypt_string(std::string srctext, unsigned char *key,
    unsigned char *iv) {
  return srctext;
}

std::string decrypt_string(char *srctext, size_t srclength, unsigned char *key,
    unsigned char *iv) {
  return std::string{srctext, srctext + srclength};
}
