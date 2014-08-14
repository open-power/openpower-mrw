/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwMruIdParser.C $                                            */
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
//******************************************************************************
// mrwMruIdParser
//
//  Created on: Feb 19, 2013
//      Author: hlava
//******************************************************************************
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <xmlutil.H>
#include <mrwParserCommon.H>

using namespace std;
extern vector<Plug*> g_plugs;

const char* USAGE = " \n\
Process MRW parts and cards to generate the MRU IDs XML file and a MRU IDs constants include (.H) file. \n\
This parser uses the mru-type-mapping.xml file to map part/card types to MRU types and as  \n\
input to generate the contants header file. \n\
\n\
Syntax: \n\
1) Generate the platform MRU IDs XML: \n\
   mrwMruIdParser [OPTIONS] --fullin xxx-full.xml --mapin yyy.xml --targets xxx-targets.xml --xmlout xxx-mru-ids.xml \n\
2) Generate the common C++ MRU type constants: \n\
   mrwMruIdParser [OPTIONS] --mapin yyy.xml --hout zzz.H \n\
\n\
Where : \n\
   --fullin xxx-full.xml : the merged (full) MRW XML input file \n\
   --targets xxx-targets.xml : the targets MRW XML file \n\
   --pcie xxx-pcie-busses.xml : the PCIE busses MRW XML file \n\
   --mapin yyy.xml  : the MRW MRU type mapping XML input file \n\
   --xmlout xxx-mru-ids.xml : the generated MRU IDs XML file name \n\
   --hout zzz.H     : the generated MRU IDs XML file name \n\
\n\
And OPTIONS are zero or more of : \n\
   --verbose : display additional diagnostics to stderr while running \n\
";

//******************************************************************************
// syntax
//******************************************************************************
void syntax()
{
	printf("%s\n", USAGE);
	exit(9);
}

//------------------------------------------------------------------------------
// Static Shared Variables
//------------------------------------------------------------------------------
static bool verbose = false;
static string fullinfile;
static string mapinfile;
static string targetsfile;
static string pciefile;
static string xmloutfile;
static string houtfile;
static FILE* xmloutf = NULL;
static FILE* houtf = NULL;

static XMLElement xml_root;
static XMLTree map_tree;
static XMLElement map_root;
static vector<XMLElement> map_elements;
static map<string,string> mru_type_cppname_mapping;
static map<string,string> mru_type_hex_mapping;
static map<string,int>    mru_type_inst_num_mapping;
static map<string,string> mru_part_type_name_mapping;
static map<string,string> mru_card_type_name_mapping;
static map<string,string> mru_unit_type_name_mapping;
static map<string,string> mru_chiplet_name_mapping;
static vector<XMLElement> mru_parts_used;
static vector<XMLElement> mru_cards_used;
static const uint32_t MAXLINESIZE = 4095;

static const char* XML_file_head =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n\
<mru-ids> \n\
\n";
static const char* XML_file_tail = " \n\
</mru-ids> \n\
\n";

static const char* H_file_head =
"#ifndef MRU_TYPE_MAPPING \n\
#define MRU_TYPE_MAPPING \n\
// GENERATED FILE: DO NOT EDIT DIRECTLY! \n\
// Generated: %s \n\
 \n\
enum mnfgMRUType_t { \n\
\n";
static const char* H_file_tail = " \n\
}; \n\
 \n\
#endif \n\
\n";

// Structures for storing MRU ID info
struct mru_id_entry_t
{
	string	hw_id;
	string	part_type;
	string	card_type;
	string	chiplet_type;
	string	unit_type;
	string	position;
	string  mru_type;
	string	mru_instance_num;
	string  mrid_hex;
	string  mrid_str;
	string  parent_mru;
	string	instance_path;
	string	loc_code;
	string	ecmd_target;
	string  plug_xpath;
	string  part_xpath;
	string  chiplet_xpath;
	string  unit_xpath;
};
static vector<mru_id_entry_t>  mru_ids;

//******************************************************************************
// debugPrint
//******************************************************************************
void debugPrint(const char* fmt, ...)
{
	static string vp_indent = "";

	if (verbose)
	{
		va_list arg_ptr;
		va_start(arg_ptr, fmt);
		if (fmt[0] == '<')
		{
			vp_indent = vp_indent.substr(3);
		}
		string l_fmt = vp_indent;
		l_fmt += fmt;
		vfprintf(stderr, l_fmt.c_str(), arg_ptr);
		if (fmt[0] == '>')
		{
			vp_indent += "   ";
		}
		fflush(NULL);
	}
}

typedef string::value_type char_t;
//******************************************************************************
// up_char
//******************************************************************************
char_t up_char( char_t ch )
{
    return (char_t)toupper(ch);
}

//******************************************************************************
// str2upper
//******************************************************************************
string str2upper( const string& src )
{
    string result;
    transform( src.begin(), src.end(), back_inserter( result ), up_char );
    return result;
}

//******************************************************************************
// str2cppname
//******************************************************************************
string str2cppname(const string& name)
{
	if (name.empty())
		return("");

	string upper_name = str2upper(name);
	string::iterator ch=upper_name.begin();
	++ch; // skip the first character since it might be a negative sign in a number
	for(; ch != upper_name.end(); ++ch)
	{
		if (*ch == '-')
			*ch = '_';
	}
	return(upper_name);
}

