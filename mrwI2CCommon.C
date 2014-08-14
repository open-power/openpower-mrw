/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwI2CCommon.C $                                              */
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

#include <iostream>
#include <mrwFSICommon.H>
#include <mrwI2CCommon.H>
#include <math.h>

using namespace std;


//Globals
vector<I2CBus*> g_i2cBusses;

//mrwParserCommon defines these globals
extern vector<Plug*> g_plugs;



string binToHexString(const string & i_data);
string combinePartials(const vector<string> & i_partials);


/**
* Returns the address for the bus, which it gets either from an endpoint or
* from a bus segment, or it builds it along the way.
*/
string I2CSystemBus::getAddress()
{

    if (iv_address.empty())
    {
        string fAddress = iv_slave->getFixedAddress();
        string sAddress = iv_slave->address();
        string address;
        string prefix;


        //The address could have either been in the fixed-address from the part XML, or
        //specified in a bus segment.


#if 0
        //TODO:  not supportiing partial addresses right now!
        //But, the endpoint for the destination unit isn't guaranteed to have a fixed address
        //it may have been assigned earlier along the bus, and
        //it might be constructed hierarchically along the bus path, like:
        //  card 0:  XX1
        //  card 1:  01X
        // and they'll have to be combined to be 0b011 = 3

        if (!iv_slave->endpoint().address().empty())
        {
            //binary with a 'X'
            if (iv_slave->endpoint().binaryAddress() &&
                            (iv_slave->endpoint().address().find('X') != string::npos))
            {
                partials.push_back(iv_slave->endpoint().address());
            }
            else //the full address, but might be in binary
            {
                if (iv_slave->endpoint().binaryAddress())
                    dAddress = binToHexString(iv_slave->endpoint().address());
                else
                    dAddress = iv_slave->endpoint().address();
            }
        }
#endif

#if 0
        //always look through the segments too, to either find more partials or
        //find cases where more than 1 address was specified, or maybe a partial
        //and a full address

        for (c=iv_connections.begin();c!=iv_connections.end();++c)
        {
            if (!(*c)->address().empty())
            {
                if ((*c)->binaryAddress() && ((*c)->address().find('X') != string::npos))
                {
                    partials.push_back((*c)->address());
                }
                else if ((*c)->binaryAddress())
                {
                    cAddress = binToHexString((*c)->address());
                }
                else
                {
                    cAddress = (*c)->address();
                }
            }
        }
#endif

        //Take off the 0x if there is one.
        if (!fAddress.empty())
        {
            prefix = fAddress.substr(0, 2);
            if ((prefix == "0x") || (prefix == "0X"))
            {
                fAddress = fAddress.substr(2, fAddress.size() - 2);
            }
        }

        if (!sAddress.empty())
        {
            prefix = sAddress.substr(0, 2);
            if ((prefix == "0x") || (prefix == "0X"))
            {
                sAddress = sAddress.substr(2, sAddress.size() - 2);
            }
        }


        //Do some checks to make sure we only have the address specified in one place

        if (!sAddress.empty()) //Was specified in the bus XML in a card
        {
            if (!fAddress.empty() && (sAddress != fAddress))
            {
                mrwLogger logger;
                logger() << "i2c device " << iv_slave->plug()->path() << "/" << iv_slave->source().id()
                                     << " has a different i2c address specified in more than one place (one is a fixed address) "
                                     << sAddress << " and " << fAddress << endl;
                logger.error(true);
            }

            address = sAddress;

        }
        else if (!fAddress.empty()) //specified as a fixed address in the i2c bus endpoint XML
        {
            address = fAddress;
        }
#if 0
        else if (!partials.empty()) //partially specified
        {
            //build up the address from the partial ones
            dAddress = combinePartials(partials);
        }
#endif

        iv_address = address;

    }

    return iv_address;
}




/**
 * Converts a binary string, like "1101" to a hex string, like "D".
 */
string binToHexString(const string & i_data)
{
    string data = "0";
    int num = 0;
    int index = 0;

    //hack off leading 0s
    size_t pos = i_data.find_first_of('1');

    if (pos != string::npos)
    {
        data = i_data.substr(pos, i_data.size() - pos);

        for (unsigned int i=0;i<data.size();i++)
        {
            index = data.size()-i-1;

            if (data[index] != '0')
            {
                num += (int) pow(2, i);
            }
        }

        char word[4];
        sprintf(word, "%X", num);

        data = word;
    }


    return data;
}



