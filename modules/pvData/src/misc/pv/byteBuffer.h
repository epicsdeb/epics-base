/* byteBuffer.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mse
 */
#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include <string>
#include <cstring>
#include <cstdlib>

#include <epicsEndian.h>
#include <shareLib.h>
#include <epicsAssert.h>
#include <compilerDependencies.h>

#include <pv/templateMeta.h>
#include <pv/pvType.h>
#include <pv/epicsException.h>


#ifndef EPICS_ALWAYS_INLINE
#  define EPICS_ALWAYS_INLINE inline
#endif

/* various compilers provide builtins for byte order swaps.
 * conditions based on boost endian library
 */
#if defined(__clang__)

#if __has_builtin(__builtin_bswap16)
#define _PVA_swap16(X) __builtin_bswap16(X)
#endif
#if __has_builtin(__builtin_bswap32)
#define _PVA_swap32(X) __builtin_bswap32(X)
#endif
#if __has_builtin(__builtin_bswap64)
#define _PVA_swap64(X) __builtin_bswap64(X)
#endif

#elif defined(__GNUC__) && ((__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=3))

#if (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=8)
#define _PVA_swap16(X) __builtin_bswap16(X)
#endif

#define _PVA_swap32(X) __builtin_bswap32(X)
#define _PVA_swap64(X) __builtin_bswap64(X)

#elif defined(_MSC_VER)

#define _PVA_swap16(X) _byteswap_ushort(X)
#define _PVA_swap32(X) _byteswap_ulong(X)
#define _PVA_swap64(X) _byteswap_uint64(X)

#endif

