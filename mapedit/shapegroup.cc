/**
 **	A group of shapes/chunks that can be used as a 'palette'.
 **
 **	Written: 1/22/02 - JSF
 **/

/*
Copyright (C) 2002 The Exult Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cassert>
#include <fstream>
#include "shapegroup.h"
#include "Flex.h"
#include "utils.h"
#include "exceptions.h"

/*
 *	Create an empty group.
 */

Shape_group::Shape_group
	(
	char *nm,			// Name.  Copied.
	Shape_group_file *f
	) : name(nm), file(f)
	{
	}

/*
 *	Init. and read in (if it exists) a groups file.
 */

Shape_group_file::Shape_group_file
	(
	char *nm			// Basename.
	) : name(nm), modified(false)
	{
	Flex *flex = 0;
	std::string patchname = "<PATCH>/" + name;
	std::string staticname = "<STATIC>/" + name;
	if (U7exists(patchname))	// First try 'patch' directory.
		flex = new Flex(patchname);
	else if (U7exists(staticname))
		flex = new Flex(staticname);
	if (flex)			// Exists?
		{
		int cnt = flex->number_of_objects();
		for (int i = 0; i < cnt; i++)
			{		// Get each group.
			std::size_t len;
			char *buf = flex->retrieve(i, len);
			char *gname = buf;	// Starts with name.
			unsigned char *ptr = (unsigned char *)
						gname + strlen(gname) + 1;
			int sz = Read2(ptr);	// Get # entries.
			assert ((len - ((char *) ptr - buf))/2 == sz);
			Shape_group *grp = new Shape_group(gname, this);
			grp->reserve(sz);
			for (int j = 0; j < sz; j++)
				grp->push_back(Read2(ptr));
			groups.push_back(grp);
			delete buf;
			}
		delete flex;
		}
	}

/*
 *	Clean up.
 */

Shape_group_file::~Shape_group_file
	(
	)
	{
	for (vector<Shape_group *>::iterator it = groups.begin();
						it != groups.end(); ++it)
		delete (*it);		// Delete each group.
	}

/*
 *	Remove and delete a group.
 */

void Shape_group_file::remove
	(
	int index
	)
	{
	assert(index >= 0 && index < groups.size());
	delete groups[index];
	groups.erase(groups.begin() + index);
	}

/*
 *	Write out to the 'patch' directory.
 */

void Shape_group_file::write
	(
	)
	{
	std::ofstream out;
	std::string patchname = "<PATCH>/" + name;
	U7open(out, patchname.c_str());
	int cnt = groups.size();	// # groups.
	Flex::write_header(out, "ExultStudio shape groups", cnt);
	unsigned char *table = new unsigned char[2*cnt*4];
	uint8 *tptr = table;
	int i;				// Write each group.
	for (i = 0; i < cnt; i++)
		{
		Write4(tptr, out.tellp());
		Shape_group *grp = groups[i];
		const char *nm = grp->get_name();
		int sz = grp->size();
					// Name, #entries, entries(2-bytes).
		long len = strlen(nm) + 1 + 2 + 2*sz;
		Write4(tptr, len);
		unsigned char *buf = new unsigned char[len];
		strcpy((char *) buf, nm);
		unsigned char *ptr = buf + strlen(nm) + 1;
		Write2(ptr, sz);	// # entries.
		for (vector<int>::iterator it = grp->begin();
						it != grp->end(); it++)
			Write2(ptr, *it);
		assert(ptr - buf == len);
		out.write(buf, len);	// Write out to file.
		delete buf;
		}
	out.seekp(0x80, ios::beg);	// Write table.
	out.write(reinterpret_cast<char*>(table), 2*cnt*4);
	delete [] table;
	out.flush();
	bool result = out.good();
	if (!result)			// ++++Better error system needed??
		throw file_write_exception(patchname.c_str());
	out.close();
	}
