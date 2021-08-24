/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	K. Tingdahl
 *Date:		Feb 2008
-*/


#include "volprocsmoother.h"

#include "smoother3d.h"
#include "keystrs.h"
#include "seisdatapack.h"
#include "survinfo.h"


namespace VolProc
{

Smoother::Smoother()
    : smoother_( new Smoother3D<float> )
{
    setOperator( BoxWindow::sName(), 0, 3, 3, 3 );
    setStepouts();
}


Smoother::~Smoother()
{
    releaseData();
}


void Smoother::releaseData()
{
    Step::releaseData();
    delete smoother_;
    smoother_ = 0;
}


TrcKeySampling Smoother::getInputHRg( const TrcKeySampling& hrg ) const
{ return Step::getInputHRg( hrg ); }

StepInterval<int> Smoother::getInputZRg( const StepInterval<int>& zrg ) const
{ return Step::getInputZRg( zrg ); }

StepInterval<int> Smoother::getInputZRgWithGeom( const StepInterval<int>& zrg,
					 Survey::Geometry::ID geomid ) const
{ return Step::getInputZRgWithGeom( zrg, geomid  ); }


bool Smoother::setOperator( const char* nm, float param,
			  int sz0, int sz1, int sz2 )
{
    const bool res = smoother_->setWindow( nm, param, sz0, sz1, sz2 );
    setStepouts();
    return res;
}


int Smoother::inlSz() const
{ return smoother_->getWindowSize( 0 ); }


int Smoother::crlSz() const
{ return smoother_->getWindowSize( 1 ); }


int Smoother::zSz() const
{ return smoother_->getWindowSize( 2 ); }


void Smoother::setStepouts()
{
    setHStep( BinID( inlSz()/2, crlSz()/2 ) );
    setVStep( zSz()/2 );
}


const char* Smoother::getOperatorName() const
{ return smoother_->getWindowName(); }


float Smoother::getOperatorParam() const
{ return smoother_->getWindowParam(); }


void Smoother::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );
    pars.set( smoother_->sKeyWinFunc(), getOperatorName() );
    pars.set( smoother_->sKeyWinParam(), getOperatorParam() );
    pars.set( sKey::StepOutInl(), inlSz()*SI().inlStep()*SI().inlDistance()/2 );
    pars.set( sKey::StepOutCrl(), crlSz()*SI().crlStep()*SI().crlDistance()/2 );
    pars.set( sKeyZStepout(), zSz()*SI().zStep()/2 );
}


bool Smoother::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    BufferString opname;
    float winparam=0.0f, inlstepout=0.0f, crlstepout=0.0f, zstepout=0.0f;

    if ( !pars.get( smoother_->sKeyWinFunc(), opname ) ||
	 !pars.get( smoother_->sKeyWinParam(), winparam ) ||
	 !pars.get( sKey::StepOutInl(), inlstepout ) ||
	 !pars.get( sKey::StepOutCrl(), crlstepout ) ||
	 !pars.get( sKeyZStepout(), zstepout ) )
    {
	return false;
    }

    return setOperator( opname.buf(), winparam,
	    mNINT32( inlstepout*2/SI().inlStep()/SI().inlDistance() ),
	    mNINT32( crlstepout*2/SI().crlStep()/SI().crlDistance() ),
	    mNINT32( zstepout*2/SI().zStep()) );
}


Task* Smoother::createTask()
{
    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !input || !output )
	return 0;

    Smoother3D<float>* task = new Smoother3D<float>( *smoother_ );
    task->setInput( input->data(0) );
    task->setOutput( output->data(0) );

    return task;
}


od_int64 Smoother::extraMemoryUsage( OutputSlotID, const TrcKeySampling& hsamp,
				     const StepInterval<int>& ) const
{
    return getComponentMemory( hsamp, false );
}


} // namespace VolProc
