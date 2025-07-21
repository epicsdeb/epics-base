/*createRequest.h*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#ifndef CREATEREQUEST_H
#define CREATEREQUEST_H
#include <string>
#include <sstream>
#include <vector>

#include <pv/pvData.h>
#include <pv/lock.h>
#include <pv/bitSet.h>

#include <shareLib.h>

namespace epics { namespace pvData {

class BitSet;

/**
 * @brief Create pvRequest structure for Channel methods.
 *
 * Many methods of the Channel class of pvAccess have an
 * argument <b>PVStructurePtr const * pvRequest</b>.
 * This class provides a method that creates a valid pvRequest.
 *
 */
class epicsShareClass CreateRequest {
    public:
    POINTER_DEFINITIONS(CreateRequest);
    /**
     * Create s new instance of CreateRequest
     * @returns A shared pointer to the new instance.
     */
    static CreateRequest::shared_pointer create();
    ~CreateRequest() {};
    /**
    * Create a request structure for the create calls in Channel.
    * See the package overview documentation for details.
    * @param request The field request. See the package overview documentation for details.
    * @return The request PVStructure if a valid request was given.
    * If a NULL PVStructure is returned then getMessage will return
    * the reason.
    */
    PVStructure::shared_pointer createRequest(std::string const & request);
    /**
     * Get the error message of createRequest returns NULL
     * return the error message
     */
    std::string getMessage() {return message;}
protected:
    CreateRequest() {}
    std::string message;
};

/** Parse and build pvRequest structure.
 *
 @params request the Request string to be parsed.  eg. "field(value)"
 @returns The resulting strucuture.  Never NULL
 @throws std::exception for various parsing errors
 */
epicsShareExtern
PVStructure::shared_pointer createRequest(std::string const & request);

/** Helper for implementations of epics::pvAccess::ChannelProvider in interpreting the
 *  'field' substructure of a pvRequest.
 *  Copies between an internal (base) Structure, and a client/user visible (requested) Structure.
 *
 * @note PVRequestMapper is not re-entrant.  It is copyable and swap()able.
 */
class epicsShareClass PVRequestMapper {
public:
    enum mode_t {
        /** Masking mode.
         *
         * Requested Structure is identical to Base.
         * The 'field' substructure of the provided pvRequest is used to construct a BitSet
         * where the bits corresponding to the "selected" fields are set.  This mask can be
         * access via. requestedMask().  The copy* and mask* methods operate only
         * on "selected" fields.
         */
        Mask,
        /** Slice mode
         *
         * The Requested Structure is a strict sub-set of the Base Structure containing
         * those fields "selected" by the 'field' substructure of the provided pvRequest.
         */
        Slice,
    };

    PVRequestMapper();
    //! @see compute()
    PVRequestMapper(const PVStructure& base,
                    const PVStructure& pvRequest,
                    mode_t mode = Mask);

    //! return to state of default ctor
    void reset();

    //! @returns the Structure of the PVStructure previously passed to compute().  NULL if never computed()'d
    inline const StructureConstPtr& base() const { return typeBase; }
    //! @returns the Structure which is the selected sub-set of the base Structure.  NULL if never computed()'d
    inline const StructureConstPtr& requested() const { return typeRequested; }

    /** A mask of all fields in the base structure which are also in the requested structure,
     *  and any parent/structure "compress" bits.  eg. bit 0 is always set.
     *
     @code
     PVRequestMapper mapper(...);
     ...
     BitSet changed = ...; // a base changed mask
     bool wouldcopy = changed.logical_and(mapper.requestedMask());
     // wouldcopy==false means that copyBaseToRequested(..., changed, ...) would be a no-op
     @endcode
     *
     * eg. allows early detection of empty monitor updates.
     */
    inline const BitSet& requestedMask() const { return maskRequested; }

    //! @returns A new instance of the requested() Structure
    PVStructurePtr buildRequested() const;
    //! @returns A new instance of the base() Structure
    PVStructurePtr buildBase() const;

