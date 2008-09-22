
#ifndef tuthortools_h
#define tuthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
 * ID       : $Id: tuthortools.h,v 1.7 2008-09-22 13:17:03 cvskris Exp $
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

    			HorTools(const char* title)
			    : Executor(title)
			    , horizon1_(0)
			    , horizon2_(0)
			    , iter_(0)
			    , nrdone_(0)
			{}
    virtual		~HorTools();

    void		setHorizons(EM::Horizon3D* hor1,EM::Horizon3D* hor2=0);
    od_int64		totalNr() const;
    od_int64		nrDone() const		{ return nrdone_; }
    void		setHorSamp(StepInterval<int>,StepInterval<int>);
    const char*		message() const		{ return "Computing..."; }
    const char*		nrDoneText() const	{ return "Points done"; }    

protected:

    BinID		bid_;
    HorSampling		hs_;
    int			nrdone_;

    HorSamplingIterator*	iter_;

    EM::Horizon3D*	horizon1_;
    EM::Horizon3D*	horizon2_;

};


class ThicknessFinder : public HorTools
{
public:
    			ThicknessFinder()
			    : HorTools("Thickness Calculation")
			{}

    int			nextStep();
    Executor*		dataSaver();
    void		init(const char*);

protected:
    EM::PosID           posid_;
    int                 dataidx_;

};


class HorSmoothener : public HorTools
{
public:
			HorSmoothener()
			    : HorTools("Horizon Smoothing")
			{}

    int			nextStep();
    void		setWeaksmooth(bool weak)	{ weaksmooth_ = weak; }
    Executor*		dataSaver(const MultiID&);
protected:

    EM::SubID		subid_;
    bool		weaksmooth_;
};


} // namespace

#endif
