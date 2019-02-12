#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "seis2dlineio.h"
#include "survgeometrytransl.h"
#include "uistring.h"

class SeisTrc;
class SEGYSeisTrcTranslator;

namespace PosInfo { class Line2DData; }


mExpClass(Seis) SEGYDirect2DLineIOProvider : public Seis2DLineIOProvider
{ mODTextTranslationClass(SEGYDirect2DLineIOProvider);
public:

			SEGYDirect2DLineIOProvider();

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


mExpClass(Seis) SEGYDirect2DLinePutter : public Seis2DLinePutter
{ mODTextTranslationClass(SEGYDirect2DLinePutter);
public:

			SEGYDirect2DLinePutter(const IOObj&,Pos::GeomID);
			~SEGYDirect2DLinePutter();

    uiString		errMsg() const			{ return errmsg_;}
    int			nrWritten() const		{ return nrwr_; }
    bool		put(const SeisTrc&);
    bool		close();

    int			nrwr_;
    BufferString	fname_;
    uiString		errmsg_;
    SEGYSeisTrcTranslator* tr_;
    BinID		bid_;
    DataCharacteristics::UserType preseldt_;

};


mExpClass(Seis) SEGYDirectSurvGeom2DTranslator : public SurvGeom2DTranslator
{
			isTranslator(SEGYDirect,SurvGeom2D);
public:
			SEGYDirectSurvGeom2DTranslator(const char* s1,
						       const char* s2)
			    : SurvGeom2DTranslator(s1,s2)	{}

    Geometry2D*		readGeometry(const IOObj&,uiString&) const;
    bool		writeGeometry(IOObj&,const Geometry2D&,uiString&) const;
    bool		isUserSelectable(bool fr) const		{ return fr; }

    static const char*	sKeySEGYDirectID();

};
