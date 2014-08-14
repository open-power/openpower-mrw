/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwFSICommon.C $                                              */
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
#include <mrwFSICommon.H>
#include <algorithm>
#include <sstream>

using namespace std;

//#define MRWDEBUG

//globals
vector<FSIBus*> g_fsiBusses;
vector<FSISingleHopBus*> g_singleHopBusses;


//mrwParserCommon defines these globals
extern vector<Plug*> g_plugs;



/**
 * Get the cable connector instance path for this bus, if there is one
 */
string FSISystemBus::getCableConnPath(FSISingleHopBus::bus_endpoint_type_t i_type)
{
    vector<FSISingleHopBus*>::iterator bus;
    string path;

    //Find the single hop bus with the cable and use that to get the cable
    //connector instance path.  Assumes there is just one bus with a cable.
    for (bus=iv_singleHopBusses.begin();bus!=iv_singleHopBusses.end();++bus)
    {
        if ((*bus)->getCable())
        {
            path = (*bus)->getCableConnPath(i_type);
            break;
        }
    }

    return path;
}

/**
 * Get the cable name from the single hop bus with the cable 
 */
string FSISystemBus::getCableName()
{
    vector<FSISingleHopBus*>::iterator bus;
    string name;

    //Again, assumes just 1 single hop bus has a cable.  this has always been
    //a valid assumption.
    for (bus=iv_singleHopBusses.begin();bus!=iv_singleHopBusses.end();++bus)
    {
        if ((*bus)->getCable())
        {
            name = (*bus)->getCableName();
            break;
        }
    }

    return name;
}


/**
 * Returns the cable connector instance path for the side of the cable specified
 * by i_type.  If no cable, returns "".
 */
string FSISingleHopBus::getCableConnPath(FSISingleHopBus::bus_endpoint_type_t i_type)
{
    string path;
    Cable* cable = getCable();


    //There's a hard way to do this, and an easy way.  Knowing that so far all
    //cables have been used to cross assemblies, the easy way is to just check for 
    //which end of the cable has an assembly that matches either the FSI source's 
    //or destination's. The hard way is to keep track of the cable connector
    //instance in the bus path while walking the bus. We'll do the easy way.

    if (cable)
    {
        string cableConnId, cableAssembly, endpointAssembly;
        SystemEndpoint* endpoint;
        Plug* cablePlug = NULL;

        if (FSI_MASTER == i_type) endpoint = iv_master;
        else endpoint = iv_slave;

        endpointAssembly = endpoint->plug()->assembly().id + '-' + endpoint->plug()->assembly().position;

        //First check if the source side of the cable has endpoint's assembly
        cablePlug = cable->sourcePlug();
        cableAssembly = cablePlug->assembly().id + '-' + cablePlug->assembly().position;

        if (endpointAssembly == cableAssembly)
        {
            cableConnId = cable->element().getChildValue("source-connector-instance");
        }
        else
        {
            //Now try the target side
            cablePlug = cable->targetPlug();
            cableAssembly = cablePlug->assembly().id + '-' + cablePlug->assembly().position;

            if (endpointAssembly == cableAssembly)
            {
                cableConnId = cable->element().getChildValue("target-connector-instance");
            }
            else
            {
                mrwLogger l;
                l() << "Couldn't find a cable endpoint with same assembly as " 
                    << endpoint->getChipPath() << " for cable " << cable->element().getChildValue("name");
                l.error();
            }
        }


        //Now build the connector instance path
        if (!cableConnId.empty())
        {

            //get the connector-instance, so we can build the instance path
            XMLElement inst = cablePlug->card().findPath("connector-instances").find("connector-instance", "id", cableConnId);
            if (!inst.empty())
            {
                string cid = inst.getChildValue("connector-id");
                string pos = inst.getChildValue("position");
                path = cablePlug->path() + "/" + cid + "-" + pos;
            }
            else
            {
                mrwLogger l;
                l() << "Couldn't find connector instance element for ID " << cableConnId << " on " << cablePlug->path();
                l.error();
            }
        }
    }


    return path;
}


