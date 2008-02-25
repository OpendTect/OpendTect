/*+
 *CopyRight:	(C) dGB Beheer B.V.
 *Author:	K. Tingdahl
 *Date:		Feb 2008
-*/

static const char* rcsID = "$Id: volprocsmoother.cc,v 1.1 2008-02-25 19:14:54 cvskris Exp $";

#include "volprocsmoother.h"

#include "smoother3d.h"


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
    setKernel( BoxWindow::sName(), 0, 3, 3, 3 );
}


Smoother::~Smoother()
{
    delete smoother_;
}    


Step*  Smoother::create( Chain& pc )
{ return new Smoother( pc ); }


bool Smoother::setKernel( const char* nm, float param,
			  int sz0, int sz1, int sz2 )
{
    return smoother_->setWindow( nm, param, sz0, sz1, sz2 );
}


void Smoother::fillPar( IOPar& pars ) const
{
    smoother_->fillPar( pars );
}


bool Smoother::usePar( const IOPar& pars )
{
    return smoother_->usePar( pars );
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
