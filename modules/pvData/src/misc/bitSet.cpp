/* bitSet.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mes
 */
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include <epicsMutex.h>

#define epicsExportSharedSymbols
#include <pv/lock.h>
#include <pv/serializeHelper.h>
#include <pv/bitSet.h>

/*
 * BitSets are packed into arrays of "words."  Currently a word is
 * a long, which consists of 64 bits, requiring 6 address bits.
 * The choice of word size is determined purely by performance concerns.
 */
#define ADDRESS_BITS_PER_WORD 6u
#define BITS_PER_WORD  (1u << ADDRESS_BITS_PER_WORD)
#define BYTES_PER_WORD sizeof(uint64)
#define BIT_INDEX_MASK (BITS_PER_WORD - 1u)

/** Used to shift left or right for a partial word mask */
#define WORD_MASK ~((uint64)0)

// index of work containing this bit
#define WORD_INDEX(bitn) ((bitn)>>ADDRESS_BITS_PER_WORD)
// bit offset within word
#define WORD_OFFSET(bitn) ((bitn)&BIT_INDEX_MASK)

// the words vector should be size()d as small as posible,
// so the last word should always have a bit set when the set is not empty
#define CHECK_POST() assert(words.empty() || words.back()!=0)

namespace epics { namespace pvData {

    BitSet::shared_pointer BitSet::create(uint32 nbits)
    {
        return BitSet::shared_pointer(new BitSet(nbits));
    }

    BitSet::BitSet() {}

    BitSet::BitSet(uint32 nbits)
    {
        words.reserve((nbits == 0) ? 1 : WORD_INDEX(nbits-1) + 1);
    }

#if __cplusplus>=201103L
    BitSet::BitSet(std::initializer_list<uint32> I)
    {
        // optimistically guess that highest bit is last (not required)
        words.reserve((I.size() == 0) ? 1 : WORD_INDEX(*(I.end()-1)) + 1);

        for(uint32 idx : I)
        {
            set(idx);
        }
    }
#endif

    BitSet::~BitSet() {}

    void BitSet::recalculateWordsInUse() {
        // step back from the end to find the first non-zero element
        size_t nsize = words.size();
        for(; nsize; nsize--) {
            if(words[nsize-1])
                break;
        }
        words.resize(nsize);
        CHECK_POST();
    }

    void BitSet::ensureCapacity(uint32 wordsRequired) {
        words.resize(std::max(words.size(), (size_t)wordsRequired), 0);
    }

    void BitSet::expandTo(uint32 wordIndex) {
        ensureCapacity(wordIndex+1);
    }

    BitSet& BitSet::flip(uint32 bitIndex) {

        uint32 wordIdx = WORD_INDEX(bitIndex);
        expandTo(wordIdx);

        words[wordIdx] ^= (((uint64)1) << WORD_OFFSET(bitIndex));

        recalculateWordsInUse();
        return *this;
    }

    BitSet& BitSet::set(uint32 bitIndex) {

        uint32 wordIdx = WORD_INDEX(bitIndex);
        expandTo(wordIdx);

        words[wordIdx] |= (((uint64)1) << WORD_OFFSET(bitIndex));
        return *this;
    }

    BitSet& BitSet::clear(uint32 bitIndex) {

        uint32 wordIdx = WORD_INDEX(bitIndex);
        if (wordIdx < words.size()) {
            words[wordIdx] &= ~(((uint64)1) << WORD_OFFSET(bitIndex));

            recalculateWordsInUse();
        }
        return *this;
    }

    void BitSet::set(uint32 bitIndex, bool value) {
        if (value)
            set(bitIndex);
        else
            clear(bitIndex);
    }

    bool BitSet::get(uint32 bitIndex) const {
        uint32 wordIdx = WORD_INDEX(bitIndex);
        return ((wordIdx < words.size())
            && ((words[wordIdx] & (((uint64)1) << WORD_OFFSET(bitIndex))) != 0));
    }

    void BitSet::clear() {
        words.clear();
    }

    uint32 BitSet::numberOfTrailingZeros(uint64 i) {
        // HD, Figure 5-14
        uint32 x, y;
        if (i == 0) return 64;
        uint32 n = 63;
        y = (uint32)i; if (y != 0) { n = n -32; x = y; } else x = (uint32)(i>>32);
        y = x <<16; if (y != 0) { n = n -16; x = y; }
        y = x << 8; if (y != 0) { n = n - 8; x = y; }
        y = x << 4; if (y != 0) { n = n - 4; x = y; }
        y = x << 2; if (y != 0) { n = n - 2; x = y; }
        return n - ((x << 1) >> 31);
    }

    uint32 BitSet::bitCount(uint64 i) {
        // HD, Figure 5-14
        i = i - ((i >> 1) & 0x5555555555555555LL);
        i = (i & 0x3333333333333333LL) + ((i >> 2) & 0x3333333333333333LL);
        i = (i + (i >> 4)) & 0x0f0f0f0f0f0f0f0fLL;
        i = i + (i >> 8);
        i = i + (i >> 16);
        i = i + (i >> 32);
        return (uint32)(i & 0x7f);
     }

    int32 BitSet::nextSetBit(uint32 fromIndex) const {

        uint32 u = WORD_INDEX(fromIndex);
        if (u >= words.size())
            return -1;

        uint64 word = words[u] & (WORD_MASK << (fromIndex % BITS_PER_WORD));

        while (true) {
            if (word != 0)
                return (u * BITS_PER_WORD) + numberOfTrailingZeros(word);
            if (++u == words.size())
                return -1;
            word = words[u];
        }
    }

