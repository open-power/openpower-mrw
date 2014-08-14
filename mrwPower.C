/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPower.C $                                                  */
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
#include <xmlutil.H>
#include <mrwPowerCommon.H>

/**
 * Creates power connection XML based on the information in the system XML, including the
 * centaur<->VRD connection information for hostboot/HWSV.
 */

using namespace std;



FILE* g_fp = NULL;
string g_targetFile;
string g_i2cFile;



struct connI2C_t
{
    PowerConnection* connection;
    string address;
    vector<string> paths;
    ecmdTarget_t target;
};

static bool multinode = false;


void usage()
{
    cout << "Usage: mrwPower --in <full system xml file> --out <output xml file> --vmem-out <output vmem xml file> --targets <targets xml file> --i2c <system i2c xml file>\n";
}



/**
 * Parses arguments
 */
int parseArgs(int i_argc, char* i_argv[],
              string & o_inputFile,
              string & o_outputFile,
              string & o_targetFile,
              string & o_vrdFile,
              string & o_i2cFile)
{
    string arg;

    for (int i=1;i<i_argc;i++)
    {
        arg = i_argv[i];


        if (arg == "--in")
            o_inputFile = i_argv[++i];
        else if (arg == "--out")
            o_outputFile = i_argv[++i];
        else if (arg == "--strict")
            mrwSetErrMode(MRW_STRICT_ERR_MODE);
        else if (arg == "--targets")
            o_targetFile = i_argv[++i];
        else if (arg == "--i2c")
            o_i2cFile = i_argv[++i];
        else if (arg == "--cent-vrd-out")
        {
            o_vrdFile = i_argv[++i];
        }
        else if (arg == "--debug")
        {
            mrwSetDebugMode(true);
        }
        else
        {
            cerr << "Unrecognized argument: " << arg << endl;
            return 1;
        }
    }

    if ((o_inputFile.empty()) || (o_outputFile.empty()) || (o_targetFile.empty()) || (o_vrdFile.empty()))
    {
        cerr << "Missing arguments\n";
        return 1;
    }


    return 0;
}


/**
 * print a power connection - for debug
 */
void printPowerConnection(PowerConnection* i_connection)
{
    fprintf(g_fp, "%s<power-connection>\n", mrwIndent(1));
    string sourcePath = mrwUnitPath(i_connection->source()->plug(), i_connection->source()->source());
    string sinkPath = mrwUnitPath(i_connection->sink()->plug(), i_connection->sink()->source());
    fprintf(g_fp, "%s<source>%s</source>\n", mrwIndent(2), sourcePath.c_str());
    fprintf(g_fp, "%s<sink>%s</sink>\n", mrwIndent(2), sinkPath.c_str());
    fprintf(g_fp, "%s</power-connection>\n", mrwIndent(1));
}



bool sortConnection(PowerConnection* i_left, PowerConnection* i_right)
{

    if (i_left->source()->getChipPath() == i_right->source()->getChipPath())
    {
        return i_left->sink()->getChipPath() <= i_right->sink()->getChipPath();

    }
    else
        return i_left->source()->getChipPath() < i_right->source()->getChipPath();

}


bool compareConnections(PowerConnection* i_left, PowerConnection* i_right)
{
    return (i_left->source()->getChipPath() == i_right->source()->getChipPath()) &&
           (i_left->sink()->getChipPath() == i_right->sink()->getChipPath());
}

void printPowerConnections(vector<PowerConnection*> & i_connections)
{
    sort(i_connections.begin(), i_connections.end(), sortConnection);

    //there's a serverwiz bug where some duplicates show up, so remove them
    size_t size = i_connections.size();
    vector<PowerConnection*>::iterator newEnd;

    newEnd = unique(i_connections.begin(), i_connections.end(), compareConnections);
    i_connections.erase(newEnd, i_connections.end());

    if (i_connections.size() != size)
    {
        mrwLogger l;
        l() << "Removed " << size - i_connections.size() << " duplicate power connections\n";
        l.info();
    }

    for_each(i_connections.begin(), i_connections.end(), printPowerConnection);
}



