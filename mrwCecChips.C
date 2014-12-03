/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwCecChips.C $                                               */
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
#include <algorithm>
#include <mrwFSICommon.H>


/**
 * Provides CEC chip information, such as:
 *
 *      Prints out XML that contains the processor and centaur scan,scom,mbox FSP device paths.
 *      The FSP path looks something like:
 *
 *            /dev/scan/L02C0E11:L0C0E10:L0C0E01P00
 */


using namespace std;



FILE* g_fp = NULL;
string g_targetFile;


void usage()
{
    cout << "Usage: mrwCecChips --in <full system xml file> --out <output xml file> --targets <ecmd targets xml file>\n";
}


/**
 * Returns the segment of the fsp device path that is based on the FSI path from the target
 * back to the FSP.  See mrwFsiGetDevicePathSubString on the content details.
 */
string getFsiPathSegments(const ecmdTarget_t & i_target, const char i_a_or_b)
{
    static vector<FSISystemBus*> s_fsiBusses;
    vector<FSISystemBus*>::iterator b;
    FSIMaster* master;
    string path;

    //The first time through, make all the FSI busses
    if (s_fsiBusses.empty())
    {
        mrwFsiMakeSystemBusses(s_fsiBusses);
    }


    for (b=s_fsiBusses.begin();b!=s_fsiBusses.end();++b)
    {
        master = (*b)->master();

        //If the master is on the FSP, see if the slave matches our target
        if (master->partType() == "fsp")
        {
            if ( (mrwGetFspAorB(master->plug()) == i_a_or_b) &&
                 ((*b)->slave()->getChipPath() == i_target.instancePath) )
            {
                //build the possibly multi-hop device path
                path = mrwFsiGetDevicePathSubString(*b);
                break;
            }
        }
    }

    if (path.empty() && (i_a_or_b == 'A')) // the B path can be empty on non-redundant platforms
    {
        mrwLogger logger;
        logger() << "No FSI path found back to the FSP from " << i_target.instancePath << " (FSP-" << i_a_or_b << ")";
        logger.error(true);
    }


    return path;
}




/**
 * Gets the engine # for the engineType passed in on the target
 */
string getEngine(const ecmdTarget_t & i_target, const string & i_engineType)
{
    string engine;

    //First, get the part instance element from the part XPath in the target
    XMLElement instance = mrwGetRoot().findXPath(mrwGetTree(), i_target.partXPath);

    if (!instance.empty())
    {
        //Get the part element

        XMLElement part = mrwGetRoot().findPath("layout/parts").find("part", "id", instance.getChildValue("part-id"));

        if (!part.empty())
        {
            //get the correct engine-unit internal-unit and return the engine element value

            XMLElement units = part.findPath("internal-units/engine-units");

            XMLElement e = units.find("engine-unit", "id", i_engineType);

            engine = e.getChildValue("engine");

        }
        else
        {
            mrwLogger logger;
            logger() << "getEngine: Could not find part from part-xpath for target " << i_target.instancePath;
            logger.error(true);
        }

    }
    else
    {
        mrwLogger logger;
        logger() << "getEngine: Could not find part instance from part-xpath for target " << i_target.instancePath;
        logger.error(true);
    }


    return engine;
}



/**
 * Returns the full FSP device path for the mailbox engine on the target
 */
string getMailboxPath(const ecmdTarget_t & i_target, const char i_a_or_b)
{
    string path;
    string engine = getEngine(i_target, "mailbox");

    //not every chip may have a mailbox engine, in that case return ''
    if (!engine.empty())
    {

        string fsipathseg = getFsiPathSegments(i_target, i_a_or_b);
        if (!fsipathseg.empty())
        {
        	path = "/dev/mbx/";
	        path += fsipathseg;
	        path += "E" + mrwPadNumToTwoDigits(engine);

	        //always port 0
	        path += "P00";
		}
    }

    return path;
}

/**
 * Returns the full FSP device path for the scom engine on the target
 */
string getScomPath(const ecmdTarget_t & i_target, const char i_a_or_b)
{
    string path;
    string fsipathseg = getFsiPathSegments(i_target, i_a_or_b);

	if (!fsipathseg.empty())
	{
    	path = "/dev/scom/";
	    path += fsipathseg;

	    //offical name for the scom engine is fsi2pib
	    path += "E" + mrwPadNumToTwoDigits(getEngine(i_target, "fsi2pib"));

	    //always port 0
	    path += "P00";
	}

    return path;
}



/**
 * Returns the full FSP device path for the scan engine on the target
 */