    int32 BitSet::nextClearBit(uint32 fromIndex) const {
        // Neither spec nor implementation handle bitsets of maximal length.

        uint32 u = WORD_INDEX(fromIndex);
        if (u >= words.size())
            return fromIndex;

        uint64 word = ~words[u] & (WORD_MASK << (fromIndex % BITS_PER_WORD));

        while (true) {
            if (word != 0)
                return (u * BITS_PER_WORD) + numberOfTrailingZeros(word);
            if (++u == words.size())
                return words.size() * BITS_PER_WORD;
            word = ~words[u];
        }
    }

    bool BitSet::isEmpty() const {
        return words.empty();
    }

    uint32 BitSet::cardinality() const {
        uint32 sum = 0;
        for (uint32 i = 0; i < words.size(); i++)
            sum += bitCount(words[i]);
        return sum;
    }

    uint32 BitSet::size() const {
        return words.size() * BITS_PER_WORD;
    }

    bool BitSet::logical_and(const BitSet& set) const
    {
        size_t nwords = std::min(words.size(), set.words.size());
        for(size_t i=0; i<nwords; i++) {
            if(words[i] & set.words[i])
                return true;
        }
        return false;
    }
    bool BitSet::logical_or(const BitSet& set) const
    {
        return !words.empty() || !set.words.empty();
    }

    BitSet& BitSet::operator&=(const BitSet& set) {
        // Check for self-assignment!
        if (this == &set) return *this;

        // the result length will be <= the shorter of the two inputs
        words.resize(std::min(words.size(), set.words.size()), 0);

        for(size_t i=0, e=words.size(); i<e; i++)
            words[i] &= set.words[i];

        recalculateWordsInUse();
        return *this;
    }

    BitSet& BitSet::operator|=(const BitSet& set) {
        // Check for self-assignment!
        if (this == &set) return *this;

        // result length will be the same as the longer of the two inputs
        words.resize(std::max(words.size(), set.words.size()), 0);

        // since we expand w/ zeros, then iterate using the size of the other vector
        for(size_t i=0, e=set.words.size(); i<e; i++)
            words[i] |= set.words[i];

        CHECK_POST();
        return *this;
    }

    BitSet& BitSet::operator^=(const BitSet& set) {
        // result length will <= the longer of the two inputs
        words.resize(std::max(words.size(), set.words.size()), 0);

        for(size_t i=0, e=set.words.size(); i<e; i++)
            words[i] ^= set.words[i];

        recalculateWordsInUse();
        return *this;
    }


    BitSet& BitSet::operator=(const BitSet &set) {
        // Check for self-assignment!
        if (this != &set) {
            words = set.words;
        }
        return *this;
    }

    void BitSet::swap(BitSet& set)
    {
        words.swap(set.words);
    }

    void BitSet::or_and(const BitSet& set1, const BitSet& set2) {

        const size_t andlen = std::min(set1.words.size(), set2.words.size());
        words.resize(std::max(words.size(), andlen), 0);

        // Perform logical AND on words in common
        for (uint32 i = 0; i < andlen; i++)
            words[i] |= (set1.words[i] & set2.words[i]);

        recalculateWordsInUse();
    }

    bool BitSet::operator==(const BitSet &set) const
    {
        if (this == &set)
            return true;

        if (words.size() != set.words.size())
            return false;

        // Check words in use by both BitSets
        for (uint32 i = 0; i < words.size(); i++)
            if (words[i] != set.words[i])
                return false;

        return true;
    }

    bool BitSet::operator!=(const BitSet &set) const
    {
        return !(*this == set);
    }

    void BitSet::serialize(ByteBuffer* buffer, SerializableControl* flusher) const {

        uint32 n = words.size();
        if (n == 0) {
            SerializeHelper::writeSize(0, buffer, flusher);
            return;
        }
        uint32 len = BYTES_PER_WORD * (n-1); // length excluding bits in the last word
        // count non-zero bytes in the last word
        for (uint64 x = words[n - 1]; x != 0; x >>= 8)
            len++;

        SerializeHelper::writeSize(len, buffer, flusher);
        flusher->ensureBuffer(len);

        n = len / 8;
        for (uint32 i = 0; i < n; i++)
            buffer->putLong(words[i]);

        if (n < words.size())
            for (uint64 x = words[words.size() - 1]; x != 0; x >>= 8)
                buffer->putByte((int8) (x & 0xff));
    }

    void BitSet::deserialize(ByteBuffer* buffer, DeserializableControl* control) {

        uint32 bytes = static_cast<uint32>(SerializeHelper::readSize(buffer, control)); // in bytes

        size_t wordsInUse = (bytes + 7) / BYTES_PER_WORD;
        words.resize(wordsInUse);

        if (wordsInUse == 0)
            return;

        control->ensureData(bytes);

        uint32 i = 0;
        uint32 longs = bytes / 8;
        while (i < longs)
            words[i++] = buffer->getLong();

        for (uint32 j = i; j < wordsInUse; j++)
            words[j] = 0;

        for (uint32 remaining = (bytes - longs * 8), j = 0; j < remaining; j++)
            words[i] |= (buffer->getByte() & 0xffLL) << (8 * j);

        recalculateWordsInUse(); // Sender shouldn't add extra zero bytes, but don't fail it it does
    }

    epicsShareExtern std::ostream& operator<<(std::ostream& o, const BitSet& b)
    {
        o << '{';
        int32 i = b.nextSetBit(0);
        if (i != -1) {
            o << i;
            for (i = b.nextSetBit(i+1); i >= 0; i = b.nextSetBit(i+1)) {
                int32 endOfRun = b.nextClearBit(i);
                do { o << ", " << i; } while (++i < endOfRun);
            }
        }
        o << '}';
        return o;
    }

}};
