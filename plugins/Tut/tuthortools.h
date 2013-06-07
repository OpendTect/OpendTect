#ifndef tuthortools_h
#define tuthortools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
 * ID       : $Id: tuthortools.h,v 1.12 2010/02/09 05:15:28 cvsnanne Exp $
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

mClass HorTool : public Executor
{
public:
    virtual		~HorTool();

    void		setHorizons(EM::Horizon3D* hor1,EM::Horizon3D* hor2=0);
    od_int64		totalNr() const;
    od_int64		nrDone() const		{ return nrdone_; }
    void		setHorSamp(const StepInterval<int>& inlrg,
		    		   const StepInterval<int>& crlrg);
    const char*		nrDoneText() const	{ return "Positions done"; }    

protected:
			HorTool(const char* title);

    BinID		bid_;
    HorSampling		hs_;
    int			nrdone_;

    HorSamplingIterator* iter_;

    EM::Horizon3D*	horizon1_;
    EM::Horizon3D*	horizon2_;

};





mClass ThicknessCalculator : public HorTool
{
public:
    			ThicknessCalculator();

    int			nextStep();
    Executor*		dataSaver();
    void		init(const char*);

    const char*		message() const	{ return "Calculating thickness"; }    

protected:

    EM::PosID		posid_;
    int			dataidx_;
    const float		usrfac_;

};


mClass HorSmoother : public HorTool
{
public:
			HorSmoother();
			   
    int			nextStep();
    void		setWeak( bool yn )	{ weak_ = yn; }
    Executor*		dataSaver(const MultiID&);

    const char*		message() const	{ return "Smoothing"; }    

protected:

    EM::SubID		subid_;
    bool		weak_;

};

} // namespace

#endif
