/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwI2CParser.C $                                              */
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
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <algorithm>
#include <xmlutil.H>
#include <mrwParserCommon.H>
#include <mrwI2CCommon.H>
#include <mrwMemCommon.H>
#include <mrwFSICommon.H>
#include <math.h>



/**
 * This program will walk the topology XML and build a separate XML file that just
 * contains information on I2C devices.
 */


using namespace std;



//Globals
FILE* g_fp = NULL;
string g_targetFile;



void usage()
{
    cout << "Usage: mrwI2CParser  --in <full system xml file> --out <output xml file> --pres-out <presence detect output xml file> --targets <ecmd targets xml file> [--strict]\n";
}


int parseArgs(int i_argc, char* i_argv[],
              string & o_inputFile,
              string & o_i2cFile,
              string & o_presFile,
              string & o_targetFile)
{
    string arg;

    if (!((i_argc == 8) || (i_argc == 9) || (i_argc == 10)) )
    {
        cerr << "Invalid number of arguments\n";
        return 1;
    }

    for (int i=1;i<i_argc;i++)
    {
        arg = i_argv[i];

        if (arg == "--in")
            o_inputFile = i_argv[++i];
        else if (arg == "--out")
            o_i2cFile = i_argv[++i];
        else if (arg == "--pres-out")
            o_presFile = i_argv[++i];
        else if (arg == "--targets")
            o_targetFile = i_argv[++i];
        else if (arg == "--strict")
            mrwSetErrMode(MRW_STRICT_ERR_MODE);
        else if (arg == "--debug")
            mrwSetDebugMode(true);
        else
        {
            cerr << "Unrecognized argument: " << arg << endl;
            return 1;
        }
    }

    if ((o_inputFile.empty()) || (o_i2cFile.empty()) ||
        (o_targetFile.empty()) || (o_presFile.empty()))
    {
        cerr << "Missing arguments\n";
        return 1;
    }

    return 0;
}


void getFSIBussesToFSP(I2CMaster* i_master, vector<FSISystemBus*> & o_fsiBusses)
{
    vector<FSISystemBus*>::iterator fsiBus;
    FSIMaster* fsiMaster;
    FSISlave*  fsiSlave;
    static vector<FSISystemBus*> s_allFsiBusses;
    static vector< pair<I2CMaster*, vector<FSISystemBus*> > > s_busCache;
    vector< pair<I2CMaster*, vector<FSISystemBus*> > >::iterator entry;


    if (s_allFsiBusses.empty())
    {
        mrwFsiMakeSystemBusses(s_allFsiBusses);
    }

    //If i_master is on the same plug and has the same id (instance) as something
    //we already found, then we won't have to look again since going
    //through the FSI busses is kinda slow
    for(entry=s_busCache.begin();entry!=s_busCache.end();++entry)
    {
        if ((entry->first->plug() == i_master->plug()) &&
            (entry->first->source().id() == i_master->source().id()))
        {
            o_fsiBusses = entry->second;
            return;
        }
    }


    //Try to find an FSISystemBus that goes from an FSP to the part that is the I2C master

    for (fsiBus=s_allFsiBusses.begin();fsiBus!=s_allFsiBusses.end();++fsiBus)
    {
        fsiMaster = (*fsiBus)->master();
        fsiSlave  = (*fsiBus)->slave();


        //If the fsiMaster is an FSP, but not the FSP<->FSP FSI link
        if ((fsiMaster->partType() == "fsp") && (fsiSlave->partType() != "fsp"))
        {

            //If the fsiSlave is on the same plug(card) as the i2C master,
            //and is also the same chip instance.
            if ((i_master->plug() == fsiSlave->plug()) &&
                (i_master->source().id() == fsiSlave->source().id()))
            {
                o_fsiBusses.push_back(*fsiBus);
            }

        }

    }

    //Save the vector in the cache so we can find it faster next time
    s_busCache.push_back(pair<I2CMaster*, vector<FSISystemBus*> >(i_master, o_fsiBusses));

}