//******************************************************************************
// trimstr
//******************************************************************************
string trimstr(const string& str)
{
	string outstr(str);
	outstr.erase(outstr.find_last_not_of(" \n\r\t")+1); // remove trailing whitespace
	outstr.erase(0, outstr.find_first_not_of(" \n\r\t")); // remove leading whitespace
	return(outstr);
}

//******************************************************************************
// processMruMapping
//******************************************************************************
void processMruMapping(XMLElement& map_elmt)
{
	debugPrint(">processMruMapping()\n");
	string name;
	string cpp_name;
	XMLElement from;
	vector<XMLElement>::iterator p;
	vector<XMLElement>::iterator q;
	vector<XMLElement> from_types;

	// Process all the mru type mapping elements
	map_elmt.getChildren(map_elements);
	for(p = map_elements.begin(); p != map_elements.end(); ++p)
	{
		// Generate the .H file output
		name = p->getChildValue("mru-type-name");
		debugPrint("Processing %s\n", name.c_str());
		cpp_name = str2cppname(name);
		debugPrint("Adding mru_type_cppname_mapping[%s] -> %s\n", name.c_str(), cpp_name.c_str());
		mru_type_cppname_mapping[name] = cpp_name;
		debugPrint("Adding mru_type_hex_mapping[%s] -> %s\n", name.c_str(), p->getChildValue("mru-type-value").substr(2).c_str());
		mru_type_hex_mapping[name] = p->getChildValue("mru-type-value").substr(2);

		if (name == "unknown")
		{
			mru_part_type_name_mapping["*"] = name;
			if (houtf)
			{
				fprintf(houtf, "\n\tMNFG_NULL_MRU\t = %s,\n", p->getChildValue("mru-type-value").c_str());
			}
			continue;
		}

		if (houtf)
		{
			fprintf(houtf, "\n\tMNFG_%s_MRU\t = %s,\n",
				cpp_name.c_str(),
				p->getChildValue("mru-type-value").c_str());
		}
		
		// Store the internal mapping info
		from = p->getChild("from-part-types");
		if (!from.empty())
		{
			from.getChildren(from_types);
			for(q = from_types.begin(); q != from_types.end(); ++q)
			{
				debugPrint("   Adding part-type map: %s -> %s\n",
					q->getValue().c_str(), name.c_str());
				mru_part_type_name_mapping[q->getValue()] = name;
			}			
		}
		from = p->getChild("from-card-types");
		if (!from.empty())
		{
			from.getChildren(from_types);
			for(q = from_types.begin(); q != from_types.end(); ++q)
			{
				debugPrint("   Adding card-type map: %s -> %s\n",
					q->getValue().c_str(), name.c_str());
				mru_card_type_name_mapping[q->getValue()] = name;
			}			
		}
		from = p->getChild("from-unit-types");
		if (!from.empty())
		{
			from.getChildren(from_types);
			for(q = from_types.begin(); q != from_types.end(); ++q)
			{
				debugPrint("   Adding unit-type map: %s -> %s\n",
					q->getValue().c_str(), name.c_str());
				mru_unit_type_name_mapping[q->getValue()] = name;
			}			
		}
		from = p->getChild("from-chiplet-target-names");
		if (!from.empty())
		{
			from.getChildren(from_types);
			for(q = from_types.begin(); q != from_types.end(); ++q)
			{
				debugPrint("   Adding chiplet target name map: %s -> %s\n",
					q->getValue().c_str(), name.c_str());
				mru_chiplet_name_mapping[q->getValue()] = name;
			}			
		}
	}

	debugPrint("<processMruMapping()\n");
}

//******************************************************************************
// mergeMruPartsUsed
//******************************************************************************
void mergeMruPartsUsed(const string& card_id, vector<XMLElement>& from_parts, vector<XMLElement>& to_parts)
{
	debugPrint(">mergeMruPartsUsed(%s)\n", card_id.c_str());
	string part_id;
	string ru;
	vector<XMLElement>::iterator p;
	vector<XMLElement>::iterator q;

	for(p = from_parts.begin(); p != from_parts.end(); ++p)
	{
		part_id = p->getChildValue("part-id");		
		debugPrint("Checking %s\n", part_id.c_str());
		// Skip non-MRU parts
		ru = str2upper(p->getChildValue("ru"));
		if (!ru.empty()) { debugPrint("Found %s with ru set to %s\n", part_id.c_str(), ru.c_str()); }
		if (ru != "MRU") { continue; }

		// See if we've already added it
		debugPrint("Found MRU part %s.  Now checking if already added\n", part_id.c_str());
		for(q = to_parts.begin(); q != to_parts.end(); ++q)
		{
			if (part_id == q->getChildValue("part-id"))
			{
				break;
			}
		}
		if (q == to_parts.end())
		{
			debugPrint("Adding MRU part used: %s\n", part_id.c_str());
			to_parts.push_back(*p);
		}
	}
	debugPrint("<mergeMruPartsUsed(%s)\n", card_id.c_str());
}

//******************************************************************************
// isCardMru
//******************************************************************************
bool isCardMru(const string i_card_id)
{
	bool rc = false;
	vector<XMLElement>::iterator p;

	for(p = mru_cards_used.begin(); p != mru_cards_used.end(); ++p)
	{
		if (i_card_id == p->getChildValue("card-id"))	
		{
			rc = true;
			break;
		}
	}

	return(rc);
}

