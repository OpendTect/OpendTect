#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "tableascio.h"

namespace PosInfo { class Line2DData; }
namespace Survey { class Geometry2D; }
namespace Table { class FormatDesc; }

/*!
\brief Ascii I/O for 2D Geometry.
*/

mExpClass(General) Geom2dAscIO : public Table::AscIO
{ mODTextTranslationClass(Geom2dAscIO)
public:
				Geom2dAscIO(const Table::FormatDesc&,
					    od_istream&);

    static Table::FormatDesc*	getDesc(bool withline=false);
    static void			fillDesc(Table::FormatDesc&,bool withline);
    bool			getData(PosInfo::Line2DData&);
    bool			getData(Survey::Geometry2D&) const;
    bool			getData(ObjectSet<Survey::Geometry2D>&) const;

protected:

    od_istream&			strm_;
    bool			readLine(int startidx,Coord&,int& trcnr,
					 float& spnr,bool isxy,bool conv) const;
};
