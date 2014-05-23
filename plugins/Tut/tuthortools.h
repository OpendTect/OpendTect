#ifndef tuthortools_h
#define tuthortools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
 * ID       : $Id$
-*/

#include "tutmod.h"
#include "executor.h"
#include "emposid.h"
#include "horsampling.h"
#include "ranges.h"

class HorSamplingIterator;

namespace EM { class Horizon3D; }

namespace Tut
{

mExpClass(Tut) HorTool : public Executor
{
public:
    virtual		~HorTool();

    void		setHorizons(EM::Horizon3D* hor1,EM::Horizon3D* hor2=0);
    od_int64		totalNr() const;
    od_int64		nrDone() const		{ return nrdone_; }
    void		setHorSamp(const StepInterval<int>& inlrg,
		    		   const StepInterval<int>& crlrg);
    uiStringCopy	uiNrDoneText() const	{ return "Positions done"; }

protected:
			HorTool(const char* title);

    HorSampling		hs_;
    int			nrdone_;

    HorSamplingIterator* iter_;

    EM::Horizon3D*	horizon1_;
    EM::Horizon3D*	horizon2_;

};





mExpClass(Tut) ThicknessCalculator : public HorTool
{
public:
    			ThicknessCalculator();

    int			nextStep();
    Executor*		dataSaver();
    void		init(const char*);

    uiStringCopy	uiMessage() const { return "Calculating thickness"; }

protected:

    EM::PosID		posid_;
    int			dataidx_;
    const float		usrfac_;

};


mExpClass(Tut) HorSmoother : public HorTool
{
public:
			HorSmoother();
			   
    int			nextStep();
    void		setWeak( bool yn )	{ weak_ = yn; }
    Executor*		dataSaver(const MultiID&);

    uiStringCopy		uiMessage() const	{ return "Smoothing"; }

protected:

    EM::SubID		subid_;
    bool		weak_;

};

} // namespace

#endif

