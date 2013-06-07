/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Nanne Hemstra
 DATE:		February 2013
_______________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "matlabstep.h"

#include "arraynd.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "sharedlibs.h"
#include "task.h"

#ifdef HAS_MATLAB

#include "matlabarray.h"
#include "matlablibmgr.h"

extern "C" {
    typedef void (*odfn)(int,mxArray**,const mxArray*);
};
#endif

namespace VolProc
{

class MatlabTask : public ParallelTask
{
public:
MatlabTask( MatlabStep& step, const Array3D<float>& in, Array3D<float>& out )
    : input_(in)
    , output_(out)
    , step_(step)
{
}

    od_int64	nrIterations() const	{ return 1; }
    bool	doWork(od_int64,od_int64,int);
    const char*	message() const		{ return message_.buf(); }

protected:

    const Array3D<float>&	input_;
    Array3D<float>&		output_;
    MatlabStep&			step_;
    BufferString		message_;
};


#ifdef HAS_MATLAB

#define mErrRet(msg) { message_ = msg; return false; }

bool MatlabTask::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !MLM().initApplication() )
	mErrRet( MLM().errMsg() );

    MatlabLibAccess mla( step_.sharedLibFileName() );
    if ( !mla.init() )
	mErrRet( mla.errMsg() );

    const char* odfnm = "mlfOd_doprocess";
    odfn fn = (odfn)mla.getFunction( odfnm );
    if ( !fn )
	mErrRet( mla.errMsg() );

    ArrayNDCopier arrndcopier( input_ );
    arrndcopier.execute();
    const mxArray* mxarrin = arrndcopier.getMxArray();
    mxArray* mxarrout = NULL;

    (*fn)( 1, &mxarrout, mxarrin );

    mxArrayCopier mxarrcopier( *mxarrout, output_ );
    mxarrcopier.execute();

    mla.terminate();
    MLM().terminateApplication();

    return true;
}

#else

bool MatlabTask::doWork( od_int64 start, od_int64 stop, int )
{
    return false;
}

#endif


// MatlabStep
MatlabStep::MatlabStep()
{}

MatlabStep::~MatlabStep()
{}


Task* MatlabStep::createTask()
{
    if ( !input_ || input_->nrCubes()<1 )
    {
	errmsg_ = "No input provided.";
	return 0;
    }

    if ( !output_ || output_->nrCubes()<1 )
    {
	errmsg_ = "No output provided.";
	return 0;
    }

    return new MatlabTask( *this, input_->getCube(0), output_->getCube(0) );
}


void MatlabStep::setSharedLibFileName( const char* fnm )
{ sharedlibfnm_ = fnm; }

const char* MatlabStep::sharedLibFileName() const
{ return sharedlibfnm_; }


void MatlabStep::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    par.set( sKey::FileName(), sharedLibFileName() );
}


bool MatlabStep::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    par.get( sKey::FileName(), sharedlibfnm_ );
    return true;
}

} // namespace VolProc