/**
 * Combines a list of partial binary values into a full binary value,
 * and then converts it to a hex string.
 * Like, XXX1, 100X, would be combined to 1001
 */
string combinePartials(const vector<string> & i_partials)
{
    string binValue(32, '0'); //max is 32 bits
    vector<string>::const_iterator p;
    int index = 0;

    for (p=i_partials.begin();p!=i_partials.end();++p)
    {

        if ((*p).size() > 32)
        {
            mrwLogger logger;
            logger() << "Found a partial I2Caddress " << (*p) << "bigger than 32 bits";
            logger.error(true);
        }
        else
        {
            for (unsigned int i=0;i<(*p).size();i++)
            {
                index = (*p).size() - i - 1;

                //if it's not masked, set/clear the bit in 'value'
                if ((*p).at(index) != 'X')
                {
                    binValue[31 - i] = (*p).at(index);
                }
            }
        }
    }

    return binToHexString(binValue);
}


/**
 * Makes a I2CBus object for every bus on every card.  Saves the object on the card (plug), and
 * also in the g_i2cBusses global vector.
 * @param i_plug
 */
void makeI2CBusses(Plug* i_plug)
{
    vector<XMLElement> busGroups;
    vector<XMLElement> busses;
    vector<XMLElement>::iterator busGroup;
    vector<XMLElement>::iterator b;
    XMLElement address, speed, pd;


    if (i_plug->type() == Plug::CARD)
    {
        i_plug->card().findPath("busses").getChildren(busGroups);

        for (busGroup=busGroups.begin();busGroup!=busGroups.end();++busGroup)
        {
            if (busGroup->getName() == "i2cs")
            {
                busGroup->getChildren(busses, "i2c");

                for (b=busses.begin();b!=busses.end();++b)
                {
                    I2CBus* bus = new I2CBus(*b, i_plug);
                    i_plug->busses().push_back(bus);
                    g_i2cBusses.push_back(bus);

                    if (b->findPath("source").getChildValue("connector-instance-id") != "")
                        bus->source().id(b->findPath("source").getChildValue("connector-instance-id"));
                    else
                        bus->source().id(b->findPath("source").getChildValue("part-instance-id"));

                    if (b->findPath("source").getChildValue("unit-name") != "")
                        bus->source().unit(b->findPath("source").getChildValue("unit-name"));

                    if (b->findPath("source").getChildValue("pin-name") != "")
                        bus->source().pin(b->findPath("source").getChildValue("pin-name"));


                    ////////////////////

                    if (b->findPath("endpoint").getChildValue("connector-instance-id") != "")
                        bus->endpoint().id(b->findPath("endpoint").getChildValue("connector-instance-id"));
                    else
                        bus->endpoint().id(b->findPath("endpoint").getChildValue("part-instance-id"));

                    if (b->findPath("endpoint").getChildValue("unit-name") != "")
                        bus->endpoint().unit(b->findPath("endpoint").getChildValue("unit-name"));

                    if (b->findPath("endpoint").getChildValue("pin-name") != "")
                        bus->endpoint().pin(b->findPath("endpoint").getChildValue("pin-name"));

                    address = b->findPath("address");
                    if ((!address.empty()) && (address.getValue() != "0x00"))
                    {
                        bus->address(address.getValue());

                        if (address.getAttributeValue("base") == "binary")
                        {
                            bus->binaryAddress(true);
                        }
                    }

                    speed = b->findPath("speed");
                    if (!speed.empty())
                    {
                        bus->speed(speed.getValue());
                    }

                    pd = b->findPath("use-for-presence-detect");
                    if (!pd.empty())
                    {
                        bus->pd(pd.getValue());
                    }

                }

            }
        }
    }



    vector<Plug*>::iterator plug;
    for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
    {
        makeI2CBusses(*plug);
    }
}



/**
 * Fills in g_i2cBusses with all the I2CBus segment objects
 */
void mrwI2CMakeI2CBusses()
{
    vector<Plug*>::iterator plug;

    for (plug=g_plugs.begin();plug!=g_plugs.end();++plug)
    {
        makeI2CBusses(*plug);
    }
}




