/* bitSet.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mse
 */
#ifndef BITSET_H
#define BITSET_H

#if __cplusplus>=201103L
#  include <initializer_list>
#endif

#include <vector>

#include <pv/pvType.h>
#include <pv/serialize.h>
#include <pv/sharedPtr.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

    class BitSet;
    typedef std::tr1::shared_ptr<BitSet> BitSetPtr;

    /**
     * @brief A vector of bits.
     *
     * This class implements a vector of bits that grows as needed. Each
     * component of the bit set has a @c bool value. The bits of a
     * @c BitSet are indexed by nonnegative integers. Individual
     * indexed bits can be examined, set, or cleared. One @c BitSet may
     * be used to modify the contents of another @c BitSet through
     * logical AND, logical inclusive OR, and logical exclusive OR
     * operations.
     *
     * <p>By default, all bits in the set initially have the value
     * @c false.
     *
     * <p>Every bit set has a current size, which is the number of bits
     * of space currently in use by the bit set. Note that the size is
     * related to the implementation of a bit set, so it may change with
     * implementation. The length of a bit set relates to logical length
     * of a bit set and is defined independently of implementation.
     *
     * <p>A @c BitSet is not safe for multithreaded use without external
     * synchronization.
     *
     * Based on Java implementation.
     *
     * @since 7.0.0 Many methods return BitSet& to facilite method chaining.
     */
    class epicsShareClass BitSet : public Serializable {
    public:
        POINTER_DEFINITIONS(BitSet);
        static BitSetPtr create(uint32 nbits);
        /**
         * Creates a new bit set. All bits are initially @c false.
         */
        BitSet();

        /**
         * Creates a bit set whose initial size is large enough to explicitly
         * represent bits with indices in the range @c 0 through
         * @c nbits-1. All bits are initially @c false.
         *
         * @param  nbits the initial size of the bit set
         */
        BitSet(uint32 nbits);

#if __cplusplus>=201103L
        /** Initialize from a list of indicies
         @code
         BitSet X({1, 5});
         assert(X.get(1) && X.get(5));
         @endcode
         */
        BitSet(std::initializer_list<uint32> I);
#endif

        /**
         * Destructor.
         */
        virtual ~BitSet();

        /**
         * Sets the bit at the specified index to the complement of its
         * current value.
         *
         * @param  bitIndex the index of the bit to flip
         */
        BitSet& flip(uint32 bitIndex);

        /**
         * Sets the bit at the specified index to @c true.
         *
         * @param  bitIndex a bit index
         */
        BitSet& set(uint32 bitIndex);

        /**
         * Sets the bit specified by the index to @c false.
         *
         * @param  bitIndex the index of the bit to be cleared
         */
        BitSet& clear(uint32 bitIndex);

        /**
         * Sets the bit at the specified index to the specified value.
         *
         * @param  bitIndex a bit index
         * @param  value a boolean value to set
         */
        void set(uint32 bitIndex, bool value);

        /**
         * Returns the value of the bit with the specified index. The value
         * is @c true if the bit with the index @c bitIndex is currently
         * set in this @c BitSet; otherwise, the result is @c false.
         *
         * @param  bitIndex   the bit index
         * @return the value of the bit with the specified index
         */
        bool get(uint32 bitIndex) const;

        /**
         * Sets all of the bits in this BitSet to @c false.
         */
        void clear();

        /**
         * Returns the index of the first bit that is set to @c true that
         * occurs on or after the specified starting index. If no such bit
         * exists then @c -1 is returned.
         *
         * <p>To iterate over the @c true bits in a @c BitSet,
         * use the following loop:
         *
         *  <pre> {@code
         * for (int32 i = bs.nextSetBit(0); i >= 0; i = bs.nextSetBit(i+1)) {
         *     // operate on index i here
         * }}</pre>
         *
         * @param  fromIndex the index to start checking from (inclusive)
         * @return the index of the next set bit, or @c -1 if there
         *         is no such bit
         */
        int32 nextSetBit(uint32 fromIndex) const;

        /**
         * Returns the index of the first bit that is set to @c false
         * that occurs on or after the specified starting index.
         *
         * @param  fromIndex the index to start checking from (inclusive)
         * @return the index of the next clear bit
         */
        int32 nextClearBit(uint32 fromIndex) const;

        /**
         * Returns true if this @c BitSet contains no bits that are set
         * to @c true.
         *
         * @return indicating whether this @c BitSet is empty
         */
        bool isEmpty() const;

        /**
         * Returns the number of bits set to @c true in this @c BitSet.
         *
         * @return the number of bits set to @c true in this @c BitSet
         */
        uint32 cardinality() const;

        /**
         * Returns the number of bits of space actually in use by this
         * @c BitSet to represent bit values.
         * The maximum element in the set is the size - 1st element.
         *
         * @return the number of bits currently in this bit set
         */
        uint32 size() const;

        //! Returns true if any bit is set in both *this and other
        bool logical_and(const BitSet& other) const;
        //! Returns true if any bit is set in both *this or other
        bool logical_or(const BitSet& other) const;

        /**
         * Performs a bitwise <b>AND</b> of this target bit set with the
         * argument bit set. This bit set is modified so that each bit in it
         * has the value @c true if and only if it both initially
         * had the value @c true and the corresponding bit in the
         * bit set argument also had the value @c true.
         *
         * @param set a bit set
         */
        BitSet& operator&=(const BitSet& set);

        /**
         * Performs a bitwise <b>OR</b> of this bit set with the bit set
         * argument. This bit set is modified so that a bit in it has the
         * value @c true if and only if it either already had the
         * value @c true or the corresponding bit in the bit set
         * argument has the value @c true.
         *
         * @param set a bit set
         */
        BitSet& operator|=(const BitSet& set);

        /**
         * Performs a bitwise <b>XOR</b> of this bit set with the bit set
         * argument. This bit set is modified so that a bit in it has the
         * value @c true if and only if one of the following
         * statements holds:
         * <ul>
         * <li>The bit initially has the value @c true, and the
         *     corresponding bit in the argument has the value @c false.
         * <li>The bit initially has the value @c false, and the
         *     corresponding bit in the argument has the value @c true.
         * </ul>
         *
         * @param  set a bit set
         */
        BitSet& operator^=(const BitSet& set);

        /**
          * Assignment operator.
          */
        BitSet& operator=(const BitSet &set);

        //! Swap contents
        void swap(BitSet& set);

        /**
         * Perform AND operation on <code>set1</code> and <code>set2</code>,
         * and OR on result and this instance.
         * @param set1
         * @param set2
         */
        void or_and(const BitSet& set1, const BitSet& set2);

        /**
         * Comparison operator.
         */
        bool operator==(const BitSet &set) const;

        bool operator!=(const BitSet &set) const;

        virtual void serialize(ByteBuffer *buffer,
            SerializableControl *flusher) const;
        virtual void deserialize(ByteBuffer *buffer,
            DeserializableControl *flusher);

    private:

        typedef std::vector<uint64> words_t;
        /** The internal field corresponding to the serialField "bits". */
        words_t words;

    private:
        /**
         * Sets the field wordsInUse to the logical size in words of the bit set.
         * WARNING: This method assumes that the number of words actually in use is
         * less than or equal to the current value of wordsInUse!
         */
        void recalculateWordsInUse();

        /**
         * Ensures that the BitSet can hold enough words.
         * @param wordsRequired the minimum acceptable number of words.
         */
        void ensureCapacity(uint32 wordsRequired);

        /**
         * Ensures that the BitSet can accommodate a given wordIndex,
         * temporarily violating the invariants.  The caller must
         * restore the invariants before returning to the user,
         * possibly using recalculateWordsInUse().
         * @param wordIndex the index to be accommodated.
         */
        void expandTo(uint32 wordIndex);

        /**
         * Returns the number of zero bits following the lowest-order ("rightmost")
         * one-bit in the two's complement binary representation of the specified
         * <tt>long</tt> value.  Returns 64 if the specified value has no
         * one-bits in its two's complement representation, in other words if it is
         * equal to zero.
         *
         * @return the number of zero bits following the lowest-order ("rightmost")
         *     one-bit in the two's complement binary representation of the
         *     specified <tt>long</tt> value, or 64 if the value is equal
         *     to zero.
         */
        static uint32 numberOfTrailingZeros(uint64 i);

        /**
         * Returns the number of one-bits in the two's complement binary
         * representation of the specified <tt>long</tt> value.  This function is
         * sometimes referred to as the <i>population count</i>.
         *
         * @return the number of one-bits in the two's complement binary
         *     representation of the specified <tt>long</tt> value.
         */
         static uint32 bitCount(uint64 i);

    };
    
    epicsShareExtern std::ostream& operator<<(std::ostream& o, const BitSet& b);

}}
#endif  /* BITSET_H */



