#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/

#include "tutmod.h"
#include "executor.h"
#include "emposid.h"
#include "trckeysampling.h"
#include "ranges.h"
#include "uistrings.h"

class TrcKeySamplingIterator;

namespace EM { class Horizon3D; }

namespace Tut
{

mExpClass(Tut) HorTool : public Executor
{ mODTextTranslationClass(HorTool);
public:
    virtual		~HorTool();

    void		setHorizons(EM::Horizon3D* hor1,EM::Horizon3D* hor2=0);
    od_int64		totalNr() const;
    od_int64		nrDone() const		{ return nrdone_; }
    void		setHorSamp(const StepInterval<int>& inlrg,
				   const StepInterval<int>& crlrg);
    uiString		nrDoneText() const
			{ return uiStrings::sPositionsDone(); }

protected:
			HorTool(const char* title);

    TrcKeySampling		hs_;
    int			nrdone_;

    TrcKeySamplingIterator* iter_;

    EM::Horizon3D*	horizon1_;
    EM::Horizon3D*	horizon2_;

};





mExpClass(Tut) ThicknessCalculator : public HorTool
{ mODTextTranslationClass(ThicknessCalculator);
public:
			ThicknessCalculator();

    int			nextStep();
    Executor*		dataSaver();
    void		init(const char*);

    uiString		message() const {
					return tr("Calculating thickness");
					  }

protected:

    EM::PosID		posid_;
    int			dataidx_;
    const float		usrfac_;

};


mExpClass(Tut) HorSmoother : public HorTool
{ mODTextTranslationClass(HorSmoother);
public:
			HorSmoother();

    int			nextStep();
    void		setWeak( bool yn )	{ weak_ = yn; }
    Executor*		dataSaver(const DBKey&);

    uiString		message() const	{ return uiStrings::sSmoothing(); }

protected:

    EM::PosID		posid_;
    bool		weak_;

};

} // namespace
