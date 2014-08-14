/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwMemParser.C $                                              */
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
#include <algorithm>
#include <mrwParserCommon.H>
#include <mrwMemCommon.H>
#include <mrwFSICommon.H>

/**
 * This program will walk the topology XML and build a separate XML file that just
 * contains information on the mainstore memory paths.
 */

using namespace std;



//Globals
FILE* g_fp = NULL;
string g_targetFile;




void usage()
{
    cout << "Usage: mrwMemParser  --in <full system xml file> --out <output xml file> --targets <ecmd targets xml file> \n";
}




/**
 * Finds the FSI link between the processor and centaur
 */
string getFsiLink(MemSystemBus* i_bus)
{
    string link;
    vector<FSISystemBus*>::iterator fsiBus;
    static vector<FSISystemBus*> fsiBusses;
    MemMcs* mcs = i_bus->mcs();
    MemMba* mba = i_bus->mba();
    FSIMaster* fsiMaster;
    FSISlave*  fsiSlave;


    if (fsiBusses.empty())
    {
        mrwFsiMakeSystemBusses(fsiBusses);
    }

    for (fsiBus=fsiBusses.begin();fsiBus!=fsiBusses.end();++fsiBus)
    {
        fsiMaster = (*fsiBus)->master();
        fsiSlave  = (*fsiBus)->slave();

        //Check if the FSI master matches the MCS (processor)
        if ((mcs->plug() == fsiMaster->plug()) && (mcs->mcsEndpoint().id() == fsiMaster->source().id()))
        {
            //Check if the FSI slave matches the MBA (centaur)
            if ((mba->plug() == fsiSlave->plug()) && (mba->mbaEndpoint().id() == fsiSlave->source().id()))
            {
                link = fsiMaster->getPort();
                break;
            }
        }

    }


    return link;
}


/**
 * Prints a memory-bus entry
 */
void printMemoryBus(MemSystemBus* i_bus)
{
    MemDram* dram = i_bus->dram();
    MemMcs* mcs = i_bus->mcs();
    MemMba* mba = i_bus->mba();
    string path;
    ecmdTarget_t target;
    static vector<string> dimmPaths;

    //If the MBA(centaur) is on the same plug as the DRAM, then we have a centaur-dimm.
    bool cdimm = (mba->plug() == dram->plug());


    //The memory busses connect up to a DRAM part.  On JEDEC DIMMs, this part is on a unique DIMM.
    //On centaur DIMMs, there will be multiple DRAM parts on the same DIMM. (There are
    //multiple Centaur DDR busses coming out of the MBAs connected up to different DRAM chips on
    //the DIMM.)  So, on Centaur DIMMs, we can skip the MBA port/slot columns and also we have
    //to make sure we just display a DIMM once since we'll have multiple memory busses for that
    //same DIMM.


    path = mrwMemGetDimmInstancePath(dram->plug());

    if (find(dimmPaths.begin(), dimmPaths.end(), path) == dimmPaths.end())
        dimmPaths.push_back(path);
    else
        return; //we've already done this with another memory bus



    /* Will look like:
    <memory-bus>
           <mcs>
               <location-code></location-code>
               <instance-path></instance-path>
               <target><name></name><node></node><position></position><chipUnit></chipUnit></target>
           </mcs>
           <mba>
               <location-code></location-code>
               <mba-port></mba-port>  //non-CDIMM only
               <mba-slot></mba-slot>  //non-CDIMM only
               <instance-path></instance-path>
               <target><name></name><node></node><position></position><chipUnit></chipUnit></target>
           </mba>
           <dimm>
               <location-code></location-code>
               <instance-path></instance-path>
               <target><name></name><node></node><position></position></target>
           </dimm>
           <fsi-link></fsi-link> //FSI link from proc to cent
       </memory-bus>
     */


    fprintf(g_fp, "%s<memory-bus>\n", mrwIndent(1));

    /////////////////////////////////////////////////////////////
    //The MCS

    path = mcs->path();
    target = mrwGetTarget(path, g_targetFile);

    fprintf(g_fp, "%s<mcs>\n", mrwIndent(2));
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), target.location.c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), path.c_str());
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position><chipUnit>%d</chipUnit></target>\n",
            mrwIndent(3), target.name.c_str(), target.node, target.position, target.chipUnit);
    fprintf(g_fp, "%s</mcs>\n", mrwIndent(2));

    /////////////////////////////////////////////////////////////
    //The MBA

    //On CDIMMs, will just list 1 of the 2 MBAs even though they're both probably connected


    path = mba->path();
    target = mrwGetTarget(path, g_targetFile);

    fprintf(g_fp, "%s<mba>\n", mrwIndent(2));
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), target.location.c_str());

    //MBA port and slot only applicable on CDIMMs
    if (!cdimm)
    {
        string port = mrwMemGetMbaPort(mba->mbaEndpoint(), mba->plug());
        string slot = mrwMemGetMbaSlot(mba->mbaEndpoint(), mba->plug());

        fprintf(g_fp, "%s<mba-port>%s</mba-port>\n", mrwIndent(3), port.c_str());
        fprintf(g_fp, "%s<mba-slot>%s</mba-slot>\n", mrwIndent(3), slot.c_str());
    }


    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), path.c_str());
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position><chipUnit>%d</chipUnit></target>\n",
            mrwIndent(3), target.name.c_str(), target.node, target.position, target.chipUnit);

    fprintf(g_fp, "%s</mba>\n", mrwIndent(2));



    /////////////////////////////////////////////////////////////
    //The DIMM

    path = mrwMemGetDimmInstancePath(dram->plug());
    target = mrwGetTarget(path, g_targetFile);

    fprintf(g_fp, "%s<dimm>\n", mrwIndent(2));
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), dram->plug()->location().c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), path.c_str());
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
            mrwIndent(3), target.name.c_str(), target.node, target.position);
    fprintf(g_fp, "%s</dimm>\n", mrwIndent(2));

    ////////////////////////////////////////////////////////////
    //The FSI connection from the proc to the centaur

    string link = getFsiLink(i_bus);

    fprintf(g_fp, "%s<fsi-link>%s</fsi-link>\n", mrwIndent(2), link.c_str());

    fprintf(g_fp, "%s</memory-bus>\n", mrwIndent(1));

}


