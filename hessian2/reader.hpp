#pragma once

#include <string>

#include "byte_order.h"

namespace hessian2 {

class Reader {
 public:
  Reader() = default;
  virtual ~Reader() = default;
  // Returns the current position that has been read.
  virtual uint64_t Offset() const { return initial_offset_; }
  // Returns the current entire buffer size, containing the portion that has
  // been read.
  virtual uint64_t Length() const = 0;
  // How much buffer is currently unread.
  virtual uint64_t ByteAvailable() const { return Length() - Offset(); }
  virtual void RawReadNbytes(void* data, size_t len, size_t offset) = 0;

  void ReadNbytes(void* data, size_t len) {
    RawReadNbytes(data, len, 0);
    initial_offset_ += len;
  }

  template <typename T, ByteOrderType Endianness = ByteOrderType::Host,
            size_t Size = sizeof(T)>
  std::pair<bool, typename std::enable_if<std::is_integral<T>::value, T>::type>
  Peek(uint64_t peek_offset = 0) {
    auto result = static_cast<T>(0);
    constexpr const auto all_bits_enabled = static_cast<T>(~static_cast<T>(0));
    if (ByteAvailable() - peek_offset < Size) {
      return std::pair<bool, T>{false, result};
    }
    constexpr const auto displacement =
        Endianness == ByteOrderType::BigEndian ? sizeof(T) - Size : 0;
    int8_t* bytes = reinterpret_cast<int8_t*>(std::addressof(result));
    RawReadNbytes(&bytes[displacement], Size, peek_offset);
    constexpr const auto most_significant_read_byte =
        Endianness == ByteOrderType::BigEndian ? displacement : Size - 1;

    // If Size == sizeof(T), we need to make sure we don't generate an invalid
    // left shift (e.g. int32 << 32), even though we know that that branch of
    // the conditional will. not be taken. Size % sizeof(T) gives us the correct
    // left shift when Size < sizeof(T), and generates a left shift of 0 bits
    // when Size == sizeof(T)
    const auto sign_extension_bits =
        std::is_signed<T>::value && Size < sizeof(T) &&
                bytes[most_significant_read_byte] < 0
            ? static_cast<T>(static_cast<typename std::make_unsigned<T>::type>(
                                 all_bits_enabled)
                             << ((Size % sizeof(T)) * CHAR_BIT))
            : static_cast<T>(0);
    return std::make_pair(true, fromEndian<Endianness>(static_cast<T>(result)) |
                                    sign_extension_bits);
  }

  template <typename T, ByteOrderType Endianness = ByteOrderType::Host,
            size_t Size = sizeof(T)>
  std::pair<bool, typename std::enable_if<std::is_integral<T>::value, T>::type>
  Read() {
    auto result = Peek<T, Endianness, Size>(0);
    if (result.first) {
      initial_offset_ += Size;
    }
    return result;
  }

  template <typename T>
  std::pair<bool, T> ReadLE() {
    return Read<T, ByteOrderType::LittleEndian, sizeof(T)>();
  }

  template <typename T>
  std::pair<bool, T> ReadBE() {
    return Read<T, ByteOrderType::BigEndian, sizeof(T)>();
  }

  template <typename T>
  std::pair<bool, T> PeekLE(uint64_t peek_offset = 0) {
    return Peek<T, ByteOrderType::LittleEndian, sizeof(T)>(peek_offset);
  }

  template <typename T>
  std::pair<bool, T> PeeBE(uint64_t peek_offset = 0) {
    return Peek<T, ByteOrderType::BigEndian, sizeof(T)>(peek_offset);
  }

 protected:
  uint64_t initial_offset_{0};
};

using ReaderPtr = std::unique_ptr<Reader>;

}  // namespace hessian2