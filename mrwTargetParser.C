/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwTargetParser.C $                                           */
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
#include <string>
#include <algorithm>
#include <xmlutil.H>
#include <mrwParserCommon.H>
#include <mrwMemCommon.H>

/*
 * This program generates the ECMD target XML from the system XML.
 */

using namespace std;


//TODO:  not supported yet:
//  selection-groups, and if multiple cards are just plugged in the same slot, like:
//  dimm1 <plug>  location C20  connector J0
//  dimm2 <plug>  location C20  connector J0

//#define MRWDEBUG

//Usage
const char* USAGE = " \n"
"Process Machine Readable Workbook to generate hardware target ID info, \n"
"such as the fields comprising ecmd targets \n"
"\n"
"Syntax: mrwTargetParser [OPTIONS] --in xxx.xml --out yyy.xml \n"
"\n"
"Where : \n"
"   --in xxx.xml : the MRW XML input file \n"
"   --out xxx : the generated file name \n"
"\n"
"And OPTIONS are zero or more of : \n"
"   --verbose : display additional diagnostics to stderr while running \n"
"\n";


//XML header and footer
static const char* XML_file_head =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
"<targets> \n"
"\n";
static const char* XML_file_tail = " \n"
"</targets> \n"
"\n";



static bool   g_verbose = false;
static string g_infile;
static string g_outfile;
static FILE*  g_outf = NULL;
int           g_node = 0;
int           g_procChipPosition = 0;
int           g_pnorChipPosition = 0;
int           g_dpssChipPosition = 0;
int           g_fspChipPosition  = 0;
int           g_cfamSChipPosition = 0;
int           g_clockChipPosition = 0;
extern vector<Plug*> g_plugs; //defined in mrwParserCommon



void resetChipPositions()
{
    g_procChipPosition = 0;
    g_pnorChipPosition = 0;
    g_dpssChipPosition = 0;
    g_fspChipPosition = 0;
    g_cfamSChipPosition = 0;
    g_clockChipPosition = 0;
}


/**
 * debugPrint
 */
void debugPrint(const char* fmt, ...)
{
    if (g_verbose)
    {
        va_list arg_ptr;
        va_start(arg_ptr, fmt);
        vfprintf(stderr, fmt, arg_ptr);
        fflush(NULL);
    }
}

/**
 * Used to hold the information that belongs to a target.
 */
class Target
{
public:

   string name() const { return iv_name; }
   int node() const { return iv_node; }
   int position() const { return iv_position; }
#ifndef MRW_OPENPOWER
   string location() const { return iv_location; }
#else
   string location() const { return string(); }
#endif
   string path() const { return iv_path; }
   int chipUnit() const { return iv_chipUnit; }
   string description() const { return iv_description; }
   string assembly() const { return iv_assembly; }
   string partId() const { return iv_partId; }

   void setNode(int i_node) { iv_node = i_node; }

   void plugXPath(string & i_xpath) { iv_plugXPath = i_xpath; }
   string plugXPath() const { return iv_plugXPath; }
   void partXPath(string & i_xpath) { iv_partXPath = i_xpath; }
   string partXPath() const { return iv_partXPath; }
   void chipletXPath(string & i_xpath) { iv_chipletXPath = i_xpath; }
   string chipletXPath() const { return iv_chipletXPath; }

   void setHtml(bool h) { iv_html = h; }
   bool getHtml() { return iv_html; }

protected:

    Target() {};

protected:

    string iv_name;
    int iv_node;
    int iv_position;
    string iv_location;
    string iv_path;
    int iv_chipUnit;
    string iv_description;
    string iv_plugXPath;
    string iv_partXPath;
    string iv_chipletXPath;
    bool iv_html;
    string iv_assembly;
    string iv_partId;

};


/**
 * A DIMM target
 */
class DimmTarget : public Target
{
public:

    DimmTarget(const string & i_path, const string & i_cardName,
               const string & i_loc, int i_node, int i_pos)
    {
        iv_name = "dimm";
        iv_node = i_node;
        iv_position = i_pos;
        iv_location = i_loc;
        iv_path = i_path;
        iv_chipUnit = -1;
        iv_html = true;
        iv_description = "Instance of " + i_cardName;
        iv_assembly = i_path.substr(0, i_path.find_first_of("/")); //get assembly
        iv_assembly = iv_assembly.substr(0, iv_assembly.find_last_of("-")); //strip off pos

#ifdef MRWDEBUG
        debugPrint("Creating %s target n%dp%d %s\n", iv_name.c_str(),
                   iv_node, iv_position, iv_path.c_str());
#endif
    }

};


