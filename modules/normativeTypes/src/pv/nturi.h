/* nturi.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTURI_H
#define NTURI_H

#include <vector>
#include <string>

#ifdef epicsExportSharedSymbols
#   define nturiEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef nturiEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef nturiEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTURI;
typedef std::tr1::shared_ptr<NTURI> NTURIPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTURI.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTURIBuilder :
        public std::tr1::enable_shared_from_this<NTURIBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTURIBuilder);

        /**
         * Adds authority field to the NTURI.
         * @return this instance of <b>NTURIBuilder</b>.
         */
        shared_pointer addAuthority();

        /**
         * Adds extra <b>Scalar</b> of ScalarType pvString
         * to the query field of the type.
         * @param name name of the field.
         * @return this instance of <b>NTURIBuilder</b>.
         */
         shared_pointer addQueryString(std::string const & name);

        /**
         * Adds extra <b>Scalar</b> of ScalarType pvDouble
         * to the query field of the type.
         * @param name name of the field.
         * @return this instance of <b>NTURIBuilder</b>.
         */
         shared_pointer addQueryDouble(std::string const & name);

        /**
         * Adds extra <b>Scalar</b> of ScalarType pvInt
         * to the query field of the type.
         * @param name name of the field.
         * @return this instance of <b>NTURIBuilder</b>.
         */
         shared_pointer addQueryInt(std::string const & name);

        /**
         * Creates a <b>Structure</b> that represents NTURI.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTURI.
         * The returned PVStructure will have labels equal to the column names.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTURI</b> instance.
         * The returned NTURI will wrap a PVStructure which will have
         * labels equal to the column names.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTURI</b>.
         */
        NTURIPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTURIBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTURIBuilder();

        void reset();

        std::vector<std::string> queryFieldNames;
        std::vector<epics::pvData::ScalarType> queryTypes;

        bool authority;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTURI;
    };

}

typedef std::tr1::shared_ptr<detail::NTURIBuilder> NTURIBuilderPtr;



/**
 * @brief Convenience Class for NTURI
 *
 * @author dgh
 */
class epicsShareClass NTURI
{
public:
    POINTER_DEFINITIONS(NTURI);

    static const std::string URI;

    /**
     * Creates an NTURI wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTURI
     * and if so returns an NTURI which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTURI instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTScalar wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTScalar or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalar instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTScalar.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTScalar through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if (is not, is) a compatible NTScalar
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTURI.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTURI through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test.
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTURI.
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTURI.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTURI through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTURI
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTURI.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTURI through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTURI
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is a valid NTURI.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure (is not, is) a valid NTURI.
     */
    bool isValid();

    /**
     * Creates an NTURI builder instance.
     * @return builder instance.
     */
    static NTURIBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTURI() {}

    /**
     * Returns the PVStructure wrapped by this instance.
     * @return the PVStructure wrapped by this instance.
     */
    epics::pvData::PVStructurePtr getPVStructure() const;

    /**
     * Returns the scheme field.
     * @return the scheme field.
     */
    epics::pvData::PVStringPtr getScheme() const;

    /**
     * Returns the authority field.
     * @return the authority field or null if no such field.
     */
    epics::pvData::PVStringPtr getAuthority() const;

    /**
     * Returns the path field.
     * @return the path field.
     */
    epics::pvData::PVStringPtr getPath() const;

    /**
     * Returns the query field.
     * @return the query field or null if no such field.
     */
    epics::pvData::PVStructurePtr getQuery() const;

    /**
     * Returns the names of the query fields for the URI.
     * For each name, calling getQueryField should return
     * the query field, which should not be null.
     * @return The query field names.
     */
    epics::pvData::StringArray const & getQueryNames() const;

    /**
     * Returns the subfield of the query field with the specified name.
     * @param name the name of the subfield.
     * @return the the subfield of the query field or null if the field does not exist.
     */
    epics::pvData::PVFieldPtr getQueryField(std::string const & name) const;

    /**
     * Returns the subfield of the query field (parameter) with the specified
     * name and of a specified expected type (for example, PVString).
     * @tparam PVT the expected type of the subfield which should be
     *             be PVString, PVInt pr PVDouble.
     * @param name the subfield of the query field or null if the field does
     *             not exist or is not of the expected type.
     * @return The PVT field.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getQueryField(std::string const & name) const
    {
        epics::pvData::PVFieldPtr pvField = getQueryField(name);
        if (pvField.get())
            return std::tr1::dynamic_pointer_cast<PVT>(pvField);
        else
            return std::tr1::shared_ptr<PVT>();
    }

private:
    NTURI(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTURI;
    friend class detail::NTURIBuilder;
};

}}
#endif  /* NTURI_H */