string getScanPath(const ecmdTarget_t & i_target, const char i_a_or_b)
{
    string path;
    string fsipathseg = getFsiPathSegments(i_target, i_a_or_b);

	if (!fsipathseg.empty())
	{
	    path = "/dev/scan/";
	    path += getFsiPathSegments(i_target, i_a_or_b);

	    //official name for scan engine is 'shift'
	    path += "E" + mrwPadNumToTwoDigits(getEngine(i_target, "shift"));
	
	    //always port 0
	    path += "P00";
	}

    return path;
}


/**
 * Returns the instance-path of the FRU (parent card).
 * Since we're not using Plug objects, need to get the
 * path by chopping the chip off the target instance-path
 */
string getFruPath(const ecmdTarget_t & i_target)
{
    int pos = i_target.instancePath.find_last_of('/');
    string path = i_target.instancePath.substr(0, pos);

    return path;
}



/**
 * Prints the XML for an entry
 */
void printPathXml(const ecmdTarget_t & i_target)
{
    string indent1(4, ' ');
    string indent2(8, ' ');
    string scomPath_a;
    string scomPath_b;
    string scanPath_a;
    string scanPath_b;
    string mboxPath_a;
    string mboxPath_b;
    string loc;

#ifndef MRW_OPENPOWER
    scomPath_a = getScomPath(i_target, 'A');
    scomPath_b = getScomPath(i_target, 'B');
    scanPath_a = getScanPath(i_target, 'A');
    scanPath_b = getScanPath(i_target, 'B');
    mboxPath_a = getMailboxPath(i_target, 'A');
    mboxPath_b = getMailboxPath(i_target, 'B');
    loc = target.location;
#endif

    if (mboxPath_a.empty())
        mboxPath_a = "n/a";
    if (mboxPath_b.empty())
        mboxPath_b = "n/a";

    fprintf(g_fp, "%s<chip>\n", indent1.c_str());

    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
            indent2.c_str(), i_target.name.c_str(), i_target.node, i_target.position);

    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", indent2.c_str(), i_target.instancePath.c_str());
    fprintf(g_fp, "%s<fru-instance-path>%s</fru-instance-path>\n", indent2.c_str(), getFruPath(i_target).c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", indent2.c_str(), loc.c_str());

    fprintf(g_fp, "%s<scom-path-a>%s</scom-path-a>\n", indent2.c_str(), scomPath_a.c_str());
    fprintf(g_fp, "%s<scom-path-b>%s</scom-path-b>\n", indent2.c_str(), scomPath_b.c_str());
    fprintf(g_fp, "%s<scan-path-a>%s</scan-path-a>\n", indent2.c_str(), scanPath_a.c_str());
    fprintf(g_fp, "%s<scan-path-b>%s</scan-path-b>\n", indent2.c_str(), scanPath_b.c_str());
    fprintf(g_fp, "%s<mailbox-path-a>%s</mailbox-path-a>\n", indent2.c_str(), mboxPath_a.c_str());
    fprintf(g_fp, "%s<mailbox-path-b>%s</mailbox-path-b>\n", indent2.c_str(), mboxPath_b.c_str());


    fprintf(g_fp, "%s</chip>\n", indent1.c_str());
}



/**
 * Used to sort the targets based on name, and then node and then position
 */
bool sortTargets(const ecmdTarget_t & i_left, const ecmdTarget_t & i_right)
{

    if (i_left.name == i_right.name)
    {
        if (i_left.node == i_right.node)
        {
            return i_left.position < i_right.position;
        }
        else
            return i_left.node < i_right.node;
    }
    else
        return i_left.name < i_right.name;

}



/**
 * High level function to print the XML
 */
void printPaths(vector<ecmdTarget_t> & i_targets)
{
    sort(i_targets.begin(), i_targets.end(), sortTargets);

    if (mrwLogger::getDebugMode()) {
        string m = "Printing CEC chips"; mrwLogger::debug(m);
    }

    for_each(i_targets.begin(), i_targets.end(), printPathXml);
}



/**
 * Load the pu & memb targets into the target file
 */
void readTargets(vector<ecmdTarget_t> & o_targets)
{
    vector<ecmdTarget_t> membufs;

    //Get the processor targets
    mrwGetTargets("pu", g_targetFile, o_targets, true);

    //Get the memory buffer targets
    mrwGetTargets("memb", g_targetFile, membufs, true);

    o_targets.insert(o_targets.end(), membufs.begin(), membufs.end());
}



int main(int argc, char* argv[])
{
    string inputFile, outputFile;
    vector<ecmdTarget_t> targets;

    if (mrwParseArgs(argc, argv, inputFile, outputFile, g_targetFile))
    {
        usage();
        return 1;
    }

    if (mrwLoadGlobalTreeAndRoot(inputFile))
        return 1;

    g_fp = mrwPrintHeader(outputFile, "chips", "mrwpaths");
    if (!g_fp) return 1;

    readTargets(targets);

    printPaths(targets);

    mrwPrintFooter("chips", g_fp);

    return mrwGetReturnCode();
}

