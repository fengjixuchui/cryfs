#include "EncryptedBlock.h"
#include <cryptopp/cryptopp/modes.h>

#include "../../utils/BlockStoreUtils.h"

using CryptoPP::CBC_Mode;
using CryptoPP::AES;

using std::make_unique;

namespace blockstore {
namespace encrypted {

constexpr unsigned int EncryptedBlock::IV_SIZE;

std::unique_ptr<EncryptedBlock> EncryptedBlock::CreateNew(std::unique_ptr<Block> baseBlock, const EncryptionKey &encKey) {
  auto block = make_unique<EncryptedBlock>(std::move(baseBlock), encKey);
  //We have to explicitly fill the block with zeroes, because otherwise the encrypted version is filled with zeroes and not the plaintext version
  utils::fillWithZeroes(block.get());
  return block;
}

EncryptedBlock::EncryptedBlock(std::unique_ptr<Block> baseBlock, const EncryptionKey &encKey)
    :Block(baseBlock->key()),
     _baseBlock(std::move(baseBlock)),
     _plaintextData(USEABLE_BLOCK_SIZE(_baseBlock->size())),
     _encKey(encKey),
     _dataChanged(false) {
  _decryptFromBaseBlock();
}

EncryptedBlock::~EncryptedBlock() {
  flush();
}

const void *EncryptedBlock::data() const {
  return _plaintextData.data();
}

void EncryptedBlock::write(const void *source, uint64_t offset, uint64_t size) {
  assert(offset <= _plaintextData.size() && offset + size <= _plaintextData.size()); //Also check offset < _data->size() because of possible overflow in the addition
  std::memcpy((uint8_t*)_plaintextData.data()+offset, source, size);
  _dataChanged = true;
}

void EncryptedBlock::flush() {
  _encryptToBaseBlock();
  return _baseBlock->flush();
}

size_t EncryptedBlock::size() const {
  return _plaintextData.size();
}

void EncryptedBlock::_decryptFromBaseBlock() {
  const byte *iv = (byte*)_baseBlock->data();
  const byte *data = (byte*)_baseBlock->data() + IV_SIZE;
  auto dec = CBC_Mode<AES>::Decryption((byte*)_encKey.data(), EncryptionKey::BINARY_LENGTH, iv);
  std::memcpy(_plaintextData.data(), (byte*)_baseBlock->data()+IV_SIZE, _plaintextData.size());
}

void EncryptedBlock::_encryptToBaseBlock() {
  if (_dataChanged) {
    FixedSizeData<IV_SIZE> iv = FixedSizeData<IV_SIZE>::CreateRandom();
    auto enc = CBC_Mode<AES>::Decryption(_encKey.data(), EncryptionKey::BINARY_LENGTH, iv.data());
    _baseBlock->write(iv.data(), 0, IV_SIZE);
    _baseBlock->write(_plaintextData.data(), IV_SIZE, _plaintextData.size());
    _dataChanged = false;
  }
}

}
}