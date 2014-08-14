/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwFSIParser.C $                                              */
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
#include <iostream>
#include <math.h>
#include <algorithm>
#include <xmlutil.H>
#include <mrwParserCommon.H>
#include <mrwFSICommon.H>


/**
 * This program will walk the topology XML and build a separate XML file that just
 * contains information on FSI busses.
 */

using namespace std;

//Globals
string g_targetFile;
FILE* g_fp = NULL;



void usage()
{
    cout << "Usage: mrwFSIParser  --in <full system xml file> --out <output xml file> --pres-out <presence detect xml file> --targets <ecmd targets xml file> \n";
}


/**
 * Parses arguments
 */
int parseArgs(int i_argc, char* i_argv[],
              string & o_inputFile,
              string & o_fsiFile,
              string & o_presFile,
              string & o_targetFile)
{
    string arg;

    for (int i=1;i<i_argc;i++)
    {
        arg = i_argv[i];


        if (arg == "--in")
            o_inputFile = i_argv[++i];
        else if (arg == "--out")
            o_fsiFile = i_argv[++i];
        else if (arg == "--strict")
            mrwSetErrMode(MRW_STRICT_ERR_MODE);
        else if (arg == "--pres-out")
            o_presFile = i_argv[++i];
        else if (arg == "--targets")
            o_targetFile = i_argv[++i];
        else if (arg == "--debug")
            mrwSetDebugMode(true);
        else
        {
            cerr << "Unrecognized argument: " << arg << endl;
            return 1;
        }
    }

    if (o_inputFile.empty() || o_fsiFile.empty() || o_targetFile.empty() || o_presFile.empty())
    {
        cerr << "Missing arguments\n";
        return 1;
    }


    return 0;
}


/**
 * Helper class to print an FSI Bus
 */
class PrintFSIBus
{
public:
    PrintFSIBus(vector<FSISystemBus*> & i_allBusses) :
        iv_busses(i_allBusses) {}

    void operator()(FSISystemBus* i_bus);
    int isPresenceDetect(FSISystemBus* i_bus);
    string getFspDevicePath(FSISlave* i_slave);

private:
    vector<FSISystemBus*> & iv_busses;
};


/**
 * Find the FSISystemBus that has the master as the fsp and the slave that matches
 * the passed in slave, and then build and return the FSI device path segment of the
 * fsp device path.
 */
string PrintFSIBus::getFspDevicePath(FSISlave* i_slave)
{
    vector<FSISystemBus*>::iterator b;
    FSIMaster* master;
    string path;

    for (b=iv_busses.begin();b!=iv_busses.end();++b)
    {
        master = (*b)->master();

        //look for the bus that has the slave that matches this one and the master as the FSP
        if (master->partType() == "fsp")
        {
            if (i_slave == (*b)->slave())
            {

                //if already found one, add a ','
                if (!path.empty())
                    path += ", ";

                //build the possibly multi-hop device path
                path += mrwFsiGetDevicePathSubString(*b);
            }
        }
    }

    return path;
}



/**
 * Says if the bus should be used for presence detect by the PRES FSP component
 */
int PrintFSIBus::isPresenceDetect(FSISystemBus* i_bus)
{
    static vector<FSISystemBus*> s_presBusses;
    static vector<FSISystemBus*>::iterator b;

    if (s_presBusses.empty())
    {
        mrwFsiGetPresenceDetectBusses(iv_busses, s_presBusses);
    }

    b = find(s_presBusses.begin(), s_presBusses.end(), i_bus);

    if (b != s_presBusses.end())
    {
        if (mrwLogger::getDebugMode()) {
            string m;
            m = "Found presence detect bus " + i_bus->master()->getChipPath() +
                " to " + i_bus->slave()->getChipPath();
            mrwLogger::debug(m);
        }
        return true;
    }

    return false;
}

/**
 * Does the printing.
 */
