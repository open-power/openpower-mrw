/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwMergeElements.C $                                          */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwMergeElements.C 1$

//******************************************************************************
// mrwMergeElements.C
//
//  Created on: Nov 3, 2011
//      Author: hlava
//******************************************************************************
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <xmlutil.H>
#include <mrwParserCommon.H>

using namespace std;

const char* USAGE = " \n\
Process Machine Readable Workbook to merge partial elements which may have come from, \n\
separate source files.\n\
\n\
Syntax: mrwMergeElements [OPTIONS] --in xxx.xml --out yyy.xml \n\
\n\
Where : \n\
   --in xxx.xml : the MRW XML input file \n\
   --out xxx : the generated file name \n\
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
static string op;
static string infile;
static string outfile;

static XMLTree xml_tree;
static XMLElement xml_root;


//******************************************************************************
// debugPrint
//******************************************************************************
void debugPrint(const char* fmt, ...)
{
	if (verbose)
	{
		va_list arg_ptr;
		va_start(arg_ptr, fmt);
		vfprintf(stderr, fmt, arg_ptr);
		fflush(NULL);
	}
}

//******************************************************************************
// mergeCardSubelement
//******************************************************************************
void mergeCardSubelement(XMLElement& from_card_subelmt,
		XMLElement& to_card,
		XMLElement& to_card_subelmt)
{
	if (from_card_subelmt.empty() || to_card.empty()) // Nothing to merge?
	{
		return;
	}
	if (to_card_subelmt.empty()) // No place to put it?
	{
		// Move the whole container over
		xml_tree.moveElement(from_card_subelmt, to_card, XMLTree::CHILD);
		return;
	}

	// If we reach here, we merge the from subelements into the to container...
	vector<XMLElement> from_elmts;
	from_card_subelmt.getChildren(from_elmts);
	for(vector<XMLElement>::iterator p = from_elmts.begin(); p != from_elmts.end(); ++p)
	{
		xml_tree.moveElement(*p, to_card_subelmt, XMLTree::CHILD);
	}
}

//******************************************************************************
// mergeCard
//******************************************************************************
void mergeCard(XMLElement& from_card, XMLElement& to_card)
{
	debugPrint("Merging two occurrences of card \"%s\"\n", from_card.getChild("id").getValue().c_str());
	XMLElement from_subelmt, to_subelmt;
	static const char* merge_subelements[] =
	{
		"parts-used",
		"connectors-used",
		"part-instances",
		"connector-instances",
		"busses",
		"power-rails",
		NULL
	};

	// Merge all the mergeable subelements
	for (int i = 0; merge_subelements[i] != NULL; ++i)
	{
		from_subelmt = from_card.getChild(merge_subelements[i]);
		to_subelmt = to_card.getChild(merge_subelements[i]);
		mergeCardSubelement(from_subelmt, to_card, to_subelmt);
	}

	// Delete the from element
	xml_tree.removeElement(from_card);
}

//******************************************************************************
// mergeCards
//******************************************************************************
void mergeCards()
{
	debugPrint(">mergeCards()\n");
	vector<XMLElement> cards;
	vector<XMLElement>::iterator p1, p2;
	bool found_dup = false;
	string card_id;

	// Get the list of all card elements
	XMLElement cards_elmt = xml_root.findPath("layout/cards");
	cards_elmt.getChildren(cards, "card");

	// Find the first duplicates and merge them
	for(p1 = cards.begin(); (p1 != cards.end()) && !found_dup; ++p1)
	{
		card_id = p1->getChild("id").getValue();
		debugPrint("   checking \"%s\"\n", card_id.c_str());
		for(p2 = (p1 + 1); p2 != cards.end(); ++p2)
		{
			if (card_id == p2->getChild("id").getValue())
			{
				mergeCard(*p2, *p1);
				found_dup = true;
				break;
			}
		}
	}

	// If we found any duplicates, recursively run the merge
	// (this is done recursively because the list of card elements must
	// be rebuilt after each merge)
	if (found_dup)
	{
		mergeCards();
	}

	debugPrint("<mergeCards()\n");
}

