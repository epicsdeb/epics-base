/* status.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mse
 */
#ifndef STATUS_H
#define STATUS_H

#include <ostream>

#include <pv/serialize.h>
#include <pv/byteBuffer.h>
#include <pv/sharedPtr.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

        /**
         * @brief Status.
         *
         * This is a class for returning status to clients.
         * @author mse
         */
        class epicsShareClass Status : public epics::pvData::Serializable {
            public:
                POINTER_DEFINITIONS(Status);
                /**
                 * Status type enum.
                 */
                enum StatusType { 
                    /** Operation completed successfully. */
                    STATUSTYPE_OK,
                    /** Operation completed successfully, but there is a warning message. */
                    STATUSTYPE_WARNING,
                    /** Operation failed due to an error. */
                    STATUSTYPE_ERROR,
                    /** Operation failed due to an unexpected error. */
                    STATUSTYPE_FATAL
                };
                
                static const char* StatusTypeName[];
                
                static Status Ok;

            static inline Status warn(const std::string& m) { return Status(STATUSTYPE_WARNING, m); }
            static inline Status error(const std::string& m) { return Status(STATUSTYPE_ERROR, m); }
            static inline Status fatal(const std::string& m) { return Status(STATUSTYPE_FATAL, m); }

            /**
             * Creates OK status; STATUSTYPE_OK, empty message and stackDump.
             */
            Status() :m_statusType(STATUSTYPE_OK) {}
                
                /**
                 * Create non-OK status.
                 */
            Status(StatusType type, std::string const & message);
        
            /**
             * Create non-OK status.
             */
            Status(StatusType type, std::string const & message, std::string const & stackDump);
        
            virtual ~Status() {}
            
            /**
             * Get status type.
             * @return status type, non-<code>null</code>.
             */
            inline StatusType getType() const { return m_statusType; }

            /**
             * Get error message describing an error. Required if error status.
             * @return error message.
             */
            inline const std::string& getMessage() const { return m_message; }

            /**
             * Get stack dump where error (exception) happened. Optional.
             * @return stack dump.
             */
            inline const std::string& getStackDump() const { return m_stackDump; }

            /**
             * Convenient OK test. Same as <code>(getType() == StatusType.OK)</code>.
             * NOTE: this will return <code>false</code> on WARNING message although operation succeeded.
             * To check if operation succeeded, use <code>isSuccess</code>.
             * @return OK status.
             * @see #isSuccess()
             */
            inline bool isOK() const {
                return (m_statusType == STATUSTYPE_OK);
            }

            /**
             * Check if operation succeeded (OK or WARNING).
             * @return operation success status.
             */
            inline bool isSuccess() const {
                return (m_statusType == STATUSTYPE_OK || m_statusType == STATUSTYPE_WARNING);
            }

#if __cplusplus>=201103L
            FORCE_INLINE explicit operator bool() const {
                return isSuccess();
            }
#else
        private:
            typedef bool (Status::*truth_type)() const;
        public:
            FORCE_INLINE operator truth_type() const {
                return isSuccess() ? &Status::isSuccess : 0;
            }
#endif

            /** override this Status if the other has higher StatusType
             @code
             Status ret;
             ret |= call1();
             if(ret)
                ret |= call2();
             return ret;
             @endcode
             */
            void maximize(const Status& o);

            //! short hand for "this->maximize(o)"
            FORCE_INLINE Status& operator|=(const Status& o) {
                maximize(o);
                return *this;
            }

            void serialize(ByteBuffer *buffer, SerializableControl *flusher) const;
            void deserialize(ByteBuffer *buffer, DeserializableControl *flusher);
    
            void dump(std::ostream& o) const;

            private:
                        
            StatusType m_statusType;
            std::string m_message;
            std::string m_stackDump;

        };

        FORCE_INLINE std::ostream& operator<<(std::ostream& o, const Status& status) {
            status.dump(o);
            return o;
        }

        FORCE_INLINE std::ostream& operator<<(std::ostream& o, const Status::StatusType& statusType) {
            o << Status::StatusTypeName[statusType];
            return o;
        }

}}
#endif  /* STATUS_H */