/**
 * A Chip target
 */
class ChipTarget : public Target
{
public:
    ChipTarget(Plug* i_plug, XMLElement & i_partInstance,
               const string & i_name, int i_node, int i_pos,
               bool i_inHtml)
    {
        iv_name = (i_name == "cpu") ? "pu" : i_name;
        iv_node = i_node;
        iv_position = i_pos;
        iv_location = i_plug->location();
        iv_chipUnit = -1;
        iv_html = i_inHtml;
        iv_assembly = i_plug->assembly().id;
        iv_partId = i_partInstance.getChildValue("part-id");

        iv_path  = i_plug->path() + "/";
        iv_path += iv_partId + "-";
        iv_path += i_partInstance.getChildValue("position");

        iv_plugXPath = mrwMakePlugXPath(i_plug);
        iv_partXPath = mrwMakePartXPath(i_plug, i_partInstance);

        iv_description = "Instance of " + i_partInstance.getChildValue("part-id") + " " + i_name;

#ifdef MRWDEBUG
        debugPrint("Creating %s target n%dp%d %s\n", iv_name.c_str(),
                   iv_node, iv_position, iv_path.c_str());
#endif
    }

};


/**
 * A chiplet target
 */
class ChipletTarget : public Target
{
public:
    ChipletTarget(const Target* i_chipTarget, const ChipletTarget* i_parentChiplet, XMLElement & i_chiplet)
    {
        iv_name = i_chiplet.getChildValue("target-name");
        iv_node = i_chipTarget->node();
        iv_position = i_chipTarget->position();
        iv_location = i_chipTarget->location();
        iv_chipUnit = atoi(i_chiplet.getChildValue("position").c_str());
        iv_plugXPath = i_chipTarget->plugXPath();
        iv_partXPath = i_chipTarget->partXPath();
        iv_html = true;
        if (i_parentChiplet)
        	iv_path = i_parentChiplet->path() + "/" + i_chiplet.getChildValue("id");
        else
        	iv_path = i_chipTarget->path() + "/" + i_chiplet.getChildValue("id");
        iv_assembly = i_chipTarget->assembly();
        iv_partId = i_chipTarget->partId();

        iv_description = "Instance of chiplet " + i_chiplet.getChildValue("id");

#ifdef MRWDEBUG
        debugPrint("Creating %s target n%dp%d unit%d %s\n", iv_name.c_str(),
                   iv_node, iv_position, iv_chipUnit, iv_path.c_str());
#endif
    }


};


/**
 * A unit target
 */
class UnitTarget : public Target
{
public:
    UnitTarget(const Target* i_chipTarget, const string & i_name,
               const XMLElement & i_unit, int i_unitPos)
    {
        iv_name = i_name;
        iv_node = i_chipTarget->node();
        iv_position = i_chipTarget->position();
        iv_chipUnit = i_unitPos;
        iv_location = i_chipTarget->location();
        iv_plugXPath = i_chipTarget->plugXPath();
        iv_partXPath = i_chipTarget->partXPath();
        iv_path = i_chipTarget->path() + "/" + i_unit.getChildValue("id");
        iv_assembly = i_chipTarget->assembly();
        iv_partId = i_chipTarget->partId();

        iv_html = false;
        iv_description = "Instance of unit " + iv_name;

#ifdef MRWDEBUG
        debugPrint("Creating %s target n%dp%d unit%d %s\n", iv_name.c_str(),
                   iv_node, iv_position, iv_chipUnit, iv_path.c_str());
#endif
    };

};




/**
 * syntax
 */
void syntax()
{
    printf("%s\n", USAGE);
    exit(9);
}



/**
 * Not supporting selection groups yet because we don't have any.
 * When we do, have to make sure the node/positions are the same
 * for the cards/chips in the same selection group.
 */
void selectionGroupCheck(Plug* i_plug)
{
    if (i_plug->inSelectionGroup())
    {
        mrwLogger logger;
        logger() << "selection-groups not supported yet when creating targets!";
        logger.error(true);
    }
}




