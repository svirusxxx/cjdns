/* vim: set expandtab ts=4 sw=4: */
/*
 * You may redistribute this program and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef Message_H
#define Message_H

#include "exception/Except.h"
#include <stdint.h>

#include "memory/Allocator.h"
#include "util/Bits.h"
#include "util/UniqueName.h"

struct Message
{
    /** The length of the message. */
    int32_t length;

    /** The number of bytes of padding BEFORE where bytes begins. */
    int32_t padding;

    /** The content. */
    uint8_t* bytes;

    /** Amount of bytes of storage space available in the message. */
    uint32_t capacity;

    /** The allocator which allocated space for this message. */
    struct Allocator* alloc;
};

#define Message_STACK(name, messageLength, amountOfPadding) \
    uint8_t UniqueName_get()[messageLength + amountOfPadding]; \
    name = &(struct Message){                                  \
        .length = messageLength,                               \
        .bytes = UniqueName_get() + amountOfPadding,           \
        .padding = amountOfPadding,                            \
        .capacity = messageLength                              \
    }

static inline struct Message* Message_clone(struct Message* toClone,
                                            struct Allocator* allocator)
{
    uint32_t len = toClone->length + toClone->padding;
    if (len < (toClone->capacity + toClone->padding)) {
        len = (toClone->capacity + toClone->padding);
    }
    uint8_t* allocation = Allocator_malloc(allocator, len);
    Bits_memcpy(allocation, toClone->bytes - toClone->padding, len);
    return Allocator_clone(allocator, (&(struct Message) {
        .length = toClone->length,
        .padding = toClone->padding,
        .bytes = allocation + toClone->padding,
        .capacity = toClone->length,
        .alloc = allocator
    }));
}

static inline void Message_copyOver(struct Message* output,
                                    struct Message* input,
                                    struct Allocator* allocator)
{
    size_t inTotalLength = input->length + input->padding;
    size_t outTotalLength = output->length + output->padding;
    uint8_t* allocation = output->bytes - output->padding;
    if (inTotalLength > outTotalLength) {
        allocation = Allocator_realloc(allocator, allocation, inTotalLength);
    }
    Bits_memcpy(allocation, input->bytes - input->padding, inTotalLength);
    output->bytes = allocation + input->padding;
    output->length = input->length;
    output->padding = input->padding;
}

/**
 * Pretend to shift the content forward by amount.
 * Really it shifts the bytes value backward.
 */
#ifndef Message_shift
static inline int Message_shift(struct Message* toShift, int32_t amount)
{
    if (amount > 0 && toShift->padding < amount) {
        Except_throw("buffer overflow");
    } else if (toShift->length < (-amount)) {
        Except_throw("buffer underflow");
    }

    toShift->length += amount;
    toShift->capacity += amount;
    toShift->bytes -= amount;
    toShift->padding -= amount;

    return 1;
}
#endif

static inline void Message_push(struct Message* restrict msg,
                                const void* restrict object,
                                size_t size)
{
    Message_shift(msg, (int)size);
    Bits_memcpy(msg->bytes, object, size);
}

static inline void Message_pop(struct Message* restrict msg,
                               void* restrict object,
                               size_t size)
{
    Message_shift(msg, -((int)size));
    Bits_memcpy(object, &msg->bytes[-((int)size)], size);
}

#define Message_popGeneric(size) \
    static inline uint ## size ## _t Message_pop ## size (struct Message* msg) \
    {                                                                          \
        uint ## size ## _t out;                                                \
        Message_pop(msg, &out, (size)/8);                                      \
        return out;                                                            \
    }

Message_popGeneric(8)
Message_popGeneric(16)
Message_popGeneric(32)
Message_popGeneric(64)


#define Message_pushGeneric(size) \
    static inline void Message_push ## size (struct Message* msg, uint ## size ## _t dat) \
    {                                                                                     \
        Message_push(msg, &dat, (size)/8);                                                \
    }

Message_pushGeneric(8)
Message_pushGeneric(16)
Message_pushGeneric(32)
Message_pushGeneric(64)

#endif