//******************************************************************************
// isPartMru
//******************************************************************************
bool isPartMru(const string i_part_id)
{
	bool rc = false;
	vector<XMLElement>::iterator p;

	for(p = mru_parts_used.begin(); p != mru_parts_used.end(); ++p)
	{
		if (i_part_id == p->getChildValue("part-id"))	
		{
			rc = true;
			break;
		}
	}

	return(rc);
}

//******************************************************************************
// getInstNum
//******************************************************************************
uint32_t getInstNum(const string& mru_type,
					Plug* i_plug,
					const string& parent_instance_path,
					string& o_inst_num_str)
{
	char buf[100];
	uint32_t inst_num = 0;

	// If there is an explicitly set instance number, use that
	if (i_plug && (i_plug->element().getChildValue("mru-instance-number") != ""))
	{
		sscanf(i_plug->element().getChildValue("mru-instance-number").c_str(), "%x", &inst_num);
	}
	else
	{
		// The instance number key is used to select when to start a new sequence with 0,
		// and when to increment the last instance number used by that mru-type within this
		// FRU.
		string inst_num_key(mru_type);
		inst_num_key += "-";
		inst_num_key += parent_instance_path;
	
		if (mru_type_inst_num_mapping.find(inst_num_key) == mru_type_inst_num_mapping.end())
		{
			mru_type_inst_num_mapping[inst_num_key] = 0;
			debugPrint("Initialized inst num for %s to 0\n", inst_num_key.c_str());
		}
		else
		{
			mru_type_inst_num_mapping[inst_num_key] += 1;
			debugPrint("Incremented the inst num for %s to %d\n", inst_num_key.c_str(), mru_type_inst_num_mapping[inst_num_key]);
		}
	
		inst_num = mru_type_inst_num_mapping[inst_num_key];
	}

	sprintf(buf, "%d", inst_num);
	o_inst_num_str = buf;

	return(inst_num);
}

//******************************************************************************
// processMruIdPart
//******************************************************************************
void processMruIdPart(Plug* i_plug, const string& i_parent_mrid, XMLElement& i_part_instance)
{
	string part_id = i_part_instance.getChildValue("part-id");
	debugPrint(">processMruIdPart(%s,..,%s)\n", i_plug->id().c_str(), part_id.c_str());
    char buf[2048];
	uint32_t inst_num;
	mru_id_entry_t mru_id;
	XMLElement elmt;

	elmt = xml_root.findPath("layout/parts").find("part", "id", part_id);
	mru_id.hw_id = part_id;
	mru_id.part_type = elmt.getChildValue("part-type");
	mru_id.position = i_part_instance.getChildValue("position");
	mru_id.mru_type = elmt.getChildValue("mru-type");
	if (mru_id.mru_type.empty())
	{
		if (mru_part_type_name_mapping.find(mru_id.part_type) != mru_part_type_name_mapping.end())
        {
			mru_id.mru_type = mru_part_type_name_mapping[mru_id.part_type];
        }
        else
        {
			mru_id.mru_type = mru_part_type_name_mapping["*"];
        }
	}
	inst_num = getInstNum(mru_id.mru_type, i_plug, i_plug->path(), mru_id.mru_instance_num);
	sprintf(buf, "%04X", inst_num);
	mru_id.mrid_hex = "0x";
	mru_id.mrid_hex += mru_type_hex_mapping[mru_id.mru_type];
	mru_id.mrid_hex += buf;
	mru_id.mrid_str = mru_type_cppname_mapping[mru_id.mru_type];
	mru_id.mrid_str += "-";
	mru_id.mrid_str += buf;
	mru_id.parent_mru = i_parent_mrid;
	mru_id.instance_path = i_plug->path(); // Start with parent card's instance path
	mru_id.instance_path += "/";
	mru_id.instance_path += part_id;
	mru_id.instance_path += "-";
	mru_id.instance_path += i_part_instance.getChildValue("position");
	mru_id.loc_code = i_plug->location(); // Use parent card's location code

	debugPrint("calling: mrwGetTarget(%s,%s)\n", i_plug->path().c_str(), targetsfile.c_str());
	ecmdTarget_t et = mrwGetTarget(i_plug->path(), targetsfile);
	mrwGetTargetString(et, mru_id.ecmd_target);

    mru_id.plug_xpath = mrwMakePlugXPath(i_plug);
    mru_id.part_xpath = mrwMakePartXPath(i_plug, i_part_instance);

	mru_ids.push_back(mru_id);

	debugPrint("<processMruIdPart(%s,..,%s)\n", i_plug->id().c_str(), part_id.c_str());
}

