
#include <pv/clientFactory.h>

#include "utilities.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

namespace {
struct Destroyer {
    pvd::Destroyable::shared_pointer D;
    Destroyer(const pvd::Destroyable::shared_pointer& d) : D(d) {}
    ~Destroyer() { if(D) D->destroy(); }
};

struct Releaser {
    pva::Monitor::shared_pointer& mon;
    pva::MonitorElementPtr& elem;
    Releaser(pva::Monitor::shared_pointer& m, pva::MonitorElementPtr& e)
        :mon(m), elem(e)
    {}
    ~Releaser() { mon->release(elem); }
};
} // namespace

int main(int argc, char *argv[])
{
    testPlan(0);
    try {
        if(argc<4) {
            std::cerr<<"Usage: "<<argv[0]<<" <channel> <fld1> <fld2> ...";
            return 1;
        }

        pvd::StructureConstPtr reqdesc(pvd::getFieldCreate()->createFieldBuilder()
                                       ->add("unused", pvd::pvBoolean)
                                       ->createStructure());

        pvd::PVStructurePtr pvreq(pvd::getPVDataCreate()->createPVStructure(reqdesc));

        pva::ClientFactory::start();

        pva::ChannelProvider::shared_pointer prov(pva::ChannelProviderRegistry::clients()->getProvider("pva"));
        if(!prov) throw std::runtime_error("No pva provider?!?");
        Destroyer prov_d(prov);

        TestChannelRequester::shared_pointer chreq(new TestChannelRequester);
        pva::Channel::shared_pointer chan(prov->createChannel(argv[1], chreq));
        Destroyer chan_d(chan);
        if(!chan) throw std::runtime_error("Failed to create channel");

        std::cout<<"Connecting...\n";
        if(!chreq->waitForConnect())
            throw std::runtime_error("Failed to connect channel");

        TestChannelMonitorRequester::shared_pointer monreq(new TestChannelMonitorRequester);
        pva::Monitor::shared_pointer mon(chan->createMonitor(monreq, pvreq));
        Destroyer mon_d(mon);
        if(!mon) throw std::runtime_error("Failed to create monitor");

        if(!monreq->waitForConnect())
            throw std::runtime_error("failed to connect monitor");

        if(!mon->start().isSuccess())
            throw std::runtime_error("failed to start monitor");

        std::cout<<"Wait for initial event...\n";
        while(monreq->waitForEvent()) {
            pva::MonitorElementPtr elem;
            while(!!(elem=mon->poll())) {
                Releaser R(mon, elem);

                pvd::PVStructurePtr A, B;
                A = elem->pvStructurePtr->getSubField<pvd::PVStructure>("a");
                B = elem->pvStructurePtr->getSubField<pvd::PVStructure>("b");
                if(!A || !B)
                    throw std::runtime_error("unexpected types");

                epicsUInt32 vA = A->getSubFieldT<pvd::PVScalar>("value")->getAs<pvd::uint32>(),
                            vB = B->getSubFieldT<pvd::PVScalar>("value")->getAs<pvd::uint32>();
                epicsUInt64 tsA = A->getSubFieldT<pvd::PVScalar>("timeStamp.secondsPastEpoch")->getAs<pvd::uint64>(),
                            tsB = B->getSubFieldT<pvd::PVScalar>("timeStamp.secondsPastEpoch")->getAs<pvd::uint64>();
                epicsUInt32 tnA = A->getSubFieldT<pvd::PVScalar>("timeStamp.nanoseconds")->getAs<pvd::uint32>(),
                            tnB = B->getSubFieldT<pvd::PVScalar>("timeStamp.nanoseconds")->getAs<pvd::uint32>();

                if(vA==vB && tsA==tsB && tnA==tnB) {
                    std::cout<<".";
                    continue;
                }
                std::cout<<"\nMismatch:\nA:\n"<<*A<<"B:\n"<<*B<<"\n";
            }
        }
        std::cout<<"Done\n";

        return 0;
    }catch(std::exception&e){
        PRINT_EXCEPTION(e);
        std::cerr<<"Error: "<<e.what()<<"\n";
        return 1;
    }
}