/**
 * Makes the processor chiplet targets
 */
void makeChipletTargets(const Target* i_procTarget,
						const ChipletTarget* i_parentChiplet,
                            XMLElement & i_chiplet,
                            const string & i_partId,
                            vector<Target*> & o_targets)
{
	Target* target = NULL;
	ChipletTarget* current_chiplet = NULL;
    if (!i_chiplet.findPath("target-name").empty())
    {
        current_chiplet = new ChipletTarget(i_procTarget, i_parentChiplet, i_chiplet);
        target = current_chiplet;
        o_targets.push_back(target);

        string chipletXPath = mrwMakeChipletXPath(i_partId, i_chiplet.getChildValue("id"));
        target->chipletXPath(chipletXPath);
    }

    vector<XMLElement> chiplets;
    vector<XMLElement>::iterator c;

    i_chiplet.getChildren(chiplets, "chiplet");

    //recursively drill down into any children chiplets (i.e. the mcs's in the mc)
    for (c=chiplets.begin();c!=chiplets.end();++c)
    {
        makeChipletTargets(i_procTarget, current_chiplet, *c, i_partId, o_targets);
    }

}


/**
 * Dives down into the processor chiplets to make the targets
 */
void traverseChiplets(const Target* i_procTarget,
                        XMLElement & i_part,
                        vector<Target*> & o_targets)
{
    vector<XMLElement> chiplets;
    vector<XMLElement>::iterator chiplet;

    i_part.findPath("chiplets").getChildren(chiplets, "chiplet");

    for (chiplet=chiplets.begin();chiplet!=chiplets.end();++chiplet)
    {
        makeChipletTargets(i_procTarget, NULL, *chiplet, i_part.getChildValue("id"), o_targets);
    }
}


/**
 * Makes PSI unit targets
 */
void makePSIUnitTargets(Plug* i_plug, Target* i_chipTarget, XMLElement & i_part, vector<Target*> & o_targets)
{
    string partType = i_part.getChildValue("part-type");

    //masters
    if ((partType == "fsp") || (partType == "cpu"))
    {
        vector<XMLElement> units;
        vector<XMLElement>::iterator u;
        int unitPos = 0;

        i_part.findPath("units/psi-units").getChildren(units);
        for (u=units.begin();u!=units.end();++u)
        {
            o_targets.push_back(new UnitTarget(i_chipTarget,
                                               (partType == "fsp") ? "psi-master" : "psi-slave",
                                               *u, unitPos));
            unitPos++;
        }
    }

}


/**
 * Makes powerbus unit targets
 */
void makePowerBusUnitTargets(Plug* i_plug, Target* i_chipTarget, XMLElement & i_part, vector<Target*> & o_targets)
{
    vector<XMLElement> units;
    vector<XMLElement>::iterator u;
    int unitPos = 0;

    i_part.findPath("units/powerbus-units").getChildren(units);

    //First to X, then A
    for (u=units.begin();u!=units.end();++u)
    {
        if (u->getChildValue("type") == "X")
        {
            o_targets.push_back(new UnitTarget(i_chipTarget, "xbus", *u, unitPos));
            unitPos++;
        }
    }

    unitPos = 0;
    for (u=units.begin();u!=units.end();++u)
    {
        if (u->getChildValue("type") == "A")
        {
            o_targets.push_back(new UnitTarget(i_chipTarget, "abus", *u, unitPos));
            unitPos++;
        }
    }

}


/**
 * Makes PCIE PHB unit targets
 */
void makePHBUnitTargets(Plug* i_plug, Target* i_chipTarget, XMLElement & i_part, vector<Target*> & o_targets)
{
    vector<XMLElement> units;
    vector<XMLElement>::iterator u;
    int unitPos = 0;

    i_part.findPath("internal-units/pcie-phb-units").getChildren(units);

    //First to X, then A
    for (u=units.begin();u!=units.end();++u)
    {
        o_targets.push_back(new UnitTarget(i_chipTarget, "phb", *u, unitPos));
        unitPos++;
    }
}



/**
 * Makes a processor target if appropriate
 */
