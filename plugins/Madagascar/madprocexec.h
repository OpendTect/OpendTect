#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagascarmod.h"
#include "enums.h"
#include "executor.h"
#include "od_ostream.h"
#include "uistring.h"
//#include <fstream>

class TextStreamProgressMeter;


namespace ODMad
{
//class ProcFlow;
class MadStream;

mExpClass(Madagascar) ProcExec : public ::Executor
{ mODTextTranslationClass(ProcExec);
public:

    enum FlowStage	{ Start, Intermediate, Finish };
    			mDeclareEnumUtils(FlowStage)

    			ProcExec(const IOPar&,od_ostream&);
    			~ProcExec();

    const IOPar&	pars() const		{ return pars_; }

    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;
    int			nextStep();

    bool		init();
    uiString		errMsg()		{ return errmsg_; }

    static const char*	sKeyFlowStage();
    static const char*	sKeyCurProc();

protected:

    IOPar&		pars_;
    FlowStage		stage_;
//    ProcFlow&		procflow_;
    uiString		errmsg_;
    od_ostream&		strm_;
    int			nrdone_;
    float*		trc_;

    MadStream*		madstream_;

    FILE*		procfptr_;
    FILE*		plotfptr_;

    od_ostream*		procstream_;
    od_ostream*		plotstream_;

    TextStreamProgressMeter* progmeter_;

    const char*		getProcString();
    const char*		getPlotString() const;

};

} // namespace ODMad