namespace epics {namespace pvData {

namespace detail {
template<typename T>
struct asInt {
    static EPICS_ALWAYS_INLINE T from(T v) { return v; }
    static EPICS_ALWAYS_INLINE T to(T v) { return v; }
};
template<>
struct asInt<float> {
    union pun {float f; uint32 i;};
    static EPICS_ALWAYS_INLINE float from(uint32 v) {
        pun P;
        P.i = v;
        return P.f;
    }
    static EPICS_ALWAYS_INLINE uint32 to(float v) {
        pun P;
        P.f = v;
        return P.i;
    }
};
template<>
struct asInt<double> {
    union pun {double f; uint64 i;};
    static EPICS_ALWAYS_INLINE double from(uint64 v) {
        pun P;
        P.i = v;
        return P.f;
    }
    static EPICS_ALWAYS_INLINE uint64 to(double v) {
        pun P;
        P.f = v;
        return P.i;
    }
};

template<int N>
struct swap; // no default
template<>
struct swap<1> {
    static EPICS_ALWAYS_INLINE uint8 op(uint8 v) { return v; }
};
template<>
struct swap<2> {
    static EPICS_ALWAYS_INLINE uint16 op(uint16 v) {
#ifdef _PVA_swap16
        return _PVA_swap16(v);
#else
        return (((v) >> 8) | ((v) << 8));
#endif
    }
};
template<>
struct swap<4> {
    static EPICS_ALWAYS_INLINE uint32 op(uint32 v) {
#ifdef _PVA_swap32
        return _PVA_swap32(v);
#else
        return ((((v) & 0xff000000) >> 24) |
                (((v) & 0x00ff0000) >>  8) |
                (((v) & 0x0000ff00) <<  8) |
                (((v) & 0x000000ff) << 24));
#endif
    }
};
template<>
struct swap<8> {
#ifdef _PVA_swap64
    static EPICS_ALWAYS_INLINE uint64 op(uint64 v) {
        return _PVA_swap64(v);
    }
#else
    static inline uint64 op(uint64 v) {
        return (((v) >> 56) | \
                (((v) >> 40) & 0x0000ff00) | \
                (((v) >> 24) & 0x00ff0000) | \
                (((v) >> 8)  & 0xff000000) | \
                (((v) << 8)  & ((uint64_t)0xff << 32)) | \
                (((v) << 24) & ((uint64_t)0xff << 40)) | \
                (((v) << 40) & ((uint64_t)0xff << 48)) | \
                (((v) << 56)));
    }
#endif
};

#undef _PVA_swap16
#undef _PVA_swap32
#undef _PVA_swap64

/* PVD serialization doesn't pay attention to alignement,
 * which some targets really care about and treat unaligned
 * access as a fault, or with a heavy penalty (~= to a syscall).
 *
 * For those targets,, we will have to live with the increase
 * in execution time and/or object code size of byte-wise copy.
 */

#ifdef _ARCH_PPC

template<typename T>
union alignu {
    T val;
    char bytes[sizeof(T)];
};

template<typename T>
EPICS_ALWAYS_INLINE void store_unaligned(char *buf, T val)
{
    alignu<T> A;
    A.val = val;
    for(unsigned i=0, N=sizeof(T); i<N; i++) {
        buf[i] = A.bytes[i];
    }
}

template<typename T>
EPICS_ALWAYS_INLINE T load_unaligned(const char *buf)
{
    alignu<T> A;
    for(unsigned i=0, N=sizeof(T); i<N; i++) {
        A.bytes[i] = buf[i];
    }
    return A.val;
}

#else /* alignement */

template<typename T>
EPICS_ALWAYS_INLINE void store_unaligned(char *buf, T val)
{
    *reinterpret_cast<T*>(buf) = val;
}

template<typename T>
EPICS_ALWAYS_INLINE T load_unaligned(const char *buf)
{
    return *reinterpret_cast<const T*>(buf);
}

#endif /* alignement */

} // namespace detail

//! Unconditional byte order swap.
//! defined for integer and floating point types
template<typename T>
EPICS_ALWAYS_INLINE T swap(T val)
{
    return detail::asInt<T>::from(
                detail::swap<sizeof(T)>::op(
                    detail::asInt<T>::to(val)));
}

#define is_aligned(POINTER, BYTE_COUNT) \
    (((std::size_t)(POINTER)) % (BYTE_COUNT) == 0)

#if defined (__GNUC__) && (__GNUC__ < 3)
#define GET(T) get((T*)0)
#else
#define GET(T) get<T>()
#endif

/**
 * @brief This class implements a Bytebuffer that is like the java.nio.ByteBuffer.
 * 
 * <p>A @c BitSet is not safe for multithreaded use without
 * external synchronization.
 *
 * Based on Java implementation.
 */
class ByteBuffer 
{
public:
    /**
     * Constructor.
     *
     * @param  size      The number of bytes.
     * @param  byteOrder The byte order.
     * Must be one of EPICS_BYTE_ORDER,EPICS_ENDIAN_LITTLE,EPICS_ENDIAN_BIG.
     */
    ByteBuffer(std::size_t size, int byteOrder = EPICS_BYTE_ORDER) :
        _buffer((char*)std::malloc(size)), _size(size),
        _reverseEndianess(byteOrder != EPICS_BYTE_ORDER),
        _reverseFloatEndianess(byteOrder != EPICS_FLOAT_WORD_ORDER),
        _wrapped(false)
    {
        if(!_buffer)
            throw std::bad_alloc();
        clear();
    }

