/* serialize.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <epicsTypes.h>

#include <pv/byteBuffer.h>
#include <pv/sharedPtr.h>

#include <shareLib.h>

#if defined(PVD_INTERNAL)
#  define PVD_DEPRECATED(msg)
#elif __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 5
#  define PVD_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#  define PVD_DEPRECATED(msg) EPICS_DEPRECATED
#endif

namespace epics { namespace pvData { 

    class SerializableControl;
    class DeserializableControl;
    class Serializable;
    class BitSetSerializable;
    class SerializableArray;
    class BitSet;
    class Field;

    /**
     * @brief Callback class for serialization.
     *
     * This must be provided by code that calls serialize.
     */
    class epicsShareClass SerializableControl {
    public:
        /**
         * Destructor.
         */
        virtual ~SerializableControl(){}
        /**
         * Done with this buffer. Flush it.
         */
        virtual void flushSerializeBuffer() =0;
        /**
         * Make sure buffer has at least size bytes remaining.
         * If not flush existing buffer and provide a new one.
         * @param size The number of bytes.
         */
        virtual void ensureBuffer(std::size_t size) =0;
        virtual void alignBuffer(std::size_t alignment) PVD_DEPRECATED("Deprecated for lack of use") {}
        /**
         * Method for serializing primitive array data.
         * Hook for supplying custom serialization implementation.
         * The serialization implementation need not be provided.
         * Returns true if method performs serialization, false otherwise.
         * This should only be used for arrays of primitive types,
         * i. e. boolean, byte,..., double.
         * It cannot be called for string, structure, or union arrays.
         * @param existingBuffer the existing buffer from the caller.
         * @param toSerialize location of data to be put into buffer.
         * @param elementCount number of elements.
         * @param elementSize element size.
         * @returns true if serialization performed, else false.
         */
        virtual bool directSerialize(
            ByteBuffer *existingBuffer,
            const char* toSerialize,
            std::size_t elementCount,
            std::size_t elementSize) = 0;
        /**
         * serialize via cache
         * @param field instance to be serialized
         * @param buffer buffer to be serialized to
         */
        virtual void cachedSerialize(
            std::tr1::shared_ptr<const Field> const & field,
            ByteBuffer* buffer) = 0;
    };

    /**
     * @brief Callback class for deserialization.
     *
     * This must be provided by code that calls deserialize.
     */
    class epicsShareClass DeserializableControl {
    public:
        /**
         * Destructor.
         */
        virtual ~DeserializableControl(){}
        /**
         * Helper method.
         * Ensures specified size of bytes, provides it if necessary.
         * @param size The number of bytes.
         */
        virtual void ensureData(std::size_t size) =0;
        // Deprecated for lack of use
        virtual void alignData(std::size_t alignment) PVD_DEPRECATED("Deprecated for lack of use") {};
        /**
         * Method for deserializing array data.
         * Hook for supplying custom deserialization implementation.
         * The deserialization implementation need not be provided.
         * Returns true if method performs deserialization, false otherwise.
         * This should only be used for arrays of primitive types.
         * i.e. boolean, byte,..., double.
         * It cannot be called for string, structure, or union arrays.
         * @param existingBuffer the existing buffer from the caller.
         * @param deserializeTo location of data.
         * @param elementCount number of elements.
         * @param elementSize element size.
         * @returns true if deserialization performed, else false.
         */
        virtual bool directDeserialize(
            ByteBuffer *existingBuffer,
            char* deserializeTo,
            std::size_t elementCount,
            std::size_t elementSize) = 0;
        /**
         * deserialize via cache
         * @param buffer buffer to be deserialized from
         */
        virtual std::tr1::shared_ptr<const Field> cachedDeserialize(
            ByteBuffer* buffer) = 0;
    };

    /**
     * @brief Base class for serialization.
     *
     */
    class epicsShareClass Serializable {
    public:
        /**
         * Destructor.
         */
        virtual ~Serializable(){}
        /**
         * Serialize field into given buffer.
         * @param buffer serialization buffer.
         * @param flusher flush interface.
         */
        virtual void serialize(ByteBuffer *buffer,
            SerializableControl *flusher) const = 0;
        /**
         * Deserialize buffer.
         * @param buffer serialization buffer.
         * @param flusher deserialization control.
         */
        virtual void deserialize(ByteBuffer *buffer,
            DeserializableControl *flusher) = 0;
    };

    /**
     * @brief Push serialize and append to the provided byte vector.
     * No caching is done.  Only complete serialization.
     *
     * @param S A Serializable object
     * @param byteOrder Byte order to write (EPICS_ENDIAN_LITTLE or EPICS_ENDIAN_BIG)
     * @param out The output vector.  Results are appended
     */
    void epicsShareFunc serializeToVector(const Serializable *S,
                           int byteOrder,
                           std::vector<epicsUInt8>& out);

    /**
     * @brief deserializeFromBuffer Deserialize into S from provided vector
     * @param S A Serializeable object.  The current contents will be replaced
     * @param in The input buffer (byte order of this buffer is used)
     * @throws std::logic_error if input buffer is too small.  State of S is then undefined.
     */
    void epicsShareFunc deserializeFromBuffer(Serializable *S,
                               ByteBuffer& in);

    /**
     * @brief deserializeFromBuffer Deserialize into S from provided vector
     * @param S A Serializeable object.  The current contents will be replaced
     * @param byteOrder Byte order to write (EPICS_ENDIAN_LITTLE or EPICS_ENDIAN_BIG)
     * @param in The input vector
     * @throws std::logic_error if input buffer is too small.  State of S is then undefined.
     */
    inline void deserializeFromVector(Serializable *S,
                                      int byteOrder,
                                      const std::vector<epicsUInt8>& in)
    {
        ByteBuffer B((char*)&in[0], in.size(), byteOrder); // we promise not the modify 'in'
        deserializeFromBuffer(S, B);
    }

    /**
     * @brief Class for serializing bitSets.
     *
     */
    class epicsShareClass BitSetSerializable {
    public:
        /**
         * Destructor.
         */
        virtual ~BitSetSerializable(){}
        /**
         * Serialize field into given buffer.
         * @param buffer serialization buffer.
         * @param flusher flush interface.
         * @param bitSet The bitSet to serialize.
         */
        virtual void serialize(ByteBuffer *buffer,
            SerializableControl *flusher,BitSet *bitSet) const = 0;
        /**
         * Deserialize buffer.
         * @param buffer serialization buffer.
         * @param flusher deserialization control.
         * @param bitSet The bitSet to deserialize.
         */
        virtual void deserialize(ByteBuffer *buffer,
            DeserializableControl *flusher,BitSet *bitSet) = 0;
    };


    /**
     * @brief Class for serializing arrays.
     *
     */
    class epicsShareClass SerializableArray : public virtual Serializable {
    public:
        /**
         * Destructor.
         */
        virtual ~SerializableArray(){}
        using Serializable::serialize;
        /**
         * Serialize field into given buffer.
         * @param buffer serialization buffer.
         * @param flusher flush interface.
         * @param offset offset in elements.
         * @param count number of elements
         */
        virtual void serialize(
            ByteBuffer *buffer,
            SerializableControl *flusher,
            std::size_t offset,
            std::size_t count) const = 0;
    };

}}
#endif  /* SERIALIZE_H */
