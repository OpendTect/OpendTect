#ifndef madprocexec_h
#define madprocexec_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2008
 * ID       : $Id$
-*/

#include "madagascarmod.h"
#include "enums.h"
#include "executor.h"
#include "od_ostream.h"

class StreamData;
class TextStreamProgressMeter;


namespace ODMad
{
//class ProcFlow;
class MadStream;

mExpClass(Madagascar) ProcExec : public ::Executor
{
public:

    enum FlowStage	{ Start, Intermediate, Finish };
    			DeclareEnumUtils(FlowStage)

    			ProcExec(const IOPar&,od_ostream&);
    			~ProcExec();

    const IOPar&	pars() const		{ return pars_; }

    uiStringCopy		uiMessage() const;
    uiStringCopy		uiNrDoneText() const;
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;
    int			nextStep();

    bool		init();
    BufferString	errMsg()		{ return errmsg_; }

    static const char*	sKeyFlowStage();
    static const char*	sKeyCurProc();

protected:

    IOPar&		pars_;
    FlowStage		stage_;
//    ProcFlow&		procflow_;
    BufferString	errmsg_;
    od_ostream&		strm_;
    int			nrdone_;
    float*		trc_;

    MadStream*		madstream_;
    StreamData&		procstream_;
    StreamData&		plotstream_;

    TextStreamProgressMeter* progmeter_;

    const char*		getProcString();
    const char*		getPlotString() const;

};

} // namespace ODMad

#endif