/**
 * Returns the first cable found in the list of iv_connectors.  So far, every system
 * just has one cable at most.
 */
Cable* FSISingleHopBus::getCable()
{
    Cable* cable = NULL;

    for (size_t i=0;i<iv_connectors.size();i++)
    {
        if (iv_connectors[i]->isCable())
        {
            cable = ((FSICable*)iv_connectors[i])->cable();
            break;
        }

    }

    return cable;
}


string FSISingleHopBus::getCableName()
{
    string name;
    Cable* cable = getCable();

    if (cable)
    {
        name = cable->element().getChildValue("name");
    }

    return name;
}




/**
 * Returns the FSI link number
 */
string FSIMaster::getPort()
{
    if (iv_port.empty())
    {
        string name = (iv_cascadedMaster) ? "fsi-cascade-master-unit" : "fsi-master-unit";
        XMLElement unit = mrwGetUnit(iv_part.getChildValue("id"), name, iv_source.unit());

        //If not the FSP, we can look in the unit, otherwise we'll have to look in the
        //dio-configs section of the part

        if (iv_part.getChild("dio-configs").empty())
        {
            iv_port = unit.getChildValue("port");
        }
        else
        {
            XMLElement dioConfig;
            dioConfig = iv_part.findPath("dio-configs").find("dio-config", "unit-name", unit.getChildValue("id"));

            if (!dioConfig.empty())
            {
                iv_port = dioConfig.getChildValue("port");

                if ((iv_port.size() > 1) && (iv_port[0] == '0')) //take out leading 0
                    iv_port = iv_port.substr(1, iv_port.size() - 1);
            }

        }

    }

    return iv_port;
}


/**
 * Returns the FSI engine
 */
string FSIMaster::getEngine()
{

    if (iv_engine.empty())
    {
        string name = (iv_cascadedMaster) ? "fsi-cascade-master-unit" : "fsi-master-unit";
        XMLElement unit = mrwGetUnit(iv_part.getChildValue("id"), name, iv_source.unit());

        //If not the FSP, we can look in the unit, otherwise we'll have to look in the
        //dio-configs section of the part

        if (iv_part.getChild("dio-configs").empty())
        {
            iv_engine = unit.getChildValue("engine");
        }
        else
        {
            XMLElement dioConfig;
            dioConfig = iv_part.findPath("dio-configs").find("dio-config", "unit-name", unit.getChildValue("id"));

            if (!dioConfig.empty())
                iv_engine = dioConfig.getChildValue("engine");

        }

    }

    return iv_engine;
}

/**
 * Returns the slave port #, either 0 or 1
 */
string FSISlave::getPort()
{
    if (iv_port.empty())
    {
        XMLElement unit = mrwGetUnit(iv_part.getChildValue("id"), "fsi-slave-unit", iv_source.unit());

        //If not a CFAM-S/FSP, we can look in the unit, otherwise we'll have to look in the
        //dio-configs section of the part

        if (iv_part.getChild("dio-configs").empty())
        {
            iv_port = unit.getChildValue("port");
        }
        else
        {
            XMLElement dioConfig;
            dioConfig = iv_part.findPath("dio-configs").find("dio-config", "unit-name", unit.getChildValue("id"));

            if (!dioConfig.empty())
            {
                iv_port = dioConfig.getChildValue("port");

                if ((iv_port.size() > 1) && (iv_port[0] == '0')) //take out leading 0
                    iv_port = iv_port.substr(1, iv_port.size() - 1);
            }

        }

        //if couldn't find it, just use 0
        if (iv_port.empty())
        {
            iv_port = "0";
        }
    }

    return iv_port;
}




/**
 * Returns true if the source & plug passed are a master FSI unit
 */
