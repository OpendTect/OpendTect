/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/

static const char* rcsID = "$Id: madprocexec.cc,v 1.3 2008-12-24 06:42:55 cvsdgb Exp $";

#include "madprocexec.h"
#include "madprocflow.h"

static const char* sKeyInp = "Input";

ODMad::ProcExec::ProcExec( const ProcFlow& flow )
    : Executor("Madagascar processing")
    , flow_(*new ODMad::ProcFlow(flow))
    , nrdone_(0)
{
}


ODMad::ProcExec::ProcExec( const IOPar& iop )
    : Executor("Madagascar processing")
    , flow_(*new ODMad::ProcFlow)
    , nrdone_(0)
{
    flow_.usePar( iop );
}


ODMad::ProcExec::~ProcExec()
{
    delete &flow_;
}


const char* ODMad::ProcExec::message() const
{
    return "Working";
}


const char* ODMad::ProcExec::nrDoneText() const
{
    return "Traces handled";
}


od_int64 ODMad::ProcExec::totalNr() const
{
    return -1;
}


int ODMad::ProcExec::nextStep()
{
    return Executor::Finished();
}