bool makeProcTarget(Plug* i_plug, XMLElement & i_instance, XMLElement & i_part, vector<Target*> & o_targets)
{
    bool found = false;

    if (i_part.getChildValue("part-type") == "cpu")
    {
        found = true;
        selectionGroupCheck(i_plug);

        //Make the target for the processor itself
        Target* target = new ChipTarget(i_plug, i_instance, i_part.getChildValue("part-type"),
                                        g_node, g_procChipPosition, true);
        o_targets.push_back(target);

        //Now do the chiplets
        traverseChiplets(target, i_part, o_targets);

        //Now do the PSI slaves
        makePSIUnitTargets(i_plug, target, i_part, o_targets);

        //Now do A & X bus
        makePowerBusUnitTargets(i_plug, target, i_part, o_targets);

        //PHB targets
        makePHBUnitTargets(i_plug, target, i_part, o_targets);

        g_procChipPosition++;
    }

    return found;
}


/**
 * Makes a PNOR target if appropriate
 */
bool makePNORTarget(Plug* i_plug, XMLElement & i_instance, XMLElement & i_part, vector<Target*> & o_targets)
{
    bool found = false;

    //Looking for a NOR_FLASH with host boot content (not FSP content)
    if ((i_instance.getChildValue("part-id") == "NOR_FLASH") &&
        (i_instance.getChildValue("content-type") == "HOST_BOOT"))
    {
        found = true;
        selectionGroupCheck(i_plug);

        o_targets.push_back(new ChipTarget(i_plug, i_instance, "pnor",
                                           g_node, g_pnorChipPosition, false));

        g_pnorChipPosition++;
    }

    return found;
}

/**
 * Makes a DPSS target if appropriate
 */
bool makeDPSSTarget(Plug* i_plug, XMLElement & i_instance, XMLElement & i_part, vector<Target*> & o_targets)
{
    bool found = false;

    if (i_part.getChildValue("part-type") == "dpss")
    {
        found = true;
        selectionGroupCheck(i_plug);

        o_targets.push_back(new ChipTarget(i_plug, i_instance, i_part.getChildValue("part-type"),
                                           g_node, g_dpssChipPosition, false));

        g_dpssChipPosition++;
    }

    return found;
}


/**
 * Makes a clock target if appropriate
 */
bool makeClockTarget(Plug* i_plug, XMLElement & i_instance, XMLElement & i_part, vector<Target*> & o_targets)
{
    bool found = false;

    if (i_part.getChildValue("part-type") == "clock-gen")
    {
        found = true;
        selectionGroupCheck(i_plug);

        o_targets.push_back(new ChipTarget(i_plug, i_instance, i_part.getChildValue("part-type"),
                                           g_node, g_clockChipPosition, false));

        g_clockChipPosition++;
    }

    return found;
}



/**
 * Makes an FSP target if appropriate
 */
bool makeFSPTarget(Plug* i_plug, XMLElement & i_instance, XMLElement & i_part, vector<Target*> & o_targets)
{
    bool found = false;

    if (i_part.getChildValue("part-type") == "fsp")
    {
        found = true;
        selectionGroupCheck(i_plug);

        o_targets.push_back(new ChipTarget(i_plug, i_instance, i_part.getChildValue("part-type"),
                                           g_node, g_fspChipPosition, false));

        g_fspChipPosition++;

        //make the psi-master targets
        makePSIUnitTargets(i_plug, o_targets.back(), i_part, o_targets);
    }

    return found;
}

/**
 * Makes a CFAM-S target if appropriate
 */
bool makeCFAMSTarget(Plug* i_plug, XMLElement & i_instance, XMLElement & i_part, vector<Target*> & o_targets)
{
    bool found = false;

    if (i_part.getChildValue("part-type") == "cfam-s")
    {
        found = true;
        selectionGroupCheck(i_plug);

        o_targets.push_back(new ChipTarget(i_plug, i_instance, i_part.getChildValue("part-type"),
                                           g_node, g_cfamSChipPosition, false));

        g_cfamSChipPosition++;

    }

    return found;
}



/**
 * Traverses each instance on the plug and calls the available target functions on each
 */
