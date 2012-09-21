/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	K. Tingdahl
 *Date:		Feb 2008
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "volprocsmoother.h"

#include "smoother3d.h"
#include "keystrs.h"
#include "survinfo.h"


namespace VolProc
{

Smoother::Smoother()
    : smoother_( new Smoother3D<float> )
{
    setOperator( BoxWindow::sName(), 0, 3, 3, 3 );
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


HorSampling Smoother::getInputHRg( const HorSampling& hrg ) const
{
    HorSampling res = hrg;
    const int inlstepout = smoother_->getWindowSize( 0 ) / 2;
    const int crlstepout = smoother_->getWindowSize( 1 ) / 2;

    res.start.inl = hrg.start.inl - res.step.inl * inlstepout;
    res.start.crl = hrg.start.crl - res.step.crl * crlstepout;
    res.stop.inl = hrg.stop.inl + res.step.inl * inlstepout;
    res.stop.crl = hrg.stop.crl + res.step.crl * crlstepout;
    return res;
}


StepInterval<int> Smoother::getInputZRg( const StepInterval<int>& inrg ) const
{
    StepInterval<int> res = inrg;
    const int zstepout =  smoother_->getWindowSize( 2 ) / 2;
    res.start -= res.step*zstepout;
    res.stop += res.step*zstepout;

    return res;
}


bool Smoother::setOperator( const char* nm, float param,
			  int sz0, int sz1, int sz2 )
{
    return smoother_->setWindow( nm, param, sz0, sz1, sz2 );
}


int Smoother::inlSz() const
{ return smoother_->getWindowSize( 0 ); }


int Smoother::crlSz() const
{ return smoother_->getWindowSize( 1 ); }


int Smoother::zSz() const
{ return smoother_->getWindowSize( 2 ); }


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
    float winparam, inlstepout, crlstepout, zstepout;

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
    if ( !input_ || !output_ )
	return 0;

    Smoother3D<float>* task = new Smoother3D<float>( *smoother_ );
    task->setInput( input_->getCube( 0 ) );
    task->setOutput( output_->getCube( 0 ) );

    return task;
}


}; //namespace
