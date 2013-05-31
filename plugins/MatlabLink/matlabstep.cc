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
    void mclmcrInitialize();
    bool mclInitializeApplication(const char **a0, size_t a1);
    void mclTerminateApplication();
    typedef void (*odfn)(int,mxArray**,const mxArray*);
    typedef bool (*initfn)();
    typedef void (*termfn)();
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

static BufferString getFnm( const char* libfnm, bool init )
{
    FilePath fp( libfnm );
    fp.setExtension( "" );
    return BufferString ( fp.fileName(), init ? "Initialize" : "Terminate" );
}

#define mErrRet(msg) { message_ = msg; return false; }

bool MatlabTask::doWork( od_int64 start, od_int64 stop, int )
{
    // TODO: Create object for this workflow
    mclmcrInitialize();
    if( !mclInitializeApplication(NULL,0) )
	mErrRet( "Cannot initialize MATLAB application" );

    ArrayNDCopier arrndcopier( input_ );
    arrndcopier.execute();

    const mxArray* mxarrin = arrndcopier.getMxArray();
    mxArray* mxarrout = NULL;

    const char* shlibfnm = step_.sharedLibFileName();
    if ( !MLM().isLoaded(shlibfnm) && !MLM().load(shlibfnm) )
	mErrRet( "Cannot load shared library" );

    const SharedLibAccess* sla =
	MLM().getSharedLibAccess( step_.sharedLibFileName() );
    if ( !sla ) return false;

    const BufferString initfnm = getFnm( step_.sharedLibFileName(), true );
    initfn ifn = (initfn)sla->getFunction( initfnm.buf() );
    if ( !ifn )
	mErrRet( BufferString("Cannot find function ",initfnm) );

    if ( !(*ifn)() )
	mErrRet( "Cannot initialize shared library" );

    const char* odfnm = "mlfOd";
    odfn fn = (odfn)sla->getFunction( odfnm );
    if ( !fn )
	mErrRet( BufferString("Cannot find function ",odfnm) );

    (*fn)( 1, &mxarrout, mxarrin );

    mxArrayCopier mxarrcopier( *mxarrout, output_ );
    mxarrcopier.execute();

    const BufferString termfnm = getFnm( step_.sharedLibFileName(), false );
    termfn tfn = (termfn)sla->getFunction( termfnm.buf() );
    (*tfn)();

    mclTerminateApplication();

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