void makeTargets(Plug* i_plug, vector<Target*> & o_targets)
{
    vector<XMLElement> instances;
    vector<XMLElement>::iterator instance;
    XMLElement part;

    i_plug->card().findPath("part-instances").getChildren(instances);

    //See if we crossed a node boundary.  If we did, reset the positions
    //A node boundary is a card that plugs at the top level
    if (i_plug->parent() == NULL)
    {
        int node = atoi(i_plug->assembly().position.c_str());

        //For brazos that has a separate node for the FSPs & clocks, we're going
        //to call that node 4 even though it shows up as node 0 in the XML.
        //It makes the algorithm sooooo much easier to just hardcode the name here
        //instead of trying to infer later which targets were on a non-proc node and
        //adjusting everthing that I'm going to do it even though it's obviously not
        //a good practice.
        if (i_plug->assembly().id.find("laramie") != string::npos)
            node = 4;

        if (node != g_node)
        {
            g_node = node;
            resetChipPositions();
        }

    }

    //Loop through each instance, seeing if it's one of the targets we want to create
    for (instance=instances.begin();instance!=instances.end();++instance)
    {
        part = mrwGetPart(instance->getChildValue("part-id"));

        if (!makeProcTarget(i_plug, *instance, part, o_targets))
            if (!makePNORTarget(i_plug, *instance, part, o_targets))
                if (!makeDPSSTarget(i_plug, *instance, part, o_targets))
                    if (!makeFSPTarget(i_plug, *instance, part, o_targets))
                        if (!makeCFAMSTarget(i_plug, *instance, part, o_targets))
                            makeClockTarget(i_plug, *instance, part, o_targets);
    }


    //recursively drill down the children
    vector<Plug*>::iterator plug;
    for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
    {
        makeTargets(*plug, o_targets);
    }

}




/**
 * Makes the centaur, MBA, and DIMM targets that are connected to the MCS target passed in
 */
void makeMemTargets(const Target* i_target,
                   const vector<MemSystemBus*> & i_busses,
                   vector<Target*> & o_targets)
{
    vector<MemSystemBus*>::const_iterator bus;
    XMLElement instance, chiplet, part, unit;
    Target* target = NULL;
    Target* centTarget = NULL;
    string path, id, plugXPath, partXPath;
    MemMcs* mcs;
    MemMba* mba;
    MemDram* dram;
    vector<string> centaurs, mbas, dimms;
    int position;



    //Find the memory busses that start at the same MCS target as i_target

    for (bus=i_busses.begin();bus!=i_busses.end();++bus)
    {
        mcs  = (*bus)->mcs();
        mba  = (*bus)->mba();
        dram = (*bus)->dram();

        ///////////////////////////////////////////////////
        //Not supporting selection groups yet because we don't have any.
        //When we do, have to make sure the node/positions are the same
        //for the cards/chips in the same selection group.
        if (mcs->plug()->inSelectionGroup() || mba->plug()->inSelectionGroup() || dram->plug()->inSelectionGroup())
        {
            mrwLogger logger;
            logger() << "memory selection-groups not supported yet!";
            logger.error(true);
        }

        path = mcs->path();

        if (i_target->path() == path)
        {

            /////////////////////////////////////////////
            /////////////////////////////////////////////
            //Centaur target

            instance = mba->plug()->card().getChild("part-instances").find("part-instance", "id", mba->mbaEndpoint().id());

            //Position is based on the MCS position, where the centaurs connected to a processor
            //need to start on a multiple of 8 since there's 8 MCSs per processor, and then then
            //offset past that is the MCS chipunit position.
            position = (i_target->position() * 8) + i_target->chipUnit();

            target = new ChipTarget(mba->plug(), instance, "memb", i_target->node(), position, true);

            //Centaur DIMMS will have multiple busses for their mcs->mba->dram  paths,
            //and since the MBAs are all on the same centaur, there will be multiple busses that
            //have that centaur, and we just need one, so skip if we already have it
            if (find(centaurs.begin(), centaurs.end(), target->path()) == centaurs.end())
            {
                centTarget = target;
                centaurs.push_back(target->path());
                o_targets.push_back(target);

                //Now do the chiplets
                part = mrwGetPart(target->partId());
                traverseChiplets(target, part, o_targets);
            }
            else
            {
                delete target;
            }



            ///////////////////////////////////////////
            ///////////////////////////////////////////
            //DIMM Target

            path = dram->dimmPath();

            //Need the MBA position from the mba chiplet from the centaur, from the ddr-master-unit
            part = mrwGetPart(mrwGetPartId(mba->plug()->card(), mba->mbaEndpoint().id()));
            unit = part.findPath("units/ddr-master-units").find("ddr-master-unit", "id", mba->mbaEndpoint().unit());
            chiplet = part.findPath("chiplets").find("chiplet", "id", unit.getChildValue("chiplet-id"));

            //On Centaur DIMMs, the position matches the centaur's position,
            if (mba->plug() == dram->plug())
            {
                position = centTarget->position();
            }
            else
            {
                //With JEDEC DIMMs, there can be:
                //8 DIMMS per Centaur
                //4 DIMMs per MBA on the centaur
                //2 DIMMs per port on the MBA
                //1 DIMMS per slot on the port


                int mbaPos = atoi(chiplet.getChildValue("position").c_str());
                int port   = atoi(mba->port().c_str());
                int slot   = atoi(mba->slot().c_str());

                position = (centTarget->position() * 8) + (mbaPos * 4) + (port * 2) + slot;
            }


            target = new DimmTarget(path, dram->plug()->id(), dram->plug()->location(),
                                    i_target->node(), position);

            //Like the above targets, the same DIMM can show up in multiple busses with centaur DIMMS,
            //so just keep the first one.
            if (find(dimms.begin(), dimms.end(), target->path()) == dimms.end())
            {
                plugXPath = mrwMakePlugXPath(dram->plug());
                target->plugXPath(plugXPath);

                dimms.push_back(target->path());
                o_targets.push_back(target);
            }
            else
            {
                delete target;
            }



            /////////////////////////////////////////////
            /////////////////////////////////////////////
            //MBA target

#if 0 //doing in traverse chiplets now

            //To build the target, need the <chiplet> element for the MBA, and the centaur target

            target = new ChipletTarget(centTarget, chiplet);

            //Like the centaurs, the same MBA can show up in multiple busses with centaur DIMMs,
            //so just keep the first one.
            if (find(mbas.begin(), mbas.end(), target->path()) == mbas.end())
            {
                string chipletXPath = mrwMakeChipletXPath(part.getChildValue("id"), chiplet.getChildValue("id"));
                target->chipletXPath(chipletXPath);

                mbas.push_back(target->path());
                o_targets.push_back(target);
            }
            else
            {
                delete target;
            }
#endif
        }

    }

}