bool isMasterFSI(Endpoint & i_source, Plug* i_plug, bool & o_cascadedMaster)
{
    bool master = false;
    string partId = mrwGetPartId(i_plug->card(), i_source.id());
    XMLElement part = mrwGetPart(partId);

    o_cascadedMaster = false;

    if (!part.empty())
    {
        XMLElement unit = part.findPath("units/fsi-master-units").find("fsi-master-unit", "id", i_source.unit());

        if (!unit.empty())
        {
            master = true;
        }
        else
        {
            //now try cascaded master units
            XMLElement unit = part.findPath("units/fsi-cascade-master-units").find("fsi-cascade-master-unit", "id", i_source.unit());

            if (!unit.empty())
            {
                master = true;
                o_cascadedMaster = true;
            }
        }
    }

    return master;
}


/**
 * Returns true if the source & plug passed are a slave FSI unit
 */
bool isSlaveFSI(Endpoint & i_source, Plug* i_plug)
{
    bool slave = false;
    string partId = mrwGetPartId(i_plug->card(), i_source.id());
    XMLElement part = mrwGetPart(partId);

    if (!part.empty())
    {
        XMLElement unit = part.findPath("units/fsi-slave-units").find("fsi-slave-unit", "id", i_source.unit());

        if (!unit.empty())
            slave = true;
    }

    return slave;
}



/**
 * Walks an FSI bus from the source unit in i_sourceBus out to the endpoint unit
 * in possibly another FSIBus, and creates a FSISingleHopBus object for the path.
 */
void walkFSIBus(FSIBus* i_sourceBus, endpointType i_type)
{
    FSIMaster* master = NULL;
    FSIBus* bus = i_sourceBus;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    endpointType type = (i_type == SOURCE) ? ENDPOINT : SOURCE;
    bool isSlave = false;
    bool cascaded = false;
    vector<FSIConnector*> connectors;
    Cable* cable = NULL;

    //Walk the FSIs from a master or cascaded master out to a slave.
    //Cascading will be done later.

    if (isMasterFSI(*source, bus->plug(), cascaded))
        master = new FSIMaster(*source, bus->plug(), cascaded);

    //Walk the bus if necessary to find the slave
    if (!isSlaveFSI(*endpoint, bus->plug()))
    {
        //save the connector in the path
        connectors.push_back(new FSIConnector(endpoint->id(), bus->plug()));

        do
        {
            cable = NULL;
            bus = mrwGetNextBus<FSIBus>(bus, type, &cable);

            if (!bus)
                break;

            //If the source is a slave, flag it, otherwise save the connector in the path
            if (isSlaveFSI(bus->source(), bus->plug()))
                isSlave = true;
            else
            {
                if (cable)
                    connectors.push_back(new FSICable(cable));
                else
                    connectors.push_back(new FSIConnector(bus->source().id(), bus->plug()));
            }


            //If the endpoint is a slave, flag it, otherwise save the connector in the path
            if (isSlaveFSI(bus->endpoint(), bus->plug()))
                isSlave = true;
            else
            {
                if (cable)
                    connectors.push_back(new FSICable(cable));
                else
                    connectors.push_back(new FSIConnector(bus->endpoint().id(), bus->plug()));
            }

        } while (!isSlave);
    }


    if (!bus)
    {
#if 0 //let's not trace this, we see it everywhere
        ostringstream msg;
        msg << "No endpoint found for FSI bus path that starts at " <<
                i_sourceBus->plug()->path() << "/" << i_sourceBus->source().id() << "/" <<i_sourceBus->source().unit();
        mrwInfo(msg.str());
#endif

        return;
    }

    //Create the full single hop bus

    FSISlave* slave;

    if (type == SOURCE)
        slave = new FSISlave(bus->source(), bus->plug());
    else
        slave = new FSISlave(bus->endpoint(), bus->plug());


	if (mrwLogger::getDebugMode()) {
		string m = "Creating FSI slave: " + mrwUnitPath(slave->plug(), slave->source());
		mrwLogger::debug(m);
	}

	FSISingleHopBus* fsi = new FSISingleHopBus(master, slave, connectors);

	g_singleHopBusses.push_back(fsi);

}



/**
 * Returns a list of FSISingleHopBus that have a master on the same part that has
 * the slave passed in.
 */
