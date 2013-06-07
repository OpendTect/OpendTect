/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : January 2009
-*/

static const char* rcsID = "$Id$";

#include "visrandomposbodydisplay.h"

#include "emmanager.h"
#include "emrandomposbody.h"
#include "executor.h"
#include "iopar.h"
#include "randcolor.h"
#include "vismaterial.h"
#include "visrandompos2body.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::RandomPosBodyDisplay );

namespace visSurvey
{

RandomPosBodyDisplay::RandomPosBodyDisplay()
    : VisualObjectImpl(true)
    , embody_( 0 )
    , displaybody_( 0 )
    , transform_( 0 )		       
{
    getMaterial()->setAmbience( 0.5 );
    setColor( getRandomColor( false ) );
}


RandomPosBodyDisplay::~RandomPosBodyDisplay()
{
    if ( transform_ ) transform_->unRef();
    if ( embody_ ) embody_->unRef();
    if ( displaybody_ ) 
    {
	removeChild( displaybody_->getInventorNode() );
	displaybody_->unRef();
    }
}


bool RandomPosBodyDisplay::setVisBody( visBase::RandomPos2Body* body )
{
    if ( !body ) return false;

    if ( displaybody_ )
    {
	removeChild( displaybody_->getInventorNode() );
	displaybody_->unRef();
	displaybody_ = 0;
    }
	
    if ( embody_ ) 
    {
	embody_->unRef();
    	embody_ = 0;
    }

    mTryAlloc( embody_, EM::RandomPosBody(EM::EMM()) );
    if ( !embody_ ) return false;

    embody_->ref();
    if ( !embody_->isOK() )
    {
	embody_->unRef();
	embody_ = 0;
	return false;
    }

    embody_->setPositions( body->getPoints() );
    EM::EMM().addObject( embody_ );

    displaybody_ = body;
    displaybody_->ref();
    displaybody_->setDisplayTransformation( transform_ );
    displaybody_->setRightHandSystem( true );
    displaybody_->setSelectable( false );
    addChild( displaybody_->getInventorNode() );

    if ( displaybody_->getMaterial() )
	getMaterial()->setFrom( *displaybody_->getMaterial() );

    displaybody_->setMaterial( 0 );
    embody_->setPreferredColor( getMaterial()->getColor() );
    
    return true;
}


EM::ObjectID RandomPosBodyDisplay::getEMID() const
{ return embody_ ? embody_->id() : -1; }


bool RandomPosBodyDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( embody_ )
    {
	embody_->unRef();
    	embody_ = 0;
    }

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::RandomPosBody*, embody, emobject.ptr() );
    if ( !embody )
    {
	if ( displaybody_ ) displaybody_->turnOn( false );
	return false;
    }

    embody_ = embody;
    embody_->ref();

    updateVisFromEM();
    return true;
}


void RandomPosBodyDisplay::updateVisFromEM()
{
    if ( !embody_ ) return;
    getMaterial()->setColor( embody_->preferredColor() );
    if ( !embody_->name().isEmpty() )
	setName( embody_->name() );
    else setName( "<New body>" );

    if ( !displaybody_ )
    {
	displaybody_ = visBase::RandomPos2Body::create();
	displaybody_->ref();
	displaybody_->setDisplayTransformation( transform_ );
	displaybody_->setRightHandSystem( true );
	displaybody_->removeSwitch();
	displaybody_->setMaterial( 0 );
	displaybody_->setSelectable( false );
	addChild( displaybody_->getInventorNode() );
    }

    displaybody_->setPoints( embody_->getPositions() );
    displaybody_->turnOn( true );
}


MultiID RandomPosBodyDisplay::getMultiID() const
{
    return embody_ ? embody_->multiID() : MultiID();
}


void RandomPosBodyDisplay::setColor( Color nc )
{
    if ( embody_ ) embody_->setPreferredColor(nc);
    getMaterial()->setColor( nc );
}


Color RandomPosBodyDisplay::getColor() const
{ return getMaterial()->getColor(); }


NotifierAccess* RandomPosBodyDisplay::materialChange()
{ return &getMaterial()->change; }


void RandomPosBodyDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    par.set( sKeyPSEarthModelID(), getMultiID() );
}


int RandomPosBodyDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newmid;
    if ( par.get(sKeyPSEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }

    return 1;
}


void RandomPosBodyDisplay::setDisplayTransformation(const mVisTrans* nt)
{
    if ( transform_ ) transform_->unRef();
    transform_ = nt;
    transform_->ref();

    if ( displaybody_ ) displaybody_->setDisplayTransformation( nt );
}


void RandomPosBodyDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( displaybody_ ) displaybody_->setRightHandSystem( yn );
}


const mVisTrans* RandomPosBodyDisplay::getDisplayTransformation() const
{ return transform_; }


}; // namespace visSurvey