/**
 * Prints all of the path-segment entries to the output XML.
 * the path-segments show the path of each bus through connectors
 * and across cards.
 */
void printI2CPath(I2CSystemBus* i_bus)
{
    I2CMaster* master = i_bus->master();
    I2CSlave* slave = i_bus->slave();
    vector<I2CConnection*>::iterator c;
    vector<FSISystemBus*> fsiBusses;
    string previous;

    fprintf(g_fp, "%s<i2c-path>\n", mrwIndent(4));

    //The source chip is first
    fprintf(g_fp, "%s<i2c-master>%s</i2c-master>\n", mrwIndent(5), master->getChipPath().c_str());

    //The source board is next
    fprintf(g_fp, "%s<cards>\n", mrwIndent(5));

    fprintf(g_fp, "%s<card>", mrwIndent(6));
    fprintf(g_fp, "<instance-path>%s</instance-path>", master->plug()->path().c_str());
    fprintf(g_fp, "<location-code>%s</location-code>", master->plug()->location().c_str());
    previous = master->plug()->path();
    fprintf(g_fp, "</card>\n");

    //Now go through the connections, to print each the connector/card/connector paths
    for (c=i_bus->connections().begin();c!=i_bus->connections().end();++c)
    {
        //Now the card, if we didn't just print it
        if (previous != (*c)->plug()->path())
        {
            fprintf(g_fp, "%s<card>", mrwIndent(6));
            fprintf(g_fp, "<instance-path>%s</instance-path>", (*c)->plug()->path().c_str());
            fprintf(g_fp, "<location-code>%s</location-code>", (*c)->plug()->location().c_str());
            fprintf(g_fp, "</card>\n");
            previous = (*c)->plug()->path().c_str();
        }
    }


    //The destination card, if not already printed
    if (previous != slave->plug()->path())
    {
        fprintf(g_fp, "%s<card>", mrwIndent(6));
        fprintf(g_fp, "<instance-path>%s</instance-path>", slave->plug()->path().c_str());
        fprintf(g_fp, "<location-code>%s</location-code>", slave->plug()->location().c_str());
        fprintf(g_fp, "</card>\n");
    }

    fprintf(g_fp, "%s</cards>\n", mrwIndent(5));

    //The destination chip is last
    fprintf(g_fp, "%s<i2c-slave>%s</i2c-slave>\n", mrwIndent(5), slave->getChipPath().c_str());

    fprintf(g_fp, "%s</i2c-path>\n", mrwIndent(4));

}



bool isHostI2CMaster(I2CMaster* i_master)
{

    //If the master isn't an FSP/CFAM-S, and the engine number isn't listed in the
    //'engine-units' CFAM listing then it must be a host accessed master only

    //FSPs & CFAM-Ss both have an IOU(dio-configs), so check for that
    //if (i_master->part().getChild("dio-configs").empty())
    if (i_master->partType() == "cpu")
    {
        XMLElement engineUnit;
        engineUnit = i_master->part().findPath("internal-units").
                                      findPath("engine-units").
                                      find("engine-unit", "engine", i_master->getEngine());

        if (engineUnit.empty())
        {
            //Not in the CFAM, so a host master
            if (mrwLogger::getDebugMode())
            {
                string m = "Found host I2C Master " + i_master->getUnitPath();
                mrwLogger::debug(m);
            }
            return true;
        }
    }

    return false;
}

/**
 * Some I2C masters are other embedded processors besides the FSP that have their
 * own I2C attached SEEPROMs, like sas-expanders, pcie-retimers, FPGAs, etc.
 * The FSP system model code needs a way to avoid these.
 *
 * @param i_master
 * @return
 */
bool isFspConnected(I2CMaster* i_master)
{
    //these are the only parts that the FSP code can access I2C through
    static const char* FSP_MASTER_PARTS[] = {"fsp", "cfam-s", "membuf", "cpu", NULL};

    if (!isHostI2CMaster(i_master))
    {
        int i = 0;
        do
        {
            if (i_master->partType() == FSP_MASTER_PARTS[i])
            {
                return true;
            }
            i++;
        } while (FSP_MASTER_PARTS[i] != NULL);

        if (mrwLogger::getDebugMode())
        {
            string m = "I2C Master is not FSP connected " + i_master->getUnitPath();
            mrwLogger::debug(m);
        }

    }


    return false;
}



