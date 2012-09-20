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

class IOPar;
class StreamData;
class TextStreamProgressMeter;


namespace ODMad
{
//class ProcFlow;
class MadStream;

mClass(Madagascar) ProcExec : public ::Executor
{
public:

    enum FlowStage	{ Start, Intermediate, Finish };
    			DeclareEnumUtils(FlowStage)

    			ProcExec(const IOPar&,std::ostream&);
    			~ProcExec();

    const IOPar&	pars() const		{ return pars_; }

    const char*		message() const;
    const char*		nrDoneText() const;
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
    std::ostream&	strm_;
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

