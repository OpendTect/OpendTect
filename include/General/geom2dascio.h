#ifndef geom2dascio_h
#define geom2dascio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Sep 2010
 RCS:		$Id: geom2dascio.h,v 1.1 2010/09/30 06:39:33 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "tableascio.h"

namespace PosInfo { class Line2DData; }
namespace Table { class FormatDesc; }

mClass Geom2dAscIO : public Table::AscIO
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
