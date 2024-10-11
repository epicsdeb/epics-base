/*status.cpp*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#define epicsExportSharedSymbols
#include <pv/epicsException.h>
#include <pv/serializeHelper.h>
#include <pv/status.h>

using std::string;

namespace epics { namespace pvData {

const char* Status::StatusTypeName[] = { "OK", "WARNING", "ERROR", "FATAL" };

Status Status::Ok;

Status::Status(StatusType type, string const & message) :
    m_statusType(type), m_message(message)
{
    if (type == STATUSTYPE_OK)
        throw std::invalid_argument("type == STATUSTYPE_OK");
}

Status::Status(StatusType type, string const & message, string const & stackDump) :
    m_statusType(type), m_message(message), m_stackDump(stackDump)
{
    if (type == STATUSTYPE_OK)
        throw std::invalid_argument("type == STATUSTYPE_OK");
}

void Status::maximize(const Status& o)
{
    if(m_statusType < o.m_statusType) {
        m_statusType = o.m_statusType;
        m_message = o.m_message;
        m_stackDump = o.m_stackDump;
    }
}

void Status::serialize(ByteBuffer *buffer, SerializableControl *flusher) const
{
    flusher->ensureBuffer(1);
    if (m_statusType == STATUSTYPE_OK)
    {
        // special code for okStatus (optimization)
        buffer->putByte((int8)-1);
    }
    else
    {
        buffer->putByte((int8)m_statusType);
        SerializeHelper::serializeString(m_message, buffer, flusher);
        SerializeHelper::serializeString(m_stackDump, buffer, flusher);
    }
}

void Status::deserialize(ByteBuffer *buffer, DeserializableControl *flusher)
{
    flusher->ensureData(1);
    int8 typeCode = buffer->getByte();
    if (typeCode == (int8)-1)
    {
       // in most of the cases status will be OK, we statistically optimize
       if (m_statusType != STATUSTYPE_OK)
       {
           m_statusType = STATUSTYPE_OK;
           m_message.clear();
           m_stackDump.clear();
       }
    }
    else
    {
        m_statusType = (StatusType)typeCode;
        m_message = SerializeHelper::deserializeString(buffer, flusher);
        m_stackDump = SerializeHelper::deserializeString(buffer, flusher);
    }
}

void Status::dump(std::ostream& o) const
{
    o << "Status [type=" << Status::StatusTypeName[m_statusType];
    if (!m_message.empty())
        o << ", message=" << m_message;
    if (!m_stackDump.empty())
        o << ", stackDump=" << std::endl << m_stackDump;
    o << ']';
}

}}