bool isCentaurVrd(PowerSink* i_sink, const char* vrd_type)
{
	string chk_vrd_type(vrd_type);
    if (chk_vrd_type == "VCS")
    	chk_vrd_type = "VCACHE";
    else if (chk_vrd_type == "VDD")
    	chk_vrd_type = "VCORE";

    return ((i_sink->part().getChildValue("part-type") == "membuf") &&
            (i_sink->source().unit() == chk_vrd_type));
}



/**
 * Finds the I2C connections to the source of every element in i_connections and fills
 * in o_connI2Cs appropriately
 */
void findI2CPaths(vector<PowerConnection*> & i_connections, vector<connI2C_t> & o_connI2Cs)
{
    XMLTree tree;
    XMLElement root;
    vector<XMLElement> devices;
    vector<XMLElement>::iterator d;
    vector<XMLElement> paths;
    vector<XMLElement>::iterator p;
    vector<PowerConnection*>::iterator c;
    vector<connI2C_t>::iterator n;
    connI2C_t conn;
    unsigned int found = 0;
    string i2cPath;


    for (c=i_connections.begin();c!=i_connections.end();++c)
    {
        //Fill in the PowerConnection, leave the I2 info empty
        connI2C_t i;
        i.connection = *c;
        o_connI2Cs.push_back(i);
    }


    //load the *-i2c-busses.xml file
    if (tree.load(g_i2cFile))
    {
        mrwLogger l;
        l() << "Failed loading  I2C file " << g_i2cFile;
        l.error(true);
        return;
    }


    tree.getRoot(root);
    root.getChildren(devices);

    //look through all the I2C devices for the specific VRDs we need
    for (d=devices.begin();d!=devices.end();++d)
    {
        for (n=o_connI2Cs.begin();n!=o_connI2Cs.end();++n)
        {
            if (n->paths.empty())
            {
                //if the source (VRD) has the same instance path as the end device, found the match
                if (n->connection->source()->getChipPath() == d->getChildValue("instance-path"))
                {
                    found++;

                    n->address = d->getChildValue("address");

                    //get all the possible device paths
                    paths.clear();
                    d->findPath("system-paths").getChildren(paths);

                    //add each device path
                    for (p=paths.begin();p!=paths.end();++p)
                    {
                        i2cPath = p->getChildValue("fsp-device-path");
                        n->paths.push_back(i2cPath);

                        if (mrwLogger::getDebugMode())
                        {
                            string m = "Found device path " + i2cPath + " " + n->address +
                                            " for VRD " + n->connection->source()->getChipPath();
                            mrwLogger::debug(m);
                        }
                    }

                }

            }

        }

        if (found == o_connI2Cs.size())
            break;
    }

}



/**
 * Prints a VRD connection entry with the I2C info
 */
void printVrdConnection(connI2C_t & i_connection)
{
    PowerSink* centaur = i_connection.connection->sink();
    PowerSource* vrd = i_connection.connection->source();
    ecmdTarget_t* target = &i_connection.target;
    string path, address;

    fprintf(g_fp, "%s<centaur-vrd-connection>\n", mrwIndent(1));

    fprintf(g_fp, "%s<centaur>\n", mrwIndent(2));
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), centaur->getChipPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), centaur->plug()->location().c_str());
    fprintf(g_fp, "%s<target><node>%d</node><position>%d</position></target>\n",
            mrwIndent(3), target->node, target->position);

    fprintf(g_fp, "%s</centaur>\n", mrwIndent(2));

    //make sure address starts with a 0x
    if (i_connection.address.empty())
    {
        mrwLogger l;
        l() << "Missing I2C connection address for " << vrd->getChipPath() << "\n";
        if ((centaur->source().unit() == "VMEM") || multinode)
        {
#ifndef MRW_OPENPOWER            
	        l.error();
	        exit(1);
#else
            l.info();
#endif            
		}
		else
		{
			l.info();
		}
	}
    if (!i_connection.address.empty() && (i_connection.address.substr(0, 2) != "0x"))
        i_connection.address = "0x" + i_connection.address;

    fprintf(g_fp, "%s<vrd>\n", mrwIndent(2));
	string prt_vrd_type(centaur->source().unit());
    if (prt_vrd_type == "VCACHE")
    	prt_vrd_type = "VCS";
    else if (prt_vrd_type == "VCORE")
    	prt_vrd_type = "VDD";
    fprintf(g_fp, "%s<type>%s</type>\n", mrwIndent(3), prt_vrd_type.c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), vrd->plug()->location().c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), vrd->getChipPath().c_str());
    fprintf(g_fp, "%s<i2c-address>%s</i2c-address>\n", mrwIndent(3), i_connection.address.c_str());

    //print all the unique I2C paths
    vector<string> added_i2c_dev_paths;
    vector<string>::iterator p;
    for (unsigned int i=0;i<i_connection.paths.size();i++)
    {
    	for (p=added_i2c_dev_paths.begin(); p != added_i2c_dev_paths.end(); ++p)
    	{
    		if (*p == i_connection.paths[i])
    			break; // already added this one
    	}
    	if (p == added_i2c_dev_paths.end()) // not yet added?
    	{
	        fprintf(g_fp, "%s<i2c-dev-path>%s</i2c-dev-path>\n", mrwIndent(3), i_connection.paths[i].c_str());
			added_i2c_dev_paths.push_back(i_connection.paths[i]);
		}
    }

    fprintf(g_fp, "%s</vrd>\n", mrwIndent(2));

    fprintf(g_fp, "%s</centaur-vrd-connection>\n", mrwIndent(1));
}