void printDRAM(MemSystemBus* i_bus)
{
    MemDram* dram = i_bus->dram();
    MemMba* mba = i_bus->mba();

    string port = mrwMemGetMbaPort(mba->mbaEndpoint(), mba->plug());
    string slot = mrwMemGetMbaSlot(mba->mbaEndpoint(), mba->plug());
    string path = mrwMemGetDimmInstancePath(dram->plug());
	assembly_t dram_asm = dram->plug()->assembly();

    fprintf(g_fp, "%s<dram>\n", mrwIndent(2));
    fprintf(g_fp, "%s<dimm-instance-path>%s</dimm-instance-path>\n", mrwIndent(3), path.c_str());
    fprintf(g_fp, "%s<mba-instance-path>%s</mba-instance-path>\n", mrwIndent(3), mba->path().c_str());
    fprintf(g_fp, "%s<dram-instance-path>%s</dram-instance-path>\n", mrwIndent(3), dram->path().c_str());

    fprintf(g_fp, "%s<mba-port>%s</mba-port>\n", mrwIndent(3), port.c_str());
    fprintf(g_fp, "%s<mba-slot>%s</mba-slot>\n", mrwIndent(3), slot.c_str());

    fprintf(g_fp, "%s<assembly-position>%s</assembly-position>\n", mrwIndent(3), dram_asm.position.c_str());

    fprintf(g_fp, "%s</dram>\n", mrwIndent(2));
}


bool sortByMcsTarget(MemSystemBus* i_left, MemSystemBus* i_right)
{
    ecmdTarget_t t1 = mrwGetTarget(i_left->mcs()->path(), g_targetFile);
    ecmdTarget_t t2 = mrwGetTarget(i_right->mcs()->path(), g_targetFile);

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
            if (t1.chipUnit < t2.chipUnit)
                return true;
        }
    }

    return false;
}



void printMemoryBusses(vector<MemSystemBus*> & i_busses)
{
    sort(i_busses.begin(), i_busses.end(), sortByMcsTarget);

    for_each(i_busses.begin(), i_busses.end(), printMemoryBus);
}

void printDRAMs(vector<MemSystemBus*> & i_busses)
{
    fprintf(g_fp, "%s<drams>\n", mrwIndent(1));
    for_each(i_busses.begin(), i_busses.end(), printDRAM);
    fprintf(g_fp, "%s</drams>\n", mrwIndent(1));
}


int main(int argc, char* argv[])
{
    string inputFile, outputFile;

    if (mrwParseArgs(argc, argv, inputFile, outputFile, g_targetFile))
    {
        usage();
        return 1;
    }

    if (SystemXML::load(inputFile))
        return 1;


    vector<MemSystemBus*> busses;
    mrwMemMakeBusses(busses);

    g_fp = mrwPrintHeader(outputFile, "memory-busses", "mrwmem");
    if (!g_fp)
        return 1;

    printMemoryBusses(busses);

    printDRAMs(busses);

    mrwPrintFooter("memory-busses", g_fp);

    return mrwGetReturnCode();
}


