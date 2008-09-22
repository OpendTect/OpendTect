#ifndef madprocexec_h
#define madprocexec_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Jan 2008
 * ID       : $Id: madprocexec.h,v 1.2 2008-09-22 13:17:03 cvskris Exp $
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
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;
    int			nextStep();

protected:

    ProcFlow&		flow_;
    int			nrdone_;

};

} // namespace ODMad

#endif
