#ifndef tuthortools_h
#define tuthortools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
 * ID       : $Id: tuthortools.h,v 1.9 2009-04-09 11:49:08 cvsranojay Exp $
-*/

#include "executor.h"
#include "emposid.h"
#include "horsampling.h"
#include "ranges.h"

class IOObj;
class HorSamplingIterator;

namespace EM { class Horizon3D; }

namespace Tut
{

mClass HorTools : public Executor
{
public:

    			HorTools(const char* title);
    virtual		~HorTools();

    void		setHorizons(EM::Horizon3D* hor1,EM::Horizon3D* hor2=0);
    od_int64		totalNr() const;
    od_int64		nrDone() const		{ return nrdone_; }
    void		setHorSamp(const StepInterval<int>& inlrg,
		    		   const StepInterval<int>& crlrg);
    const char*		message() const		{ return "Computing..."; }
    const char*		nrDoneText() const	{ return "Points done"; }    

protected:

    BinID		bid_;
    HorSampling		hs_;
    int			nrdone_;

    HorSamplingIterator* iter_;

    EM::Horizon3D*	horizon1_;
    EM::Horizon3D*	horizon2_;

};





mClass ThicknessFinder : public HorTools
{
public:
    			ThicknessFinder();
			 
    int			nextStep();
    Executor*		dataSaver();
    void		init(const char*);

protected:
    EM::PosID		posid_;
    int			dataidx_;

};


mClass HorSmoother : public HorTools
{
public:
			HorSmoother();
			   
    int			nextStep();
    void		setWeak( bool yn )	{ weak_ = yn; }
    Executor*		dataSaver(const MultiID&);
protected:

    EM::SubID		subid_;
    bool		weak_;
};


} // namespace

#endif