/**
 * Walks an I2C bus from source unit to end unit.  Creates an I2CSystemBus object for every full bus path it
 * finds and adds them to o_i2cBusses.  Recursive because an I2C bus can spread out on the other side
 * of a connector to duplicate devices.
*/
void mrwI2CWalkI2CBus(I2CBus* i_sourceBus,
                      endpointType i_type,
                      I2CMaster *& i_master,
                      vector<I2CConnection*> & i_connections,
                      const string i_speed,
                      const string i_address,
                      bool i_pd, //if used for presence detect
                      vector<I2CSystemBus*> & o_i2cBusses)
{
    vector<busAndType_t<I2CBus> > busses;
    vector<busAndType_t<I2CBus> >::iterator b;
    I2CMaster* master = NULL;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    endpointType type = i_type;
    I2CBus* bus;
    string s, speed = i_speed;
    string a, address = i_address;
    bool pd = (i_pd) ? true : i_sourceBus->pd();


    //If the source has a unit, then create the source of the system bus
    if (!source->unit().empty())
    {
        master = new I2CMaster(i_sourceBus->plug(), *source);
    }



    //we found the end
    if (!endpoint->unit().empty())
    {
        //if we didn't walk any busses, may need to get the address & speed here
        a = (!i_address.empty()) ? i_address : i_sourceBus->address();

        s = (!i_speed.empty()) ? i_speed : i_sourceBus->speed();


        //Create the system destination
        I2CSlave* slave = new I2CSlave(i_sourceBus->plug(), *endpoint, a, s);

        if (mrwLogger::getDebugMode())
        {
            string m = "Creating slave " + mrwUnitPath(slave->plug(), slave->source());
            mrwLogger::debug(m);
        }

        //Create the system bus object, passing in the master, slave, and connections along the way
        I2CSystemBus* systemBus = new I2CSystemBus(i_master ? i_master : master,
                                                   slave, pd,
                                                   i_connections);

        o_i2cBusses.push_back(systemBus);

    }
    else //will need to find the next bus segment
    {

        //Set the speed from this bus segment
        s = i_sourceBus->speed();
        if (!speed.empty())
        {
            if (!s.empty())
            {
                if (s != speed)
                {
                    mrwLogger logger;
                    logger() << "Different segments along an I2C bus have different speeds " << speed << " and " << s
                             << " master is ";
                    if (i_master)
                        logger() << i_master->plug()->path() << "/" << i_master->source().id() << "/" << i_master->source().unit();
                    else if (master)
                        logger() << master->plug()->path() << "/" << master->source().id() << "/" << master->source().unit();
                    else
                        logger() << i_sourceBus->plug()->path() << "/" << source->id() << "/" << source->unit();
                    logger.error(true);
                }
            }
        }
        else
            speed = s;

        //Set the address from this bus segment
        a = i_sourceBus->address();
        if (!address.empty())
        {
            if (!a.empty())
            {
                if (a != address)
                {
                    mrwLogger logger;
                    logger() << "Different segments along an I2C bus have different addresses " << address << " and " << a
                             << " master is ";
                    if (i_master)
                        logger() << i_master->plug()->path() << "/" << i_master->source().id() << "/" << i_master->source().unit();
                    else if (master)
                        logger() << master->plug()->path() << "/" << master->source().id() << "/" << master->source().unit();
                    else
                        logger() << i_sourceBus->plug()->path() << "/" << source->id() << "/" << source->unit();
                    logger.error(true);
                }
            }
        }
        else
            address = a;


        //i_sourceBus may be connected to duplicate busses on the other side
        //so need to get back all of them.  In this case when we do reach
        //the end devices they will all have the same master.

        mrwGetNextBusses<I2CBus>(i_sourceBus->plug(), *endpoint, busses);

        for (b=busses.begin();b!=busses.end();++b)
        {
            bus  = b->bus;
            type = b->type;

            //Set the speed for the bus
            s = bus->speed();
            if (!speed.empty())
            {
                if (!s.empty())
                {
                    if (s != speed)
                    {
                        mrwLogger logger;
                        logger() << "Different segments along an I2C bus have different speeds " << speed << " and " << s
                                 << " master is ";
                        if (i_master)
                            logger() << i_master->plug()->path() << "/" << i_master->source().id() << "/" << i_master->source().unit();
                        else if (master)
                            logger() << master->plug()->path() << "/" << master->source().id() << "/" << master->source().unit();
                        else
                            logger() << i_sourceBus->plug()->path() << "/" << source->id() << "/" << source->unit();
                        logger.error(true);
                    }
                }
                else
                    s = speed;
            }

            //Set the address for the bus
            a = bus->address();
            if (!address.empty())
            {
                if (!a.empty())
                {
                    if (a != address)
                    {
                        mrwLogger logger;
                        logger() << "Different segments along an I2C bus have different addresses " << address << " and " << a
                                  << " master is ";
                        if (i_master)
                            logger() << i_master->plug()->path() << "/" << i_master->source().id() << "/" << i_master->source().unit();
                        else if (master)
                            logger() << master->plug()->path() << "/" << master->source().id() << "/" << master->source().unit();
                        else
                            logger() << i_sourceBus->plug()->path() << "/" << source->id() << "/" << source->unit();
                        logger.error(true);
                    }
                }
                else
                    a = address;
            }

            //Save the connection points
            vector<I2CConnection*> connections = i_connections;
            I2CConnection* connection;

            //Save both sides of the intermediate plugs and connectors
            connection = new I2CConnection(bus->plug(),
                                           (type == SOURCE) ? bus->source().id() : bus->endpoint().id());
            connections.push_back(connection);

            connection = new I2CConnection(bus->plug(),
                                           (type == SOURCE) ? bus->endpoint().id() : bus->source().id());
            connections.push_back(connection);


            //walk the next part of the bus
            mrwI2CWalkI2CBus(bus, type, i_master ? i_master : master,
                             connections, s, a, pd, o_i2cBusses);

        }
    }
}