    /**
     * Constructor for wrapping an existing buffer.
     * Given buffer will not be released by the ByteBuffer instance.
     * @param  buffer    Existing buffer.  May not be NULL.
     * @param  size      The number of bytes.
     * @param  byteOrder The byte order.
     * Must be one of EPICS_BYTE_ORDER,EPICS_ENDIAN_LITTLE,EPICS_ENDIAN_BIG.
     */
    ByteBuffer(char* buffer, std::size_t size, int byteOrder = EPICS_BYTE_ORDER) :
        _buffer(buffer), _size(size),
        _reverseEndianess(byteOrder != EPICS_BYTE_ORDER),
        _reverseFloatEndianess(byteOrder != EPICS_FLOAT_WORD_ORDER),
        _wrapped(true)
    {
        if(!_buffer)
            throw std::invalid_argument("ByteBuffer can't be constructed with NULL");
        clear();
    }
    /**
     * Destructor
     */
    ~ByteBuffer()
    {
        if (_buffer && !_wrapped) std::free(_buffer);
    }
    /**
     * Set the byte order.
     *
     * @param  byteOrder The byte order.
     * Must be one of EPICS_BYTE_ORDER,EPICS_ENDIAN_LITTLE,EPICS_ENDIAN_BIG,
     */
    inline void setEndianess(int byteOrder)
    {
        _reverseEndianess = (byteOrder != EPICS_BYTE_ORDER);
        _reverseFloatEndianess = (byteOrder != EPICS_FLOAT_WORD_ORDER);
    }
    /**
     * Get the raw buffer data.
     * @return the raw buffer data.
     */
    inline const char* getBuffer() const
    {
        return _buffer;
    }
    /**
     * Makes a buffer ready for a new sequence of channel-read or relative put operations:
     * It sets the limit to the capacity and the position to zero.
     */
    inline void clear()
    {
        _position = _buffer;
        _limit = _buffer + _size;
    }
    /**
     * Makes a buffer ready to read out previously written values.
     *
     * Typically _limit==_buffer+_size is the initial state, but this is not
     * required.
     *
     * V _buffer             V _position          V _limit         V _buffer+_size
     * |_______written_______|____uninitialized___|____allocated___|
     *
     * becomes
     *
     * V _buffer/_position   V _limit                              V _buffer+size
     * |_______written_______|________________allocated____________|
     */
    inline void flip() {
        _limit = _position;
        _position = _buffer;
    }
    /**
     * Makes a buffer ready for re-reading the data that it already contains:
     * It leaves the limit unchanged and sets the position to zero.
     *
     * Note that this may allow reading of uninitialized values.  flip() should be considered
     *
     * V _buffer             V _position          V _limit         V _buffer+_size
     * |_______written_______|____uninitialized___|____allocated___|
     *
     * becomes
     *
     * V _buffer/_position                        V _limit         V _buffer+size
     * |_______written_______|____uninitialized___|____allocated___|
     */
    inline void rewind() {
        _position = _buffer;
    }
    /**
     * Returns the current position.
     * @return The current position in the raw data.
     */
    inline std::size_t getPosition() const
    {
        return _position - _buffer;
    }
    /**
     * Sets the buffer position.
     * If the mark is defined and larger than the new position then it is discarded.
     *
     * @param  pos The offset into the raw buffer.
     * The new position value; must be no larger than the current limit
     */
    inline void setPosition(std::size_t pos)
    {
        assert(pos<=_size);
        _position = _buffer + pos;
        assert(_position<=_limit);
    }
    /**
     * Returns this buffer's limit. 
     *
     * @return The offset into the raw buffer.
     */
    inline std::size_t getLimit() const
    {
        return _limit - _buffer;
    }
    /**
     * Sets this buffer's limit.
     * If the position is larger than the new limit then it is set to the new limit.s
     * If the mark is defined and larger than the new limit then it is discarded. 
     *
     * @param  limit The new position value;
     * must be no larger than the current limit 
     */
    inline void setLimit(std::size_t limit)
    {
        assert(limit<=_size);
        _limit = _buffer + limit;
        assert(_position<=_limit);
    }
    /**
     * Returns the number of elements between the current position and the limit.
     *
     * @return The number of elements remaining in this buffer.
     */
    inline std::size_t getRemaining() const
    {
        return _limit - _position;
    }
    /**
     * Returns The size, i.e. capacity of the raw data buffer in bytes.
     *
     * @return The size of the raw data buffer.
     */
    EPICS_ALWAYS_INLINE std::size_t getSize() const
    {
        return _size;
    }
    /**
     * Put the value into the raw buffer as a byte stream in the current byte order.
     *
     * @param  value The value to be put into the byte buffer.
     */
    template<typename T>
    inline void put(T value);
    /**
     * Put the value into the raw buffer at the specified index  as a byte stream in the current byte order.
     *
     * @param  index Offset in the byte buffer.
     * @param  value The value to be put into the byte buffer.
     */
    template<typename T>
    inline void put(std::size_t index, T value) const;
    /**
     * Get the new object from  the byte buffer. The item MUST have type @c T.
     * The position is adjusted based on the type.
     *
     * @return The object.
     */
#if defined (__GNUC__) && (__GNUC__ < 3)
    template<typename T>
    inline T get(const T*);
#else
    template<typename T>
    inline T get();
#endif
    /**
     * Get the new object from  the byte buffer at the specified index.
     * The item MUST have type @c T.
     * The position is adjusted based on the type.
     *
     * @param index The location in the byte buffer.
     * @return The object.
     */
    template<typename T>
    inline T get(std::size_t index) const;
    /**
     * Put a sub-array of bytes into the byte buffer.
     * The position is increased by the count.
     *
     * @param  src        The source array.
     * @param  src_offset The starting position within src.
     * @param  count      The number of bytes to put into the byte buffer.
     *                    Must be less than getRemaining()
     */
    inline void put(const char* src, std::size_t src_offset, std::size_t count) {
        assert(count<=getRemaining());
        memcpy(_position, src + src_offset, count);
        _position += count;
    }
    /**
     * Get a sub-array of bytes from the byte buffer.
     * The position is increased by the count.
     *
     * @param  dest        The destination array.
     * @param  dest_offset The starting position within src.
     * @param  count       The number of bytes to put into the byte buffer.
     *                     Must be less than getRemaining()
     */
    inline void get(char* dest, std::size_t dest_offset, std::size_t count) {
        assert(count<=getRemaining());
        memcpy(dest + dest_offset, _position, count);
        _position += count;
    }
    /**
     * Put an array of type @c T into the byte buffer.
     * The position is adjusted.
     *
     * @param  values The input array.
     * @param  count  The number of elements.
     */
    template<typename T>
    inline void putArray(const T* values, std::size_t count);
    /**
     * Get an array of type @c T from the byte buffer.
     * The position is adjusted.
     *
     * @param  values The destination array.
     * @param  count  The number of elements.
     */
    template<typename T>
    inline void getArray(T* values, std::size_t count);
    /**
     * Is the byte order the EPICS_BYTE_ORDER
     * @return (false,true) if (is, is not) the EPICS_BYTE_ORDER
     */
    template<typename T>
    EPICS_ALWAYS_INLINE bool reverse() const
    {
        return sizeof(T)>1 && _reverseEndianess;
    }
    /**
     * Put a boolean value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putBoolean(  bool value) { put<  int8>(value ? 1 : 0); }
    /**
     * Put a byte value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putByte   (  int8 value) { put<  int8>(value); }
    /**
     * Put a short value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putShort  ( int16 value) { put< int16>(value); }
    /**
     * Put an int value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putInt    ( int32 value) { put< int32>(value); }
    /**
     * Put a long value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putLong   ( int64 value) { put< int64>(value); }
    /**
     * Put a float value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putFloat  ( float value) { put< float>(value); }
    /**
     * Put a double value into the byte buffer.
     *
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putDouble (double value) { put<double>(value); }

    /**
     * Put a boolean value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putBoolean(std::size_t  index,  bool value) { put<  int8>(index, value); }
    /**
     * Put a byte value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putByte   (std::size_t  index,  int8 value) { put<  int8>(index, value); }
    /**
     * Put a short value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putShort  (std::size_t  index, int16 value) { put< int16>(index, value); }
    /**
     * Put an int value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putInt    (std::size_t  index, int32 value) { put< int32>(index, value); }
    /**
     * Put a long value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putLong   (std::size_t  index, int64 value) { put< int64>(index, value); }
    /**
     * Put a float value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putFloat  (std::size_t  index, float value) { put< float>(index, value); }
    /**
     * Put a double value into the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer,
     * @param  value The value.
     */
    EPICS_ALWAYS_INLINE void putDouble (std::size_t  index, double value) { put<double>(index, value); }
    /**
     * Get a boolean value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE   bool getBoolean() { return GET(  int8) != 0; }
    /**
     * Get a byte value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE   int8 getByte   () { return GET(  int8); }
    /**
     * Get a short value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  int16 getShort  () { return GET( int16); }
    /**
     * Get a int value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  int32 getInt    () { return GET( int32); }
    /**
     * Get a long value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  int64 getLong   () { return GET( int64); }
    /**
     * Get a float value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  float getFloat  () { return GET( float); }
    /**
     * Get a double value from the byte buffer.
     *
     * @return The value.
     */
    EPICS_ALWAYS_INLINE double getDouble () { return GET(double); }
    /**
     * Get a boolean value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE   bool getBoolean(std::size_t  index) { return get<  int8>(index) != 0; }
    /**
     * Get a byte value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE   int8 getByte   (std::size_t  index) { return get<  int8>(index); }
    /**
     * Get a short value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  int16 getShort  (std::size_t  index) { return get< int16>(index); }
    /**
     * Get an int value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  int32 getInt    (std::size_t  index) { return get< int32>(index); }
    /**
     * Get a long value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  int64 getLong   (std::size_t  index) { return get< int64>(index); }
    /**
     * Get a float value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE  float getFloat  (std::size_t  index) { return get< float>(index); }
    /**
     * Get a boolean value from the byte buffer at the specified index.
     *
     * @param  index The offset in the byte buffer.
     * @return The value.
     */
    EPICS_ALWAYS_INLINE double getDouble (std::size_t  index) { return get<double>(index); }