void PrintFSIBus::operator()(FSISystemBus* i_bus)
{
    //For this we just need the single segment busses
    if (i_bus->numHops() != 1)
        return;

    FSIMaster* master = i_bus->master();
    FSISlave* slave = i_bus->slave();
    string fspPath, type, cableConnPath;


    ecmdTarget_t target = mrwGetTarget(master->getChipPath(), g_targetFile);
    string loc = (target.node != -1) ? target.location : master->plug()->location();


    if (master->cascadedMaster())
    {
        type = "Cascaded Master";
    }
    else
    {
        XMLElement part = mrwGetPart(mrwGetPartId(master->plug()->card(),
                                                  master->source().id()));

        if (part.getChildValue("part-type") == "cpu")
            type = "Hub Master";
        else
            type = "FSP Master";
    }

 
    fprintf(g_fp, "%s<fsi-bus>\n", mrwIndent(1));

    //////////////////////////////////////////////////
    //Master

    fprintf(g_fp, "%s<master>\n", mrwIndent(2));
    fprintf(g_fp, "%s<part-id>%s</part-id>\n", mrwIndent(3), master->partId().c_str());
    fprintf(g_fp, "%s<unit-id>%s</unit-id>\n", mrwIndent(3), master->source().unit().c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), master->getChipPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), loc.c_str());

    //only print target if it has one
    if (target.node != -1)
    {
        fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                mrwIndent(3), target.name.c_str(), target.node, target.position);
    }

    fprintf(g_fp, "%s<engine>%s</engine>\n", mrwIndent(3), master->getEngine().c_str());
    fprintf(g_fp, "%s<link>%s</link>\n", mrwIndent(3), master->getPort().c_str());
    fprintf(g_fp, "%s<type>%s</type>\n", mrwIndent(3), type.c_str());

    cableConnPath = i_bus->getCableConnPath(FSISingleHopBus::FSI_MASTER);
    fprintf(g_fp, "%s<cable-connector-instance-path>%s</cable-connector-instance-path>\n",
            mrwIndent(3), cableConnPath.c_str());

    fprintf(g_fp, "%s</master>\n", mrwIndent(2));


    /////////////////////////////////////////////////
    //Slave

    target = mrwGetTarget(slave->getChipPath(), g_targetFile);
    loc = (target.node != -1) ? target.location : master->plug()->location();
    fspPath = getFspDevicePath(slave);

    fprintf(g_fp, "%s<slave>\n", mrwIndent(2));
    fprintf(g_fp, "%s<part-id>%s</part-id>\n", mrwIndent(3), slave->partId().c_str());
    fprintf(g_fp, "%s<unit-id>%s</unit-id>\n", mrwIndent(3), slave->source().unit().c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), slave->getChipPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), loc.c_str());

    //only print target if it has one
    if (target.node != -1)
    {
        fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                mrwIndent(3), target.name.c_str(), target.node, target.position);
    }

    fprintf(g_fp, "%s<fsp-device-path-segments>%s</fsp-device-path-segments>\n", mrwIndent(3), fspPath.c_str());
    fprintf(g_fp, "%s<port>%s</port>\n", mrwIndent(3), slave->getPort().c_str());

    cableConnPath = i_bus->getCableConnPath(FSISingleHopBus::FSI_SLAVE);
    fprintf(g_fp, "%s<cable-connector-instance-path>%s</cable-connector-instance-path>\n",
            mrwIndent(3), cableConnPath.c_str());

    fprintf(g_fp, "%s</slave>\n", mrwIndent(2));

    ////////////////////////////////////////////////////
    //if used for presence-detect

    fprintf(g_fp, "%s<presence>%d</presence>\n", mrwIndent(2), isPresenceDetect(i_bus));

    fprintf(g_fp, "%s<cable-name>%s</cable-name>\n", mrwIndent(2), i_bus->getCableName().c_str());

    fprintf(g_fp, "%s</fsi-bus>\n", mrwIndent(1));
    
}



