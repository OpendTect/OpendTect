#ifndef geom2dascio_h
#define geom2dascio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Sep 2010
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "tableascio.h"

namespace PosInfo { class Line2DData; }
namespace Table { class FormatDesc; }

mClass(General) Geom2dAscIO : public Table::AscIO
{
public:
				Geom2dAscIO(const Table::FormatDesc&,
					    std::istream&);
    static Table::FormatDesc*	getDesc();
    bool			getData(PosInfo::Line2DData&);

protected:

    std::istream&	strm_;
};

#endif