void getNextSingleHopBusses(FSISlave* i_slave, vector<FSISingleHopBus*> & o_busses)
{
    vector<FSISingleHopBus*>::iterator b;

    //Get the busses that have masters on the same chip as this slave

    if (i_slave->partType() != "fsp") //but not the FSP
    {
        for (b=g_singleHopBusses.begin();b!=g_singleHopBusses.end();++b)
        {
            if (((*b)->master()->source().id() == i_slave->source().id()) &&  ((*b)->master()->plug() == i_slave->plug()))
            {
                o_busses.push_back(*b);
            }
        }
    }
}



/**
 * Walks the single hop bus passed in out to other single hop busses that have an FSI master
 * at the passed in bus's FSI slave, and puts them into a multi-hop FSISystemBus object.
 * For a complete multi-hop path, would also get additional objects for the sub-paths.
 * If the single hop bus passed in was FSP->processor1, could get all of these objects:
 *
 *   FSP->processor1
 *   FSP->processor1->processor2
 *   FSP->processor1->processor2->centaur.
 *   processor2->centaur
 *
 *   When the master is a processor, only single hop FSISystem bus objects will be created,
 *   and not processor1->processor2->centaur
 *
 */
void createMultiHopBusses(FSISingleHopBus* i_bus,
                          vector<FSISystemBus*> & o_multiHopBusses)
{
    FSISystemBus* bus = NULL;
#ifndef MRW_OPENPOWER    
    vector<FSISingleHopBus*> busses;
    vector<FSISingleHopBus*>::iterator b;
    vector<FSISingleHopBus*> farBusses;
    vector<FSISingleHopBus*>::iterator farBus;
#endif

    //Create this single hop FSP<->chip bus as a system bus
    bus = new FSISystemBus(i_bus->master(), i_bus->slave());
    bus->addSingleHopBus(i_bus);
    o_multiHopBusses.push_back(bus);

//OpenPower doesn't care about MultiHop busses; just put
//every FSISingleHopBus into an FSISystemBus
#ifndef MRW_OPENPOWER

	// In the case where the master & slave are on the same part, we're done
    if ((i_bus->master()->source().id() == i_bus->slave()->source().id()) &&
		(i_bus->master()->plug() == i_bus->slave()->plug()))
    {
		return;
	}

    //To find the next segment in the bus, get the busses that have
    //masters on the same chip as this slave.
    getNextSingleHopBusses(i_bus->slave(), busses);

    for (b=busses.begin();b!=busses.end();++b)
    {
        //On redundant FSP systems with both slave ports used, only want to
        //keep the centaur bus that uses the same slave port as the processor's
        //slave port. (i.e. if proc has input on slave port 0, the the centaur
        //will only have an input on slave port 0).  The other FSP connection
        //then uses the other one and we'll print that one then.
        if ((*b)->master()->cascadedMaster())
        {
            if ((*b)->slave()->getPort() != i_bus->slave()->getPort())
            {
                continue;
            }
        }

        //Create this single hop system bus
        bus = new FSISystemBus((*b)->master(), (*b)->slave());
        bus->addSingleHopBus(*b);
        o_multiHopBusses.push_back(bus);

        //Create the system bus that goes 2 hops back to the FSP
        bus = new FSISystemBus(i_bus->master(), (*b)->slave());
        o_multiHopBusses.push_back(bus);

        //Add the busses for the 2 segments
        bus->addSingleHopBus(i_bus);
        bus->addSingleHopBus(*b);


        //Now get the busses that connect to this slave,
        //so 3 hops out.
        farBusses.clear();
        getNextSingleHopBusses((*b)->slave(), farBusses);

        for (farBus=farBusses.begin();farBus!=farBusses.end();++farBus)
        {
            //Only want the centaur busses out this far,
            //not the proc->proc ones
            if ((*farBus)->master()->cascadedMaster())
            {
                //Check the centaur ports again
                if ((*farBus)->slave()->getPort() != (*b)->slave()->getPort())
                {
                    continue;
                }

                //Create this single hop system bus
                bus = new FSISystemBus((*farBus)->master(), (*farBus)->slave());
                bus->addSingleHopBus(*farBus);
                o_multiHopBusses.push_back(bus);

                //Create the 3 hop system bus back to the FSP
                bus = new FSISystemBus(i_bus->master(), (*farBus)->slave());
                bus->addSingleHopBus(i_bus);
                bus->addSingleHopBus(*b);
                bus->addSingleHopBus(*farBus);
                o_multiHopBusses.push_back(bus);
            }
        }

    }

#endif

}




