// IBM_PROLOG_BEGIN_TAG 
// This is an automatically generated prolog. 
//  
// fips830 src/mrw/xml/consumers/common/mrwProcSpiCommon.C 1.3 
//  
// IBM CONFIDENTIAL 
//  
// OBJECT CODE ONLY SOURCE MATERIALS 
//  
// COPYRIGHT International Business Machines Corp. 2012 
// All Rights Reserved 
//  
// The source code for this program is not published or otherwise 
// divested of its trade secrets, irrespective of what has been 
// deposited with the U.S. Copyright Office. 
//  
// IBM_PROLOG_END_TAG 
#include <mrwProcSpiCommon.H>


using namespace std;


//Globals
vector<SpiBus*> g_spiBusses;

//Globals defined by mrwParserCommon
extern vector<Plug*> g_plugs;



/**
 * Walks a SPI bus from the source unit to the endpoint unit.  Creates a SpiSystemBus object
 * for every full bus path.
 */
void mrwWalkSpiBus(SpiBus* i_sourceBus, endpointType i_type, SpiMaster *& i_master, vector<SpiSystemBus*> & o_busses)
{
    vector<busAndType_t<SpiBus> > busses;
    vector<busAndType_t<SpiBus> >::iterator b;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    SpiMaster* master = NULL;

    if (!source->unit().empty())
    {
        master = new SpiMaster(*source, i_sourceBus->plug());
    }


    if (!endpoint->unit().empty())
    {
        SpiSlave* slave = new SpiSlave(*endpoint, i_sourceBus->plug());

        SpiSystemBus* systemBus = new SpiSystemBus(i_master ? i_master : master, slave);
        o_busses.push_back(systemBus);

        if (mrwLogger::getDebugMode())
        {
            string m;
            m = "Creating SPI slave " + mrwUnitPath(slave->plug(), slave->source());
            mrwLogger::debug(m);
        }
    }
    else
    {
        //i_sourceBus may be connected to multiple busses on the other side
        //so need to get back all of them.  In this case when we do reach
        //the end devices they will all have the same master.

        mrwGetNextBusses<SpiBus>(i_sourceBus->plug(), *endpoint, busses);

        for (b=busses.begin();b!=busses.end();++b)
        {
            //walk the next part of the bus
            mrwWalkSpiBus(b->bus, b->type, i_master ? i_master : master, o_busses);
        }


    }


}


/**
 * Finds every SpiBus object segment that starts at a source processor unit and walks it out
 * to make the SpiSystemBus object.
 */
void processSpiBusses(vector<SpiSystemBus* > & o_spiBusses)
{
    vector<SpiBus*>::iterator bus;
    SpiMaster* master = NULL;

    for (bus=g_spiBusses.begin();bus!=g_spiBusses.end();++bus)
    {

        if ((!(*bus)->source().unit().empty()) &&
            (mrwGetPartType((*bus)->plug()->card(), (*bus)->source().id()) == "cpu"))
        {
            if (mrwLogger::getDebugMode())
            {
                string m;
                m = "Walking out SPI bus from processor " + mrwUnitPath((*bus)->plug(), (*bus)->source());
                mrwLogger::debug(m);
            }

            mrwWalkSpiBus(*bus, SOURCE, master, o_spiBusses);
        }

        else if ((!(*bus)->endpoint().unit().empty()) &&
                 (mrwGetPartType((*bus)->plug()->card(), (*bus)->endpoint().id()) == "cpu"))
        {
            if (mrwLogger::getDebugMode())
            {
                string m;
                m = "Walking out SPI bus from processor " + mrwUnitPath((*bus)->plug(), (*bus)->endpoint());
                mrwLogger::debug(m);
            }

            mrwWalkSpiBus(*bus, ENDPOINT, master, o_spiBusses);
        }
    }
}



void mrwProcSpiMakeBusses(vector<SpiSystemBus*> & o_spiBusses)
{

    mrwMakePlugs();

    mrwMakeBusses<SpiBus>(g_plugs, "spi", g_spiBusses);

    processSpiBusses(o_spiBusses);
}
