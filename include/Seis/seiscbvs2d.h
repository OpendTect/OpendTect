#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seis2dlineio.h"
#include "uistring.h"

class SeisTrc;
class CBVSSeisTrcTranslator;

namespace PosInfo { class Line2DData; }


mExpClass(Seis) SeisCBVS2DLineIOProvider : public Seis2DLineIOProvider
{
public:

			SeisCBVS2DLineIOProvider();

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


mExpClass(Seis) SeisCBVS2DLinePutter : public Seis2DLinePutter
{ mODTextTranslationClass(SeisCBVS2DLinePutter);
public:

			SeisCBVS2DLinePutter(const IOObj&,Pos::GeomID);
			~SeisCBVS2DLinePutter();

    uiString		errMsg() const			{ return errmsg_;}
    int			nrWritten() const		{ return nrwr_; }
    bool		put(const SeisTrc&);
    bool		close();
    void		setComponentNames(const BufferStringSet&);

    int                 		nrwr_;
    BufferString        		fname_;
    uiString				errmsg_;
    CBVSSeisTrcTranslator*		tr_;
    BinID               		bid_;
    DataCharacteristics::UserType	preseldt_;

};


class SeisCBVS2DLineGetter : public Seis2DLineGetter
{
public:
			SeisCBVS2DLineGetter(const char* fnm,SeisTrcBuf&,
					     int trcsperstep,
					     const Seis::SelData&);
			SeisCBVS2DLineGetter(const char* fnm,Pos::GeomID,
					     SeisTrcBuf&,int trcsperstep,
					     const Seis::SelData&);
			~SeisCBVS2DLineGetter();

    od_int64		nrDone() const		{ return curnr_; }
    od_int64		totalNr() const		{ return totnr_; }

    const SeisTrcTranslator*	translator() const;

protected:

    void		addTrc(SeisTrc*);
    int			nextStep();

    int				curnr_;
    int				totnr_;
    BufferString		fname_;
    CBVSSeisTrcTranslator*	tr_;
    int				trcstep_;
    const int			linenr_;
    const int			trcsperstep_;

};