//******************************************************************************
// processChiplet
//******************************************************************************
void processChiplet(Plug* i_plug, const string& i_parent_mrid, XMLElement& i_part_instance, XMLElement& i_chiplet, const string& i_ipath)
{
	string id = i_chiplet.getChildValue("id");
	string part_id = i_part_instance.getChildValue("part-id");
	debugPrint(">processChiplet(...,%s)\n", id.c_str());
	map<string,string>::iterator p_map;
	vector<XMLElement>::iterator p;
	vector<XMLElement> child_chiplets;
	char buf[100];
	uint32_t inst_num;
	uint32_t mru_inst_num;
	string chip_num;
	string chiplet_mrid(i_parent_mrid);
	string current_ipath(i_ipath);

	p_map = mru_chiplet_name_mapping.find( i_chiplet.getChildValue("target-name") );
	if (p_map != mru_chiplet_name_mapping.end())
	{
		// Process this chiplet
		debugPrint("Found chiplet MRU with target-name %s\n", i_chiplet.getChildValue("target-name").c_str());
		mru_id_entry_t mru_id;
		mru_id.hw_id = id;
		mru_id.chiplet_type = i_chiplet.getChildValue("target-name");
		mru_id.position = i_chiplet.getChildValue("position");
		mru_id.mru_type = p_map->second;
		// MRU ID for chiplet: 0xMMMMdpcc
        //     MMMM = MRU type value
        //     d = module number on this FRU, currently zero for all known systems
		//     p = processor chip number within the module
		//     cc = core/ex number
		chip_num = i_part_instance.getChildValue("position");
		inst_num = atoi(mru_id.position.c_str());
		sprintf(buf, "0%s%02X", chip_num.c_str(), inst_num);
		mru_inst_num = strtol(buf, NULL, 16);
		mru_id.mrid_hex = "0x";
		mru_id.mrid_hex += mru_type_hex_mapping[mru_id.mru_type];
		mru_id.mrid_hex += buf;
		mru_id.mrid_str = mru_type_cppname_mapping[mru_id.mru_type];
		mru_id.mrid_str += "-";
		mru_id.mrid_str += buf;
		//chiplet_mrid = mru_id.mrid_str;  <--- Leave it as PARENT_FRU per request from FWSM
		mru_id.parent_mru = i_parent_mrid;
		mru_id.instance_path = i_ipath;
		mru_id.instance_path += "/";
		mru_id.instance_path += id;
		current_ipath = mru_id.instance_path;
		mru_id.loc_code = i_plug->location(); // Use parent card's location code

		sprintf(buf, "%d", mru_inst_num);
		mru_id.mru_instance_num = buf;

	    mru_id.plug_xpath = mrwMakePlugXPath(i_plug);
	    mru_id.part_xpath = mrwMakePartXPath(i_plug, i_part_instance);
	    mru_id.chiplet_xpath = mrwMakeChipletXPath(part_id, id);

		mru_ids.push_back(mru_id);
	}

	// Process all nested (child) chiplets
	i_chiplet.getChildren(child_chiplets, "chiplet");
	for(p = child_chiplets.begin(); p != child_chiplets.end(); ++p)
	{
		processChiplet(i_plug, chiplet_mrid, i_part_instance, *p, current_ipath);
	}

	debugPrint("<processChiplet(...,%s)\n", id.c_str());
}