    // TODO remove
    EPICS_ALWAYS_INLINE const char* getArray() const EPICS_DEPRECATED
    {
        return _buffer;
    }

    
private:
    char* const _buffer;
    char* _position;
    char* _limit;
    const std::size_t  _size;
    bool _reverseEndianess; 
    bool _reverseFloatEndianess;
    const bool _wrapped;
};

    template<>
    EPICS_ALWAYS_INLINE bool ByteBuffer::reverse<bool>() const
    {
        return false;
    }

    template<>
    EPICS_ALWAYS_INLINE bool ByteBuffer::reverse<int8>() const
    {
        return false;
    }

    template<>
    EPICS_ALWAYS_INLINE bool ByteBuffer::reverse<uint8>() const
    {
        return false;
    }

    template<>
    EPICS_ALWAYS_INLINE bool ByteBuffer::reverse<float>() const
    {
        return _reverseFloatEndianess;
    }

    template<>
    EPICS_ALWAYS_INLINE bool ByteBuffer::reverse<double>() const
    {
        return _reverseFloatEndianess;
    }

    // the following methods must come after the specialized reverse<>() methods to make pre-gcc3 happy

    template<typename T>
    inline void ByteBuffer::put(T value)
    {
        assert(sizeof(T)<=getRemaining());

        if(reverse<T>())
            value = swap<T>(value);

        detail::store_unaligned(_position, value);
        _position += sizeof(T);
    }

