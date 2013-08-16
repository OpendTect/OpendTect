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
#include "task.h"

#include "matlablibmgr.h"

#ifdef HAS_MATLAB

# include "matlabarray.h"

extern "C" {
    // mlfOd_doprocess
    typedef void (*odfn)(int,mxArray**,mxArray*,mxArray*);
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
    , mla_(0)
{
}

    bool	init();
    od_int64	nrIterations() const	{ return 1; }
    bool	doWork(od_int64,od_int64,int);
    const char*	message() const		{ return message_.buf(); }

protected:

    const Array3D<float>&	input_;
    Array3D<float>&		output_;
    MatlabStep&			step_;
    BufferString		message_;

    MatlabLibAccess*		mla_;
};


#ifdef HAS_MATLAB

static mxArray* createParameterArray( const BufferStringSet& names,
				      const BufferStringSet& values )
{
    const int nrfields = names.size();
    char** fieldnames = new char*[nrfields];
    for ( int idx=0; idx<nrfields; idx++ )
    {
	fieldnames[idx] = new char[80];
	strcpy( fieldnames[idx], names.get(idx) );
    }

    mxArray* parsarr = mxCreateStructMatrix( 1, 1, nrfields,
			const_cast<const char**>(fieldnames) );
    if ( !parsarr )
	return 0;

    for ( int idx=0; idx<nrfields; idx++ )
    {
	const double val = toDouble( values.get(idx) );
	mxArray* valuearr = mxCreateDoubleScalar( val );
	mxSetFieldByNumber( parsarr, 0, idx, valuearr );
    }

    return parsarr;
}


#define mErrRet(msg) { message_ = msg; return false; }

bool MatlabTask::init()
{
    if ( !MLM().isOK() )
	mErrRet( MLM().errMsg() );

    mla_ = MLM().getMatlabLibAccess( step_.sharedLibFileName(), true );
    if ( !mla_ )
	mErrRet( MLM().errMsg() );

    return true;
}


bool MatlabTask::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !mla_ ) return false;

    const char* odfnm = "mlfOd_doprocess";
    odfn fn = (odfn)mla_->getFunction( odfnm );
    if ( !fn )
	mErrRet( mla_->errMsg() );

    BufferStringSet names, values;
    step_.getParameters( names, values );
    mxArray* pars = createParameterArray( names, values );

    ArrayNDCopier arrndcopier( input_ );
    arrndcopier.init( true );
    arrndcopier.execute();
    mxArray* mxarrin = arrndcopier.getMxArray();

    // for multiple inputs:
    // int dims[1]; dims[0] = nrinputs;
    // mxArray* mxarrin = mxCreateCellArray( 1,  dims );
    mxArray* mxarrout = 0;
    (*fn)( 1, &mxarrout, pars, mxarrin );

    if ( !mxarrout )
	mErrRet( "No MATLAB output generated" );

    mxArrayCopier mxarrcopier( *mxarrout, output_ );
    mxarrcopier.init();
    mxarrcopier.execute();

    return true;
}


#else

bool MatlabTask::init()				{ return false; }
bool MatlabTask::doWork(od_int64,od_int64,int)	{ return false; }

#endif


static const char* sKeyParNames()	{ return "Parameter Names"; }
static const char* sKeyParValues()	{ return "Parameter Values"; }

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

    MatlabTask* task =
	new MatlabTask( *this, input_->getCube(0), output_->getCube(0) );
    if ( !task->init() )
    {
	errmsg_ = task->message();
	delete task;
	task = 0;
    }

    return task;
}


void MatlabStep::setSharedLibFileName( const char* fnm )
{ sharedlibfnm_ = fnm; }

const char* MatlabStep::sharedLibFileName() const
{ return sharedlibfnm_; }


void MatlabStep::setParameters( const BufferStringSet& nms,
				const BufferStringSet& vals )
{ parnames_ = nms; parvalues_ = vals; }

void MatlabStep::getParameters( BufferStringSet& nms,
				BufferStringSet& vals ) const
{ nms = parnames_; vals = parvalues_; }


void MatlabStep::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    par.set( sKey::FileName(), sharedLibFileName() );
    par.set( sKeyParNames(), parnames_ );
    par.set( sKeyParValues(), parvalues_ );
}


bool MatlabStep::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    par.get( sKey::FileName(), sharedlibfnm_ );
    par.get( sKeyParNames(), parnames_ );
    par.get( sKeyParValues(), parvalues_ );

    return true;
}

} // namespace VolProc