/**
 * Load the connI2C_t.target element from i_targets.
 */
void loadTarget(connI2C_t & i_connection, const vector<ecmdTarget_t> & i_targets)
{
    for (vector<ecmdTarget_t>::const_iterator t=i_targets.begin();
         t!=i_targets.end();++t)
    {
        //looking for the centaur (sink) target...
        if (i_connection.connection->sink()->getChipPath() == t->instancePath)
        {
            i_connection.target = *t;
            break;
        }
    }

}


bool sortCentConnection(PowerConnection* i_left, PowerConnection* i_right)
{
    //sort based on the Centaur target
    ecmdTarget_t t1 = mrwGetTarget(i_left->sink()->getChipPath(), g_targetFile);
    ecmdTarget_t t2 = mrwGetTarget(i_right->sink()->getChipPath(), g_targetFile);

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

    }

    return false;
}


void logMissingVRD(ecmdTarget_t& cent, vector<connI2C_t>& vrdI2Cs, const string& vrd_type)
{
    vector<connI2C_t>::iterator v;
	string chk_vrd_type(vrd_type);
    if (chk_vrd_type == "VCS")
    	chk_vrd_type = "VCACHE";
    else if (chk_vrd_type == "VDD")
    	chk_vrd_type = "VCORE";

	// Check if this centaur has a VRD target that points to it
    for (v=vrdI2Cs.begin();v!=vrdI2Cs.end();++v)
    {
        if ((v->connection->sink()->source().unit() == chk_vrd_type) &&
    		(v->target.instancePath == cent.instancePath))
    		break;
    }

	// If not, log it as an error
	if (v == vrdI2Cs.end())
	{
        mrwLogger l;
        l() << cent.instancePath;
        l() << " is missing " << vrd_type << " connection";
        if (multinode)
#ifndef MRW_OPENPOWER            
	        l.error();
#else
            l.info();
#endif            
		else
			l.info();
	}
}