    /** (re)compute the selected subset of provided base structure.
     * @param base A full base structure.
     *             Must be "top level" (field offset zero).
     * @param pvRequest The user/client provided request modifier
     * @param mode Control how the mapping is constructed.  @see mode_t for a description of mapping modes.
     *
     * @post Updates warnings()
     * @throws std::runtime_error For errors involving invalid pvRequest
     * @throws std::logic_error if the provided base is not a "top level" PVStructure.
     */
    void compute(const PVStructure& base,
                 const PVStructure& pvRequest,
                 mode_t mode = Mask);

    //! After compute(), check if !warnings().empty()
    inline const std::string& warnings() const { return messages; }

    /** Copy field values from Base structure into Requested structure
     *
     * @param base An instance of the base Structure.  Field values are copied from it.
     *                Need not be the same instance passed to compute().
     * @param baseMask A bit mask selecting those base fields to copy.
     * @param request An instance of the requested() Structure.  Field values are copied to it.
     * @param requestMask A bit mask indicating which requested fields were copied.
     *                    BitSet::clear() is not called.
     */
    void copyBaseToRequested(
            const PVStructure& base,
            const BitSet& baseMask,
            PVStructure& request,
            BitSet& requestMask
    ) const;

    /** Copy field values into Base structure from Requested structure
     *
     * @param base An instance of the base Structure.  Field values are copied into it.
     *             Need not be the same instance passed to compute().
     * @param baseMask A bit mask indicating which base fields were copied.
     *                 BitSet::clear() is not called.
     * @param request An instance of the requested() Structure.  Field values are copied from it.
     * @param requestMask A bit mask selecting those requested fields to copy.
     */
    void copyBaseFromRequested(
            PVStructure& base,
            BitSet& baseMask,
            const PVStructure& request,
            const BitSet& requestMask
    ) const;

    //! Translate Base bit mask into requested bit mask.
    //! BitSet::clear() is not called.
    inline void maskBaseToRequested(
            const BitSet& baseMask,
            BitSet& requestMask
            ) const
    { _mapMask(baseMask, requestMask, false); }

    //! Translate requested bit mask into base bit mask.
    //! BitSet::clear() is not called.
    inline void maskBaseFromRequested(
            BitSet& baseMask,
            const BitSet& requestMask
            ) const
    { _mapMask(requestMask, baseMask, true); }

    //! Exchange contents of two mappers.  O(0) and never throws.
    void swap(PVRequestMapper& other);

private:
    bool _compute(const PVStructure& base, const PVStructure& pvReq,
                  FieldBuilderPtr& builder, bool keepids, unsigned depth);

    void _map(const PVStructure& src,
              const BitSet& maskSrc,
              PVStructure& dest,
              BitSet& maskDest,
              bool dir_r2b) const;
    void _mapMask(const BitSet& maskSrc,
                  BitSet& maskDest,
                  bool dir_r2b) const;

    StructureConstPtr typeBase, typeRequested;
    BitSet maskRequested;
    // Map between field offsets of base and requested Structures.
    // Include all fields, both leaf and sub-structure.
    struct Mapping {
        size_t to; // offset in destination Structure
        BitSet tomask,   // if !leaf these are the other bits in the destination mask to changed
               frommask; // if !leaf these are the other bits in the source mask to be copied
        bool valid; // only true in (sparse) base -> requested mapping
        bool leaf; // not a (sub)Structure?
        Mapping() :valid(false) {}
        Mapping(size_t to, bool leaf) :to(to), valid(true), leaf(leaf) {}
    };
    typedef std::vector<Mapping> mapping_t;
    mapping_t base2req, req2base;

    std::string messages;

    mutable BitSet scratch; // avoid temporary allocs.  (we aren't re-entrant!)
};

}}

#endif /* CREATEREQUEST_H */

