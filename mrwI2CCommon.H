/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwI2CCommon.H $                                              */
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
//  LAST_VERSION_FROM_CC: 
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwI2CCommon.H 1$
#ifndef MRWI2CCOMMON_H
#define MRWI2CCOMMON_H


/**
 * This file contains interfaces and classes that the I2C parsing code provides.
 */

#include <mrwParserCommon.H>



/**
 * Contains information on a segment of an I2C connection where
 * it plugs into a connector.
 */
class I2CConnection
{
public:

    I2CConnection(Plug* i_plug, const std::string & i_connector) :
        iv_plug(i_plug), iv_connector(i_connector) {}

    Plug* plug() { return iv_plug; }
    std::string connector() { return iv_connector; }


private:

    Plug* iv_plug;
    std::string iv_connector;

};



/**
 * Represents an I2C bus as it is in the cards XML.
 */
class I2CBus : public BusSegment
{

public:
    I2CBus(XMLElement& i_element, Plug* i_plug) :
        BusSegment(i_element, i_plug), iv_binaryAddress(false), iv_pd(false) {};

    std::string speed() { return iv_speed; }

    void speed(const std::string & i_speed)
    {
        iv_speed = i_speed;
    }

    bool pd() { return iv_pd; }

    void pd(const std::string & i_pd)
    {
        if (!mrwStrcmpi(i_pd, "yes"))
            iv_pd = true;
        else
            iv_pd = false;
    }

    std::string address() { return iv_address; }
    void address(const std::string & a) { iv_address = a; }

    bool binaryAddress() { return iv_binaryAddress; }
    void binaryAddress(bool i_binary) { iv_binaryAddress = i_binary; }

private:
    std::string iv_speed;
    std::string iv_address;
    bool iv_binaryAddress;
    bool iv_pd; //used for presence detect

};




/**
 * An I2CMaster
 */
class I2CMaster : public SystemEndpoint
{
public:
    I2CMaster(Plug* i_plug, Endpoint & i_source) :
        SystemEndpoint(i_source, i_plug) {}


    std::string getEngine()
    {
        if (iv_engine.empty())
        {
            XMLElement u = unit();

            //If not the FSP/CFAM-S, we can look in the unit, otherwise we'll have to look in the
            //dio-configs section of the part

            if (iv_part.getChild("dio-configs").empty())
            {
                iv_engine = u.getChildValue("engine");
            }
            else
            {
                XMLElement dioConfig;
                dioConfig = iv_part.findPath("dio-configs").find("dio-config", "unit-name", u.getChildValue("id"));
                iv_engine = dioConfig.getChildValue("engine");
            }


            if (iv_engine.empty())
            {
                mrwLogger l;
                l() << "No I2C engine found for i2c master " << getUnitPath();
                l.error(true);
            }

        }



        return iv_engine;
    }

    std::string getPort()
    {

        if (iv_port.empty())
        {
            XMLElement u = unit();

            //If not the FSP, we can look in the unit, otherwise we'll have to look in the
            //dio-configs section of the part

            if (iv_part.getChild("dio-configs").empty())
            {
                iv_port = u.getChildValue("port");
            }
            else
            {
                XMLElement dioConfig;
                dioConfig = iv_part.findPath("dio-configs").find("dio-config", "unit-name", u.getChildValue("id"));

                iv_port = dioConfig.getChildValue("port");

                if ((iv_port.size() > 1) && (iv_port[0] == '0')) //take out leading 0
                    iv_port = iv_port.substr(1, iv_port.size() - 1);

                if (iv_port.empty())
                {
                    mrwLogger l;
                    l() << "No I2C port found for i2c master " << getUnitPath();
                    l.error(true);
                }

            }


        }

        return iv_port;
    }


    XMLElement & unit()
    {
        if (iv_unit.empty())
        {
            std::string partId = mrwGetPartId(plug()->card(), source().id());
            iv_unit = mrwGetUnit(partId, "i2c-master-unit", source().unit());
        }

        return iv_unit;
    }

private:

    std::string iv_engine;
    std::string iv_port;
    XMLElement iv_unit;

};




/**
 * An I2CSlave
 */
class I2CSlave : public SystemEndpoint
{
public:
    I2CSlave(Plug* i_plug, Endpoint & i_source,
             const std::string & i_address,
             const std::string & i_speed) :
        SystemEndpoint(i_source, i_plug),
        iv_address(i_address),
        iv_speed(i_speed) {}

    //Returns the address that was stored along the bus segments
    std::string address() { return iv_address; }

    //returns the 'fixed-address' from the unit
    std::string getFixedAddress()
    {
        XMLElement unit = mrwGetUnit(part().getChildValue("id"),
                                     "i2c-slave-unit", source().unit());

        return unit.getChildValue("fixed-address");
    }

    int getSpeed()
    {
        if (iv_speed.empty())
            return 400; //just return 400 for now if empty
        else
            return atoi(iv_speed.c_str());
    }

    std::string getContentType()
    {
        return mrwGetPartInstanceData(plug(), source().id(), "content-type");
    }

    std::string getDeviceSize()
    {
        //try to get it from //part-instance/vpd-size
        std::string size = mrwGetPartInstanceData(plug(), source().id(), "vpd-size");

        //if not there, try to get it from //part/vpd-size
        if (size.empty())
        {
            size = part().getChildValue("vpd-size");
        }

        return size;
    }


private:

    std::string iv_address;
    std::string iv_speed;

};



/**
 * Represents an i2c bus as it goes from its source unit to its device endpoint,
 */
class I2CSystemBus
{
public:

    I2CSystemBus(I2CMaster* i_master,
                 I2CSlave*  i_slave,
                 bool i_pd, //presence detect
                 std::vector<I2CConnection*> i_connections) :
        iv_master(i_master),
        iv_slave(i_slave),
        iv_pd(i_pd),
        iv_connections(i_connections)
    {}

    I2CMaster* master() { return iv_master; }
    I2CSlave* slave() { return iv_slave; }
    std::vector<I2CConnection*>& connections() { return iv_connections; }

    /**
     * Returns the address for the bus, which it gets either from an endpoint or
     * from a bus segment, or it builds it along the way.
     */
    std::string getAddress();

    bool pd() { return iv_pd; }

private:

    I2CMaster* iv_master;
    I2CSlave* iv_slave;
    bool iv_pd;
    std::vector<I2CConnection*> iv_connections;
    std::string iv_address;

};



/**
 * Fills in g_i2cBusses with all the I2CBus segment objects
 */
void mrwI2CMakeI2CBusses();


/**
 * Walks an I2C bus from source unit to end unit.  Creates an I2CSystemBus object for every full bus path it
 * finds and adds them to o_i2cBusses.  Recursive because an I2C bus can spread out on the other side
 * of a connector to multiple devices.
*/
void mrwI2CWalkI2CBus(I2CBus* i_sourceBus,
                      endpointType i_type,
                      I2CMaster *& i_master,
                      std::vector<I2CConnection*> & i_connections,
                      const std::string i_speed,
                      const std::string i_address,
                      bool i_pd,
                      std::vector<I2CSystemBus*> & o_i2cBusses);

/**
 * Creates a lot of all of the I2C busses in the system, which indicate
 * a path from an I2C Master to an I2C Slave.
 */
void mrwI2CMakeBusses(std::vector<I2CSystemBus*> & o_i2cBusses);


/**
 * Makes the firmware device path string when the I2C bus is a direct attach
 * to the FSP
 *
 * @param - i_master - the I2C master
 * @param - o_iou - set to true if the I2C engine is in the IOU, otherwise false
 */
std::string mrwI2CGetDirectConnectDevicePath(I2CMaster* i_master, bool & o_iou);


class FSISystemBus;

/**
 * Makes the firmware device path for when the I2C bus connects via an FSI bus
 * back to the FSP
 */
std::string mrwI2CGetFsiI2cDevicePath(FSISystemBus* i_fsiBus, I2CMaster* i_i2cMaster);


#endif

