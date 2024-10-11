/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <sstream>

#include <epicsAssert.h>
#include <epicsTypes.h>
#include <epicsVersion.h>
#include <epicsConvert.h>
#include <epicsAssert.h>
#include <epicsMutex.h>
#include <epicsGuard.h>

#define epicsExportSharedSymbols
#include <pv/createRequest.h>
#include <pv/epicsException.h>
#include <pv/bitSet.h>

// Our arbitrary limit on pvRequest structure depth to bound stack usage during recursion
static const unsigned maxDepth = 5;

namespace epics{namespace pvData {

PVRequestMapper::PVRequestMapper() {}

PVRequestMapper::PVRequestMapper(const PVStructure &base,
                                 const PVStructure &pvRequest,
                                 mode_t mode)
{
    compute(base, pvRequest, mode);
}

PVStructurePtr PVRequestMapper::buildRequested() const
{
    if(!typeRequested)
        THROW_EXCEPTION2(std::logic_error, "No mapping compute()d");
    return typeRequested->build();
}

PVStructurePtr PVRequestMapper::buildBase() const
{
    if(!typeBase)
        THROW_EXCEPTION2(std::logic_error, "No mapping compute()d");
    return typeBase->build();
}

void PVRequestMapper::compute(const PVStructure &base,
                              const PVStructure &pvRequest,
                              mode_t mode)
{
    if(base.getFieldOffset()!=0)
        THROW_EXCEPTION2(std::logic_error, "Mapper must be used with top level PVStructure");

    bool ok = true;

    // we want to be transactional, which requires a second copy of everything.
    PVRequestMapper temp;

    // whether to preserve IDs of partial structures.
    bool keepids = false;
    PVScalar::const_shared_pointer pbp(pvRequest.getSubField<PVScalar>("record._options.keepIDs"));
    try {
        if(pbp) keepids = pbp->getAs<boolean>();
    }catch(std::runtime_error& e){
        std::ostringstream msg;
        msg<<"Can't parse keepIDs : '"<<e.what()<<"' ";
        temp.messages+=msg.str();
    }

    PVStructure::const_shared_pointer fields(pvRequest.getSubField<PVStructure>("field"));
    if(!fields || fields->getPVFields().empty()) {
        // not selection, or empty selection, treated as select all
        temp.typeBase = temp.typeRequested = base.getStructure();

        for(size_t i=1, N=base.getNextFieldOffset(); i<N; i++)
            temp.maskRequested.set(i);

    } else {
        FieldBuilderPtr builder(getFieldCreate()->createFieldBuilder());

        if(keepids)
            builder = builder->setId(base.getStructure()->getID());

        ok &= temp._compute(base, *fields, builder, keepids, 0); // fills in builder

        temp.typeBase = base.getStructure();
        temp.typeRequested = builder->createStructure();
        // possible that typeBase==typeRequested if all fields explicitly selected
    }

    if(mode==Mask) {
        // short circuit use of masked Structure, but keep maskRequested
        temp.typeRequested = temp.typeBase;
    }

    {
        PVStructurePtr proto(temp.typeRequested->build());

        // base -> request may be sparce mapping
        temp.base2req.resize(base.getNextFieldOffset());
        // request -> base is dense mapping
        temp.req2base.resize(proto->getNextFieldOffset());

        // special handling for whole structure mapping.  in part because getSubField(0) isn't allowed
        temp.base2req[0] = Mapping(0, false);
        temp.req2base[0] = Mapping(0, false);

        // Iterate prototype of requested to map with base field offsets.
        // which is handled as a special case below.
        // We also don't try to prevent redundant copies if both leaf and compress bits are set.
        for(size_t r=1, N=proto->getNextFieldOffset(); r<N; r++) {
            PVField::const_shared_pointer fld_req(proto->getSubFieldT(r)),
                    fld_base(base.getSubFieldT(fld_req->getFullName()));
            const size_t b = fld_base->getFieldOffset();

            if(!temp.requestedMask().get(b))
                continue;

            bool leaf = fld_base->getField()->getType()!=structure;

            // initialize mapping when our bit is set
            temp.base2req[b] = Mapping(r, leaf);
            temp.req2base[r] = Mapping(b, leaf);

            // add ourself to all "compress" bit mappings of enclosing structures
            for(const PVStructure *parent = fld_req->getParent(); parent; parent = parent->getParent()) {
                temp.req2base[parent->getFieldOffset()].tomask  .set(b);
                temp.req2base[parent->getFieldOffset()].frommask.set(r);
            }

            for(const PVStructure *parent = fld_base->getParent(); parent; parent = parent->getParent()) {
                temp.base2req[parent->getFieldOffset()].tomask  .set(r);
                temp.base2req[parent->getFieldOffset()].frommask.set(b);
            }
        }
    }

    temp.maskRequested.set(0);

    if(temp.maskRequested.nextSetBit(1)==-1) {
        ok = false;
        temp.messages+="Empty field selection";
    }

    if(!ok)
        throw std::runtime_error(temp.messages);

    swap(temp);
}

bool PVRequestMapper::_compute(const PVStructure& base, const PVStructure& pvReq,
                               FieldBuilderPtr& builder, bool keepids, unsigned depth)
{
    bool ok = true;
    const StringArray& reqNames = pvReq.getStructure()->getFieldNames();

    for(size_t i=0, N=reqNames.size(); i<N; i++) {
        // iterate through requested fields

        PVField::const_shared_pointer subtype(base.getSubField(reqNames[i]));
        const FieldConstPtr& subReq = pvReq.getStructure()->getFields()[i];

        if(subReq->getType()!=structure) {
            // pvRequest .field was not properly composed
            std::ostringstream msg;
            // not a great warning message as it doesn't distinguish 'a.value' from 'b.value',
            // but getFullName() whould prefix with 'field.', which would probably cause
            // more frequent confusion...
            msg<<"request invalid '"<<pvReq.getStructure()->getFieldNames()[i]<<"' ";
            messages+=msg.str();
            ok = false;

        } else if(!subtype) {
            // requested field does not actually exist in base
            std::ostringstream msg;
            msg<<"No field '"<<pvReq.getStructure()->getFieldNames()[i]<<"' ";
            messages+=msg.str();

        } else if(depth>=maxDepth // exceeds max recursion depth
                  || subtype->getField()->getType()!=structure // requested field is a leaf
                  || static_cast<const Structure&>(*subReq).getFieldNames().empty() // requests all sub-fields
                  )
        {
            // just add the whole thing
            builder = builder->add(reqNames[i], subtype->getField());
            for(size_t j=subtype->getFieldOffset(), N=subtype->getNextFieldOffset(); j<N; j++)
                maskRequested.set(j);

            if(subtype->getField()->getType()!=structure
                    && !static_cast<const Structure&>(*subReq).getFieldNames().empty())
            {
                // attempt to select below a leaf field
                std::ostringstream msg;
                msg<<"Leaf field '"<<pvReq.getFullName()<<"' ";
                messages+=msg.str();
            } else if(depth>=maxDepth) {
                std::ostringstream msg;
                msg<<"selection truncated at '"<<pvReq.getFullName()<<"' ";
                messages+=msg.str();
            }

        } else {
            // recurse into sub-structure
            const PVStructure& substruct = static_cast<const PVStructure&>(*subtype);

            builder = builder->addNestedStructure(reqNames[i]);
            maskRequested.set(substruct.getFieldOffset());

            if(keepids)
                builder = builder->setId(substruct.getStructure()->getID());

            _compute(substruct,
                     static_cast<const PVStructure&>(*pvReq.getPVFields()[i]),
                     builder, keepids, depth+1u);

            builder = builder->endNested();
        }
    }
    return ok;
}

void PVRequestMapper::copyBaseToRequested(
        const PVStructure& base,
        const BitSet& baseMask,
        PVStructure& request,
        BitSet& requestMask
) const {
    assert(base.getStructure()==typeBase);
    assert(request.getStructure()==typeRequested);
    _map(base, baseMask, request, requestMask, false);
}

void PVRequestMapper::copyBaseFromRequested(
        PVStructure& base,
        BitSet& baseMask,
        const PVStructure& request,
        const BitSet& requestMask
) const {
    assert(base.getStructure()==typeBase);
    assert(request.getStructure()==typeRequested);
    _map(request, requestMask, base, baseMask, true);
}

void PVRequestMapper::_map(const PVStructure& src, const BitSet& maskSrc,
                           PVStructure& dest, BitSet& maskDest,
                           bool dir_r2b) const
{
    {
        scratch = maskSrc;
        const mapping_t& map = dir_r2b ? req2base : base2req;

        assert(map.size()==src.getNumberFields());

        for(int32 i=scratch.nextSetBit(0), N=map.size(); i>=0 && i<N; i=scratch.nextSetBit(i+1)) {
            const Mapping& M = map[i];
            if(!M.valid) {
                assert(!dir_r2b); // only base -> requested mapping can have holes

            } else if(M.leaf) {
                // just copy
                dest.getSubFieldT(M.to)->copy(*src.getSubFieldT(i));
                maskDest.set(M.to);

            } else {
                // set bits of all sub-fields (in requested structure)
                // these indicies are always >i
                scratch |= M.frommask;

                // we will also set the individual bits, but if a compress bit is set in the input,
                // then set the corresponding bit in the output.
                maskDest.set(M.to);
            }
        }
    }
}

void PVRequestMapper::_mapMask(const BitSet& maskSrc,
                               BitSet& maskDest,
                               bool dir_r2b) const
{
    if(maskSrc.isEmpty()) {
        // no-op

    } else {
        const mapping_t& map = dir_r2b ? req2base : base2req;

        for(int32 i=maskSrc.nextSetBit(0), N=map.size(); i>=0 && i<N; i=maskSrc.nextSetBit(i+1)) {
            const Mapping& M = map[i];
            if(!M.valid) {
                assert(!dir_r2b); // only base -> requested mapping can have holes

            } else {
                maskDest.set(M.to);

                if(!M.leaf) {
                    maskDest |= M.tomask;
                }
            }
        }
    }

}

void PVRequestMapper::swap(PVRequestMapper& other)
{
    typeBase.swap(other.typeBase);
    typeRequested.swap(other.typeRequested);
    maskRequested.swap(other.maskRequested);
    base2req.swap(other.base2req);
    req2base.swap(other.req2base);
    messages.swap(other.messages);
    scratch.swap(other.scratch); // paranoia
}

void PVRequestMapper::reset()
{
    typeBase.reset();
    typeRequested.reset();
    maskRequested.clear();
    base2req.clear();
    req2base.clear();
    messages.clear();
    scratch.clear(); // paranoia
}

}} //namespace epics::pvData