    template<typename T>
    inline void ByteBuffer::put(std::size_t index, T value) const
    {
        assert(_buffer+index<=_limit);

        if(reverse<T>())
            value = swap<T>(value);

        detail::store_unaligned(_buffer+index, value);
    }

#if defined (__GNUC__) && (__GNUC__ < 3)
    template<typename T>
    inline T ByteBuffer::get(const T*)
#else
    template<typename T>
    inline T ByteBuffer::get()
#endif
    {
        assert(sizeof(T)<=getRemaining());

        T value = detail::load_unaligned<T>(_position);
        _position += sizeof(T);

        if(reverse<T>())
            value = swap<T>(value);
        return value;
    }

    template<typename T>
    inline T ByteBuffer::get(std::size_t index) const
    {
        assert(_buffer+index<=_limit);

        T value = detail::load_unaligned<T>(_buffer + index);

        if(reverse<T>())
            value = swap<T>(value);
        return value;
    }

    template<typename T>
    inline void ByteBuffer::putArray(const T* values, std::size_t count)
    {
        size_t n = sizeof(T)*count; // bytes
        assert(n<=getRemaining());

        if (reverse<T>()) {
            for(std::size_t i=0; i<count; i++) {
                detail::store_unaligned(_position+i*sizeof(T), swap<T>(values[i]));
            }
        } else {
            memcpy(_position, values, n);
        }
        _position += n;
    }

    template<typename T>
    inline void ByteBuffer::getArray(T* values, std::size_t count)
    {
        size_t n = sizeof(T)*count; // bytes
        assert(n<=getRemaining());

        if (reverse<T>()) {
            for(std::size_t i=0; i<count; i++) {
                values[i] = swap<T>(detail::load_unaligned<T>(_position+i*sizeof(T)));
            }
        } else {
            memcpy(values, _position, n);
        }
        _position += n;
    }

}}
#endif  /* BYTEBUFFER_H */