/**
 * top level call to make the centaur, MBA, and DIMM memory targets
 */
void makeMemTargets(vector<Target*> & io_targets)
{
    vector<Target*>::iterator t;
    vector<Target*> targets;
    vector<MemSystemBus*> memBusses;

    mrwMemMakeBusses(memBusses);

    for (t=io_targets.begin();t!=io_targets.end();++t)
    {
        if ((*t)->name() == "mcs")
        {
            //Make the memory targets for everything attached to this MCS
            makeMemTargets(*t, memBusses, targets);
        }
    }

    io_targets.insert(io_targets.end(), targets.begin(), targets.end());

}



/**
 * top level call to make targets
 */
void makeTargets(vector<Target*> & o_targets)
{
    vector<Plug*>::iterator plug;

    //Do the processor targets first, and then the memory targets, because
    //the memory target positions are all relative to connected processor position

    for (plug=g_plugs.begin();plug!=g_plugs.end();++plug)
    {
        makeTargets(*plug, o_targets);
    }

    makeMemTargets(o_targets);

}


/**
 * prints a target in XML
 */
void printTarget(Target* i_target)
{
    string indent1(4, ' ');
    string indent2(8, ' ');

    fprintf(g_outf, "%s<target", indent1.c_str());

    //Some targets shouldn't be in the HTMl
    if (i_target->getHtml())
        fprintf(g_outf, ">\n");
    else
        fprintf(g_outf, " skiphtml=\"true\">\n");

    fprintf(g_outf, "%s<ecmd-common-name>%s</ecmd-common-name>\n", indent2.c_str(), i_target->name().c_str());
    fprintf(g_outf, "%s<description>%s</description>\n", indent2.c_str(), i_target->description().c_str());
    fprintf(g_outf, "%s<node>%d</node>\n", indent2.c_str(), i_target->node());
    fprintf(g_outf, "%s<position>%d</position>\n", indent2.c_str(), i_target->position());

    if (i_target->chipUnit() != -1)
        fprintf(g_outf, "%s<chip-unit>%d</chip-unit>\n", indent2.c_str(), i_target->chipUnit());

    fprintf(g_outf, "%s<instance-path>%s</instance-path>\n", indent2.c_str(), i_target->path().c_str());
    fprintf(g_outf, "%s<plug-xpath>%s</plug-xpath>\n", indent2.c_str(), i_target->plugXPath().c_str());

    if (!i_target->partXPath().empty())
        fprintf(g_outf, "%s<part-xpath>%s</part-xpath>\n", indent2.c_str(), i_target->partXPath().c_str());

    if (!i_target->chipletXPath().empty())
        fprintf(g_outf, "%s<chiplet-xpath>%s</chiplet-xpath>\n", indent2.c_str(), i_target->chipletXPath().c_str());


    fprintf(g_outf, "%s<location>%s</location>\n", indent2.c_str(), i_target->location().c_str());

    fprintf(g_outf, "%s</target>\n", indent1.c_str());
}


