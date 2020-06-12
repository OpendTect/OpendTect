#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2008
-*/

#include "madagascarmod.h"
#include "enums.h"
#include "executor.h"
#include "od_ostream.h"
#include "uistring.h"

class TextStreamProgressMeter;

namespace OS { class MachineCommand; }


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

    uiString		message() const;
    uiString		nrDoneText() const;
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
    od_ostream*		procstream_;
    od_ostream*		plotstream_;

    TextStreamProgressMeter* progmeter_;

    bool		getProcString(OS::MachineCommand&);
    bool		getPlotString(OS::MachineCommand&) const;

};

} // namespace ODMad
