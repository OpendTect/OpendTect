/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Nanne Hemstra
 DATE:		February 2013
_______________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "matlabstep.h"

#include "iopar.h"
#include "task.h"

namespace VolProc
{

class MatlabTask : public ParallelTask
{
public:
MatlabTask( MatlabStep& step, const Array3D<float>& input,
	    			Array3D<float>& output )
	: input_( input )
	, output_( output )
	, step_(step)
{
}

    od_int64	nrIterations() const { return input_.info().getTotalSz(); }
    int		maxNrThreads() const { return 1; } //Todo: remove
    bool	doWork(od_int64,od_int64,int);

protected:

    const Array3D<float>&	input_;
    Array3D<float>&		output_;
    MatlabStep&			step_;
};


bool MatlabTask::doWork( od_int64 start, od_int64 stop, int )
{
    return true;
}


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


void MatlabStep::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
}


bool MatlabStep::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    return true;
}

} // namespace VolProc
