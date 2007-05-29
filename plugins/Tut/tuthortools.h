
#ifndef tuthortools_h
#define tuthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
 * ID       : $ $
-*/

#include "executor.h"
#include "bufstring.h"
#include "emmanager.h"
#include "emposid.h"
#include "emhorizon.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "cubesampling.h"
#include "survinfo.h"

class IOObj;


namespace Tut
{

class HorTools : public Executor
{
public:

    			HorTools()
			    : Executor("Tutorial Tools : Thickness Calculation")
			    , hortop_(0)
			    , horbot_(0)
			    , horout_(0)
			    , iter_(0)
			    , nrdone_(0)
			{}
    virtual		~HorTools();

    void		setTop(EM::Horizon3D* hor)	{ hortop_ = hor; }
    void		setBot(EM::Horizon3D* hor)      { horbot_ = hor; }
    void		setOutHor(bool top);
    void		setIdx(int idx)			{ dataidx_ = idx; }
    void		setId(EM::PosID id)		{ posid_ = id; }
    EM::Horizon3D*	getOutHor()			{ return horout_; }
    int			totalNr() const;
    int			nrDone() const			{ return nrdone_; }
    const char*         nrDoneText() const      { return "Points Computed"; }
    const char*		message() const		{return "Computing Thickness";}
    void		setHorSamp(StepInterval<int>,StepInterval<int>);

    int			nextStep();
protected:

    EM::PosID 		posid_;
    BinID		bid_;
    int			dataidx_;
    HorSampling		hs_;
    int			nrdone_;

    HorSamplingIterator*	iter_;

    EM::Horizon3D*	hortop_;
    EM::Horizon3D*	horbot_;
    EM::Horizon3D*	horout_;

};


class HorFilter : public Executor
{
public:

    			HorFilter()
			    : Executor("Tutorial Tools : Horizon Filter")
			    , hor_(0)
			    , iter_(0)
			    , nrdone_(0)
			{}
    virtual		~HorFilter();

    void		setInput(EM::Horizon3D* hor)	{ hor_ = hor; }
    EM::Horizon3D*	getHor()			{ return hor_; }
    int			totalNr() const;
    int			nrDone() const			{ return nrdone_; }
    const char*         nrDoneText() const      { return "Points Done"; }
    const char*		message() const		{ return "Smoothening..."; }
    void		setHorSamp(StepInterval<int>,StepInterval<int>);

    int			nextStep();
protected:

    BinID		bid_;
    EM::SubID		subid_;
    HorSampling		hs_;
    int			nrdone_;

    HorSamplingIterator*	iter_;

    EM::Horizon3D*	hor_;

};


} // namespace

#endif