void printSystemPathEntries(I2CSystemBus* i_bus, vector<FSISystemBus*> & i_fsiBusses)
{
    ecmdTarget_t target;
    string loc;
    string path;

    //if the I2C master doesn't connect back to the FSP via FSI,
    //don't need to print a device path, only the I2C path
    if (isHostI2CMaster(i_bus->master()))
    {
        fprintf(g_fp, "%s<system-paths>\n", mrwIndent(2));
        fprintf(g_fp, "%s<system-path>\n", mrwIndent(3));
        printI2CPath(i_bus);
        fprintf(g_fp, "%s</system-path>\n", mrwIndent(3));
        fprintf(g_fp, "%s</system-paths>\n", mrwIndent(2));
        return;
    }

    fprintf(g_fp, "%s<system-paths>\n", mrwIndent(2));


    //If this device connects an via FSI to the FSP, print those paths, otherwise
    //we'll just print the I2C device path for direct attach devices.

    int numPaths = (i_fsiBusses.empty()) ? 1 : i_fsiBusses.size();
    if (mrwLogger::getDebugMode()) {
        string m = "printSystemPathEntries() for FSP I2C master ";
		m += i_bus->master()->getChipPath();
		m += " numPaths=";
		char buf[20];
		sprintf(buf, "%d", numPaths);
		m += buf;
		mrwLogger::debug(m);
    }

    for (int i=0;i<numPaths;i++)
    {
        fprintf(g_fp, "%s<system-path>\n", mrwIndent(3));

        if (!i_fsiBusses.empty())
        {
            FSISystemBus* fsiBus = i_fsiBusses[i];
            vector<FSISingleHopBus*> & singleHopBusses = fsiBus->singleHopBusses();
            vector<FSISingleHopBus*>::iterator shb;
            vector<FSIConnector*>::iterator c;
            string previous;

            for (shb=singleHopBusses.begin();shb!=singleHopBusses.end();++shb)
            {

                ////////////////////////////////////////////////////////
                //Master

                target = mrwGetTarget((*shb)->master()->getChipPath(), g_targetFile);
                loc = (target.node != -1) ? target.location : (*shb)->master()->plug()->location();

                fprintf(g_fp, "%s<fsi-path-segment>\n", mrwIndent(4));
                fprintf(g_fp, "%s<fsi-master>\n", mrwIndent(5));
                fprintf(g_fp, "%s<link>%s</link>", mrwIndent(6), (*shb)->master()->getPort().c_str());
                fprintf(g_fp, "<location-code>%s</location-code>\n", loc.c_str());
                if (target.node != -1)
                {
                    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                            mrwIndent(6), target.name.c_str(), target.node, target.position);
                }

                fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(6), (*shb)->master()->getChipPath().c_str());
                fprintf(g_fp, "%s</fsi-master>\n", mrwIndent(5));


                ////////////////////////////////////////////////////////
                //Intermediate cards

                fprintf(g_fp, "%s<cards>\n", mrwIndent(5));
                previous = "";

                //if no connector in path, still need the card between the chips
                if ((*shb)->connectors().empty())
                {
                    fprintf(g_fp, "%s<card>", mrwIndent(6));
                    fprintf(g_fp, "<instance-path>%s</instance-path>", (*shb)->master()->plug()->path().c_str());
                    fprintf(g_fp, "<location-code>%s</location-code>", (*shb)->master()->plug()->location().c_str());
                    fprintf(g_fp, "</card>\n");
                }

                for (c=(*shb)->connectors().begin();c!=(*shb)->connectors().end();++c)
                {
                    if ((*c)->isCable())
                    {
                        Cable* cbl = ((FSICable*)*c)->cable();
                        if (previous != cbl->element().getChildValue("id"))
                        {
                            fprintf(g_fp, "%s<cable><name>%s</name></cable>\n",
                                    mrwIndent(6), cbl->element().getChildValue("name").c_str());
                            previous = cbl->element().getChildValue("id");
                        }
                    }
                    else if (previous != (*c)->plug()->path())
                    {
                        fprintf(g_fp, "%s<card>", mrwIndent(6));
                        fprintf(g_fp, "<instance-path>%s</instance-path>", (*c)->plug()->path().c_str());
                        fprintf(g_fp, "<location-code>%s</location-code>", (*c)->plug()->location().c_str());
                        fprintf(g_fp, "</card>\n");
                        previous = (*c)->plug()->path();
                    }
                }

                fprintf(g_fp, "%s</cards>\n", mrwIndent(5));

                ////////////////////////////////////////////////////////
                //Slave

                target = mrwGetTarget((*shb)->slave()->getChipPath(), g_targetFile);
                loc = (target.node != -1) ? target.location : (*shb)->slave()->plug()->location();

                fprintf(g_fp, "%s<fsi-slave>\n", mrwIndent(5));
                fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(6), loc.c_str());
                if (target.node != -1)
                {
                    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                            mrwIndent(6), target.name.c_str(), target.node, target.position);
                }

                fprintf(g_fp, "%s<instance-path>%s</instance-path>", mrwIndent(6), (*shb)->slave()->getChipPath().c_str());
                fprintf(g_fp, "<port>%s</port>\n", (*shb)->slave()->getPort().c_str());
                fprintf(g_fp, "%s</fsi-slave>\n", mrwIndent(5));
                fprintf(g_fp, "%s</fsi-path-segment>\n", mrwIndent(4));
            }

            ////////////////////////////////////////////////////////
            //Device Path

            path = mrwI2CGetFsiI2cDevicePath(fsiBus, i_bus->master());

            fprintf(g_fp, "%s<fsp-device-path>%s</fsp-device-path>\n", mrwIndent(4), path.c_str());
        }
        else
        {
            //If it's an FSP, find the path back
            if (i_bus->master()->partType() == "fsp")
            {
                bool iou = false;

                path = mrwI2CGetDirectConnectDevicePath(i_bus->master(), iou);

                //If it is an IOU engine, then the FSI is internal, so master+slave are both FSP
                if (iou)
                {
                    fprintf(g_fp, "%s<fsi-path-segment>\n", mrwIndent(4));

                    fprintf(g_fp, "%s<fsi-master>\n", mrwIndent(5));
                    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(6),
                            i_bus->master()->getChipPath().c_str());
                    fprintf(g_fp, "%s<link>0</link>", mrwIndent(6)); //always link 0
                    fprintf(g_fp, "<location-code>%s</location-code>\n", i_bus->master()->plug()->path().c_str());
                    fprintf(g_fp, "%s</fsi-master>\n", mrwIndent(5));

                    fprintf(g_fp, "%s<fsi-slave>\n", mrwIndent(5));
                    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n",
                            mrwIndent(6), i_bus->master()->getChipPath().c_str());
                    fprintf(g_fp, "%s<port>0</port>\n", mrwIndent(6)); //always port 0
                    fprintf(g_fp, "%s</fsi-slave>\n", mrwIndent(5));

                    fprintf(g_fp, "%s</fsi-path-segment>\n", mrwIndent(4));
                }

                fprintf(g_fp, "%s<fsp-device-path>%s</fsp-device-path>\n", mrwIndent(4), path.c_str());
            }
            else //Not an FSP, and not something FSI connected, so something like an I2C expander
            {
                fprintf(g_fp, "%s<fsp-device-path>See path where this part is the slave</fsp-device-path>\n",
                        mrwIndent(4));
            }
        }

        //////////////////////////////////////////////////////////
        //I2C Path
        printI2CPath(i_bus);

        fprintf(g_fp, "%s</system-path>\n", mrwIndent(3));

    }


    fprintf(g_fp, "%s</system-paths>\n", mrwIndent(2));
}