//******************************************************************************
// mergePartSubelement
//******************************************************************************
void mergePartSubelement(XMLElement& from_part_subelmt,
		XMLElement& to_part,
		XMLElement& to_part_subelmt)
{
	if (from_part_subelmt.empty() || to_part.empty()) // Nothing to merge?
	{
		return;
	}
	if (to_part_subelmt.empty()) // No place to put it?
	{
		// Move the whole container over
		xml_tree.moveElement(from_part_subelmt, to_part, XMLTree::CHILD);
		return;
	}

	// If we reach here, we merge the from subelements into the to container...
	vector<XMLElement> from_elmts;
	from_part_subelmt.getChildren(from_elmts);
	for(vector<XMLElement>::iterator p = from_elmts.begin(); p != from_elmts.end(); ++p)
	{
		xml_tree.moveElement(*p, to_part_subelmt, XMLTree::CHILD);
	}
}

//******************************************************************************
// mergePart
//******************************************************************************
void mergePart(XMLElement& from_part, XMLElement& to_part)
{
	debugPrint("Merging two occurrences of part \"%s\"\n", from_part.getChild("id").getValue().c_str());
	XMLElement from_subelmt, to_subelmt;
	static const char* merge_subelements[] =
	{
		"units",
		NULL
	};

	// Merge all the mergeable subelements
	for (int i = 0; merge_subelements[i] != NULL; ++i)
	{
		from_subelmt = from_part.getChild(merge_subelements[i]);
		to_subelmt = to_part.getChild(merge_subelements[i]);
		mergePartSubelement(from_subelmt, to_part, to_subelmt);
	}

	// Delete the from element
	xml_tree.removeElement(from_part);
}

//******************************************************************************
// mergeParts
//******************************************************************************
void mergeParts()
{
	debugPrint(">mergeParts()\n");
	vector<XMLElement> parts;
	vector<XMLElement>::iterator p1, p2;
	bool found_dup = false;
	string part_id;

	// Get the list of all part elements
	XMLElement parts_elmt = xml_root.findPath("layout/parts");
	parts_elmt.getChildren(parts, "part");

	// Find the first duplicates and merge them
	for(p1 = parts.begin(); (p1 != parts.end()) && !found_dup; ++p1)
	{
		part_id = p1->getChild("id").getValue();
		debugPrint("   checking \"%s\"\n", part_id.c_str());
		for(p2 = (p1 + 1); p2 != parts.end(); ++p2)
		{
			if (part_id == p2->getChild("id").getValue())
			{
				mergePart(*p2, *p1);
				found_dup = true;
				break;
			}
		}
	}

	// If we found any duplicates, recursively run the merge
	// (this is done recursively because the list of part elements must
	// be rebuilt after each merge)
	if (found_dup)
	{
		mergeParts();
	}

	debugPrint("<mergeParts()\n");
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

	//--------------------------------------------------------------------------
	// Parse the arguments
	//--------------------------------------------------------------------------
	if (argc < 2) { syntax(); }
	for(i=1; i < argc; ++i)
	{
		arg = argv[i];

		if ((arg == "-?")||(arg == "-h")||(arg == "--help")) { syntax(); }
		if (arg == "--verbose") { verbose = true; continue; }
		if (arg == "--in") { infile = argv[++i]; continue; }
		if (arg == "--out") { outfile = argv[++i]; continue; }

		fprintf(stderr, "ERROR: unrecognized option: %s\n", argv[i]);
		exit(9);
	}

	if (infile == "")
	{
		fprintf(stderr, "ERROR: you must specify a --in argument\n");
		exit(9);
	}
	if (outfile == "")
	{
		fprintf(stderr, "ERROR: you must specify a --out argument\n");
		exit(9);
	}

	//--------------------------------------------------------------------------
	// Open the input and output files and load the XML tree
	//--------------------------------------------------------------------------
	// Uncomment for REALLY verbose output...
	//g_xmlutil_debug = verbose;

	// Load the input XML file
	if (xml_tree.load(infile))
	{
		fprintf(stderr, "ERROR: Failed to read %s\n", infile.c_str());
		exit(9);
	}
	xml_tree.getRoot(xml_root);

	// Merge the cards
	mergeCards();

	// Merge the parts
	mergeParts();

	// Write the resulting tree to the output file
	if (xml_tree.save(outfile))
	{
		fprintf(stderr, "ERROR: Failed to write %s\n", outfile.c_str());
		exit(9);
	}

    return(0);
}


