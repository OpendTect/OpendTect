#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "seismod.h"
#include "survgeometrytransl.h"
#include "seis2dlineio.h"
#include "uistring.h"

class SeisTrc;
class SEGYSeisTrcTranslator;

namespace PosInfo { class Line2DData; }


mExpClass(Seis) SEGYDirect2DLineIOProvider : public Seis2DLineIOProvider
{
public:

			SEGYDirect2DLineIOProvider();

    bool		isEmpty(const IOObj&,Pos::GeomID) const;

    bool		getGeomIDs(const IOObj&,TypeSet<Pos::GeomID>&) const;
    bool		getGeometry(const IOObj&,Pos::GeomID,
				    PosInfo::Line2DData&) const;
    Executor*		getFetcher(const IOObj&,Pos::GeomID,SeisTrcBuf&,int,
				   const Seis::SelData* sd=0);
    Seis2DLinePutter*	getPutter(const IOObj& obj,Pos::GeomID);

    bool		getTxtInfo(const IOObj&,Pos::GeomID,BufferString&,
				   BufferString&) const;
    bool		getRanges(const IOObj&,Pos::GeomID,StepInterval<int>&,
				  StepInterval<float>&) const;

    bool		removeImpl(const IOObj&,Pos::GeomID) const;
    bool		renameImpl(const IOObj&,const char*) const;

    static const OD::String&	getFileName(const IOObj&,Pos::GeomID);

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

    int					nrwr_ = 0;
    BufferString			fname_;
    uiString				errmsg_;
    SEGYSeisTrcTranslator*		tr_;
    BinID				bid_;
    DataCharacteristics::UserType	preseldt_;

};


mExpClass(Seis) SEGYDirect2DLineGetter : public Seis2DLineGetter
{
public:
			SEGYDirect2DLineGetter(const char* fnm,SeisTrcBuf&,
					     int trcsperstep,
					     const Seis::SelData&);
			~SEGYDirect2DLineGetter();

    od_int64		nrDone() const		{ return curnr_; }
    od_int64		totalNr() const		{ return totnr_; }

    const SeisTrcTranslator*	translator() const;

protected:

    void		addTrc(SeisTrc*);
    int			nextStep();

    int				curnr_ = 0;
    int				totnr_ = 0;
    BufferString		fname_;
    SEGYSeisTrcTranslator*	tr_;
    const int			trcsperstep_;

};


mExpClass(Seis) SEGYDirectSurvGeom2DTranslator : public SurvGeom2DTranslator
{
			isTranslator(SEGYDirect,SurvGeom2D);
public:
			SEGYDirectSurvGeom2DTranslator(const char* s1,
						       const char* s2)
			    : SurvGeom2DTranslator(s1,s2)	{}

    Survey::Geometry*	readGeometry(const IOObj&,uiString&) const;
    bool		writeGeometry(IOObj&,Survey::Geometry&,uiString&) const;
    bool		isUserSelectable(bool fr) const		{ return fr; }

    static const char*	sKeySEGYDirectID();

};