/**
 * Prints an I2C device entry
 */
void printI2CBus(I2CSystemBus* i_bus)
{
    I2CMaster* master = i_bus->master();
    I2CSlave* slave = i_bus->slave();
    vector<FSISystemBus*> fsiBusses;
    static int count = 0;

    getFSIBussesToFSP(i_bus->master(), fsiBusses);

    ///////////////////////////////////////////////

    string slavePath = slave->getChipPath();
    string address = i_bus->getAddress();
    string content = slave->getContentType();
    string size = slave->getDeviceSize();

    if (address.empty())
    {
        mrwLogger logger;
        logger() << "I2C Address missing for " << slavePath;
        logger.error();
    }


    ///////////////////////////////////////////////

    fprintf(g_fp, "%s<i2c-device>\n", mrwIndent(1));
    fprintf(g_fp, "%s<part-id>%s</part-id>\n", mrwIndent(2), slave->partId().c_str());
    fprintf(g_fp, "%s<part-type>%s</part-type>\n", mrwIndent(2), slave->partType().c_str());
    fprintf(g_fp, "%s<part-num>%s</part-num>\n", mrwIndent(2), slave->part().getChildValue("part-num").c_str());
    fprintf(g_fp, "%s<card-id>%s</card-id>\n", mrwIndent(2), slave->plug()->id().c_str());
    fprintf(g_fp, "%s<card-location-code>%s</card-location-code>\n", mrwIndent(2), slave->plug()->location().c_str());
    fprintf(g_fp, "%s<address>%s</address>\n", mrwIndent(2), address.c_str());
    fprintf(g_fp, "%s<speed>%d</speed>\n", mrwIndent(2), slave->getSpeed());
    fprintf(g_fp, "%s<presence>%d</presence>\n", mrwIndent(2), (i_bus->pd()) ? 1 : 0);

    if (!content.empty())
        fprintf(g_fp, "%s<content-type>%s</content-type>\n", mrwIndent(2), content.c_str());

    if (!size.empty())
        fprintf(g_fp, "%s<size>%s</size>\n", mrwIndent(2), size.c_str());

    ecmdTarget_t target = mrwGetTarget(slavePath, g_targetFile);
    if (target.node != -1)
    {
        fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                mrwIndent(2), target.name.c_str(), target.node, target.position);
    }

    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(2), slavePath.c_str());

    string restriction = master->getRestriction();
    if (!restriction.empty())
    {
	    fprintf(g_fp, "%s<restrict-to-variation-id>%s</restrict-to-variation-id>\n", mrwIndent(2), restriction.c_str());
    }
    else
    {
	    restriction = slave->getRestriction();
	    if (!restriction.empty())
	    {
		    fprintf(g_fp, "%s<restrict-to-variation-id>%s</restrict-to-variation-id>\n", mrwIndent(2), restriction.c_str());
	    }
    }


    ///////////////////////////////////////////////
    //Master

    string masterPath = master->getChipPath();
    target = mrwGetTarget(masterPath, g_targetFile); //master chip target (like a proc/cent)


    fprintf(g_fp, "%s<i2c-master>\n", mrwIndent(2));
    fprintf(g_fp, "%s<part-id>%s</part-id>\n", mrwIndent(3), master->partId().c_str());
    fprintf(g_fp, "%s<i2c-engine>%s</i2c-engine>\n", mrwIndent(3), master->getEngine().c_str());
    fprintf(g_fp, "%s<i2c-port>%s</i2c-port>\n", mrwIndent(3), master->getPort().c_str());
    fprintf(g_fp, "%s<part-type>%s</part-type>\n", mrwIndent(3), master->partType().c_str());
    fprintf(g_fp, "%s<unit-id>%s</unit-id>\n", mrwIndent(3), master->source().unit().c_str());
    fprintf(g_fp, "%s<card-id>%s</card-id>\n", mrwIndent(3), master->plug()->id().c_str());
    if (target.node != -1)
    {
        fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                mrwIndent(3), target.name.c_str(), target.node, target.position);
    }
    fprintf(g_fp, "%s<card-location-code>%s</card-location-code>\n", mrwIndent(3), master->plug()->location().c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), masterPath.c_str());
    fprintf(g_fp, "%s<host-connected>%d</host-connected>\n", mrwIndent(3), isHostI2CMaster(master) ? 1 : 0);
    fprintf(g_fp, "%s<fsp-connected>%d</fsp-connected>\n", mrwIndent(3), isFspConnected(master) ? 1 : 0);
    fprintf(g_fp, "%s</i2c-master>\n", mrwIndent(2));


    ///////////////////////////////////////////////
    //Print the system path segments

    printSystemPathEntries(i_bus, fsiBusses);

    ///////////////////////////////////////////////
    //Need an index that the xslt can use

    fprintf(g_fp, "%s<entry-num>%d</entry-num>\n", mrwIndent(2), count++);
    fprintf(g_fp, "%s</i2c-device>\n", mrwIndent(1));
}


