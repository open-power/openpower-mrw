/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwFSICommon.H $                                              */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014                             */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#ifndef MRWFSICOMMON_H
#define MRWFSICOMMON_H


/**
 * Utilities for parsing and walking FSI busses.
 */


#include <stdio.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <xmlutil.H>
#include <mrwParserCommon.H>



/**
 * An FSI bus segment, as seen in the card <fsi> XML
 */
class FSIBus : public BusSegment
{
public:

    FSIBus(XMLElement& i_element, Plug* i_plug) : BusSegment(i_element, i_plug) {};
};




/**
 * Represents an FSI master, contains the source from the <fsi> XML.
 */
class FSIMaster : public SystemEndpoint
{
public:
    FSIMaster(Endpoint& i_source, Plug* i_plug, bool i_cascadedMaster) :
        SystemEndpoint(i_source, i_plug),
        iv_cascadedMaster(i_cascadedMaster)
    {
    }
    
    bool cascadedMaster() { return iv_cascadedMaster; }
    std::string getEngine();
    std::string getPort();

private:
    
    bool iv_cascadedMaster;
    std::string iv_engine;
    std::string iv_port;
};




/**
 * Represents an FSI slave, contains the source from the <fsi> XML.
 */
class FSISlave : public SystemEndpoint
{
public:

    FSISlave(Endpoint& i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug) {};

    std::string getPort();

private:
    std::string iv_port;
    
};


//The next 2 classes are for being able to follow an FSI bus through all of
//the cards and connectors in a system.  

/**
 * Represents a connector along the FSI bus path
 */
class FSIConnector
{
public:
    FSIConnector(std::string i_connector, Plug* i_plug, bool i_cable = false) :
        iv_connector(i_connector), iv_plug(i_plug), iv_isCable(i_cable) {};
    
    std::string connector() { return iv_connector; }
    Plug* plug() { return iv_plug; }
    bool isCable() { return iv_isCable; }
    
private:
    std::string iv_connector;
    Plug* iv_plug;
    bool iv_isCable;
};


/**
 * Represents a cable along the path, instead of a connector
 */
class FSICable : public FSIConnector
{
public:
    FSICable(Cable* i_cable) : FSIConnector("", NULL, true),
    iv_cable(i_cable) {}

    Cable* cable() { return iv_cable; }
private:

    Cable* iv_cable;
};


/**
 * Represents an FSI bus from an FSI master out to an FSI slave, 
 * not going through any other masters
 */
class FSISingleHopBus
{
public:
    FSISingleHopBus(FSIMaster* i_master,
                 FSISlave* i_slave,
                 std::vector<FSIConnector*> & i_connectors) :
        iv_master(i_master), 
        iv_slave(i_slave),
        iv_connectors(i_connectors)
        {
        }
    
    FSIMaster* master() { return iv_master; }
    FSISlave* slave()  { return iv_slave; }    
    std::vector<FSIConnector*>& connectors() { return iv_connectors; }

    enum bus_endpoint_type_t
    {
        FSI_MASTER,
        FSI_SLAVE
    };

    std::string getCableConnPath(bus_endpoint_type_t i_type);

    Cable* getCable(); 

    std::string getCableName();

    
private:

    FSIMaster* iv_master;
    FSISlave*  iv_slave;
    std::vector<FSIConnector*> iv_connectors;
};




/**
 * Represents a complete system FSI bus from a master out to a slave,
 * containing all of the single hop busses along the way.
 * It can contain multiple single hop busses as the bus goes through
 * other masters and slaves along the way.
 */
class FSISystemBus
{
public:
    FSISystemBus(FSIMaster* i_master,
                 FSISlave* i_slave) :
        iv_master(i_master), 
        iv_slave(i_slave) 
    {
    }

    FSIMaster* master() { return iv_master; }
    FSISlave* slave()  { return iv_slave; }
    
    void addSingleHopBus(FSISingleHopBus* i_bus)
    {
        iv_singleHopBusses.push_back(i_bus);
    }
    
    void addSingleHopBusses(FSISystemBus* i_bus)
    {
        std::vector<FSISingleHopBus*> busses;
        std::vector<FSISingleHopBus*>::iterator b;
        
        busses = i_bus->singleHopBusses();
        for (b=busses.begin();b!=busses.end();++b)
        {
            addSingleHopBus(*b);
        }
    }
    
    std::vector<FSISingleHopBus*> & singleHopBusses() { return iv_singleHopBusses; }
    

    int numHops()
    {
        return iv_singleHopBusses.size();
    }

    std::string getCableConnPath(FSISingleHopBus::bus_endpoint_type_t i_type);

    std::string getCableName();

private:
    
    FSIMaster* iv_master;
    FSISlave* iv_slave;
    std::vector<FSISingleHopBus*> iv_singleHopBusses;
    
};




/**
 * Used to generate the list of all FSI buses in the system.  An FSISystem bus
 * can contain multiple segments through multiple masters, such as
 * FSP->processor1->processor2->centaur, when the master is an FSP.
 * In that case, aside from an object for that path there'd also be objects for the paths:
 *   FSP->processor1
 *   FSP->processor1->processor2
 *   processor1->processor2
 *   processor2->centaur
 *
 * For masters that are on processors, only single hop busses will be created.
 */
void mrwFsiMakeSystemBusses(std::vector<FSISystemBus*> & o_fsiSystemBusses);


/**
 * Returns the FSI segment of the firmware device path for the system bus
 * passed in.
 */
std::string mrwFsiGetDevicePathSubString(FSISystemBus* i_bus);


/**
 * Finds all the FSI busses that go out to a card and hence that CFAM hot plug event can be
 * used for presence detect for that FRU.
 */
void mrwFsiGetPresenceDetectBusses(const std::vector<FSISystemBus*> & i_allBusses,
                                   std::vector<FSISystemBus*> & o_presBusses);

#endif