/**
 * Returns true if the unit for the endpoint passed in is an i2c-master-unit.
 */
bool isMasterI2C(Endpoint & i_source, Plug* i_plug)
{
    bool master = false;
    string partId = mrwGetPartId(i_plug->card(), i_source.id());
    XMLElement part = mrwGetPart(partId);

    if (!part.empty())
    {
        vector<XMLElement> unitGroups;
        vector<XMLElement> units;
        vector<XMLElement>::iterator group;
        vector<XMLElement>::iterator unit;

        part.findPath("units").getChildren(unitGroups);

        for (group=unitGroups.begin();group!=unitGroups.end();++group)
        {
            group->getChildren(units);

            for (unit=units.begin();unit!=units.end();++unit)
            {
                if (unit->getName() == "i2c-master-unit")
                {
                    if (unit->getChildValue("id") == i_source.unit())
                    {
                        master = true;
                        break;
                    }
                }
            }
            if (master)
                break;
        }


    }

    return master;
}


/**
 * Finds every I2CBus object segment that hsa a unit at the source and walks it out
 * to every destination.
 */
void processI2CBusses(vector<I2CSystemBus*> & o_i2cBusses)
{
    vector<I2CBus*>::iterator bus;
    I2CMaster* master = NULL;

    for (bus=g_i2cBusses.begin();bus!=g_i2cBusses.end();++bus)
    {
        string speed, address;

        if (isMasterI2C((*bus)->source(), (*bus)->plug()))
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out I2C Master: " + mrwUnitPath((*bus)->plug(), (*bus)->source());
                mrwLogger::debug(m);
            }

            vector<I2CConnection*> connections;
            mrwI2CWalkI2CBus(*bus, SOURCE, master, connections, speed, address, false, o_i2cBusses);
        }

        else if (isMasterI2C((*bus)->endpoint(), (*bus)->plug()))
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out I2C Master: " + mrwUnitPath((*bus)->plug(), (*bus)->endpoint());
                mrwLogger::debug(m);
            }

            vector<I2CConnection*> connections;
            mrwI2CWalkI2CBus(*bus, ENDPOINT, master, connections, speed, address, false, o_i2cBusses);
        }
    }
}



//TODO:  if necessary, could also check that all devices on the same master/engine/port run
//at the same speed, or could set the bus speed to the lowest speed

/**
 * Prints errors for any devices that have the same address as another device as the same bus.
 */