//******************************************************************************
// processPartUnitsChiplets
//******************************************************************************
void processPartUnitsChiplets(Plug* i_plug, const string& i_parent_mrid, XMLElement& i_part_instance)
{
	string part_id = i_part_instance.getChildValue("part-id");
	debugPrint(">processPartUnitsChiplets(%s,..,%s)\n", i_plug->id().c_str(), part_id.c_str());
	XMLElement part_elmt = xml_root.findPath("layout/parts").find("part", "id", part_id);

	size_t pos;
	vector<XMLElement>::iterator p;
	vector<XMLElement>::iterator p_unit;
	map<string,string>::iterator p_map;
	vector<XMLElement> chiplets;
	vector<XMLElement> unit_types;
	vector<XMLElement> units;
	string chk_unit_type;
	string chk_unit_type_group;
	string chk_unit_subtype;
	char buf[100];
	uint32_t inst_num;
	string part_ipath;

	// Build instance path to containing part
	part_ipath = i_plug->path(); // Start with parent card's instance path
	part_ipath += "/";
	part_ipath += part_id;
	part_ipath += "-";
	part_ipath += i_part_instance.getChildValue("position");

	//--------------------------------------------------------------------------
	// Process all the chiplets
	//--------------------------------------------------------------------------
	// Get all the chiplets under chiplets element
	part_elmt.getChild("chiplets").getChildren(chiplets);
	for (p = chiplets.begin(); p != chiplets.end(); ++p)
	{
		processChiplet(i_plug, i_parent_mrid, i_part_instance, *p, part_ipath);
	}

	//--------------------------------------------------------------------------
	// Process all the units
	//--------------------------------------------------------------------------
	// Get all the unit types under the part
	part_elmt.getChild("units").getChildren(unit_types);
	for (p = unit_types.begin(); p != unit_types.end(); ++p)
	{
		// Loop through the units mapping to see if this is a possible MRU
		for (p_map = mru_unit_type_name_mapping.begin();
			 p_map != mru_unit_type_name_mapping.end();
			 ++p_map)
		{
			// Parse the value into type & subtype
			chk_unit_type = p_map->first;
			pos = chk_unit_type.find(",");
			if (pos == string::npos)
			{
				fprintf(stderr, "WARNING: bad MRU unit-type value found: \"%s\".  Skipping it.\n",
						chk_unit_type.c_str());
				continue;
			}
			chk_unit_type = chk_unit_type.substr(0, pos);

			// Check if type matches
			chk_unit_type_group = chk_unit_type;	
			chk_unit_type_group += "s";
			if (chk_unit_type_group == p->getName())
			{
				break;
			}
		}

		// Did we find a possible unit type group?
		if (p_map != mru_unit_type_name_mapping.end())
		{
			debugPrint("Searching %s for MRUs...\n", chk_unit_type_group.c_str());
			// Yes, so loop through the subelements and check each one...
			p->getChildren(units);
			for(p_unit = units.begin(); p_unit != units.end(); ++p_unit)
			{
				debugPrint("   checking %s with type=%s\n", p_unit->getChildValue("id").c_str(),
							p_unit->getChildValue("type").c_str());
				// Check for a type/subtype match
				for (p_map = mru_unit_type_name_mapping.begin();
					 p_map != mru_unit_type_name_mapping.end();
					 ++p_map)
				{
					// Parse the value into type & subtype
					chk_unit_type = p_map->first;
					pos = chk_unit_type.find(",");
					if (pos == string::npos)
					{
						fprintf(stderr, "WARNING: bad MRU unit-type value found: \"%s\".  Skipping it.\n",
								chk_unit_type.c_str());
						continue;
					}
					chk_unit_subtype = chk_unit_type.substr(pos+1);
					chk_unit_type = chk_unit_type.substr(0, pos);
					debugPrint("      chk_unit_type=%s,  chk_unit_subtype=%s\n",
						chk_unit_type.c_str(), chk_unit_subtype.c_str());

					// Check if type and subtype  matches
					if ( (chk_unit_type == p_unit->getName()) &&
					     ((chk_unit_subtype == "*") || (chk_unit_subtype == p_unit->getChildValue("type")))
					   )
					{
						debugPrint("      It matches %s,%s!\n", chk_unit_type.c_str(), chk_unit_subtype.c_str());
						break;
					}
				}
				if (p_map !=  mru_unit_type_name_mapping.end())
				{
					debugPrint("Found %s unit MRU with subtype %s\n",
							p_unit->getName().c_str(),
							p_unit->getChildValue("type").c_str());
					mru_id_entry_t mru_id;
					mru_id.hw_id = p_unit->getChildValue("id");
					mru_id.unit_type = p_unit->getName();
					mru_id.mru_type = p_map->second;
					inst_num = getInstNum(mru_id.mru_type, NULL, i_plug->path(), mru_id.mru_instance_num);
					sprintf(buf, "%04X", inst_num);
					mru_id.mrid_hex = "0x";
					mru_id.mrid_hex += mru_type_hex_mapping[mru_id.mru_type];
					mru_id.mrid_hex += buf;
					mru_id.mrid_str = mru_type_cppname_mapping[mru_id.mru_type];
					mru_id.mrid_str += "-";
					mru_id.mrid_str += buf;
					mru_id.parent_mru = i_parent_mrid;
					mru_id.instance_path = i_plug->path(); // Start with parent card's instance path
					mru_id.instance_path += "/";
					mru_id.instance_path += part_id;
					mru_id.instance_path += "-";
					mru_id.instance_path += i_part_instance.getChildValue("position");
					mru_id.instance_path += "/";
					mru_id.instance_path += p_unit->getChildValue("id");
					sprintf(buf, "%d", inst_num);
					mru_id.position = buf;
					mru_id.loc_code = i_plug->location(); // Use parent card's location code

				    mru_id.plug_xpath = mrwMakePlugXPath(i_plug);
				    mru_id.part_xpath = mrwMakePartXPath(i_plug, i_part_instance);
				    mru_id.unit_xpath = mrwMakeUnitXPath(part_id, p_unit->getName(), p_unit->getChildValue("id"));

					mru_ids.push_back(mru_id);
				}
			}
		}
	}

	debugPrint("<processPartUnitsChiplets(%s,..,%s)\n", i_plug->id().c_str(), part_id.c_str());
}