/**
 * After we have the FSISingleHopBus objects, walk those to create the
 * multi-hop FSISystemBus objects, starting at the FSP
 */
void createMultiHopBusses(vector<FSISystemBus*> & o_multiHopBusses)
{
    vector<FSISingleHopBus*>::iterator b;

    o_multiHopBusses.clear();

    for (b=g_singleHopBusses.begin();b!=g_singleHopBusses.end();++b)
    {
        //OpenPower would like every bus in an FSISystemBus
#ifndef MRW_OPENPOWER        
        if ((*b)->master()->partType() == "fsp")
#endif
            createMultiHopBusses(*b, o_multiHopBusses);
    }

#ifndef MRW_OPENPOWER
    if (o_multiHopBusses.empty())
    {
        mrwLogger logger;
        logger() << "Could not find an FSI connection out from the FSP even with "
                 << g_singleHopBusses.size() << " single hop FSI busses.";
        logger.error(true);
    }
#endif

}


/**
 * Finds all the master FSI units in g_fsiBusses and walks them out to slaves
 * to create the FSISystemBus objects
 */
void mrwFSIProcessBusses(vector<FSISystemBus*> & o_fsiSystemBusses)
{
    vector<FSIBus*>::iterator bus;
    bool cascaded = false;


    for (bus=g_fsiBusses.begin();bus!=g_fsiBusses.end();++bus)
    {
        if (isMasterFSI((*bus)->source(), (*bus)->plug(), cascaded))
        {
            if (mrwLogger::getDebugMode()) {
                string m = "Walking out FSI Master: " + mrwUnitPath((*bus)->plug(), (*bus)->source());
                mrwLogger::debug(m);
            }

            //Adds each single hop bus to g_singleHopBusses
            walkFSIBus(*bus, SOURCE);
        }
        if (isMasterFSI((*bus)->endpoint(), (*bus)->plug(), cascaded))
        {
            if (mrwLogger::getDebugMode()) {
                string m = "Walking out FSI Master: " + mrwUnitPath((*bus)->plug(), (*bus)->endpoint());
                mrwLogger::debug(m);
            }

            //Adds each single hop bus to g_singleHopBusses
            walkFSIBus(*bus, ENDPOINT);
        }

    }


    //Creates the FSISystemBus objects that can span multiple hops
    if (mrwLogger::getDebugMode()) {
        string m = "Creating multihop FSI Busses";
        mrwLogger::debug(m);
    }

    createMultiHopBusses(o_fsiSystemBusses);

    if (mrwLogger::getDebugMode()) {
        ostringstream m;
        m << "Done creating multihop FSI Busses";
        mrwLogger::debug(m.str());

        vector<FSISystemBus*>::iterator b;
        vector<FSISingleHopBus*>::iterator h;
        for (b=o_fsiSystemBusses.begin();b!=o_fsiSystemBusses.end();++b)
        {
            m.str("");
            m << "Created " << (*b)->singleHopBusses().size() << " hop FSISystemBus from "
              << (*b)->master()->getUnitPath() << " to " << (*b)->slave()->getUnitPath()
              << " port " << (*b)->slave()->getPort();
#ifdef MRWDEBUG
            m << "\nSingleHop Busses:\n";
            for (h=(*b)->singleHopBusses().begin();h!=(*b)->singleHopBusses().end();++h)
            {
                m << "   Master:  " << (*h)->master()->getUnitPath() << endl;
                m << "   Slave:   " << (*h)->slave()->getUnitPath() << " Port: " << (*h)->slave()->getPort() << endl;
            }

            m << "----------------------------------------------\n";
#endif
            mrwLogger::debug(m.str());
        }

    }




}


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
void mrwFsiMakeSystemBusses(vector<FSISystemBus*> & o_fsiSystemBusses)
{
    g_fsiBusses.clear();
    g_singleHopBusses.clear();

    mrwMakePlugs();

    mrwMakeBusses<FSIBus>(g_plugs, "fsi", g_fsiBusses);

    mrwFSIProcessBusses(o_fsiSystemBusses);

    if (mrwLogger::getDebugMode()) {
        ostringstream m; m << "Found " << o_fsiSystemBusses.size() << " FSISystemBus objects";
        mrwLogger::debug(m.str());
    }

}



