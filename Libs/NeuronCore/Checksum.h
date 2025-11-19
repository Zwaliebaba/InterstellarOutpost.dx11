#pragma once

// RFC 1071 (Internet) 16-bit one's-complement checksum.
// Returns the one's-complement of the one's-complement sum of all 16-bit words
// formed from the input (big-endian/network byte order). An odd trailing byte
// is padded with zero in the low-order bits.
// A zero-length buffer returns 0xFFFF (the complement of 0).
[[nodiscard]] constexpr uint16_t Checksum(const uint8_t* data, size_t size) noexcept
{
  if (!data)
  {
    return 0; // Treat null pointer as empty (caller error if size != 0)
  }

  uint32_t sum = 0;

  // Process 16-bit chunks
  while (size >= 2)
  {
    // Combine two bytes as big-endian into a 16-bit word
    uint16_t word = static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]));
    sum += word;
    data += 2;
    size -= 2;

    // Fold once per loop to keep sum small (optional micro-optimization)
    if (sum & 0xFFFF0000u)
    {
      sum = (sum & 0xFFFFu) + (sum >> 16);
    }
  }

  // Handle odd leftover byte (pad low 8 bits with zero)
  if (size == 1)
  {
    uint16_t word = static_cast<uint16_t>(static_cast<uint16_t>(data[0]) << 8);
    sum += word;
  }

  // Final end-around carry fold (could be needed up to twice)
  while (sum >> 16)
  {
    sum = (sum & 0xFFFFu) + (sum >> 16);
  }

  return static_cast<uint16_t>(~sum);
}