//******************************************************************************
// processMruIdsPlug
//******************************************************************************
void processMruIdsPlug(Plug* i_plug, const string& i_parent_mrid)
{
	debugPrint(">processMruIdsPlug(%s)\n", i_plug->id().c_str());
	string my_mrid("PARENT_FRU");
	string parent_inst_path;
	size_t pos;
	XMLElement elmt;
	vector<XMLElement> part_instances;
	
	// Process this plug
	if (isCardMru(i_plug->id()) || isPartMru(i_plug->id()))
	{
		debugPrint("Plug is a MRU, so get info and add it...\n");
		char buf[100];
		uint32_t inst_num;
		mru_id_entry_t mru_id;

		mru_id.hw_id = i_plug->id();
		mru_id.position = i_plug->position();
		if (i_plug->type() == Plug::CARD)
		{
			elmt = xml_root.findPath("layout/cards").find("card", "id", mru_id.hw_id);
			mru_id.card_type = elmt.getChildValue("card-type");
			mru_id.mru_type = elmt.getChildValue("mru-type");
			if (mru_id.mru_type == "")
			{
				if (mru_card_type_name_mapping.find(mru_id.card_type) !=
                    mru_card_type_name_mapping.end())
                {
					mru_id.mru_type = mru_card_type_name_mapping[mru_id.card_type];
                }
                else
                {
					mru_id.mru_type = mru_card_type_name_mapping["*"];
                }
			}
		}
		else
		{
			elmt = xml_root.findPath("layout/parts").find("part", "id", mru_id.hw_id);
			mru_id.part_type = elmt.getChildValue("part-type");
			mru_id.mru_type = elmt.getChildValue("mru-type");
			if (mru_id.mru_type == "")
			{
				if (mru_part_type_name_mapping.find(mru_id.part_type) !=
                    mru_part_type_name_mapping.end())
                {
					mru_id.mru_type = mru_part_type_name_mapping[mru_id.part_type];
                }
                else
                {
					mru_id.mru_type = mru_part_type_name_mapping["*"];
                }
			}
		}
		parent_inst_path = i_plug->path();
		pos = parent_inst_path.rfind("/");
		if (pos != string::npos)
		{
			parent_inst_path = parent_inst_path.substr(0, pos);
		}
		inst_num = getInstNum(mru_id.mru_type, i_plug, parent_inst_path, mru_id.mru_instance_num);
		sprintf(buf, "%04X", inst_num);
		mru_id.mrid_hex = "0x";
		mru_id.mrid_hex += mru_type_hex_mapping[mru_id.mru_type];
		mru_id.mrid_hex += buf;
		mru_id.mrid_str = mru_type_cppname_mapping[mru_id.mru_type];
		mru_id.mrid_str += "-";
		mru_id.mrid_str += buf;
		//my_mrid = mru_id.mrid_str;   <--- Leave it as PARENT_FRU per request from FWSM
		mru_id.parent_mru = i_parent_mrid;
		mru_id.instance_path = i_plug->path();
		mru_id.loc_code = i_plug->location();

		debugPrint("calling: mrwGetTarget(%s,%s)\n", i_plug->path().c_str(), targetsfile.c_str());
		ecmdTarget_t et = mrwGetTarget(i_plug->path(), targetsfile);
		mrwGetTargetString(et, mru_id.ecmd_target);

	    mru_id.plug_xpath = mrwMakePlugXPath(i_plug);

		mru_ids.push_back(mru_id);
	}

	// Process any MRU parts on this card
	elmt = xml_root.findPath("layout/cards").find("card", "id", i_plug->id()).getChild("part-instances");
	elmt.getChildren(part_instances);
    vector<XMLElement>::iterator p;
    for (p = part_instances.begin(); p != part_instances.end(); ++p)
    {
    	if ( isPartMru(p->getChildValue("part-id")) )
    	{
    		processMruIdPart(i_plug, my_mrid, *p);
    	}
    	// Process MRU units/chiplets on this part (if any)
		processPartUnitsChiplets(i_plug, my_mrid, *p);    	
    }

	// Process all child plugs
    vector<Plug*>::iterator p_plug;
    for (p_plug=i_plug->children().begin(); p_plug!=i_plug->children().end(); ++p_plug)
    {
        processMruIdsPlug(*p_plug, my_mrid);
    }

	debugPrint("<processMruIdsPlug(%s)\n", i_plug->id().c_str());
}

//******************************************************************************
// processMruIds
//******************************************************************************
void processMruIds(XMLElement& map_elmt)
{
	debugPrint(">processMruIds()\n");
	string card_id;
	string ru;
	XMLElement elmt;
	XMLElement pu_elmt;
	vector<XMLElement>::iterator p;
	vector<XMLElement> cards_used;
	vector<XMLElement> tmp_parts_used;
	vector<Plug*>::iterator p_plugs;

	// Get a list of all cards used
	elmt = map_elmt.getChild("layout").getChild("cards-used");	
	elmt.getChildren(cards_used);

	// Get a list of all MRU cards and parts used
	for(p = cards_used.begin(); p != cards_used.end(); ++p)
	{
		card_id = p->getChildValue("card-id");
		ru = str2upper(p->getChildValue("ru"));
		if (ru == "MRU")
		{
			debugPrint("Adding MRU cards-used %s\n", card_id.c_str());
			mru_cards_used.push_back(*p);
		}
		elmt = xml_root.findPath("layout/cards").find("card", "id", card_id);
		pu_elmt = elmt.getChild("parts-used");				
		pu_elmt.getChildren(tmp_parts_used);
		mergeMruPartsUsed(card_id, tmp_parts_used, mru_parts_used);
	}

	// Walk the plugs to process the MRUs
	for(p_plugs = g_plugs.begin(); p_plugs != g_plugs.end(); ++p_plugs)
	{
		// Clear the MRU instance num mapping to start instance numbers over
		// on new plug
		mru_type_inst_num_mapping.clear();

		// Process the plug and all its children
		processMruIdsPlug(*p_plugs, "PARENT_FRU");
	}	

	// Output the XML for the MRU info that we've accumulated
	vector<mru_id_entry_t>::iterator p_mrid;	
	for(p_mrid = mru_ids.begin(); p_mrid != mru_ids.end(); ++p_mrid)
	{
		fprintf(xmloutf, "<mru-id>\n");
		fprintf(xmloutf, "   <mrid>%s</mrid>\n", p_mrid->mrid_str.c_str());
		fprintf(xmloutf, "   <mrid-value>%s</mrid-value>\n", p_mrid->mrid_hex.c_str());
		fprintf(xmloutf, "   <instance-path>%s</instance-path>\n", p_mrid->instance_path.c_str());
		fprintf(xmloutf, "   <location-code>%s</location-code>\n", p_mrid->loc_code.c_str());
		fprintf(xmloutf, "   <hardware-id>%s</hardware-id>\n", p_mrid->hw_id.c_str());
		fprintf(xmloutf, "   <part-type>%s</part-type>\n", p_mrid->part_type.c_str());
		fprintf(xmloutf, "   <card-type>%s</card-type>\n", p_mrid->card_type.c_str());
		fprintf(xmloutf, "   <chiplet-type>%s</chiplet-type>\n", p_mrid->chiplet_type.c_str());
		fprintf(xmloutf, "   <unit-type>%s</unit-type>\n", p_mrid->unit_type.c_str());
		fprintf(xmloutf, "   <position>%s</position>\n", p_mrid->position.c_str());
		fprintf(xmloutf, "   <mru-type>%s</mru-type>\n", p_mrid->mru_type.c_str());
		fprintf(xmloutf, "   <mru-instance-num>%s</mru-instance-num>\n", p_mrid->mru_instance_num.c_str());
		fprintf(xmloutf, "   <parent-mru>%s</parent-mru>\n", p_mrid->parent_mru.c_str());
		fprintf(xmloutf, "   <ecmd-target>%s</ecmd-target>\n", p_mrid->ecmd_target.c_str());
		fprintf(xmloutf, "   <plug-xpath>%s</plug-xpath>\n", p_mrid->plug_xpath.c_str());
		fprintf(xmloutf, "   <part-xpath>%s</part-xpath>\n", p_mrid->part_xpath.c_str());
		fprintf(xmloutf, "   <chiplet-xpath>%s</chiplet-xpath>\n", p_mrid->chiplet_xpath.c_str());
		fprintf(xmloutf, "   <unit-xpath>%s</unit-xpath>\n", p_mrid->unit_xpath.c_str());
		fprintf(xmloutf, "</mru-id>\n");
	}

	debugPrint("<processMruIds()\n");
}

