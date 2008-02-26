/*+
 *CopyRight:	(C) dGB Beheer B.V.
 *Author:	K. Tingdahl
 *Date:		Feb 2008
-*/

static const char* rcsID = "$Id: volprocsmoother.cc,v 1.2 2008-02-26 23:01:57 cvskris Exp $";

#include "volprocsmoother.h"

#include "smoother3d.h"
#include "keystrs.h"
#include "survinfo.h"


namespace VolProc
{

void Smoother::initClass()
{
    VolProc::PS().addCreator( create, Smoother::sKeyType(),
	    		      "Smoother");
}
    
    
Smoother::Smoother(Chain& pc)
    : Step( pc )
    , smoother_( new Smoother3D<float> )
{
    setOperator( BoxWindow::sName(), 0, 3, 3, 3 );
}


Smoother::~Smoother()
{
    delete smoother_;
}    


Step*  Smoother::create( Chain& pc )
{ return new Smoother( pc ); }


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
    pars.set( smoother_->sKeyWinFunc(), getOperatorName() );
    pars.set( smoother_->sKeyWinParam(), getOperatorParam() );
    pars.set( sKey::StepOutInl, inlSz()*SI().inlStep()*SI().inlDistance()/2 );
    pars.set( sKey::StepOutCrl, crlSz()*SI().crlStep()*SI().crlDistance()/2 );
    pars.set( sKeyZStepout(), zSz()*SI().zStep()/2 );
}


bool Smoother::usePar( const IOPar& pars )
{
    BufferString opname;
    float winparam, inlstepout, crlstepout, zstepout;

    if ( !pars.get( smoother_->sKeyWinFunc(), opname ) ||
	 !pars.get( smoother_->sKeyWinParam(), winparam ) ||
	 !pars.get( sKey::StepOutInl, inlstepout ) ||
	 !pars.get( sKey::StepOutCrl, crlstepout ) ||
	 !pars.get( sKeyZStepout(), zstepout ) )
    {
	return false;
    }

    return setOperator( opname.buf(), winparam,
	    mNINT( inlstepout*2/SI().inlStep()/SI().inlDistance() ),
	    mNINT( crlstepout*2/SI().crlStep()/SI().crlDistance() ),
	    mNINT( zstepout*2/SI().zStep()) );
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
