#ifndef madprocexec_h
#define madprocexec_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Jan 2008
 * ID       : $Id: madprocexec.h,v 1.1 2008-01-28 16:38:58 cvsbert Exp $
-*/

#include "executor.h"
class IOPar;


namespace ODMad
{
class ProcFlow;

class ProcExec : public ::Executor
{
public:

    			ProcExec(const ProcFlow&);
    			ProcExec(const IOPar&);
    			~ProcExec();

    const ProcFlow&	flow() const		{ return flow_; }

    const char*		message() const;
    const char*		nrDoneText() const;
    int			nrDone() const		{ return nrdone_; }
    int			totalNr() const;
    int			nextStep();

protected:

    ProcFlow&		flow_;
    int			nrdone_;

};

} // namespace ODMad

#endif