//******************************************************************************
// fixupMruUnitInstancePaths
//******************************************************************************
void fixupMruUnitInstancePaths()
{
	debugPrint(">fixupMruUnitInstancePaths()\n");
//  Leaving this function here but no-op'd in case we find a need for adding fixup
// back in at a later date (this will be a placeholder/template for that).
	debugPrint("no-op. returning\n");
#if 0
	XMLElement mru_xml_root;
	XMLTree mru_tree;
	string mru_unit_path;
	string target_path;
	string file_to_fix;
	vector<XMLElement> mru_ids;
	vector<XMLElement>::iterator p;
	string cmd;
	size_t pos;
	char tmpname[50];
	sprintf(tmpname, "/tmp/mrwMruIdParser-%d.tmp", getpid());

	// Load the generated MRU IDs XML
	if (mru_tree.load(xmloutfile))
	{
		fprintf(stderr, "ERROR: unable to load %s for targets fixup\n", xmloutfile.c_str());
		exit(9);
	}
	mru_tree.getRoot(mru_xml_root);
	mru_xml_root.getChildren(mru_ids, "mru-id");

	// Process all the unit MRUs requiring instance path fixup in targets
	for(p = mru_ids.begin(); p != mru_ids.end(); ++p)
	{
		if ( !(p->getChildValue("unit-type").empty()) )
		{
			mru_unit_path = p->getChildValue("instance-path");
			debugPrint("Doing target path fixup for %s with path %s\n",
					p->getChildValue("mrid").c_str(),
					mru_unit_path.c_str());
			target_path = mru_unit_path;
			pos = target_path.rfind("/");
			target_path = target_path.substr(0, pos+1);
			target_path += p->getChildValue("hardware-id");
		
			// Issue the commands to update the instance path
			if (p->getChildValue("unit-type") == "pcie-root-unit")
				file_to_fix = pciefile;
			else
				file_to_fix = targetsfile;
			cmd = "cat ";
			cmd += file_to_fix;
			cmd += " | sed 's%";
			cmd += target_path;
			cmd += "%";
			cmd += mru_unit_path;
			cmd += "%' >";
			cmd += tmpname;
			debugPrint("cmd: %s\n", cmd.c_str());
			if (system(cmd.c_str()))
			{
				fprintf(stderr, "ERROR: cmd to fixup targets instance path failed: %s\n", cmd.c_str());
				exit(9);
			}

			// Replace the updated file
			cmd = "cp ";
			cmd += tmpname;
			cmd += " ";
			cmd += file_to_fix;
			debugPrint("cmd: %s\n", cmd.c_str());
			if (system(cmd.c_str()))
			{
				fprintf(stderr, "ERROR: cmd to replace targets file after instance path fixup failed: %s\n", cmd.c_str());
				exit(9);
			}
		}	
	}

	unlink(tmpname);
#endif
	debugPrint("<fixupMruUnitInstancePaths()\n");
}

//******************************************************************************
// validateXPaths
//******************************************************************************
void validateXPaths()
{
	debugPrint(">validateXPaths()\n");
	XMLTree mruid_tree;
	XMLElement mruid_root;
	vector<XMLElement> mruids;
	vector<XMLElement>::iterator p;
	string xpath;
	unsigned int num_errs = 0;

	if (mruid_tree.load(xmloutfile))
	{
		fprintf(stderr, "ERROR: unable to load %s for XPath validation\n", xmloutfile.c_str());
		exit(9);
	}
	mruid_tree.getRoot(mruid_root);

	// Loop through each mru-id element and validate any set XPath element
	mruid_root.getChildren(mruids);	
	debugPrint("%d MRU ID elements found.  validating...\n", mruids.size());
	for(p = mruids.begin(); p != mruids.end(); ++p)
	{
		xpath = p->getChildValue("plug-xpath");		
		num_errs += mrwValidateXPath(xpath);
		xpath = p->getChildValue("part-xpath");		
		num_errs += mrwValidateXPath(xpath);
		xpath = p->getChildValue("chiplet-xpath");		
		num_errs += mrwValidateXPath(xpath);
		xpath = p->getChildValue("unit-xpath");		
		num_errs += mrwValidateXPath(xpath);
	}

	if (num_errs)
	{
		fprintf(stderr, "ERROR: one or more XPath validation errors\n");
		exit(9);
	}

	debugPrint("<validateXPaths()\n");
}

