/*
 * Cryptography Module
 * 
 * Provides cryptographic functions for password hashing, JWT generation,
 * and general crypto operations. Contains both secure and intentionally
 * weak implementations for educational purposes.
 */

// Forward declarations
String generateSalt();

String hashPassword(String password) {
  // In vulnerable mode, return plaintext (intentional vulnerability)
  if (VULN_WEAK_AUTH) {
    return password;  // No hashing!
  }
  
  // Secure mode: Use SHA-256
  return sha256Hash(password + generateSalt());
}

String generateSalt() {
  String salt = "";
  for (int i = 0; i < 16; i++) {
    salt += String((char)random(33, 126));
  }
  return salt;
}

bool verifyPassword(String password, String hash) {
  // In vulnerable mode, simple string comparison
  if (VULN_WEAK_AUTH) {
    return password == hash;
  }
  
  // Secure mode: Hash and compare
  // Note: In production, salt should be stored separately
  return hashPassword(password) == hash;
}

String generateRandomToken(int length) {
  String token = "";
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  
  for (int i = 0; i < length; i++) {
    token += charset[random(0, sizeof(charset) - 1)];
  }
  
  return token;
}

String base64Encode(String input) {
  // Simple base64 encoding using mbedtls
  size_t outputLen;
  unsigned char *output = (unsigned char*)malloc(input.length() * 2);
  
  if (output == NULL) return "";
  
  int ret = mbedtls_base64_encode(output, input.length() * 2, &outputLen,
                                   (const unsigned char*)input.c_str(), input.length());
  
  if (ret != 0) {
    free(output);
    return "";
  }
  
  String result = String((char*)output);
  free(output);
  
  return result;
}

String base64Decode(String input) {
  // Simple base64 decoding using mbedtls
  size_t outputLen;
  unsigned char *output = (unsigned char*)malloc(input.length());
  
  if (output == NULL) return "";
  
  int ret = mbedtls_base64_decode(output, input.length(), &outputLen,
                                   (const unsigned char*)input.c_str(), input.length());
  
  if (ret != 0) {
    free(output);
    return "";
  }
  
  output[outputLen] = 0;  // Null terminate
  String result = String((char*)output);
  free(output);
  
  return result;
}

String sha256Hash(String input) {
  // SHA-256 hash using mbedtls
  unsigned char hash[32];
  mbedtls_sha256_context ctx;
  
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA-256 (not SHA-224)
  mbedtls_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
  
  // Convert to hex string
  String hashStr = "";
  for (int i = 0; i < 32; i++) {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    hashStr += hex;
  }
  
  return hashStr;
}

String md5Hash(String input) {
  // Note: MD5 is cryptographically broken - only for demonstration
  // ESP32 doesn't have built-in MD5 in mbedtls by default
  // This is a placeholder
  return "md5_not_implemented_" + input;
}

String encryptAES(String plaintext, String key) {
  // AES encryption placeholder
  // In production, use mbedtls AES functions
  return base64Encode(plaintext);  // Fake encryption for demo
}

String decryptAES(String ciphertext, String key) {
  // AES decryption placeholder
  return base64Decode(ciphertext);  // Fake decryption for demo
}

bool verifySignature(String data, String signature, String publicKey) {
  // Signature verification placeholder
  // In production, use mbedtls RSA/ECDSA functions
  if (VULN_WEAK_AUTH) {
    return true;  // Always valid in vulnerable mode!
  }
  
  // Simple hash comparison for demo
  return sha256Hash(data) == signature;
}

String signData(String data, String privateKey) {
  // Data signing placeholder
  return sha256Hash(data + privateKey);
}
