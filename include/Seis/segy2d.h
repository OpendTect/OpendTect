#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2020
________________________________________________________________________

-*/

#include "seis2dlineio.h"
#include "survgeometrytransl.h"
#include "uistring.h"

class SeisTrc;
class SEGYSeisTrcTranslator;

namespace PosInfo { class Line2DData; }


mExpClass(Seis) SEGY2DLineIOProvider : public Seis2DLineIOProvider
{ mODTextTranslationClass(SEGY2DLineIOProvider);
public:

			SEGY2DLineIOProvider();

    bool		isEmpty(const IOObj&,Pos::GeomID) const;

    uiRetVal		getGeomIDs(const IOObj&,GeomIDSet&) const;
    uiRetVal		getGeometry(const IOObj&,Pos::GeomID,
				    PosInfo::Line2DData&) const;
    Seis2DTraceGetter*	getTraceGetter(const IOObj&,Pos::GeomID,
					const Seis::SelData*,uiRetVal&);
    Seis2DLinePutter*	getPutter(const IOObj& obj,Pos::GeomID,uiRetVal&);

    bool		getTxtInfo(const IOObj&,Pos::GeomID,BufferString&,
				   BufferString&) const;
    bool		getRanges(const IOObj&,Pos::GeomID,StepInterval<int>&,
				  StepInterval<float>&) const;

    bool		removeImpl(const IOObj&,Pos::GeomID) const;
    bool		renameImpl(const IOObj&,const char*) const;

private:

    static int		factid_;

};