/**
 * top level call to print the targets to xml
 */
void printTargets(vector<Target*> & i_targets)
{
    g_outf = fopen(g_outfile.c_str(), "w");
    if (!g_outf)
    {
        fprintf(stderr, "ERROR: unable to open output file %s\n", g_outfile.c_str());
        exit(9);
    }


    fprintf(g_outf, "%s", XML_file_head);

    for_each(i_targets.begin(), i_targets.end(), printTarget);

    fprintf(g_outf, "%s", XML_file_tail);

    fclose(g_outf);
}


/**
 * Adjust the node numbers of the non-proc nodes to be unique, and be the
 * first available node after all the proc nodes.
 *
 * So, for example, on a brazos system with a FSP drawer and 4 processor drawers:
 *  before:
 *      maxdale:       node 0
 *      proc-drawers:  nodes 0-3
 *
 *  after:
 *      maxdale:       node 4
 *      proc-drawers:  nodes 0-3
 *
 */
#if 0
//Don't need this since I'm harcoding the laramie assembly check above.
//if I ever have to put this in, it still doesn't work quite right for
//the times when a target exists on both proc & non proc because I still
//need to adjust the proc node positions down to remove the holes.
void adjustNodes(vector<Target*> & i_targets)
{
    vector<string> procAssemblies;
    vector<Target*>::iterator t;
    int highestProcNode = 0;

    //save the proc assemblies and the highest node #
    for (t=i_targets.begin();t!=i_targets.end();++t)
    {
        if ((*t)->name() == "pu")
        {
            if (find(procAssemblies.begin(), procAssemblies.end(),
                     (*t)->assembly()) == procAssemblies.end())
            {
                procAssemblies.push_back((*t)->assembly());
            }
        }

        if (highestProcNode < (*t)->node())
            highestProcNode = (*t)->node();

    }

    //reassign the non-proc nodes to the next available #
    for (t=i_targets.begin();t!=i_targets.end();++t)
    {
        if (find(procAssemblies.begin(), procAssemblies.end(),
                 (*t)->assembly()) == procAssemblies.end())
        {

#ifdef MRWDEBUG
            debugPrint("Changing node of %s from %d to %d\n", (*t)->name().c_str(),
                       (*t)->node(), highestProcNode + 1);
#endif

            (*t)->setNode(highestProcNode + 1);
        }
    }

}
#endif


int main(int argc, char **argv)
{
    string arg;
    int i;

    //--------------------------------------------------------------------------
    // Parse the arguments
    //--------------------------------------------------------------------------
    if (argc < 2) { syntax(); }
    for(i=1; i < argc; ++i)
    {
        arg = argv[i];

        if ((arg == "-?")||(arg == "-h")||(arg == "--help")) { syntax(); }
        if (arg == "--verbose") { g_verbose = true;  continue; }
        if (arg == "--in") { g_infile = argv[++i]; continue; }
        if (arg == "--out") { g_outfile = argv[++i]; continue; }

        fprintf(stderr, "ERROR: unrecognized option: %s\n", argv[i]);
        exit(9);
    }

    if (g_infile == "")
    {
        fprintf(stderr, "ERROR: you must specify a --in argument\n");
        exit(9);
    }
    if (g_outfile == "")
    {
        fprintf(stderr, "ERROR: you must specify a --out argument\n");
        exit(9);
    }


    if (mrwLoadGlobalTreeAndRoot(g_infile))
        return 1;


    mrwMakePlugs();

    vector<Target*> targets;
    makeTargets(targets);

    printTargets(targets);


    return 0;
}


