#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Sep 2010
________________________________________________________________________

-*/


#include "generalmod.h"
#include "tableascio.h"

namespace Survey { class Geometry2D; }
namespace Table { class FormatDesc; }

/*!
\brief Ascii I/O for 2D Geometry.
*/

mExpClass(General) Geom2DAscIO : public Table::AscIO
{ mODTextTranslationClass(Geom2DAscIO)
public:
				Geom2DAscIO(const Table::FormatDesc&,
					    od_istream&);
    static Table::FormatDesc*	getDesc(bool withline);
    static void			fillDesc(Table::FormatDesc&,bool withline);
    bool			getData(SurvGeom2D&) const;
    bool			getData(ObjectSet<SurvGeom2D>&) const;

protected:

    od_istream&			strm_;
    bool			readLine(int startidx,Coord&,int&,int&,
					 bool,bool) const;
};
