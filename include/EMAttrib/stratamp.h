#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emattribmod.h"

#include "emposid.h"
#include "stattype.h"
#include "task.h"
#include "trckeysampling.h"

class TaskRunner;
class SeisTrcReader;

namespace EM { class Horizon3D; }
namespace Attrib { class DescSet; class Processor; }

mExpClass(EMAttrib) StratAmpCalc  : public SequentialTask
{ mODTextTranslationClass(StratAmpCalc)
public:

    enum class SumMode		{ All, Positive, Negative };

			StratAmpCalc(const EM::Horizon3D&,const EM::Horizon3D*,
				     const TrcKeySampling&,bool outputfold);
			~StratAmpCalc();

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override;

    bool		usePar(const IOPar&);

    const TypeSet<int>& attribIdxs() const;
    const TypeSet<int>& foldAttribIdxs() const;
    int			getFoldIdx() const;

    static const char*	sKeyTopHorizonID();
    static const char*	sKeyBottomHorizonID();
    static const char*	sKeySingleHorizonYN();
    static const char*	sKeyAddToTopYN();
    static const char*	sKeyAmplitudeOption();
    static const char*	sKeyTopShift();
    static const char*	sKeyBottomShift();
    static const char*	sKeyOutputFoldYN();
    static const char*	sKeyAttribName();
    static const char*	sKeyIsClassification();
    static const char*	sKeyIsOverwriteYN();

private:

    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream* =nullptr) override;

    od_int64		totalNr() const override	{ return totnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }

    Stats::Type			stattyp_;
    SumMode			summode_			= SumMode::All;
    bool			isclassification_		= false;
    SeisTrcReader*		rdr_				= nullptr;
    bool			usesstored_			= false;
    ConstRefMan<EM::Horizon3D>	tophorizon_;
    ConstRefMan<EM::Horizon3D>	bothorizon_;
    int				nrdone_;
    int				totnr_;
    float			tophorshift_			= mUdf(float);
    float			bothorshift_			= mUdf(float);
    EM::PosID			posid_;
    EM::PosID			posidfold_;
    TypeSet<int>		selcomps_;
    TypeSet<int>		dataidxs_;
    TypeSet<int>		dataidxsfold_;
    bool			addtotop_;
    bool			outfold_;
    TrcKeySampling		hs_;
    Attrib::DescSet*		descset_;
    Attrib::Processor*		proc_				= nullptr;

    mutable uiString		msg_;
};