//******************************************************************************
//                          main
//******************************************************************************
int main(int argc, char **argv)
{
	string arg;
	int i;
	vector<XMLElement>::iterator p;
	XMLElement elmt;
	string elmt_name;

	//--------------------------------------------------------------------------
	// Parse the arguments
	//--------------------------------------------------------------------------
	if (argc < 2) { syntax(); }
	for(i=1; i < argc; ++i)
	{
		arg = argv[i];

		if ((arg == "-?")||(arg == "-h")||(arg == "--help")) { syntax(); }
		if (arg == "--verbose") { verbose = true; continue; }

		if (arg == "--fullin") { fullinfile = argv[++i]; continue; }
		if (arg == "--mapin") { mapinfile = argv[++i]; continue; }
		if (arg == "--targets") { targetsfile = argv[++i]; continue; }
		if (arg == "--pcie") { pciefile = argv[++i]; continue; }
		if (arg == "--xmlout") { xmloutfile = argv[++i]; continue; }
		if (arg == "--hout") { houtfile = argv[++i]; continue; }

		fprintf(stderr, "ERROR: unrecognized option: %s\n", argv[i]);
		exit(9);
	}

	if (mapinfile == "")
	{
		fprintf(stderr, "ERROR: you must specify a --mapin argument\n");
		exit(9);
	}
	if (xmloutfile.empty() && houtfile.empty())
	{
		fprintf(stderr, "ERROR: you must specify either a --xmlout or -hout argument\n");
		exit(9);
	}
	if (!xmloutfile.empty() && !houtfile.empty())
	{
		fprintf(stderr, "ERROR: you must specify only one of --xmlout and --hout\n");
		exit(9);
	}
	if (!xmloutfile.empty() && (fullinfile == ""))
	{
		fprintf(stderr, "ERROR: you must specify a --fullin argument\n");
		exit(9);
	}
	if (!xmloutfile.empty() && (targetsfile == ""))
	{
		fprintf(stderr, "ERROR: you must specify a --targets argument\n");
		exit(9);
	}
	if (!xmloutfile.empty() && (pciefile == ""))
	{
		fprintf(stderr, "ERROR: you must specify a --pcie argument\n");
		exit(9);
	}

	//--------------------------------------------------------------------------
	// Open the input and output files and load the XML tree
	//--------------------------------------------------------------------------
	// Uncomment for REALLY verbose output...
	//g_xmlutil_debug = verbose;

	// Open and init the output file
	if (!xmloutfile.empty())
	{
		xmloutf = fopen(xmloutfile.c_str(), "w");
		if (xmloutf == NULL)
		{
			fprintf(stderr, "ERROR: unable to open output file %s\n", xmloutfile.c_str());
			exit(9);
		}

		// Write the boiler-plate beginning stuff to the output file
		fprintf(xmloutf, "%s", XML_file_head);

		// print a timestamp in comments
		fprintf(xmloutf, "<!-- GENERATED FILE: DO NOT EDIT DIRECTLY! -->\n");
		mrwPrintTimeStampComment(xmloutf);

		// Load the input XML file
		if (SystemXML::load(fullinfile))
		{
			fprintf(stderr, "ERROR: Failed to read %s\n", fullinfile.c_str());
			exit(9);
		}
		xml_root = SystemXML::root();
		mrwMakePlugs();
	}
	else
	{
		houtf = fopen(houtfile.c_str(), "w");
		if (houtf == NULL)
		{
			fprintf(stderr, "ERROR: unable to open output file %s\n", houtfile.c_str());
			exit(9);
		}

		// Write the boiler-plate beginning stuff to the output file
		string ts_str = mrwGetCurrentDateTime();
		fprintf(houtf, H_file_head, ts_str.c_str());
	}



	// Load the mru type mapping file
	if (map_tree.load(mapinfile))
	{
		fprintf(stderr, "ERROR: Failed to read %s\n", mapinfile.c_str());
		exit(9);
	}
	map_tree.getRoot(map_root);


	// Store mapping in internal data used during remaining processing
	// and optionally generate the MRU type mapping header file
	processMruMapping(map_root);

	// Generate the MRU IDs XML file (if requested)
	if (xmloutf)
	{
		processMruIds(xml_root);
	}

	// Write the boiler-plate ending stuff to the output file
	if (xmloutf)
	{
		fprintf(xmloutf, "%s", XML_file_tail);
		fclose(xmloutf);

		// Fix up the MRU unit instance paths in the other XML files.
		// (I looked at implementing this some other way but I didn't see
		// a good alternative without moving a ton of MRU ID-specific code into
		// mrwParserCommon and then have mrwMruIdParser, mrwPCIEParser, and mrwTargetParser
		// repeat a lot of the same processing.  The implementation as a
		// post-processing fixup seemed like the simplest,
		// best-performing, and least risky alternative.)
		fixupMruUnitInstancePaths();

		// Do a final check that all the generated XPaths are at least valid
		// syntax and point to single XML elements.  This will prevent hard-to-find
		// bugs from slipping through to later in the build or to runtime.
		validateXPaths();
	}
	else
	{
		fprintf(houtf, "%s", H_file_tail);
		fclose(houtf);
	}

    return(0);
}