void doBusChecks(const vector<I2CSystemBus*> & i_i2cBusses)
{
    vector<I2CSystemBus*>::const_iterator bus1;
    vector<I2CSystemBus*>::const_iterator bus2;
    string address1, address2;
    string port1, port2;
    string engine1, engine2;


    for (bus1=i_i2cBusses.begin();bus1!=i_i2cBusses.end();++bus1)
    {
        engine1  = (*bus1)->master()->getEngine();
        port1    = (*bus1)->master()->getPort();
        address1 = (*bus1)->getAddress();


        for (bus2=bus1+1;bus2!=i_i2cBusses.end();++bus2)
        {

            engine2  = (*bus2)->master()->getEngine();
            port2    = (*bus2)->master()->getPort();
            address2 = (*bus2)->getAddress();

            //Check that if they have the same master, engine, and port then they
            //must each have a different address

            if ((*bus1)->master()->getChipPath() == (*bus2)->master()->getChipPath())
            {
                if ((engine1 == engine2) && (port1 == port2) && !address1.empty() && (address1 == address2))
                {
                    //if they are on different plugs, but have the same location code then they are
                    //alternate versions of the same card so it's OK
                    if (!( ((*bus1)->slave()->plug() != (*bus2)->slave()->plug()) &&
                           ((*bus1)->slave()->plug()->location() == (*bus2)->slave()->plug()->location())))
                    {
                        mrwLogger logger;
                        logger() << "Slave device " << (*bus1)->slave()->getChipPath() << " has the same address "
                                        << (*bus1)->getAddress() << " as slave device " << (*bus2)->slave()->getChipPath()
                                        << " even though it's on the same I2C Bus (engine " << engine1 << ", port " << port1 << ").  Master is "
                                        << mrwUnitPath((*bus1)->master()->plug(), (*bus1)->master()->source());
                        logger.error(true);
                    }
                }
            }
        }

    }

}

/**
 * Creates a lot of all of the I2C busses in the system, which indicate
 * a path from an I2C Master to an I2C Slave.
 */
void mrwI2CMakeBusses(vector<I2CSystemBus*> & o_i2cBusses)
{
    g_i2cBusses.clear();

    mrwMakePlugs();

    mrwI2CMakeI2CBusses();

    processI2CBusses(o_i2cBusses);

    //While here, check for some problems on the bus

    doBusChecks(o_i2cBusses);

}


/**
 * Makes the firmware device path string when the I2C bus is a direct attach
 * to the FSP
 *
 * @param - i_master - the I2C master
 * @param - o_iou - set to true if the I2C engine is in the IOU, otherwise false
 */
string mrwI2CGetDirectConnectDevicePath(I2CMaster* i_master, bool & o_iou)
{
    string path = "/dev/iic/";
    XMLElement unit = i_master->unit();
    XMLElement dioConfig;

    //Either /dev/iic/Lxx/Cx/Exx/Pxx if through the IOU, or /dev/iic/<engine>


    dioConfig = i_master->part().findPath("dio-configs").find("dio-config", "unit-name", i_master->source().unit());
    string dio = dioConfig.getChildValue("id");

    //If the <id> element is -1, then it doesn't come out of the IOU, except for 
    //the BOE I2C, which is in the IOU at engine 14 but doesn't go through the IOUMux
    if ((dio == "-1") && (atoi(i_master->getEngine().c_str()) < 5)) //the nonIOU engines are 0-4
    {
        //For the direct connect links (no IOU/fsi) it's just /dev/iic/engine
        path += i_master->getEngine();
        o_iou = false;
    }
    else
    {
        //If it comes out of the IOU, then it uses FSI link 0 CFAM 0
        path += "L00C0";
        path += "E" + mrwPadNumToTwoDigits(i_master->getEngine());
        path += "P" + mrwPadNumToTwoDigits(i_master->getPort());
        o_iou = true;
    }

    return path;
}


/**
 * Makes the firmware device path for when the I2C bus connects via an FSI bus
 * back to the FSP
 */
string mrwI2CGetFsiI2cDevicePath(FSISystemBus* i_fsiBus, I2CMaster* i_i2cMaster)
{
    string path = "/dev/iic/";
    string fsiPath = mrwFsiGetDevicePathSubString(i_fsiBus);

    //Tack on the I2C engine & port to the FSI part of the path returned above.

    path += fsiPath;

    path += "E" + mrwPadNumToTwoDigits(i_i2cMaster->getEngine());
    path += "P" + mrwPadNumToTwoDigits(i_i2cMaster->getPort());

    return path;
}