void printPresenceBus(I2CSystemBus* i_bus)
{
    I2CMaster* master = i_bus->master();
    I2CSlave* slave = i_bus->slave();

    fprintf(g_fp, "%s<presence-detect>\n", mrwIndent(1));

    fprintf(g_fp, "%s<method>I2C</method>\n", mrwIndent(2));
    fprintf(g_fp, "%s<fru-instance-path>%s</fru-instance-path>\n", mrwIndent(2), slave->plug()->path().c_str());
    fprintf(g_fp, "%s<fru-location-code>%s</fru-location-code>\n", mrwIndent(2), slave->plug()->location().c_str());
    fprintf(g_fp, "%s<i2c-master>%s</i2c-master>\n", mrwIndent(2), master->getChipPath().c_str());
    fprintf(g_fp, "%s<i2c-engine>%s</i2c-engine>\n", mrwIndent(2), master->getEngine().c_str());
    fprintf(g_fp, "%s<i2c-port>%s</i2c-port>\n", mrwIndent(2), master->getPort().c_str());

    fprintf(g_fp, "%s</presence-detect>\n", mrwIndent(1));
}

bool sortI2CBus(I2CSystemBus* i_left, I2CSystemBus* i_right)
{
    if (i_left->master()->getChipPath() == i_right->master()->getChipPath())
    {
        return (i_left->slave()->getChipPath() < i_right->slave()->getChipPath());

    }
    else
        return (i_left->master()->getChipPath() < i_right->master()->getChipPath());
}


