#pragma once
#ifndef MESSMER_CRYFS_SRC_CONFIG_CRYPTO_KDF_SCRYPT_H
#define MESSMER_CRYFS_SRC_CONFIG_CRYPTO_KDF_SCRYPT_H

#include <messmer/cpp-utils/macros.h>
#include <messmer/cpp-utils/random/Random.h>
extern "C" {
    #include <messmer/scrypt/lib/crypto/crypto_scrypt.h>
}
#include <stdexcept>
#include "DerivedKey.h"

namespace cryfs {

    struct SCryptParanoidSettings {
        constexpr static size_t SALT_LEN = 32; // Size of the salt
        constexpr static uint64_t N = 1048576; // CPU/Memory cost
        constexpr static uint32_t r = 8; // Blocksize
        constexpr static uint32_t p = 16; // Parallelization
    };

    struct SCryptDefaultSettings {
        constexpr static size_t SALT_LEN = 32; // Size of the salt
        constexpr static uint64_t N = 524288; // CPU/Memory cost
        constexpr static uint32_t r = 1; // Blocksize
        constexpr static uint32_t p = 1; // Parallelization
    };

    class SCrypt {
    public:
        SCrypt() {}

        template<size_t KEYSIZE, class Settings = SCryptDefaultSettings> DerivedKey<KEYSIZE> generateKey(const std::string &password) {
            auto salt = cpputils::Random::PseudoRandom().get(Settings::SALT_LEN);
            auto config = DerivedKeyConfig(std::move(salt), Settings::N, Settings::r, Settings::p);
            auto key = generateKeyFromConfig<KEYSIZE>(password, config);
            return DerivedKey<KEYSIZE>(std::move(config), key);
        }

        template<size_t KEYSIZE> cpputils::FixedSizeData<KEYSIZE> generateKeyFromConfig(const std::string &password, const DerivedKeyConfig &config) {
            auto key = cpputils::FixedSizeData<KEYSIZE>::Null();
            int errorcode = crypto_scrypt(reinterpret_cast<const uint8_t*>(password.c_str()), password.size(),
                          reinterpret_cast<const uint8_t*>(config.salt().data()), config.salt().size(),
                          config.N(), config.r(), config.p(),
                          static_cast<uint8_t*>(key.data()), KEYSIZE);
            if (errorcode != 0) {
                throw std::runtime_error("Error running scrypt key derivation.");
            }
            return key;
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(SCrypt);
    };
}

#endif
