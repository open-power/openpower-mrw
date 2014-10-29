// IBM_PROLOG_BEGIN_TAG 
// This is an automatically generated prolog. 
//  
// fips830 src/mrw/xml/consumers/common/mrwProcSpiCommon.H 1.3 
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
#include <mrwParserCommon.H>

/**
 *
 */


/**
* An SPI bus segment, as seen in the card XML
*/
class SpiBus : public BusSegment
{
public:
    SpiBus(XMLElement& i_element, Plug* i_plug) :
        BusSegment(i_element, i_plug) {}

};


/**
 * Represents an SPI master, i.e. the processor
 */
class SpiMaster : public SystemEndpoint
{
public:

    SpiMaster(Endpoint & i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug)
    {
    }

};


/**
 * Represents an SPI slave
 */
class SpiSlave : public SystemEndpoint
{
public:

    SpiSlave(Endpoint & i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug)
    {
    }

};

/**
 * Represents the whole connection, from master to slave
 */
class SpiSystemBus
{
public:
    SpiSystemBus(SpiMaster* i_master, SpiSlave* i_slave) :
        iv_master(i_master), iv_slave(i_slave) {};

    SpiMaster* master() { return iv_master; }
    SpiSlave* slave() { return iv_slave; }


private:

    SpiMaster* iv_master;
    SpiSlave*  iv_slave;

};


/**
 * Walks a SPI bus from the source unit to the endpoint unit.  Creates a SpiSystemBus object
 * for every full bus path.
 */
void mrwWalkSpiBus(SpiBus* i_sourceBus, endpointType i_type, SpiMaster *& i_master, std::vector<SpiSystemBus*> & o_busses);

/**
 * Creates SpiSystemBus objects for every processor->slave SPI connection
 */
void mrwProcSpiMakeBusses(std::vector<SpiSystemBus*> & o_spiBusses);