void printI2CBusses(vector<I2CSystemBus*> & i_busses)
{
    if (mrwLogger::getDebugMode()) {
        string m = "Sorting and printing I2C busses"; mrwLogger::debug(m);
    }

    sort(i_busses.begin(), i_busses.end(), sortI2CBus);

    for_each(i_busses.begin(), i_busses.end(), printI2CBus);

    if (mrwLogger::getDebugMode()) {
        string m = "Done sorting and printing I2C busses"; mrwLogger::debug(m);
    }
}


void printPresenceBusses(vector<I2CSystemBus*> & i_busses)
{
    vector<I2CSystemBus*>::iterator b;

    for (b=i_busses.begin();b!=i_busses.end();++b)
    {
        if ((*b)->pd())
            printPresenceBus(*b);
    }

}



int main(int argc, char* argv[])
{
    string inputFile, i2cFile, presFile;

    if (parseArgs(argc, argv, inputFile, i2cFile, presFile, g_targetFile))
    {
        usage();
        return 1;
    }

    if (SystemXML::load(inputFile))
        return 1;

    vector<I2CSystemBus*> busses;

    mrwI2CMakeBusses(busses);

    ///////////////////////////////////////////////
    //I2C file

    g_fp = mrwPrintHeader(i2cFile, "i2c-devices", "mrwi2c");
    if (!g_fp)
        return 1;

    printI2CBusses(busses);

    mrwPrintFooter("i2c-devices", g_fp);

    //////////////////////////////////////////////
    //I2C Presence file

    g_fp = mrwPrintHeader(presFile, "presence-detects", "mrwipres");
    if (!g_fp)
        return 1;

    printPresenceBusses(busses);

    mrwPrintFooter("presence-detects", g_fp);


    return mrwGetReturnCode();
}



