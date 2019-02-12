#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2004
________________________________________________________________________

-*/

#include "seis2dlineio.h"
#include "uistring.h"

class SeisTrc;
class CBVSSeisTrcTranslator;

namespace PosInfo { class Line2DData; }


mExpClass(Seis) SeisCBVS2DLineIOProvider : public Seis2DLineIOProvider
{ mODTextTranslationClass(SeisCBVS2DLineIOProvider);
public:

			SeisCBVS2DLineIOProvider();

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

    static BufferString	getFileName(const IOObj&,Pos::GeomID);

private:

    static int		factid_;
};


mExpClass(Seis) SeisCBVS2DLinePutter : public Seis2DLinePutter
{ mODTextTranslationClass(SeisCBVS2DLinePutter);
public:

			SeisCBVS2DLinePutter(const IOObj&,Pos::GeomID);
			~SeisCBVS2DLinePutter();

    uiString		errMsg() const			{ return errmsg_;}
    int			nrWritten() const		{ return nrwr_; }
    bool		put(const SeisTrc&);
    bool		close();

    int			nrwr_;
    BufferString	fname_;
    uiString		errmsg_;
    CBVSSeisTrcTranslator* tr_;
    BinID		bid_;
    DataCharacteristics::UserType preseldt_;

};
