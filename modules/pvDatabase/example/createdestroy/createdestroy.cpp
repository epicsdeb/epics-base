/******************************************************************************
* This is modeled after a test program created by Bertrand Bauvir from the ITER Organization
******************************************************************************/
#include <iostream>
#include <epicsGetopt.h>

#include <pv/pvData.h>
#include <pv/pvDatabase.h>
#include <pv/serverContext.h>
#include <pv/channelProviderLocal.h>
#include <pva/client.h>
#include <epicsEvent.h>

// Local header files

// Constants

#define DEFAULT_RECORD_NAME "examplechannel"

using std::tr1::static_pointer_cast;

class Record : public ::epics::pvDatabase::PVRecord
{
public:
    std::shared_ptr<::epics::pvData::PVStructure> __pv;
    static std::shared_ptr<Record> create (std::string const & name, std::shared_ptr<::epics::pvData::PVStructure> const & pvstruct);
    Record (std::string const & name, std::shared_ptr<epics::pvData::PVStructure> const & pvstruct)
      : epics::pvDatabase::PVRecord(name, pvstruct) { __pv = pvstruct; };
    virtual void process (void);
};

std::shared_ptr<Record> Record::create (std::string const & name, std::shared_ptr<::epics::pvData::PVStructure> const & pvstruct)
{
  std::shared_ptr<Record> pvrecord (new Record (name, pvstruct));
  // Need to be explicitly called .. not part of the base constructor
  if(!pvrecord->init()) pvrecord.reset();
  return pvrecord;
}

void Record::process (void)
{
  PVRecord::process();
  std::string name = this->getRecordName();
  std::cout << this->getRecordName()
            << " process\n";
}

class MyMonitor
{
private:
    std::tr1::shared_ptr<::pvac::MonitorSync> monitor;
    MyMonitor(std::tr1::shared_ptr<::pvac::ClientChannel> const &channel)
    {
         monitor = std::tr1::shared_ptr<::pvac::MonitorSync>(new ::pvac::MonitorSync(channel->monitor()));
    }
public:
    static std::tr1::shared_ptr<MyMonitor> create(std::tr1::shared_ptr<::pvac::ClientChannel> const &channel)
    {
         return std::tr1::shared_ptr<MyMonitor>(new MyMonitor(channel));
    }
    void getData();
};

void MyMonitor::getData()
{
    while (true) {
       if(!monitor->wait(.001)) break;
       switch(monitor->event.event) {
            case pvac::MonitorEvent::Fail:
                std::cerr<<monitor->name()<<" : Error : "<<monitor->event.message<<"\n";
                return;
            case pvac::MonitorEvent::Cancel:
                std::cout<<monitor->name()<<" <Cancel>\n";
                return;
            case pvac::MonitorEvent::Disconnect:
                std::cout<<monitor->name()<<" <Disconnect>\n";
                return;
            case pvac::MonitorEvent::Data:
                while(monitor->poll()) {
                    std::cout<<monitor->name()<<" : "<<monitor->root;
                }
                if(monitor->complete()) {
                   return;
                }
        }
    }
}


int main (int argc, char** argv)
{
  int verbose = 0;
  unsigned loopctr = 0;
  unsigned pausectr = 0;
  bool allowExit = false;
  bool callRecord = false;
  bool callDatabase = false;
  int opt;
  while((opt = getopt(argc, argv, "v:ardh")) != -1) {
      switch(opt) {
          case 'v':
             verbose = std::stoi(optarg);
             break;
          case 'a' :
             allowExit = true;
             break;
          case 'r' :
             callRecord = true;
             break;
          case 'd' :
             callDatabase = true;
             break;
          case 'h':
             std::cout << " -v level -a -r -d -h \n";
             std::cout << "-r call pvRecord->remove -d call master->removeRecord\n";
             std::cout << "default\n";
             std::cout << "-v " << verbose
                  << " -a  false"
                  << " -d"
                  << "\n";
              return 0;
          default:
              std::cerr<<"Unknown argument: "<<opt<<"\n";
              return -1;
      }
  }
  if(!callRecord && !callDatabase) callDatabase = true;
  ::epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
  ::epics::pvDatabase::ChannelProviderLocalPtr channelProvider = epics::pvDatabase::getChannelProviderLocal();
  epics::pvAccess::ServerContext::shared_pointer context
      = epics::pvAccess::startPVAServer(epics::pvAccess::PVACCESS_ALL_PROVIDERS, 0, true, true);
  std::string startset("starting set of puts valuectr = ");

  while (true) {
      loopctr++;
      std::string name = DEFAULT_RECORD_NAME + std::to_string(loopctr);

      // Create record
      // Create record structure
      ::epics::pvData::FieldBuilderPtr builder = epics::pvData::getFieldCreate()->createFieldBuilder();
      builder->add("value", ::epics::pvData::pvULong);
      std::shared_ptr<::epics::pvData::PVStructure> pvstruct
          = ::epics::pvData::getPVDataCreate()->createPVStructure(builder->createStructure());
      std::shared_ptr<Record> pvrecord = Record::create(std::string(name), pvstruct);
      master->addRecord(pvrecord);
      pvrecord->setTraceLevel(verbose);
      // Start PVA (local) client
      std::tr1::shared_ptr<::pvac::ClientProvider> provider
          = std::tr1::shared_ptr<::pvac::ClientProvider>(new ::pvac::ClientProvider ("pva"));
      std::tr1::shared_ptr<::pvac::ClientChannel> channel
          = std::tr1::shared_ptr<::pvac::ClientChannel>(new ::pvac::ClientChannel (provider->connect(name)));
      std::tr1::shared_ptr<MyMonitor> mymonitor = MyMonitor::create(channel);
      unsigned valuectr = loopctr;
      std::cout << startset << loopctr << "\n";
      for (int ind=0; ind<100; ind++) {
          channel->put().set("value",valuectr++).exec();
          mymonitor->getData();
      }
      pausectr++;
      if(allowExit && pausectr>10) {
          pausectr = 0;
          std::cout << "Type exit to stop: \n";
          int c = std::cin.peek();  // peek character
          if ( c == EOF ) continue;
          std::string str;
          std::getline(std::cin,str);
          if(str.compare("exit")==0) break;
      }
      if(callRecord) {
std::cout << "callRecord\n";
          pvrecord->remove();
      }
      if(callDatabase) {
std::cout << "callDatabase\n";
          master->removeRecord(pvrecord);
      }
  }
  return (0);
}
