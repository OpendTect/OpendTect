#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seis2dlineio.h"
#include "uistring.h"

class SeisTrc;
class CBVSSeisTrcTranslator;

namespace PosInfo { class Line2DData; }
namespace Survey { class Geometry2D; }


mExpClass(Seis) SeisCBVS2DLineIOProvider : public Seis2DLineIOProvider
{
public:

			SeisCBVS2DLineIOProvider();
			~SeisCBVS2DLineIOProvider();

    bool		isEmpty(const IOObj&,Pos::GeomID) const override;

    bool		getGeomIDs(const IOObj&,
				   TypeSet<Pos::GeomID>&) const override;
    bool		getGeometry(const IOObj&,Pos::GeomID,
				    PosInfo::Line2DData&) const override;
    Executor*		getFetcher(const IOObj&,Pos::GeomID,SeisTrcBuf&,int,
				   const Seis::SelData* sd=0) override;
    Seis2DLinePutter*	getPutter(const IOObj& obj,Pos::GeomID) override;

    bool		getTxtInfo(const IOObj&,Pos::GeomID,BufferString&,
				   BufferString&) const override;
    bool		getRanges(const IOObj&,Pos::GeomID,StepInterval<int>&,
				  StepInterval<float>&) const override;

    bool		removeImpl(const IOObj&,Pos::GeomID) const override;
    bool		renameImpl(const IOObj&,const char*) const override;

    static const OD::String&	getFileName(const IOObj&,Pos::GeomID);

private:

    static int		factid_;
};


mExpClass(Seis) SeisCBVS2DLinePutter : public Seis2DLinePutter
{ mODTextTranslationClass(SeisCBVS2DLinePutter);
public:

			SeisCBVS2DLinePutter(const IOObj&,Pos::GeomID);
			~SeisCBVS2DLinePutter();

    uiString		errMsg() const override		{ return errmsg_;}
    int			nrWritten() const override	{ return nrwr_; }
    bool		put(const SeisTrc&) override;
    bool		close() override;
    void		setComponentNames(const BufferStringSet&) override;

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
			SeisCBVS2DLineGetter(const char* fnm,Pos::GeomID,
					     SeisTrcBuf&,int trcsperstep,
					     const Seis::SelData&);
			~SeisCBVS2DLineGetter();

    od_int64		nrDone() const override		{ return curnr_; }
    od_int64		totalNr() const override	{ return totnr_; }

    const SeisTrcTranslator*	translator() const override;

protected:

    void		addTrc(SeisTrc*);
    int			nextStep() override;

    int				curnr_		= 0;
    int				totnr_		= 0;
    BufferString		fname_;
    CBVSSeisTrcTranslator*	tr_;
    const Survey::Geometry2D&	geom2d_;
    int				trcstep_	= 1;
    const int			linenr_;
    const int			trcsperstep_;

};
