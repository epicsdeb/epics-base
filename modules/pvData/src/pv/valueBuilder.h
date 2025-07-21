/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef VALUEBUILDER_H
#define VALUEBUILDER_H

#include <map>

#include <pv/templateMeta.h>
#include <pv/pvIntrospect.h>
#include <pv/sharedVector.h>

namespace epics{namespace pvData{

class PVStructure;

/** Incrementally define and initialize a PVStructure
 *
 * Equivalent to FieldBuilder with the added ability to assign initial values.
 *
 @code
 epics::pvData::PVStructurePtr val(epics::pvData::ValueBuilder()
                                   .add<pvInt>("intfld", 42)
                                   .addNested("sub")
                                    .add<pvString>("strfld", "testing")
                                   .endNested()
                                   .buildPVStructure());
 @endcode
 */
class epicsShareClass ValueBuilder
{
public:
    //! empty structure
    explicit ValueBuilder(const std::string& id=std::string());
    //! Clone existing definition and value
    explicit ValueBuilder(const PVStructure&);
    ~ValueBuilder();

    //! Add a scalar field with a given name and initial value
    template<ScalarType ENUM>
    FORCE_INLINE ValueBuilder& add(const std::string& name, typename meta::arg_type<typename ScalarTypeTraits<ENUM>::type>::type V)
    {
        _add(name, ENUM, &V);
        return *this;
    }

    //! Add a scalar array field
    template<class T>
    FORCE_INLINE ValueBuilder& add(const std::string& name, const shared_vector<const T>& V)
    {
        _add(name, V);
        return *this;
    }

    FORCE_INLINE ValueBuilder& add(const std::string& name, const PVStructure& V) {
        _add(name, V);
        return *this;
    }

    //! Start a sub-structure
    ValueBuilder& addNested(const std::string& name, Type type=structure, const std::string& id = std::string());
    //! End a sub-structure
    ValueBuilder& endNested();

    /** Complete building structure
     *
     * @note ValueBuilder may be re-used after calling buildPVStructure()
     */
    std::tr1::shared_ptr<PVStructure> buildPVStructure() const;

private:
    void _add(const std::string& name, ScalarType stype, const void *V);
    void _add(const std::string& name, const shared_vector<const void> &V);
    void _add(const std::string& name, const PVStructure& V);

    ValueBuilder(ValueBuilder*, const std::string &id = std::string());

    ValueBuilder * const parent;
    struct child;
    friend struct child;
    struct child_struct;
    friend struct child_struct;
    struct child_scalar_base;
    friend struct child_scalar_base;
    template <typename T> struct child_scalar;
    template <typename T> friend struct child_scalar;
    struct child_scalar_array;
    friend struct child_scalar_array;

    typedef std::map<std::string, child*> children_t;
    children_t children;
    std::string id;

    ValueBuilder(const ValueBuilder&);
    ValueBuilder& operator=(const ValueBuilder&);
};

}}// namespace epics::pvData

#endif // VALUEBUILDER_H