bool sortByTarget(FSISystemBus* i_bus1, FSISystemBus* i_bus2)
{
    ecmdTarget_t t1 = mrwGetTarget(i_bus1->master()->getChipPath(), g_targetFile);
    ecmdTarget_t t2 = mrwGetTarget(i_bus2->master()->getChipPath(), g_targetFile);
    bool sortSlave = false;

    if (t1.node < t2.node)
    {
        return true;
    }
    else if (t1.node == t2.node)
    {
        if (t1.position < t2.position)
        {
            return true;
        }
        else if (t1.position == t2.position)
        {
            //masters are the same, now do the slave
            sortSlave = true;
        }
    }

    //masters are the same, so now look at the slave
    //could be different device types, so lets look at chip path instead
    if (sortSlave)
    {
        return (i_bus1->slave()->getChipPath() < i_bus2->slave()->getChipPath());
    }


    return false;
}



void printFSIBusses(vector<FSISystemBus*> & i_busses)
{
    if (mrwLogger::getDebugMode) {
        string m = "Sorting FSI busses"; mrwLogger::debug(m);
    }

    sort(i_busses.begin(), i_busses.end(), sortByTarget);

    if (mrwLogger::getDebugMode) {
        string m = "Done sorting FSI busses"; mrwLogger::debug(m);
    }

    if (mrwLogger::getDebugMode) {
        string m = "Printing FSI busses"; mrwLogger::debug(m);
    }

    for_each(i_busses.begin(), i_busses.end(), PrintFSIBus(i_busses));

    if (mrwLogger::getDebugMode) {
        string m = "Done printing FSI busses"; mrwLogger::debug(m);
    }
}



void printFsiPresenceDetect(FSISystemBus* i_fsi)
{
    FSIMaster* master = i_fsi->master();
    FSISlave* slave   = i_fsi->slave();

    fprintf(g_fp, "%s<presence-detect>\n", mrwIndent(1));
    fprintf(g_fp, "%s<method>FSI</method>\n", mrwIndent(2));
    fprintf(g_fp, "%s<fru-instance-path>%s</fru-instance-path>\n", mrwIndent(2), slave->plug()->path().c_str());
    fprintf(g_fp, "%s<fru-location-code>%s</fru-location-code>\n", mrwIndent(2), slave->plug()->location().c_str());

    fprintf(g_fp, "%s<fsi-slave-instance-path>%s</fsi-slave-instance-path>\n", mrwIndent(2), slave->getChipPath().c_str());
    fprintf(g_fp, "%s<fsi-master-instance-path>%s</fsi-master-instance-path>\n",
            mrwIndent(2), master->getChipPath().c_str());
    fprintf(g_fp, "%s<fsi-master-location-code>%s</fsi-master-location-code>\n",
            mrwIndent(2), master->plug()->location().c_str());

    fprintf(g_fp, "%s<fsi-link>%s</fsi-link>\n", mrwIndent(2), master->getPort().c_str());
    fprintf(g_fp, "%s<fsi-engine>%s</fsi-engine>\n", mrwIndent(2), master->getEngine().c_str());

    fprintf(g_fp, "%s</presence-detect>\n", mrwIndent(1));
}


void printPresenceDetects(vector<FSISystemBus*> & i_busses)
{
    for_each(i_busses.begin(), i_busses.end(), printFsiPresenceDetect);
}


int main(int argc, char* argv[])
{
    string inputFile, outputFile, presFile;

    if (parseArgs(argc, argv, inputFile, outputFile, presFile, g_targetFile))
    {
        usage();
        return 1;
    }

    if (SystemXML::load(inputFile))
           return 1;

    vector<FSISystemBus*> busses;
    vector<FSISystemBus*> presBusses;
    mrwFsiMakeSystemBusses(busses);


    ///////////////////////////////////////////////
    //FSI Bus File

    g_fp = mrwPrintHeader(outputFile, "fsi-busses", "mrwfsi");
    if (!g_fp)
        return 1;

    printFSIBusses(busses);

    mrwPrintFooter("fsi-busses", g_fp);


    ///////////////////////////////////////////////
    //FSI Presence Detect File

    g_fp = mrwPrintHeader(presFile, "presence-detects", "mrwfpres");
    if (!g_fp)
        return 1;

    mrwFsiGetPresenceDetectBusses(busses, presBusses);

    printPresenceDetects(presBusses);

    mrwPrintFooter("presence-detects", g_fp);


    return mrwGetReturnCode();
}

