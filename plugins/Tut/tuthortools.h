	    #pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"
#include "executor.h"
#include "emposid.h"
#include "trckeysampling.h"
#include "ranges.h"

class TrcKeySamplingIterator;

namespace EM { class Horizon3D; }

namespace Tut
{

mExpClass(Tut) HorTool : public Executor
{ mODTextTranslationClass(HorTool);
public:
    virtual	    ~HorTool();

    void	    setHorizons(EM::Horizon3D* hor1,
						EM::Horizon3D* hor2=nullptr);
    od_int64	    totalNr() const override;
    od_int64	    nrDone() const override { return nrdone_; }
    uiString	    uiNrDoneText() const override { return msg_; }

    void	    setHorSamp(const StepInterval<int>& inlrg,
				    const StepInterval<int>& crlrg);

protected:
				HorTool(const char* title);

    TrcKeySampling		hs_;
    int				nrdone_ = 0;
    uiString			msg_;

    TrcKeySamplingIterator*	iter_ = nullptr;

    RefMan<EM::Horizon3D>	horizon1_;
    RefMan<EM::Horizon3D>	horizon2_;
};


mExpClass(Tut) ThicknessCalculator : public HorTool
{ mODTextTranslationClass(ThicknessCalculator);
public:
			ThicknessCalculator();

    Executor*		dataSaver();
    void		setAttribName(const char*);

    uiString		uiMessage() const override
				    { return tr("Calculating thickness"); }

protected:

    EM::PosID		posid_;
    int			dataidx_ = 0;
    const float		usrfac_;

private:
    int			nextStep() override;

};


mExpClass(Tut) HorSmoother : public HorTool
{ mODTextTranslationClass(HorSmoother);
public:
			HorSmoother();

    void		setWeak( bool yn ) { weak_ = yn; }
    Executor*		dataSaver(const MultiID&);

    uiString		uiMessage() const override  { return tr("Smoothing"); }

protected:

    bool		weak_ = false;

private:
    int			nextStep() override;
};

} // namespace