/**
 * Returns the FSI segment of the firmware device path for the system bus
 * passed in.
 */
string mrwFsiGetDevicePathSubString(FSISystemBus* i_bus)
{
    vector<FSISingleHopBus*> & busses = i_bus->singleHopBusses();
    vector<FSISingleHopBus*>::iterator shb;
    FSIMaster* master;
    string path;



    /*
     * The FSI portions of the fsp device paths look like:
     *    /dev/<type>/LxxCxExxPxx  for devices directly connected to the FSP
     *
     * or, for device going through other hub/cascaded FSI masters
     *   /dev/<type>/LxxCxExx:LxCxExxPxx                   fsp->pu
     *   /dev/<type>/LxxCxExx:LxCxExx:LxCxExxPxx           fsp->pu->pu
     *   /dev/<type>/LxxCxExx:LxCxExx:LxCxExx:LxCxExxPxx   fsp->pu->pu->cent
     *
     *
     * where L: FSI link #
     *       C: chained CFAM #.  always zero
     *       E: engine number
     *       P: port number in the engine
     *
     * Note that for all sublinks (hub or cascaded), it's just Lx and not Lxx
     *
     * This code will just return the LxxCxExx... of the FSI path.  It won't
     * have the last engine since that isn't an FSI engine, rather i2c, gpio, etc.
     * So, for a 3 link path it would return something like:
     *  L02C0E11:L1C0E10:L3C0
     */


    for (shb=busses.begin();shb!=busses.end();++shb)
    {
        master = (*shb)->master();

        if (shb != busses.begin()) //not first bus
        {
            //stick the engine onto the full path
            path += "E" + mrwPadNumToTwoDigits(master->getEngine()) + ":";

            //sublinks are always 1 character
            path += "L" + master->getPort();

        }
        else
        {
            //fsp master links are always 2 character
            path += "L" + mrwPadNumToTwoDigits(master->getPort());

        }


        path += "C0"; //CFAM # is always zero for ipSeries

    }

    return path;
}


/**
 * Finds all the FSI busses that go out to a card and hence that CFAM hot plug event
 * of the slave can be used for presence detect for that FRU.
 */
void mrwFsiGetPresenceDetectBusses(const vector<FSISystemBus*> & i_allBusses,
                                   vector<FSISystemBus*> & o_presBusses)
{
    vector<FSISystemBus*>::const_iterator bus;
    vector<Plug*> plugs;

    //Look through all the 1 hop FSI busses where the master and slave are on
    //different cards.  The existence of the slave CFAM is what can be used for
    //the presence detection.

    for (bus=i_allBusses.begin();bus!=i_allBusses.end();++bus)
    {
        //Just look for the single hop busses

        if ((*bus)->singleHopBusses().size() != 1)
            continue;


        //check that we haven't already found an FSI bus for this card(plug)
        if (find(plugs.begin(), plugs.end(), (*bus)->slave()->plug()) == plugs.end())
        {
            //Check master & slave aren't on the same card
            if ((*bus)->master()->plug() != (*bus)->slave()->plug())
            {
                o_presBusses.push_back(*bus);

                //slave the plug so we won't save anything on this card again
                plugs.push_back((*bus)->slave()->plug());
            }
        }
    }

}

