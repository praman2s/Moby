/****************************************************************************
 * Copyright 2007 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifndef _MOBY_XML_WRITER_H
#define _MOBY_XML_WRITER_H

#include <list>
#include <string>
#include <Moby/Types.h>

namespace Moby {

/// Class for writing Base objects (and their derivatives) to XML
class XMLWriter
{
  public:
    static void serialize_to_xml(const std::string& filename, const std::list<boost::shared_ptr<const Base> >& objects);
    static void serialize_to_xml(const std::string& filename, boost::shared_ptr<const Base> object);
};

} // end namespace

#endif