void printVrdConnections(vector<PowerConnection*> & i_connections)
{
    vector<PowerConnection*>::iterator c;
    vector<PowerConnection*> vrds;
    vector<connI2C_t> vrdI2Cs;
    vector<connI2C_t>::iterator v;
    vector<ecmdTarget_t> centTargets;


    //find the centaur VRD connections only
    for (c=i_connections.begin();c!=i_connections.end();++c)
    {
        if (isCentaurVrd((*c)->sink(), "VMEM"))
        {
            vrds.push_back(*c);
        }
        else if (isCentaurVrd((*c)->sink(), "VPP"))
        {
            vrds.push_back(*c);
        }
        else if (isCentaurVrd((*c)->sink(), "VCS"))
        {
            vrds.push_back(*c);
        }
        else if (isCentaurVrd((*c)->sink(), "VDD"))
        {
            vrds.push_back(*c);
        }
        else if (isCentaurVrd((*c)->sink(), "AVDD"))
        {
            vrds.push_back(*c);
        }
    }

	if (mrwLogger::getDebugMode())
	{
		int conn_count = 0;
		for(vector<PowerConnection*>::iterator p=vrds.begin(); p!=vrds.end(); ++p)
		{
			++conn_count;
			fflush(NULL);
		}
		fflush(NULL);
	}

    sort(vrds.begin(), vrds.end(), sortCentConnection);

    //find their I2C path info
    findI2CPaths(vrds, vrdI2Cs);

    //Get the memory buffer targets
    mrwGetTargets("memb", g_targetFile, centTargets);

    //Print just the VRD connections
    for (v=vrdI2Cs.begin();v!=vrdI2Cs.end();++v)
    {
        loadTarget(*v, centTargets);
        printVrdConnection(*v);
    }

    //Make sure we found the right amount
    unsigned int expected_vrd_count = centTargets.size() * 5;
    if (expected_vrd_count != vrdI2Cs.size())
    {
        mrwLogger l;
        l() << "Expected " << expected_vrd_count << " VRDs (from " << centTargets.size() << " centaur targets) but found "
	            << vrdI2Cs.size() << " centaur VRD connections\n";
		if (multinode)
#ifndef MRW_OPENPOWER            
	        l.error();
#else
            l.info();
#endif            
		else
			l.info();
    	// Try to figure out a detailed error message to pinpoint the issue...
    	if (expected_vrd_count < vrdI2Cs.size())
    	{
    		// Modeled too many VRDs.
    		// Not sure how THIS would happen, so issue "generic" message.
	        l() << "Centaurs with VRD connections:\n";
			if (multinode)
#ifndef MRW_OPENPOWER                
		        l.error();
#else
                l.info();
#endif                
			else
				l.info();
	
	        for (v=vrdI2Cs.begin();v!=vrdI2Cs.end();++v)
	        {
	            l() << v->target.instancePath;
				if (multinode)
#ifndef MRW_OPENPOWER                    
			        l.error();
#else
                    l.info();
#endif                    
				else
					l.info();
	        }
    	}
    	else
    	{
    		// Missing one or more VRDs.
    		// Try to identify exactly which ones are missing.
		    vector<ecmdTarget_t>::iterator p;
			for(p = centTargets.begin(); p != centTargets.end(); ++p)
			{
				logMissingVRD(*p, vrdI2Cs, "VMEM");
				logMissingVRD(*p, vrdI2Cs, "VPP");
				logMissingVRD(*p, vrdI2Cs, "VCS");
				logMissingVRD(*p, vrdI2Cs, "VDD");
				logMissingVRD(*p, vrdI2Cs, "AVDD");
			}
    	}

    }
}



int main(int argc, char* argv[])
{
    string inputFile, outputFile, vrdFile;

    if (parseArgs(argc, argv, inputFile, outputFile, g_targetFile, vrdFile, g_i2cFile))
    {
        usage();
        return 1;
    }

    if (mrwLoadGlobalTreeAndRoot(inputFile))
            return 1;

	// Find out if we're working on a single or multinode system
	if (SystemXML::root().getChildValue("enclosure-type") == "MULTIDRAWER")
		multinode = true;

    vector<PowerConnection*> connections;

    /////////////////////////////////////////////
    //Print the power summary

    mrwPowerMakeConnections(connections);

	if (mrwLogger::getDebugMode())
	{
		int conn_count = 0;
		for(vector<PowerConnection*>::iterator p=connections.begin(); p!=connections.end(); ++p)
		{
			++conn_count;
			fflush(NULL);
		}
		fflush(NULL);
	}

    g_fp = mrwPrintHeader(outputFile, "power-connections", "mrwpower");
    if (!g_fp) return 1;

    printPowerConnections(connections);
	if (mrwLogger::getDebugMode())
	{
		int conn_count = 0;
		for(vector<PowerConnection*>::iterator p=connections.begin(); p!=connections.end(); ++p)
		{
			++conn_count;
			fflush(NULL);
		}
		fflush(NULL);
	}

    mrwPrintFooter("power-connections", g_fp);


    /////////////////////////////////////////////
    //Print the centaur-vrd info

    g_fp = mrwPrintHeader(vrdFile, "centaur-vrd-connections", "mrwcvc");
    if (!g_fp) return 1;

    printVrdConnections(connections);

    mrwPrintFooter("centaur-vrd-connections", g_fp);

    return mrwGetReturnCode();
}



