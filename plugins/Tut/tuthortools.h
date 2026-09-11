	    #pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"

#include "emhorizon3d.h"
#include "emposid.h"
#include "executor.h"
#include "ranges.h"
#include "trckeysampling.h"


namespace Tut
{

mExpClass(Tut) HorTool : public Executor
{ mODTextTranslationClass(HorTool);
public:
		    ~HorTool();

    void	    setHorizons(EM::Horizon3D& hor1,
				EM::Horizon3D* hor2=nullptr);
    od_int64	    totalNr() const override;
    od_int64	    nrDone() const override { return nrdone_; }
    uiString	    uiNrDoneText() const override { return msg_; }

    void	    setHorSamp(const StepInterval<int>& inlrg,
			       const StepInterval<int>& crlrg);

protected:
		    HorTool(const char* title);

    bool	    doPrepare(od_ostream* =nullptr) override;
    bool	    doFinish(bool success, od_ostream* = nullptr) override;

    TrcKeySampling		hs_;
    int				nrdone_			=0;
    uiString			msg_;

    TrcKeySamplingIterator*	iter_			=nullptr;
    BinID			bid_;

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
    bool		doPrepare( od_ostream* =nullptr) override;
};


mExpClass(Tut) HorSmoother : public HorTool
{ mODTextTranslationClass(HorSmoother);
public:
			HorSmoother(EM::Horizon3D&);

    void		setWeak( bool yn ) { weak_ = yn; }
    Executor*		dataSaver(const MultiID&);

    uiString		uiMessage() const override  { return tr("Smoothing"); }

protected:

    bool			weak_			=false;

private:
    int				nextStep() override;
    bool			doPrepare(od_ostream* = nullptr) override;
    RefMan<EM::Horizon3D>	horizonoutput_;
};

} // namespace
