/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : November 2007
-*/

static const char* rcsID = "$Id: volprocbodyfiller.cc,v 1.1 2009-09-18 18:13:43 cvskris Exp $";

#include "volprocbodyfiller.h"

#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "iopar.h"
#include "embody.h"
#include "separstr.h"

namespace VolProc
{


void BodyFiller::initClass()
{
    SeparString keys( BodyFiller::sKeyType(), VolProc::PS().cSeparator() );
    keys += BodyFiller::sKeyOldType();

    VolProc::PS().addCreator( create, keys, BodyFiller::sUserName() );
}    


Step* BodyFiller::create( Chain& pc )
{ 
    return new VolProc::BodyFiller( pc ); 
}


BodyFiller::BodyFiller( Chain& pc )
    : Step( pc )
    , body_( 0 )  
    , emobj_( 0 )
    , implicitbody_( 0 )
    , insideval_( mUdf(float) )
    , outsideval_( mUdf(float) )
{}


BodyFiller::~BodyFiller()
{
    if ( emobj_ ) emobj_->unRef();    
    delete implicitbody_;
}


bool BodyFiller::setSurface( const MultiID& mid )
{
    if ( emobj_ )
    {
	emobj_->unRef();
	body_ = 0;
	emobj_ = 0;
    }

    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<Executor> loader = EM::EMM().objectLoader( mid );
	if ( !loader || !loader->execute() )
	    return false;
	
	emid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( emid );
    }
    
    mDynamicCastGet( EM::Body*, newsurf, emobj.ptr() );
    if ( !newsurf ) return false;
    
    emobj->ref();
   
    mid_ = mid;
    body_ = newsurf;
    emobj_ = emobj;
    emobj = 0;

    delete implicitbody_;
    implicitbody_ = 0;

    return true;
}


void BodyFiller::setInsideOutsideValue( const float in, const float out )
{
    insideval_ = in;
    outsideval_ = out;
}


bool BodyFiller::computeBinID( const BinID& bid, int )
{
    //ToDo: Interpolate values from Implicit version of Surface(array).
    if ( !output_ || !output_->nrCubes() || !implicitbody_ )
	return false;
   
    const int outputinlidx = output_->inlsampling.nearestIndex( bid.inl );
    const int outputcrlidx = output_->crlsampling.nearestIndex( bid.crl );
    const int outputzsz = output_->getZSz();

    const int inputinlidx = input_ 
	? input_->inlsampling.nearestIndex(bid.inl) : 0;
    const int inputcrlidx = input_
	? input_->crlsampling.nearestIndex(bid.crl) : 0;
   
    const int bodyinlidx = implicitbody_->inlsampling_.nearestIndex( bid.inl );
    const int bodycrlidx = implicitbody_->crlsampling_.nearestIndex( bid.crl );

    const bool alloutside =
	bodyinlidx<0 || bodyinlidx>=implicitbody_->arr_->info().getSize(0) ||
	bodycrlidx<0 || bodycrlidx>=implicitbody_->arr_->info().getSize(1);

    for ( int idx=0; idx<outputzsz; idx++ )
    {
	float val;
	if ( !alloutside )
	{
	    const float z = (output_->z0+idx)*output_->zstep;
	    const int bodyzidx = implicitbody_->zsampling_.nearestIndex( z );
	    if ( bodyzidx<0 || bodyzidx>=implicitbody_->arr_->info().getSize(2))
		val = outsideval_;
	    else
	    {
		const float bodyval =
		    implicitbody_->arr_->get( bodyinlidx, bodycrlidx, bodyzidx);

		if ( mIsUdf(bodyval) )
		    val = mUdf(float);
		else
		    val = bodyval>implicitbody_->threshold_
			? insideval_ : outsideval_;
	    }
	}
	else
	    val = outsideval_;
	   
	if ( mIsUdf(val) && input_ && input_->nrCubes() )
	{
	    const int inputidx = idx+output_->z0-input_->z0;
	    if ( input_->getCube(0).info().
		    validPos(inputinlidx, inputcrlidx, inputidx) )
	    {
		val = input_->getCube(0).get(inputinlidx,inputcrlidx,inputidx);
	    }
	}

	output_->setValue( 0, outputinlidx, outputcrlidx, idx, val );
    }

    return true;
}


bool BodyFiller::needsInput( const HorSampling& ) const
{ return true; }


void BodyFiller::setOutput( Attrib::DataCubes* ni )
{
    if ( output_ ) 
	output_->unRef();

    output_ = ni;
    if ( output_ ) output_->ref();
}


void BodyFiller::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    par.set( sKeyMultiID(), mid_ );
    par.set( sKeyInsideOutsideValue(), insideval_, outsideval_ );
    par.setYN( sKeyEnabled(), enabled_ );
}


bool BodyFiller::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    MultiID mid;
    if ( (!par.get(sKeyMultiID(), mid) && !par.get(sKeyOldMultiID(), mid ) ) ||
          !setSurface( mid ) )
	return false;

    float inv, outv;
    if ( par.get(sKeyInsideOutsideValue(), inv, outv ) )
	setInsideOutsideValue( inv, outv);

    if ( !par.getYN(sKeyEnabled(), enabled_) )
	return false;

    return true;
}


Task* BodyFiller::createTask()
{
    if ( !output_ || !body_ )
	return 0;

    delete implicitbody_;
    implicitbody_ = body_->createImplicitBody( 0 );

    if ( !implicitbody_ )
	return 0;

    return Step::createTask();
}


}; //namespace
