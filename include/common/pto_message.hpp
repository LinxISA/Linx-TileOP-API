// Typed kernel messages for the opt-in scalar MSGB transport.
#ifndef PTO_MESSAGE_HPP
#define PTO_MESSAGE_HPP

#include "linx_msgb.h"

namespace pto {
namespace detail {

template <typename T>
constexpr size_t message_word_count()
{
    static_assert(__is_class(T) && __is_trivially_copyable(T),
                  "MSGB requires a trivially copyable struct or class");
    static_assert(sizeof(T) <= LINX_MSGB_MAX_WORDS * sizeof(uint64_t),
                  "MSGB payload exceeds 80 bytes");
    return (sizeof(T) + sizeof(uint64_t) - 1) / sizeof(uint64_t);
}

// Keep these bounded copies inline on the freestanding Linx target.
template <size_t Bytes>
inline void copy_message_bytes(void *destination, const void *source)
{
#if defined(__linx_v5) && defined(__clang__)
    __builtin_memcpy_inline(destination, source, Bytes);
#else
    __builtin_memcpy(destination, source, Bytes);
#endif
}

} // namespace detail

// Both endpoints use the same payload type and object representation.
template <typename T>
inline linx_msgb_result PTOSendMessage(uint32_t destination, const T &payload)
{
    constexpr size_t count = detail::message_word_count<T>();
    uint64_t words[count];
    words[count - 1] = 0;  // Clear the unused bytes of a partial final word.
    detail::copy_message_bytes<sizeof(T)>(words, __builtin_addressof(payload));
    linx_msgb_id id;
    return linx_msgb_try_send(destination, words, count, &id);
}

template <typename T>
inline linx_msgb_result PTORecvMessage(T &payload)
{
    constexpr size_t count = detail::message_word_count<T>();
    linx_msgb_message message;
    linx_msgb_result result = linx_msgb_try_claim(&message);
    if (result != LINX_MSGB_OK)
        return result;
    if (message.count != count) {
        result = linx_msgb_release(message.claim);
        return result == LINX_MSGB_OK ? LINX_MSGB_BAD_COMMAND : result;
    }

    result = linx_msgb_ack(message.claim);
    if (result != LINX_MSGB_OK && result != LINX_MSGB_DUPLICATE)
        return result;
    result = linx_msgb_release(message.claim);
    if (result == LINX_MSGB_OK)
        detail::copy_message_bytes<sizeof(T)>(__builtin_addressof(payload), message.words);
    return result;
}

} // namespace pto

#endif
