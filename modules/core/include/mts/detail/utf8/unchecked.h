// Copyright 2006 Nemanja Trifunovic

/*
Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef UTF8_FOR_CPP_UNCHECKED_H_2675DCD0_9480_4c0c_B92A_CC14C027B731
#define UTF8_FOR_CPP_UNCHECKED_H_2675DCD0_9480_4c0c_B92A_CC14C027B731

#include "core.h"

MTS_BEGIN_SUB_NAMESPACE(utf8)
namespace unchecked {
template <typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result) {
  if (cp < 0x80) // one octet
    *(result++) = static_cast<uint8_t>(cp);
  else if (cp < 0x800) { // two octets
    *(result++) = static_cast<uint8_t>((cp >> 6) | 0xc0);
    *(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
  }
  else if (cp < 0x10000) { // three octets
    *(result++) = static_cast<uint8_t>((cp >> 12) | 0xe0);
    *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
  }
  else { // four octets
    *(result++) = static_cast<uint8_t>((cp >> 18) | 0xf0);
    *(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f) | 0x80);
    *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
  }
  return result;
}

template <typename octet_iterator, typename output_iterator>
output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out, uint32_t replacement) {
  while (start != end) {
    octet_iterator sequence_start = start;
    internal::utf_error err_code = utf8::internal::validate_next(start, end);
    switch (err_code) {
    case internal::UTF8_OK:
      for (octet_iterator it = sequence_start; it != start; ++it)
        *out++ = *it;
      break;
    case internal::NOT_ENOUGH_ROOM:
      out = utf8::unchecked::append(replacement, out);
      start = end;
      break;
    case internal::INVALID_LEAD:
      out = utf8::unchecked::append(replacement, out);
      ++start;
      break;
    case internal::INCOMPLETE_SEQUENCE:
    case internal::OVERLONG_SEQUENCE:
    case internal::INVALID_CODE_POINT:
      out = utf8::unchecked::append(replacement, out);
      ++start;
      // just one replacement mark for the sequence
      while (start != end && utf8::internal::is_trail(*start))
        ++start;
      break;
    }
  }
  return out;
}

template <typename octet_iterator, typename output_iterator>
inline output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out) {
  static const uint32_t replacement_marker = utf8::internal::mask16(0xfffd);
  return utf8::unchecked::replace_invalid(start, end, out, replacement_marker);
}

template <typename octet_iterator>
uint32_t next(octet_iterator& it) {
  uint32_t cp = utf8::internal::mask8(*it);
  typename std::iterator_traits<octet_iterator>::difference_type length = utf8::internal::sequence_length(it);
  switch (length) {
  case 1:
    break;
  case 2:
    it++;
    cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
    break;
  case 3:
    ++it;
    cp = ((cp << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);
    ++it;
    cp += (*it) & 0x3f;
    break;
  case 4:
    ++it;
    cp = ((cp << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);
    ++it;
    cp += (utf8::internal::mask8(*it) << 6) & 0xfff;
    ++it;
    cp += (*it) & 0x3f;
    break;
  }
  ++it;
  return cp;
}

template <typename octet_iterator>
uint32_t peek_next(octet_iterator it) {
  return utf8::unchecked::next(it);
}

template <typename octet_iterator>
uint32_t prior(octet_iterator& it) {
  while (utf8::internal::is_trail(*(--it)))
    ;
  octet_iterator temp = it;
  return utf8::unchecked::next(temp);
}

template <typename octet_iterator, typename distance_type>
void advance(octet_iterator& it, distance_type n) {
  const distance_type zero(0);
  if (n < zero) {
    // backward
    for (distance_type i = n; i < zero; ++i)
      utf8::unchecked::prior(it);
  }
  else {
    // forward
    for (distance_type i = zero; i < n; ++i)
      utf8::unchecked::next(it);
  }
}

template <typename octet_iterator>
typename std::iterator_traits<octet_iterator>::difference_type distance(octet_iterator first, octet_iterator last) {
  typename std::iterator_traits<octet_iterator>::difference_type dist;
  for (dist = 0; first < last; ++dist)
    utf8::unchecked::next(first);
  return dist;
}

template <typename u16bit_iterator, typename octet_iterator>
octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result) {
  while (start != end) {
    uint32_t cp = utf8::internal::mask16(*start++);
    // Take care of surrogate pairs first
    if (utf8::internal::is_lead_surrogate(cp)) {
      uint32_t trail_surrogate = utf8::internal::mask16(*start++);
      cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
    }
    result = utf8::unchecked::append(cp, result);
  }
  return result;
}

template <typename u16bit_iterator, typename octet_iterator>
u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result) {
  while (start < end) {
    uint32_t cp = utf8::unchecked::next(start);
    if (cp > 0xffff) { // make a surrogate pair
      *result++ = static_cast<uint16_t>((cp >> 10) + internal::LEAD_OFFSET);
      *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
    }
    else
      *result++ = static_cast<uint16_t>(cp);
  }
  return result;
}

template <typename octet_iterator, typename u32bit_iterator>
octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result) {
  while (start != end)
    result = utf8::unchecked::append(*(start++), result);

  return result;
}

template <typename octet_iterator, typename u32bit_iterator>
u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result) {
  while (start < end)
    (*result++) = utf8::unchecked::next(start);

  return result;
}

// The iterator class
template <typename octet_iterator>
class iterator {
  octet_iterator it;

public:
  typedef uint32_t value_type;
  typedef uint32_t* pointer;
  typedef uint32_t& reference;
  typedef std::ptrdiff_t difference_type;
  typedef std::bidirectional_iterator_tag iterator_category;
  iterator() {}
  explicit iterator(const octet_iterator& octet_it)
      : it(octet_it) {}
  // the default "big three" are OK
  octet_iterator base() const { return it; }
  uint32_t operator*() const {
    octet_iterator temp = it;
    return utf8::unchecked::next(temp);
  }
  bool operator==(const iterator& rhs) const { return (it == rhs.it); }
  bool operator!=(const iterator& rhs) const { return !(operator==(rhs)); }
  iterator& operator++() {
    ::std::advance(it, utf8::internal::sequence_length(it));
    return *this;
  }
  iterator operator++(int) {
    iterator temp = *this;
    ::std::advance(it, utf8::internal::sequence_length(it));
    return temp;
  }
  iterator& operator--() {
    utf8::unchecked::prior(it);
    return *this;
  }
  iterator operator--(int) {
    iterator temp = *this;
    utf8::unchecked::prior(it);
    return temp;
  }
}; // class iterator

} // namespace unchecked
MTS_END_SUB_NAMESPACE(utf8)

#endif // header guard
